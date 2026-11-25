#!/bin/bash
# Helper functions for installer tests

verify_bootstrap_downloads() (
  local mock_dir bootstrap_shell python_executable tool
  mock_dir=$(mktemp -d)
  trap 'rm -rf -- "$mock_dir"' EXIT
  bootstrap_shell=$(command -v bash)
  python_executable=$(command -v python3)
  for tool in git mktemp rm; do
    ln -s "$(command -v "$tool")" "$mock_dir/$tool"
  done

  cat > "$mock_dir/downloader" <<'EOF'
#!/bin/bash
head_request=0
follow_redirects=0
destination=
while [[ $# -gt 0 ]]; do
  case "$1" in
    --head|--method=HEAD) head_request=1 ;;
    --location) follow_redirects=1 ;;
    -o|-qO) shift; destination=$1 ;;
  esac
  shift
done
if [[ "$MOCK_DOWNLOAD" == redirect && "$0" == */curl && $follow_redirects == 0 ]]; then
  exit 0
fi
if [[ $head_request == 1 ]]; then
  [[ "$MOCK_DOWNLOAD" != head-failure ]]
  exit
fi
if [[ "$MOCK_DOWNLOAD" == empty ]]; then
  exit 0
fi
script='import sys
print("installer arguments: " + repr(sys.argv[1:]))'
printf '%s\n' "$script" > "$destination"
if [[ "$MOCK_DOWNLOAD" == get-failure ]]; then
  exit 18
fi
EOF
  chmod +x "$mock_dir/downloader"

  local downloader outcome output status
  for downloader in curl wget; do
    ln -s "$mock_dir/downloader" "$mock_dir/$downloader"
    for outcome in success redirect head-failure get-failure empty; do
      status=0
      output=$(PATH="$mock_dir" MOCK_DOWNLOAD="$outcome" \
        PYTHON_EXECUTABLE="$python_executable" INSTALL_PY_PATH= \
        INSTALL_PY_URL="http://example.invalid/install.py" \
        "$bootstrap_shell" utils/install.sh 'two words' --flag 2>&1) || status=$?
      case "$outcome:$status:$output" in
        success:0:*"installer arguments: ['two words', '--flag']"*|\
        redirect:0:*"installer arguments: ['two words', '--flag']"*) continue ;;
        *"installer arguments:"*) ;;
        *-failure:1:*"Could not download"*|empty:1:*"Could not download"*) continue ;;
      esac
      echo "Unexpected bootstrap result for $downloader ($outcome)"
      echo "$output"
      exit 1
    done
    rm "$mock_dir/$downloader"
  done
)

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
