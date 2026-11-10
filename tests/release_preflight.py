"""Check release metadata against the real C++ hooks and pinned SDK."""

import argparse
import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--sdk-header", required=True, type=Path)
args = parser.parse_args()


class ReleasePreflight(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((ROOT / "openrac-mod.json").read_text(encoding="utf-8"))
        cls.source = (ROOT / "src/mod.cpp").read_text(encoding="utf-8")
        cls.sdk = args.sdk_header.read_text(encoding="utf-8")

    def test_release_identity_and_version(self):
        self.assertEqual(self.manifest["id"], "pause-weapon-wheel")
        self.assertEqual(self.manifest["version"], "1.0.2")
        match = re.search(r'OPENRAC_MOD_CPP\("([^"]+)",\s*"([^"]+)"', self.source)
        self.assertIsNotNone(match, "Native mod ID/version must be declared")
        self.assertEqual(match.group(1), self.manifest["id"])
        self.assertEqual(match.group(2), self.manifest["version"])

    def test_all_native_hooks_are_declared_in_manifest(self):
        sdk_ids = dict(re.findall(
            r'struct\s+(\w+)\s*\{\s*static constexpr const char\* id = "([^"]+)"',
            self.sdk,
        ))
        installed_hook_types = set(re.findall(
            r'mod\.hook<openrac::rac1::(\w+)>', self.source,
        ))
        self.assertTrue(installed_hook_types, "Must discover the actual mod hooks")
        unknown = installed_hook_types - set(sdk_ids)
        self.assertFalse(unknown, f"Hooks missing from pinned SDK: {sorted(unknown)}")
        expected_ids = {sdk_ids[hook] for hook in installed_hook_types}
        declared_ids = set(self.manifest["hooks"])
        self.assertEqual(
            declared_ids, expected_ids,
            f"Manifest hook IDs must match code: missing {sorted(expected_ids - declared_ids)}, "
            f"unused {sorted(declared_ids - expected_ids)}",
        )


if __name__ == "__main__":
    unittest.main(argv=["release_preflight.py"], verbosity=2)
