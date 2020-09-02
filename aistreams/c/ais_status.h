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

#ifndef AISTREAMS_C_AIS_STATUS_H_
#define AISTREAMS_C_AIS_STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AIS_Status AIS_Status;

// --------------------------------------------------------------------------
// AIS_Code holds an error code; their values are the same as those in
// aistreams/util/status/status.h.
typedef enum AIS_Code {
  AIS_OK = 0,
  AIS_CANCELLED = 1,
  AIS_UNKNOWN = 2,
  AIS_INVALID_ARGUMENT = 3,
  AIS_DEADLINE_EXCEEDED = 4,
  AIS_NOT_FOUND = 5,
  AIS_ALREADY_EXISTS = 6,
  AIS_PERMISSION_DENIED = 7,
  AIS_RESOURCE_EXHAUSTED = 8,
  AIS_FAILED_PRECONDITION = 9,
  AIS_ABORTED = 10,
  AIS_OUT_OF_RANGE = 11,
  AIS_UNIMPLEMENTED = 12,
  AIS_INTERNAL = 13,
  AIS_UNAVAILABLE = 14,
  AIS_DATA_LOSS = 15,
  AIS_UNAUTHENTICATED = 16,
} AIS_Code;

// --------------------------------------------------------------------------

// Return a new status object.
extern AIS_Status* AIS_NewStatus(void);

// Delete a previously created status object.
extern void AIS_DeleteStatus(AIS_Status*);

// Record the given (code, msg) in *s.
// A common pattern is to clear a status: AIS_SetStatus(s, AIS_OK, "");
extern void AIS_SetStatus(AIS_Status* s, AIS_Code code, const char* msg);

// Return the error code in *s.
extern AIS_Code AIS_GetCode(const AIS_Status* s);

// Return a pointer to error message in *s.
//
// The return value is only usable until the next mutation to *s.
// Returns an empty string if AIS_GetCode(s) is AIS_OK.
extern const char* AIS_Message(const AIS_Status* s);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_STATUS_H_
