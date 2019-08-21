#ifndef NANOCUBE_H_
#define NANOCUBE_H_

// #define NTRACE

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <boost/assert.hpp>
#include "utils.h"
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <unordered_set>

namespace nc2 {

struct RefTrackedNCDimNode
{
  std::vector<NCNodePointerType> refs_;
  NCNodePointerType l_, r_, n_;
  // left_, right_, next_;
  
  RefTrackedNCDimNode():
      refs_(), l_(-1), r_(-1), n_(-1) {}

  RefTrackedNCDimNode(NCNodePointerType l,
                      NCNodePointerType r,
                      NCNodePointerType n):
      refs_(), l_(l), r_(r), n_(n) {}

  bool is_singleton() const {
    return (l_ == -1 && r_ != -1) || (l_ != -1 && r_ == -1);
  }

  // if you call this while !is_singleton, UB!
  NCNodePointerType singleton_child() const {
    if (l_ != -1)
      return r_;
    else
      return l_;
  }
};

struct NCDim2
{
  std::vector<RefTrackedNCDimNode> nodes_;

  size_t width;

  explicit NCDim2(size_t w):
      nodes_(),
      width(w) {}

  size_t size() const {
    return nodes_.size();
  }

  RefTrackedNCDimNode &operator[](NCNodePointerType v) {
    return nodes_[size_t(v)];
  }

  const RefTrackedNCDimNode &operator[](NCNodePointerType v) const {
    return nodes_[size_t(v)];
  }

};

std::ostream &operator<<(std::ostream &os, const RefTrackedNCDimNode &node);

template <typename Summary>
struct NanoCube
{
 public:
  NCNodePointerType base_root_;
  std::vector<NCDim2> dims_;
  std::vector<Summary> summaries_;

  // holds a cache of the constructed spine to avoid recreating it
  // at every fork.
  std::vector<std::vector<NCNodePointerType> > spine_cache_;

  void report_storage()
  {
    for (size_t i=0; i<dims_.size(); ++i) {
      std::cerr << "dim " << i << ": " << dims_[i].nodes_.size() << std::endl;
    }
    std::cerr << "summaries: " << summaries_.size() << std::endl;
  }
  
  void clear_spine()
  {
    TRACE_BLOCK("clear_spine");
    for (auto &v: spine_cache_) {
      std::fill(v.begin(), v.end(), -1);
    }
  }

  NCNodePointerType get_spine(
      const Summary &summary,
      const std::vector<size_t> &addresses,
      int dim, int bit)
  {
    TRACE_BLOCK("get_spine ( " << stream_vector(addresses) << ") " << dim << " " << bit << " " << summary);
    if (spine_cache_[dim][bit] != -1) {
      TRACE_BLOCK("hit cache");
      return spine_cache_[dim][bit];
    } else if (dim == dims_.size()) {
      TRACE_BLOCK("hit summary dim");
      BOOST_ASSERT(bit == 0);
      size_t pos = summaries_.size();
      spine_cache_[dim][0] = pos;
      summaries_.push_back(summary);
      return pos;
    }
    size_t width = dims_[dim].width;
    if (bit == width) {
      NCNodePointerType next = get_spine(summary, addresses, dim+1, 0);
      auto result = add_node(dim, -1, -1, next);
      spine_cache_[dim][bit] = result;
      return result;
    } else {
      bool right = path_refines_towards_right(addresses[dim], bit, width);
      NCNodePointerType ref = get_spine(summary, addresses, dim, bit+1);
      NCNodePointerType next = dims_[dim].nodes_[ref].n_;
      auto result = add_node(dim,
                             !right ? ref : -1,
                             right ? ref : -1,
                             next);
      spine_cache_[dim][bit] = result;
      return result;
    }
  }

  explicit NanoCube(const std::vector<size_t> &widths):
      base_root_(-1) {
    for (auto &e: widths) {
      dims_.push_back(NCDim2(e));
      spine_cache_.push_back(std::vector<NCNodePointerType>(e + 1, -1));
    }
    spine_cache_.push_back(std::vector<NCNodePointerType>(1, -1));
  }

