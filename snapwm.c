 /* snapwm.c [ 0.4.0 ]
 *
 *  Started from catwm 31/12/10
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
/* If you have a multimedia keyboard uncomment the following line */
//#include <X11/XF86keysym.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <locale.h>
#include <string.h>

#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union {
    const char** com;
    const int i;
} Arg;

// Structs
typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
} key;

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;

    // The window
    Window win;
};

typedef struct desktop desktop;
struct desktop{
    int master_size;
    int mode, growth, numwins;
    client *head;
    client *current;
    client *transient;
};

typedef struct {
    const char *class;
    int preferredd;
    int followwin;
} Convenience;

typedef struct {
    Window sb_win;
    const char *label;
    int width;
} Barwin;

typedef struct {
    unsigned long color;
    const char *modename;
    GC gc;
} Theme;

// Functions
static void add_window(Window w, int tw);
static void buttonpressed(XEvent *e);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void follow_client_to_desktop(const Arg arg);
static unsigned long getcolor(const char* color);
static void getwindowname();
static void grabkeys();
static void keypress(XEvent *e);
static void kill_client();
static void last_desktop();
static void logger(const char* e);
static void maprequest(XEvent *e);
static void move_down();
static void move_up();
static void next_win();
static void prev_win();
static void propertynotify(XEvent *e);
static void quit();
static void remove_window(Window w, int dr);
static void read_rcfile();
static void resize_master(const Arg arg);
static void resize_stack(const Arg arg);
static void rotate_desktop(const Arg arg);
static void save_desktop(int i);
static void select_desktop(int i);
static void setup();
static void setup_status_bar();
static void set_defaults();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static void status_bar();
static void status_text(const char* sb_text);
static void swap_master();
static void switch_mode(const Arg arg);
static void tile();
static void toggle_bar();
static void unmapnotify(XEvent *e);    // Thunderbird's write window just unmaps...
static void update_bar();
static void update_config();
static void update_current();
static void update_output(int messg);
static void warp_pointer();

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static int attachaside;
static int bdw;             // border width
static int bool_quit;
static int clicktofocus;
static int current_desktop;
static int dowarp;
static int followmouse;
static int growth;
static int master_size;
static int mode;
static int msize;
static int previous_desktop;
static int sb_desks;        // width of the desktop switcher
static int sb_height;
static int sb_width;
static int sh;
static int sw;
static int screen;
static int show_bar;
static int showopen;        // whether the desktop switcher shows number of open windows
static int topbar;
static int top_stack;
static int ufalpha;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask;        /* dynamic key lock mask */
static Window root;
static Window sb_area;
static client *head;
static client *current;
static client *transient;
static char fontbarname[80];
static XFontStruct *fontbar;
static Atom alphaatom, wm_delete_window;
static XWindowAttributes attr;
// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [Expose] = expose,
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [EnterNotify] = enternotify,
    [UnmapNotify] = unmapnotify,
    [ButtonPress] = buttonpressed,
    [DestroyNotify] = destroynotify,
    [PropertyNotify] = propertynotify,
    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest
};

// Desktop array
static desktop desktops[DESKTOPS];
static Barwin sb_bar[DESKTOPS];
static Theme theme[5];
#include "bar.c"
#include "readrc.c"

