##snapwm
### it's minimal and dynamic

I started this from catwm 31/12/10 ( https://bbs.archlinux.org/viewtopic.php?id=100215&p=1 )
    See snapwm.c for thanks and licensing.
Screenshots and ramblings/updates at https://bbs.archlinux.org/viewtopic.php?id=126463


###Summary
-------


**snapwm** is a xinerama aware, very minimal and lightweight dynamic tiling window manager.

All configuration is read from three files in ~/.config/snapwm/ .

*rc.conf* has colours and window manager configurations.

*key.conf* is mandatory for shortcuts and commands to run.

*apps.conf* is optional and where apps settings are read from.

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

 All accessible with keyboard shortcuts defined in key.conf file.
 
 * The window *W* at the top of the stack can be resized on a per desktop basis.
 * Changing a tiling mode or window size on one desktop doesn't affect the other desktops.
 * Windows can be added/removed to/from the master area with keyboard shortcuts
 * There is a bar with a desktop switcher, space to show the focused window's name and space to show external text.
 * The rc file is reloadable 'on the run'.


###Recent Changes
--------------

4/3/14

  * Watch for WM_NAME on new windows for the apps.conf file


###Status
------

  * Added bar with desktop switcher and statusbar to dminiwm.
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
  * Seperated the window and bar colours in the config.h and rc files
  * Added support for multibyte fonts
  * Added 'double buffer' for text in the bar
  * Changed grid mode: First column and second window can be resized
  * Moved the RC FILE to $HOME/.config/snapwm/rc.conf
  * Windows can be added/removed to/from the master area with keyboard shortcuts
  * Keyboard shortcuts and apps settings moved to files in ~/.config/snapwm/
  * rc.conf changed to semi-colon seperated
  * Number of desktops now read from rc.conf
  * Whether to left align the current windows' name in the bar moved to rc.conf
  * No need to rebuild the wm to change a setting
  * Colours changed in rc.conf
  * Background colour can be changed
  * Added multi monitor support
  * Added option in rc.conf to select the monitor the bar is on
  * Added option in rc.conf to switch from stacking mode to another
     mode when reaching a set number of open windows
  * Use 'NULL' for no modifier key in key.conf
  * Added option in rc.conf to set the initial number of extra
     windows in the master area for each desktop
  * Added a marker for the previous desktop in the switcher
  * The BAR can have a transparency value in rc.conf
  * Transient windows can be cycled through with normal windows
  * Add new function "terminate" for keyboard shortcut to
     close any open windows, exit the window manager and shutdown(1)
     or reboot(2)
  * Add option BAR_SHORT to rc.conf to shorten the bar's 
     length to have room for an external app
  *  Don't manage apps with '_NET_WM_WINDOW_TYPE_DOCK' set
      e.g. trayer
  * Right click an unfocused desktop in the switcher to move the
     focused window to that desktop


###Installation
------------

Need Xlib, then:

    Copy the sample.rc.conf file to $HOME/.config/snapwm/rc.conf and edit it to suit.

    Copy the sample.apps.conf file to $HOME/.config/snapwm/apps.conf and edit it to suit.

    Copy the sample.key.conf file to $HOME/.config/snapwm/key.conf and edit it to suit.

    $ make
    # make install
    $ make clean


###Bugs
----

[ * No bugs for the moment ;) (I mean, no importants bugs ;)]


###Todo
----

  * Maybe xft fonts
  * Maybe Xrandr