  NanoCube(const std::vector<size_t> &widths,
           const std::vector<std::pair<std::vector<size_t>, Summary> > &values):
      NanoCube(widths)
  {
    int i=0;
    for (auto &value: values) {
      insert(value.first, value.second);
      // std::ostringstream fn;
      // fn << "nc" << ++i << ".dot";
      // dump_nc(true);
      // {
      //   std::ofstream of(fn.str().c_str());
      //   print_dot(of);
      // }
    }
  }

  void release_node_ref(NCNodePointerType node_from,
                        NCNodePointerType node_to, size_t dim) {
    TRACE_BLOCK(rang::fg::blue << "release_node_ref " << rang::style::reset
                << dim << " " << node_from << "->" << node_to);
    if (node_to == -1)
      return;
    auto &v = dims_[dim].nodes_[node_to].refs_;
    TRACE(stream_vector(v));
    v.erase(std::remove(v.begin(), v.end(), node_from), v.end());
    TRACE(stream_vector(v));
  }
  
  inline void make_node_ref(NCNodePointerType node_from,
                              NCNodePointerType node_to, size_t dim) {
    TRACE_BLOCK(rang::fg::blue << "make_node_ref " << rang::style::reset
                << dim << " " << node_from << "->" << node_to);
    if (node_from == -1) {
      return;
    }
    if (node_to == -1) {
      return;
    }
    if (dim != dims_.size()) {
      auto &refs = dims_[dim].nodes_[node_to].refs_;
      refs.push_back(node_from);
      std::sort(refs.begin(), refs.end());
      return;
    }
  }

  inline NCNodePointerType
  add_node(size_t dim,
           NCNodePointerType l, NCNodePointerType r, NCNodePointerType n)
  {
    size_t new_index = dims_[dim].size();
    dims_[dim].nodes_.push_back(RefTrackedNCDimNode(l, r, n));
    TRACE_BLOCK(rang::fg::red << "add node" << rang::style::reset
                << " dim:" << dim << " index:" << new_index << " value: " << l << ":" << r << ":" << n);
    make_node_ref(new_index, l, dim);
    make_node_ref(new_index, r, dim);
    // note that we don't actually record references on the next pointer
    return new_index;
  }

  inline void set_node(
      size_t dim, size_t index,
      NCNodePointerType l, NCNodePointerType r, NCNodePointerType n)
  {
    TRACE_BLOCK(rang::fg::red << "set node" << rang::style::reset
                << " dim:" << dim << " index:" << index << " current value: " << dims_[dim].nodes_[index]
                << " new value: " << l << ":" << r << ":" << n);
    if (dims_[dim].nodes_[index].l_ != l) {
      make_node_ref(index, l, dim);
      release_node_ref(index, dims_[dim].nodes_[index].l_, dim);
    }
    if (dims_[dim].nodes_[index].r_ != r) {
      make_node_ref(index, r, dim);
      release_node_ref(index, dims_[dim].nodes_[index].r_, dim);
    }
    // note that we don't actually record references on the next pointer
    dims_[dim].nodes_[index].l_ = l;
    dims_[dim].nodes_[index].r_ = r;
    dims_[dim].nodes_[index].n_ = n;
  }

