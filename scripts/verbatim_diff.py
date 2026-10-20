#!/usr/bin/env python3
"""w5 verbatim-diff: token-level comparison of PORT_OVERRIDE bodies against retail ps2/src bodies.

Read-only: uses `git ls-tree` and `git cat-file --batch` on one branch. Writes nothing.

Usage (run with -I -B):
  python -I -B verbatim_diff.py [--repo DIR] [--branch REF] [--all] [--name QUALIFIED] [--max-hunks N]

Selection (default): every PORT_OVERRIDE in a file named ixion_*, every override whose comment
block says verbatim/unchanged/retail/copied, and RunEvent/SetEventScript/IsEventRun/EventScriptSetup.
--all considers every PORT_OVERRIDE definition in port/src, still filtered by --since
on its declaration-line blame date. Use --all --since 0000-01-01 for all history.

Method: comments and whitespace are dropped by the tokenizer; each override body is diffed token by
token (difflib) against the retail body with the same qualified name and parameter skeleton in
ps2/src. Each non-equal hunk is classified:
  MARKED     an identifier containing "ixion" sits inside the hunk on the override side
  NEAR       an "ixion" identifier sits within 200 tokens before (or 20 after) the hunk
  UNMARKED   no Ixion identifier nearby; treat as a native-behaviour change until read by hand
Classification is a heuristic. Gating must still be confirmed by reading the hunk's enclosing if.

Duplicate checks: two overrides with the same qualified name and parameter skeleton, or two
overrides whose exact-signature match resolves to the same retail definition.

Exit status: 0 = no UNMARKED hunks in the selected overrides and no duplicates;
             1 = UNMARKED hunks or duplicates found; 2 = lookup/usage error.
"""
import argparse
import difflib
import hashlib
import json
import os
import re
import subprocess
import sys
import time

DEFAULT_REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_BRANCH = "HEAD"
EVENT_NAMES = {"RunEvent", "SetEventScript", "IsEventRun", "EventScriptSetup"}
CLAIM_RE = re.compile(r"verbatim|unchanged|retail|copied", re.I)
IXION_RE = re.compile(r"ixion", re.I)
IDENT_RE = re.compile(r"[A-Za-z_]\w*$")
CONTROL = {"if", "while", "for", "switch", "catch", "return", "sizeof", "else", "do"}
SRC_EXT = (".cpp", ".c", ".cc", ".h", ".hpp")
TOK_RE = re.compile(
    r"(?P<ws>[ \t\r\f\v\n]+)"
    r"|(?P<lc>//[^\n]*)"
    r"|(?P<bc>/\*.*?\*/)"
    r'|(?P<str>"(?:\\.|[^"\\\n])*")'
    r"|(?P<chr>'(?:\\.|[^'\\\n])*')"
    r"|(?P<id>[A-Za-z_]\w*)"
    r"|(?P<num>\d\w*(?:\.\w*)?)"
    r"|(?P<op>::|->|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||\+\+|--|[-+*/%&|^]=|[{}()\[\];,.<>=!~?:*&|^%+\-/#])"
    r"|(?P<other>.)",
    re.S,
)
GIT_ENV = dict(os.environ, GIT_OPTIONAL_LOCKS="0", GIT_PAGER="cat")

