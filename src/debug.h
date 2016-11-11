#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <iostream>
#include "nanocube.h"

template <typename Summary>
void print_dot(std::ostream &os, const Nanocube<Summary> &nc);

void print_dot_ncdim(std::ostream &os, const NCDim &dim, int d, bool draw_next);

#include "debug.inc"

