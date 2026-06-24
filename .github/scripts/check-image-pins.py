#!/usr/bin/env python3
"""Monitor container base-image digest pins for drift, staleness, and rot.

The OSSF hardening work pinned every container base image by digest. Those pins
can silently break or fall behind:

* a rolling upstream tag (e.g. quay.io/pypa/manylinux_2_28_*) moves and the
  registry garbage-collects the old digest, so the pinned digest no longer
  resolves and the build fails;
* a mutable version tag (ubuntu:22.04, alpine:3.23) advances to pick up security
  fixes, leaving the static pin behind.

This checker discovers every ``@sha256:`` digest across the Dockerfiles and the
docker-bake ``.hcl`` files, maps each to its ``image:tag``, and classifies it:

* DEAD     - the pinned digest no longer resolves (build-critical).
* MOVED    - a mutable tag now resolves to a different digest (security drift).
* STALE    - a dated tag is more than ``--threshold-days`` behind the newest
             dated tag available in the registry.
* UNMAPPED - a digest was found but could not be mapped to an image:tag. This is
             a coverage gap and is treated as build-critical so a new, unhandled
             pinning pattern can never be monitored silently.

Resolution uses ``docker buildx imagetools inspect`` because that matches exactly
how the build resolves an image. Registry REST APIs are used only to list tags
for the dated-tag staleness check.

The script always exits 0 after writing its report; the calling workflow decides
the job status from the emitted severity counts.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
import urllib.request
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path

DIGEST_RE = re.compile(r"sha256:[0-9a-f]{64}")
REF_WITH_DIGEST_RE = re.compile(r"(?P<ref>[^\s\"']+)@(?P<digest>sha256:[0-9a-f]{64})")
ARG_RE = re.compile(r"^\s*ARG\s+([A-Za-z_][A-Za-z0-9_]*)=(.*?)\s*$")
FROM_RE = re.compile(r"^\s*FROM\s+(.*)$", re.IGNORECASE)
HCL_LOOKUP_RE = re.compile(
    r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*lookup\(\s*\{(.*?)\}", re.DOTALL
)
HCL_PAIR_RE = re.compile(r"\"([^\"]+)\"\s*=\s*\"(sha256:[0-9a-f]{64})\"")

SEVERITY_ORDER = ("DEAD", "UNMAPPED", "MOVED", "STALE", "WARN")


@dataclass(frozen=True)
class Pin:
    image: str
    tag: str | None
    digest: str
    sources: tuple[str, ...]

    def pinned_ref(self) -> str:
        base = f"{self.image}:{self.tag}" if self.tag else self.image
        return f"{base}@{self.digest}"

    def tag_ref(self) -> str:
        return f"{self.image}:{self.tag}" if self.tag else self.image


@dataclass
class Finding:
    severity: str
    pin: Pin
    detail: str


@dataclass
class Inventory:
    sources: dict[tuple[str, str | None, str], set[str]] = field(default_factory=dict)
    unmapped: list[tuple[str, str]] = field(default_factory=list)

    def add(self, image: str, tag: str | None, digest: str, source: str) -> None:
        self.sources.setdefault((image, tag, digest), set()).add(Path(source).name)

    @property
    def pins(self) -> dict[tuple[str, str | None, str], Pin]:
        return {
            key: Pin(key[0], key[1], key[2], tuple(sorted(self.sources[key])))
            for key in self.sources
        }


def strip_platform(ref_line: str) -> str:
    """Drop ``--platform=...`` flags and a trailing ``AS <stage>`` from a FROM body."""
    tokens = ref_line.split()
    tokens = [t for t in tokens if not t.startswith("--platform")]
    if len(tokens) >= 2 and tokens[-2].upper() == "AS":
        tokens = tokens[:-2]
    return tokens[0] if tokens else ""


def expand_vars(value: str, args: dict[str, str]) -> str:
    """Substitute ``${VAR}`` and ``$VAR`` from collected ARG defaults."""

    def repl(match: re.Match[str]) -> str:
        name = match.group(1) or match.group(2)
        return args.get(name, match.group(0))

    return re.sub(
        r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}|\$([A-Za-z_][A-Za-z0-9_]*)", repl, value
    )


def parse_ref(ref: str) -> tuple[str, str | None, str] | None:
    """Split ``image[:tag]@sha256:digest`` into (image, tag, digest)."""
    match = REF_WITH_DIGEST_RE.fullmatch(ref.strip().strip('"').strip("'"))
    if not match:
        return None
    image_part, digest = match.group("ref"), match.group("digest")
    if ":" in image_part.rsplit("/", 1)[-1]:
        image, tag = image_part.rsplit(":", 1)
    else:
        image, tag = image_part, None
    return image, tag, digest


def claim_digests(text: str, claimed: set[str], inv: Inventory, source: str) -> None:
    """Record any digest present in the file but not claimed by a resolved ref."""
    lines = text.splitlines()
    seen: set[str] = set()
    for digest in DIGEST_RE.findall(text):
        if digest in claimed or digest in seen:
            continue
        seen.add(digest)
        line = next(
            (i + 1 for i, ln in enumerate(lines) if digest in ln),
            0,
        )
        inv.unmapped.append((f"{source}:{line}", digest))


def parse_dockerfile(path: Path, inv: Inventory) -> None:
    text = path.read_text()
    lines = text.splitlines()
    args: dict[str, str] = {}
    for line in lines:
        m = ARG_RE.match(line)
        if m:
            args.setdefault(m.group(1), m.group(2).strip().strip('"').strip("'"))

    claimed: set[str] = set()
    for line in lines:
        m = FROM_RE.match(line)
        if not m:
            continue
        ref = expand_vars(strip_platform(m.group(1)), args)
        if "@sha256:" not in ref:
            continue
        parsed = parse_ref(ref)
        if parsed:
            inv.add(*parsed, source=str(path))
            claimed.add(parsed[2])

    claim_digests(text, claimed, inv, str(path))


def parse_hcl(path: Path, inv: Inventory) -> None:
    text = path.read_text()
    claimed: set[str] = set()

    for ref in REF_WITH_DIGEST_RE.findall(text):
        full = f"{ref[0]}@{ref[1]}"
        parsed = parse_ref(full)
        if parsed:
            inv.add(*parsed, source=str(path))
            claimed.add(parsed[2])

    for varname, block in HCL_LOOKUP_RE.findall(text):
        image = (
            varname[: -len("_DIGEST")].lower()
            if varname.upper().endswith("_DIGEST")
            else varname.lower()
        )
        for tag, digest in HCL_PAIR_RE.findall(block):
            inv.add(image, tag, digest, source=str(path))
            claimed.add(digest)

    claim_digests(text, claimed, inv, str(path))


def build_inventory(root: Path) -> Inventory:
    inv = Inventory()
    docker_dir = root / "utils" / "docker"
    for path in sorted(docker_dir.glob("Dockerfile.*")):
        parse_dockerfile(path, inv)
    for path in sorted(docker_dir.glob("*.hcl")):
        parse_hcl(path, inv)
    devcontainer = root / ".devcontainer" / "Dockerfile"
    if devcontainer.exists():
        parse_dockerfile(devcontainer, inv)
    return inv


NOT_FOUND_MARKERS = (
    "not found",
    "manifest unknown",
    "manifest_unknown",
    "no such manifest",
    "does not exist",
)

REGISTRY_ERROR_DETAIL = "registry error; pin could not be verified"


def imagetools_resolve(ref: str, attempts: int = 3) -> tuple[str, str | None]:
    """Resolve ``ref`` via buildx, distinguishing absence from transient failure.

    Returns one of:

    * ``("ok", digest)`` - resolved to a manifest digest;
    * ``("notfound", None)`` - the registry definitively reports the ref absent;
    * ``("error", None)`` - a transient failure (rate limit, network, 5xx) that
      persisted across retries. Callers must never treat this as a dead pin.
    """
    for attempt in range(attempts):
        proc = subprocess.run(
            [
                "docker",
                "buildx",
                "imagetools",
                "inspect",
                ref,
                "--format",
                "{{.Manifest.Digest}}",
            ],
            stdin=subprocess.DEVNULL,
            capture_output=True,
            text=True,
        )
        if proc.returncode == 0:
            out = proc.stdout.strip()
            return ("ok", out) if DIGEST_RE.fullmatch(out) else ("error", None)
        err = (proc.stderr or "").lower()
        if any(marker in err for marker in NOT_FOUND_MARKERS):
            return ("notfound", None)
        if attempt < attempts - 1:
            time.sleep(2 * (attempt + 1))
    return ("error", None)


def quay_latest_dated_tag(image: str) -> tuple[str, date] | None:
    """Newest dated tag for a quay.io/<ns>/<repo> image, via the quay REST API."""
    prefix = "quay.io/"
    if not image.startswith(prefix):
        return None
    repo = image[len(prefix) :]
    best: tuple[str, date, int] | None = None
    for page in range(1, 11):  # cap at 10 pages (1000 tags) to bound API calls
        url = (
            f"https://quay.io/api/v1/repository/{repo}/tag/"
            f"?onlyActiveTags=true&limit=100&page={page}"
        )
        try:
            with urllib.request.urlopen(url, timeout=30) as resp:
                data = json.load(resp)
        except Exception:
            return None
        for entry in data.get("tags", []):
            name = entry.get("name", "")
            parsed = parse_tag_date(name)
            if not parsed:
                continue
            counter_match = re.search(r"-(\d+)$", name)
            counter = int(counter_match.group(1)) if counter_match else -1
            if best is None or (parsed, counter) > (best[1], best[2]):
                best = (name, parsed, counter)
        if not data.get("has_additional"):
            break
    return (best[0], best[1]) if best else None


def parse_tag_date(tag: str) -> date | None:
    m = re.match(r"(\d{4})\.(\d{2})\.(\d{2})", tag)
    if not m:
        return None
    try:
        return date(int(m.group(1)), int(m.group(2)), int(m.group(3)))
    except ValueError:
        return None


def _pinned_digest_finding(pin: Pin) -> Finding | None:
    """Re-resolve a pin's full ``image:tag@digest`` ref to tell DEAD from a transient error.

    Returns a ``DEAD`` finding when the registry reports the digest absent, a
    ``WARN`` finding on a transient failure, or ``None`` when the digest still
    resolves (the caller then decides MOVED vs. healthy).
    """
    pinned_status, _ = imagetools_resolve(pin.pinned_ref())
    if pinned_status == "notfound":
        return Finding("DEAD", pin, "pinned digest no longer resolves in the registry")
    if pinned_status == "error":
        return Finding("WARN", pin, REGISTRY_ERROR_DETAIL)
    return None


def evaluate(inv: Inventory, threshold_days: int) -> list[Finding]:
    findings: list[Finding] = []

    for path, digest in inv.unmapped:
        findings.append(
            Finding(
                "UNMAPPED",
                Pin("?", None, digest, (path,)),
                "digest could not be mapped to an image:tag",
            )
        )

    for pin in sorted(inv.pins.values(), key=lambda p: (p.image, p.tag or "")):
        if pin.tag is None:
            # A bare image@digest pin has no mutable tag to drift from; resolving
            # the tagless image would query :latest and flag a spurious MOVED, so
            # only check whether the pinned digest still resolves.
            finding = _pinned_digest_finding(pin)
            if finding is not None:
                findings.append(finding)
            continue

        pinned_date = parse_tag_date(pin.tag or "")
        status, current = imagetools_resolve(pin.tag_ref())

        if status == "error":
            findings.append(Finding("WARN", pin, REGISTRY_ERROR_DETAIL))
            continue

        if status == "notfound" or (current is not None and current != pin.digest):
            # The tag no longer points at the pinned digest. Confirm whether the
            # pinned digest itself is gone (DEAD) or merely superseded (MOVED).
            finding = _pinned_digest_finding(pin)
            if finding is not None:
                findings.append(finding)
                continue
            detail = (
                "tag no longer exists in the registry (pinned digest still resolves)"
                if current is None
                else f"tag now resolves to {current}"
            )
            findings.append(Finding("MOVED", pin, detail))
            continue

        # status == "ok" and current == pinned digest: the pin is alive and current.
        if pinned_date is not None:
            latest = quay_latest_dated_tag(pin.image)
            if latest is None:
                # Only quay images can have their dated tags enumerated here. A dated
                # tag on any other registry is unsupported for staleness, so skip it
                # rather than emitting a WARN that can never clear on every run.
                if pin.image.startswith("quay.io/"):
                    findings.append(
                        Finding("WARN", pin, "could not determine the newest dated tag")
                    )
                continue
            behind = (latest[1] - pinned_date).days
            if behind > threshold_days:
                findings.append(
                    Finding(
                        "STALE",
                        pin,
                        f"{behind} days behind newest dated tag {latest[0]} (>{threshold_days})",
                    )
                )

    return findings


def severity_counts(findings: list[Finding]) -> dict[str, int]:
    """Count findings per severity, keyed in ``SEVERITY_ORDER``."""
    return {sev: sum(f.severity == sev for f in findings) for sev in SEVERITY_ORDER}


def render_report(findings: list[Finding], inv: Inventory, threshold_days: int) -> str:
    """Render the report body to match the repo's General Issue form.

    The body mirrors a submitted ``general_issue.yml`` form: a ``### Summary``
    section followed, when there are findings, by a ``### Appendix`` section.
    """
    order = SEVERITY_ORDER
    counts = severity_counts(findings)
    total = len(inv.pins)

    lines = ["### Summary", ""]
    if not findings:
        lines.append(
            f"Checked **{total}** pinned base images across the Dockerfiles and bake "
            "files. All pins resolve, no mutable tag has moved, and no dated tag is "
            "stale. :white_check_mark:"
        )
        return "\n".join(lines) + "\n"

    breakdown = ", ".join(
        f"{counts[sev]} {sev.lower()}" for sev in order if counts[sev]
    )
    lines.append(
        f"Checked **{total}** pinned base images across the Dockerfiles and bake "
        f"files. Found {breakdown}. Dead and unmapped pins fail the build; moved and "
        "stale pins are advisory."
    )

    titles = {
        "DEAD": ":x: Dead pins (build-critical: digest no longer resolves)",
        "UNMAPPED": ":x: Unmapped digests (coverage gap: teach the checker this pattern)",
        "MOVED": ":warning: Moved tags (mutable tag advanced; consider re-pinning)",
        "STALE": f":hourglass: Stale dated tags (>{threshold_days} days behind newest)",
        "WARN": ":grey_question: Could not evaluate",
    }
    lines += ["", "### Appendix", ""]
    for sev in order:
        group = [f for f in findings if f.severity == sev]
        if not group:
            continue
        lines.append(f"#### {titles[sev]}")
        lines.append("")
        for f in group:
            where = ", ".join(f.pin.sources) if f.pin.sources else "?"
            lines.append(f"- `{f.pin.pinned_ref()}` — {f.detail} ({where})")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", type=Path)
    parser.add_argument(
        "--threshold-days",
        type=int,
        default=int(os.environ.get("PIN_STALE_THRESHOLD_DAYS", "30")),
    )
    parser.add_argument("--report", default="report.md", type=Path)
    parser.add_argument(
        "--list-only",
        action="store_true",
        help="print the discovered pin inventory and exit, without contacting any registry",
    )
    args = parser.parse_args()

    inv = build_inventory(args.root)

    if args.list_only:
        pins = inv.pins
        for key in sorted(pins):
            pin = pins[key]
            kind = "dated" if parse_tag_date(pin.tag or "") is not None else "mutable"
            print(f"{kind:8} {pin.tag_ref()} {pin.digest} <- {', '.join(pin.sources)}")
        print(f"\n{len(pins)} pins, {len(inv.unmapped)} unmapped")
        for path, digest in inv.unmapped:
            print(f"UNMAPPED {path} {digest}")
        return 0
    findings = evaluate(inv, args.threshold_days)
    if not inv.pins and not inv.unmapped:
        # Discovering zero pins almost always means the scan roots are wrong, not
        # that the repo is unpinned. Surface it as inconclusive so a broken scan
        # never reports "all healthy" and closes the tracking issue.
        findings.append(
            Finding(
                "WARN",
                Pin("base-image inventory", None, "empty", ()),
                "no pinned base images were discovered; the scan roots may be wrong",
            )
        )
    report = render_report(findings, inv, args.threshold_days)

    args.report.write_text(report)
    sys.stdout.write(report)

    counts = severity_counts(findings)
    critical = counts["DEAD"] + counts["UNMAPPED"]
    actionable = critical + counts["MOVED"] + counts["STALE"]
    total = len(inv.pins)

    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as fh:
            fh.write(f"total={total}\n")
            fh.write(f"actionable={actionable}\n")
            fh.write(f"critical={critical}\n")
            for sev, n in counts.items():
                fh.write(f"{sev.lower()}={n}\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
