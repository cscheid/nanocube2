#ifndef NANOCUBE_H_
#define NANOCUBE_H_

// #define NTRACE

#include <vector>
#include <boost/assert.hpp>
#include "utils.h"

namespace nc2 {

typedef int32_t NCNodePointerType;

struct NCDimNode {
  NCNodePointerType left_, right_, next_;
  NCDimNode(NCNodePointerType left, NCNodePointerType right, NCNodePointerType next):
      left_(left), right_(right), next_(next) {};
};

std::ostream &operator<<(std::ostream &os, const NCDimNode &node);

struct NCDim {
  std::vector<NCDimNode> nodes;
  size_t width;

  explicit NCDim(size_t w):
      nodes(),
      width(w) {}

  size_t size() const {
    return nodes.size();
  }

  NCDimNode &operator[](NCNodePointerType v) {
    return nodes[size_t(v)];
  }

  const NCDimNode &operator[](NCNodePointerType v) const {
    return nodes[size_t(v)];
  }
  
};

inline bool get_bit(size_t value, size_t bit) {
  return ((value >> bit) & 1) == 1;
}

inline bool path_refines_towards_right(size_t value, size_t bit, size_t width) {
  return get_bit(value, width - bit - 1);
}

template <typename Summary>
struct NanoCube
{
  NCNodePointerType base_root_;
  std::vector<NCDim> dims_;
  std::vector<Summary> summaries_;

  inline void add_node(size_t dim,
                       NCNodePointerType l, NCNodePointerType r, NCNodePointerType n)
  {
    TRACE_BLOCK(rang::fg::red << "add node" << rang::style::reset
                << " dim:" << dim << " index:" << dims_[dim].size() << " value: " << l << ":" << r << ":" << n);
    dims_[dim].nodes.push_back(NCDimNode(l, r, n));
  }

  inline void report_node(size_t dim, size_t index)
  {
    TRACE_BLOCK(rang::fg::red << "set node" << rang::style::reset
                << " dim:" << dim << " index:" << index << " value: " << dims_[dim].nodes[index]);
  }
  
  explicit NanoCube(const std::vector<size_t> &widths) {
    BOOST_ASSERT(widths.size() > 0);
    base_root_ = -1;
    for (auto &e: widths) {
      dims_.push_back(NCDim(e));
    }
  }

  NCNodePointerType
  insert_fresh_node(const Summary &summary, const std::vector<size_t> &addresses,
                    size_t dim, size_t bit)
  {
    TRACE_BLOCK("insert_fresh_node");
    summaries_.push_back(summary);
    NCNodePointerType new_summary_ref = NCNodePointerType(summaries_.size()-1);
    NCNodePointerType next_node = new_summary_ref;
    
    for (int d = int(dims_.size())-1; d >= int(dim); d--) {
      TRACE(d);
      TRACE(addresses[d]);
      size_t width = dims_[d].width;

      // on first dimension, only create as many as needed to get to the bottom
      size_t lo = dim == d ? bit : 0;

      add_node(d, -1, -1, next_node);
      NCNodePointerType refine_node = dims_[d].size() - 1;
      for (int b = width-1; b >= int(lo); --b) {
        TRACE(b);
        TRACE(width - b - 1);
        bool right = path_refines_towards_right(addresses[d], b, width); // width - b - 1);
        TRACE(right);
        TRACE(refine_node);
        add_node(d,
                 !right ? refine_node : -1,
                  right ? refine_node : -1,
                 next_node);
        refine_node = dims_[d].size() - 1;
      }
      next_node = refine_node;
    }
    return next_node;
  }

