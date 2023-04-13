import argparse
import subprocess
import sys
import os
import shlex

# shlex.split() splits according to shell quoting rules
WASMEDGE = shlex.split(os.getenv("TEST_RUNTIME_EXE", "wasmedge"))

parser = argparse.ArgumentParser()
parser.add_argument("--version", action="store_true")
parser.add_argument("--test-file", action="store")
parser.add_argument("--arg", action="append", default=[])
parser.add_argument("--env", action="append", default=[])
parser.add_argument("--dir", action="append", default=[])

args = parser.parse_args()

if args.version:
    # ensure no args when version is queried
    subprocess.run(WASMEDGE[0:1] + ["--version"])
    sys.exit(0)

TEST_FILE = args.test_file
PROG_ARGS = args.arg
ENV_ARGS = [j for i in args.env for j in ["--env", i]]
DIR_ARGS = [j for i in args.dir for j in ["--dir", i]]

r = subprocess.run(WASMEDGE + ENV_ARGS + DIR_ARGS + [TEST_FILE] + PROG_ARGS)
sys.exit(r.returncode)
