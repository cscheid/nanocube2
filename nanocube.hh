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
  vector<NCDim> dims;
  vector<Summary> summaries;
  void insert(Summary &, vector<int> &);
  set<int> next_indices(int root_index,
                        int address, int dim);
  int insert_dim(Summary &, vector<int> &addresses,
                 int root, int dim);
  int base_root;
};

