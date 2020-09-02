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

#include "aistreams/c/ais_status.h"

#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/status.h"

using ::aistreams::OkStatus;
using ::aistreams::Status;
using ::aistreams::StatusCode;

AIS_Status* AIS_NewStatus() { return new AIS_Status; }

void AIS_DeleteStatus(AIS_Status* s) { delete s; }

void AIS_SetStatus(AIS_Status* s, AIS_Code code, const char* msg) {
  if (code == AIS_OK) {
    s->status = OkStatus();
    return;
  }
  s->status = Status(static_cast<StatusCode>(code), msg);
}

AIS_Code AIS_GetCode(const AIS_Status* s) {
  return static_cast<AIS_Code>(s->status.code());
}

const char* AIS_Message(const AIS_Status* s) {
  return s->status.error_message().c_str();
}
