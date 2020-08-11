// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/util/status/status_builder.h"

#include "aistreams/port/gtest.h"

namespace aistreams {

TEST(StatusBuilder, AnnotateMode) {
  ::aistreams::Status status =
      StatusBuilder(::aistreams::Status(::aistreams::StatusCode::kNotFound,
                                        "original message"),
                    AISTREAMS_LOC)
      << "annotated message1 "
      << "annotated message2";
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), ::aistreams::StatusCode::kNotFound);
  EXPECT_EQ(status.error_message(),
            "original message; annotated message1 annotated message2");
}

TEST(StatusBuilder, PrependMode) {
  ::aistreams::Status status =
      StatusBuilder(
          ::aistreams::Status(::aistreams::StatusCode::kInvalidArgument,
                              "original message"),
          AISTREAMS_LOC)
          .SetPrepend()
      << "prepended message1 "
      << "prepended message2 ";
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), ::aistreams::StatusCode::kInvalidArgument);
  EXPECT_EQ(status.error_message(),
            "prepended message1 prepended message2 original message");
}

TEST(StatusBuilder, AppendMode) {
  ::aistreams::Status status =
      StatusBuilder(::aistreams::Status(::aistreams::StatusCode::kInternal,
                                        "original message"),
                    AISTREAMS_LOC)
          .SetAppend()
      << " extra message1"
      << " extra message2";
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), ::aistreams::StatusCode::kInternal);
  EXPECT_EQ(status.error_message(),
            "original message extra message1 extra message2");
}

TEST(StatusBuilder, NoLoggingMode) {
  ::aistreams::Status status =
      StatusBuilder(::aistreams::Status(::aistreams::StatusCode::kUnavailable,
                                        "original message"),
                    AISTREAMS_LOC)
          .SetNoLogging()
      << " extra message";
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), ::aistreams::StatusCode::kUnavailable);
  EXPECT_EQ(status.error_message(), "original message");
}

}  // namespace aistreams
