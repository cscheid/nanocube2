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

    int qtreeLevel = 15;
    int dim3Level = 16;

    vector<int> schema = {qtreeLevel*2, dim3Level};
    // use a quadtree
    //Nanocube<int> nc({qtreeLevel, qtreeLevel, 16});
    Nanocube<int> nc(schema);

    vector<pair<int,int> > dataarray;

    int i = 0;

    clock_t begin = clock();

    while (std::getline(is, s)) {
        vector<string> output;
        boost::split(output,s,boost::is_any_of("\t"));
        if (output.size() != 5) {
            cerr << "Bad line:" << s << endl;
            continue;
        }
        if (atof(output[2]) < -85.0511 || atof(output[2]) > 85.0511) {
            cerr << "Invalid latitude: " << output[2] << " (should be in [-85.0511, 85.0511])" << endl;
            continue;
        }

        string year = output[1].substr(0, 4), 
               month = output[1].substr(5, 2), 
               day = output[1].substr(8, 2),
               hour = output[1].substr(11, 2), 
               minute = output[1].substr(14, 2), 
               sec = output[1].substr(17, 2);

        ptime d(date(atoi(year), atoi(month), atoi(day)), hours(atoi(hour)) + minutes(atoi(minute)) + seconds(atoi(sec)));

        ptime beg_of_time(date(2000, 1, 1));

        double lat = atof(output[2]) * M_PI / 180.0, lon = atof(output[3]) * M_PI / 180.0;
        // turn (x,y) into an address of the quadtree
        // Interleave bits
        int64_t z = loc2addr(lat, lon, qtreeLevel);

        int total_seconds = (d - beg_of_time).total_seconds();
        int t = total_seconds / 3600 / 24;


        nc.insert(1, {z, t});
        dataarray.push_back({z,t});

        if (++i % 10000 == 0) {
            {
                //ofstream os("brightkite.nc");
                //nc.write_to_binary_stream(os);
            }
            //nc.report_size();
            cout << i << endl;
        }
    }

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    cout << "Running time: " << elapsed_secs << endl;

    /**************************************************/
    // Test
    /**************************************************/
    int n_regions = 10;
    for (int l=0; l<n_regions; ++l) {
        vector<pair<int64_t, int64_t> > region = random_region(schema);
        int rq = ortho_range_query(nc, region);
        int count = 0;
        for(int dlength = 0; dlength < dataarray.size(); dlength++) {
            if( (dataarray[dlength].first > region[0].first && 
                        dataarray[dlength].first < region[0].second) && 
                    (dataarray[dlength].second > region[1].first &&
                     dataarray[dlength].second < region[1].second) ) {
                count ++;
            }
        }
        cout << "Query from Nanocubea: "<< rq << " | Linear scan: " << count << endl;
    }

    /**************************************************/
    // Save
    /**************************************************/

    {
        //nc.content_compact();
        //ofstream os("brightkite.nc");
        //nc.write_to_binary_stream(os);
    }

    // std::string s("2001-10-9"); //2001-October-09
    // date d(from_simple_string(s));
}
