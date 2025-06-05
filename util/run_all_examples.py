
import os
import subprocess
from pathlib import Path

expected_errors = [
    "exit.slo",
    "mismatched.slo",
    "for_in_iterating.slo",
    "locals.slo",
]

if __name__ == "__main__":
    # Get the current working directory
    current_dir = Path(os.getcwd())
    example_dir = Path(current_dir, "resources", "tests")
    print(f"Running tests in: {example_dir}")

    failed_files = []
    # walk the directory to find all *.slo files
    for slo_file in example_dir.rglob("*.slo"):
        print(f"Found SLO file: {slo_file}")
        try:
            subprocess.run(["./build/cslo", str(slo_file)], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except KeyboardInterrupt:
            print("Execution interrupted by user.")
            print(f"Last file was: {slo_file}")
            break
        except subprocess.CalledProcessError as e:
            if slo_file.name in expected_errors:
                print(f"Expected error for {slo_file}: {e}")
            else:
                # If the error is not expected, we log it
                print(f"Unexpected error for {slo_file}: {e}")
                failed_files.append(slo_file)
        print(f"Executed: {slo_file}\n")

    if failed_files:
        print("\nSome files failed to execute:")
        for failed_file in failed_files:
            print(f"- {failed_file}")
