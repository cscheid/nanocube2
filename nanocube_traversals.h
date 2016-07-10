#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include "nanocube.h"

// All nanocube traversal algorithms ultimately flow from a minimal cover
// of a 1D interval by recursive binary splits

void minimal_cover(const NCDim &dim, int starting_node,
                   int lower_bound, int upper_bound, int resolution,
                   std::vector<int> &nodes);

