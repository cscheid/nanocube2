#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <vector>
#include <utility>
#include <set>
#include <stack>
#include <map>
#include <unordered_map>
#include <cassert>
#include <fstream>

#include "ref_counted_vec.h"

using namespace std;

struct NCDimNode {
  NCDimNode() {};
  NCDimNode(const NCDimNode &other): left(other.left), right(other.right), next(other.next) {};
  NCDimNode(int l, int r, int n): left(l), right(r), next(n) {};

  int left, right, next;
};
  
struct NCDim {
  RefCountedVec<NCDimNode> nodes;
  int width;
  inline NCDimNode &at(int i) { return nodes.values.at(i); };
  inline const NCDimNode &at(int i) const { return nodes.values.at(i); };
  inline size_t size() const { return nodes.values.size(); };
};

struct NCMutantNode {
  //root_index is the root of the tree which contains this node
  int index, dim, root_index; 
  bool from_insert; // we need to record if this node need to be mutated because of a insert happened
  NCMutantNode(int i, int d, int r, int insert): 
    index(i), dim(d), root_index(r), from_insert(insert) {};
};

struct PairHasher {
  size_t operator()(const pair<int, int> &p) const {
    // FIXME FIND BETTER HASHER
    return ((size_t) p.first * 65521 + (size_t) p.second * 65537) & 65535;
  }
};

template <typename Summary>
struct Nanocube {
  typedef std::unordered_map<pair<int, int>, int, PairHasher> SummaryCache;

  pair<int, int> insert_fresh_node
  (const Summary &summary, const vector<int64_t> &addresses, int dim, int bit);
  
  //pair<int, int> insert_node
  //(const Summary &summary, const vector<int64_t> &addresses, int current_node, int current_dim, int current_bit, SummaryCache &summary_cache);

  pair<int, int> merge(int left, int right, int dim, SummaryCache &summary_cache);

  void insert(const Summary &summary, const vector<int64_t> &addresses);

  void new_insert(const Summary &summary, const vector<int64_t> &addresses);
  void update_tree(const Summary &summary, const vector<int64_t> &addresses, 
                   const int dim, const int node_index, 
                   stack<NCMutantNode> &mutant_nodes,
                   pair<int,int> &insert_cache);
  pair<int, int> copy_tree(const vector<int64_t> &addresses, int index, int dim);
  void update_next_pointer(const vector<int64_t> &addresses, 
                           int dim, int root_index, int stop_index, 
                           int old_next, int new_next);
  inline bool parent_has_single_child(const vector<int64_t> &addresses, int dim, int node_index, int root_index);

  /****************************************************************************/
  // simple accessors
  inline int get_summary_index(int node_index, int dim);
  inline NCDimNode get_children(int node_index, int dim);

  /****************************************************************************/
  // queries
  Summary ortho_range_query(const vector<pair<int, int> > &region);
  
  /****************************************************************************/
  // utility
  inline void release_node_ref(int node_index, int dim);
  inline int make_node_ref(int node_index, int dim);
  inline int get_node_ref(int node_index, int dim);

  inline void set_left_node_ref(int node_index, int dim, int value);
  inline void set_right_node_ref(int node_index, int dim, int value);
  inline void set_next_node_ref(int node_index, int dim, int value);
  
  void compact();

  // use summary values to extract a minimal-size nanocube.
  // NB: after calling content_compact(), insert_node will yield
  // undefined behavior.

  // NB, this is fairly inefficient at the moment.
  void content_compact();

  void dump_internals(bool force_print=false);

  void report_size() const;

  bool validate_refcounts();
  
  /****************************************************************************/
  // members
  int base_root;
  vector<NCDim> dims;
  RefCountedVec<Summary> summaries;

  explicit Nanocube(const vector<int> &widths, bool debug=false);
  Nanocube(const Nanocube<Summary> &other);

  void write_to_binary_stream(ostream &os);

 private:
  std::ofstream unopened;
  ostream &debug_out;
};

/******************************************************************************/

#include "nanocube.inc"
