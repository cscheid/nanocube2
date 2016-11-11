#include "debug.h"

// the return "" is an ugly hack, but hey.
inline std::string node_id(std::ostream &os, int i, int dim)
{
  os << "\"" << i << "_" << dim << "\"";
  return "";
}

void print_dot_ncdim(std::ostream &os, const NCDim &dim, int d, bool draw_next)
{
  os << " subgraph cluster" << d << " {\n";
  os << " label=\"Dim. " << d << "\";\n";
  for (size_t i=0; i<dim.nodes.values.size(); ++i) {
    if (dim.nodes.ref_counts[i] > 0) {
      os << "  " << node_id(os, i, d);
      os << " [label=\"" << i << ":" << dim.nodes.values[i].next << "\"]";
      os << ";\n";
    }
  }
  os << " }\n";
  for (size_t i=0; i<dim.nodes.values.size(); ++i) {
    const NCDimNode &node = dim.nodes.values.at(i);
    if (node.left != -1) {
      os << "  " << node_id(os, i, d) << " -> " << node_id(os, node.left, d) << " [label=\"0\"];\n";
    }
    if (node.right != -1) {
      os << "  " << node_id(os, i, d) << " -> " << node_id(os, node.right, d) << " [label=\"1\"];\n";
    }
  }
}
