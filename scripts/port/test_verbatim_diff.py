"""Regression checks for the audit's evidence citation and selection contract."""
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/verbatim_diff.py"
spec = importlib.util.spec_from_file_location("verbatim_diff", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class AuditContract(unittest.TestCase):
    def test_reviewed_body_requires_exact_override_and_retail_tokens(self):
        row = next(r for r in audit.REVIEWED_BODIES if r["name"] == "GetRoboPartsInfo")
        pair = row["override_sha256"], row["retail_sha256"]
        self.assertEqual(audit.accepted_reason(row["name"], row["path"], 1, "", pair), row["reason"])
        for changed in (("0" * 64, pair[1]), (pair[0], "0" * 64)):
            self.assertIsNone(audit.accepted_reason(row["name"], row["path"], 1, "", changed))

    def test_body_digest_ignores_comments_but_detects_behavior_changes(self):
        original = audit.tokenize("{ return 0; }")
        self.assertEqual(audit.body_digest(original), audit.body_digest(audit.tokenize("{ /* why */ return 0; }")))
        self.assertNotEqual(audit.body_digest(original), audit.body_digest(audit.tokenize("{ return 1; }")))

    def test_all_help_discloses_date_filter_and_full_history_invocation(self):
        result = subprocess.run([sys.executable, "-I", "-B", str(SCRIPT), "--help"],
                                capture_output=True, text=True, check=True)
        help_text = " ".join(result.stdout.split())
        self.assertIn("still filtered by --since declaration-line blame date", help_text)
        self.assertIn("--since 0000-01-01 for all history", help_text)

    def test_set_damage_reason_cites_existing_test_and_its_actual_assertions(self):
        reason = next(item[4] for item in audit.ACCEPTED if item[0] == "CColPrim::SetDamage")
        source = (ROOT / "port/src/tests/ixion_room_test.cpp").read_text()
        citations = re.findall(r"(Ixion\w+)\.(\w+)", reason)
        self.assertTrue(citations)
        for suite, name in citations:
            match = re.search(r"TEST\(\s*" + suite + r"\s*,\s*" + name + r"\s*\)\s*\{", source)
            self.assertIsNotNone(match, f"missing cited test {suite}.{name}")
            body = source[match.end():].split("\nTEST(", 1)[0]
            self.assertIn('projectile.SetDamage(ranged->name, 0)', body)
            self.assertIn('projectile.SetDamage(ranged->name, 1)', body)
            self.assertIn('projectile.SetDamage(const_cast<char *>("not-a-damage-row"), 0)', body)
            for age in (0, 1, 3):
                self.assertIn(f"EXPECT_EQ(out[0].val.i, {age})", body)
        self.assertIn("does not establish retirement coverage", reason)


if __name__ == "__main__":
    unittest.main()
