# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""AI Streams CLI."""

import logging
import os
import pathlib
import shlex

import aistreams.python.apps as apps
import aistreams.python.cli.util as util
import click

_CHUNKER_APP_NAME = "chunker_app"


def _exec_chunker_app(args):
  """Execute the chunker app."""
  chunker_app_config = {
      "app_path":
          os.path.join(
              pathlib.Path(apps.__file__).parents[0], _CHUNKER_APP_NAME),
      "max_frames_per_file":
          args["max_frames_per_file"],
      "output_dir":
          util.normalize_string_for_commandline(args["output_dir"]),
      "file_prefix":
          util.normalize_string_for_commandline(args["output_file_prefix"]),
      "upload_to_gcs":
          util.to_cpp_bool_string(args["gcs_bucket_name"]),
      "gcs_bucket_name":
          util.normalize_string_for_commandline(args["gcs_bucket_name"]),
      "gcs_object_dir":
          util.normalize_string_for_commandline(args["gcs_bucket_subdir_name"]),
      "keep_local":
          util.to_cpp_bool_string(args["upload_keep_local"]),
      "use_uri_source":
          util.to_cpp_bool_string("source_uri" in args),
      "working_buffer_size":
          args["working_buffer_size"],
      "finalization_deadline_in_sec":
          args["finalization_deadline_in_sec"],
  }

  chunker_app_tpl = (
      "{app_path} "
      "--max_frames_per_file={max_frames_per_file} "
      "--output_dir={output_dir} "
      "--file_prefix={file_prefix} "
      "--upload_to_gcs={upload_to_gcs} "
      "--gcs_bucket_name={gcs_bucket_name} "
      "--gcs_object_dir={gcs_object_dir} "
      "--keep_local={keep_local} "
      "--use_uri_source={use_uri_source} "
      "--working_buffer_size={working_buffer_size} "
      "--finalization_deadline_in_sec={finalization_deadline_in_sec} ")

  if "source_uri" in args:
    chunker_app_config["source_uri"] = util.normalize_string_for_commandline(
        args["source_uri"])
    chunker_app_tpl += "--source_uri={source_uri} "
  if "gstreamer_input_pipeline" in args:
    chunker_app_config["use_gstreamer_input_source"] = util.to_cpp_bool_string(
        True)
    chunker_app_config[
        "gstreamer_input_pipeline"] = util.normalize_string_for_commandline(
            args["gstreamer_input_pipeline"])
    chunker_app_tpl += "--use_gstreamer_input_source={use_gstreamer_input_source} "
    chunker_app_tpl += "--gstreamer_input_pipeline={gstreamer_input_pipeline} "
  if "receiver_timeout_in_sec" in args:
    chunker_app_config["receiver_timeout_in_sec"] = args[
        "receiver_timeout_in_sec"]
    chunker_app_tpl += "--receiver_timeout_in_sec={receiver_timeout_in_sec} "
  if "authenticate_with_google" in args:
    chunker_app_config["authenticate_with_google"] = util.to_cpp_bool_string(
        args["authenticate_with_google"])
    chunker_app_tpl += "--authenticate_with_google={authenticate_with_google} "
  if "use_insecure_channel" in args:
    chunker_app_config["use_insecure_channel"] = util.to_cpp_bool_string(
        args["use_insecure_channel"])
    chunker_app_tpl += "--use_insecure_channel={use_insecure_channel} "
  if "target_address" in args:
    chunker_app_config[
        "target_address"] = util.normalize_string_for_commandline(
            args["target_address"])
    chunker_app_tpl += "--target_address={target_address} "
  if "ssl_domain_name" in args:
    chunker_app_config[
        "ssl_domain_name"] = util.normalize_string_for_commandline(
            args["ssl_domain_name"])
    chunker_app_tpl += "--ssl_domain_name={ssl_domain_name} "
  if "ssl_root_cert_path" in args:
    chunker_app_config[
        "ssl_root_cert_path"] = util.normalize_string_for_commandline(
            args["ssl_root_cert_path"])
    chunker_app_tpl += "--ssl_root_cert_path={ssl_root_cert_path} "
  if "stream_name" in args:
    chunker_app_config["stream_name"] = util.normalize_string_for_commandline(
        args["stream_name"])
    chunker_app_tpl += "--stream_name={stream_name} "

  chunker_app_cmd = chunker_app_tpl.format(**chunker_app_config)
  logging.debug("Executing command %s.", chunker_app_cmd)
  chunker_app_tokens = shlex.split(chunker_app_cmd)
  os.execlp(chunker_app_tokens[0], *chunker_app_tokens)