/* ***************************** Window Management ******************************* */
void add_window(Window w, int tw) {
    client *c,*t;

    if(!(c = (client *)calloc(1,sizeof(client)))) {
        logger("\033[0;31mError calloc!");
        exit(1);
    }

    if(tw == 1) { // For the transient window
        if(transient == NULL) {
            c->prev = NULL;
            c->next = NULL;
            c->win = w;
            transient = c;
        } else {
            t=transient;
            c->prev = NULL;
            c->next = t;
            c->win = w;
            t->prev = c;
            transient = c;
        }
        save_desktop(current_desktop);
        return;
    }

    if(head == NULL) {
        c->next = NULL; c->prev = NULL;
        c->win = w; head = c;
    } else {
        if(attachaside == 0) {
            if(top_stack == 0) {
                if(head->next == NULL) {
                    c->prev = head; c->next = NULL;
                } else {
                    t = head->next;
                    c->prev = t->prev; c->next = t;
                    t->prev = c;
                }
                c->win = w; head->next = c;
            } else {
                for(t=head;t->next;t=t->next); // Start at the last in the stack
                c->next = NULL; c->prev = t;
                c->win = w; t->next = c;
            }
        } else {
            t=head;
            c->prev = NULL;
            c->next = t;
            c->win = w;
            t->prev = c;
            head = c; current = c;
            warp_pointer();
        }
    }

    current = head;
    desktops[current_desktop].numwins += 1;
    if(growth > 0) growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
    else growth = 0;
    save_desktop(current_desktop);
    // for folow mouse and statusbar updates
    if(followmouse == 0 && STATUS_BAR == 0)
        XSelectInput(dis, c->win, EnterWindowMask|PropertyChangeMask);
    else if(followmouse == 0)
        XSelectInput(dis, c->win, EnterWindowMask);
    else if(STATUS_BAR == 0)
        XSelectInput(dis, c->win, PropertyChangeMask);
}

void remove_window(Window w, int dr) {
    client *c;

    if(transient != NULL) {
        for(c=transient;c;c=c->next) {
            if(c->win == w) {
                if(c->prev == NULL && c->next == NULL) transient = NULL;
                else if(c->prev == NULL) {
                    transient = c->next;
                    c->next->prev = NULL;
                }
                else if(c->next == NULL) {
                    c->prev->next = NULL;
                }
                else {
                    c->prev->next = c->next;
                    c->next->prev = c->prev;
                }
                free(c);
                save_desktop(current_desktop);
                update_current();
                return;
            }
        }
    }

    for(c=head;c;c=c->next) {
        if(c->win == w) {
            if(desktops[current_desktop].numwins < 4) growth = 0;
            else growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
            XUnmapWindow(dis, c->win);
            if(c->prev == NULL && c->next == NULL) {
                head = NULL;
                current = NULL;
            } else if(c->prev == NULL) {
                head = c->next;
                c->next->prev = NULL;
                current = c->next;
            } else if(c->next == NULL) {
                c->prev->next = NULL;
                current = c->prev;
            } else {
                c->prev->next = c->next;
                c->next->prev = c->prev;
                current = c->prev;
            }

            if(dr == 0) free(c);
            desktops[current_desktop].numwins -= 1;
            save_desktop(current_desktop);
            tile();
            update_current();
            if(desktops[current_desktop].numwins > 1) warp_pointer();
            if(STATUS_BAR == 0) getwindowname();
            return;
        }
    }
}

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(mode == 1) XUnmapWindow(dis, current->win);
        if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        save_desktop(current_desktop);
        if(mode == 1) tile();
        update_current();
        warp_pointer();
    }
}

void prev_win() {
    client *c;

    if(head->next == NULL) return;
    if(mode == 1) XUnmapWindow(dis, current->win);
    if(current->prev == NULL)
        for(c=head;c->next;c=c->next);
    else
        c = current->prev;

    current = c;
    save_desktop(current_desktop);
    if(mode == 1) tile();
    update_current();
    warp_pointer();
}

void move_down() {
    Window tmp;
    if(current == NULL || current->next == NULL || current->win == head->win || current->prev == NULL)
        return;

    tmp = current->win;
    current->win = current->next->win;
    current->next->win = tmp;
    //keep the moved window activated
    next_win();
    save_desktop(current_desktop);
    tile();
}

void move_up() {
    Window tmp;
    if(current == NULL || current->prev == head || current->win == head->win) {
        return;
    }
    tmp = current->win;
    current->win = current->prev->win;
    current->prev->win = tmp;
    prev_win();
    save_desktop(current_desktop);
    tile();
}

