#ifndef UTILS_H_
#define UTILS_H_

namespace nc2 {

template <typename Summary>
struct CombineSummaryPolicy
{
  Summary total;
  CombineSummaryPolicy(): total() {}
  
  void add(const Summary &v) {
    total += v;
  }
};

};

#ifdef NTRACE
#define TRACE(v)
#else
#define TRACE(v) std::cerr << __FILE__ << ":" << __LINE__ << " " << #v << ": " << (v) << std::endl;
#endif

#endif
