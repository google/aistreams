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

#include "aistreams/port/gmock.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/util/status/canonical_errors.h"
#include "aistreams/util/status/source_location.h"
#include "aistreams/util/status/statusor.h"

namespace aistreams {

namespace {
using ::google::LogSeverity;
using ::google::LogSink;
using ::testing::_;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Test;

// We use `#line` to produce some `source_location` values pointing at various
// different (fake) files to test e.g. `VLog`, but we use it at the end of this
// file so as not to mess up the source location data for the whole file.
// Making them static data members lets us forward-declare them and define them
// at the end.
struct Locs {
  static const SourceLocation kSecret;
  static const SourceLocation kLevel0;
  static const SourceLocation kLevel1;
  static const SourceLocation kLevel2;
  static const SourceLocation kFoo;
  static const SourceLocation kBar;
};

// Converts a StatusBuilder to a Status.
Status ToStatus(const StatusBuilder& s) { return s; }

// Converts a StatusBuilder to a Status and then ignores it.
void ConvertToStatusAndIgnore(const StatusBuilder& s) {
  Status status = s;
  (void)status;
}

// Converts a StatusBuilder to a StatusOr<T>.
template <typename T>
StatusOr<T> ToStatusOr(const StatusBuilder& s) {
  return s;
}

class MockLogSink : public LogSink {
 public:
  MOCK_METHOD(void, send,
              (LogSeverity severity, const char* full_filename,
               const char* base_filename, int line, const struct ::tm* tm_time,
               const char* message, size_t message_len),
              (override));
};

class StatusBuilderTest : public Test {
 protected:
  void SetUp() override {
    mock_log_sink_ = std::make_unique<MockLogSink>();
    ::google::AddLogSink(mock_log_sink_.get());
  }

  void TearDown() override {
    ::google::RemoveLogSink(mock_log_sink_.get());
    mock_log_sink_.reset();
  }

