#include <iostream>

#include <boost/chrono.hpp>

#include "persistentsatcube.h"
#include "sparsesatcube.h"
#include "nanocube.h"
#include "garbagecube.h"

using namespace std;
using namespace nc2;

template <template <typename> class Cube>
void benchmark(const std::vector<std::vector<size_t> > &entries)
{
  Cube<int> cube({5, 6});
  auto start = boost::chrono::steady_clock::now();
  {
    for (auto &e: entries) {
      cube.insert(e, 1);
    }
  }
  auto end = boost::chrono::steady_clock::now();
  typedef boost::chrono::duration<double> sec;  // seconds, stored with a double
  sec d = end - start;
  std::cerr << __PRETTY_FUNCTION__ << "\ntime: " << d << std::endl;
  std::cerr << "storage:" << std::endl;
  cube.report_storage();
  
  {
    CombineSummaryPolicy<int> policy;
    cube.range_query(policy, { std::make_pair(0, 32), std::make_pair(0, 64) });
    cout << "Total Count: " << policy.total << endl;
  }
  {
    CombineSummaryPolicy<int> policy;
    cube.range_query(policy, { std::make_pair(0, 8), std::make_pair(0, 64) });
    cout << "Total Count: " << policy.total << endl;
  }
  {
    CombineSummaryPolicy<int> policy;
    cube.range_query(policy, { std::make_pair(8, 32), std::make_pair(0, 64) });
    cout << "Total Count: " << policy.total << endl;
  }
  
}

template <typename Summary>
struct PersistentSATGarbageCube: public PersistentSATCube<Summary, GarbageCube> {
  explicit PersistentSATGarbageCube(const std::vector<size_t> &widths):
      PersistentSATCube<Summary, GarbageCube>(widths) {}
};

template <typename Summary>
struct PersistentSATNanoCube: public PersistentSATCube<Summary, NanoCube> {
  explicit PersistentSATNanoCube(const std::vector<size_t> &widths):
      PersistentSATCube<Summary, NanoCube>(widths) {}
};

template <typename Summary>
struct SparseSATGarbageCube: public SparseSATCube<Summary, GarbageCube> {
  explicit SparseSATGarbageCube(const std::vector<size_t> &widths):
      SparseSATCube<Summary, GarbageCube>(widths) {}
};

template <typename Summary>
struct SparseSATNanoCube: public SparseSATCube<Summary, NanoCube> {
  explicit SparseSATNanoCube(const std::vector<size_t> &widths):
      SparseSATCube<Summary, NanoCube>(widths) {}
};

int main()
{
  GarbageCube<int> cube({5, 6});

  size_t hour, minute;
  size_t count = 0;
  size_t max_count = 100000;

  std::vector<std::vector<size_t> > entries;
  
  while (cin >> hour >> minute) {
    entries.push_back({hour, minute});
    if (++count == max_count)
      break;
  }

  benchmark<GarbageCube>(entries);
  // benchmark<PersistentSATGarbageCube>(entries);
  benchmark<SparseSATGarbageCube>(entries);
  benchmark<NanoCube>(entries);
  // benchmark<PersistentSATNanoCube>(entries);
  benchmark<SparseSATNanoCube>(entries);
}


