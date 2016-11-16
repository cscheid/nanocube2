template <typename Summary>
void Naivecube<Summary>::insert(const Summary &value, const vector<int64_t> &address)
{
  data.push_back(make_pair(address, value));
}

template <typename Summary>
Naivecube<Summary>::Naivecube(const vector<int> &schema) {
  indexDims = schema.size();
  for(int i = 0; i < indexDims; i ++) {
    dimWidth.push_back(schema[i]);
  }
}

template <typename Summary>
json NaiveCubeQuery(json &q, const Naivecube<Summary> &nc)
{
  //////////////////////////////////////////////////////////////////////////////
  // parse the query json object
  //////////////////////////////////////////////////////////////////////////////
  int op; // 0 for find, 1 for split, 2 for range, 3 for all
  int64_t prefix_addr;
  int prefix_depth;
  int resolution;
  int64_t lowerBound_addr;
  int lowerBound_depth;
  int64_t upperBound_addr;
  int upperBound_depth;

  typedef pair<int64_t, int> prefix;

  vector<pair<prefix, prefix> > extents;

  bool hasSplit = false;

  prefix p1, p2;

  for(int i = 0; i < nc.indexDims; i ++) {
    if(q.count(to_string(i)) == 0) {
      prefix p1 = make_pair(0,0);
      prefix p2 = make_pair((int64_t)1 << nc.dimWidth.at(i), nc.dimWidth.at(i));
      extents.push_back(make_pair(p1, p2));
    } else {
      json clause = q[to_string(i)];
      string op_str = clause["operation"];
      if(op_str == "find") op = 0;
      else if(op_str == "split") op = 1;
      else if(op_str == "range") op = 2;
      else if(op_str == "all") op = 3;
      else op = 4;

      switch(op) {
        case 0: prefix_addr = clause["prefix"]["address"];
                prefix_depth = clause["prefix"]["depth"];
                p1 = make_pair(prefix_addr, prefix_depth);
                extents.push_back(make_pair(p1, p1));
                break;
        case 1: prefix_addr = clause["prefix"]["address"];
                prefix_depth = clause["prefix"]["depth"];
                resolution = clause["resolution"];
                // TODO use real split
                p1 = make_pair(0,0);
                p2 = make_pair((int64_t)1 << nc.dimWidth.at(i), 
                                      nc.dimWidth.at(i));
                extents.push_back(make_pair(p1, p2));

                hasSplit = true;
                break;
        case 2: lowerBound_addr = clause["lowerBound"]["address"];
                lowerBound_depth = clause["lowerBound"]["depth"];
                upperBound_addr = clause["upperBound"]["address"];
                upperBound_depth = clause["upperBound"]["depth"];
                p1 = make_pair(lowerBound_addr, lowerBound_depth);
                p2 = make_pair(upperBound_addr, upperBound_depth);
                extents.push_back(make_pair(p1, p2));
                break;
        case 3:
                p1 = make_pair(0,0);
                p2 = make_pair((int64_t)1 << nc.dimWidth.at(i), 
                                      nc.dimWidth.at(i));
                extents.push_back(make_pair(p1, p2));
                break;
        default: return Summary();
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // run the query
  //////////////////////////////////////////////////////////////////////////////
  if (!hasSplit) {
    // no split operation, the result should just be a <Summary> value
    // we just need to scan and sum all valid rows
    Summary sum = Summary();
    for( auto it = nc.data.begin(); it != nc.data.end(); ++it) {
      bool inExtents = true;
      for(int i = 0; i < extents.size(); ++ i) {
        // shift the address for comparision
        int64_t left = it->first[i] >> (nc.dimWidth.at(i)-extents[i].first.second); 
        int64_t right = it->first[i] >> (nc.dimWidth.at(i)-extents[i].second.second);
        if( left >= extents[i].first.first && right <= extents[i].second.first) {
          continue; // in extent
        } else {
          inExtents = false;
          break;
        }
      }
      if (inExtents) {
        sum += it->second;
      }
    }
    return sum;
  } else {
    // TODO extra work for split
    return Summary();
  }
}
