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
  """Execute the manager app for managing clusters in the managed service."""
  manager_app_config = {
      "app_path":
          os.path.join(
              pathlib.Path(apps.__file__).parents[0], _MANAGEMENT_APP_NAME),
      "op_id":
          op_id,
      "target_address":
          util.normalize_string_for_commandline(args["service_address"]),
      "project":
          util.normalize_string_for_commandline(args["project_id"]),
      "use_google_managed_service":
          util.to_cpp_bool_string(True),
  }
  manager_app_tpl = (
      "{app_path} "
      "--op_id={op_id} "
      "--target_address={target_address} "
      "--project={project} "
      "--use_google_managed_service={use_google_managed_service} ")

  if "cluster_name" in args:
    manager_app_config["cluster_name"] = util.normalize_string_for_commandline(
        args["cluster_name"])
    manager_app_tpl += "--cluster_name={cluster_name} "

  if "region" in args:
    manager_app_config["location"] = util.normalize_string_for_commandline(
        args["region"])
    manager_app_tpl += "--location={location} "

  manager_app_cmd = manager_app_tpl.format(**manager_app_config)
  logging.debug("Executing command %s.", manager_app_cmd)
  manager_app_tokens = shlex.split(manager_app_cmd)
  os.execlp(manager_app_tokens[0], *manager_app_tokens)


@click.group(name="cluster")
@click.option(
    "--service-address",
    type=str,
    default="aistreams.googleapis.com",
    help="Address/domain name to the managed service API.")
@click.option(
    "--project-id",
    type=str,
    required=True,
    help="GCP project id of the Google managed service.")
@click.pass_context
def cli(ctx, service_address, project_id):
  """Manage clusters."""
  ctx.ensure_object(dict)
  ctx.obj["service_address"] = service_address
  ctx.obj["project_id"] = project_id


@cli.command()
@click.option("--cluster-name", type=str, required=True, help="Cluster name.")
@click.option("--region", type=str, default="us-central1", help="Region.")
@click.pass_context
def create(ctx, cluster_name, region):
  args = dict(ctx.obj)
  args["cluster_name"] = cluster_name
  args["region"] = region
  _exec_manager_app(3, args)


@cli.command()
@click.option("--cluster-name", type=str, required=True, help="Cluster name.")
@click.option("--region", type=str, default="us-central1", help="Region.")
@click.pass_context
def delete(ctx, cluster_name, region):
  args = dict(ctx.obj)
  args["cluster_name"] = cluster_name
  args["region"] = region
  _exec_manager_app(5, args)


@cli.command(name="list")
@click.pass_context
def list_clusters(ctx):
  args = dict(ctx.obj)
  _exec_manager_app(4, args)
