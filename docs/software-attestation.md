# Software Attestation and Supply Chain Integrity in WasmEdge

## Summary

This document describes the current state of software attestation in WasmEdge, identifies gaps, and outlines integration options for supply chain attestations. It addresses the CNCF TAG-Security request in issue #4246.

## Current Status

WasmEdge does not currently implement software attestation or supply chain attestation in this repository. Attestation is expected to be provided by the surrounding platform and build pipeline.

## Existing Security Artifacts and Controls

WasmEdge already publishes artifacts and follows practices that can support a future attestation workflow:

- SBOM: `LICENSE.spdx` is the software bill of materials for the project.
- Dependency scanning: FOSSA is used for license compliance.
- CI and review: pull requests require review and CI checks.
- DCO sign-off is required for contributions.
- OSS-Fuzz integration is used to identify vulnerabilities in the codebase.

These practices provide inputs for supply chain provenance but do not themselves constitute attestation.

## Scope and Threat Model (Target Use Cases)

Potential attestation goals include:

- Verifying software running on edge devices or in distributed environments.
- Ensuring the integrity of WasmEdge build outputs and release artifacts.
- Providing supply chain provenance for downstream users and integrators.

Any attestation implementation should define:

- The assets to be attested (binaries, container images, plugins, or SBOMs).
- The identity of the signer or issuer.
- The verification policy and trust roots used by consumers.

## Integration Options

WasmEdge can integrate with existing attestation ecosystems without inventing a new format:

- **in-toto** for supply chain metadata and verification of build steps.
- **SLSA** for defining provenance requirements and levels of build assurance.
- **SCITT** for transparent and auditable distribution of attestation statements.

These frameworks can be layered on the build and release process to produce signed provenance and attestations for WasmEdge artifacts.

## Deployment Guidance

Until native attestation is implemented, recommended guidance is:

- Use platform-level attestation for edge devices or container runtimes.
- Verify WasmEdge artifacts using signatures and SBOMs supplied by the build system.
- Store and validate provenance data alongside release artifacts.

## Future Work

- Define an explicit attestation threat model for WasmEdge.
- Decide which artifacts require attestations and what format to adopt.
- Integrate provenance generation into release pipelines.
- Publish verification guidance for downstream consumers.

## References

- in-toto: https://github.com/in-toto/in-toto
- SLSA: https://slsa.dev/
- SCITT: https://datatracker.ietf.org/group/scitt/about/

Last updated: 2026-02-05
