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

# Verify env.fish was created (Fish shell users)
verify_fish_env() {
  local ipath=${1:-~/.wasmedge}
  if [ -f "$ipath/env.fish" ]; then
    echo "✓ env.fish found: $ipath/env.fish"
  else
    echo "✗ env.fish not found: $ipath/env.fish"
    exit 1
  fi
}

# Verify env.fish was NOT created (non-Fish shell users)
verify_no_fish_env() {
  local ipath=${1:-~/.wasmedge}
  if [ ! -f "$ipath/env.fish" ]; then
    echo "✓ env.fish correctly absent for non-Fish shell"
  else
    echo "✗ env.fish unexpectedly created for non-Fish shell"
    exit 1
  fi
}
