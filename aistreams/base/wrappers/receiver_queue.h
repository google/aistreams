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

#ifndef AISTREAMS_BASE_WRAPPERS_RECEIVER_QUEUE_H_
#define AISTREAMS_BASE_WRAPPERS_RECEIVER_QUEUE_H_

#include <memory>

#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/producer_consumer_queue.h"

namespace aistreams {

// The ReceiverQueue grants a consumer share to a producer/consumer queue.
template <typename T>
class ReceiverQueue {
 public:
  // Removes the oldest element from the queue and receives it in `elem`. Waits
  // up to `timeout` for the queue to become non-empty.
  bool TryPop(T& elem, absl::Duration timeout);

  // Returns the capacity of the queue.
  int capacity() const;

  // Construct an instance owning a share to the given producer/consumer queue.
  ReceiverQueue(std::shared_ptr<ProducerConsumerQueue<T>>);

  // Copy-control. Movable but not copyable.
  ReceiverQueue() = default;
  ~ReceiverQueue() = default;
  ReceiverQueue(ReceiverQueue&&) = default;
  ReceiverQueue& operator=(ReceiverQueue&&) = default;
  ReceiverQueue(const ReceiverQueue&) = delete;
  ReceiverQueue& operator=(const ReceiverQueue&) = delete;

 private:
  std::shared_ptr<ProducerConsumerQueue<T>> pcqueue_;
};

// ---------------------------------------------------------------------
// Implementation below

template <typename T>
ReceiverQueue<T>::ReceiverQueue(std::shared_ptr<ProducerConsumerQueue<T>> q)
    : pcqueue_(q) {}

template <typename T>
bool ReceiverQueue<T>::TryPop(T& elem, absl::Duration timeout) {
  return pcqueue_->TryPop(elem, timeout);
}

template <typename T>
int ReceiverQueue<T>::capacity() const {
  return pcqueue_->capacity();
}

}  // namespace aistreams

#endif  // AISTREAMS_BASE_WRAPPERS_RECEIVER_QUEUE_H_
