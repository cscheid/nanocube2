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
  Nanocube<int> nc({1});
  nc.insert(1, {0});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("1_1.dot");
    print_dot(os, nc);
  }
  nc.insert(1, {1});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("1_2.dot");
    print_dot(os, nc);
  }
  nc.compact();
}

void test_2()
{
  Nanocube<int> nc({2, 2});
  nc.insert(1, {0, 0});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("2_1.dot");
    print_dot(os, nc);
  }
  nc.insert(-1, {3, 3});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("2_2.dot");
    print_dot(os, nc);
  }
}

void test_3()
{
  Nanocube<int> nc({2,2,2,2});
  nc.insert(1, {0,0,0,0});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("3_1.dot");
    print_dot(os, nc);
  }
  nc.insert(1, {3,3,3,3});
  nc.compact();
  nc.dump_internals();
  {
    ofstream os("3_2.dot");
    print_dot(os, nc);
  }
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

vector<pair<int, int> > random_region(const vector<int> &schema)
{
  vector<int> p1 = random_point(schema), p2 = random_point(schema);
  vector<pair<int, int> > result;
  for (int i=0; i<p1.size(); ++i) {
    result.push_back(make_pair(min(p1[i], p2[i]+1), max(p1[i], p2[i]+1)));
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
  // vector<int> schema {25, 3, 3};
  // // vector<int> schema { 5 };
  // int n = 3;
  // Nanocube<int> nc(schema);
  // for (int j=0; j<10000; ++j) {
  //   vector<int> point(random_point(schema));
  //   // print_vector(point);
  //   nc.insert(1, point);
  //   // if (j <= n) {
  //   //   stringstream ss;
  //   //   ss << j << ".dot";
  //   //   ofstream os(ss.str());
  //   //   print_dot(os, nc);
  //   //   cout << "================================================================================" << endl;
  //   //   nc.dump_internals(true);
  //   //   nc.compact();
  //   //   nc.dump_internals(true);
  //   // }
  //   // if (j == n) {
  //   //   exit(0);
  //   // }
  // }
  // nc.compact();
  // cout << "Summaries:" << nc.summaries.values.size() << endl;
  // cout << endl;
    
  // smoke-property test: adding random points to random schemata doesn't crash
  // a nanocube

  int n_points = 1000;
  int n_tests = 30;
  for (int i=0; i<n_tests; ++i) {
    vector<int> schema = random_schema();
    cout << "Schema: ";
    print_vector(schema);
    Nanocube<int> nc(schema);
    vector<vector<int> > points;
    for (int j=0; j<n_points; ++j) {
      vector<int> point(random_point(schema));
      points.push_back(point);
      nc.insert(1, point);
    }

    // property 0: the root node of a nanocube should have count equal to
    // the number of points inserted
    if (nc.summaries.at(nc.get_summary_index(nc.base_root, 0)) != n_points) {
      cerr << "FAILED PROPERTY 0" << endl;
      exit(1);
    }
    
    // property 1: every node in the nanocube partitions its refinement.
    // in other words: summary(node.left) + summary(node.right) = summary(node.next)
    for (size_t j=0; j<nc.dims.size(); ++j) {
      const NCDim &nc_dim = nc.dims.at(j);
      for (size_t k=0; k<nc_dim.size(); ++k) {
        const NCDimNode &node = nc_dim.at(k);
        if        (node.left == -1 && node.right != -1) {
          if (nc.get_summary_index(k, j) != nc.get_summary_index(node.right, j)) {
            cerr << "FAILED PROPERTY 1-eps for node " << j << "," << k << endl;
            nc.dump_internals(true);
            exit(1);
          }
        } else if (node.left != -1 && node.right == -1) {
          if (nc.get_summary_index(k, j) != nc.get_summary_index(node.left, j)) {
            cerr << "FAILED PROPERTY 1-eps for node " << j << "," << k << endl;
            nc.dump_internals(true);
            exit(1);
          }
        } else if (node.left != -1 && node.right != -1) {
          if (nc.summaries.at(nc.get_summary_index(k, j)) !=
              nc.summaries.at(nc.get_summary_index(node.left, j)) +
              nc.summaries.at(nc.get_summary_index(node.right, j))) {
            cerr << "FAILED PROPERTY 1 for node " << j << "," << k << endl;
            nc.dump_internals(true);
            exit(1);
          }
        }
      }
    }

    // property 2: for every orthogonal range in the nanocube, the result of
    // the range query is equal to the linear scan of the point array
    int n_regions = 100;
    for (int l=0; l<n_regions; ++l) {
      vector<pair<int, int> > region = random_region(schema);
      int count = 0;
      for (size_t j=0; j<points.size(); ++j) {
        bool outside = false;
        for (size_t k=0; k<region.size(); ++k) {
          if (points[j][k] < region[k].first || points[j][k] >= region[k].second) {
            outside = true;
            break;
          }
        }
        if (!outside) {
          ++count;
        }
      }
      int rq = ortho_range_query(nc, region);
      if (count != rq) {
        cerr << "PROPERTY 2 VIOLATED " << var(count) << var(rq) << endl;
        for (size_t j=0; j<region.size(); ++j) {
          cerr << region[j].first << "-" << region[j].second << " ";
        }
        cerr << endl;
        {
          ofstream os("ugh.dot");
          print_dot(os, nc);
        }
        exit(1);
      }
    }
  }
}

/******************************************************************************/

int main(int argc, char **argv)
{
  // smoke_tests();
  property_tests();
}