  std::unique_ptr<MockLogSink> mock_log_sink_ = nullptr;
};

TEST_F(StatusBuilderTest, Size) {
  EXPECT_LE(sizeof(StatusBuilder), 40)
      << "Relax this test with caution and thorough testing. If StatusBuilder "
         "is too large it can potentially blow stacks, especially in debug "
         "builds. See the comments for StatusBuilder::Rep.";
}

TEST_F(StatusBuilderTest, ExplicitSourceLocation) {
  const SourceLocation kLocation = AIS_LOC;

  {
    const StatusBuilder builder(OkStatus(), kLocation);
    EXPECT_THAT(builder.source_location().file_name(),
                Eq(kLocation.file_name()));
    EXPECT_THAT(builder.source_location().line(), Eq(kLocation.line()));
  }
}

TEST_F(StatusBuilderTest, ImplicitSourceLocation) {
  const StatusBuilder builder(OkStatus());
  auto loc = AIS_LOC;
  EXPECT_THAT(builder.source_location().file_name(),
              AnyOf(Eq(loc.file_name()), Eq("<source_location>")));
  EXPECT_THAT(builder.source_location().line(),
              AnyOf(Eq(1), Eq(loc.line() - 1)));
}

TEST_F(StatusBuilderTest, StatusCode) {
  // OK
  {
    const StatusBuilder builder(StatusCode::kOk);
    EXPECT_TRUE(builder.ok());
    EXPECT_THAT(builder.code(), Eq(StatusCode::kOk));
  }
  // Non-OK code
  {
    const StatusBuilder builder(StatusCode::kInvalidArgument);
    EXPECT_FALSE(builder.ok());
    EXPECT_THAT(builder.code(), Eq(StatusCode::kInvalidArgument));
  }
}

TEST_F(StatusBuilderTest, Streaming) {
  EXPECT_THAT(ToStatus(StatusBuilder(CancelledError(), Locs::kFoo) << "booyah"),
              Eq(CancelledError("[/foo/foo.cc:1337] booyah")));
  EXPECT_THAT(
      ToStatus(StatusBuilder(AbortedError("hello"), Locs::kFoo) << "world"),
      Eq(AbortedError("hello; [/foo/foo.cc:1337] world")));
}

TEST_F(StatusBuilderTest, PrependLvalue) {
  {
    StatusBuilder builder(CancelledError(), SourceLocation());
    EXPECT_THAT(ToStatus(builder.SetPrepend() << "booyah"),
                Eq(CancelledError("booyah")));
  }
  {
    StatusBuilder builder(AbortedError(" hello"), SourceLocation());
    EXPECT_THAT(ToStatus(builder.SetPrepend() << "world"),
                Eq(AbortedError("world hello")));
  }
}

TEST_F(StatusBuilderTest, PrependRvalue) {
  EXPECT_THAT(
      ToStatus(StatusBuilder(CancelledError(), SourceLocation()).SetPrepend()
               << "booyah"),
      Eq(CancelledError("booyah")));
  EXPECT_THAT(
      ToStatus(
          StatusBuilder(AbortedError(" hello"), SourceLocation()).SetPrepend()
          << "world"),
      Eq(AbortedError("world hello")));
}

TEST_F(StatusBuilderTest, AppendLvalue) {
  {
    StatusBuilder builder(CancelledError(), SourceLocation());
    EXPECT_THAT(ToStatus(builder.SetAppend() << "booyah"),
                Eq(CancelledError("booyah")));
  }
  {
    StatusBuilder builder(AbortedError("hello"), SourceLocation());
    EXPECT_THAT(ToStatus(builder.SetAppend() << " world"),
                Eq(AbortedError("hello world")));
  }
}

TEST_F(StatusBuilderTest, AppendRvalue) {
  EXPECT_THAT(
      ToStatus(StatusBuilder(CancelledError(), SourceLocation()).SetAppend()
               << "booyah"),
      Eq(CancelledError("booyah")));
  EXPECT_THAT(
      ToStatus(
          StatusBuilder(AbortedError("hello"), SourceLocation()).SetAppend()
          << " world"),
      Eq(AbortedError("hello world")));
}

TEST_F(StatusBuilderTest, LogToMultipleErrorLevelsLvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(1);
  EXPECT_CALL(*mock_log_sink_,
              send(::google::ERROR, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("yes!"), _))
      .Times(1);
  EXPECT_CALL(*mock_log_sink_,
              send(::google::INFO, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("Oui!"), _))
      .Times(1);
  {
    StatusBuilder builder(CancelledError(), Locs::kSecret);
    ConvertToStatusAndIgnore(builder.Log(::google::WARNING) << "no!");
  }
  {
    StatusBuilder builder(AbortedError(""), Locs::kSecret);

    ConvertToStatusAndIgnore(builder.Log(::google::ERROR) << "yes!");

    // This one shouldn't log because vlogging is disabled.
    FLAGS_v = 0;
    ConvertToStatusAndIgnore(builder.VLog(2) << "Non!");

    FLAGS_v = 2;
    ConvertToStatusAndIgnore(builder.VLog(2) << "Oui!");
  }
}

TEST_F(StatusBuilderTest, LogToMultipleErrorLevelsRvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(1);
  EXPECT_CALL(*mock_log_sink_,
              send(::google::ERROR, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("yes!"), _))
      .Times(1);
  EXPECT_CALL(*mock_log_sink_,
              send(::google::INFO, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("Oui!"), _))
      .Times(1);
  ConvertToStatusAndIgnore(
      StatusBuilder(CancelledError(), Locs::kSecret).Log(::google::WARNING)
      << "no!");
  ConvertToStatusAndIgnore(
      StatusBuilder(AbortedError(""), Locs::kSecret).Log(::google::ERROR)
      << "yes!");
  // This one shouldn't log because vlogging is disabled.
  FLAGS_v = 0;
  ConvertToStatusAndIgnore(
      StatusBuilder(AbortedError(""), Locs::kSecret).VLog(2) << "Non!");
  FLAGS_v = 2;
  ConvertToStatusAndIgnore(
      StatusBuilder(AbortedError(""), Locs::kSecret).VLog(2) << "Oui!");
}

TEST_F(StatusBuilderTest, LogEveryNFirstLogs) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(1);

