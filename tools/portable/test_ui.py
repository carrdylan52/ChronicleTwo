"""Interactive desktop QA of this launcher's native controls and ISO picker."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
from pathlib import Path
import shutil
import subprocess
import time

from test_portable import disc

u = c.WinDLL("user32", use_last_error=True)
g = c.WinDLL("gdi32", use_last_error=True)
k = c.WinDLL("kernel32", use_last_error=True)
u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.GetDlgItem.argtypes = [w.HWND, c.c_int]
u.GetDlgItem.restype = w.HWND
u.GetParent.argtypes = [w.HWND]
u.GetParent.restype = w.HWND
u.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
u.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
u.SetWindowTextW.argtypes = [w.HWND, w.LPCWSTR]
u.GetWindowTextW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
u.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
u.IsWindowEnabled.argtypes = [w.HWND]
u.PrintWindow.argtypes = [w.HWND, w.HDC, w.UINT]
u.GetDC.argtypes = [w.HWND]
u.GetDC.restype = w.HDC
g.CreateCompatibleDC.argtypes = [w.HDC]
g.CreateCompatibleDC.restype = w.HDC
g.CreateCompatibleBitmap.argtypes = [w.HDC, c.c_int, c.c_int]
g.CreateCompatibleBitmap.restype = w.HBITMAP
g.SelectObject.argtypes = [w.HDC, w.HGDIOBJ]
g.SelectObject.restype = w.HGDIOBJ
g.GetDIBits.argtypes = [w.HDC, w.HBITMAP, w.UINT, w.UINT, c.c_void_p, c.c_void_p, w.UINT]
g.DeleteObject.argtypes = [w.HGDIOBJ]
g.DeleteDC.argtypes = [w.HDC]
u.ReleaseDC.argtypes = [w.HWND, w.HDC]
Callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
u.EnumWindows.argtypes = [Callback, w.LPARAM]
u.EnumChildWindows.argtypes = [w.HWND, Callback, w.LPARAM]
u.GetDlgCtrlID.argtypes = [w.HWND]
u.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
u.IsWindowVisible.argtypes = [w.HWND]
u.SendMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
u.SetActiveWindow.argtypes = [w.HWND]
u.SetActiveWindow.restype = w.HWND
u.AttachThreadInput.argtypes = [w.DWORD, w.DWORD, w.BOOL]
u.SetForegroundWindow.argtypes = [w.HWND]
u.IsWindow.argtypes = [w.HWND]


def text(handle, class_name=False):
    buffer = c.create_unicode_buffer(256 if class_name else 32768)
    if not class_name and text(handle, True) == "Edit":
        u.SendMessageW(handle, 0xD, len(buffer), c.cast(buffer, c.c_void_p).value)
    else:
        (u.GetClassNameW if class_name else u.GetWindowTextW)(handle, buffer, len(buffer))
    return buffer.value


def windows(pid):
    result = []
    def visit(handle, _):
        owner = w.DWORD()
        u.GetWindowThreadProcessId(handle, c.byref(owner))
        if owner.value == pid:
            result.append(handle)
        return True
    u.EnumWindows(Callback(visit), 0)
    return result


def children(handle):
    result = []
    def visit(child, _):
        result.append(child)
        return True
    u.EnumChildWindows(handle, Callback(visit), 0)
    return result


def replace_text(handle, value):
    buffer = c.create_unicode_buffer(value)
    u.SendMessageW(handle, 0xB1, 0, -1)
    u.SendMessageW(handle, 0xC2, 1, c.cast(buffer, c.c_void_p).value)
    assert text(handle) == value, f"The filename field contains {text(handle)!r}."


def wait(find, seconds=10):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        result = find()
        if result:
            return result
        time.sleep(.03)
    raise AssertionError("Native UI did not reach the expected state.")


def screenshot(handle, path):
    bounds = w.RECT()
    u.GetWindowRect(handle, c.byref(bounds))
    width, height = bounds.right - bounds.left, bounds.bottom - bounds.top
    screen = u.GetDC(handle)
    memory = g.CreateCompatibleDC(screen)
    bitmap = g.CreateCompatibleBitmap(screen, width, height)
    previous = g.SelectObject(memory, bitmap)
    try:
        if not u.PrintWindow(handle, memory, 2):
            raise AssertionError("Could not capture the native setup window.")
        g.SelectObject(memory, previous)
        header = struct_header(width, height)
        pixels = c.create_string_buffer(width * height * 4)
        if not g.GetDIBits(memory, bitmap, 0, height, pixels, header, 0):
            raise AssertionError("Could not read native setup pixels.")
        import struct
        import zlib
        rgba = bytearray(pixels.raw)
        rgba[0::4], rgba[2::4] = pixels.raw[2::4], pixels.raw[0::4]
        rgba[3::4] = b"\xff" * (width * height)
        rows = b"".join(b"\0" + rgba[y * width * 4:(y + 1) * width * 4] for y in range(height))
        def chunk(kind, body):
            return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
        path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
                         chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))
    finally:
        g.DeleteObject(bitmap)
        g.DeleteDC(memory)
        u.ReleaseDC(handle, screen)


def struct_header(width, height):
    import struct
    return c.create_string_buffer(struct.pack("<IiiHHIIiiII", 40, width, -height, 1, 32, 0, width * height * 4, 0, 0, 0, 0))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    package = args.output / "picker package 日本語"
    package.mkdir()
    shutil.copy2(args.launcher, package / "Play.exe")
    for name in ("dc2data.exe", "darkchronicle.exe"):
        shutil.copy2(args.fixture, package / name)
    iso = disc(args.output / "Selected disc 日本語.iso")
    process = subprocess.Popen([str(package / "Play.exe")])
    try:
        main = wait(lambda: next((h for h in windows(process.pid) if text(h, True) == "ChronicleTwoPortable"), None))
        time.sleep(.15)
        assert not u.IsWindowEnabled(u.GetDlgItem(main, 102)), "Prepare is enabled before a disc is selected."
        screenshot(main, args.output / "setup.png")
        u.PostMessageW(main, 0x111, 101, 0)
        picker = wait(lambda: next((h for h in windows(process.pid) if text(h) == "Choose your PAL Dark Chronicle ISO"), None))
        time.sleep(.15)
        screenshot(picker, args.output / "picker.png")
        descendants = children(picker)
        controls = [{"class":text(h, True), "text":text(h), "id":u.GetDlgCtrlID(h), "parent_id":u.GetDlgCtrlID(u.GetParent(h))} for h in descendants]
        (args.output / "picker-controls.json").write_text(json.dumps(controls, ensure_ascii=False, indent=2), encoding="utf-8")
        u.PostMessageW(main, 0x10, 0, 0)
        time.sleep(.1)
        assert u.IsWindow(main), "The picker owner was destroyed while its modal dialog was open."
        u.PostMessageW(picker, 0x10, 0, 0)
        wait(lambda: not u.IsWindow(picker) and u.IsWindowEnabled(main))
        assert not u.IsWindowEnabled(u.GetDlgItem(main, 102)), "Cancelling the picker enabled preparation."
        u.PostMessageW(main, 0x111, 101, 0)
        picker = wait(lambda: next((h for h in windows(process.pid) if text(h) == "Choose your PAL Dark Chronicle ISO"), None))
        edit = wait(lambda: next((h for h in children(picker) if text(h, True) == "Edit" and u.GetDlgCtrlID(u.GetParent(h)) == 1148), None))
        time.sleep(.5)
        u.PostMessageW(edit, 0x100, 27, 0)
        u.PostMessageW(edit, 0x101, 27, 0)
        wait(lambda: not u.IsWindow(picker) and u.IsWindowEnabled(main))
        assert not u.IsWindowEnabled(u.GetDlgItem(main, 102)), "Escape enabled preparation."
        # Paste-path recovery works independently of the dialog, without starting extraction.
        replace_text(u.GetDlgItem(main, 104), '  "' + str(iso.resolve()) + '"  ')
        wait(lambda: u.IsWindowEnabled(u.GetDlgItem(main, 102)))
        screenshot(main, args.output / "pasted-path.png")
        replace_text(u.GetDlgItem(main, 104), "")
        wait(lambda: not u.IsWindowEnabled(u.GetDlgItem(main, 102)))
        u.PostMessageW(main, 0x111, 101, 0)
        picker = wait(lambda: next((h for h in windows(process.pid) if text(h) == "Choose your PAL Dark Chronicle ISO"), None))
        edit = wait(lambda: next((h for h in children(picker) if text(h, True) == "Edit" and u.GetDlgCtrlID(u.GetParent(h)) == 1148), None))
        open_button = wait(lambda: next((h for h in children(picker) if text(h, True) == "Button" and u.GetDlgCtrlID(h) == 1), None))
        time.sleep(.5)  # The shell initializes the filename after creating its child HWNDs.
        replace_text(edit, str(iso.resolve()))
        u.PostMessageW(picker, 0x111, 1, open_button)
        try:
            wait(lambda: not u.IsWindow(picker) and u.IsWindowEnabled(main) and u.IsWindowEnabled(u.GetDlgItem(main, 102)))
        except AssertionError:
            state = []
            for handle in windows(process.pid):
                screenshot(handle, args.output / f"failure-{handle}.png")
                state.append({"window": text(handle), "class": text(handle, True), "controls":
                              [{"class":text(h, True), "text":text(h), "id":u.GetDlgCtrlID(h)} for h in children(handle)]})
            (args.output / "failure-state.json").write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
            raise
        assert any(str(iso.resolve()) == text(h) for h in children(main)), "The selected ISO is not displayed."
        time.sleep(.15)
        screenshot(main, args.output / "selected.png")
        u.PostMessageW(main, 0x111, 102, 0)
        assert process.wait(timeout=15) == 0
        calls = [json.loads(line) for line in (package / "calls.jsonl").read_text(encoding="utf-8").splitlines()]
        assert [call["program"] for call in calls] == ["extractor", "game"]
        print("Native picker: Unicode selection, cancellation, Escape, owner-close protection, paste-path recovery, visible preparation and launch: PASS")
        # Explorer's Copy as path adds quotes; prepare from that pasted entry alone.
        fallback = args.output / "paste package 日本語"
        fallback.mkdir()
        shutil.copy2(args.launcher, fallback / "Play.exe")
        for name in ("dc2data.exe", "darkchronicle.exe"):
            shutil.copy2(args.fixture, fallback / name)
        process = subprocess.Popen([str(fallback / "Play.exe")])
        main = wait(lambda: next((h for h in windows(process.pid) if text(h, True) == "ChronicleTwoPortable"), None))
        replace_text(u.GetDlgItem(main, 104), '  "' + str(iso.resolve()) + '"  ')
        wait(lambda: u.IsWindowEnabled(u.GetDlgItem(main, 102)))
        u.PostMessageW(main, 0x111, 102, 0)
        assert process.wait(timeout=15) == 0
        calls = [json.loads(line) for line in (fallback / "calls.jsonl").read_text(encoding="utf-8").splitlines()]
        assert [call["program"] for call in calls] == ["extractor", "game"]
        assert calls[0]["arguments"][1] == str(iso.resolve())
        print("Quoted Explorer path with surrounding whitespace prepares and launches without opening a dialog: PASS")
    finally:
        if process.poll() is None:
            for handle in windows(process.pid):
                u.PostMessageW(handle, 0x10, 0, 0)
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=3)
