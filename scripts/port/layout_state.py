#!/usr/bin/env python3
"""Check docs/port/LAYOUTS.md against a milestone.

    python3 scripts/port/layout_state.py --milestone M1
    python3 scripts/port/layout_state.py --position boot:30 --trace port/build/pc/trace.log \
        --truncations port/build/pc/truncations.json

Fails when:
- a STATIC_ASSERT(sizeof) record, or a truncations.json site in ps2/src, has no row;
- a row whose first_use is boot:<n> at or before the position is not repaired (or n/a);
- a row is open whatever its first_use, since every row needs a decision;
- a guarded-unreached row's guard is gone: the inventory row of an override or callee
  [unit:function] it names is no longer a stub, or the guard line no longer calls PortPtr32 or
  PortLayoutGuard;
- the trace names the function of a guarded-unreached row, which must then be repaired.
"""

import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# The last boot-trace step each milestone's trace may reach (scripts/port/boot_trace.md): M1 stops at
# the first stub in init, M3 at mgInit, M4 at sndInitMngr at the latest, M6 at the title screen.
MILESTONES = {"M1": 2, "M3": 11, "M4": 30, "M6": 66}

ASSERT = re.compile(r"^\s*STATIC_ASSERT\(sizeof\((.+)\)\s*==\s*[^)]+\);")
TOKEN = re.compile(r"\[(\w+):([^\]]+)\]")
GUARD_LINE = re.compile(r"\[(\w+):(\d+)\]")
DONE = {"repaired", "n/a"}
STATES = {"repaired", "guarded-unreached", "open", "n/a"}


def read_tables(path):
    tables = {}
    title = None
    columns = None
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("## "):
                title, columns = line[3:].strip(), None
                continue
            if not line.startswith("|") or title is None:
                columns = None
                continue
            cells = [c.strip().replace("\\|", "|") for c in re.split(r"(?<!\\)\|", line)[1:-1]]
            if columns is None:
                columns = cells
                tables.setdefault(title, [])
            elif not all(re.fullmatch(r"-+", c) for c in cells):
                tables[title].append(dict(zip(columns, cells)))
    return tables


def read_inventory(path):
    states = {}
    with open(path) as f:
        for line in f:
            cells = [c.strip() for c in line.split("|")[1:-1]]
            if len(cells) == 7 and cells[0].startswith("ps2/"):
                unit = os.path.splitext(os.path.basename(cells[0]))[0]
                states[(unit, cells[1].split("(")[0])] = cells[6]
    return states


def asserted_records():
    found = set()
    for folder in ("ps2/include", "ps2/include/sce", "ps2/include/std", "ps2/src"):
        for name in sorted(os.listdir(os.path.join(ROOT, folder))):
            path = os.path.join(folder, name)
            if not os.path.isfile(os.path.join(ROOT, path)):
                continue
            with open(os.path.join(ROOT, path), encoding="latin-1") as f:
                for line in f:
                    match = ASSERT.match(line)
                    if match:
                        found.add("%s %s" % (path, match.group(1).strip()))
    return found


def boot_step(first_use):
    match = re.fullmatch(r"boot:(\d+)", first_use or "")
    return int(match.group(1)) if match else None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    where = parser.add_mutually_exclusive_group()
    where.add_argument("--milestone", choices=sorted(MILESTONES))
    where.add_argument("--position", help="boot:<n>, the last boot-trace step the trace reaches")
    parser.add_argument("--trace", help="a trace log; any guarded row whose function it names fails")
    parser.add_argument("--truncations", help="truncations JSON whose ps2/src sites must all have rows")
    parser.add_argument("--layouts", default="docs/port/LAYOUTS.md")
    parser.add_argument("--inventory", default="docs/port/HARDWARE_INVENTORY.md")
    options = parser.parse_args()

    if options.milestone:
        limit = MILESTONES[options.milestone]
    elif options.position:
        limit = boot_step(options.position)
        if limit is None:
            parser.error("--position takes boot:<n>")
    else:
        limit = 0

    tables = read_tables(os.path.join(ROOT, options.layouts))
    inventory = read_inventory(os.path.join(ROOT, options.inventory))
    rows = [(title, row) for title, table in tables.items() for row in table if "state" in row]
    errors = []

    records = {row["key"] for row in tables.get("Records", [])}
    for missing in sorted(asserted_records() - records):
        errors.append("no Records row for STATIC_ASSERT of %s" % missing)
    if options.truncations:
        with open(os.path.join(ROOT, options.truncations)) as f:
            sites = [s for s in json.load(f) if s["file"].startswith("ps2/")]
        casts = {(r["site"], r["expression"].strip("`")) for r in tables.get("Casts", [])}
        for site in sites:
            where = "%s:%d" % (site["file"], site["line"])
            if (where, re.sub(r"\s+", " ", site["expression"])) not in casts:
                errors.append("no Casts row for %s %s" % (where, site["expression"]))

    traced = ""
    if options.trace:
        with open(os.path.join(ROOT, options.trace), errors="replace") as f:
            traced = f.read()

    counts = {}
    for title, row in rows:
        state = row["state"]
        name = "%s %s" % (title, row.get("site", ""))
        counts[state] = counts.get(state, 0) + 1
        if state not in STATES:
            errors.append("%s: unknown state %r" % (name, state))
            continue
        if state == "open":
            errors.append("%s: open" % name)
        step = boot_step(row.get("first_use"))
        if step is not None and step <= limit and state not in DONE:
            errors.append("%s: first_use %s is at or before boot:%d but the row is %s" % (
                name, row["first_use"], limit, state))
        if state != "guarded-unreached":
            continue
        fix = row.get("fix", "")
        tokens = TOKEN.findall(fix)
        lines = GUARD_LINE.findall(fix)
        if fix.startswith(("override", "callee")):
            for unit, function in tokens:
                if inventory.get((unit, function)) != "stub":
                    errors.append("%s: [%s:%s] is no longer a stub, so the row must be repaired" % (
                        name, unit, function))
        elif fix.startswith(("guard", "PortLayoutGuard")):
            places = [(row["site"].rsplit(":", 1)[0], int(row["site"].rsplit(":", 1)[1]))]
            if lines:
                places = [("ps2/src/%s.cpp" % unit, int(line)) for unit, line in lines]
            for path, number in places:
                with open(os.path.join(ROOT, path), encoding="latin-1") as f:
                    text = f.read().split("\n")
                window = "\n".join(text[max(number - 2, 0):number])
                if "PortPtr32" not in window and "PortLayoutGuard" not in window:
                    errors.append("%s: no guard at %s:%d" % (name, path, number))
        else:
            errors.append("%s: guarded-unreached without an override, callee or guard fix" % name)
        function = row.get("function", "")
        if traced and function and re.search(r"(?<![\w:])%s(?![\w])" % re.escape(function), traced):
            errors.append("%s: the trace reaches %s, which is guarded-unreached" % (name, function))

    for error in errors:
        print("layout_state.py: %s" % error, file=sys.stderr)
    print("%s: %d rows (%s) checked through boot:%d: %s" % (
        options.layouts, len(rows), ", ".join("%s %d" % kv for kv in sorted(counts.items())), limit,
        "%d problems" % len(errors) if errors else "ok"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
