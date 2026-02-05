# Secure Update Mechanisms for WasmEdge on Edge Devices

## Summary

This document describes the current status of secure update support for WasmEdge and outlines recommended approaches for integrating secure update frameworks on edge devices. It addresses the CNCF TAG-Security request in issue #4243.

## Current Status

WasmEdge does not provide a built-in update agent or a secure update mechanism in this repository. Updates are expected to be handled by the surrounding platform or deployment system.

## Existing Security Artifacts and Controls

WasmEdge publishes and relies on artifacts that can be used as inputs for secure update systems:

- SBOM: `LICENSE.spdx` is the software bill of materials for the project.
- Dependency scanning: FOSSA is used for license compliance.
- CI and review: pull requests require review and CI checks.
- DCO sign-off is required for contributions.
- OSS-Fuzz integration is used to identify vulnerabilities in the codebase.

These items support provenance and integrity validation but do not provide a secure update workflow by themselves.

## Secure Update Goals

A secure update mechanism for edge devices should provide:

- Integrity and authenticity of update artifacts.
- Protection against rollback and replay attacks.
- Safe handling of partial or failed updates.
- Clear recovery and rollback procedures.
- Auditable update metadata and policies.

## Integration Options

WasmEdge deployments can adopt established secure update frameworks without introducing a new format:

- **TUF** for signed metadata, threshold trust, and rollback protection.
- **Uptane** for automotive and constrained edge scenarios built on TUF.

These frameworks can be integrated at the device or platform level to manage WasmEdge updates.

## Deployment Guidance

Until a native mechanism exists, recommended practices are:

- Use platform update systems that already implement TUF or Uptane.
- Distribute WasmEdge artifacts with signed metadata and verified checksums.
- Separate update policies for core runtime and optional plugins.
- Maintain a tested rollback path for failed updates.

## Future Work

- Define update targets and metadata for WasmEdge releases.
- Decide which keys and trust roots sign update metadata.
- Integrate update metadata generation into release pipelines.
- Publish verification guidance for downstream consumers.

## References

- Uptane: https://uptane.org/
- The Update Framework (TUF): https://theupdateframework.com/
- CNCF TUF Project: https://github.com/theupdateframework/python-tuf

Last updated: 2026-02-05
