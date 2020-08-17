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

#ifndef AISTREAMS_BASE_PACKET_AS_H_
#define AISTREAMS_BASE_PACKET_AS_H_

#include <utility>

#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

// A PacketAs object is an adapter used to acquire/access the data in a given
// Packet. It provides methods so that the user can do so safely and
// conveniently.
//
// There are two categories of data that can be extracted from a Packet:
// 1. Packet *metadata*: this kind of information is fixed over the lifetime of
//    a Packet and tells information about the payload. Examples include the
//    packet timestamp, and generally anything within and including the
//    packet header itself.
// 2. Packet *value*: this is the actual value of the packet encoded in the
//    payload. Since every packet is of a specific type, one must make sure
//    to interpret the bytes appropriately. The PacketAs infrastructure will
//    make sure this is done correctly on your behalf.
//
// The usage of PacketAs is similar in spirit to StatusOr.
// Every PacketAs adapts the given source packet to a value of type T on
// construction. The adaptation is either successful or not, indicated by
// whether ok() is true. If ok() is false, then you can query why the adaptation
// was unsuccessful by the value of the Status object returned by status().
//
// You may access the packet metadata even when ok() is false. However, you may
// only access the packet value only when ok() is true. Accessing the value when
// ok() is false will CHECK-fail.
template <typename T>
class PacketAs {
 public:
  // Constructs an instance by copying a source packet.
  explicit PacketAs(const Packet&);

  // Constructs an instance by moving in the source packet.
  explicit PacketAs(Packet&&);

  // Return the source packet's timestamp in microseconds.
  //
  // This is counted since the unix epoch is the source Packet is built using
  // MakePacket. However, it can be a different interpretation if the Packet is
  // constructed differently; in this case, check with your source Packet
  // provider.
  int64_t microseconds() const;

  // Return a copy of the source packet's header.
  PacketHeader header() const;

  // Returns true if the value of the source packet is successfully adapted and
  // ready for access as a value of type T.
  bool ok();

  // Returns the status of the packet adaptation.
  Status status() const;

  // Acquire the value of the packet as an object of type T.
  const T& ValueOrDie() const&;
  T& ValueOrDie() &;
  const T&& ValueOrDie() const&&;
  T&& ValueOrDie() &&;

  // Constructs an instance with a default constructed source Packet.
  // ok() will be false.
  PacketAs();

  // PacketAs is copy constructable/assignable if T is.
  PacketAs(const PacketAs<T>&) = default;
  PacketAs<T>& operator=(const PacketAs<T>&) = default;

  // PacketAs is move constructable/assignable if T is.
  PacketAs(PacketAs<T>&&) = default;
  PacketAs<T>& operator=(PacketAs<T>&&) = default;

 private:
  void Adapt();
  void EnsureOk();

  // Status indicating whether the adaptation was successful and why if not.
  Status status_;

  // This is a Packet that holds the same metadata as the source packet.
  // However, its payload may differ and is likely empty.
  Packet packet_;

  // If ok() is true, this holds the source packet value adapted as a value of
  // type T. Otherwise, accessing this value is undefined behavior.
  T value_;
};

// -----------------------------------------------------------
// Implementation details.

template <typename T>
PacketAs<T>::PacketAs() {
  status_ = UnknownError("This is a default constructed PacketAs");
}

template <typename T>
PacketAs<T>::PacketAs(const Packet& packet) : packet_(packet) {
  Adapt();
}

template <typename T>
PacketAs<T>::PacketAs(Packet&& packet) : packet_(std::move(packet)) {
  Adapt();
}

// Only call this during construction.
template <typename T>
void PacketAs<T>::Adapt() {
  Packet hollow_packet;
  *hollow_packet.mutable_header() = packet_.header();
  status_ = Unpack(std::move(packet_), &value_);
  packet_ = hollow_packet;
  return;
}

template <typename T>
bool PacketAs<T>::ok() {
  return status_.ok();
}

template <typename T>
Status PacketAs<T>::status() const {
  return status_;
}

template <typename T>
int64_t PacketAs<T>::microseconds() const {
  return packet_.header().timestamp().seconds() * 1000000 +
         packet_.header().timestamp().nanos() / 1000;
}

template <typename T>
PacketHeader PacketAs<T>::header() const {
  return packet_.header();
}

template <typename T>
void PacketAs<T>::EnsureOk() {
  if (!ok()) {
    LOG(FATAL) << "The PacketAs was not successfully adapted: " << status_;
  }
  return;
}

template <typename T>
const T& PacketAs<T>::ValueOrDie() const& {
  EnsureOk();
  return value_;
}

template <typename T>
T& PacketAs<T>::ValueOrDie() & {
  EnsureOk();
  return value_;
}

template <typename T>
const T&& PacketAs<T>::ValueOrDie() const&& {
  EnsureOk();
  return std::move(value_);
}

template <typename T>
T&& PacketAs<T>::ValueOrDie() && {
  EnsureOk();
  return std::move(value_);
}

}  // namespace aistreams

#endif  // AISTREAMS_BASE_PACKET_AS_H_
