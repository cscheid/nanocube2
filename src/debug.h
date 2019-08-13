#ifndef DEBUG_H_
#define DEBUG_H_

#include <rang.hpp>

namespace nc2 {

struct TraceBlock
{
  TraceBlock();
  ~TraceBlock();
};

struct EnableTracing
{
  EnableTracing();
  ~EnableTracing();
};

struct DisableTracing
{
  DisableTracing();
  ~DisableTracing();
};

int get_debug_indent();
bool tracing_enabled();

};

#ifdef NTRACE

#define TRACE(v)

#define TRACE_BLOCK(exp)

#else

#define TRACE(v) {                                              \
  if (tracing_enabled())                                        \
    std::cerr << std::string(get_debug_indent(), ' ')           \
              << rang::style::dim                               \
              << __FILE__ << ":" << __LINE__ << " "             \
              << rang::style::reset                             \
              << #v                                             \
              << rang::style::dim << ": "                       \
              << rang::style::reset << (v) << std::endl;        \
}

#define TRACE_BLOCK(exp)                                \
  if (tracing_enabled())                                \
    std::cerr << std::string(get_debug_indent(), ' ')   \
              << rang::style::dim                       \
              << __FILE__ << ":" << __LINE__ << " "     \
              << rang::style::reset                     \
              << exp << std::endl;                      \
  TraceBlock __trace_block_hope_we_do_not_clash_shrug;

#define ENABLE_TRACING nc2::EnableTracing __enable_tracing_hope_we_do_not_clash_shrug;
#define DISABLE_TRACING nc2::DisableTracing __disable_tracing_hope_we_do_not_clash_shrug;

#endif

#endif
