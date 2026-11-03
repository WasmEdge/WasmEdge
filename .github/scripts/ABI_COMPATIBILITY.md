# ABI baselines

The existing Ubuntu x86-64 release build adds debug information without changing
its Release optimization or LTO settings. Packaging retains the existing strip
policy. The unstripped build-tree library is saved separately with its source
commit, version, ELF identity, SONAME, exported symbols, checksum and tool versions.

`save_abi_baseline.py LIBRARY VERSION` validates DWARF and a self-comparison before
writing the JSON manifest and archive. It does not need a previous baseline and
does not compile anything. The first normal release establishes the starting
point; no historical debug recovery or Boost dependency is introduced.

Only release jobs publish baselines. PR comparison is intentionally a separate
change, to be enabled after a formal release has supplied a baseline. A draft
release's assets are not yet an approved compatibility contract.

This does not recover missing historical type information. Local historical
rebuilds are regression evidence, not automatically trusted release baselines.
The scope is Linux x86-64, not all release platforms or Debian package upgrades.