  NCNodePointerType
  merge_spine(size_t dim, NCNodePointerType cube_node_index, NCNodePointerType spine_node_index)
  {
    TRACE_BLOCK("merge_spine")
    if (dim == dims_.size()) {
      // merge summaries
      TRACE_BLOCK("summaries dim, bottomed out");
      TRACE(dim);
      TRACE(cube_node_index);
      TRACE(spine_node_index);
      const Summary &cube_summ = summaries_[cube_node_index];
      const Summary &spine_summ = summaries_[spine_node_index];
      summaries_.push_back(cube_summ + spine_summ);
      return summaries_.size() - 1;
    }
    BOOST_ASSERT(cube_node_index != -1);
    BOOST_ASSERT(spine_node_index != -1);

    const NCDimNode &cube_node = dims_[dim].nodes[cube_node_index];
    const NCDimNode &spine_node = dims_[dim].nodes[spine_node_index];

    BOOST_ASSERT(spine_node.left_ == -1 || spine_node.right_ == -1);

    // case 1: bottom of dimension
    if (spine_node.left_ == -1 && spine_node.right_ == -1) {
      TRACE_BLOCK("bottom of dimension");
      BOOST_ASSERT(cube_node.left_ == -1 && cube_node.right_ == -1);
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      add_node(dim, -1, -1, result);
      return NCNodePointerType(dims_[dim].size() - 1);
    }

    // case 2: cube_node is a singleton path in the same direction of spine_node
    if (cube_node.left_ == -1 && spine_node.left_ == -1 &&
        cube_node.right_ != -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("singleton path same direction, right");
      NCNodePointerType result = merge_spine(dim, cube_node.right_, spine_node.right_);
      add_node(dim, -1, result, dims_[dim].nodes[result].next_);
      return NCNodePointerType(dims_[dim].size() - 1);
    }
    if (cube_node.left_ != -1 && spine_node.left_ != -1 &&
        cube_node.right_ == -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("singleton path same direction, left");
      NCNodePointerType result = merge_spine(dim, cube_node.left_, spine_node.left_);
      add_node(dim, result, -1, dims_[dim].nodes[result].next_);
      return NCNodePointerType(dims_[dim].size() - 1);
    }

    // case 3: cube_node is a singleton path in the opposite direction of spine_node
    if (cube_node.left_ != -1 && spine_node.left_ == -1 &&
        cube_node.right_ == -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("singleton path opposite direction, right");
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      add_node(dim, cube_node.left_, spine_node.right_, result);
      return NCNodePointerType(dims_[dim].size() - 1);
    }
    if (cube_node.left_ == -1 && spine_node.left_ != -1 &&
        cube_node.right_ != -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("singleton path opposite direction, left");
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      add_node(dim, spine_node.left_, cube_node.right_, result);
      return NCNodePointerType(dims_[dim].size() - 1);
    }

    // case 4: cube_node is not a singleton path
    if (cube_node.left_ != -1 && spine_node.left_ == -1 &&
        cube_node.right_ != -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("not singleton path, right");
      NCNodePointerType next = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      NCNodePointerType ref = merge_spine(dim, cube_node.right_, spine_node.right_);
      add_node(dim, cube_node.left_, ref, next);
      return NCNodePointerType(dims_[dim].size() - 1);
    }
    if (cube_node.left_ != -1 && spine_node.left_ != -1 &&
        cube_node.right_ != -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("not singleton path, left");
      NCNodePointerType next = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      NCNodePointerType ref = merge_spine(dim, cube_node.left_, spine_node.left_);
      add_node(dim, ref, cube_node.right_, next);
      return NCNodePointerType(dims_[dim].size() - 1);
    }

    TRACE(cube_node);
    TRACE(spine_node);
    BOOST_ASSERT(false);
  }

  void
  add_to(const Summary &summary, const std::vector<size_t> &addresses,
         size_t dim, size_t bit, NCNodePointerType current_node_index)
  {
    TRACE_BLOCK("add_to " << dim << " " << bit << " " << current_node_index);
    // reached summaries array: we bottom out here by simply mutating
    // the summary for this particular address.
    if (dim == dims_.size()) {
      TRACE_BLOCK("bottom out on summaries");
      BOOST_ASSERT(bit == 0);
      summaries_[current_node_index] = summaries_[current_node_index] + summary;
      return;
    }
    
    // reached a previously unseen address?
    if (-1 == current_node_index) {
      BOOST_ASSERT(false);
      insert_fresh_node(summary, addresses, dim, bit);
      return;
    }

    NCDimNode &current_node = dims_[dim].nodes[current_node_index];
    NCNodePointerType l = current_node.left_;
    NCNodePointerType r = current_node.right_;
    NCNodePointerType n = current_node.next_;
    
    size_t width = dims_[dim].width;
    bool right = path_refines_towards_right(addresses[dim], bit, width);
    TRACE(addresses[dim]);
    TRACE(current_node);
    TRACE(right);
    TRACE(bit);
    TRACE(dim);
    TRACE(width);

    // WATCH OUT! don't use current_node here because it might have been
    // invalidated by a reallocation of the dim vector in the recursive call
    
    if (l == -1 && r == -1) {
      // bottom of this dimension
      TRACE_BLOCK("bottom");
      add_to(summary, addresses, dim+1, 0, n);
    } else if (right && l == -1 && r != -1) {
      // refine along a singleton path to the right
      TRACE_BLOCK("singleton refine to the right");
      add_to(summary, addresses, dim, bit+1, r);
      dims_[dim].nodes[current_node_index].next_ = dims_[dim].nodes[r].next_;
      report_node(dim, current_node_index);
    } else if (!right && l != -1 && r == -1) {
      // refine along a singleton path to the left
      TRACE_BLOCK("singleton refine to the left");
      add_to(summary, addresses, dim, bit+1, l);
      dims_[dim].nodes[current_node_index].next_ = dims_[dim].nodes[l].next_;
      report_node(dim, current_node_index);
    } else if (right && l != -1 && r == -1) {
      TRACE_BLOCK("create new branching path to the right");
      // create new branching path to the right
      auto new_refine = insert_fresh_node(summary, addresses, dim, bit+1);
      TRACE(new_refine);
      dims_[dim].nodes[current_node_index].right_ = new_refine;
      dims_[dim].nodes[current_node_index].next_ = merge_spine(dim+1, n, dims_[dim].nodes[new_refine].next_);
      report_node(dim, current_node_index);
    } else if (!right && l == -1 && r != -1) {
      // create new branching path to the left
      TRACE_BLOCK("create new branching path to the left");
      auto new_refine = insert_fresh_node(summary, addresses, dim, bit+1);
      TRACE(new_refine);
      dims_[dim].nodes[current_node_index].left_ = new_refine;
      dims_[dim].nodes[current_node_index].next_ = merge_spine(dim+1, n, dims_[dim].nodes[new_refine].next_);
      report_node(dim, current_node_index);
    } else {
      // go along one side of an existing path, must add to refinement _and_
      // next, since next points to a non-trivial merged summary.
      BOOST_ASSERT(l != -1 && r != -1);
      if (right) {
        TRACE_BLOCK("add to refine and next on right");
        add_to(summary, addresses, dim, bit+1, r);
        add_to(summary, addresses, dim+1, 0, n);
      } else {
        TRACE_BLOCK("add to refine and next on left");
        add_to(summary, addresses, dim, bit+1, l);
        add_to(summary, addresses, dim+1, 0, n);
      }
    }
  }
  
