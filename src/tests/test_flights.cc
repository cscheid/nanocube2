// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <iostream>
#include <algorithm>
#include <iterator>
#include <ctime>

#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"
#include "test_utils.h"

int atoi(const std::string &s) { return atoi(s.c_str()); }
double atof(const std::string &s) { return atof(s.c_str()); }

// convert lat,lon to quad tree address
int64_t loc2addr(double lat, double lon, int qtreeLevel)
{
    double xd = (lon + M_PI) / (2.0 * M_PI);
    double yd = (log(tan(M_PI / 4.0 + lat / 2.0)) + M_PI) / (2.0 * M_PI);
    //cout << lat << " " << lon << " " << endl;
    int x = xd * (1 << qtreeLevel), y = yd * (1 << qtreeLevel);

    int64_t z = 0; // z gets the resulting Morton Number.

    for (int i = 0; i < sizeof(x) * 8; i++) // unroll for more speed...
    {
        z |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
    }

    return z;
}

int main(int argc, char **argv)
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    ifstream is(argv[1]);
    std::cout << "Data file: " << argv[1] << std::endl;
    string s;

    int qtreeLevel = 16;

    vector<int> schema = {qtreeLevel*2, qtreeLevel*2};
    // use a quadtree
    Nanocube<int> nc(schema);

    vector<pair<int64_t,int64_t> > dataarray;

    int i = 0;

    clock_t begin = clock();

    while (std::getline(is, s)) {
        vector<string> output;
        boost::split(output,s,boost::is_any_of("\t"));
        if (output.size() != 4) {
            cerr << "Bad line:" << s << endl;
            continue;
        }

        double ori_lat = atof(output[0]) * M_PI / 180.0;
        double ori_lon = atof(output[1]) * M_PI / 180.0;
        double des_lat = atof(output[2]) * M_PI / 180.0;
        double des_lon = atof(output[3]) * M_PI / 180.0;

        if (ori_lat > 85.0511 || ori_lat < -85.0511 ||
            des_lat > 85.0511 || des_lat < -85.0511 ){
            cerr << "Invalid latitude: " << ori_lat << ", " << des_lat;
            cerr << " (should be in [-85.0511, 85.0511])" << endl;
            continue;
        }

        int64_t d1 = loc2addr(ori_lat, ori_lon, qtreeLevel);
        int64_t d2 = loc2addr(des_lat, des_lon, qtreeLevel);

        nc.insert(1, {d1, d2});
        dataarray.push_back({d1,d2});

        if (++i % 10000 == 0) {
            //nc.report_size();
            cout << i << endl;
        }
    }

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    cout << "Running time: " << elapsed_secs << endl;

    //nc.dump_internals(true);
    //{
        ////nc.content_compact();
        //ofstream os("flights.nc");
        //nc.write_to_binary_stream(os);
    //}

    test(nc, dataarray, schema);
}