  NCNodePointerType merge(
      size_t dim, NCNodePointerType index_1, NCNodePointerType index_2)
  {
    TRACE_BLOCK("merge: " << dim << ", " << index_1 << ", " << index_2);
    if (dim == this->dims_.size()) {
      TRACE_BLOCK("merge: summaries");
      const Summary &summ_1 = this->summaries_[index_1],
                    &summ_2 = this->summaries_[index_2];
      NCNodePointerType pos = this->summaries_.size();
      this->summaries_.push_back(summ_1 + summ_2);
      return pos;
    }

    // A merge with an empty node is the same as the original
    // node. This provides one source of sharing in a nanocube.
    if (index_1 == -1) {
      TRACE_BLOCK("merge: left refine empty, return right");
      return index_2;
    }
    if (index_2 == -1) {
      TRACE_BLOCK("merge: right refine empty, return left");
      return index_1;
    }

    const RefTrackedNCDimNode
        &node_1 = this->dims_[dim].nodes_[index_1],
        &node_2 = this->dims_[dim].nodes_[index_2];

    NCNodePointerType l1 = node_1.l_, l2 = node_2.l_,
                      r1 = node_1.r_, r2 = node_2.r_,
                      n1 = node_1.n_, n2 = node_2.n_;
    
    // The other source of sharing comes from when the result
    // of a merge is a singleton node. In that case, we can
    // safely share the next edge with the result.
    if (l1 == l2 && r1 == r2 && l1 == -1 && r1 == -1) {
      NCNodePointerType new_next = merge(dim+1, n1, n2);
      const RefTrackedNCDimNode &new_next_node = this->dims_[dim].nodes_[new_next];
      return this->add_node(dim, -1, -1, new_next);
    }
    if (l1 == l2 && l1 == -1) {
      TRACE_BLOCK("merge only right");
      NCNodePointerType new_right = merge(dim, r1, r2);
      const RefTrackedNCDimNode &new_right_node = this->dims_[dim].nodes_[new_right];
      TRACE(new_right);
      TRACE(new_right_node);
      return this->add_node(dim, -1, new_right, new_right_node.n_);
    }
    if (r1 == r2 && r1 == -1) {
      TRACE_BLOCK("merge only left");
      NCNodePointerType new_left = merge(dim, l1, l2);
      const RefTrackedNCDimNode &new_left_node = this->dims_[dim].nodes_[new_left];
      TRACE(new_left);
      TRACE(new_left_node);
      return this->add_node(dim, new_left, -1, new_left_node.n_);
    }

    {
      TRACE_BLOCK("merge: complex, recursing");

      return this->add_node(dim,
                            merge(dim, l1, l2),
                            merge(dim, r1, r2),
                            merge(dim+1, n1, n2));
    }
  }

  void fix_parents_next(size_t dim,
                        NCNodePointerType index,
                        NCNodePointerType next_node)
      // set parent's next node to be the given next node, and call
      // itself recursively.  this is called after an update that
      // added a new next node.
      // 
      // post-condition: for every node in dimension dim, if it has a
      // single refinement node, its next pointer matches that of the
      // refinement node.
  {
    TRACE_BLOCK("fix_parents_next " << dim << " " << index << " " << next_node);
    const RefTrackedNCDimNode &n = dims_[dim].nodes_[index];
    std::vector<NCNodePointerType> parents_to_fix;
    for (auto &parent_i: n.refs_) {
      const RefTrackedNCDimNode &p = dims_[dim].nodes_[parent_i];
      // don't recurse if parent has already been fixed.
      // FIXME: does this actually ever happen?
      if (p.n_ == next_node)
        continue;
      // stop recursing if parent is a fork.
      if (p.l_ != -1 && p.r_ != -1)
        continue;
      BOOST_ASSERT(p.l_ != -1 || p.r_ != -1);
      // It might appear unsafe to call
      // set_node and fix_parents_next directly, since
      // set_node potentially n.refs_, but we're only
      // ever changing the next node pointers, and those
      // are not tracked by refcounting in any case.
      TRACE_BLOCK("will call set_node");
      set_node(dim, parent_i, p.l_, p.r_, next_node);
      fix_parents_next(dim, parent_i, next_node);
    }
  }
  