# Hunks reviewed by hand (2026-10-14) that are not Ixion-named but are gated or equivalent.
# Each entry: (qualified name, override file suffix, override line range or None, required substring or None, reason).
# A hunk is ACCEPTED only if it is UNMARKED, matches name+file+range, and contains the substring (when given).
# Anything else UNMARKED stays a failure. Line ranges are a snapshot of fix-gates-20261014.
ACCEPTED = [
    ("CActionChara::RunScript", "ixion_toan_combat.cpp", None, "toan_combat",
     "Toan combat hooks: ToanCombatBegin leaves active=false and jumping=0 unless DC2_IXION_PLAYER_COMBAT=dc1 "
     "and DC2_IXION_PLAYER_BODY=toan; the jumping test is then true, the damage-entry branch is skipped only when active"),
    ("calcWeaponParamWhp", "ixion_toan_combat.cpp", None, "ToanCombatTakeWear",
     "ToanCombatTakeWear returns false unless ToanCombatEnabled() and a live donor window/burst owns the prim"),
    ("SetupMainUnit", "ixion_toan_body.cpp", (524, 540), None,
     "Robo part table: const int part_stack[5] = {0,1,2,2,3} replaces SetupPartStack.stack_no; same values and "
     "indices, but NOT verbatim (retail charasetup.cpp:308-326)"),
    ("CMap::GetLightInfo", "ixion_room_atmos.cpp", None, "LightingOwnedBy",
     "ApplyLighting runs only when LightingOwnedBy(Runtime(), this), which needs rt.installed"),
    ("_COLPRIM_CREATE", "ixion_room_ownership.cpp", None, "Running",
     "Running identifies the same script as retail now_script: Step sets EffScriptMan, and argc >= 1 "
     "keeps the EXT operand pointer inside its live stack; native EffectWorld executes both opcodes "
     "without an installed room, with no invariant miss or ownership growth"),
    ("_COLPRIM_CREATE", "ixion_room_ownership.cpp", (77, 77), "StackString",
     "StackString returns slot->val.s exactly as retail GetStackString; moving the attribution hook "
     "after SetDamage puts this unchanged helper alias outside the heuristic's 20-token lookahead. "
     "IxionRoomRound2.NativeEffectWorldKeepsCreationAndOpcodesWithoutOwnershipGrowth executes the real "
     "damage-name opcode and asserts its active primitive retains native owner 25"),
    ("_ES_CREATE", "ixion_room_ownership.cpp", None, "Running",
     "The same stack-range proof applies to ES_CREATE; its native child retains the parent's user_id. "
     "The file-static retail now_script cannot be reached without replacing Step or editing ps2"),
    ("CColPrim::SetDamage", "ixion_dc1_monster.cpp", (1282, 1311), None,
     "Reviewed donor-R4 copy: param renamed row, explicit this removed, std C functions, braces and "
     "target ternary retain all native writes/returns. The only added call observes successful owner-0 "
     "non-melee damage creation; PlayerAttackStarted returns without donor/host or while paused. "
     "IxionDc1Werewolf.MotionDamageResetAndGuardSearchObserveAcceptedHostAttacks asserts native 245 age "
     "0 after an owner-0 ranged SetDamage, age 1 after a missing row or foreign-owner shot, and age 3 "
     "through an event. It does not establish retirement coverage"),
    ("CMonsterMan::CollisionCheck", "ixion_room_monster.cpp", None, "printf",
     "HIT receipt moved after the same collision's catch_state=NONE; damage and physics writes are unchanged, "
     "and the throw is already consumed before another overlap query"),
]


def body_digest(body):
    """Freeze a reviewed token stream; whitespace and comments do not affect it."""
    return hashlib.sha256(" ".join(t for t, _ in body).encode("utf-8")).hexdigest()


def accepted_reason(name, path, line, hunk_text, reviewed_pair=None):
    if reviewed_pair is not None:
        # Both bodies must still match the independently reviewed snapshot. A
        # function-name exception alone would silently accept future regressions.
        for row in REVIEWED_BODIES:
            if (row["name"], row["path"], row["override_sha256"], row["retail_sha256"]) == (
                    name, path, *reviewed_pair):
                return row["reason"]
    for a_name, a_file, a_range, a_sub, reason in ACCEPTED:
        if a_name != name or not path.endswith(a_file):
            continue
        if a_range and not (a_range[0] <= line <= a_range[1]):
            continue
        if a_sub and a_sub not in hunk_text:
            continue
        return reason
    return None


with open(os.path.join(os.path.dirname(__file__), "verbatim_accepted.json"), encoding="utf-8") as _reviewed:
    REVIEWED_BODIES = json.load(_reviewed)


def git(repo, *args, stdin=None):
    res = subprocess.run(["git", "-C", repo, *args], input=stdin, capture_output=True, env=GIT_ENV)
    if res.returncode != 0:
        sys.exit("git %s failed: %s" % (" ".join(args), res.stderr.decode(errors="replace")))
    return res.stdout


def list_files(repo, branch, prefix):
    out = git(repo, "ls-tree", "-r", "--name-only", branch, "--", prefix).decode()
    return [p for p in out.splitlines() if p.endswith(SRC_EXT)]


