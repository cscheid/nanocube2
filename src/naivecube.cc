#include "naivecube.h"

namespace nc2 {

bool test_naive_cube_doesnt_smoke()
{
  NaiveCube<int> nc({2, 2}); 
  nc.add({0, 0}, 1);
  nc.add({3, 3}, 1);
}

};
