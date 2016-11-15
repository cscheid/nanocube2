#pragma once

#include <vector>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;


template <typename Summary>
struct Naivecube {
  int indexDims;
  vector<int> dimWidth;
  vector<pair<vector<int64_t>, Summary> > data;

  void insert(const Summary &value, const vector<int64_t> &address);

  Naivecube(const vector<int> &schema);
};


template <typename Summary>
json NaiveCubeQuery(json &q, const Naivecube<Summary> &nc);


#include "naivecube.inc"
