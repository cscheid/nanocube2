#include "utils.h"

namespace nc2 {

std::ostream &operator<<(std::ostream &os, const NCDimNode &node)
{
  os << node.left_ << ":" << node.right_ << ":" << node.next_;
  return os;
}

};