  StatusBuilder builder(CancelledError(), Locs::kSecret);
  ConvertToStatusAndIgnore(builder.LogEveryN(::google::WARNING, 3) << "no!");
}

TEST_F(StatusBuilderTest, LogEveryN2Lvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(3);

  StatusBuilder builder(CancelledError(), Locs::kSecret);
  // Only 3 of the 6 should log.
  for (int i = 0; i < 6; ++i) {
    ConvertToStatusAndIgnore(builder.LogEveryN(::google::WARNING, 2) << "no!");
  }
}

TEST_F(StatusBuilderTest, LogEveryN3Lvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(2);

  StatusBuilder builder(CancelledError(), Locs::kSecret);
  // Only 2 of the 6 should log.
  for (int i = 0; i < 6; ++i) {
    ConvertToStatusAndIgnore(builder.LogEveryN(::google::WARNING, 3) << "no!");
  }
}

TEST_F(StatusBuilderTest, LogEveryN7Lvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(3);

  StatusBuilder builder(CancelledError(), Locs::kSecret);
  // Only 3 of the 20 should log.
  for (int i = 0; i < 20; ++i) {
    ConvertToStatusAndIgnore(builder.LogEveryN(::google::WARNING, 7) << "no!");
  }
}

TEST_F(StatusBuilderTest, LogEveryNRvalue) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr(Locs::kSecret.file_name()), _,
                   Locs::kSecret.line(), _, HasSubstr("no!"), _))
      .Times(2);

  // Only 2 of the 4 should log.
  for (int i = 0; i < 4; ++i) {
    ConvertToStatusAndIgnore(StatusBuilder(CancelledError(), Locs::kSecret)
                                 .LogEveryN(::google::WARNING, 2)
                             << "no!");
  }
}

TEST_F(StatusBuilderTest, LogIncludesFileAndLine) {
  EXPECT_CALL(*mock_log_sink_,
              send(::google::WARNING, HasSubstr("/foo/secret.cc"), _,
                   Locs::kSecret.line(), _, HasSubstr("maybe?"), _))
      .Times(1);
  ConvertToStatusAndIgnore(
      StatusBuilder(AbortedError(""), Locs::kSecret).Log(::google::WARNING)
      << "maybe?");
}

TEST_F(StatusBuilderTest, NoLoggingLvalue) {
  EXPECT_CALL(*mock_log_sink_, send(_, _, _, _, _, _, _)).Times(0);

  {
    StatusBuilder builder(AbortedError(""), Locs::kSecret);
    EXPECT_THAT(ToStatus(builder << "nope"),
                Eq(AbortedError("[/foo/secret.cc:1337] nope")));
  }
  {
    StatusBuilder builder(AbortedError(""), Locs::kSecret);
    // Enable and then disable logging.
    EXPECT_THAT(
        ToStatus(builder.Log(::google::WARNING).SetNoLogging() << "not at all"),
        Eq(AbortedError("[/foo/secret.cc:1337] not at all")));
  }
}

TEST_F(StatusBuilderTest, NoLoggingRvalue) {
  EXPECT_CALL(*mock_log_sink_, send(_, _, _, _, _, _, _)).Times(0);
  EXPECT_THAT(
      ToStatus(StatusBuilder(AbortedError(""), Locs::kSecret) << "nope"),
      Eq(AbortedError("[/foo/secret.cc:1337] nope")));
  // Enable and then disable logging.
  EXPECT_THAT(ToStatus(StatusBuilder(AbortedError(""), Locs::kSecret)
                           .Log(::google::WARNING)
                           .SetNoLogging()
                       << "not at all"),
              Eq(AbortedError("[/foo/secret.cc:1337] not at all")));
}

TEST_F(StatusBuilderTest, EmitStackTracePlusSomethingLikelyUniqueLvalue) {
  EXPECT_CALL(
      *mock_log_sink_,
      send(::google::ERROR, HasSubstr("/bar/baz.cc"), _, Locs::kSecret.line(),
           _, HasSubstr("EmitStackTracePlusSomethingLikelyUniqueLvalue"), _))
      .Times(1);
  StatusBuilder builder(AbortedError(""), Locs::kBar);
  ConvertToStatusAndIgnore(builder.LogError().EmitStackTrace() << "maybe?");
}

TEST_F(StatusBuilderTest, EmitStackTracePlusSomethingLikelyUniqueRvalue) {
  EXPECT_CALL(
      *mock_log_sink_,
      send(::google::ERROR, HasSubstr("/bar/baz.cc"), _, Locs::kSecret.line(),
           _, HasSubstr("EmitStackTracePlusSomethingLikelyUniqueRvalue"), _))
      .Times(1);
  ConvertToStatusAndIgnore(
      StatusBuilder(AbortedError(""), Locs::kBar).LogError().EmitStackTrace()
      << "maybe?");
}

TEST_F(StatusBuilderTest, WithRvalueRef) {
  auto policy = [](StatusBuilder sb) { return sb << "policy"; };
  EXPECT_THAT(
      ToStatus(
          StatusBuilder(AbortedError("hello"), Locs::kLevel0).With(policy)),
      Eq(AbortedError("hello; [/tmp/level0.cc:1234] policy")));
}

