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
"""AI Streams Ingest CLI."""

import logging
import os
import pathlib
import shlex

import aistreams.python.apps as apps
import aistreams.python.cli.util as util
import click

_INGESTION_APP_NAME = "ingester_app"


@click.command(name="ingest")
@click.option(
    "--target-address",
    "-t",
    required=True,
    type=str,
    default="localhost:50051")
@click.option("--ssl-root-cert-path", type=str)
@click.option("--ssl-domain-name", type=str, default="aistreams.googleapis.com")
@click.option("--authenticate-with-google", "-a", is_flag=True)
@click.option("--use-insecure-channel", "-u", is_flag=True)
@click.option("--stream-name", "-s", required=True, type=str)
@click.option("--source-uri", "-i", required=True, type=str)
@click.option("--loop", "-l", is_flag=True)
@click.option("--trace-probability", type=float, default=0)
def cli(target_address, ssl_root_cert_path, ssl_domain_name,
        authenticate_with_google, use_insecure_channel, stream_name, source_uri,
        loop, trace_probability):
  """Ingest a video source into a stream."""
  ingester_app_config = {
      "app_path":
          os.path.join(
              pathlib.Path(apps.__file__).parents[0], _INGESTION_APP_NAME),
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
      "loop":
          util.to_cpp_bool_string(loop),
      "source_uri":
          util.normalize_string_for_commandline(source_uri),
      "use_insecure_channel":
          util.to_cpp_bool_string(use_insecure_channel),
      "trace_probability": trace_probability,
  }

  ingester_app_cmd = ("{app_path} "
                      "--target_address={target_address} "
                      "--authenticate_with_google={authenticate_with_google} "
                      "--ssl_root_cert_path={ssl_root_cert_path} "
                      "--ssl_domain_name={ssl_domain_name} "
                      "--stream_name={stream_name} "
                      "--loop={loop} "
                      "--source_uri={source_uri} "
                      "--trace_probability={trace_probability} "
                      "--use_insecure_channel={use_insecure_channel} ".format(
                          **ingester_app_config))
  logging.debug("Executing command %s.", ingester_app_cmd)

  ingester_app_tokens = shlex.split(ingester_app_cmd)
  os.execlp(ingester_app_tokens[0], *ingester_app_tokens)
