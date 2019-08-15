#include <iostream>
#include "satgarbagecube.h"
#include "garbagecube.h"

using namespace std;
using namespace nc2;

int main()
{
  GarbageCube<int> cube({5, 6});

  size_t hour, minute;
  size_t count = 0;
  while (cin >> hour >> minute) {
    cube.insert({hour, minute}, 1);
    if (++count % 1000 == 0) {
      cerr << "Count: " << count << endl;
    }
  }

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


