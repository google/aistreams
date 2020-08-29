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
#ifndef AISTREAMS_CC_DECODED_RECEIVERS_H_
#define AISTREAMS_CC_DECODED_RECEIVERS_H_

#include <functional>

#include "absl/time/time.h"
#include "aistreams/cc/aistreams_lite.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Create a ReceiverQueue containing Packets of RawImages decoded from a server
// stream specified in `option`.
//
// Of course, not all server streams are decodable as RawImages. Should this
// happen, the returned Status will indicate the reason.
//
// `queue_size`: This is the size of the `receiver_queue` to create.
// `timeout`: This is the amount of time within which the server must yield a
//            new source Packet. If this expires, `receiver_queue` will be given
//            an EOS packet indicating this.
//
// Unlike MakePacketReceiverQueue, decoded RawImages are pushed into
// `receiver_queue` if there is space; otherwise, they are dropped.
//
// TODO: Add some unit tests for this. We need to add a toy/mock server to do
// this thoroughly.
Status MakeDecodedReceiverQueue(const ReceiverOptions& options, int queue_size,
                                absl::Duration timeout,
                                ReceiverQueue<Packet>* receiver_queue);

}  // namespace aistreams

#endif  // AISTREAMS_CC_DECODED_RECEIVERS_H_
