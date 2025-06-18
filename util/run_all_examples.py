
import os
import subprocess
from pathlib import Path

expected_errors = [
    "exit.slo",
    "mismatched.slo",
    "for_in_iterating.slo",
    "locals.slo",
    "errors.slo",
    "pop.slo",
]

if __name__ == "__main__":
    # Get the current working directory
    current_dir = Path(os.getcwd())
    example_dir = Path(current_dir, "tests", "slo")
    print(f"Running tests in: {example_dir}")

    failed_files = []
    # walk the directory to find all *.slo files
    for slo_file in example_dir.rglob("*.slo"):
        print(f"Found SLO file: {slo_file}")
        try:
            output = subprocess.check_output(["./build/cslo", str(slo_file)], stderr=subprocess.DEVNULL)
            out_path = Path(slo_file.parent, slo_file.name.replace(".slo", ".out"))
            if out_path.exists():
                with open(out_path, "r", encoding="utf8") as exp_output_f:
                    exp_output = exp_output_f.read()
                if output.decode("utf8") != exp_output:
                    print(f"Output didn't match expected output for {slo_file}.")
                    failed_files.append(slo_file)
            # write the output to a file - just used to generate initial expected outputs
            # else:
            #     with open(out_path, "w", encoding="utf8") as out_f:
            #         out_f.write(output.decode("utf8"))
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
