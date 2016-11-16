#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <stack>
#include <sstream>

#include "nanocube.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

///////////////////////////////////////////////////////////////////////////////
// Data Structures
///////////////////////////////////////////////////////////////////////////////

// store a single node info during query
struct QueryNode {
  QueryNode() {};
  QueryNode(const QueryNode &other): index(other.index), depth(other.depth), 
            dim(other.dim), address(other.address) {};
  QueryNode(int i, int de, int di, int a): 
    index(i), depth(de), dim(di), address(a) {};

  int index, depth, dim;
  int64_t address;
};

std::ostream& operator<<(std::ostream& os, const QueryNode &n);

// use as the key when merging query result
struct ResultKey {
  ResultKey() {};
  ResultKey(int64_t a, int de, int di):
    address(a), depth(de), dimension(di) {};
  int64_t address;
  int depth, dimension;

  friend bool operator <(const ResultKey &r1, const ResultKey &r2) {
    if (r1.dimension < r2.dimension) {
      return true;
    } else if (r1.depth < r2.depth) {
      return true;
    } else if (r1.address < r2.address) {
      return true;
    } else {
      return false;
    }
  }
};

///////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////

// merge the raw result when possible
// for 'find' and 'range', the raw result could be an array
// TODO needs more testing
template <typename Summary>
json merge_query_result(const json &raw);

///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////
template <typename T> 
void query_range(const Nanocube<T> &nc, int dim_index, int starting_node,
                 int64_t lower_bound, int64_t upper_bound, 
                 int lo_depth, int up_depth,
                 std::vector<QueryNode> &nodes,
                 bool insert_partial_overlap = false);

template <typename T> 
void query_find(const Nanocube<T> &nc, int dim_index, int starting_node, 
                int64_t address, int depth, std::vector<QueryNode> &nodes);

template <typename T> 
void query_split(const Nanocube<T> &nc, int dim_index, int starting_node,
                 int64_t prefix, int depth, int resolution,
                 std::vector<QueryNode> &nodes);

///////////////////////////////////////////////////////////////////////////////
// APIs
///////////////////////////////////////////////////////////////////////////////
template <typename Summary>
json NCQuery(json &q,
             const Nanocube<Summary> &nc,
             bool insert_partial_overlap = false,
             int dim = 0,
             int index = -1);

#include "nanocube_traversals.inc"
