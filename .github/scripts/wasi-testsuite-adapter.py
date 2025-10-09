import subprocess
import os
import shlex
from pathlib import Path
from typing import Dict, List, Tuple

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


def compute_argv(test_path: str,
                 args: List[str],
                 env: Dict[str, str],
                 dirs: List[Tuple[Path, str]],
                 wasi_version: str) -> List[str]:
    argv = [] + WASMEDGE
    for k, v in env.items():
        argv += ["--env", f"{k}={v}"]
    for host, guest in dirs:
        argv += ["--dir", f"{guest}:{host}"]
    argv += [test_path]
    argv += args
    return argv
