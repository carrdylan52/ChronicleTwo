"""Exercise the installed launcher with isolated disc and child-process fixtures."""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
import unittest


def both(value):
    return struct.pack("<I", value) + struct.pack(">I", value)


def record(name, sector, size=2048, directory=False):
    encoded = name.encode("ascii")
    length = 33 + len(encoded)
    length += length % 2
    result = bytearray(length)
    result[0] = length
    result[2:10] = both(sector)
    result[10:18] = both(size)
    result[25] = 2 if directory else 0
    result[28:32] = b"\x01\x00\x00\x01"
    result[32] = len(encoded)
    result[33:33 + len(encoded)] = encoded
    return result


def disc(path, region="SCES_511.90", missing=None):
    data = bytearray(40 * 2048)
    descriptor = bytearray(2048)
    descriptor[0:7] = b"\x01CD001\x01"
    descriptor[80:88] = both(40)
    descriptor[128:132] = b"\x00\x08\x08\x00"
    descriptor[156:190] = record("\x00", 20, directory=True)
    data[16 * 2048:17 * 2048] = descriptor
    entries = [(region + ";1", 21, False), ("SYSTEM.CNF;1", 22, False),
               ("DATA.HD4;1", 23, False), ("DATA.DAT;1", 24, False),
               ("SOUND.HD3;1", 25, False), ("SOUND.DAT;1", 26, False), ("MOVIE", 27, True)]
    root = record("\x00", 20, directory=True) + record("\x01", 20, directory=True)
    for name, sector, directory in entries:
        if name.split(";")[0] != missing:
            root += record(name, sector, directory=directory)
    data[20 * 2048:20 * 2048 + len(root)] = root
    data[21 * 2048:21 * 2048 + 4] = b"\x7fELF"
    boot = f"BOOT2 = cdrom0:\\{region};1\r\n".encode()
    data[22 * 2048:22 * 2048 + len(boot)] = boot
    path.write_bytes(data)
    return path


