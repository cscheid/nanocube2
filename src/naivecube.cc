#include <iostream>
#include "naivecube.h"

namespace nc2 {

bool test_naive_cube_doesnt_smoke()
{
  NaiveCube<int> nc({2, 2}); 
  nc.insert({0, 0}, 1);
  nc.insert({3, 3}, 1);
  std::cerr << "Didn't smoke!" << std::endl;
  return true;
}

};
