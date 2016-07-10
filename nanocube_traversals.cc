#include "nanocube_traversals.h"
#include <stack>

using namespace std;

struct BoundedIndex {
  BoundedIndex(int l, int r, int i, int d): left(l), right(r), index(i), depth(d) {}
  BoundedIndex(const BoundedIndex &other):
      left(other.left), right(other.right), index(other.index), depth(other.depth) {};
  
  int left, right, index, depth;
};

void minimal_cover(const NCDim &dim, int starting_node,
                   int lo, int up, int resolution,
                   vector<int> &nodes)
{
  stack<BoundedIndex> node_indices;
  node_indices.push(BoundedIndex(0, 1 << dim.width, starting_node, 0));
  while (node_indices.size()) {
    BoundedIndex t = node_indices.top();
    NCDimNode &node = dim.nodes.at(t.index);
    node_indices.pop();
    if (t.left >= lo && t.right <= up) {
      nodes.push(node.next);
    } else if (up <= t.left || t.right >= lo) {
      continue;
    } else {
      // avoid underflow for large values. Remember Java's lesson..
      int mid = lo + ((up - lo) / 2);
      if (node.left != -1) {
        node_indices.push(t.left, mid, node.left, t.depth+1);
      }
      if (node.right != -1) {
        node_indices.push(mid, t.right, node.right, t.depth+1);
      }
    }
  }
}
