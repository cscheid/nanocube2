template <typename Summary>
void print_dot(std::ostream &os, const Nanocube<Summary> &nc)
{
  os << "digraph G {\n";
  os << "    splines=line;\n";
  for (int i=0; i<nc.dims.size(); ++i) {
    print_dot_ncdim(os, nc.dims.at(i), i, i < (nc.dims.size()-1));
  }
  os << "}\n";
}

/******************************************************************************/

/* Local Variables:  */
/* mode: c++         */
/* End:              */
