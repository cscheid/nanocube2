#ifndef BASECUBE_H_
#define BASECUBE_H_

#include <vector>
#include <algorithm>
#include <boost/assert.hpp>
#include "refcounted_vec.h"
#include "utils.h"

namespace nc2 {

template <typename Summary>
struct BaseCube
{
 public:
  NCNodePointerType base_root_;
  std::vector<NCDim> dims_;
  RefCountedVec<Summary> summaries_;

  BaseCube(const std::vector<size_t> &widths) {
    BOOST_ASSERT(widths.size() > 0);
    base_root_ = -1;
    for (auto &e: widths) {
      dims_.push_back(NCDim(e));
    }
  }

  inline size_t make_node_ref(NCNodePointerType node_index, size_t dim) {
    TRACE_BLOCK(rang::fg::blue << "make_node_ref " << rang::style::reset
                << dim << " " << node_index);
    if (node_index == -1) {
      return 0;
    }
    if (dim == dims_.size()) {
      return summaries_.make_ref(node_index);
    }
    return dims_[dim].nodes.make_ref(node_index);
  }

  void clean_node(NCNodePointerType node_index, size_t dim) {
    BOOST_ASSERT(dims_[dim].nodes.ref_counts[node_index] == 0);
    TRACE_BLOCK("clean_node " << dim << " " << node_index);
    // recurse
    NCNodePointerType l = dims_[dim].nodes[node_index].left_,
                      r = dims_[dim].nodes[node_index].right_,
                      n = dims_[dim].nodes[node_index].next_;
    release_node_ref(l, dim);
    release_node_ref(r, dim);
    release_node_ref(n, dim+1);
    dims_[dim].nodes[node_index].left_ =
        dims_[dim].nodes[node_index].right_ =
        dims_[dim].nodes[node_index].next_ = -1;
  }

  void release_node_ref(NCNodePointerType node_index, size_t dim) {
    TRACE_BLOCK(rang::fg::blue << "release_node_ref " << rang::style::reset
                << dim << " " << node_index);
    if (node_index == -1)
      return;
    if (dim == dims_.size()) {
      summaries_.release_ref(node_index);
      return;
    }
    TRACE(dims_[dim].nodes[node_index]);
    size_t new_ref_count = dims_[dim].nodes.release_ref(node_index);
    if (new_ref_count == 0) {
      clean_node(node_index, dim);
    } else {
      TRACE_BLOCK("new ref count: " << new_ref_count);
    }
  }

  inline NCNodePointerType
  add_node(size_t dim,
           NCNodePointerType l, NCNodePointerType r, NCNodePointerType n)
  {
    make_node_ref(l, dim);
    make_node_ref(r, dim);
    make_node_ref(n, dim+1);
    size_t new_index = dims_[dim].nodes.insert(NCDimNode(l, r, n));
    TRACE_BLOCK(rang::fg::red << "add node" << rang::style::reset
                << " dim:" << dim << " index:" << new_index << " value: " << l << ":" << r << ":" << n);
    return new_index;
  }

  inline void set_node(
      size_t dim, size_t index,
      NCNodePointerType l, NCNodePointerType r, NCNodePointerType n)
  {
    TRACE_BLOCK(rang::fg::red << "set node" << rang::style::reset
                << " dim:" << dim << " index:" << index << " value: " << dims_[dim].nodes[index]);
    make_node_ref(l, dim);
    make_node_ref(r, dim);
    make_node_ref(n, dim+1);
    release_node_ref(dims_[dim].nodes[index].left_, dim);
    release_node_ref(dims_[dim].nodes[index].right_, dim);
    release_node_ref(dims_[dim].nodes[index].next_, dim+1);
    dims_[dim].nodes[index].left_ = l;
    dims_[dim].nodes[index].right_ = r;
    dims_[dim].nodes[index].next_ = n;
  }

