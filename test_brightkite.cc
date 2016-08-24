// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"

int atoi(const std::string &s) { return atoi(s.c_str()); }
double atof(const std::string &s) { return atof(s.c_str()); }

int main(int argc, char **argv)
{
  using namespace boost::gregorian;
  using namespace boost::posix_time;

  ifstream is("/dev/stdin");
  string s;

  Nanocube<int> nc({24, 24, 16});

  int i = 0;
  while (std::getline(is, s)) {
    vector<string> output;
    boost::split(output,s,boost::is_any_of("\t"));
    if (output.size() != 5) {
      cerr << "Bad line:" << s << endl;
    }

    string year = output[1].substr(0, 4), month = output[1].substr(5, 2), day = output[1].substr(8, 2),
           hour = output[1].substr(11, 2), minute = output[1].substr(14, 2), sec = output[1].substr(17, 2);

    ptime d(date(atoi(year), atoi(month), atoi(day)), hours(atoi(hour)) + minutes(atoi(minute)) + seconds(atoi(sec)));

    ptime beg_of_time(date(2000, 1, 1));
    double lat = atof(output[2]) * M_PI / 180.0, lon = atof(output[3]) * M_PI / 180.0;

    int total_seconds = (d - beg_of_time).total_seconds();
    double xd = (lon + M_PI) / (2.0 * M_PI);
    double yd = (log(tan(M_PI / 4.0 + lat / 2.0)) + M_PI) / (2.0 * M_PI);
    // cout << lat << " " << lon << " ";
    int x = xd * (1 << 24), y = yd * (1 << 24);
    
    nc.insert(1, {x, y, total_seconds / 3600});
    if (++i % 100000 == 0) {
      {
        ofstream os("brightkite.nc");
        nc.write_to_binary_stream(os);
      }
      nc.report_size();
    }
  }

  {
    nc.content_compact();
    ofstream os("brightkite_compacted.nc");
    nc.write_to_binary_stream(os);
  }

  // std::string s("2001-10-9"); //2001-October-09
  // date d(from_simple_string(s));
}
