#pragma once

#include <vector>
#include <utility>
#include <set>
#include <map>
#include <cassert>

using namespace std;

template <typename T>
struct RefCountedVec {

  // add a reference to an existing value. returns new reference count
  int make_ref(int index);

  // remove a reference from an existing value. returns new reference count
  int release_ref(int index);

  // insert fresh value and return index of reference to it. Returns reference
  // (*not* count, but rather the index)
  int insert(const T &value);

  // compacts the vector, ensuring that free_list.size() == 0 after the call.

  // returns the transposition map of the compaction. It's the responsibility
  // of the caller to update upstream references
  map<int, int> compact();
  
  vector<T> values;
  vector<int> ref_counts;
  vector<int> free_list;
};

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

template <typename Summary>
struct Nanocube {
  pair<int, int> insert_node
  (const Summary &summary, const vector<int> &addresses, int current_node, int current_dim, int current_bit, std::map<int, int> &summary_cache);

  void insert(const Summary &summary, const vector<int> &addresses);

  /****************************************************************************/
  // simple accessors
  inline int get_summary_index(int node_index, int dim);
  inline NCDimNode get_children(int node_index, int dim);

  /****************************************************************************/
  // utility
  int release_node_ref(int node_index, int dim);
  void compact();

  void dump_internals();
  
  /****************************************************************************/
  // members
  int base_root;
  vector<NCDim> dims;
  RefCountedVec<Summary> summaries;

  explicit Nanocube(const vector<int> &widths);
};

#include "nanocube.inc"
