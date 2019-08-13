#include <iostream>
#include "naivecube.h"
#include "nanocube.h"
#define RUN_TEST(t) if (!t()) { std::cerr << "test " << #t << " failed." << std::endl; exit(1); } else { std::cerr << "test " << #t << " passed." << std::endl; }

int main()
{
  // RUN_TEST(nc2::test_naive_cube_doesnt_smoke);
  // RUN_TEST(nc2::test_naive_cube_and_nanocube_equivalence_1);
  {
    ENABLE_TRACING;
    RUN_TEST(nc2::test_naive_cube_and_nanocube_equivalence_2);
  }
}
