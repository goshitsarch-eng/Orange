#!/usr/bin/env python3
"""Launch an extracted release without SDK paths, then close via the UI shortcut."""
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import time

archive = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='orange-bundle-smoke-') as temporary:
    root = Path(temporary)
    with tarfile.open(archive) as tar:
        tar.extractall(root, filter='data')
    bundle = next(root.glob('orange-*'))
    profile = root / 'profile'
    env = dict(os.environ)
    for key in list(env):
        if key.startswith(('GST_', 'GSTREAMER_', 'CARGO_', 'RUSTUP_', 'PUB_', 'FLUTTER_')) or key in ('LD_LIBRARY_PATH', 'PKG_CONFIG_PATH', 'PKG_CONFIG_SYSROOT_DIR', 'GIO_MODULE_DIR', 'ORANGE_RUST_LIBRARY'):
            del env[key]
    env.update(PATH='/usr/bin:/bin', ORANGE_PROFILE_DIR=str(profile), ORANGE_AUDIO_OUTPUT='null')
    subprocess.check_call([str(bundle / 'orange-cli'), '--version'], env=env)
    subprocess.check_call([str(bundle / 'orange-cli'), '--headless'], env=env)
    # X11 is used only by this QA harness; the application also supports Wayland.
    x11 = ctypes.CDLL('libX11.so.6')
    x11.XOpenDisplay.restype = ctypes.c_void_p
    display = x11.XOpenDisplay(None)
    if not display:
        raise RuntimeError('Run this test under Xvfb or an X11 session')
    x11.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
    x11.XDefaultRootWindow.restype = ctypes.c_ulong
    x11.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)), ctypes.POINTER(ctypes.c_uint)]
    x11.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_char_p)]
    x11.XFree.argtypes = [ctypes.c_void_p]

    def find_window():
        parent, returned_root = ctypes.c_ulong(), ctypes.c_ulong()
        children = ctypes.POINTER(ctypes.c_ulong)()
        count = ctypes.c_uint()
        x11.XQueryTree(display, x11.XDefaultRootWindow(display), ctypes.byref(returned_root), ctypes.byref(parent), ctypes.byref(children), ctypes.byref(count))
        found = None
        for i in range(count.value):
            name = ctypes.c_char_p()
            if x11.XFetchName(display, children[i], ctypes.byref(name)) and name.value:
                if name.value.decode(errors='replace') == 'Orange Music Player':
                    found = children[i]
                x11.XFree(name)
        if children:
            x11.XFree(children)
        return found

    log = root / 'application.log'
    with log.open('w') as output:
        started = time.monotonic()
        process = subprocess.Popen([str(bundle / 'orange')], env=env, stdout=output, stderr=subprocess.STDOUT)
        try:
            window = None
            for _ in range(150):
                if process.poll() is not None:
                    raise RuntimeError(f'Packaged GUI exited early: {log.read_text()}')
                window = find_window()
                if window and (profile / 'data/orange/orange/orange.db').is_file():
                    break
                time.sleep(0.1)
            if not window:
                raise RuntimeError('Packaged Orange did not create its native window')
            startup = time.monotonic() - started
            memory = next((line for line in Path(f'/proc/{process.pid}/status').read_text().splitlines() if line.startswith('VmRSS:')), 'RSS unavailable')
            print(f'Empty-profile native startup: {startup:.3f}s; {memory}')
            time.sleep(1)
            x11.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
            x11.XSetInputFocus(display, window, 1, 0)
            x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
            x11.XKeysymToKeycode.restype = ctypes.c_uint
            xtst = ctypes.CDLL('libXtst.so.6')
            xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
            control = x11.XKeysymToKeycode(display, 0xffe3)
            key_q = x11.XKeysymToKeycode(display, ord('q'))
            for key, pressed in [(control, 1), (key_q, 1), (key_q, 0), (control, 0)]:
                xtst.XTestFakeKeyEvent(display, key, pressed, 50)
            x11.XFlush.argtypes = [ctypes.c_void_p]
            x11.XFlush(display)
            if process.wait(timeout=15) != 0:
                raise RuntimeError(f'Packaged Orange quit failed: {log.read_text()}')
            if not (profile / 'config/orange/desktop.json').is_file():
                raise RuntimeError('UI quit did not persist settings')
            if any(marker in log.read_text() for marker in ['Orange startup failed', 'Unhandled Exception', 'Could not load']):
                raise RuntimeError(log.read_text())
            print('Extracted Flutter archive: native startup, Rust database, Ctrl+Q and settings persistence passed')
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
    x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
    x11.XCloseDisplay(display)
