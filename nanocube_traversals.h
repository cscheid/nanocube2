#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include "nanocube.h"

// All nanocube traversal algorithms ultimately flow from a minimal cover
// of a 1D interval by recursive binary splits

// the nodes vector denotes a vector of node indices and their respective depths
// in the tree
void minimal_cover(const NCDim &dim, int starting_node,
                   int lower_bound, int upper_bound, int resolution,
                   std::vector<pair<int, int> > &nodes,
                   bool insert_partial_overlap = false);

// select single node from any given dimension. returns node index. O(dim.depth)
int select(const NCDim &dim, int starting_node, int value);

