vswm - very stupid window manager
=================================

Probably the most stupid window manager ever created, written over an ancient
relic of a library called Xlib -- a library so old that it preceded the birth
of planet Earth itself.

- There are no workspaces.
- All windows are maximised.
- Windows can not be moved or resized.
- Only one window is visible at a time.
- This certainly isn't for everyone.

Screenshot: https://www.reddit.com/r/unixporn/comments/pbf4wu/vswm_hello_friend/


Keybindings
-----------

MOD4 + Tab               focus next window
MOD4 + Shift + Tab       focus prev window
MOD4 + Shift + q         kill window
MOD4 + Shift + r         refresh wm [*]

MOD4 + b                 firefox
MOD4 + Return            xterm
MOD4 + d                 dmn [1]
MOD4 + p                 scr [2]

XF86_MonBrightnessDown   xbacklight -dec 5
XF86_MonBrightnessUp     xbacklight -inc 5
XF86_AudioLowerVolume    pamixer -d 5
XF86_AudioRaiseVolume    pamixer -i 5
XF86_AudioMute           pamixer -t

[*] Resize and reposition windows. Useful when connecting or disconnecting an
    external monitor, if e.g. screen size differ.

[1] Launcher script for dmenu:
    https://github.com/fehawen/bin/blob/master/scripts/dmn

[2] Screenshot script:
    https://github.com/fehawen/bin/blob/master/scripts/scr


Configuration
-------------

Modify the keybindings to your liking.


Dependencies
------------

You need the Xlib header files.


Installation
------------

Clean.

    $ make clean

Compile.

    $ make

Install.

    $ make install

All in one go.

    $ make clean install

Uninstall.

    $ make uninstall

You may need to run install as root.
DESTDIR and PREFIX are supported.


Credits
-------

i3: https://github.com/i3/i3
dwm: https://git.suckless.org/dwm
sowm: https://github.com/dylanaraps/sowm
berry: https://github.com/JLErvin/berry
tinywm: http://incise.org/tinywm.html
katriawm: https://www.uninformativ.de/git/katriawm
