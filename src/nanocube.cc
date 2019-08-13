#include <cstdlib>
#include <algorithm>
#include <utility>
#include <iostream>
#include <iterator>
#include <fstream>

#include "nanocube.h"
#include "naivecube.h" // for test
#include "utils.h"


namespace nc2 {

std::ostream &operator<<(std::ostream &os, const NCDimNode &node)
{
  os << node.left_ << ":" << node.right_ << ":" << node.next_;
  return os;
}

bool test_naive_cube_and_nanocube_equivalence()
{
  std::vector<size_t> widths { 4 };
  for (size_t i=0; i<1000; ++i) {
    NaiveCube<int> naivecube(widths);
    NanoCube<int> nanocube(widths);
    std::vector<size_t> points;

    for (int j=0; j<10; ++j) {
      size_t v = rand() % 16;
      points.push_back(v);
      std::vector<size_t> address { v };
      naivecube.insert(address, 1);
      nanocube.insert(address, 1);
    }

    for (int j=0; j<10; ++j) {
      size_t v1 = rand() % 16, v2 = rand() % 16;

      std::vector<std::pair<size_t, size_t> > range {
        std::make_pair(std::min(v1, v2), std::max(v1, v2))
      };

      CombineSummaryPolicy<int> p1, p2;

      naivecube.range_query(p1, range);
      nanocube.range_query(p2, range);

      if (p1.total != p2.total) {
        std::cerr << "Failure! On dataset:" << std::endl;
        copy(points.begin(), points.end(),
             std::ostream_iterator<size_t>(std::cerr, " "));
        std::cerr << std::endl;
        std::cerr << "with query (" << range[0].first << "," << range[0].second << ")" << std::endl;
        std::cerr << "data structures disagree: naivecube " << p1.total << " vs nanocube " << p2.total << std::endl;

        {
          std::ofstream out("nc.dot");
          nanocube.print_dot(out);
        }
        return false;
      }
    }
  }
  std::cerr << "could not find counterexample" << std::endl;
  return true;
}

bool test_naive_cube_and_nanocube_equivalence_1()
{
  std::vector<size_t> widths { 4 };
  NaiveCube<int> naivecube(widths);
  NanoCube<int> nanocube(widths);
  std::vector<size_t> addresses { 7, 1, 9, 10, 2, 8, 8, 14, 3, 13 };
  for (auto &e: addresses) {
    naivecube.insert({e}, 1);
    nanocube.insert({e}, 1); 
  }

  std::vector<std::pair<size_t, size_t> > range { std::make_pair(5, 8) };

  CombineSummaryPolicy<int> p1, p2;
  naivecube.range_query(p1, range);
  nanocube.range_query(p2, range);

  if (p1.total != p2.total) {
    std::cerr << "Failure! On dataset:" << std::endl;
    copy(addresses.begin(), addresses.end(),
         std::ostream_iterator<size_t>(std::cerr, " "));
    std::cerr << std::endl;
    std::cerr << "with query (" << range[0].first << "," << range[0].second << ")" << std::endl;
    std::cerr << "data structures disagree: naivecube " << p1.total << " vs nanocube " << p2.total << std::endl;
    std::cerr << std::endl;
    nanocube.dump_nc();
    return false;
  }
    {
      std::ofstream out("nc.dot");
      nanocube.print_dot(out);
    }
  return true;
}

};
