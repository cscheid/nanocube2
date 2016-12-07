#include <iostream>
#include <algorithm>
#include <iterator>
#include <ctime>
#include <typeinfo>

#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include "mongoose.h"
#include "json.hpp"

#include "nanocube.h"
#include "nanocube_traversals.h"
#include "debug.h"

using json = nlohmann::json;


static const char *s_http_port = "8800";
static struct mg_serve_http_opts s_http_server_opts;

static int qtreeLevel = 10;
//static vector<int> schema = {qtreeLevel*2, qtreeLevel*2};
static vector<int> schema = {3, 3};
static Nanocube<int> nc(schema);

// convert lat,lon to quad tree address
int64_t loc2addr(double lat, double lon, int qtreeLevel)
{
  double xd = (lon + M_PI) / (2.0 * M_PI);
  double yd = (log(tan(M_PI / 4.0 + lat / 2.0)) + M_PI) / (2.0 * M_PI);
  //cout << lat << " " << lon << " " << endl;
  int x = xd * (1 << qtreeLevel), y = yd * (1 << qtreeLevel);

  int64_t z = 0; // z gets the resulting Morton Number.

  for (int i = 0; i < sizeof(x) * 8; i++) {// unroll for more speed...
    z |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
  }

  return z;
}

void buildCubes()
{
  cout << "Start building Nanocubes..." << endl;
  using namespace boost::gregorian;
  using namespace boost::posix_time;

  ifstream is("../sample_data/flights_100K.csv.txt");
  string s;

  int i = 0;

  while(std::getline(is, s)) {
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

    if(ori_lat > 85.0511 || ori_lat < -85.0511 ||
        des_lat > 85.0511 || des_lat < -85.0511 ) {
      cerr << "Invalid latitude: " << ori_lat << ", " << des_lat;
      cerr << " (should be in [-85.0511, 85.0511])" << endl;
      continue;
    }

    int64_t d1 = loc2addr(ori_lat, ori_lon, qtreeLevel);
    int64_t d2 = loc2addr(des_lat, des_lon, qtreeLevel);

    //nc.insert(1, {d1, d2});
   
    nc.new_insert(1, {d1, d2});
    
    //nc.new_insert(1, {0, 0});
    //nc.insert(1, {0, 0});
    //if (++i == 10) {
      //return;
    //}

    if (++i % 10000 == 0) {
      //nc.report_size();
      cout << i << endl;
    }
  }
}

void buildTestCubes()
{
  //nc.insert(1, {0, 0});
  //nc.insert(1, {0, 3});
  //nc.insert(1, {1, 0});
  //
  //nc.new_insert(1, {0, 0});
  //nc.new_insert(1, {0, 3});
  //nc.new_insert(1, {1, 0});
  cout << "insert 1,0,0" << endl;
  nc.new_insert(1, {0, 0});
  //nc.new_insert(1, {0, 3});
  cout << "insert 1,1,0" << endl;
  nc.new_insert(1, {1, 0});

  cout << "insert 1,2,0" << endl;
  nc.new_insert(1, {2, 0});
  {
    ofstream os("nc_3.dot");
    print_dot(os, nc);
  }

  //nc.new_insert(1, {3, 0});
  cout << "insert 1,0,0" << endl;
  nc.new_insert(1, {0, 0});
  {
    ofstream os("nc_4.dot");
    print_dot(os, nc);
  }
  //nc.new_insert(1, {0, 1});
  //nc.new_insert(1, {0, 2});
  //nc.new_insert(1, {0, 3});
}

static void handle_query_call(struct mg_connection *c, struct http_message *hm) {

  json q = json::parse(string(hm->body.p, hm->body.len));
  json result = NCQuery(q, nc);

  /* Send result */
  std::string msg_content = result.dump();
  const std::string sep = "\r\n";

  std::stringstream ss;
  ss << "HTTP/1.1 200 OK"             << sep
    << "Content-Type: application/json" << sep
    << "Access-Control-Allow-Origin: *" << sep
    << "Content-Length: %d"             << sep << sep
    << "%s";

  mg_printf(c, ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      if (mg_vcmp(&hm->uri, "/query") == 0) {
        handle_query_call(c, hm); /* Handle RESTful call */
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

  s_http_server_opts.document_root = "./";
  s_http_server_opts.enable_directory_listing = "no";

  clock_t begin = clock();


  // build Gaussian Cubes
  //buildCubes();
  buildTestCubes();
  //nc.dump_internals(true);
  //{
    //ofstream os("nc.dot");
    //print_dot(os, nc);
  //}

  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  cout << "Used Time: " << elapsed_secs << " seconds" << endl;

  printf("Starting server on port %s\n", s_http_port);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }


  mg_mgr_free(&mgr);

  return 0;
}
