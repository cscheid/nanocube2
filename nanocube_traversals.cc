#include "nanocube_traversals.h"

#include <stack>
#include <sstream>

#include "json.hpp"

using json = nlohmann::json;
using namespace std;


std::ostream& operator<<(std::ostream& os, const QueryNode &n)
{
  os << "{\"index\":" << n.index<< ",\"depth\":" << n.depth;
  os << ",\"dim\":" << n.dim << ",\"address\":" << n.address << "}";
  return os;
}

//string QueryTestFind(Nanocube<int> &nc, int dim, int64_t address, int depth, bool validate, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema)
//{
    //std::vector<QueryNode> nodes;
    //if(dim == 0) {
        //query_find(nc, dim, nc.base_root, address, depth, nodes);
        //if (nodes.size() > 0) {
            //std::stringstream buffer;
            //buffer << "[\"" << nodes[0].index << "-" << nodes[0].depth;
            //buffer << "-" << nodes[0].address << "\"]";
            //return buffer.str();
        //} else {
            //return "";
        //}
    //} else {
        //return "";
    //}
//}

//string QueryTestSplit(Nanocube<int> &nc, int dim, int64_t prefix, int depth,
                   //int resolution,
                   //bool validate, 
                   //vector<pair<int64_t,int64_t> > &dataarray,
                   //vector<int> &schema)
//{
    //std::vector<QueryNode> nodes;
    //if(dim == 0) {
        //query_split(nc, dim, nc.base_root, prefix, depth, resolution, nodes);
        //if (nodes.size() > 0) {
            //std::stringstream buffer;
            //buffer << "[";
            //for(int i = 0; i < nodes.size(); i ++) {
                //if( i != 0) buffer << ",";
                //buffer << "\"" << nodes[i].index << "-" << nodes[i].depth;
                //buffer << "-" << nodes[i].address << "\"";
            //}
            //buffer << "]";
            //return buffer.str();
        //} else {
            //return "";
        //}
    //} else {
        //return "";
    //}
//}

//string QueryTestRange(Nanocube<int> &nc, int dim, int64_t lo, int64_t up,
                      //int depth,
                      //bool validate, 
                      //vector<pair<int64_t,int64_t> > &dataarray,
                      //vector<int> &schema)
//{
    //std::vector<QueryNode> nodes;
    //if(dim == 0) {
        //query_range(nc, dim, nc.base_root, lo, up, depth, nodes);
        //if (nodes.size() > 0) {
            //std::stringstream buffer;
            //buffer << "[";
            //for(int i = 0; i < nodes.size(); i ++) {
                //if( i != 0) buffer << ",";
                //buffer << "\"" << nodes[i].index << "-" << nodes[i].depth;
                //buffer << "-" << nodes[i].address << "\"";
            //}
            //buffer << "]";
            //return buffer.str();
        //} else {
            //return "";
        //}
    //} else {
        //return "";
    //}
//}
