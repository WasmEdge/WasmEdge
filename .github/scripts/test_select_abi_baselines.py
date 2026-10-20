#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import unittest

from select_abi_baselines import compare, normalize, select


def release(tag, **flags):
    return {"tag_name": tag, "draft": False, "prerelease": False, **flags}


class BaselineTests(unittest.TestCase):
    def test_common_names(self):
        for tag, expected in (
            ("v1.2", "1.2"), ("release-1.2.3.4", "1.2.3.4"),
            ("v1.2-RC1", "1.2~rc1"),
            ("release/2026.04", "2026.04"), ("1.2rc1", "1.2~rc1"),
            ("1.2-beta.2", "1.2~beta.2"), ("1.2~rc1", "1.2~rc1"),
            ("2:1.2+dfsg-3", "2:1.2+dfsg-3"), ("1.2p1", "1.2p1"),
        ):
            with self.subTest(tag=tag):
                self.assertEqual(normalize(tag), expected)

    def test_debian_ordering(self):
        for old, new in (("1.9", "1.10"), ("1.2~rc1", "1.2"),
                         ("1.2", "1.2p1"), ("1:9.0", "2:1.0")):
            with self.subTest(old=old, new=new):
                self.assertEqual(compare(old, new), -1)

    def test_unknown_stable_tag_fails_or_is_mapped(self):
        releases = [release("spring")]
        with self.assertRaises(ValueError):
            select(releases, "2026.10")
        self.assertEqual(len(select(releases, "2026.10", {"spring": "2026.04"})), 1)

    def test_maintenance_and_historical_releases(self):
        releases = [release(tag) for tag in ("1.0.0", "1.1.0", "1.0.2", "2.0.0")]
        self.assertEqual(
            [entry["tag"] for entry in select(releases, "1.1.0")],
            ["1.0.0", "1.0.2"],
        )

    def test_new_releases_automatically_join_baseline(self):
        releases = [release("1.0.0"), release("1.1.0")]
        self.assertEqual(len(select(releases, "1.2.0")), 2)

    def test_development_includes_last_tag(self):
        self.assertEqual(len(select([release("1.1.0")], "1.1.0-7-gabcdef")), 1)

    def test_exclude_unpublished_and_prereleases(self):
        releases = [release("1.0.0"), release("1.1.0", draft=True),
                    release("1.2.0", prerelease=True), release("2.0.0-rc.1", prerelease=True)]
        self.assertEqual(len(select(releases, "2.0.0-rc.2")), 1)

    def test_release_candidate_description_does_not_include_final(self):
        releases = [release("1.0.0"), release("1.1.0")]
        self.assertEqual(len(select(releases, "1.1.0-rc.1-2-gabcdef")), 1)

    def test_invalid_metadata(self):
        with self.assertRaises(ValueError):
            select([release("1.0.0", draft="false")], "2.0.0")
        with self.assertRaises(ValueError):
            select([release("1.0.0")], "master")

    def test_missing_baseline_fails(self):
        with self.assertRaises(ValueError):
            select([], "1.0.0")
        with self.assertRaises(ValueError):
            select([release("1.0.0")], "1.0.0")


if __name__ == "__main__":
    unittest.main()
