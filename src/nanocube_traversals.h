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
///////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////
inline double jtod(json j){
  std::string t = j;
  return strtod(t.c_str(), NULL);
}


///////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////
template <typename T> 
void query_range(const Nanocube<T> &nc, int dim_index, int starting_node,
                 int64_t lower_bound, int64_t upper_bound, int depth,
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
// Test Functions
///////////////////////////////////////////////////////////////////////////////

//string QueryTestFind(Nanocube<int> &nc, int dim, int64_t address, int depth,
                //bool validate, 
                //vector<pair<int64_t,int64_t> > &dataarray,
                //vector<int> &schema);

//string QueryTestSplit(Nanocube<int> &nc, int dim, int64_t prefix, int depth,
                   //int resolution,
                   //bool validate, 
                   //vector<pair<int64_t,int64_t> > &dataarray,
                   //vector<int> &schema);

//string QueryTestRange(Nanocube<int> &nc, int dim, int64_t lo, int64_t up,
                      //int depth,
                      //bool validate, 
                      //vector<pair<int64_t,int64_t> > &dataarray,
                      //vector<int> &schema);


///////////////////////////////////////////////////////////////////////////////
// APIs
///////////////////////////////////////////////////////////////////////////////
template <typename Summary>
json NCQuery(json &q,
             const Nanocube<Summary> &nc,
             bool insert_partial_overlap = false,
             int dim = 0,
             int index = -1);

//template <typename Summary>
//Summary ortho_range_query(const Nanocube<Summary> &nc,
                          //const vector<pair<int64_t, int64_t> > &range,
                          //bool insert_partial_overlap = false,
                          //int dim = 0,
                          //int index = -1);

#include "nanocube_traversals.inc"
