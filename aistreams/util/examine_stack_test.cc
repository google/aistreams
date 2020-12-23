#include "aistreams/util/examine_stack.h"

#include "absl/strings/match.h"
#include "aistreams/port/gtest.h"

namespace aistreams {
namespace util {

namespace {
TEST(ExamineStack, CurrentStackTrace) {
  std::string stack_trace = CurrentStackTrace();
  EXPECT_TRUE(
      absl::StrContains(stack_trace, "ExamineStack_CurrentStackTrace_Test"));
}
}  // namespace

}  // namespace util
}  // namespace aistreams
