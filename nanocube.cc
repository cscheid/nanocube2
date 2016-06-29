#include "nanocube.hh"

inline int get_bit(int value, int bit)
{
  return (value >> bit) & 1;
}

template <typename Summary>
set<int> Nanocube<Summary>::next_indices
(int root_index, int address, int dim)
{
  NCDim &nc_dim = dims[dim];
  set<int> result;
  int current_index = root_index;
  for (int i=0; i<width; ++i) {
    result.insert(nc_dim.next[current_index]);
    int bit = get_bit(address, width-i-1);
    if (bit) {
      current_index = nc_dim.refine[current_index].second;
    } else {
      current_index = nc_dim.refine[current_index].first;
    }
  }
  return result;
};

template <typename Summary>
int Nanocube::insert_dim
(Summary &summary, vector<int> &addresses, int root_index, int dim)
{
  if (dim == dims.length) {
    // FIXME handle summaries
  } else {
    int current_address = addresses[dim];
    set<int> next_is = next_indices(root_index, current_address, dim);
    int current_index = root_index;
    int width = nc_dim[dim].width;
    for (int i=0; i<width; ++i) {
      int new_index = insert(summary, addresses, nc_dim[dim].next[current_index], dim+1);
      // THIS IS NOWHERE FINISHED;
    }
  }
}
 
