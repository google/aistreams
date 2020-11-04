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

_MANAGEMENT_APP_NAME = "manager_app"


def _exec_manager_app(op_id, args):
  """Configure and execute the manager app for onprem clusters."""
  manager_app_config = {
      "app_path":
          os.path.join(
              pathlib.Path(apps.__file__).parents[0], _MANAGEMENT_APP_NAME),
      "op_id":
          op_id,
      "target_address":
          util.normalize_string_for_commandline(args["target_address"]),
      "ssl_domain_name":
          util.normalize_string_for_commandline(args["ssl_domain_name"]),
      "ssl_root_cert_path":
          util.normalize_string_for_commandline(args["ssl_root_cert_path"]),
      "use_google_managed_service":
          util.to_cpp_bool_string(False),
      "use_insecure_channel":
          util.to_cpp_bool_string(args["use_insecure_channel"]),
  }
  manager_app_tpl = (
      "{app_path} "
      "--op_id={op_id} "
      "--ssl_domain_name={ssl_domain_name} "
      "--ssl_root_cert_path={ssl_root_cert_path} "
      "--target_address={target_address} "
      "--use_google_managed_service={use_google_managed_service} "
      "--use_insecure_channel={use_insecure_channel} ")

  if "stream_name" in args:
    manager_app_config["stream_name"] = util.normalize_string_for_commandline(
        args["stream_name"])
    manager_app_tpl += "--stream_name={stream_name} "

  manager_app_cmd = manager_app_tpl.format(**manager_app_config)
  logging.debug("Executing command %s.", manager_app_cmd)
  manager_app_tokens = shlex.split(manager_app_cmd)
  os.execlp(manager_app_tokens[0], *manager_app_tokens)


@click.group(name="stream")
@click.option(
    "--target-address",
    "-t",
    type=str,
    help="Address (ip:port) to the ingress.")
@click.option(
    "--ssl-root-cert-path",
    type=str,
    help="Path to the ssl cert of the ingress.")
@click.option(
    "--ssl-domain-name",
    type=str,
    default="aistreams.googleapis.com",
    help="Expected ssl domain name of the ingress.")
@click.option(
    "--use-insecure-channel",
    "-u",
    is_flag=True,
    help="Use an insecure channel.")
@click.pass_context
def cli(ctx, target_address, ssl_root_cert_path, ssl_domain_name,
        use_insecure_channel):
  """Manage streams."""
  ctx.ensure_object(dict)
  ctx.obj["target_address"] = target_address
  ctx.obj["ssl_root_cert_path"] = ssl_root_cert_path
  ctx.obj["ssl_domain_name"] = ssl_domain_name
  ctx.obj["use_insecure_channel"] = use_insecure_channel


@cli.command()
@click.option(
    "--stream-name", "-s", type=str, required=True, help="Stream name.")
@click.option(
    "--stream-retention-seconds",
    "-r",
    type=int,
    default=86400,
    help="Stream retention period.")
@click.pass_context
def create(ctx, stream_name, stream_retention_seconds):
  """Create a stream."""
  args = dict(ctx.obj)
  args["stream_name"] = stream_name
  args["stream_retention_seconds"] = stream_retention_seconds
  _exec_manager_app(0, args)


@cli.command()
@click.option(
    "--stream-name", "-s", type=str, required=True, help="Stream name.")
@click.pass_context
def delete(ctx, stream_name):
  """Delete a stream."""
  args = dict(ctx.obj)
  args["stream_name"] = stream_name
  _exec_manager_app(2, args)


@cli.command(name="list")
@click.pass_context
def list_streams(ctx):
  """List all streams."""
  args = dict(ctx.obj)
  _exec_manager_app(1, args)