void swap_master() {
    Window tmp;

    if(head->next != NULL && current != NULL && mode != 1) {
        if(current == head) {
            tmp = head->next->win;
            head->next->win = head->win;
            head->win = tmp;
        } else {
            tmp = head->win;
            head->win = current->win;
            current->win = tmp;
            current = head;
        }
        save_desktop(current_desktop);
        tile();
        update_current();
    }
}

/* **************************** Desktop Management ************************************* */
void change_desktop(const Arg arg) {
    client *c;

    if(arg.i == current_desktop)
        return;

    // Save current "properties"
    save_desktop(current_desktop);
    previous_desktop = current_desktop;

    // Unmap all window
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XUnmapWindow(dis,c->win);

    if(head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis,c->win);

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XMapWindow(dis,c->win);

    if(head != NULL) {
        if(mode != 1)
            for(c=head;c;c=c->next)
                XMapWindow(dis,c->win);
    }

    tile();
    update_current();
    warp_pointer();
    if(STATUS_BAR == 0) update_bar();
}

void last_desktop() {
    Arg a = {.i = previous_desktop};
    change_desktop(a);
}

void rotate_desktop(const Arg arg) {
    Arg a = {.i = (current_desktop + TABLENGTH(desktops) + arg.i) % TABLENGTH(desktops)};
     change_desktop(a);
}

void follow_client_to_desktop(const Arg arg) {
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win, 0);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    XUnmapWindow(dis,tmp->win);
    remove_window(tmp->win, 0);
    save_desktop(tmp2);
    tile();
    update_current();
    change_desktop(arg);
}

void client_to_desktop(const Arg arg) {
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win, 0);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    XUnmapWindow(dis,tmp->win);
    remove_window(tmp->win, 0);
    save_desktop(tmp2);
    tile();
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].head = head;
    desktops[i].current = current;
    desktops[i].transient = transient;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    head = desktops[i].head;
    current = desktops[i].current;
    transient = desktops[i].transient;
    current_desktop = i;
}