def cat_files(repo, branch, paths):
    """Return {path: text} using one `git cat-file --batch` call."""
    if not paths:
        return {}
    specs = "".join("%s:%s\n" % (branch, p) for p in paths).encode()
    out = git(repo, "cat-file", "--batch", stdin=specs)
    result, pos = {}, 0
    for p in paths:
        nl = out.index(b"\n", pos)
        header = out[pos:nl].decode(errors="replace")
        pos = nl + 1
        if header.endswith("missing"):
            result[p] = None
            continue
        size = int(header.split()[2])
        result[p] = out[pos:pos + size].decode("utf-8", errors="replace")
        pos += size + 1
    return result


def tokenize(text):
    """Return [(token, line)] with comments and whitespace removed."""
    toks, line = [], 1
    for m in TOK_RE.finditer(text):
        s = m.group(0)
        if m.lastgroup not in ("ws", "lc", "bc"):
            toks.append((s, line))
        line += s.count("\n")
    return toks


def match_close(texts, i, open_s, close_s):
    depth = 0
    for k in range(i, len(texts)):
        if texts[k] == open_s:
            depth += 1
        elif texts[k] == close_s:
            depth -= 1
            if depth == 0:
                return k
    return None


def name_before(texts, p):
    """Qualified name ending just before the '(' at index p. Returns (name, start_index) or (None, None)."""
    j = p - 1
    if j < 1:
        return None, None
    if texts[j - 1] == "operator" and not IDENT_RE.match(texts[j]):
        parts, j = ["operator" + texts[j]], j - 2
    elif IDENT_RE.match(texts[j]) and texts[j] not in CONTROL:
        parts, j = [texts[j]], j - 1
    else:
        return None, None
    start = j + 1
    while j >= 1 and texts[j] == "::" and IDENT_RE.match(texts[j - 1]):
        parts.insert(0, texts[j - 1])
        j -= 2
        start = j + 1
    return "::".join(parts), start


def has_port_override(texts, start):
    """Index of a PORT_OVERRIDE token in the declaration run before `start`, or None."""
    j = start - 1
    while j >= 0 and texts[j] not in (";", "{", "}"):
        if texts[j] == "PORT_OVERRIDE":
            return j
        j -= 1
    return None


def skeleton(params):
    """Parameter types only: identifiers that name a parameter are dropped."""
    out = []
    for i, t in enumerate(params):
        nxt = params[i + 1] if i + 1 < len(params) else ")"
        prev = params[i - 1] if i > 0 else None
        if (IDENT_RE.match(t) and nxt in (",", ")", "[") and prev is not None
                and (IDENT_RE.match(prev) or prev in ("*", "&", ">"))):
            continue
        out.append(t)
    return " ".join(out)


def find_defs(text):
    toks = tokenize(text)
    texts = [t for t, _ in toks]
    defs = []
    for p, t in enumerate(texts):
        if t != "(":
            continue
        name, start = name_before(texts, p)
        if name is None:
            continue
        if start >= 1 and texts[start - 1] in (".", "->"):
            continue
        close = match_close(texts, p, "(", ")")
        if close is None:
            continue
        k = close + 1
        while k < len(texts) and texts[k] == "const":
            k += 1
        if k >= len(texts) or texts[k] != "{":
            continue
        bclose = match_close(texts, k, "{", "}")
        if bclose is None:
            continue
        port_idx = has_port_override(texts, start)
        defs.append({
            "name": name,
            "line": toks[start][1],
            "port": port_idx is not None,
            "port_line": toks[port_idx][1] if port_idx is not None else None,
            "skel": skeleton(texts[p + 1:close]),
            "body": toks[k:bclose + 1],
        })
    return defs


def claim_text(lines, line_no):
    """Comment block directly above line_no (1-based)."""
    out, i = [], line_no - 2
    while i >= 0:
        s = lines[i].strip()
        if s.startswith(("//", "/*", "*")):
            out.insert(0, s)
            i -= 1
        else:
            break
    return " ".join(out)


