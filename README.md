##snapwm
### it's minimal and dynamic

I started this from catwm 31/12/10 ( https://bbs.archlinux.org/viewtopic.php?id=100215&p=1 )
    See snapwm.c or config.h for thanks and licensing.
Screenshots and ramblings/updates at https://bbs.archlinux.org/viewtopic.php?id=126463


###Summary
-------


**snapwm** is a very minimal and lightweight dynamic tiling window manager.


###Modes
-----

It allows the "normal" method of tiling window managers(with the new window as the master)
    and with the new window opened at the bottom of the stack(like dwm's attach_aside)

 *There's vertical tiling mode:*

    --------------
    |        | W |
    |        |___|
    | Master |   |
    |        |___|
    |        |   |
    --------------

 *Horizontal tiling mode:*

    -------------
    |           |
    |  Master   |
    |-----------|
    | W |   |   |
    -------------

 *Grid tiling mode:*

    -------------
    |      | W  |
    |Master|    |
    |------|----|
    |      |    |
    -------------

 *Stacking mode:*

    -------------
    |   _______  |
    |  |  ___  | |
    |  | |___| | |
    |  |_______| |
    -------------


 *Fullscreen mode*(which you'll know when you see it)

 All accessible with keyboard shortcuts defined in the config.h file.
 
 * The window *W* at the top of the stack can be resized on a per desktop basis.
 * Changing a tiling mode or window size on one desktop doesn't affect the other desktops.
 * There is a bar with a desktop switcher, space to show the focused window's name and space to show external text.
 * The rc file is reloadable 'on the run'.


###Recent Changes
--------------

13/4/12

  * Seperated the window and bar colours in the config.h and rc files



###Status
------

There are more options in the config file than the original catwm.

  * Fixed the window manager crashing on a bad window error.
  * Fixed the keyboard shortcuts not working if numlock was on.
  * Added some functions.
  * Added an option to focus the window the mouse just moved to.
  * Fixed a window being destroyed on another desktop creating ghost windows.
  * Added ability to resize the window on the top of the stack
  * Added having applications open on specified desktop
  * Added a click to focus option
  * Added ability to change back to last opened desktop.
  * Added bar with desktop switcher and statusbar.
  * Colours and font are read from an rc file and can be updated with a keyboard shortcut.
  * Text (e.g. conky) can be piped into the status bar from .xinitrc.
  * Unfocused windows have an alpha value so can be transparent if e.g. cairo-compmgr is used
  * Lots of things can be changed in the running wm from the rc file.
	* e.g. Whether the bar is at the top or bottom
	* font, colours, border width, default mode etc (see the sample rc file).
  * In the rc file, lines starting with a hash are ignored.
  * Desktop switcher can show number of open windows on unfocused desktops and in fullscreen mode
  * Option to show number of windows open in the desktop switcher in the config and rc file
  * Clicking on the current desktop in the switcher will focus the next window
  * Added option in the config and rc files for opening new window at the 
  top or bottom of the stack when using attach aside
  * Added a stacking window mode


###Installation
------------

Need Xlib, then:

    Copy the rc file to $HOME and edit it to suit.
    Edit the config.h.def file to suit your needs and
        save it as config.h making sure to add
        the correct path for the rc file.

    $ make
    # make install
    $ make clean


###Bugs
----

[ * No bugs for the moment ;) (I mean, no importants bugs ;)]


###Todo
----

  * Improve font handling
  * Add commands and keyboard shortcuts to rc file.
