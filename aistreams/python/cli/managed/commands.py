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
import aistreams.python.cli.managed.cluster.commands as cluster
import aistreams.python.cli.managed.stream.commands as stream
import click


@click.group(name="managed")
def cli():
  """Manage resources on the Google managed service."""
  pass


cli.add_command(cluster.cli)
cli.add_command(stream.cli)
