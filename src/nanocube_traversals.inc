// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

struct BoundedIndex {
  BoundedIndex(int64_t l, int64_t r, int64_t a, int i, int d): 
               left(l), right(r), address(a), index(i), depth(d) {}
  BoundedIndex(const BoundedIndex &other):
               left(other.left), right(other.right), address(other.address), 
               index(other.index), depth(other.depth) {};
  
  int64_t left, right;
  int64_t address;
  int index, depth;
};

template <typename T> 
void query_range(const Nanocube<T> &nc, int dim_index, int starting_node,
                 int64_t lo, int64_t up, int lo_depth, int up_depth,
                 vector<QueryNode> &nodes, bool insert_partial_overlap)
{
  const NCDim dim = nc.dims.at(dim_index);
  stack<BoundedIndex> node_indices;
  node_indices.push(BoundedIndex(0, (int64_t)1 << dim.width, 0, starting_node, 0));

  while (node_indices.size()) {
    BoundedIndex t = node_indices.top();
    const NCDimNode &node = dim.at(t.index);
    node_indices.pop();
    if ( (t.left >> (dim.width-lo_depth)) >= lo && 
         (t.right >> (dim.width-up_depth)) <= up) {
      nodes.push_back(QueryNode(t.index, t.depth, dim_index, t.address));
    } else if (up < (t.left >> (dim.width-up_depth)) ||
               (t.right >> (dim.width-lo_depth)) < lo) {
      continue;
    } else if (t.depth == dim.width) {
      if (insert_partial_overlap) {
        nodes.push_back(QueryNode(t.index, t.depth, dim_index, t.address));
      }
    } else {
      // avoid underflow for large values. Remember Java's lesson..
      int64_t mid = t.left + ((t.right - t.left) / 2);
      if (node.left != -1) {
        node_indices.push(
            BoundedIndex(t.left, mid, t.address << 1, node.left, t.depth+1));
      }
      if (node.right != -1) {
        node_indices.push(
            BoundedIndex(mid, t.right, (t.address << 1)+1, node.right, t.depth+1));
      }
    }
  }
}

template <typename T> 
void query_find(const Nanocube<T> &nc, int dim_index, int starting_node,
                int64_t value, int depth, std::vector<QueryNode> &nodes)
{
  const NCDim dim = nc.dims.at(dim_index);
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
      nodes.push_back(QueryNode(result, depth, dim_index, value));
  }
}

template <typename T>
void query_split(const Nanocube<T> &nc, int dim_index, int starting_node,
                 int64_t prefix, int depth, int resolution,
                 std::vector<QueryNode> &nodes)
{
  const NCDim dim = nc.dims.at(dim_index);
  vector<QueryNode> splitNode;
  query_find(nc, dim_index, starting_node, prefix, depth, splitNode);
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
        s.push(QueryNode(node.left, t.depth+1, dim_index, t.address<<1));
      }
      if (node.right != -1) {
        s.push(QueryNode(node.right, t.depth+1, dim_index, (t.address<<1)+1));
      }
    }
  }
}

template <typename Summary>
json NCQuery(json &q,
             const Nanocube<Summary> &nc,
             bool insert_partial_overlap,
             int dim, 
             int index)
{

  //////////////////////////////////////////////////////////////////////////////
  // parse json for current operation and corresponding parameters
  //////////////////////////////////////////////////////////////////////////////
  int op; // 0 for find, 1 for split, 2 for range, 3 for all
  int64_t prefix_addr;
  int prefix_depth;
  int resolution;
  int64_t lowerBound_addr;
  int lowerBound_depth;
  int64_t upperBound_addr;
  int upperBound_depth;

  if(q.count(to_string(dim)) == 0) {
    op = 3; // no clause for this dimention means "all" 
  } else {
    json clause = q[to_string(dim)];
    string op_str = clause["operation"];
    if(op_str == "find") op = 0;
    else if(op_str == "split") op = 1;
    else if(op_str == "range") op = 2;
    else if(op_str == "all") op = 3;
    else op = 4;

    switch(op) {
      case 0: prefix_addr = clause["prefix"]["address"];
              prefix_depth = clause["prefix"]["depth"];
              break;
      case 1: prefix_addr = clause["prefix"]["address"];
              prefix_depth = clause["prefix"]["depth"];
              resolution = clause["resolution"];
              break;
      case 2: lowerBound_addr = clause["lowerBound"]["address"];
              lowerBound_depth = clause["lowerBound"]["depth"];
              upperBound_addr = clause["upperBound"]["address"];
              upperBound_depth = clause["upperBound"]["depth"];
              break;
      case 3: break;
      default: return Summary();
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Find all valid nodes
  //////////////////////////////////////////////////////////////////////////////
  std::vector<QueryNode> nodes;

  if (dim == 0) {
    index = nc.base_root;
  }

  switch(op) {
    case 0: query_find(nc, dim, index, prefix_addr, prefix_depth, nodes);
            break;
    case 1: query_split(nc, dim, index, prefix_addr, prefix_depth, resolution, 
                        nodes);
            break;
    case 2: query_range(nc, dim, index, lowerBound_addr, upperBound_addr,
                        lowerBound_depth, upperBound_depth, nodes);
            break;
    case 3: nodes.push_back(QueryNode(index, 0, dim, 0));
            break;
  }
  if (nodes.size() == 0) {
    return Summary();
  }

  //////////////////////////////////////////////////////////////////////////////
  // assemble summary results from nodes
  //////////////////////////////////////////////////////////////////////////////
  json result;
  if(dim == nc.dims.size() - 1) { // get summary
    if(op == 1) { // split
      for(int i = 0; i < nodes.size(); i ++) {
        Summary s = (nc.summaries.at(nc.dims.at(dim).at(nodes[i].index).next));
        result[to_string(nodes[i].address)] = s;
      }

    } else { // find || range || all
      Summary summary = Summary();
      for(int i = 0; i < nodes.size(); i ++) {
        summary += nc.summaries.at(nc.dims.at(dim).at(nodes[i].index).next);
      }
      result = summary;
    }

  } else { // get nodes for the next dimension
    if(op == 1) { // split
      for(int i = 0; i < nodes.size(); i ++) {
        result[to_string(nodes[i].address)] = 
          NCQuery(q, nc, insert_partial_overlap, dim+1,
                  nc.dims.at(dim).at(nodes[i].index).next);
      }

    } else { // find || range || all
      for(int i = 0; i < nodes.size(); i ++) {
        result.push_back(NCQuery(q, nc, insert_partial_overlap, dim+1,
                         nc.dims.at(dim).at(nodes[i].index).next));
      }
    }

  }

  if(result.is_array()) {
    return merge_query_result<Summary>(result);
  } else {
    return result;
  }
}

template <typename Summary>
json merge_query_result(const json &raw)
{
  if(raw[0].is_object()) {
    map<string, json> resultMap;
    for(auto it = raw.begin(); it != raw.end(); ++ it) {
      for(auto it2 = it->begin(); it2 != it->end(); ++ it2) {
        string k = it2.key();
        auto f = resultMap.find(k);
        if(f == resultMap.end()) {
          resultMap[k] = it2.value();
        } else {
          resultMap[k] = 
            merge_query_result<Summary>({resultMap[k], it2.value()});
        }
      }
    }

    json merge;
    for(auto it = resultMap.begin(); it != resultMap.end(); ++it) {
      merge[it->first] = it->second;
    }
    return merge;

  } else {
    Summary sum = Summary();
    for(int i = 0; i < raw.size(); i ++) {
      Summary t = raw[i];
      sum += t;
    }
    return sum;
  }
}