@click.group(name="chunk")
@click.option(
    "--max-frames-per-file",
    type=int,
    default=200,
    help="The maximum number of video frames per output file.")
@click.option(
    "--output-dir",
    "-o",
    type=str,
    default="",
    help=("The name of the local directory to save output files into. "
          "Defaults to the current working directory."))
@click.option(
    "--output-file-prefix",
    type=str,
    default="",
    help="Optional prefix to attach to the output video files.")
@click.option(
    "--gcs-bucket-name",
    type=str,
    default="",
    help="The GCS bucket to upload to. Empty means no upload.")
@click.option(
    "--gcs-bucket-subdir-name",
    type=str,
    default="",
    help="The subdirectory in the GCS bucket to upload into.")
@click.option(
    "--upload-keep-local",
    is_flag=True,
    help="If uploading, keep a local copy of the videos.")
@click.option(
    "--working-buffer-size",
    type=int,
    default=100,
    help="Size of the internal work buffers.")
@click.option(
    "--finalization-deadline-in-sec",
    type=int,
    default=5,
    help="The timeout for internal works to finalize their tasks.")
@click.pass_context
def cli(ctx, max_frames_per_file, output_dir, output_file_prefix,
        gcs_bucket_name, gcs_bucket_subdir_name, upload_keep_local,
        working_buffer_size, finalization_deadline_in_sec):
  """Chunk an input video source into video files."""
  ctx.ensure_object(dict)
  ctx.obj["max_frames_per_file"] = max_frames_per_file
  ctx.obj["output_dir"] = output_dir
  ctx.obj["output_file_prefix"] = output_file_prefix
  ctx.obj["gcs_bucket_name"] = gcs_bucket_name
  ctx.obj["gcs_bucket_subdir_name"] = gcs_bucket_subdir_name
  ctx.obj["upload_keep_local"] = upload_keep_local
  ctx.obj["working_buffer_size"] = working_buffer_size
  ctx.obj["finalization_deadline_in_sec"] = finalization_deadline_in_sec


@cli.command()
@click.option("--target-address", "-t", required=True, type=str)
@click.option("--ssl-root-cert-path", "-c", type=str)
@click.option("--ssl-domain-name", type=str, default="aistreams.googleapis.com")
@click.option("--authenticate-with-google", "-a", is_flag=True)
@click.option("--use-insecure-channel", "-u", is_flag=True)
@click.option("--stream-name", "-s", required=True, type=str)
@click.option(
    "--receiver-timeout-in-sec",
    type=int,
    default=15,
    help="The timeout for the stream server to deliver a packet.")
@click.pass_context
def stream(ctx, target_address, ssl_root_cert_path, ssl_domain_name,
           authenticate_with_google, use_insecure_channel, stream_name,
           receiver_timeout_in_sec):
  """Use an AI Streams stream as the video input source."""
  args = dict(ctx.obj)
  args["target_address"] = target_address
  args["ssl_root_cert_path"] = ssl_root_cert_path
  args["ssl_domain_name"] = ssl_domain_name
  args["authenticate_with_google"] = authenticate_with_google
  args["use_insecure_channel"] = use_insecure_channel
  args["stream_name"] = stream_name
  args["receiver_timeout_in_sec"] = receiver_timeout_in_sec
  _exec_chunker_app(args)


@cli.command()
@click.option(
    "--source-uri",
    "-i",
    required=True,
    type=str,
    help=("The uri of the input source. We assume you are providing a file"
          "if no protocol prefix is present."))
@click.pass_context
def uri(ctx, source_uri):
  """Use a URI as the video input source."""
  args = dict(ctx.obj)
  args["source_uri"] = source_uri
  _exec_chunker_app(args)


@cli.command()
@click.option(
    "--input-pipeline",
    "-i",
    required=True,
    type=str,
    help=("A gstreamer pipeline that produces video/x-raw as output. "
          "The produced raw images will be used as the video source."))
@click.pass_context
def gstreamer(ctx, input_pipeline):
  """Use a Gstreamer pipeline as the video input source."""
  args = dict(ctx.obj)
  args["gstreamer_input_pipeline"] = input_pipeline
  _exec_chunker_app(args)
