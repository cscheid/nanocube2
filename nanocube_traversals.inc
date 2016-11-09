// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

//template <typename Summary>
//Summary ortho_range_query(const Nanocube<Summary> &nc,
                          //const vector<pair<int64_t, int64_t> > &range,
                          //bool insert_partial_overlap,
                          //int dim, int index)
//{
  //if (dim == 0) {
    //index = nc.base_root;
  //}

  //std::vector<std::pair<int, int> > result;
  //query_range(nc.dims.at(dim), index,
                //range.at(dim).first, range.at(dim).second, nc.dims.at(dim).width,
                //result,
                //insert_partial_overlap);

  //Summary summary = Summary();
  //if (dim == nc.dims.size() - 1) {
    //for (size_t i=0; i<result.size(); ++i)
      //summary += nc.summaries.at(nc.dims.at(dim).at(result[i].first).next);
  //} else {
    //for (size_t i=0; i<result.size(); ++i)
      //summary += ortho_range_query(nc, range, insert_partial_overlap, dim+1,
                                   //nc.dims.at(dim).at(result[i].first).next);
  //}
  //return summary;
//}

/* Local Variables:  */
/* mode: c++         */
/* End:              */
