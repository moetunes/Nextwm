 /* config.h for snapwm.c [ 0.6.4 ]
 *
 *  RC FILE set to $HOME/.config/snapwm/rc.conf
 */

#ifndef CONFIG_H
#define CONFIG_H

#define STATUS_BAR  0  /* 1=Don't 0=Make the bar */
#define FONTS_ERROR 1  /* 1=Don't 0=Print errors about missing fonts */

// The settings from here to defaultfont[] are in the rc file, which has precedence,
// so if the rc file all has these 15 options they don't have to be changed.

#define UF_ALPHA        75 /* Percentage transparency for unfocused windows - 100 to turn off */
#define MASTER_SIZE     55
#define BORDER_WIDTH    2
#define RESIZEMOVEKEY   MOD1
#define ATTACH_ASIDE    1  /* 1=Don't 0=Place new window in the stack */
#define TOP_STACK       0  /* 1=Don't 0=Place new window at the top of the stack when using ATTACH_ASIDE*/
#define DEFAULT_MODE    4  /* 0=Vertical, 1=Fullscreen 2=Horizontal 3=Grid 4=Stacking*/
#define FOLLOW_MOUSE    0  /* 1=Don't 0=Focus the window the mouse just entered */
#define CLICK_TO_FOCUS  1  /* 1=Don't 0=Focus an unfocused window when clicked */
#define TOP_BAR         0  /* 1=Don't 0=Have the bar at the top instead of the bottom */
#define SHOW_BAR        0  /* 1=Don't 0=Have the bar shown at startup */
#define SHOW_NUM_OPEN   1  /* 1=Dont' 0=Show the number of open windows in the switcher */
#define WINDOW_NAME_LENGTH 35 /* Character length for the current window's name in the bar */

static const char *defaultdesktopnames[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", };
static const char *defaultbarcolor[] = { "#ff0000", "#00ff00", "#0000ff", "#000000", };
static const char *defaulttextcolor[] = { "#000000", "#ffffff", "#ffffff", "#ffff00", "#ff00ff", "#f0f0f0", "#0f0f0f", "#000000", "#000000", "#000000", };
static const char *defaultwincolor[] = { "#ff0000", "#00ff00", };
static const char *defaultmodename[] = { "[0]", "[1]", "[2]", "[3]", "[4]", };
static const char defaultfontlist[] = "-*-terminus-medium-r-*-*-12-*-*-*-*-*-*-*";

#endif