void tile() {
    client *c;
    int n = 0;
    int x = 0;
    int y = 0;

    // For a top bar
    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4;
    else y = 0;

    // If only one window
    if(head != NULL && head->next == NULL) {
        if(mode == 1) XMapWindow(dis, current->win);
        XMoveResizeWindow(dis,head->win,0,y,sw+bdw,sh+bdw);
    }

    else if(head != NULL) {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,master_size - bdw,sh - bdw);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                XMoveResizeWindow(dis,head->next->win,master_size + bdw,y,sw-master_size-(2*bdw),(sh/n)+growth - bdw);
                y += (sh/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,master_size + bdw,y,sw-master_size-(2*bdw),(sh/n)-(growth/(n-1)) - bdw);
                    y += (sh/n)-(growth/(n-1));
                }
                break;
            case 1: /* Fullscreen */
                XMoveResizeWindow(dis,current->win,0,y,sw+2*bdw,sh+2*bdw);
                XMapWindow(dis, current->win);
                break;
            case 2: /* Horizontal */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,sw-bdw,master_size - bdw);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                XMoveResizeWindow(dis,head->next->win,0,y+master_size + bdw,(sw/n)+growth-bdw,sh-master_size-(2*bdw));
                x = (sw/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,x,y+master_size + bdw,(sw/n)-(growth/(n-1)) - bdw,sh-master_size-(2*bdw));
                    x += (sw/n)-(growth/(n-1));
                }
                break;
            case 3: { // Grid
                int xpos = 0;
                int wdt = 0;
                int ht = 0;

                for(c=head;c;c=c->next) ++x;
                for(c=head;c;c=c->next) {
                    ++n;
                    if(x >= 7) {
                        wdt = (sw/3) - BORDER_WIDTH;
                        ht  = (sh/3) - BORDER_WIDTH;
                        if((n == 1) || (n == 4) || (n == 7)) xpos = 0;
                        if((n == 2) || (n == 5) || (n == 8)) xpos = (sw/3) + BORDER_WIDTH;
                        if((n == 3) || (n == 6) || (n == 9)) xpos = (2*(sw/3)) + BORDER_WIDTH;
                        if((n == 4) || (n == 7)) y += (sh/3) + BORDER_WIDTH;
                        if((n == x) && (n == 7)) wdt = sw - BORDER_WIDTH;
                        if((n == x) && (n == 8)) wdt = 2*sw/3 - BORDER_WIDTH;
                    } else if(x >= 5) {
                        wdt = (sw/3) - BORDER_WIDTH;
                        ht  = (sh/2) - BORDER_WIDTH;
                        if((n == 1) || (n == 4)) xpos = 0;
                        if((n == 2) || (n == 5)) xpos = (sw/3) + BORDER_WIDTH;
                        if((n == 3) || (n == 6)) xpos = (2*(sw/3)) + BORDER_WIDTH;
                        if(n == 4) y += (sh/2); // + BORDER_WIDTH;
                        if((n == x) && (n == 5)) wdt = 2*sw/3 - BORDER_WIDTH;
                    } else {
                        if(x > 2) {
                            if((n == 1) || (n == 2)) ht = (sh/2) + growth - BORDER_WIDTH;
                            if(n >= 3) ht = (sh/2) - growth - 2*BORDER_WIDTH;
                        } else ht = sh - BORDER_WIDTH;
                        if((n == 1) || (n == 3)) {
                            xpos = 0;
                            wdt = master_size - BORDER_WIDTH;
                        }
                        if((n == 2) || (n == 4)) {
                            xpos = master_size+BORDER_WIDTH;
                            wdt = (sw - master_size) - 2*BORDER_WIDTH;
                        }
                        if(n == 3) y += (sh/2) + growth + BORDER_WIDTH;
                        if((n == x) && (n == 3)) wdt = sw - BORDER_WIDTH;
                    }
                    XMoveResizeWindow(dis,c->win,xpos,y,wdt,ht);
                }
                break;
            }
            default:
                break;
        }
    }
}

void update_current() {
    client *c;
    unsigned long opacity = (ufalpha/100.00) * 0xffffffff;

    for(c=head;c;c=c->next) {
        if((head->next == NULL) || (mode == 1))
            XSetWindowBorderWidth(dis,c->win,0);
        else
            XSetWindowBorderWidth(dis,c->win,bdw);

        if(current == c && transient == NULL) {
            // "Enable" current window
            if(ufalpha < 100) XDeleteProperty(dis, c->win, alphaatom);
            XSetWindowBorder(dis,c->win,theme[0].color);
            XSetInputFocus(dis,c->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,c->win);
            if(clicktofocus == 0) XUngrabButton(dis, AnyButton, AnyModifier, c->win);
        }
        else {
            if(ufalpha < 100) XChangeProperty(dis, c->win, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &opacity, 1l);
            XSetWindowBorder(dis,c->win,theme[1].color);
            if(clicktofocus == 0) XGrabButton(dis, AnyButton, AnyModifier, c->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
    }
    if(transient != NULL) {
        XSetInputFocus(dis,transient->win,RevertToParent,CurrentTime);
        XRaiseWindow(dis,transient->win);
    }
    if(STATUS_BAR == 0 && show_bar == 0) {
        if(head != NULL)
            getwindowname();
        else
            getwindowname();
    }
    XSync(dis, False);
}

void switch_mode(const Arg arg) {
    client *c;

    if(mode == arg.i) return;
    growth = 0;
    if(mode == 1 && head != NULL) {
        XUnmapWindow(dis, current->win);
        for(c=head;c;c=c->next)
            XMapWindow(dis, c->win);
    }

    mode = arg.i;
    if(mode == 0 || mode == 3) master_size = (sw*msize)/100;
    if(mode == 1 && head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis, c->win);

    if(mode == 2) master_size = (sh*msize)/100;
    save_desktop(current_desktop);
    tile();
    update_current();
    if(STATUS_BAR == 0 && show_bar == 0) update_bar();
}

void resize_master(const Arg arg) {
    if(arg.i > 0) {
        if((mode != 2 && sw-master_size > 70) || (mode == 2 && sh-master_size > 70))
            master_size += arg.i;
    } else if(master_size > 70) master_size += arg.i;
    tile();
}

void resize_stack(const Arg arg) {
    if(desktops[current_desktop].numwins > 2) {
        int n = desktops[current_desktop].numwins-1;
        if(arg.i >0) {
            if((mode != 2 && sh-(growth+sh/n) > (n-1)*70) || (mode == 2 && sw-(growth+sw/n) > (n-1)*70))
                growth += arg.i;
        } else {
            if((mode != 2 && (sh/n+growth) > 70) || (mode == 2 && (sw/n+growth) > 70))
                growth += arg.i;
        }
        tile();
    }
}

/* ********************** Keyboard Management ********************** */
void grabkeys() {
    int i, j;
    XModifierKeymap *modmap;
    KeyCode code;

    XUngrabKey(dis, AnyKey, AnyModifier, root);
    // numlock workaround
    numlockmask = 0;
    modmap = XGetModifierMapping(dis);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if(modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dis, XK_Num_Lock))
                numlockmask = (1 << i);
        }
    }
    XFreeModifiermap(modmap);

    // For each shortcuts
    for(i=0;i<TABLENGTH(keys);++i) {
        code = XKeysymToKeycode(dis,keys[i].keysym);
        XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | LockMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask | LockMask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void keypress(XEvent *e) {
    static unsigned int len = sizeof keys / sizeof keys[0];
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XKeycodeToKeysym(dis, (KeyCode)ev->keycode, 0);
    for(i = 0; i < len; i++) {
        if(keysym == keys[i].keysym && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)) {
            if(keys[i].function)
                keys[i].function(keys[i].arg);
        }
    }
}