  void update(const std::vector<size_t> &addresses,
              size_t dim,
              size_t bit,
              const Summary &summary,
              const std::vector<NCNodePointerType> &nodes,
              std::vector<NCNodePointerType> &forks)
  {
    TRACE_BLOCK("update " << dim << " " << bit << " " <<
                summary << " ( " << 
                stream_vector(nodes) << ") ( " <<
                stream_vector(forks) << ")");
    if (dim == dims_.size()) {
      // update reached summary dimension.
      //
      // mutate all summaries pointed to by nodes, then we're done.
      for (auto &n: nodes) {
        summaries_[n] += summary;
      }
      return;
    }
    
    size_t width = dims_[dim].width;
    if (width == bit) {
      // hit the bottom of the current dimension. forks becomes nodes,
      // create new forks array, call next dimension
      for (auto &ni: nodes) {
        const RefTrackedNCDimNode &n = dims_[dim].nodes_[ni];
        TRACE_BLOCK("adding to forks " << n.n_);
        forks.push_back(n.n_);
      }
      std::vector<NCNodePointerType> next_forks;
      
      update(addresses, dim+1, 0, summary, forks, next_forks);
      return;
    }
    
    std::map<NCNodePointerType, std::vector<NCNodePointerType> > next_node_parents;
    std::vector<NCNodePointerType> next_nodes;
    
    // mark all children with the parents it came from
    for (auto &ni: nodes) {
      const RefTrackedNCDimNode &n = dims_[dim].nodes_[ni];
      if ((n.l_ == -1 && n.r_ == -1) || (n.l_ != -1 && n.r_ != -1)) {
        TRACE_BLOCK("adding to forks " << n.n_);
        forks.push_back(n.n_);
      }
      NCNodePointerType r = path_refines_towards_right(addresses[dim], bit, width) ?
                            dims_[dim].nodes_[ni].r_ :
                            dims_[dim].nodes_[ni].l_;
      next_node_parents[r].push_back(ni);
    }
    {
      TRACE_BLOCK("next_node_parents:");
      for (auto &p: next_node_parents) {
        TRACE(p.first);
        TRACE(stream_vector(p.second));
      }
    }
    
    for (auto &p: next_node_parents) {
      bool points_right = path_refines_towards_right(addresses[dim], bit, width);
      NCNodePointerType ref = p.first;
      std::sort(p.second.begin(), p.second.end());
      if (p.first == -1) {
        // when p.first == -1, p.second records the set of nodes that
        // pointed to nil.
        // 
        // Since we don't add bottom nodes to next_node_parents, these
        // are all new forks.
        //
        // we need these to point to the newly-created spine at the
        // right position.
        //
        // Finally, note that we don't add these to next_nodes because
        // after the merge, we are done.
        TRACE_BLOCK("new forks: " << stream_vector(p.second));
        size_t spine_node = get_spine(summary, addresses, dim, bit+1);
        auto &nodes = dims_[dim].nodes_;
        for (auto &node_to_fork_id: p.second) {
          if (points_right) {
            // if path pointed right and p.first == -1, then node_to_fork_id
            // points to a node with an existing left node, or it'd be
            // the case that l == r == -1
            NCNodePointerType l = dims_[dim].nodes_[node_to_fork_id].l_; 
            NCNodePointerType merged_next = merge(
                dim+1, nodes[l].n_, nodes[spine_node].n_);
            TRACE_BLOCK("will call set_node");
            set_node(dim, node_to_fork_id, l, spine_node, merged_next);
            fix_parents_next(dim, node_to_fork_id, merged_next);
          } else {
            // conversely, if path pointed left and p.first == -1, then node_to_fork_id
            // points to a node with an existing right node, or it'd be
            // the case that l == r == -1
            NCNodePointerType r = dims_[dim].nodes_[node_to_fork_id].r_;
            NCNodePointerType merged_next = merge(
                dim+1, nodes[r].n_, nodes[spine_node].n_);
            TRACE_BLOCK("will call set_node");
            set_node(dim, node_to_fork_id, spine_node, r, merged_next);
            fix_parents_next(dim, node_to_fork_id, merged_next);
          }
        }
      } else if (p.second != dims_[dim].nodes_[ref].refs_) {
        // if the set of marked parents doesn't match the refinement refs,
        // then this means that at least one of the refinement refs
        // should not contain the summary we're trying to update.
        //
        // the refinement refs that are missing in the marked nodes
        // should not be mutated
        // 
        // action: merge the current refinement node with the spine, set all
        // marked nodes to result.
        TRACE_BLOCK("marked parents don't match refinement nodes");
        TRACE(stream_vector(p.second));
        TRACE(stream_vector(dims_[dim].nodes_[ref].refs_));
        NCNodePointerType spine_node = get_spine(summary, addresses, dim, bit+1);
        NCNodePointerType spine_next = dims_[dim].nodes_[spine_node].n_;
        NCNodePointerType merged_ref = merge(dim, ref, spine_node);

        for (auto &node_to_update: p.second) {
          auto const &n = dims_[dim].nodes_[node_to_update];
          if (points_right) {
            NCNodePointerType node_left = n.l_;
            TRACE_BLOCK("will call set_node");
            set_node(dim, node_to_update, node_left, merged_ref,
                     n.is_singleton() ? dims_[dim].nodes_[merged_ref].n_ : n.n_);
          } else {
            NCNodePointerType node_right = n.r_;
            TRACE_BLOCK("will call set_node");
            set_node(dim, node_to_update, merged_ref, node_right,
                     n.is_singleton() ? dims_[dim].nodes_[merged_ref].n_ : n.n_);
          }
        }
      } else {
        // here, p.second == dims_[dim].nodes_[ref].refs_
        // 
        // This means that all paths that can reach this node want to
        // update it. Because of that, we can safely mutate the node
        // rather than non-destructively create a merge. This is more
        // efficient.
        //
        // action: add child node to next_nodes
        next_nodes.push_back(p.first);
      }
    }

    // All left to do now is recurse on next_nodes if there are paths to follow
    if (next_nodes.size() > 0) {
      TRACE_BLOCK("updating next nodes " << stream_vector(next_nodes))
      update(addresses,
             dim,
             bit + 1,
             summary,
             next_nodes,
             forks);
    } else if (forks.size() > 0) {
      TRACE_BLOCK("out of nodes in next_nodes, but need to fix next dimension " << stream_vector(forks));
      // or recurse to next dimension if not, using the forks we found.
      std::vector<NCNodePointerType> new_forks;
      update(addresses,
             dim + 1,
             0,
             summary,
             forks,
             new_forks);
    }
  }
 
