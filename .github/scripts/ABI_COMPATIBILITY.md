# Public function ABI checks

This proposal assumes the versioned-symbol checks from #5443 are available in
`check_symbols.sh`. That script remains responsible for the declared aliases.
The additional gate compares public function signatures and type layouts with
the latest applicable recorded stable release, using an existing Linux x86-64
build. PR and release jobs use the same comparison path; neither rebuilds old
releases or installs Boost.

## Recording

The existing release build adds debug information without changing Release
optimization or LTO. CPack retains its strip policy. `save_abi_baseline.py` saves
the unstripped library and metadata (source, compiler, ELF identity, SONAME and
checksum). Export metadata is retained for inspection, not used as a separate
historical symbol gate.

Recording does not require an older baseline. Enable comparison only after a
normal stable release has produced one. Releases declaring
`.github/abi-baseline-schema` are expected to supply both assets. For the latest
applicable recorded release, missing, duplicate or corrupt assets fail the check;
there is no fallback to an older baseline when the selected one's assets are bad.
Drafts and prereleases are excluded. No historical data recovery is attempted.

## Comparison

The checker downloads exactly one manifest and one type archive. It verifies the
source tag/commit, platform, SONAME and archive library checksum before analysis.
Version ordering handles maintenance candidates without blindly using GitHub's
latest release. Unrecognized versions fail explicitly.

The CI entry point uses `check_abi.py --types-only`: it does not run a separate
export-set comparison. Libabigail and the function identity guard can still
report removed interfaces as part of type analysis. The older symbol-check
entry point remains available for regression fixtures; it is not an additional
CI gate. There is no full-history manifest scan, archive validation or type audit.
This deliberately does not claim compatibility with every older type baseline.

Function identity comparison preserves typedef targets, nested public members,
callbacks and variadic parameters before exact opaque-pointer layout suppressions.
The suppression file is the single source of opaque names, taken from the public
WasmEdge and wasm-c-api declarations; public vector/value containers are excluded.
Non-opaque record names alone are not an identity failure: libabigail judges their
ABI, allowing renamed frozen compatibility structs with unchanged layouts. It complements libabigail rather than replacing layout or
calling-convention analysis. Do not broaden suppressions without regression tests.

## Scope and rollout

The selected Ubuntu job checks before uploading its package and new baseline.
Other platforms and manual publication are outside this gate. The proposal does
not add Debian build or submission automation. Discuss the value and maintenance
cost of the extra type guard separately from #5443's inexpensive symbol checks.

Local tests:

```sh
python3 -B -m unittest discover -s .github/scripts -p 'test_*abi*.py'
```

Script-only edits run the dedicated script tests, not the full Core matrix.
The identity guard resolves typedef and qualifier wrappers to their underlying
types; qualifier compatibility remains libabigail's responsibility. The paired
const-handle test permits a read-only wrapper but still rejects a changed handle.

## Rule checklist

| Rule | Owner / evidence |
| --- | --- |
| Declared default and compatibility aliases must exist | Upstream `check_symbols.sh` (#5443); not duplicated here |
| Select the latest applicable recorded stable version | `select_abi_baselines.py`; draft/newer-version and ordering tests |
| Selected baseline must not silently fall back when missing | `test_abi_flow.py` |
| Manifest schema, tag, source commit, ELF platform and SONAME must match | `test_release_abi.py` |
| Required manifest/archive must each exist exactly once | `test_abi_flow.py` |
| Downloaded library checksum must match | `test_abi_flow.py` |
| Type analysis requires embedded DWARF; tool errors fail | `test_abi_types.py` |
| Same-name by-value to pointer and public layout changes fail | `test_abi_types.py` |
| Opaque identity changes in parameters, returns, typedefs and nested callbacks fail | `test_abi_identity.py` |
| Exact declared opaque pointer private layouts may change | `test_abi_opaque.py`; Compiler/Async/wasm engine included |
| Public value containers are not blanket-suppressed | Exact inventory excludes vector/value containers |
| Graph ID renumbering, sharing and recursion are not ABI changes | `test_abi_signature_graph.py` |
| Callback variadic differences remain visible | `test_abi_signature_graph.py` |
| Renamed frozen old shim with new default passes; broken old layout fails | `test_abi_shim.py` |
| Const wrapping the same handle passes; changing its target fails | `test_abi_qualifiers.py` |
| First baseline can be saved without older data | `test_save_abi_baseline.py` |

These are bounded checks, not a claim of complete C++ ABI, ownership/behavior,
all architectures, all historical releases or Debian package upgrade coverage.
New policy exceptions require paired compatible/incompatible fixtures.

## Function-only boundary

The blocking type gate covers exported functions and their reachable types.
Defined exported data, TLS and indirect-function symbols stop analysis with an
explicit out-of-scope error; they are not silently reported compatible. Undefined
imports and absolute linker version nodes are not data interfaces. No data ABI
analyzer is added. A refusal is a coverage limitation, not proof of an ABI break.
The first version does not promise complete C++ ABI or runtime behavior coverage.
