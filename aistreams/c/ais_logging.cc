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
#include "aistreams/c/ais_logging.h"

#include "aistreams/port/logging.h"

void AIS_Log(AIS_LogLevel level, const char* message) {
  if (level < AIS_INFO || level > AIS_FATAL) return;
  switch (level) {
    case AIS_INFO:
      LOG(INFO) << message;
      break;
    case AIS_WARNING:
      LOG(WARNING) << message;
      break;
    case AIS_ERROR:
      LOG(ERROR) << message;
      break;
    case AIS_FATAL:
      LOG(FATAL) << message;
      break;
  }
}