  void insert(const std::vector<size_t> &addresses, const Summary &summary)
  {
    TRACE_BLOCK("insert " << stream_vector(addresses) << ": " << summary);
    if (base_root_ == -1) {
      base_root_ = get_spine(summary, addresses, 0, 0);
    } else {
      clear_spine();
      std::vector<NCNodePointerType> forks;
      update(addresses, 0, 0, summary, { base_root_ }, forks);
    }
    
    // if (!check_health()) {
    //   {
    //     std::ofstream of("bad-state.dot");
    //     print_dot(of);
    //     dump_nc(true);
    //   }
    //   std::cerr << "data structure doesn't pass health check!" << std::endl;
    //   exit(1);
    // }
  }

  template <typename SummaryPolicy>
  void range_query(
      SummaryPolicy &policy,
      const std::vector<std::pair<size_t, size_t> > &bounds) const
  {
    range_query_internal(policy, bounds, base_root_,
                         0, 0, 1 << dims_[0].width);
  }
  
  template <typename SummaryPolicy>
  void range_query_internal
  (SummaryPolicy &policy,
   const std::vector<std::pair<size_t, size_t> > &bounds,
   NCNodePointerType current_node, size_t dim,
   size_t node_left, size_t node_right) const;

  void print_dot_ncdim(std::ostream &os, const NCDim2 &dim,
                       size_t d, bool draw_next) {
    os << " subgraph cluster_" << d << " {\n";
    os << " label=\"Dim. " << d << "\";\n";
    for (int i=0; i<dim.nodes_.size(); ++i) {
      os << "  \"" << i << "_" << d << "\" [label=\"" << i << ":";
      if (dim.nodes_[i].n_ == -1)
        os << "null";
      else
        os << dim.nodes_[i].n_;
      os << "\"];\n";
    }
    for (int i=0; i<dim.nodes_.size(); ++i) {
      const RefTrackedNCDimNode &node = dim.nodes_[i];
      if (node.l_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.l_ << "_" << d << "\" [label=\"0\"];\n";
      if (node.r_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.r_ << "_" << d << "\" [label=\"1\"];\n";
    }
    os << "}\n";
  }
  
  void print_dot(std::ostream &os) {
    os << "digraph G {\n";
    os << "  splines=line;\n";
    for (size_t i=0; i<dims_.size(); ++i) {
      const NCDim2 &dim = dims_[i];
      print_dot_ncdim(os, dim, i, i < (dims_.size() - 1));
    }
    os << "}\n";
  };

  void dump_nc(bool show_parent_links=false) {
    std::cerr << "base_root_: " << base_root_ << std::endl;
    for (int i=0; i<dims_.size(); ++i) {
      std::cerr << "Dim " << i << ":\n";
      const NCDim2 &dim = dims_[i];
      for (int j=0; j<dim.size(); ++j) {
        std::cerr << "  " << j << ": "
                  << dim[j].l_ << ":"
                  << dim[j].r_ << ":"
                  << dim[j].n_;
        if (show_parent_links) {
          std::cerr << " (";
          std::copy(dim[j].refs_.begin(), dim[j].refs_.end(),
                    std::ostream_iterator<NCNodePointerType>(std::cerr, ", "));
          std::cerr << ")";
        }
        std::cerr << "\n";
      }
      std::cerr << "\n";
    }
    std::cerr << "Summaries:\n";
    for (int i=0; i<summaries_.size(); ++i) {
      std::cerr << "  " << i << ": " << summaries_[i] << std::endl;
    }
  }

  // returns false if there exist nodes not reachable from the root
  bool check_all_nodes_reachable()
  {
    std::set<std::pair<size_t, size_t> > reached_nodes, nodes;
    
    nodes.insert(std::make_pair(0, base_root_));
    while (nodes.size()) {
      std::pair<size_t, size_t> top = *nodes.begin();
      nodes.erase(*nodes.begin());
      reached_nodes.insert(top);
      if (top.first == dims_.size())
        continue;
      const RefTrackedNCDimNode &n = dims_[top.first].nodes_[top.second];
      if (n.l_ != -1 &&
          reached_nodes.find(std::make_pair(top.first, n.l_)) == reached_nodes.end())
          nodes.insert(std::make_pair(top.first, n.l_));
      if (n.r_ != -1 &&
          reached_nodes.find(std::make_pair(top.first, n.r_)) == reached_nodes.end())
          nodes.insert(std::make_pair(top.first, n.r_));
      if (reached_nodes.find(std::make_pair(top.first + 1, n.n_)) == reached_nodes.end())
        nodes.insert(std::make_pair(top.first + 1, n.n_));
    }

    for (int d=0; d<dims_.size(); ++d) {
      for (int i=0; i<dims_[d].nodes_.size(); ++i) {
        if (reached_nodes.find(std::make_pair(d, i)) == reached_nodes.end()) {
          std::cerr << "Node " << d << " " << i << " is unreferenced!" << std::endl;
          return false;
        }
      }
    }

    for (int i=0; i<summaries_.size(); ++i) {
      if (reached_nodes.find(std::make_pair(dims_.size(), i)) == reached_nodes.end()) {
        std::cerr << "Summary " << i << " is unreferenced!" << std::endl;
        return false;
      }
    }

    WHEN_TRACING {
      std::cerr << "Nodes are all reachable." << std::endl;
    }
    return true;
  }

  bool check_ref_tracking_health()
  {
    for (int d=0; d<dims_.size(); ++d) {
      std::map<size_t, std::vector<NCNodePointerType> > back_refs;
      for (int i=0; i<dims_[d].nodes_.size(); ++i) {
        const RefTrackedNCDimNode &n = dims_[d].nodes_[i];
        back_refs[n.l_].push_back(i);
        sort(back_refs[n.l_].begin(), back_refs[n.l_].end());
        back_refs[n.r_].push_back(i);
        sort(back_refs[n.r_].begin(), back_refs[n.r_].end());
      }
      for (int i=0; i<dims_[d].nodes_.size(); ++i) {
        const RefTrackedNCDimNode &n = dims_[d].nodes_[i];
        auto &v = back_refs[i];
        if (n.refs_ != v) {
          std::cerr << "Node " << d << " " << i <<
              "has bad parent refs." << std::endl;
          return false;
        }
      }
    }
    WHEN_TRACING {
      std::cerr << "parent refs are correct." << std::endl;
    }
    return true;
  }

  bool check_singletons_next_points_to_childrens_next()
  {
    for (int d=0; d<dims_.size(); ++d) {
      for (int i=0; i<dims_[d].nodes_.size(); ++i) {
        const RefTrackedNCDimNode &n = dims_[d].nodes_[i];
        if ((n.l_ == -1 && n.r_ != -1 && n.n_ != dims_[d].nodes_[n.r_].n_) ||
            (n.l_ != -1 && n.r_ == -1 && n.n_ != dims_[d].nodes_[n.l_].n_)) {
          std::cerr << "Singleton node " << d << " " << i
                    << " has next pointer different from child." << std::endl;
        }
      }
    }
    return true;
  }
  
  bool check_health()
  {
    return check_all_nodes_reachable() &&
        check_ref_tracking_health() &&
        check_singletons_next_points_to_childrens_next();
  }

  
};


template <typename Summary>
template <typename SummaryPolicy>
void NanoCube<Summary>::range_query_internal(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds,
    NCNodePointerType current_node_index, size_t dim,
    size_t node_left, size_t node_right) const
{
  TRACE_BLOCK("range_query_internal " << current_node_index << " "
              << dim << " "
              << node_left << " "
              << node_right);
  if (current_node_index == -1) {
    TRACE_BLOCK("null, done.");
    return;
  }
  const RefTrackedNCDimNode &current_node = dims_[dim].nodes_[current_node_index];
  
  size_t query_left = bounds[dim].first,
        query_right = bounds[dim].second;
  TRACE(current_node);
  TRACE(node_left);
  TRACE(node_right);
  TRACE(query_left);
  TRACE(query_right);

  if (query_right <= node_left || query_left >= node_right) {
    // provably empty ranges, no nodes here
    TRACE_BLOCK("entirely outside, done");
    return;
  } else if (node_left >= query_left && node_right <= query_right) {
    TRACE_BLOCK("entirely inside, recursing to next dim");
    // node is entirely inside query, recurse to next dim (or report summary)
    // when in last dimension, report summary instead of recursing
    if (dim+1 == dims_.size()) {
      policy.add(summaries_[current_node.n_]);
    } else {
      range_query_internal(
          policy,
          bounds, current_node.n_, dim+1,
          0, 1 << dims_[dim+1].width);
    }
  } else {
    TRACE_BLOCK("partial overlap, recurse on refinement");
    // range partially overlaps; recurse on refinement nodes
    size_t center = node_left + ((node_right - node_left) >> 1);
    range_query_internal(
        policy, bounds, current_node.l_, dim, node_left, center);
    range_query_internal(
        policy, bounds, current_node.r_, dim, center, node_right);
  }
}

/******************************************************************************/

bool test_nanocube_1();
bool test_naivecube_and_nanocube_equivalence();
bool test_naivecube_and_nanocube_equivalence_1();
bool test_naivecube_and_nanocube_equivalence_2();
bool test_naivecube_and_nanocube_equivalence_3();
bool test_naivecube_and_nanocube_equivalence_4();

std::ostream &operator<<(std::ostream &os, const RefTrackedNCDimNode &n);

};

#endif
