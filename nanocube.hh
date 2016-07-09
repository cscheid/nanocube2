#pragma once

#include <vector>
#include <utility>
#include <set>
#include <map>

using namespace std;

template <typename T>
struct RefCountedSummaryVec {

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
  int left, int right, int next;
};
  
struct NCDim {
  RefCountedVec<NCDimNode> nodes;
  int width;
};

template <typename Summary>
struct Nanocube {
  void insert(Summary &, vector<int> &);
  set<int> next_indices(int root_index,
                        int address, int dim);
  int insert_dim(Summary &, vector<int> &addresses,
                 int root, int dim);
  pair<int, int> insert_node
  (Summary &summary, vector<int> &addresses,
   int current_node, int current_dim, int current_bit);

  /****************************************************************************/
  // simple accessors
  inline int get_summary_index(int node_index, int dim);
  inline pair<int, int> Nanocube<Summary>::get_children(int node_index, int dim);

  /****************************************************************************/
  // members
  int base_root;
  vector<NCDim> dims;
  RefCountedVec<Summary> summaries;
};

