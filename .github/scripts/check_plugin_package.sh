#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

# Usage: check_plugin_package.sh <archive.tar.gz> <directory holding libwasmedge> [--no-load]
#
# Checks a packaged plug-in:
# 1. The archive holds exactly one shared object (*.so or *.dylib).
# 2. The shared object depends only on libwasmedge and system libraries. With
#    --no-load, the CUDA libraries are also permitted on Linux.
# 3. Without --no-load, a separate process loads it with
#    dlopen(RTLD_NOW|RTLD_LOCAL). The given directory is the only entry of
#    LD_LIBRARY_PATH or DYLD_LIBRARY_PATH.
#
# On macOS, a plug-in from a build tree keeps the run path of that tree. Step 3
# then shows only that it binds and initializes. Step 2 shows that it needs
# nothing else.
#
# Prints the dependency list on stdout. Exit status: 0 for a pass, 1 for a
# failure, with the reason on stderr.

set -euo pipefail

Name=${0##*/}

fail() {
  printf '%s: %s\n' "$Name" "$*" >&2
  exit 1
}

usage() {
  printf 'usage: %s <archive.tar.gz> <directory holding libwasmedge> [--no-load]\n' "$Name" >&2
  exit 1
}

is_allowed() {
  case $1 in
  .. | ../* | */.. | */../*) return 1 ;;
  esac
  case $Platform in
  Linux)
    case $1 in
    */*) return 1 ;;
    'libwasmedge.so'* | 'libc.so'* | 'libm.so'* | 'libdl.so'* | 'libpthread.so'* | \
      'librt.so'* | 'libstdc++.so'* | 'libgcc_s.so'* | 'ld-linux'*)
      return 0
      ;;
    esac
    if [ "$NoLoad" -eq 1 ]; then
      case $1 in
      'libcuda'*'.so'* | 'libcublas'*'.so'* | 'libnvidia'*'.so'*)
        return 0
        ;;
      esac
    fi
    ;;
  Darwin)
    case $1 in
    '@rpath/libwasmedge'*/*) return 1 ;;
    '@rpath/libwasmedge'* | '/usr/lib/'* | '/System/Library/'*) return 0 ;;
    esac
    ;;
  esac
  return 1
}

NoLoad=0
case $# in
2) ;;
3)
  [ "$3" = --no-load ] || usage
  NoLoad=1
  ;;
*) usage ;;
esac

Archive=$1
[ -f "$Archive" ] || fail "no such archive: $Archive"
[ -d "$2" ] || fail "no such directory: $2"
LibDir=$(CDPATH='' cd -- "$2" && pwd) || fail "cannot enter the directory $2"

Platform=$(uname -s)
case $Platform in
Linux) Tool=readelf ToolCall='readelf -d' ;;
Darwin) Tool=otool ToolCall='otool -L' ;;
*) fail "unsupported platform: $Platform" ;;
esac
command -v "$Tool" >/dev/null 2>&1 || fail "$Tool is required and was not found"

Tmp=$(mktemp -d "${TMPDIR:-/tmp}/check_plugin_package.XXXXXX") ||
  fail "cannot create a temporary directory"
trap 'chmod -R u+rwX "$Tmp" 2>/dev/null || true; rm -rf "$Tmp"' EXIT

Unpacked=$Tmp/unpacked
mkdir "$Unpacked"
tar -xzf - -C "$Unpacked" <"$Archive" || fail "cannot unpack $Archive"

Found=$(find "$Unpacked" -type f \( -name '*.so' -o -name '*.dylib' \) | LC_ALL=C sort) ||
  fail "cannot list the files unpacked from $Archive"
Objects=0
SharedObject=
while IFS= read -r File; do
  [ -n "$File" ] || continue
  Objects=$((Objects + 1))
  SharedObject=$File
done <<<"$Found"
[ "$Objects" -gt 0 ] || fail "$Archive holds no shared object (*.so or *.dylib)"
if [ "$Objects" -gt 1 ]; then
  printf '%s: %s holds %d shared objects, expected exactly one:\n' "$Name" "$Archive" "$Objects" >&2
  while IFS= read -r File; do
    printf '  %s\n' "${File#"$Unpacked"/}" >&2
  done <<<"$Found"
  exit 1
fi
Shown=${SharedObject#"$Unpacked"/}

Magic=$(od -An -tx1 -N4 "$SharedObject" | tr -d ' \n') || fail "cannot read $Shown"
Id=
case $Platform in
Linux)
  [ "$Magic" = 7f454c46 ] || fail "$Shown is not an ELF file"
  Listing=$(LC_ALL=C readelf -d "$SharedObject") || fail "readelf cannot read $Shown"
  Dependencies=$(printf '%s\n' "$Listing" |
    sed -n -e '/(NEEDED)/!d' -e 's/^.*Shared library: \[\(.*\)\][[:space:]]*$/\1/' -e p)
  ;;
Darwin)
  case $Magic in
  cffaedfe | cefaedfe | feedfacf | feedface | cafebabe | cafebabf) ;;
  *) fail "$Shown is not a Mach-O file" ;;
  esac
  Listing=$(otool -L "$SharedObject") || fail "otool cannot read $Shown"
  Dependencies=$(printf '%s\n' "$Listing" |
    sed -n -e 's/ (compatibility version [^)]*)$//' -e 's/^[[:space:]][[:space:]]*//p')
  Id=$(otool -D "$SharedObject" | sed -n '2p') || fail "otool cannot read $Shown"
  ;;