  NCNodePointerType
  insert_fresh_node(const Summary &summary, const std::vector<size_t> &addresses,
                    size_t dim, size_t bit)
  {
    TRACE_BLOCK("insert_fresh_node");
    size_t summary_index = summaries_.insert(summary);
    NCNodePointerType new_summary_ref = NCNodePointerType(summary_index);
    NCNodePointerType next_node = new_summary_ref;
    
    for (int d = int(dims_.size())-1; d >= int(dim); d--) {
      TRACE(d);
      TRACE(addresses[d]);
      size_t width = dims_[d].width;

      // on first dimension, only create as many as needed to get to the bottom
      size_t lo = dim == d ? bit : 0;

      NCNodePointerType refine_node = add_node(d, -1, -1, next_node);
      for (int b = width-1; b >= int(lo); --b) {
        TRACE(b);
        TRACE(width - b - 1);
        bool right = path_refines_towards_right(addresses[d], b, width); // width - b - 1);
        TRACE(right);
        TRACE(refine_node);
        refine_node = add_node(d,
                               !right ? refine_node : -1,
                               right ? refine_node : -1,
                               next_node);
      }
      next_node = refine_node;
    }
    return next_node;
  }
  
  NCNodePointerType merge(
      size_t dim, NCNodePointerType index_1, NCNodePointerType index_2)
  {
    TRACE_BLOCK("merge: " << dim << ", " << index_1 << ", " << index_2);
    if (dim == this->dims_.size()) {
      TRACE_BLOCK("merge: summaries");
      const Summary &summ_1 = this->summaries_[index_1],
                    &summ_2 = this->summaries_[index_2];
      return this->summaries_.insert(summ_1 + summ_2);
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

    const NCDimNode &node_1 = this->dims_[dim].nodes[index_1],
                    &node_2 = this->dims_[dim].nodes[index_2];

    NCNodePointerType l1 = node_1.left_,  l2 = node_2.left_,
                      r1 = node_1.right_, r2 = node_2.right_,
                      n1 = node_1.next_,  n2 = node_2.next_;
    
    // The other source of sharing comes from when the result
    // of a merge is a singleton node. In that case, we can
    // safely share the next edge with the result.
    if (l1 == l2 && r1 == r2 && l1 == -1 && r1 == -1) {
      NCNodePointerType new_next = merge(dim+1, n1, n2);
      const NCDimNode &new_next_node = this->dims_[dim].nodes[new_next];
      return this->add_node(dim, -1, -1, new_next);
    }
    if (l1 == l2 && l1 == -1) {
      TRACE_BLOCK("merge only right");
      NCNodePointerType new_right = merge(dim, r1, r2);
      const NCDimNode &new_right_node = this->dims_[dim].nodes[new_right];
      TRACE(new_right);
      TRACE(new_right_node);
      return this->add_node(dim, -1, new_right, new_right_node.next_);
    }
    if (r1 == r2 && r1 == -1) {
      TRACE_BLOCK("merge only left");
      NCNodePointerType new_left = merge(dim, l1, l2);
      const NCDimNode &new_left_node = this->dims_[dim].nodes[new_left];
      TRACE(new_left);
      TRACE(new_left_node);
      return this->add_node(dim, new_left, -1, new_left_node.next_);
    }

    {
      TRACE_BLOCK("merge: complex, recursing");

      return this->add_node(dim,
                            merge(dim, l1, l2),
                            merge(dim, r1, r2),
                            merge(dim+1, n1, n2));
    }
  }

  template <typename SummaryPolicy>
  void range_query(
      SummaryPolicy &policy,
      const std::vector<std::pair<size_t, size_t> > &bounds) const;
  
  template <typename SummaryPolicy>
  void range_query_internal
  (SummaryPolicy &policy,
   const std::vector<std::pair<size_t, size_t> > &bounds,
   NCNodePointerType current_node, size_t dim,
   size_t node_left, size_t node_right) const;


  void print_dot_ncdim(std::ostream &os, const NCDim &dim,
                       size_t d, bool draw_next, bool draw_garbage) {
    os << " subgraph cluster_" << d << " {\n";
    os << " label=\"Dim. " << d << "\";\n";
    for (int i=0; i<dim.nodes.size(); ++i) {
      if (!draw_garbage && dim.nodes.ref_counts[i] == 0)
        continue;
      os << "  \"" << i << "_" << d << "\" [label=\"" << i << ":";
      if (dim.nodes[i].next_ == -1)
        os << "null";
      else
        os << dim.nodes[i].next_;
      os << "\"];\n";
    }
    for (int i=0; i<dim.nodes.size(); ++i) {
      if (!draw_garbage && dim.nodes.ref_counts[i] == 0)
        continue;
      const NCDimNode &node = dim.nodes[i];
      if (node.left_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.left_ << "_" << d << "\" [label=\"0\"];\n";
      if (node.right_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.right_ << "_" << d << "\" [label=\"1\"];\n";
    }
    os << "}\n";
  }
  
  void print_dot(std::ostream &os, bool draw_garbage=false) {
    os << "digraph G {\n";
    os << "  splines=line;\n";
    for (size_t i=0; i<dims_.size(); ++i) {
      const NCDim &dim = dims_[i];
      print_dot_ncdim(os, dim, i, i < (dims_.size() - 1), draw_garbage);
    }
    os << "}\n";
  };

  void dump_nc(bool show_garbage=false) {
    for (int i=0; i<dims_.size(); ++i) {
      std::cerr << "Dim " << i << ":\n";
      const NCDim &dim = dims_[i];
      for (int j=0; j<dim.size(); ++j) {
        if (!show_garbage && dim.nodes.ref_counts[j] == 0)
          continue;
        std::cerr << "  " << j << ": " << dim[j] << " (" << dim.nodes.ref_counts[j] << ")" << std::endl;
      }
      std::cerr << "\n";
    }
    std::cerr << "Summaries:\n";
    for (int i=0; i<summaries_.size(); ++i) {
      if (!show_garbage && summaries_.ref_counts[i] == 0)
        continue;
      std::cerr << "  " << i << ": " << summaries_[i] << " (" << summaries_.ref_counts[i] << ")" << std::endl;
    }
  }
};
template <typename Summary>
template <typename SummaryPolicy>
void BaseCube<Summary>::range_query_internal(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds,
    NCNodePointerType current_node_index, size_t dim,
    size_t node_left, size_t node_right) const
{
  if (current_node_index == -1) {
    return;
  }
  const NCDimNode &current_node = dims_[dim].nodes[current_node_index];
  
  size_t query_left = bounds[dim].first,
        query_right = bounds[dim].second;
  TRACE(current_node);
  TRACE(node_left);
  TRACE(node_right);
  TRACE(query_left);
  TRACE(query_right);

  if (query_right <= node_left || query_left >= node_right) {
    // provably empty ranges, no nodes here
    return;
  } else if (node_left >= query_left && node_right <= query_right) {
    // node is entirely inside query, recurse to next dim (or report summary)
    // when in last dimension, report summary instead of recursing
    if (dim+1 == dims_.size()) {
      policy.add(summaries_[current_node.next_]);
    } else {
      range_query_internal(
          policy,
          bounds, current_node.next_, dim+1,
          0, 1 << dims_[dim+1].width);
    }
  } else {
    // range partially overlaps; recurse on refinement nodes
    size_t center = node_left + ((node_right - node_left) >> 1);
    range_query_internal(
        policy, bounds, current_node.left_, dim, node_left, center);
    range_query_internal(
        policy, bounds, current_node.right_, dim, center, node_right);
  }
}

template <typename Summary>
template <typename SummaryPolicy>
void BaseCube<Summary>::range_query(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds) const
{
  range_query_internal(policy, bounds,
                       base_root_, 0,
                       0, 1 << dims_[0].width);
}

};

#endif