void warp_pointer() {
    // Move cursor to the center of the current window
    if(FOLLOW_MOUSE == 0 && dowarp < 1 && head != NULL) {
        XGetWindowAttributes(dis, current->win, &attr);
        XWarpPointer(dis, None, current->win, 0, 0, 0, 0, attr.width/2, attr.height/2);
    }
}

void configurenotify(XEvent *e) {
    // Do nothing for the moment
}

/* ********************** Signal Management ************************** */
void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    int y = 0;

    wc.x = ev->x;
    if(STATUS_BAR == 0 && topbar == 0) y = sb_height+4;
    wc.y = ev->y + y;
    if(ev->width < sw-bdw) wc.width = ev->width;
    else wc.width = sw+bdw;
    if(ev->height < sh-bdw) wc.height = ev->height;
    else wc.height = sh+bdw;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
    if(STATUS_BAR == 0) update_bar();
    XSync(dis, False);
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    XGetWindowAttributes(dis, ev->window, &attr);
    if(attr.override_redirect == True) return;

    // For fullscreen mplayer (and maybe some other program)
    client *c;

    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            XMoveResizeWindow(dis,c->win,0,0,sw,sh);
            return;
        }

    Window trans = None;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
        add_window(ev->window, 1); XMapWindow(dis, ev->window);
        XSetWindowBorderWidth(dis,ev->window,bdw);
        XSetWindowBorder(dis,ev->window,theme[0].color);
        update_current();
        return;
    }

    XClassHint ch = {0};
    static unsigned int len = sizeof convenience / sizeof convenience[0];
    int i=0, j=0;
    int tmp = current_desktop;
    if(XGetClassHint(dis, ev->window, &ch))
        for(i=0;i<len;i++)
            if(strcmp(ch.res_class, convenience[i].class) == 0) {
                save_desktop(tmp);
                select_desktop(convenience[i].preferredd-1);
                for(c=head;c;c=c->next)
                    if(ev->window == c->win)
                        ++j;
                if(j < 1) add_window(ev->window, 0);
                if(tmp == convenience[i].preferredd-1) {
                    XMapWindow(dis, ev->window);
                    tile();
                    update_current();
                } else select_desktop(tmp);
                if(convenience[i].followwin == 0) {
                    Arg a = {.i = convenience[i].preferredd-1};
                    change_desktop(a);
                }
                if(ch.res_class) XFree(ch.res_class);
                if(ch.res_name) XFree(ch.res_name);
                if(STATUS_BAR == 0) update_bar();
                return;
            }

    add_window(ev->window, 0);
    tile();
    if(mode != 1) XMapWindow(dis,ev->window);
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void destroynotify(XEvent *e) {
    int i = 0, tmp = current_desktop;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if(transient != NULL) {
        for(c=transient;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0);
                return;
            }
    }
    save_desktop(tmp);
    for(i=0;i<TABLENGTH(desktops);++i) {
        select_desktop(i);
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0);
                select_desktop(tmp);
                if(STATUS_BAR == 0) {
                    update_bar();
                    getwindowname();
                }
                return;
            }
    }
    select_desktop(tmp);
}

