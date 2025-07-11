"""Utility methods for cslo."""

import multiprocessing
import multiprocessing.pool
import os
import subprocess  # noqa: S404
from pathlib import Path

from slomanlogger import SlomanLogger

_DEFAULT_LOGGER: SlomanLogger = SlomanLogger(__name__)


def _is_file_exp_error(slo_file: Path) -> bool:
    """Checks if the file is expected to error or not.

    Args:
        slo_file (Path): the file to check

    Returns:
        bool: whether the file is expected to error
    """
    with open(slo_file, encoding="utf-8") as o_f:
        for line in o_f.readlines()[:5]:
            if line == "# slo: exp error\n":
                return True
    return False


def run_slo_file(file: Path) -> str:
    """Function for running a slo file.

    Args:
        file (Path): the file to run.

    Raises:
        FileNotFoundError: if the cslo binary is not found.
        PermissionError: if the cslo binary is not executable.

    Returns:
        str: the output of the file
    """
    binary_path = Path("./build/cslo")
    _DEFAULT_LOGGER.debug("Binary path is: %s", binary_path)
    if not binary_path.is_file():
        _DEFAULT_LOGGER.error("cslo binary not found at %s", binary_path)
        raise FileNotFoundError
    if not os.access(binary_path, os.X_OK):
        _DEFAULT_LOGGER.error("cslo binary at %s is not executable", binary_path)
        raise PermissionError

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


def run_single_test(slo_file: Path, check_output: bool, logger: SlomanLogger = _DEFAULT_LOGGER) -> bool:
    """Wrapper for running a single test.

    Will run the file, check the output, handle errors, etc.
    Returns true or false if the test passed.

    Args:
        slo_file (Path): the slo test file
        check_output (bool): whether to check the output of the file contents
        logger (SlomanLogger, optional): logger to use. Defaults to _DEFAULT_LOGGER.

    Returns:
        bool: true if the test passed, false if it didn't
    """
    logger.verbose("Found SLO file: %s", slo_file)

    expected_error: bool = _is_file_exp_error(slo_file)
    logger.verbose("'%s' exp error status: %s", slo_file, expected_error)
    try:
        output = run_slo_file(slo_file)
        if check_output and not check_slo_output(slo_file, output):
            logger.error("Output didn't match expected output for %s.", slo_file)
            return False
    except KeyboardInterrupt:
        logger.warning("Execution interrupted by user.")
        logger.debug("Last file was: %s", slo_file)
        return False
    except subprocess.CalledProcessError as e:
        if expected_error:
            logger.warning("Expected error for %s: %s", slo_file, e)
            return True
        # If the error is not expected, we log it
        logger.exception("Unexpected error for %s", slo_file)
        return False
    else:
        if expected_error:
            # we didn't error when we should have
            logger.error("File DIDN'T error when expected to: %s", slo_file)
            return False
        logger.verbose("Executed: %s\n", slo_file)
        return True


def runner(
    directory: Path,
    check_output: bool = False,
    use_multiprocess: bool = False,
    logger: SlomanLogger = _DEFAULT_LOGGER,
) -> tuple[list[Path], list[Path]]:
    """Runs all the slo files in the given directory.

    Args:
        directory (Path): the directory to search for slo files.
        check_output (bool, optional): whether to check the output of the files. Defaults to False.
        use_multiprocess (bool): whether to use multiprocessing to speed up the tests. Defaults to False.
        logger (logging.Logger, optional): the logger to use. Defaults to _DEFAULT_LOGGER.

    Returns:
        tuple[list[Path], list[Path]]: a tuple containing the list of passed and failed files.
    """
    failed_files: list[Path] = []
    passed_files: list[Path] = []

    if not use_multiprocess:
        for slo_file in directory.rglob("*.slo"):
            result = run_single_test(slo_file, check_output, logger)
            if result:
                passed_files.append(slo_file)
            else:
                failed_files.append(slo_file)
    else:
        with multiprocessing.pool.Pool(4) as pool:
            inputs = [[slo_file, check_output] for slo_file in directory.rglob("*.slo")]
            pool_results = pool.starmap(run_single_test, inputs)
            for idx, result in enumerate(pool_results):
                slo_file = inputs[idx][0]
                if result:
                    passed_files.append(slo_file)
                else:
                    failed_files.append(slo_file)

    return passed_files, failed_files
