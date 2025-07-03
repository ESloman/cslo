#!/usr/bin/python

"""Runs all the slo test files."""

import os
from pathlib import Path

from slomanlogger import SlomanLogger

try:
    from util import util
except ImportError:
    import util

expected_errors: list[str] = [
    "exit.slo",
    "mismatched.slo",
    "for_in_iterating.slo",
    "locals.slo",
    "errors.slo",
    "pop.slo",
]

LOGGER: SlomanLogger = SlomanLogger(__name__)

if __name__ == "__main__":
    # Get the current working directory
    current_dir = Path(os.getcwd())
    test_dir = Path(current_dir, "tests", "slo")
    LOGGER.info("Running tests in: %s", test_dir)

    passed, failed = util.runner(test_dir, expected_errors=expected_errors, logger=LOGGER)

    if failed:
        LOGGER.error("\nSome files failed to execute:")
        for failed_file in failed:
            LOGGER.warning("- %s", failed_file)
    else:
        LOGGER.info("All files executed successfully.")
