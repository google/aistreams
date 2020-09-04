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

#ifndef AISTREAMS_C_AIS_C_API_INTERNAL_H_
#define AISTREAMS_C_AIS_C_API_INTERNAL_H_

#include <memory>

#include "aistreams/base/connection_options.h"
#include "aistreams/base/wrappers/receivers.h"
#include "aistreams/base/wrappers/senders.h"
#include "aistreams/c/c_api.h"

struct AIS_ConnectionOptions {
  aistreams::ConnectionOptions connection_options;
};

struct AIS_Sender {
  std::unique_ptr<aistreams::PacketSender> packet_sender = nullptr;
};

struct AIS_Receiver {
  std::unique_ptr<aistreams::ReceiverQueue<aistreams::Packet>> receiver_queue =
      nullptr;
};

#endif  // AISTREAMS_C_AIS_C_API_INTERNAL_H_
