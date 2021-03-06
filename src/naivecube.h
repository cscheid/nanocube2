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

  explicit Naivecube(const vector<int> &schema);
};


template <typename Summary>
json NaiveCubeQuery(const json &q, const Naivecube<Summary> &nc);

json to_nested(const json &j);

#include "naivecube.inc"