  void
  insert(const std::vector<size_t> &addresses, const Summary &summary)
  {
    TRACE_BLOCK("insert " << stream_vector<size_t>(addresses));
    // base case: initialize an empty nanocube
    if (base_root_ == -1) {
      base_root_ = insert_fresh_node(summary, addresses, 0, 0);
      TRACE(base_root_);
    } else {
      add_to(summary, addresses, 0, 0, base_root_);
    }
  }

  template <typename SummaryPolicy>
  void range_query(
      SummaryPolicy &policy,
      const std::vector<std::pair<size_t, size_t> > &bounds);
  
  template <typename SummaryPolicy>
  void range_query_internal
  (SummaryPolicy &policy,
   const std::vector<std::pair<size_t, size_t> > &bounds,
   NCNodePointerType current_node, size_t dim,
   size_t node_left, size_t node_right);


  void print_dot_ncdim(std::ostream &os, const NCDim &dim,
                       size_t d, bool draw_next) {
    os << " subgraph cluster_" << d << " {\n";
    os << " label=\"Dim. " << d << "\";\n";
    for (int i=0; i<dim.nodes.size(); ++i) {
      os << "  \"" << i << "_" << d << "\" [label=\"" << i << ":";
      if (dim.nodes[i].next_ == -1)
        os << "null";
      else
        os << dim.nodes[i].next_;
      os << "\"];\n";
    }
    for (int i=0; i<dim.nodes.size(); ++i) {
      const NCDimNode &node = dim.nodes[i];
      if (node.left_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.left_ << "_" << d << "\" [label=\"0\"];\n";
      if (node.right_ >= 0)
        os << "  \"" << i << "_" << d << "\" -> \"" << node.right_ << "_" << d << "\" [label=\"1\"];\n";
    }
    os << "}\n";
  }
  
  void print_dot(std::ostream &os) {
    os << "digraph G {\n";
    os << "  splines=line;\n";
    for (size_t i=0; i<dims_.size(); ++i) {
      const NCDim &dim = dims_[i];
      print_dot_ncdim(os, dim, i, i < (dims_.size() - 1));
    }
    os << "}\n";
  };

  void dump_nc() {
    for (int i=0; i<dims_.size(); ++i) {
      std::cerr << "Dim " << i << ":\n";
      const NCDim &dim = dims_[i];
      for (int j=0; j<dim.size(); ++j) {
        std::cerr << "  " << j << ": " << dim[j] << std::endl;
      }
      std::cerr << "\n";
    }
    std::cerr << "Summaries:\n";
    for (int i=0; i<summaries_.size(); ++i) {
      std::cerr << "  " << i << ": " << summaries_[i] << std::endl;
    }
  }
};

template <typename Summary>
template <typename SummaryPolicy>
void NanoCube<Summary>::range_query_internal(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds,
    NCNodePointerType current_node_index, size_t dim,
    size_t node_left, size_t node_right)
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
void NanoCube<Summary>::range_query(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds)
{
  range_query_internal(policy, bounds,
                       base_root_, 0,
                       0, 1 << dims_[0].width);
}

bool test_naive_cube_and_nanocube_equivalence();
bool test_naive_cube_and_nanocube_equivalence_1();
bool test_naive_cube_and_nanocube_equivalence_2();

};

std::ostream &operator<<(std::ostream &os, const nc2::NCDimNode &node);


#endif
