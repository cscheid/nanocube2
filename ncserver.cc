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
#include "mongoose.h"
//#include "test_utils.h"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static int qtreeLevel = 10;
static vector<int> schema = {qtreeLevel*2, qtreeLevel*2};
static Nanocube<int> nc(schema);
static vector<pair<int64_t,int64_t> > dataarray;

// convert lat,lon to quad tree address
static int64_t loc2addr(double lat, double lon, int qtreeLevel)
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

static void buildCubes()
{
    cout << "Start building Gaussian Cubes..." << endl;
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    ifstream is("../sample_data/flights_100K.csv.txt");
    string s;

    int i = 0;

    while (std::getline(is, s)) {
        vector<string> output;
        boost::split(output,s,boost::is_any_of("\t"));
        if (output.size() != 4) {
            cerr << "Bad line:" << s << endl;
            continue;
        }

        double ori_lat = atof(output[0].c_str()) * M_PI / 180.0;
        double ori_lon = atof(output[1].c_str()) * M_PI / 180.0;
        double des_lat = atof(output[2].c_str()) * M_PI / 180.0;
        double des_lon = atof(output[3].c_str()) * M_PI / 180.0;

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
}

static void handle_sum_call(struct mg_connection *c, struct http_message *hm) {
  // test query
  //test(nc, dataarray, schema);

  char op[10], addr[100], depth[100], resolution[100], lbound[100], ubound[100];
  string result;

  /* Get form variables */
  mg_get_http_var(&hm->body, "op", op, sizeof(op));
  mg_get_http_var(&hm->body, "addr", addr, sizeof(addr));
  mg_get_http_var(&hm->body, "depth", depth, sizeof(depth));
  mg_get_http_var(&hm->body, "resolution", resolution, sizeof(resolution));
  mg_get_http_var(&hm->body, "lbound", lbound, sizeof(lbound));
  mg_get_http_var(&hm->body, "ubound", ubound, sizeof(ubound));

  /* Send headers */
  mg_printf(c, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  /* Compute the result and send it back as a JSON object */
  int operation = (int)strtod(op, NULL);
  switch(operation) {
      case 0: result = QueryTestFind(nc, 0, strtod(addr, NULL), strtod(depth, NULL), 
                                     true, dataarray, schema); break;
      case 1: result = QueryTestSplit(nc, 0, strtod(addr, NULL), strtod(depth, NULL),
                                      strtod(resolution, NULL), true, 
                                      dataarray, schema); break;
      case 2: break;
  }
  mg_printf_http_chunk(c, "{ \"result\": %s }", result.c_str());
  mg_send_http_chunk(c, "", 0); /* Send empty chunk, the end of response */

}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      if (mg_vcmp(&hm->uri, "/query") == 0) {
        handle_sum_call(c, hm); /* Handle RESTful call */
      } 
      else {
        mg_serve_http(c, hm, s_http_server_opts); /* Serve static content */
      }
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {

  struct mg_mgr mgr;
  struct mg_connection *c;

  mg_mgr_init(&mgr, NULL);
  c = mg_bind(&mgr, s_http_port, ev_handler);
  mg_set_protocol_http_websocket(c);

  s_http_server_opts.document_root = "..";  // Serve parent directory
  s_http_server_opts.enable_directory_listing = "yes";

  // build Gaussian Cubes
  buildCubes();

  printf("Starting server on port %s, serving %s\n", s_http_port,
         s_http_server_opts.document_root);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }


  mg_mgr_free(&mgr);

  return 0;
}
