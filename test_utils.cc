#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <vector>

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"
#include "test_utils.h"

float uniform_variate()
{
    typedef boost::mt19937 RNGType;
    static RNGType rng;
    static boost::uniform_01<> d;
    static boost::variate_generator< RNGType, boost::uniform_01<> > gen(rng, d);
    return gen();
}

vector<int64_t> random_point(const vector<int> &schema)
{
    vector<int64_t> result;
    for (int i=0; i<schema.size(); ++i) {
        double u = uniform_variate();
        int64_t r = ((int64_t)1 << schema.at(i));
        int64_t ui = std::max<int64_t>(0, std::min<int64_t>(r - 1, int64_t(u * r)));
        result.push_back(ui);
    }
    return result;
}

vector<pair<int64_t, int64_t> > random_region(const vector<int> &schema)
{
    vector<int64_t> p1 = random_point(schema), p2 = random_point(schema);
    vector<pair<int64_t, int64_t> > result;
    for (int i=0; i<p1.size(); ++i) {
        result.push_back(make_pair(min(p1[i], p2[i]+1), max(p1[i], p2[i]+1)));
    }
    return result;
}

void test(Nanocube<int> &nc, vector<pair<int64_t,int64_t> > &dataarray, vector<int> &schema)
{
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
}
