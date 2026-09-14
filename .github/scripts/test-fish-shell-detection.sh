#!/usr/bin/env bash
# Exercise fish shell env detection for install_v2.sh and install.py.
# Requires: fish, curl, python3, and network access to download WasmEdge releases.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
# shellcheck source=/dev/null
. "${SCRIPT_DIR}/installer-test-helpers.sh"

TEST_VERSION="${TEST_VERSION:-0.14.1}"
FISH_BIN="$(command -v fish || true)"
ORIG_HOME="${HOME}"
ORIG_PWD="$(pwd)"
TEST_HOME=""

if [ -z "${FISH_BIN}" ]; then
  echo "[FAIL] fish is required but was not found in PATH"
  exit 1
fi

cd "${REPO_ROOT}"

setup_isolated_home() {
  TEST_HOME="$(mktemp -d "${TMPDIR:-/tmp}/wasmedge-fish-test.XXXXXX")"
  export HOME="${TEST_HOME}"
  mkdir -p "${HOME}/.config/fish"
  printf '%s\n' '# wasmedge fish installer test config' >"${HOME}/.config/fish/config.fish"
  touch "${HOME}/.profile" "${HOME}/.bashrc"
  echo "[INFO] Isolated HOME=${HOME}"
}

cleanup_home() {
  export HOME="${ORIG_HOME}"
  if [ -n "${TEST_HOME}" ] && [ -d "${TEST_HOME}" ]; then
    rm -rf "${TEST_HOME}"
  fi
  TEST_HOME=""
}

assert_posix_env_fails_in_fish() {
  local ipath="${1:-$HOME/.wasmedge}"
  local output
  local status
  set +e
  output=$(fish -c "source \"${ipath}/env\"" 2>&1)
  status=$?
  set -e
  if [ "${status}" -eq 0 ]; then
    echo "[FAIL] expected POSIX env to fail under fish"
    echo "${output}"
    exit 1
  fi
  if ! echo "${output}" | grep -Eqi 'case'; then
    echo "[FAIL] expected fish case/switch error when sourcing POSIX env"
    echo "${output}"
    exit 1
  fi
  echo "[PASS] POSIX env fails under fish (baseline bug reproduced)"
}

run_fish_positive() {
  local installer="$1"
  local label="$2"
  local ipath

  setup_isolated_home
  ipath="${HOME}/.wasmedge"

  printf '\n=== %s (SHELL=fish) ===\n' "${label}"
  export SHELL="${FISH_BIN}"

  if [ "${installer}" = "v2" ]; then
    bash "${REPO_ROOT}/utils/install_v2.sh" -V --version="${TEST_VERSION}"
  else
    python3 "${REPO_ROOT}/utils/install.py" -v "${TEST_VERSION}" -D
  fi

  verify_fish_env_generated "${ipath}"
  verify_fish_config_hook "${ipath}"
  assert_posix_env_fails_in_fish "${ipath}"
  verify_fish_env_works "${ipath}"

  bash "${REPO_ROOT}/utils/uninstall.sh" -q -V -p "${ipath}"
  verify_fish_cleanup "${ipath}"

  cleanup_home
}

run_bash_negative() {
  local installer="$1"
  local label="$2"
  local ipath
  local config_before

  setup_isolated_home
  ipath="${HOME}/.wasmedge"
  config_before="$(cat "${HOME}/.config/fish/config.fish")"

  printf '\n=== %s (SHELL=bash, fish installed) ===\n' "${label}"
  export SHELL=/bin/bash

  if [ "${installer}" = "v2" ]; then
    bash "${REPO_ROOT}/utils/install_v2.sh" -V --version="${TEST_VERSION}"
  else
    python3 "${REPO_ROOT}/utils/install.py" -v "${TEST_VERSION}" -D
  fi

  verify_no_fish_artifacts "${ipath}"

  if [ "$(cat "${HOME}/.config/fish/config.fish")" != "${config_before}" ]; then
    echo "[FAIL] config.fish was modified for bash SHELL"
    exit 1
  fi
  echo "[PASS] config.fish unchanged for bash SHELL"

  bash "${REPO_ROOT}/utils/uninstall.sh" -q -V -p "${ipath}"
  cleanup_home
}

trap cleanup_home EXIT

printf '=== Fish shell detection tests ===\n'
printf 'Repo: %s\n' "${REPO_ROOT}"
printf 'fish: %s\n' "$(fish --version)"
printf 'TEST_VERSION: %s\n' "${TEST_VERSION}"

run_fish_positive "v2" "install_v2.sh"
run_fish_positive "py" "install.py"
run_bash_negative "v2" "install_v2.sh"
run_bash_negative "py" "install.py"

trap - EXIT
export HOME="${ORIG_HOME}"
cd "${ORIG_PWD}"

printf '\n=== All fish shell detection tests passed ===\n'
