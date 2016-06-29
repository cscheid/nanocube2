#pragma once

#include <vector>
#include <utility>
#include <set>

using namespace std;

struct NCDim {
  vector <int> next;
  vector < pair<int,int> > refine;
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
  (Summary &summary, vector<int> &addresses, int dim, int current_node, int current_address, int current_bit);

  /****************************************************************************/
  // simple accessors
  inline int get_summary_index(int node_index, int dim);
  inline pair<int, int> Nanocube<Summary>::get_children(int node_index, int dim);

  /****************************************************************************/
  // members
  int base_root;
  vector<NCDim> dims;
  vector<Summary> summaries;
};

