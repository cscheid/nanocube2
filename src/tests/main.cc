// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <iostream>
#include <algorithm>
#include <iterator>

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"

using namespace std;

void test_1()
{
  Nanocube<int> nc({1}, true);
  nc.compact();
  nc.dump_internals();
  nc.insert(1, {0});
  nc.compact();
  nc.dump_internals();
  nc.insert(1, {1});
  nc.compact();
  nc.dump_internals();
}

void test_2()
{
  Nanocube<int> nc({2, 2}, true);
  nc.insert(1, {0, 0});
  nc.dump_internals();
  nc.insert(-1, {3, 3});
  nc.dump_internals();
}

void test_3()
{
  Nanocube<int> nc({2,2,2,2}, true);
  nc.insert(1, {0,0,0,0});
  nc.insert(1, {3,3,3,3});
  nc.dump_internals(true);
}


void print_cover(Nanocube<int> &nc, int lo, int hi, int resolution, bool insert_partial_overlap=false)
{
  vector<pair<int, int> > nodes;
  minimal_cover(nc.dims.at(0), nc.base_root, lo, hi, resolution, nodes, insert_partial_overlap);
  for (size_t i=0; i<nodes.size(); ++i) {
    if (i > 0)
      cout << " ";
    cout << "(" << nodes[i].first << "," << nodes[i].second << ")";
  }
  cout << endl;
}

void test_4()
{
  Nanocube<int> nc({5});
  nc.insert(1, {10});
  // nc.dump_internals(true);
  nc.insert(1, {12});
  // // nc.insert(1, {29});
  nc.dump_internals(true);

  print_cover(nc, 0, 32, 1);
  print_cover(nc, 16, 32, 2);
  print_cover(nc, 9, 13, 10);
  print_cover(nc, 9, 13, 1);
  print_cover(nc, 9, 13, 1, true);
}

void test_5()
{
  Nanocube<int> nc({2,2,2,2});
  nc.insert(1, {0,0,0,0});
  nc.insert(1, {3,3,3,3});
  print_dot(cout, nc);
}

void test_6()
{
  Nanocube<int> nc({2,2,2});
  nc.insert(1, {0,0,0});
  {
    ofstream os("1.dot");
    print_dot(os, nc);
  }
  nc.insert(1, {1,1,1});
  {
    ofstream os("2.dot");
    print_dot(os, nc);
  }
  nc.insert(1, {2,2,2});
  {
    ofstream os("3.dot");
    print_dot(os, nc);
  }
  nc.insert(1, {3,3,3});
  {
    ofstream os("4.dot");
    print_dot(os, nc);
  }
}


void test_7()
{
  Nanocube<int> nc({2,2}, true);
  nc.insert(1, {0,0});
  {
    ofstream os("1.dot");
    print_dot(os, nc);
  }
  cout << "--------------------------------------------------------------------------------" << endl;
  nc.insert(1, {1,1});
  {
    ofstream os("2.dot");
    print_dot(os, nc);
  }
  cout << "--------------------------------------------------------------------------------" << endl;
  nc.insert(1, {2,2});
  {
    ofstream os("3.dot");
    print_dot(os, nc);
  }
  cout << "--------------------------------------------------------------------------------" << endl;
  nc.insert(1, {3,3});
  {
    ofstream os("4.dot");
    print_dot(os, nc);
  }
  cout << "--------------------------------------------------------------------------------" << endl;
}

int main(int argc, char **argv)
{
  // test_1();
  // test_2();
  // test_3();
  // test_4();
  // test_5();
  test_6();
  // test_7();
  return 0;
}
