#include "nanocube_traversals.h"
#include <stack>

using namespace std;

namespace {

struct BoundedIndex {
  BoundedIndex(int64_t l, int64_t r, int64_t i, int64_t d): left(l), right(r), index(i), depth(d) {}
  BoundedIndex(const BoundedIndex &other):
      left(other.left), right(other.right), index(other.index), depth(other.depth) {};
  
  int64_t left, right, index, depth;
};

};

void minimal_cover
(const NCDim &dim, int starting_node, int64_t lo, int64_t up, int resolution,
 vector<pair<int, int> > &nodes, bool insert_partial_overlap)
{
  stack<BoundedIndex> node_indices;
  
  node_indices.push(BoundedIndex(0, (int64_t)1 << dim.width, starting_node, 0));
  while (node_indices.size()) {
    BoundedIndex t = node_indices.top();
    const NCDimNode &node = dim.at(t.index);
    node_indices.pop();
    if (t.left >= lo && t.right <= up) {
      nodes.push_back(make_pair(t.index, t.depth));
    } else if (up <= t.left || t.right <= lo) {
      continue;
    } else if (t.depth == resolution) {
      if (insert_partial_overlap) {
        nodes.push_back(make_pair(t.index, t.depth));
      }
    } else {
      // avoid underflow for large values. Remember Java's lesson..
      int64_t mid = t.left + ((t.right - t.left) / 2);
      if (node.left != -1) {
        node_indices.push(BoundedIndex(t.left, mid, node.left, t.depth+1));
      }
      if (node.right != -1) {
        node_indices.push(BoundedIndex(mid, t.right, node.right, t.depth+1));
      }
    }
  }
}

int select(const NCDim &dim, int starting_node, int64_t value, int depth)
{
  int d = depth < dim.width ? depth : dim.width;
  int result = starting_node;
  for (int i=0; i<d; ++i) {
    if (result == -1) return -1;
    const NCDimNode &node = dim.nodes.values.at(result);
    int which_direction = get_bit(value, d-i-1);
    if (which_direction) {
      result = node.right;
    } else {
      result = node.left;
    }
    //cout << result << " | " << which_direction << " <-- " << value << endl;
  }
  return result;
}

int GCQueryFind(Nanocube<int> &gc, int dim, int64_t address, int depth, bool validate, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema)
{
    if(dim == 0) {
        int index = select(gc.dims.at(dim), gc.base_root, address, depth);
        return index;
    } else {
        return -1;
    }

}