def blame_commit(repo, branch, path, line):
    """(short sha, YYYY-MM-DD) of the commit that last changed `line` (git blame, committer date)."""
    out = git(repo, "blame", "-L", "%d,%d" % (line, line), "--porcelain", branch, "--", path).decode(errors="replace")
    sha = out.split()[0][:10]
    t = int(re.search(r"^committer-time (\d+)", out, re.M).group(1))
    return sha, time.strftime("%Y-%m-%d", time.localtime(t))


def short(s, n=240):
    return s if len(s) <= n else s[:n] + " ..."


def main():
    # Retail identifiers include Japanese text; redirected Windows output must
    # preserve the audit rather than fail midway through a valid report.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", default=DEFAULT_REPO)
    ap.add_argument("--branch", default=DEFAULT_BRANCH)
    ap.add_argument("--all", action="store_true", help="consider every PORT_OVERRIDE; still filtered by --since declaration-line blame date (use --since 0000-01-01 for all history)")
    ap.add_argument("--name", action="append", default=[], help="only detail these qualified names")
    ap.add_argument("--max-hunks", type=int, default=60)
    ap.add_argument("--since", default="2026-10-14", help="YYYY-MM-DD; blame date cut-off for 'today's lanes'")
    args = ap.parse_args()

    ov_paths = list_files(args.repo, args.branch, "port/src")
    ps_paths = list_files(args.repo, args.branch, "ps2/src")
    if not ov_paths or not ps_paths:
        print("no port/src or ps2/src files on %s" % args.branch, file=sys.stderr)
        return 2
    ov_text = cat_files(args.repo, args.branch, ov_paths)
    ps_text = cat_files(args.repo, args.branch, ps_paths)

    overrides = []
    for path in ov_paths:
        text = ov_text.get(path)
        if not text:
            continue
        lines = text.splitlines()
        for d in find_defs(text):
            if not d["port"]:
                continue
            d["path"] = path
            d["claim"] = claim_text(lines, d["port_line"])
            overrides.append(d)

    retail_by_name = {}
    for path in ps_paths:
        text = ps_text.get(path)
        if not text:
            continue
        for d in find_defs(text):
            retail_by_name.setdefault(d["name"], []).append((path, d))

    for o in overrides:
        best = None
        for path, r in retail_by_name.get(o["name"], []):
            ratio = difflib.SequenceMatcher(None, o["skel"].split(), r["skel"].split(), autojunk=False).ratio()
            if best is None or ratio > best[0]:
                best = (ratio, path, r)
        o["match"] = best  # (ratio, retail_path, retail_def) or None

    # Duplicate checks.
    dup_fail = False
    by_key = {}
    for o in overrides:
        by_key.setdefault((o["name"], o["skel"]), []).append(o)
    print("== DUPLICATE CHECK 1: same qualified name and parameter skeleton defined by >1 PORT_OVERRIDE")
    dups = [(k, v) for k, v in by_key.items() if len(v) > 1]
    for (name, skel), v in dups:
        dup_fail = True
        print("  DUP %s (%s): %s" % (name, skel, ", ".join("%s:%d" % (o["path"], o["line"]) for o in v)))
    if not dups:
        print("  none")

    print("== DUPLICATE CHECK 2: exact-signature match resolves to the same retail definition")
    by_target = {}
    for o in overrides:
        if o["match"] and o["match"][0] >= 0.999:
            _, rpath, rdef = o["match"]
            by_target.setdefault((rpath, rdef["line"], o["name"]), []).append(o)
    tdups = [(k, v) for k, v in by_target.items() if len(v) > 1]
    for (rpath, rline, name), v in tdups:
        dup_fail = True
        print("  DUP-TARGET %s at %s:%d: %s" % (name, rpath, rline,
              ", ".join("%s:%d" % (o["path"], o["line"]) for o in v)))
    if not tdups:
        print("  none")

    unmatched = [o for o in overrides if not o["match"]]
    print("== OVERRIDES: %d total, %d without a retail definition of the same name" % (len(overrides), len(unmatched)))
    for o in unmatched:
        print("  NO-RETAIL %s %s:%d" % (o["name"], o["path"], o["line"]))

    candidates = [o for o in overrides
                  if args.all
                  or "ixion" in o["path"].lower()
                  or CLAIM_RE.search(o["claim"])
                  or o["name"].split("::")[-1] in EVENT_NAMES
                  or o["name"] in args.name]
    # Keep only overrides whose PORT_OVERRIDE line was last changed on or after --since (git blame).
    selected = []
    for o in candidates:
        sha, when = blame_commit(args.repo, args.branch, o["path"], o["port_line"])
        o["blame"] = (sha, when)
        if when >= args.since:
            selected.append(o)
    if args.name:
        selected = [o for o in selected if o["name"] in args.name] or selected
    print("== SELECTED: %d overrides (candidates %d) touched on/after %s by blame"
          % (len(selected), len(candidates), args.since))

    total_unmarked = 0
    print("== BODY DIFFS: %d selected overrides" % len(selected))
    for o in selected:
        print()
        print("#### %s  %s:%d  (PORT_OVERRIDE line blame %s %s)"
              % (o["name"], o["path"], o["line"], o["blame"][0], o["blame"][1]))
        print("  claim: %s" % short(o["claim"] or "(no comment block)", 200))
        if not o["match"]:
            print("  NO RETAIL MATCH")
            continue
        ratio, rpath, rdef = o["match"]
        print("  retail: %s:%d  skeleton-match=%.2f  override-body=%d tokens  retail-body=%d tokens"
              % (rpath, rdef["line"], ratio, len(o["body"]), len(rdef["body"])))
        a = [t for t, _ in rdef["body"]]
        b = [t for t, _ in o["body"]]
        sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
        hunks = [op for op in sm.get_opcodes() if op[0] != "equal"]
        counts = {"MARKED": 0, "NEAR": 0, "UNMARKED": 0}
        rendered = []
        for tag, i1, i2, j1, j2 in hunks:
            # Ixion identifier inside the hunk, else nearest one before it or just after it.
            inside = any(IXION_RE.search(b[k]) for k in range(j1, j2))
            near_idx = None
            for k in range(j2 - 1, max(-1, j1 - 201), -1):
                if IXION_RE.search(b[k]):
                    near_idx = k
                    break
            if near_idx is None:
                for k in range(j2, min(len(b), j2 + 20)):
                    if IXION_RE.search(b[k]):
                        near_idx = k
                        break
            r_line = rdef["body"][min(i1, len(a) - 1)][1] if a else rdef["line"]
            o_line = o["body"][min(j1, len(b) - 1)][1] if b else o["line"]
            rtxt = short(" ".join(a[i1:i2]), 300)
            otxt = short(" ".join(b[j1:j2]), 300)
            if inside:
                status = "MARKED"
            elif near_idx is not None:
                status = "NEAR"
            else:
                status = "UNMARKED"
                reason = accepted_reason(o["name"], o["path"], o_line, rtxt + " " + otxt,
                                         (body_digest(o["body"]), body_digest(rdef["body"])))
                if reason:
                    status = "ACCEPTED"
                    rtxt = rtxt + "  [accepted: " + reason + "]"
            counts[status] = counts.get(status, 0) + 1
            if status == "UNMARKED":
                total_unmarked += 1
            ctx = " ".join(b[max(0, j1 - 8):j1])
            marker = ("nearest ixion token %s:%d" % (o["path"], o["body"][near_idx][1])) if near_idx is not None else "none"
            rendered.append("  [%s %s] retail %s:%d | override %s:%d\n"
                            "      - R: %s\n      + O: %s\n      ctx: ...%s\n      marker: %s"
                            % (status, tag, rpath, r_line, o["path"], o_line, rtxt or "(none)",
                               otxt or "(none)", short(ctx, 160), marker))
        print("  hunks=%d  MARKED=%d  NEAR=%d  ACCEPTED=%d  UNMARKED=%d"
              % (len(hunks), counts["MARKED"], counts["NEAR"], counts.get("ACCEPTED", 0), counts["UNMARKED"]))
        for r in rendered[:args.max_hunks]:
            print(r)
        if len(rendered) > args.max_hunks:
            print("  ... %d more hunks not shown (--max-hunks)" % (len(rendered) - args.max_hunks))

    print()
    print("== RESULT: unmarked hunks=%d, duplicates=%s" % (total_unmarked, "yes" if dup_fail else "no"))
    return 1 if (total_unmarked or dup_fail) else 0


if __name__ == "__main__":
    sys.exit(main())