void enternotify(XEvent *e) {
    client *c; int i;
    XCrossingEvent *ev = &e->xcrossing;

    if(followmouse == 0) {
        if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
            return;
        if(transient != NULL) return;
        for(i=0;i<DESKTOPS;i++)
            if(sb_bar[i].sb_win == ev->window) dowarp = 1;
        if(ev->window == sb_area) dowarp = 1;
        if(ev->window == root) dowarp = 0;
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                current = c;
                update_current();
                dowarp = 0;
                return;
       }
   }
}

void buttonpressed(XEvent *e) {
    client *c;
    XButtonPressedEvent *ev = &e->xbutton;

    if(STATUS_BAR == 0) {
        int i;
        for(i=0;i<DESKTOPS;i++)
            if(i != current_desktop && sb_bar[i].sb_win == ev->window) {
                Arg a = {.i = i};
                change_desktop(a);
            } else {
                if(i == current_desktop && sb_bar[i].sb_win == ev->window)
                    next_win();
            }
        return;
    }

    // change focus with LMB
    if(clicktofocus == 0 && ev->window != current->win && ev->button == Button1)
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                current = c;
                update_current();
                return;
            }
}

void propertynotify(XEvent *e) {
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete) {
        //logger("prop notify delete");
        return;
    } else
        if(STATUS_BAR == 0 && ev->window == root && ev->atom == XA_WM_NAME) update_output(0);
    else
        if(STATUS_BAR == 0) getwindowname();
}

void unmapnotify(XEvent *e) { // for thunderbird's write window and maybe others
    XUnmapEvent *ev = &e->xunmap;
    int i = 0, tmp = current_desktop;
    client *c;

    if(ev->send_event == 1) {
        save_desktop(tmp);
        for(i=0;i<TABLENGTH(desktops);++i) {
            select_desktop(i);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    remove_window(ev->window, 1);
                    select_desktop(tmp);
                    if(STATUS_BAR == 0) update_bar();
                    return;
                }
        }
        select_desktop(tmp);
    }
}

void expose(XEvent *e) {
    XExposeEvent *ev = &e->xexpose;

    if(STATUS_BAR == 0 && ev->count == 0 && ev->window == sb_area) {
        update_output(1);
        getwindowname();
        update_bar();
    }
}

void kill_client() {
    if(head == NULL) return;
    Atom *protocols;
    int n, i, can_delete = 0;
    XEvent ke;

    if (XGetWMProtocols(dis, current->win, &protocols, &n) != 0)
        for (i=0;i<n;i++)
            if (protocols[i] == wm_delete_window) can_delete = 1;

    //XFree(protocols);
    if(can_delete == 1) {
        ke.type = ClientMessage;
        ke.xclient.window = current->win;
        ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
        ke.xclient.format = 32;
        ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
        ke.xclient.data.l[1] = CurrentTime;
        XSendEvent(dis, current->win, False, NoEventMask, &ke);
    } else XKillClient(dis, current->win);
    remove_window(current->win, 0);
}

