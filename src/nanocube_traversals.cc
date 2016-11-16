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

bool isQueryValid(const json &q)
{
  for (auto it = q.begin(); it != q.end(); ++ it) {
    // the key must be a number
    char* p;
    long converted = strtol(it.key().c_str(), &p, 10);
    if (*p) { // conversion failed
      return false;
    }

    json clause = it.value();
    if ( !clause.is_object() ) return false;
    if ( !clause["operation"].is_string() ) return false;

    string op_str = clause["operation"];
    int op;
    if(op_str == "find") op = 0;
    else if(op_str == "split") op = 1;
    else if(op_str == "range") op = 2;
    else if(op_str == "all") op = 3;
    else return false;

    switch(op) {
      case 0:
        if (clause.count("prefix") != 1) return false;
        if (clause["prefix"].count("address") != 1) return false;
        if (clause["prefix"].count("depth") != 1) return false;
        if ( ! clause["prefix"]["address"].is_number() ) return false;
        if ( ! clause["prefix"]["depth"].is_number() ) return false;
        break;
      case 1: 
        if (clause.count("prefix") != 1) return false;
        if (clause.count("resolution") != 1) return false;
        if (clause["prefix"].count("address") != 1) return false;
        if (clause["prefix"].count("depth") != 1) return false;
        if ( ! clause["prefix"]["address"].is_number() ) return false;
        if ( ! clause["prefix"]["depth"].is_number() ) return false;
        if ( ! clause["resolution"].is_number() ) return false;
        break;
      case 2: 
        if (clause.count("lowerBound") != 1) return false;
        if (clause.count("upperBound") != 1) return false;
        if (clause["lowerBound"].count("address") != 1) return false;
        if (clause["lowerBound"].count("depth") != 1) return false;
        if (clause["upperBound"].count("address") != 1) return false;
        if (clause["upperBound"].count("depth") != 1) return false;
        if ( ! clause["lowerBound"]["address"].is_number() ) return false;
        if ( ! clause["lowerBound"]["depth"].is_number() ) return false;
        if ( ! clause["upperBound"]["address"].is_number() ) return false;
        if ( ! clause["upperBound"]["depth"].is_number() ) return false;
        break;
    }

  }

  return true;
}
