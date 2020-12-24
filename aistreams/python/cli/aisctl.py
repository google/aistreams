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
import sys

import aistreams.gstreamer as gst
import aistreams.python.cli.chunk.commands as chunk
import aistreams.python.cli.ingest.commands as ingest
import aistreams.python.cli.managed.commands as managed
import aistreams.python.cli.onprem.commands as onprem
import aistreams.python.cli.playback.commands as playback
import aistreams.python.cli.printer.commands as printer
import click

_LOGGING_FORMAT = "%(levelname)s: %(message)s"


def _set_environment_variables():
  gst_plugin_path = os.path.join(
      pathlib.Path(gst.__file__).parents[0], "gst-plugins")
  logging.debug("Setting GST_PLUGIN_PATH to \"%s\"", gst_plugin_path)
  os.environ["GST_PLUGIN_PATH"] = gst_plugin_path
  os.environ["GLOG_alsologtostderr"] = "1"


@click.group()
@click.option("--verbose", "-v", is_flag=True, help="Verbose output.")
def main(verbose):
  """AI Streams CLI."""
  if verbose:
    logging.basicConfig(format=_LOGGING_FORMAT, level=logging.DEBUG)
  else:
    logging.basicConfig(format=_LOGGING_FORMAT, level=logging.INFO)

  _set_environment_variables()


main.add_command(chunk.cli)
main.add_command(ingest.cli)
main.add_command(managed.cli)
main.add_command(onprem.cli)
main.add_command(playback.cli)
main.add_command(printer.cli)

if __name__ == "__main__":
  # pylint: disable=no-value-for-parameter
  sys.exit(main())
