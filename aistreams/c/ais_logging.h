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

#ifndef AISTREAMS_C_AIS_LOGGING_H_
#define AISTREAMS_C_AIS_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// C API for aistreams's logging.

typedef enum AIS_LogLevel {
  AIS_INFO = 0,
  AIS_WARNING = 1,
  AIS_ERROR = 2,
  AIS_FATAL = 3,
} AIS_LogLevel;

// TODO: Make this take a fmt and va_args.
extern void AIS_Log(AIS_LogLevel level, const char* msg);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_LOGGING_H_
