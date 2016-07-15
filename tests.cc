// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
using namespace std;

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"

using namespace std;

/******************************************************************************/
// smoke tests

void test_1()
{
  Nanocube<int> nc({1}, true);
  nc.compact();
  nc.insert(1, {0});
  nc.dump_internals();
  {
    ofstream os("1.dot");
    print_dot(os, nc);
  }
  nc.compact();
  nc.insert(1, {1});
  nc.dump_internals();
  {
    ofstream os("2.dot");
    print_dot(os, nc);
  }
  nc.compact();
}

void test_2()
{
  Nanocube<int> nc({2, 2});
  nc.insert(1, {0, 0});
  nc.insert(-1, {3, 3});
}

void test_3()
{
  Nanocube<int> nc({2,2,2,2});
  nc.insert(1, {0,0,0,0});
  nc.insert(1, {3,3,3,3});
}

void smoke_tests()
{
  test_1();
  test_2();
  test_3();
}

/******************************************************************************/
// property tests

// random generation of test data. NOT THREAD-SAFE
float exponential_variate()
{
  typedef boost::mt19937 RNGType;
  static RNGType rng;
  static boost::exponential_distribution<> d( 1.0 );
  static boost::variate_generator< RNGType, boost::exponential_distribution<> >
      gen(rng, d);
  return gen();
}

float uniform_variate()
{
  typedef boost::mt19937 RNGType;
  static RNGType rng;
  static boost::uniform_01<> d;
  static boost::variate_generator< RNGType, boost::uniform_01<> >
      gen(rng, d);
  return gen();
}

vector<int> random_schema()
{
  int length = int(exponential_variate() + 1.0);
  vector<int> result;
  for (int i=0; i<length; ++i) {
    int depth = std::min(int(exponential_variate() * 6.0 + 1.0), 30);
    result.push_back(depth);
  }
  return result;
}

vector<int> random_point(const vector<int> &schema)
{
  vector<int> result;
  for (int i=0; i<schema.size(); ++i) {
    float u = uniform_variate();
    int   r = (1 << schema.at(i));
    int ui = std::max(0, std::min(r - 1, int(u * r)));
    result.push_back(ui);
  }
  return result;
}

Nanocube<int> random_nanocube(bool debug=false)
{
  return Nanocube<int>(random_schema(), debug);
}

template <typename T>
void print_vector(const vector<T> &v)
{
  copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
  cout << endl;
}

void property_tests()
{
  // smoke-property test: adding random points to random schemata doesn't crash
  // a nanocube
  for (int i=0; i<30; ++i) {
    vector<int> schema = random_schema();
    cout << "Schema: ";
    print_vector(schema);
    Nanocube<int> nc(schema);
    for (int j=0; j<1000; ++j) {
      vector<int> point(random_point(schema));
      nc.insert(1, point);
      // cout << ".";
      // cout.flush();
      // if (!nc.validate_refcounts()) {
      //   exit(1);
      // }
    }
    nc.compact();
    cout << "Summaries:" << nc.summaries.values.size() << endl;
    cout << endl;
  }
}

/******************************************************************************/

int main(int argc, char **argv)
{
  // smoke_tests();
  property_tests();
}
