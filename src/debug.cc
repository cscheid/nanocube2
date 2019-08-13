#include "debug.h"

#include <vector>

namespace nc2 {

static int debug_indent = 0;
static std::vector<bool> tracing { false };

TraceBlock::TraceBlock() {
  debug_indent += 2;
}
TraceBlock::~TraceBlock() {
  debug_indent -= 2;
}

EnableTracing::EnableTracing() {
  tracing.push_back(true);
}
EnableTracing::~EnableTracing() {
  tracing.pop_back();
}

DisableTracing::DisableTracing() {
  tracing.push_back(false);
}
DisableTracing::~DisableTracing() {
  tracing.pop_back();
}

int get_debug_indent() { return debug_indent; }

bool tracing_enabled() {
  return tracing.back();
}

};
