/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AISTREAMS_UTIL_STATUS_STATUS_MATCHERS_H_
#define AISTREAMS_UTIL_STATUS_STATUS_MATCHERS_H_

#include "aistreams/port/gtest.h"
#include "aistreams/util/status/status.h"

#define AISTREAMS_EXPECT_OK(statement) EXPECT_TRUE((statement).ok())
#define AISTREAMS_ASSERT_OK(statement) ASSERT_TRUE((statement).ok())

#endif  // AISTREAMS_UTIL_STATUS_STATUS_MATCHERS_H_
