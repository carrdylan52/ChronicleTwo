#!/usr/bin/env python3
"""Generate the objdiff report, as decomp.dev reads it, and summarise it."""

import json
from pathlib import Path
import subprocess

import layout
from verify import Image


ROOT = Path(__file__).resolve().parents[2]
IMAGE = "SCES_511.90"


def image_matches():
    built = ROOT / layout.BUILD / IMAGE
    if not built.exists():
        return False
    retail = Image(ROOT / layout.ELF_PATH)
    linked = Image(built)
    return (retail.memory_end == linked.memory_end and
            len(retail.bytes) == len(linked.bytes) and
            all(retail.span(lo, hi) == linked.span(lo, hi)
                for section, lo, hi in layout.SECTIONS if section not in layout.NOBITS))


def print_summary(report):
    red, green, yellow, gray, reset = "\033[91m", "\033[92m", "\033[93m", "\033[90m", "\033[0m"
    measures = report["measures"]
    units = report["units"]
    fuzzy = sum(1 for unit in units for function in unit.get("functions", ())
                if "fuzzy_match_percent" in function and
                function["fuzzy_match_percent"] < 100.0)
    # Objdiff has no source-side score for functions still supplied by assembly.
    asm = sum(1 for unit in units for function in unit.get("functions", ())
              if "fuzzy_match_percent" not in function)
    perfect = measures["matched_functions"]
    unmatched = measures["total_functions"] - perfect - fuzzy - asm
    perfect_share = float(measures["matched_code_percent"])
    fuzzy_share = max(0.0, float(measures["fuzzy_match_percent"]) - perfect_share)
    asm_share = max(0.0, 100.0 - float(measures["fuzzy_match_percent"]))
    unmatched_share = max(0.0, 100.0 - perfect_share - fuzzy_share - asm_share)
    status = f"{green}OK{reset}" if image_matches() else f"{red}FAILED{reset}"
    print(f"{IMAGE}: {status} {gray}({perfect} perfect, {fuzzy} fuzzy, "
          f"{asm} asm, {unmatched} unmatched){reset}")
    print(f"Outstanding functions: {measures['total_functions'] - perfect} "
          f"({fuzzy} fuzzy + {asm} assembly + {unmatched} other)")
    print("\nCode, by byte")
    for label, color, share, count in (
        ("Perfect", green, perfect_share, perfect),
        ("Fuzzy", yellow, fuzzy_share, fuzzy),
        ("Asm", gray, asm_share, asm),
        ("Unmatched", red, unmatched_share, unmatched),
    ):
        print(f"  {color}{label:<11}{reset} {share:6.2f}%    {count:4d} functions")
    total_data = int(measures.get("total_data", 0))
    if total_data:
        unmatched_data = total_data - int(measures.get("matched_data", 0))
        print(f"  Data       {yellow}{unmatched_data} bytes not matched by objdiff{reset}")
    differences = [(unit["name"], function["name"])
                   for unit in units for function in unit.get("functions", ())
                   if "fuzzy_match_percent" in function and
                   function["fuzzy_match_percent"] < 100.0]
    for unit, function in differences[:10]:
        print(f"    {yellow}fuzzy{reset} {unit}/{function}")
    if len(differences) > 10:
        print(f"    ... and {len(differences) - 10} more")


def main():
    output = ROOT / "progress" / "report.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["objdiff-cli", "report", "generate", "--project", str(ROOT),
                    "--output", str(output)], cwd=ROOT, check=True)
    print_summary(json.loads(output.read_text(encoding="utf-8")))


if __name__ == "__main__":
    main()
