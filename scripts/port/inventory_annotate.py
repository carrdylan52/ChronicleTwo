#!/usr/bin/env python3
"""Fill the hand-written columns of the hardware inventory and add its review rows.

    ledger_reach.py
    tools/kazarov/scripts/hw_inventory.py scan --map build/pal/SCES_511.90.xMAP \
        --objects build/pal/main_o_files --ledger docs/port/ASM_LEDGER.tsv
    inventory_annotate.py [--inventory docs/port/HARDWARE_INVENTORY.md] [--trace scripts/port/boot_trace.md] [--force]

trace_position is boot:<step> of the first step of the boot trace that names the row, else the part of
the game that reaches it (menu, debug, mode). replacement_wp is the plan's work package that owns the
row's unit. A cell that already holds a value is kept unless --force is given; a row whose state starts
with dead is left alone.
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "kazarov", "scripts"))
import hw_inventory as hw  # noqa: E402

# Functions that link and run but keep PS2 behaviour; the scan cannot see them.
REVIEW = [
    ("ps2/src/mglib.cpp", "mgGetTopVRAMAddress()", "register",
     "review: mglib.cpp:620; VRAM page arithmetic (GS block and page sizes)", "T8"),
    ("ps2/src/mg_texture.cpp", "mgCTextureManager::Initialize(int,int)", "register",
     "review: mg_texture.cpp:511; texture blocks laid out over a GS VRAM address range", "T8"),
    ("ps2/src/mg_texture.cpp", "mgCTextureManager::GetRemainVRAM(int)", "register",
     "review: mg_texture.cpp:702; VRAM accounting", "T8"),
    ("ps2/src/mglib.cpp", "mgTransWorldPrim(int*,float*)", "register",
     "review: mglib.cpp:1740; screen coordinates in GS 12.4 fixed point", "T7"),
    ("ps2/src/mglib.cpp", "mgTransViewPrim(int*,float*)", "register",
     "review: mglib.cpp:1761; screen coordinates in GS 12.4 fixed point", "T7"),
    ("ps2/src/mglib.cpp", "mgTransWorldPrim3DSprite(int*,int*,float*,float,float,int)", "register",
     "review: mglib.cpp:1806; screen coordinates in GS 12.4 fixed point", "T7"),
    ("ps2/src/mglib.cpp", "mgTransWorldScreen(int*,float*)", "register",
     "review: mglib.cpp:1754; subtracts the GS screen offset in 12.4", "T7"),
    ("ps2/src/dataread.cpp", "size_to_sector(int)", "iop-rpc",
     "review: dataread.cpp:130; CD sector arithmetic (2048-byte sectors)", "T5"),
]

# Rows the scan flags whose evidence is not hardware: they compile and run unchanged.
NOT_HARDWARE = {
    "ITEM_USE_FLAG_CURE_DRY": "not hardware: ITEM_USE_FLAG_CURE_DRY is the item flag 0x10000000",
    "SCE_GS_ZGEQUAL": "not hardware: sets the z test of an mgCDrawEnv copy, which T7's RenderState reads",
}

DEBUG_UNITS = re.compile(r"^(edit\w*|eventedit|\w+viewlp|dbg_font|dng_debug)$")
MENU_UNITS = re.compile(r"^(menu\w*|inventmn|dngmenu)$")

UNIT_WP = [
    (r"^(dataread|filesocket)$", "T5"),
    (r"^(main|mainloop|gamepad|nowload|sceneload|fishing)$", "T6"),
    (r"^(mglib|mg_drawenv|mg_memory)$", "T7"),
    (r"^(mg_texture|mg_tanime)$", "T8"),
    (r"^(mg_drawprim|menudraw|prespr|nd_meswin|effectlist|screeneffect|photo|outline|gyorace)$", "T9"),
    (r"^(title|font|gaiji|menuop)$", "T9b"),
    (r"^(memcard|savedata|savedatadungeon|convviewlp)$", "T10"),
    (r"^(sound|ezmidi|ezbgm|snd_mngr)$", "T11"),
    (r"^(padcontrol)$", "T12"),
    (r"^movie$", "T13e"),
    (r"^(mg_frame|visualmotion|mg_math|gameutil|collision|editcoll|editmap2)$", "V3"),
    (r"^(mg_visual|mg_dataset)$", "V2"),
    (r"^(mg_shadow|mg_sprite|water|dng_effect)$", "V4"),
]

FUNCTION_WP = {
    "mgGetVuProgPacket": "V4",
    "mgEndDrawShadow": "V4",
    "mgDrawDirect": "V2",
    "mgDrawDirectStart": "V2",
    "mgDrawDirectEnd": "V2",
}


def trace_steps(path):
    first = {}
    with open(path) as f:
        for line in f:
            step = re.match(r"\|\s*(\d+)\s*\|", line)
            if not step:
                continue
            for token in re.findall(r"\[(\w+):([\w:~]+)\]", line):
                first.setdefault(token, int(step[1]))
    return first


def short_unit(unit):
    return os.path.splitext(os.path.basename(unit))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--inventory", default="docs/port/HARDWARE_INVENTORY.md")
    parser.add_argument("--trace", default="scripts/port/boot_trace.md")
    parser.add_argument("--force", action="store_true")
    options = parser.parse_args()
    inventory = os.path.join(ROOT, options.inventory)
    rows = hw.read_inventory(inventory)
    if not rows:
        sys.exit(f"{inventory}: no rows")
    present = {(r["unit"], r["function"]) for r in rows}
    for unit, function, kind, evidence, _ in REVIEW:
        if (unit, function) not in present:
            rows.append({"unit": unit, "function": function, "kind": kind, "evidence": evidence,
                         "trace_position": "", "replacement_wp": "", "state": ""})
    review_wp = {(u, f): wp for u, f, _, _, wp in REVIEW}
    first = trace_steps(os.path.join(ROOT, options.trace))
    for row in rows:
        if row["state"].startswith("dead"):
            row["replacement_wp"] = row["replacement_wp"] or "-"
            continue
        unit = short_unit(row["unit"])
        spelled = hw.spelled(row["function"])
        step = first.get((unit, spelled))
        if step is not None:
            position = "boot:%d" % step
        elif DEBUG_UNITS.match(unit):
            position = "debug"
        elif MENU_UNITS.match(unit):
            position = "menu"
        else:
            position = "mode"
        wp = review_wp.get((row["unit"], row["function"])) or FUNCTION_WP.get(spelled.split("::")[-1]) or next(
            (w for pattern, w in UNIT_WP if re.match(pattern, unit)), "T16b")
        state = "stub"
        evidence_names = set(re.findall(r"\b[A-Z_][A-Z0-9_]+\b", row["evidence"].split(";", 1)[-1]))
        kinds = set(row["kind"].split(", "))
        reason = None
        for name, why in NOT_HARDWARE.items():
            other = re.search(r"\b(sce|0x|via)\w*| \+\d", row["evidence"])
            if name in evidence_names and len(kinds) == 1 and not other:
                reason = why
        if reason:
            wp, state = "- (" + reason + ")", "native"
        for column, value in (("trace_position", position), ("replacement_wp", wp), ("state", state)):
            if options.force or not row[column]:
                row[column] = value
    hw.write_inventory(sorted(rows, key=lambda r: (r["unit"], r["function"])), inventory)
    print("%d rows in %s" % (len(rows), os.path.relpath(inventory, ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
