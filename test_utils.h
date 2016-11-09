#pragma once

#include <vector>
#include "nanocube.h"

float uniform_variate();
vector<int64_t> random_point(const vector<int> &schema);
vector<pair<int64_t, int64_t> > random_region(const vector<int> &schema);
void test(Nanocube<int> &nc, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema);
int GCQueryFind(Nanocube<int> &gc, int dim, int64_t address, int depth,
                bool validate, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema);
