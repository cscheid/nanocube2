#include "nanocube_traversals.h"
#include <stack>
#include <sstream>

using namespace std;

namespace {

struct BoundedIndex {
  BoundedIndex(int64_t l, int64_t r, int64_t i, int64_t d): left(l), right(r), index(i), depth(d) {}
  BoundedIndex(const BoundedIndex &other):
      left(other.left), right(other.right), index(other.index), depth(other.depth) {};
  
  int64_t left, right, index, depth;
};

};

void query_range 
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

void query_find(const NCDim &dim, int starting_node, int64_t value, int depth,
                std::vector<QueryNode> &nodes)
{
  int d = depth < dim.width ? depth : dim.width;
  int result = starting_node;
  for (int i=0; i<d; ++i) {
    if (result == -1) {
        return;
    }
    const NCDimNode &node = dim.nodes.values.at(result);
    int which_direction = get_bit(value, d-i-1);
    if (which_direction) {
      result = node.right;
    } else {
      result = node.left;
    }
    //cout << result << " | " << which_direction << " <-- " << value << endl;
  }
  if(result != -1) {
      nodes.push_back(QueryNode(result, depth, value));
  }
}

void query_split(const NCDim &dim, int starting_node,
                 int64_t prefix, int depth, int resolution,
                 std::vector<QueryNode> &nodes)
{
    vector<QueryNode> splitNode;
    query_find(dim, starting_node, prefix, depth, splitNode);
    if (splitNode.size() == 0) {
        return;
    }

    stack<QueryNode> s;
    s.push(splitNode[0]);

    while(s.size()) {
        QueryNode t = s.top();
        //cout << t.index << ":" << t.depth << ":" << t.address << endl;
        const NCDimNode &node = dim.at(t.index);
        s.pop();
        if (t.depth == depth+resolution || t.depth == dim.width) {
          nodes.push_back(t);
        } else {
          if (node.left != -1) {
            s.push(QueryNode(node.left, t.depth+1, t.address << 1));
          }
          if (node.right != -1) {
            s.push(QueryNode(node.right, t.depth+1, (t.address << 1) + 1));
          }
        }
    }
}


string QueryTestFind(Nanocube<int> &gc, int dim, int64_t address, int depth, bool validate, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema)
{
    std::vector<QueryNode> nodes;
    if(dim == 0) {
        query_find(gc.dims.at(dim), gc.base_root, address, depth, nodes);
        if (nodes.size() > 0) {
            std::stringstream buffer;
            buffer << nodes[0].index << endl;
            return buffer.str();
        } else {
            return "";
        }
    } else {
        return "";
    }
}

string QueryTestSplit(Nanocube<int> &gc, int dim, int64_t prefix, int depth,
                   int resolution,
                   bool validate, 
                   vector<pair<int64_t,int64_t> > &dataarray,
                   vector<int> &schema)
{
    std::vector<QueryNode> nodes;
    if(dim == 0) {
        query_split(gc.dims.at(dim), gc.base_root, prefix, depth, resolution, nodes);
        if (nodes.size() > 0) {
            std::stringstream buffer;
            buffer << "\"";
            for(int i = 0; i < nodes.size(); i ++) {
                buffer << nodes[i].index << "-" << nodes[i].depth << "-" << nodes[i].address << ",";
            }
            buffer << "\"";
            return buffer.str();
        } else {
            return "";
        }
    } else {
        return "";
    }
}