class Portable(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="c2-portable-")
        self.root = Path(self.temporary.name)
        self.package = self.root / "Portable game 日本語 & spaces"
        self.package.mkdir()
        shutil.copy2(ARGS.launcher, self.package / "Play.exe")
        for name in ("darkchronicle.exe", "dc2data.exe"):
            shutil.copy2(ARGS.fixture, self.package / name)
        self.iso = disc(self.root / "My disc 日本語.iso")
        self.save = self.package / "userdata/saves/saves/progress.bin"
        self.save.parent.mkdir(parents=True)
        self.save.write_bytes(b"existing progress must survive")

    def tearDown(self):
        self.temporary.cleanup()

    def run_launcher(self, *arguments, env=None):
        return subprocess.run([str(self.package / "Play.exe"), "--no-ui", *map(str, arguments)],
                              cwd=self.root, env=env, capture_output=True, text=True, encoding="utf-8", timeout=20)

    def calls(self):
        path = self.package / "calls.jsonl"
        return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()] if path.exists() else []

    def prepare(self):
        result = self.run_launcher("--iso", self.iso, "--prepare-only")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_first_run_extracts_before_game_and_preserves_progress(self):
        result = self.run_launcher("--iso", self.iso)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([call["program"] for call in self.calls()], ["extractor", "game"])
        self.assertEqual(Path(self.calls()[0]["arguments"][2]), self.package / "userdata/data")
        self.assertEqual(Path(self.calls()[1]["arguments"][1]), self.package / "userdata")
        self.assertEqual(self.save.read_bytes(), b"existing progress must survive")

    def test_repeat_launch_does_not_need_iso_or_extract_again(self):
        self.prepare()
        self.iso.unlink()
        setup_log = self.package / "userdata/logs/setup.log"
        stamp = setup_log.stat().st_mtime_ns
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([call["program"] for call in self.calls()], ["extractor", "game"])
        self.assertEqual(setup_log.stat().st_mtime_ns, stamp)

    def test_relocation_and_windows_argument_quoting(self):
        self.prepare()
        destination = self.root / "Moved folder é 日本語"
        self.package.rename(destination)
        self.package = destination
        arguments = ["a b", 'quote"inside', "", "trailing\\", 'space trailing\\', "日本語", "$(not-a-command)"]
        result = self.run_launcher("--", *arguments)
        self.assertEqual(result.returncode, 0, result.stderr)
        call = self.calls()[-1]
        self.assertEqual(call["arguments"][2:], arguments)
        self.assertEqual(Path(call["arguments"][1]), destination / "userdata")
        self.assertEqual(Path(call["cwd"]), destination)

    def test_replacing_binaries_preserves_config_and_saves(self):
        self.prepare()
        config = self.package / "userdata/saves/config.json"
        config.write_bytes(b'{"custom":"keep"}')
        shutil.copy2(ARGS.launcher, self.package / "Play.exe")
        self.assertEqual(self.run_launcher().returncode, 0)
        self.assertEqual(config.read_bytes(), b'{"custom":"keep"}')
        self.assertEqual(self.save.read_bytes(), b"existing progress must survive")

    def test_wrong_region_never_starts_extractor_or_game(self):
        disc(self.iso, region="SCUS_972.13")
        result = self.run_launcher("--iso", self.iso)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("PAL European", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_extraction_failure_preserves_previous_files_and_does_not_launch(self):
        stale = self.package / "userdata/data.json"
        stale.write_bytes(b'{"schema_version":1,"data_version":0}')
        env = dict(os.environ, CT_PORTABLE_FIXTURE_FAIL="1")
        result = self.run_launcher("--iso", self.iso, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual([call["program"] for call in self.calls()], ["extractor"])
        self.assertEqual(stale.read_bytes(), b'{"schema_version":1,"data_version":0}')
        self.assertEqual(self.save.read_bytes(), b"existing progress must survive")

    def test_incomplete_package_has_actionable_error(self):
        (self.package / "dc2data.exe").unlink()
        result = self.run_launcher()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("entire ZIP", result.stderr)

    def test_incompatible_data_requires_setup(self):
        self.prepare()
        marker = self.package / "userdata/data.json"
        for value in ({"schema_version":1,"data_version":0}, {"schema_version":1,"data_version":float(ARGS.version)}, [], "invalid"):
            with self.subTest(marker=value):
                marker.write_text(json.dumps(value), encoding="utf-8")
                result = self.run_launcher()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Choose your PAL", result.stderr)

    def test_missing_converted_manifest_requires_setup(self):
        self.prepare()
        (self.package / "userdata/port_data/manifest.json").unlink()
        self.assertNotEqual(self.run_launcher().returncode, 0)

    def test_portable_root_cannot_be_overridden_by_game_arguments(self):
        result = self.run_launcher("--", "--save=" + str(self.root / "elsewhere"))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.calls(), [])

    def test_second_instance_cannot_share_live_game_root(self):
        self.prepare()
        first = subprocess.Popen([str(self.package / "Play.exe"), "--no-ui", "--", "--fixture-sleep"],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 10
            while len(self.calls()) < 2 and first.poll() is None and time.monotonic() < deadline:
                time.sleep(.02)
            self.assertIsNone(first.poll())
            second = self.run_launcher()
            self.assertNotEqual(second.returncode, 0)
            self.assertIn("already running", second.stderr)
            output, error = first.communicate(timeout=10)
            self.assertEqual(first.returncode, 0, error)
        finally:
            if first.poll() is None:
                first.terminate()
                first.communicate(timeout=10)

    def test_game_failure_reaches_launcher_exit_status(self):
        self.prepare()
        result = self.run_launcher(env=dict(os.environ, CT_PORTABLE_FIXTURE_GAME_FAIL="1"))
        self.assertEqual(result.returncode, 4)
        self.assertIn("game.log", result.stderr)

    def test_crash_log_survives_next_launch(self):
        self.prepare()
        self.assertEqual(self.run_launcher(env=dict(os.environ, CT_PORTABLE_FIXTURE_GAME_FAIL="1")).returncode, 4)
        log = self.package / "userdata/logs/game.log"
        failure = log.read_bytes()
        self.assertIn(b"retain this log", failure)
        self.assertEqual(self.run_launcher().returncode, 0)
        self.assertEqual((log.parent / "game.previous.log").read_bytes(), failure)

    def test_launcher_crash_leaves_game_alive_and_folder_locked(self):
        self.prepare()
        first = subprocess.Popen([str(self.package / "Play.exe"), "--no-ui", "--", "--fixture-sleep"],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 10
            while len(self.calls()) < 2 and first.poll() is None and time.monotonic() < deadline:
                time.sleep(.02)
            self.assertEqual(len(self.calls()), 2)
            first.terminate()
            first.communicate(timeout=3)
            self.assertIn("already running", self.run_launcher().stderr)
            done = self.package / "game.done"
            deadline = time.monotonic() + 10
            while not done.exists() and time.monotonic() < deadline:
                time.sleep(.02)
            self.assertTrue(done.exists(), "Killing the launcher killed the running game.")
            # The child has to exit and close its inherited lock after writing the marker.
            time.sleep(.1)
            self.assertEqual(self.run_launcher().returncode, 0)
        finally:
            if first.poll() is None:
                first.terminate()
                first.communicate(timeout=10)

    def test_iso_identity_and_corruption_checks(self):
        def check():
            return subprocess.run([str(self.package / "Play.exe"), "--check-iso", str(self.iso)],
                                  capture_output=True, text=True, timeout=10)
        self.assertEqual(check().returncode, 0)
        for missing in ("DATA.HD4", "DATA.DAT", "SOUND.HD3", "SOUND.DAT", "MOVIE"):
            with self.subTest(missing=missing):
                disc(self.iso, missing=missing)
                self.assertNotEqual(check().returncode, 0)
        valid = disc(self.iso).read_bytes()
        for offset, value in ((16 * 2048 + 87, 0), (20 * 2048, 10), (21 * 2048, 0), (16 * 2048 + 128, 1)):
            with self.subTest(corruption=offset):
                damaged = bytearray(valid)
                damaged[offset] = value
                self.iso.write_bytes(damaged)
                self.assertNotEqual(check().returncode, 0)
        self.iso.write_bytes(valid[:30 * 2048])
        self.assertNotEqual(check().returncode, 0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--version", type=int, required=True)
    ARGS, remaining = parser.parse_known_args()
    ARGS.launcher = ARGS.launcher.resolve()
    ARGS.fixture = ARGS.fixture.resolve()
    unittest.main(argv=[__file__, *remaining])
