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
json merge_query_result(json raw) {

  if(raw.is_array()) {

    if(raw[0].is_object()) {
      map<ResultKey, json> resultMap;
      for(auto it = raw.begin(); it != raw.end(); ++ it) {
        if ((*it).find("0") != (*it).end()) {
          for(int i = 0; i < it->size(); ++i) {
            json temp = (*it)["s"+to_string(i)];
            ResultKey k = ResultKey(temp["address"], 
                                    temp["depth"], 
                                    temp["dimension"]);
            auto f = resultMap.find(k);
            if(f == resultMap.end()) {
              resultMap[k] = temp["value"];
            } else {
              resultMap[k] = 
                merge_query_result<Summary>({resultMap[k], temp["value"]});
            }
          }
        } else {
          // TODO maybe this case don't exist
          cout << "this merge exitst!!" << endl;
          ResultKey k = ResultKey((*it)["address"], 
                                  (*it)["depth"], 
                                  (*it)["dimension"]);
          auto f = resultMap.find(k);
          if(f == resultMap.end()) {
            resultMap[k] = (*it)["value"];
          } else {
            resultMap[k] = 
              merge_query_result<Summary>({resultMap[k], (*it)["value"]});
          }
        }
      }

      json merge;
      int count = 0;
      for(auto it = resultMap.begin(); it != resultMap.end(); ++it) {
        merge["s"+to_string(count)] = { {"address", it->first.address}, 
                                        {"depth", it->first.depth}, 
                                        {"dimension", it->first.dimension}, 
                                        {"value", it->second}};
        count ++;
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

  } else if(raw.is_object()) {
    for(int i = 0; i < raw.size(); ++i) {
      raw["s"+to_string(i)]["value"] = 
        merge_query_result<Summary>(raw["s"+to_string(i)]["value"]);
    }
    return raw;
  } else {
    return raw;
  }

}

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