esac

printf 'Archive: %s\n' "$Archive"
printf 'Shared object: %s\n' "$Shown"
printf 'Dependencies (%s):\n' "$ToolCall"
Listed=0
Refused=()
while IFS= read -r Dependency; do
  [ -n "$Dependency" ] || continue
  [ "$Dependency" != "$Id" ] || continue
  Listed=$((Listed + 1))
  printf '  %s\n' "$Dependency"
  is_allowed "$Dependency" || Refused+=("$Dependency")
done <<<"$Dependencies"
[ "$Listed" -gt 0 ] || fail "no dependency was listed for $Shown, so the listing was not read"
if [ "${#Refused[@]}" -gt 0 ]; then
  for Dependency in "${Refused[@]}"; do
    printf '%s: not provided by an installed WasmEdge: %s\n' "$Name" "$Dependency" >&2
  done
  exit 1
fi

if [ "$NoLoad" -eq 1 ]; then
  printf 'Load step skipped (--no-load).\n'
  exit 0
fi

command -v cc >/dev/null 2>&1 || fail "cc is required for the load step and was not found"
case $LibDir in
*:*) fail "the directory cannot be a library search path entry, its path has a colon: $LibDir" ;;
esac
cat >"$Tmp/load.c" <<'EOF'
#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char **argv) {
  void *Handle;
  const char *Error;
  if (argc != 2) {
    fprintf(stderr, "usage: %s <shared object>\n", argv[0]);
    return 2;
  }
  Handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (Handle == NULL) {
    Error = dlerror();
    fprintf(stderr, "%s\n", Error != NULL ? Error : "dlopen failed");
    return 1;
  }
  return 0;
}
EOF

Status=0
if [ "$Platform" = Linux ]; then
  Variable=LD_LIBRARY_PATH
  (cd "$Tmp" && cc -o load load.c -ldl) || fail "cc cannot compile the loader"
  printf 'Loading %s with %s=%s\n' "$Shown" "$Variable" "$LibDir"
  (cd "$Tmp" && LD_LIBRARY_PATH=$LibDir ./load "$SharedObject") || Status=$?
else
  Variable=DYLD_LIBRARY_PATH
  (cd "$Tmp" && cc -o load load.c) || fail "cc cannot compile the loader"
  printf 'Loading %s with %s=%s\n' "$Shown" "$Variable" "$LibDir"
  (cd "$Tmp" && DYLD_LIBRARY_PATH=$LibDir ./load "$SharedObject") || Status=$?
fi
[ "$Status" -eq 0 ] || fail "$Shown does not load (the loader exited with status $Status)"
printf '%s loads with %s=%s\n' "$Shown" "$Variable" "$LibDir"
