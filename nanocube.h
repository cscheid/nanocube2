#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <vector>
#include <utility>
#include <set>
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

struct PairHasher {
  size_t operator()(const pair<int, int> &p) const {
    // FIXME FIND BETTER HASHER
    return ((size_t) p.first * 65521 + (size_t) p.second * 65537) & 65535;
  }
};
template <typename Summary>
struct Nanocube {
  int cache_hit, cache_miss;
  typedef std::unordered_map<pair<int, int>, int, PairHasher> SummaryCache;
  
  pair<int, int> insert_node
  (const Summary &summary, const vector<int> &addresses, int current_node, int current_dim, int current_bit, SummaryCache &summary_cache);

  pair<int, int> merge(int left, int right, int dim, SummaryCache &summary_cache);

  void insert(const Summary &summary, const vector<int> &addresses);

  /****************************************************************************/
  // simple accessors
  inline int get_summary_index(int node_index, int dim);
  inline NCDimNode get_children(int node_index, int dim);

  /****************************************************************************/
  // utility
  void release_node_ref(int node_index, int dim);
  int make_node_ref(int node_index, int dim);

  void set_left_node_ref(int node_index, int dim, int value);
  void set_right_node_ref(int node_index, int dim, int value);
  void set_next_node_ref(int node_index, int dim, int value);
  
  void compact();

  void dump_internals(bool force_print=false);

  bool validate_refcounts();
  
  /****************************************************************************/
  // members
  int base_root;
  vector<NCDim> dims;
  RefCountedVec<Summary> summaries;

  explicit Nanocube(const vector<int> &widths, bool debug=false);
  Nanocube(const Nanocube<Summary> &other);

 private:
  std::ofstream unopened;
  ostream &debug_out;
};

/******************************************************************************/

#include "nanocube.inc"
