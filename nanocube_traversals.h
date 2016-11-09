#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include "nanocube.h"


struct QueryNode{
    QueryNode() {};
    QueryNode(const QueryNode &other): index(other.index), depth(other.depth), address(other.address) {};
    QueryNode(int i, int d, int a): index(i), depth(d), address(a) {};

    int index, depth;
    int64_t address;
};

// All nanocube traversal algorithms ultimately flow from a minimal cover
// of a 1D interval by recursive binary splits

// the nodes vector denotes a vector of node indices and their respective depths
// in the tree
void query_range(const NCDim &dim, int starting_node,
                   int64_t lower_bound, int64_t upper_bound, int resolution,
                   std::vector<pair<int, int> > &nodes,
                   bool insert_partial_overlap = false);

// select single node from any given dimension. returns node index. O(dim.depth)
void query_find(const NCDim &dim, int starting_node, int64_t address, int depth,
               std::vector<QueryNode> &nodes);

void query_split(const NCDim &dim, int starting_node,
                 int64_t prefix, int depth, int resolution,
                 std::vector<QueryNode> &nodes);


//////////////////////////////////////////////////////////////////////////

string QueryTestFind(Nanocube<int> &gc, int dim, int64_t address, int depth,
                bool validate, 
                vector<pair<int64_t,int64_t> > &dataarray,
                vector<int> &schema);

string QueryTestSplit(Nanocube<int> &gc, int dim, int64_t prefix, int depth,
                   int resolution,
                   bool validate, 
                   vector<pair<int64_t,int64_t> > &dataarray,
                   vector<int> &schema);

template <typename Summary>
Summary ortho_range_query(const Nanocube<Summary> &nc,
                          const vector<pair<int64_t, int64_t> > &range,
                          bool insert_partial_overlap = false,
                          int dim = 0,
                          int index = -1);

#include "nanocube_traversals.inc"
