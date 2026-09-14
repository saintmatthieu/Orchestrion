#!/usr/bin/env python3
"""Synthetic X input through XTEST, for driving Orchestrion on an Xvfb display.

  DISPLAY=:99 python3 xin.py move X Y click sleep 1.5 key 6 keydown 7 sleep 0.4 keyup 7 ...

move X Y      absolute pointer position     click [N]  press+release button N (default 1)
key SYM       press+release the keysym      keydown/keyup SYM
sleep S       wait (flushes first)
"""
import sys, time
from Xlib import display, X, XK
from Xlib.ext import xtest

d = display.Display(); args = sys.argv[1:]; i = 0
while i < len(args):
    a = args[i]
    if a == 'move':
        xtest.fake_input(d, X.MotionNotify, x=int(args[i+1]), y=int(args[i+2])); i += 3
    elif a == 'click':
        b = 1
        if i + 1 < len(args) and args[i+1].isdigit():
            b = int(args[i+1]); i += 1
        xtest.fake_input(d, X.ButtonPress, b); d.sync(); time.sleep(0.05)
        xtest.fake_input(d, X.ButtonRelease, b); i += 1
    elif a in ('key', 'keydown', 'keyup'):
        kc = d.keysym_to_keycode(XK.string_to_keysym(args[i+1]))
        if a != 'keyup':
            xtest.fake_input(d, X.KeyPress, kc); d.sync()
        if a == 'key':
            time.sleep(0.08)
        if a != 'keydown':
            xtest.fake_input(d, X.KeyRelease, kc)
        i += 2
    elif a == 'sleep':
        d.sync(); time.sleep(float(args[i+1])); i += 2
    else:
        raise SystemExit('bad arg ' + a)
    d.sync()
