"""Utility methods for cslo."""

import os
import subprocess  # noqa: S404
from pathlib import Path

from slomanlogger import SlomanLogger

_DEFAULT_LOGGER: SlomanLogger = SlomanLogger(__name__)


def run_slo_file(file: Path) -> str:
    """Function for running a slo file.

    Args:
        file (Path): the file to run.

    Returns:
        str: the output of the file
    """
    binary_path = Path("./build/cslo")
    _DEFAULT_LOGGER.debug("Binary path is: %s", binary_path)
    if not binary_path.is_file():
        _DEFAULT_LOGGER.error("cslo binary not found at %s", binary_path)
        raise FileNotFoundError(f"cslo binary not found at {binary_path}")
    if not os.access(binary_path, os.X_OK):
        _DEFAULT_LOGGER.error("cslo binary at %s is not executable", binary_path)
        raise PermissionError(f"cslo binary at {binary_path} is not executable")

    return subprocess.check_output(["./build/cslo", str(file)], stderr=subprocess.DEVNULL)  # noqa: S603


def check_slo_output(file: Path, actual_output: str) -> bool:
    """Checks the output of the given slo file with the matching .out file.

    Args:
        file (Path): the path to the slo file.
        actual_output (str): the actual output of the slo file.

    Returns:
        bool: whether the output matched or not
    """
    out_path = Path(file.parent, file.name.replace(".slo", ".out"))
    if out_path.exists():
        exp_output = Path(out_path).read_text(encoding="utf8")
        if actual_output.decode("utf8") != exp_output:
            return False
    return True


def runner(
    directory: Path,
    check_output: bool = False,
    expected_errors: list[str] | None = None,
    logger: SlomanLogger = _DEFAULT_LOGGER,
) -> tuple[list[Path], list[Path]]:
    """Runs all the slo files in the given directory.

    Args:
        directory (Path): the directory to search for slo files.
        check_output (bool, optional): whether to check the output of the files. Defaults to False.
        expected_errors (list[str], optional): a list of expected error messages. Defaults to None.
        logger (logging.Logger, optional): the logger to use. Defaults to _DEFAULT_LOGGER.

    Returns:
        tuple[list[Path], list[Path]]: a tuple containing the list of passed and failed files.
    """
    if not expected_errors:
        expected_errors = []

    failed_files: list[Path] = []
    passed_files: list[Path] = []
    for slo_file in directory.rglob("*.slo"):
        logger.verbose("Found SLO file: %s", slo_file)
        try:
            output = run_slo_file(slo_file)
            if check_output and not check_slo_output(slo_file, output):
                logger.error("Output didn't match expected output for %s.", slo_file)
                failed_files.append(slo_file)
        except KeyboardInterrupt:
            logger.warning("Execution interrupted by user.")
            logger.debug("Last file was: %s", slo_file)
            return passed_files, failed_files
        except subprocess.CalledProcessError as e:
            if slo_file.name in expected_errors:
                logger.warning("Expected error for %s: %s", slo_file, e)
            else:
                # If the error is not expected, we log it
                logger.exception("Unexpected error for %s", slo_file)
                failed_files.append(slo_file)
        else:
            passed_files.append(slo_file)
            logger.verbose("Executed: %s\n", slo_file)

    return passed_files, failed_files
