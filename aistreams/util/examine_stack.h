#ifndef AISTREAMS_UTIL_EXAMINE_STACK_H_
#define AISTREAMS_UTIL_EXAMINE_STACK_H_

#include <string>

namespace aistreams {
namespace util {

// Type of function used for printing in stack trace dumping, etc.
// We avoid closures to keep things simple.
typedef void DebugWriter(const char*, void*);

// Dump current stack trace omitting the topmost 'skip_count' stack frames.
void DumpStackTrace(int skip_count, DebugWriter* w, void* arg,
                    bool short_format = false);

// Dump given pc and stack trace.
void DumpPCAndStackTrace(void* pc, void* const stack[], int depth,
                         DebugWriter* writerfn, void* arg,
                         bool short_format = false);

// Return the current stack trace as a string (on multiple lines, beginning with
// "Stack trace:\n").
std::string CurrentStackTrace(bool short_format = false);

}  // namespace util
}  // namespace aistreams

#endif  // AISTREAMS_UTIL_EXAMINE_STACK_H_