void quit() {
    int i;
    client *c;
    logger("\033[0;34mYou Quit : Thanks for using!");
    for(i=0;i<DESKTOPS;++i) {
        select_desktop(i);
        for(c=head;c;c=c->next)
            kill_client();
    }
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    for(i=0;i<5;i++)
        XFreeGC(dis, theme[i].gc);
    XDestroySubwindows(dis, root);
    XSync(dis, False);
    //sleep(1);
    bool_quit = 1;
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c)) {
        logger("\033[0;31mError parsing color!");
        return 1;
    }
    return c.pixel;
}

void logger(const char* e) {
    fprintf(stderr,"\n\033[0;34m:: snapwm : %s \033[0;m\n", e);
}

void setup() {
    // Install a signal
    sigchld(0);

    // Screen and root window
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);

    msize = MASTER_SIZE;
    ufalpha = UF_ALPHA;
    bdw = BORDER_WIDTH;
    attachaside = ATTACH_ASIDE;
    top_stack = TOP_STACK;
    followmouse = FOLLOW_MOUSE;
    clicktofocus = CLICK_TO_FOCUS;
    topbar = TOP_BAR;
    showopen = SHOW_NUM_OPEN;

    // Read in RCFILE
    if(!setlocale(LC_CTYPE, "")) logger("\033[0;31mLocale failed");
    read_rcfile();
    if(STATUS_BAR == 0) {
        show_bar = STATUS_BAR;
        setup_status_bar();
        status_bar();
        update_output(1);
        if(SHOW_BAR > 0) toggle_bar();
    } else set_defaults();

    // Shortcuts
    grabkeys();

    // Default stack
    mode = DEFAULT_MODE;

    // For exiting
    bool_quit = 0;

    // List of client
    head = NULL;
    current = NULL;
    transient = NULL;

    // Master size
    if(mode == 2) master_size = (sh*msize)/100;
    else master_size = (sw*msize)/100;

    // Set up all desktop
    int i;
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].growth = 0;
        desktops[i].numwins = 0;
        desktops[i].head = head;
        desktops[i].current = current;
        desktops[i].transient = transient;
    }

    // Select first dekstop by default
    const Arg arg = {.i = 0};
    current_desktop = arg.i;
    change_desktop(arg);
    alphaatom = XInternAtom(dis, "_NET_WM_WINDOW_OPACITY", False);
    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", False);
    update_current();

    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask|PropertyChangeMask|EnterWindowMask);
    XSetErrorHandler(xerror);
    logger("\033[0;32mWe're up and running!");
}

void sigchld(int unused) {
    if(signal(SIGCHLD, sigchld) == SIG_ERR) {
        logger("\033[0;31mCan't install SIGCHLD handler");
        exit(1);
        }
    while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const Arg arg) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis) close(ConnectionNumber(dis));
            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }
        exit(0);
    }
}

/* There's no way to check accesses to destroyed windows, thus those cases are ignored (especially on UnmapNotify's).  Other types of errors call Xlibs default error handler, which may call exit.  */
int xerror(Display *dis, XErrorEvent *ee) {
    if(ee->error_code == BadWindow
    || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
    || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
    || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
    || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
    || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
    || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
    || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    logger("\033[0;31mBad Window Error!");
    return xerrorxlib(dis, ee); /* may call exit */
}

void start() {
    XEvent ev;

    while(!bool_quit && !XNextEvent(dis,&ev)) {
        if(events[ev.type])
            events[ev.type](&ev);
    }
}


int main(int argc, char **argv) {
    // Open display
    if(!(dis = XOpenDisplay(NULL))) {
        logger("\033[0;31mCannot open display!");
        exit(1);
    }

    // Setup env
    setup();

    // Start wm
    start();

    // Close display
    XCloseDisplay(dis);

    exit(0);
}
