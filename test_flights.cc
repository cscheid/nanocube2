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

int atoi(const std::string &s) { return atoi(s.c_str()); }
double atof(const std::string &s) { return atof(s.c_str()); }

static const unsigned short MortonTable256[256] = 
{
    0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
    0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
    0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
    0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
    0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
    0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
    0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
    0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
    0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
    0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
    0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
    0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
    0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
    0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
    0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
    0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
    0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
    0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
    0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
    0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
    0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
    0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
    0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
    0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
    0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
    0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
    0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
    0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
    0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
    0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
    0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
    0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

float uniform_variate()
{
    typedef boost::mt19937 RNGType;
    static RNGType rng;
    static boost::uniform_01<> d;
    static boost::variate_generator< RNGType, boost::uniform_01<> > gen(rng, d);
    return gen();
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

// convert lat,lon to quad tree address
int loc2addr(double lat, double lon, int qtreeLevel)
{
    double xd = (lon + M_PI) / (2.0 * M_PI);
    double yd = (log(tan(M_PI / 4.0 + lat / 2.0)) + M_PI) / (2.0 * M_PI);
    //cout << lat << " " << lon << " " << endl;
    unsigned short x = xd * (1 << qtreeLevel), y = yd * (1 << qtreeLevel);

    // turn (x,y) into an address of the quadtree
    // Interleave bits
    // TODO use int64_t
    int z = MortonTable256[y >> 8]   << 17 | 
        MortonTable256[x >> 8]   << 16 |
        MortonTable256[y & 0xFF] <<  1 | 
        MortonTable256[x & 0xFF];

    return z;
}

void test(Nanocube<int> &nc, vector<pair<int,int> > &dataarray, vector<int> &schema)
{
    /**************************************************/
    // Test
    /**************************************************/
    int n_regions = 10;
    for (int l=0; l<n_regions; ++l) {
        vector<pair<int, int> > region = random_region(schema);
        //vector<pair<int, int> > region;
        //region.push_back({500,3200});
        //region.push_back({300,19000});
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
}


int main(int argc, char **argv)
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    ifstream is(argv[1]);
    std::cout << "Data file: " << argv[1] << std::endl;
    string s;

    int qtreeLevel = 15;

    vector<int> schema = {qtreeLevel*2, qtreeLevel*2};
    // use a quadtree
    Nanocube<int> nc(schema);

    vector<pair<int,int> > dataarray;

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

        int d1 = loc2addr(ori_lat, ori_lon, qtreeLevel);
        int d2 = loc2addr(des_lat, des_lon, qtreeLevel);

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

    {
        //nc.content_compact();
        ofstream os("flights.nc");
        nc.write_to_binary_stream(os);
    }

    test(nc, dataarray, schema);
}
