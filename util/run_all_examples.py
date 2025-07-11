#!/usr/bin/python

"""Runs all the slo example files."""

import os
from pathlib import Path

from slomanlogger import SlomanLogger

try:
    from util import util
except ImportError:
    import util

LOGGER: SlomanLogger = SlomanLogger(__name__)

if __name__ == "__main__":
    # Get the current working directory
    current_dir = Path(os.getcwd())
    example_dir = Path(current_dir, "examples")
    LOGGER.info("Running examples in: %s", example_dir)

    passed, failed = util.runner(example_dir, use_multiprocess=True, logger=LOGGER)

    if failed:
        LOGGER.error("\nSome files failed to execute:")
        for failed_file in failed:
            LOGGER.warning("- %s", failed_file)
    else:
        LOGGER.info("All files executed successfully.")
