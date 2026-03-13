import subprocess
import os
import shlex
import sys
from pathlib import Path
from typing import Dict, List, Tuple
import importlib


# shlex.split() splits according to shell quoting rules
WASMEDGE = shlex.split(os.getenv("WASMEDGE", "wasmedge"))


def get_name() -> str:
    return "wasmedge"


def get_version() -> str:
    # ensure no args when version is queried
    result = subprocess.run(WASMEDGE[0:1] + ["--version"],
                            encoding="UTF-8", capture_output=True,
                            check=True)
    output = result.stdout.splitlines()[0].split(" ")
    return output[1]


def get_wasi_versions() -> List[str]:
    return ["wasm32-wasip1"]


def get_wasi_worlds() -> List[str]:
    return ["wasi:cli/command"]


def compute_argv(test_path: str,
                 args_env_dirs: Tuple[List[str], Dict[str, str], List[Tuple[Path, str]]],
                 proposals: List[str],
                 wasi_world: str,
                 wasi_version: str) -> List[str]:

    argv = []
    argv += WASMEDGE
    args, env, dirs = args_env_dirs

    for k, v in env.items():
        argv += ["--env", f"{k}={v}"]

    for host, guest in dirs:
        argv += ["--dir", f"{guest}:{host}"]

    argv += [test_path]

    argv += args
    return argv
