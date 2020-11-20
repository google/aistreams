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
"""AI Streams Playback CLI."""

import logging
import os
import pathlib
import shlex

import aistreams.python.apps as apps
import aistreams.python.cli.util as util
import click

_PLAYBACK_APP_NAME = "playback_app"


@click.command(name="playback")
@click.option(
    "--target-address",
    "-t",
    required=True,
    type=str,
    default="localhost:50051",
    help="The ip:port to the ingress.")
@click.option(
    "--authenticate-with-google",
    "-a",
    is_flag=True,
    help="Pass this if and only if you are using the Google managed service.")
@click.option(
    "--ssl-root-cert-path",
    type=str,
    help="The path to the ssl certificate of the ingress.")
@click.option("--ssl-domain-name", type=str, default="aistreams.googleapis.com")
@click.option("--use-insecure-channel", "-u", is_flag=True)
@click.option(
    "--stream-name",
    "-s",
    required=True,
    type=str,
    help="The name of the stream to playback from.")
@click.option(
    "--receiver-timeout",
    default=15,
    type=int,
    help="The timeout (in seconds) for the server to yield a packet.")
@click.option(
    "--playback-duration",
    default=-1,
    help="Maximum playback duration in seconds. -1 for no constraints.")
@click.option(
    "--output-mp4",
    "-o",
    default="",
    help="If non-empty, save the playback to a mp4 file. Otherwise, render to screen."
)
def cli(target_address, authenticate_with_google, ssl_root_cert_path,
        ssl_domain_name, use_insecure_channel, stream_name, receiver_timeout,
        playback_duration, output_mp4):
  """Playback a stream. The packet type must be convertible to a raw image."""
  playback_app_config = {
      "app_path":
          os.path.join(
              pathlib.Path(apps.__file__).parents[0], _PLAYBACK_APP_NAME),
      "target_address":
          util.normalize_string_for_commandline(target_address),
      "authenticate_with_google":
          util.to_cpp_bool_string(authenticate_with_google),
      "ssl_domain_name":
          util.normalize_string_for_commandline(ssl_domain_name),
      "ssl_root_cert_path":
          util.normalize_string_for_commandline(ssl_root_cert_path),
      "stream_name":
          util.normalize_string_for_commandline(stream_name),
      "use_insecure_channel":
          util.to_cpp_bool_string(use_insecure_channel),
      "timeout_in_sec":
          receiver_timeout,
      "playback_duration_in_sec":
          playback_duration,
      "output_mp4":
          util.normalize_string_for_commandline(output_mp4),
  }

  playback_app_cmd = ("{app_path} "
                      "--target_address={target_address} "
                      "--authenticate_with_google={authenticate_with_google} "
                      "--ssl_root_cert_path={ssl_root_cert_path} "
                      "--ssl_domain_name={ssl_domain_name} "
                      "--stream_name={stream_name} "
                      "--playback_duration_in_sec={playback_duration_in_sec} "
                      "--output_mp4={output_mp4} "
                      "--timeout_in_sec={timeout_in_sec} "
                      "--use_insecure_channel={use_insecure_channel} ".format(
                          **playback_app_config))
  logging.debug("Executing command %s.", playback_app_cmd)

  playback_app_tokens = shlex.split(playback_app_cmd)
  os.execlp(playback_app_tokens[0], *playback_app_tokens)
