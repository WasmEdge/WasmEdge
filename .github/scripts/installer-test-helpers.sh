#!/bin/bash
# Helper functions for installer tests

# Verify installed version matches expected version
verify_version() {
  local expected=$1
  local installed=$(~/.wasmedge/bin/wasmedge --version | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
  echo "Installed version: $installed"

  if [ "$installed" = "$expected" ]; then
    echo "✓ Version matches: $installed"
  else
    echo "✗ Version mismatch: expected $expected, got $installed"
    exit 1
  fi
}

# Verify plugin file exists
verify_plugin() {
  local plugin_file=$1
  if ls ~/.wasmedge/plugin/ | grep -q "$plugin_file"; then
    echo "✓ Plugin found: $plugin_file"
  else
    echo "✗ Plugin not found: $plugin_file"
    exit 1
  fi
}

verify_shell_config() {
  local project="$1"
  local want_change="$2"
  local config=""

  for f in .profile .bashrc .bash_profile .zshrc .zsh_profile .zshenv; do
    if ! [ -f "$HOME/$f" ]; then
      continue
    elif grep -Fq "$project/env" "$HOME/$f"; then
      config="$f"
      break
    fi
  done

  if [ "$want_change" = "1" ]; then
    if [ -z "$config" ]; then
      echo "✗ Want change but got none"
      exit 1
    fi
    echo "✓ shell configured"
  else
    if [ -n "$config" ]; then
      echo "✗ Want no change but got $config"
      exit 1
    fi
    echo "✓ --no-modify-shell-profile works"
  fi
}

# Verify fish-native env.fish was generated with expected syntax
verify_fish_env_generated() {
  local ipath="${1:-$HOME/.wasmedge}"
  local fish_env="$ipath/env.fish"

  if [ ! -f "$fish_env" ]; then
    echo "[FAIL] env.fish not found at: $fish_env"
    exit 1
  fi
  if ! grep -q 'fish_add_path' "$fish_env"; then
    echo "[FAIL] env.fish missing fish_add_path"
    exit 1
  fi
  if ! grep -q 'set -gx' "$fish_env"; then
    echo "[FAIL] env.fish missing set -gx"
    exit 1
  fi
  if grep -q 'case :' "$fish_env"; then
    echo "[FAIL] env.fish contains POSIX case syntax"
    exit 1
  fi
  echo "[PASS] env.fish generated with fish-native syntax"
}

# Verify config.fish sources env.fish
verify_fish_config_hook() {
  local ipath="${1:-$HOME/.wasmedge}"
  local fish_config="$HOME/.config/fish/config.fish"

  if [ ! -f "$fish_config" ]; then
    echo "[FAIL] config.fish not found at: $fish_config"
    exit 1
  fi
  if ! grep -Fq "$ipath/env.fish" "$fish_config"; then
    echo "[FAIL] config.fish missing source hook for $ipath/env.fish"
    exit 1
  fi
  echo "[PASS] config.fish sources env.fish"
}

# Verify fish can source env.fish and run wasmedge
verify_fish_env_works() {
  local ipath="${1:-$HOME/.wasmedge}"
  local fish_env="$ipath/env.fish"
  local output

  if ! command -v fish >/dev/null 2>&1; then
    echo "[FAIL] fish binary not found in PATH"
    exit 1
  fi

  output=$(fish -c "source \"$fish_env\"; wasmedge --version" 2>&1) || {
    echo "[FAIL] fish failed to source env.fish or run wasmedge"
    echo "$output"
    exit 1
  }
  if ! echo "$output" | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' >/dev/null; then
    echo "[FAIL] wasmedge version not found in fish output: $output"
    exit 1
  fi
  echo "[PASS] fish sourced env.fish and ran wasmedge: $output"
}

# Verify bash/zsh install did not create fish artifacts
verify_no_fish_artifacts() {
  local ipath="${1:-$HOME/.wasmedge}"
  local fish_config="$HOME/.config/fish/config.fish"

  if [ -f "$ipath/env.fish" ]; then
    echo "[FAIL] env.fish unexpectedly created for non-fish SHELL"
    exit 1
  fi
  if [ -f "$fish_config" ] && grep -Fq "$ipath/env.fish" "$fish_config"; then
    echo "[FAIL] config.fish unexpectedly hooked for non-fish SHELL"
    exit 1
  fi
  echo "[PASS] no fish artifacts for non-fish SHELL"
}

# Verify uninstall removed env.fish and config.fish hook
verify_fish_cleanup() {
  local ipath="${1:-$HOME/.wasmedge}"
  local fish_config="$HOME/.config/fish/config.fish"

  if [ -f "$ipath/env.fish" ]; then
    echo "[FAIL] env.fish still present after uninstall: $ipath/env.fish"
    exit 1
  fi
  if [ -f "$fish_config" ] && grep -Fq "$ipath/env.fish" "$fish_config"; then
    echo "[FAIL] config.fish still contains env.fish hook after uninstall"
    exit 1
  fi
  echo "[PASS] fish env artifacts cleaned up after uninstall"
}
