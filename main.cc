// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include "nanocube.h"
#include <iostream>

using namespace std;

void test_1()
{
  Nanocube<int> nc({1});
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
  Nanocube<int> nc({2, 2});

  nc.insert(1, {0, 0});
  nc.dump_internals(true);
  nc.insert(1, {3, 3});
  nc.dump_internals(true);
}

int main(int argc, char **argv)
{
  test_2();
  // cout << sorted_array_has_no_duplicates({0, 1, 2, 3, 4}) << endl;
  // cout << sorted_array_has_no_duplicates({0, 1, 2, 2, 4}) << endl;
  return 0;
}
