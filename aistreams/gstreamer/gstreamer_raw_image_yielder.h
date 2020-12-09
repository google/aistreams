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

#ifndef AISTREAMS_GSTREAMER_GSTREAMER_RAW_IMAGE_YIELDER_H_
#define AISTREAMS_GSTREAMER_GSTREAMER_RAW_IMAGE_YIELDER_H_

#include <functional>
#include <memory>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/gstreamer/gstreamer_runner.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// This class is responsible for using Gstreamer to yield a sequence of
// RawImages from a stream of GstreamerBuffers of the same caps.
//
// Use calls to Feed() to supply the GstreamerBuffer sequence from which to
// yield RawImages. You can call SignalEOS() to indicate that the sequence is
// done or wait for the destructor to be called to do this (where EOS will be
// sent if you haven't already).
//
// TODO(dschao): Check that the yielded raw image sequence is "maximal".
class GstreamerRawImageYielder {
 public:
  // Options to configure the GstreamerRawImageYielder.
  //
  // `caps_string`: indicates the caps of all fed GstreamerBuffers.
  // `callback`: will be called as soon as a new RawImage is available.
  //
  // The argument passed to the callback can contain a RawImage when no special
  // conditions or errors have been encountered upstream or during decoding.
  // Below are error codes that require special handling:
  // `kNotFound`: This indicates that EOS (end-of-stream) is
  //                       reached. You should quit gracefully.
  using Callback = std::function<Status(StatusOr<RawImage>)>;
  struct Options {
    std::string caps_string;
    Callback callback;
  };

  // Create an instance in a fully initialized state.
  static StatusOr<std::unique_ptr<GstreamerRawImageYielder>> Create(
      const Options&);

  // Feed a GstreamerBuffer into the yielder for processing.
  Status Feed(const GstreamerBuffer&);

  // Signal that no more inputs are to be fed.
  //
  // You may do this yourself if you want your subscribers to be notified
  // earlier. Otherwise, you may wait for the destructor.
  Status SignalEOS();

  // Copy-control members. Use Create() rather than the constructors.
  ~GstreamerRawImageYielder();
  GstreamerRawImageYielder(const Options&);
  GstreamerRawImageYielder() = delete;
  GstreamerRawImageYielder(const GstreamerRawImageYielder&) = delete;
  GstreamerRawImageYielder& operator=(const GstreamerRawImageYielder&) = delete;
  GstreamerRawImageYielder(GstreamerRawImageYielder&&) = delete;
  GstreamerRawImageYielder& operator=(GstreamerRawImageYielder&&) = delete;

 private:
  Status Initialize();

  Options options_;
  bool eos_signaled_ = false;
  std::unique_ptr<GstreamerRunner> gstreamer_runner_ = nullptr;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_RAW_IMAGE_YIELDER_H_
