#pragma once

#include "json.hpp"
using json = nlohmann::json;

/*
 * cast the json str to double 
 */
inline double jtod(json j){
    std::string t = j;
    return strtod(t.c_str(), NULL);
}
