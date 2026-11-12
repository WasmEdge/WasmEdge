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