TEST_F(StatusBuilderTest, WithRef) {
  auto policy = [](StatusBuilder sb) { return sb << "policy"; };
  StatusBuilder sb(AbortedError("zomg"), Locs::kLevel1);
  EXPECT_THAT(ToStatus(sb.With(policy)),
              Eq(AbortedError("zomg; [/tmp/level1.cc:1234] policy")));
}

TEST_F(StatusBuilderTest, WithTypeChange) {
  auto policy = [](StatusBuilder sb) -> std::string {
    return sb.ok() ? "true" : "false";
  };
  EXPECT_EQ(StatusBuilder(CancelledError(), SourceLocation()).With(policy),
            "false");
  EXPECT_EQ(StatusBuilder(OkStatus(), SourceLocation()).With(policy), "true");
}

TEST_F(StatusBuilderTest, WithVoidTypeAndSideEffects) {
  StatusCode code;
  auto policy = [&code](Status status) { code = status.code(); };
  StatusBuilder(CancelledError(), SourceLocation()).With(policy);
  EXPECT_EQ(StatusCode::kCancelled, code);
  StatusBuilder(OkStatus(), SourceLocation()).With(policy);
  EXPECT_EQ(StatusCode::kOk, code);
}

struct MoveOnlyAdaptor {
  std::unique_ptr<int> value;
  std::unique_ptr<int> operator()(const Status&) && { return std::move(value); }
};

TEST_F(StatusBuilderTest, WithMoveOnlyAdaptor) {
  StatusBuilder sb(AbortedError("zomg"), SourceLocation());
  EXPECT_THAT(sb.With(MoveOnlyAdaptor{std::make_unique<int>(100)}),
              Pointee(100));
  EXPECT_THAT(StatusBuilder(AbortedError("zomg"), SourceLocation())
                  .With(MoveOnlyAdaptor{std::make_unique<int>(100)}),
              Pointee(100));
}

template <typename T>
std::string ToStringViaStream(const T& x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

TEST_F(StatusBuilderTest, StreamInsertionOperator) {
  Status status = AbortedError("zomg");
  StatusBuilder builder(status, SourceLocation());
  EXPECT_EQ(ToStringViaStream(status), ToStringViaStream(builder));
  EXPECT_EQ(ToStringViaStream(status),
            ToStringViaStream(StatusBuilder(status, SourceLocation())));
}

TEST(WithExtraMessagePolicyTest, AppendsToExtraMessage) {
  // The policy simply calls operator<< on the builder; the following examples
  // demonstrate that, without duplicating all of the above tests.
  EXPECT_THAT(ToStatus(StatusBuilder(AbortedError("hello"), Locs::kLevel2)
                           .With(ExtraMessage("world"))),
              Eq(AbortedError("hello; [/tmp/level2.cc:1234] world")));
  EXPECT_THAT(ToStatus(StatusBuilder(AbortedError("hello"), Locs::kLevel2)
                           .With(ExtraMessage() << "world")),
              Eq(AbortedError("hello; [/tmp/level2.cc:1234] world")));
  EXPECT_THAT(ToStatus(StatusBuilder(AbortedError("hello"), Locs::kLevel2)
                           .With(ExtraMessage("world"))
                           .With(ExtraMessage("!"))),
              Eq(AbortedError("hello; [/tmp/level2.cc:1234] world!")));
  EXPECT_THAT(ToStatus(StatusBuilder(AbortedError("hello"), Locs::kLevel2)
                           .With(ExtraMessage("world, "))
                           .SetPrepend()),
              Eq(AbortedError("world, hello")));

  // The above examples use temporary StatusBuilder rvalues; verify things also
  // work fine when StatusBuilder is an lvalue.
  StatusBuilder builder(AbortedError("hello"), Locs::kLevel2);
  EXPECT_THAT(
      ToStatus(builder.With(ExtraMessage("world")).With(ExtraMessage("!"))),
      Eq(AbortedError("hello; [/tmp/level2.cc:1234] world!")));
}

#line 1337 "/foo/secret.cc"
const SourceLocation Locs::kSecret = SourceLocation::current();
#line 1234 "/tmp/level0.cc"
const SourceLocation Locs::kLevel0 = SourceLocation::current();
#line 1234 "/tmp/level1.cc"
const SourceLocation Locs::kLevel1 = SourceLocation::current();
#line 1234 "/tmp/level2.cc"
const SourceLocation Locs::kLevel2 = SourceLocation::current();
#line 1337 "/foo/foo.cc"
const SourceLocation Locs::kFoo = SourceLocation::current();
#line 1337 "/bar/baz.cc"
const SourceLocation Locs::kBar = SourceLocation::current();
}  // namespace

}  // namespace aistreams
