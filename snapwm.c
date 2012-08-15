 /* snapwm.c [ 0.6.1 ]
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

#define _BSD_SOURCE
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
//#include <X11/keysym.h>
/* If you have a multimedia keyboard uncomment the following line */
//#include <X11/XF86keysym.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <locale.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union {
    char *com[256];
    int i;
} Arg;

// Structs
typedef struct {
    unsigned int mod;
    char * keysym;
    void (*myfunction)(const Arg arg);
    Arg arg;
} key;

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;

    // The window
    Window win;
    unsigned int x, y, width, height, order;
};

typedef struct desktop desktop;
struct desktop{
    unsigned int master_size;
    unsigned int mode, growth, numwins, nmaster;
    client *head;
    client *current;
    client *transient;
};

typedef struct {
    const char *class;
    unsigned int preferredd;
    unsigned int followwin;
} Convenience;

typedef struct {
    const char *class;
    int x, y, width, height;
} Positional;

typedef struct {
    XFontStruct *font;
    XFontSet fontset;
    int height;
    int width;
    unsigned int fh;            /* Y coordinate to draw characters */
    int ascent;
    int descent;
} Iammanyfonts;

typedef struct {
    Window sb_win;
    char *label;
    int width;
    int labelwidth;
} Barwin;

typedef struct {
    unsigned long barcolor, wincolor, textcolor;
    char *modename;
    GC gc;
} Theme;

typedef struct {
    char *name;
    char *list[100];
} Commands;

// Functions
static void add_window(Window w, int tw);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void draw_desk(Window win, int barcolor, int gc, int x, char *string, int len);
static void draw_text(Window win, int gc, int x, char *string, int len);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void follow_client_to_desktop(const Arg arg);
static unsigned long getcolor(const char* color);
static void get_font();
static void getwindowname();
static void grabkeys();
static void keypress(XEvent *e);
static void kill_client();
static void kill_client_now(Window w);
static void last_desktop();
static void leavenotify(XEvent *e);
static void logger(const char* e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void more_master(const Arg arg);
static void move_down(const Arg arg);
static void move_up(const Arg arg);
static void move_right(const Arg arg);
static void move_left(const Arg arg);
static void next_win();
static void prev_win();
static void propertynotify(XEvent *e);
static void quit();
static void remove_window(Window w, unsigned int dr, unsigned int tw);
static void read_apps_file();
static void read_keys_file();
static void read_rcfile();
static void resize_master(const Arg arg);
static void resize_stack(const Arg arg);
static void rotate_desktop(const Arg arg);
static void rotate_mode(const Arg arg);
static void save_desktop(int i);
static void select_desktop(int i);
static void setup();
static void setup_status_bar();
static void set_defaults();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static void status_bar();
static void status_text(char* sb_text);
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
static unsigned int wc_size(char *string);

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static unsigned int attachaside, bdw, bool_quit, clicktofocus, current_desktop, doresize, dowarp;
static unsigned int followmouse, mode, msize, previous_desktop, DESKTOPS;
static int growth, sh, sw, master_size, nmaster;
static unsigned int sb_desks;        // width of the desktop switcher
static unsigned int sb_height, sb_width, screen, show_bar;
static unsigned int showopen;        // whether the desktop switcher shows number of open windows
static unsigned int topbar, top_stack, windownamelength, keycount, cmdcount, dtcount, pcount, LA_WINDOWNAME;
static int ufalpha;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask;        /* dynamic key lock mask */
static Window root;
static Window sb_area;
static client *head, *current, *transient;
static char font_list[256];
static char RC_FILE[100], KEY_FILE[100], APPS_FILE[100];
static Atom alphaatom, wm_delete_window, protos, *protocols;
static XWindowAttributes attr;
static XButtonEvent starter;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [Expose] = expose,
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [EnterNotify] = enternotify,
    [LeaveNotify] = leavenotify,
    [UnmapNotify] = unmapnotify,
    [ButtonPress] = buttonpress,
    [MotionNotify] = motionnotify,
    [ButtonRelease] = buttonrelease,
    [DestroyNotify] = destroynotify,
    [PropertyNotify] = propertynotify,
    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest
};

// Desktop array
static desktop desktops[10];
static Barwin sb_bar[10];
static Theme theme[8];
static Iammanyfonts font;
static key keys[80];
static Commands cmds[50];
static Convenience convenience[20];
static Positional positional[20];
#include "bar.c"
#include "readrc.c"
#include "readkeysapps.c"

/* ***************************** Window Management ******************************* */
void add_window(Window w, int tw) {
    client *c,*t, *dummy;

    if(!(c = (client *)calloc(1,sizeof(client)))) {
        logger("\033[0;31mError calloc!");
        exit(1);
    }

    if(tw == 0) {
        XClassHint chh = {0};
        unsigned int i, j=0;
        if(XGetClassHint(dis, w, &chh)) {
            for(i=0;i<pcount;i++)
                if(strcmp(chh.res_class, positional[i].class) == 0) {
                    XMoveResizeWindow(dis,w,positional[i].x,positional[i].y,positional[i].width,positional[i].height);
                    j++;
                }
            if(chh.res_class) XFree(chh.res_class);
            if(chh.res_name) XFree(chh.res_name);
        } 
        if(j < 1) {
            XGetWindowAttributes(dis, w, &attr);
            XMoveWindow(dis, w,sw/2-(attr.width/2),(sh+sb_height+4)/2-(attr.height/2));
        }
        XGetWindowAttributes(dis, w, &attr);
        c->x = attr.x;
        if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0 && attr.y < sb_height+4) c->y = sb_height+4;
        else c->y = attr.y;
        c->width = attr.width;
        c->height = attr.height;
    }

    c->win = w; c->order = 0;
    dummy = (tw == 1) ? transient : head;
    for(t=dummy;t;t=t->next)
        t->order++;

    if(dummy == NULL) {
        c->next = NULL; c->prev = NULL;
        dummy = c;
    } else {
        if(ATTACH_ASIDE == 0) {
            if(TOP_STACK == 0) {
                c->next = dummy->next; c->prev = dummy;
                dummy->next = c;
            } else {
                for(t=dummy;t->next;t=t->next); // Start at the last in the stack
                t->next = c; c->next = NULL;
                c->prev = t;
            }
        } else {
            c->prev = NULL; c->next = dummy;
            c->next->prev = c;
            dummy = c;
        }
    }

    if(tw == 1) {
        transient = dummy;
        save_desktop(current_desktop);
        return;
    } else head = dummy;
    current = c;
    desktops[current_desktop].numwins += 1;
    if(growth > 0) growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
    else growth = 0;
    save_desktop(current_desktop);
    // for folow mouse and statusbar updates
    if(followmouse == 0 && STATUS_BAR == 0)
        XSelectInput(dis, c->win, PointerMotionMask|PropertyChangeMask);
    else if(followmouse == 0)
        XSelectInput(dis, c->win, PointerMotionMask);
    else if(STATUS_BAR == 0)
        XSelectInput(dis, c->win, PropertyChangeMask);
}

void remove_window(Window w, unsigned int dr, unsigned int tw) {
    client *c, *t, *dummy;

    dummy = (tw == 1) ? transient : head;
    for(c=dummy;c;c=c->next) {
        if(c->win == w) {
            if(c->prev == NULL && c->next == NULL) {
                dummy = NULL;
            } else if(c->prev == NULL) {
                dummy = c->next;
                c->next->prev = NULL;
            } else if(c->next == NULL) {
                c->prev->next = NULL;
            } else {
                c->prev->next = c->next;
                c->next->prev = c->prev;
            } break;
        }
    }
    if(tw == 1) {
        transient = dummy;
        free(c);
        save_desktop(current_desktop);
        update_current();
        return;
    } else {
        head = dummy;
        XUngrabButton(dis, AnyButton, AnyModifier, c->win);
        XUnmapWindow(dis, c->win);
        desktops[current_desktop].numwins -= 1;
        if(head != NULL) {
            for(t=head;t;t=t->next) {
                if(t->order > c->order) t->order--;
                if(t->order == 0) current = t;
            }
        } else current = NULL;
        if(dr == 0) free(c);
        if(desktops[current_desktop].numwins < 3) growth = 0;
        else growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
        if(nmaster > 0 && nmaster == (desktops[current_desktop].numwins-1)) nmaster -= 1;
        save_desktop(current_desktop);
        if(mode != 4) tile();
        warp_pointer();
        update_current();
        if(STATUS_BAR == 0)
            update_bar();
        return;
    }
}

void next_win() {
    client *c;

    if(desktops[current_desktop].numwins < 2) return;
    //if(head->next == NULL) return;
    if(mode == 1) XUnmapWindow(dis, current->win);
    c = (current->next == NULL) ? head : current->next;

    current = c;
    save_desktop(current_desktop);
    if(mode == 1) tile();
    update_current();
    warp_pointer();
}

void prev_win() {
    client *c;

    if(desktops[current_desktop].numwins < 2) return;
    //if(head->next == NULL) return;
    if(mode == 1) XUnmapWindow(dis, current->win);
    if(current->prev == NULL) for(c=head;c->next;c=c->next);
    else c = current->prev;

    current = c;
    save_desktop(current_desktop);
    if(mode == 1) tile();
    update_current();
    warp_pointer();
}

void move_down(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->y += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
        return;
    }
    Window tmp;
    if(current == NULL || current->next == NULL || current->win == head->win || current->prev == NULL)
        return;

    tmp = current->win;
    current->win = current->next->win;
    current->next->win = tmp;
    //keep the moved window activated
    //next_win();
    update_current();
    save_desktop(current_desktop);
    tile();
}

void move_up(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->y += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
        return;
    }
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

void move_left(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->x += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    }
}

void move_right(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->x += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    }
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
        warp_pointer();
        update_current();
    }
}

/* **************************** Desktop Management ************************************* */

void change_desktop(const Arg arg) {
    if(arg.i >= DESKTOPS) return;
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
    if(head != NULL) {
        if(mode != 1) {
            for(c=head;c;c=c->next)
                XMapWindow(dis,c->win);
        }
    }

    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XMapWindow(dis,c->win);

    tile();
    warp_pointer();
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void last_desktop() {
    Arg a = {.i = previous_desktop};
    change_desktop(a);
}

void rotate_desktop(const Arg arg) {
    Arg a = {.i = (current_desktop + DESKTOPS + arg.i) % DESKTOPS};
     change_desktop(a);
}

void rotate_mode(const Arg arg) {
    Arg a = {.i = (mode + 5 + arg.i) % 5};
     switch_mode(a);
}

void follow_client_to_desktop(const Arg arg) {
    if(arg.i >= DESKTOPS) return;
    client_to_desktop(arg);
    change_desktop(arg);
}

void client_to_desktop(const Arg arg) {
    if(arg.i == current_desktop || current == NULL ||arg.i >= DESKTOPS) {
        printf("CLIENT_TO_DEKTOP FAIL !!!\n");
        return;
    }
    client *tmp = current;
    unsigned int tmp2 = current_desktop;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win, 0);
    save_desktop(arg.i);

    select_desktop(tmp2);
    // Remove client from current desktop
    remove_window(current->win, 0, 0);

    if(STATUS_BAR == 0) update_bar();
    printf("C_TO_D - REMOVED CLIENT\n");
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].nmaster = nmaster;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].head = head;
    desktops[i].current = current;
    desktops[i].transient = transient;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    nmaster = desktops[i].nmaster;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    head = desktops[i].head;
    current = desktops[i].current;
    transient = desktops[i].transient;
    current_desktop = i;
}

void more_master (const Arg arg) {
    if(arg.i > 0) {
        if((desktops[current_desktop].numwins < 3) || (nmaster == (desktops[current_desktop].numwins-2))) return;
        nmaster += 1;
    } else {
        if(nmaster == 0) return;
        nmaster -= 1;
    }
    save_desktop(current_desktop);
    tile();
    //update_current();
}

void tile() {
    if(head == NULL) return;
    client *c, *d=NULL;
    unsigned int x = 0, xpos = 0, ypos=0, wdt = 0, msw, ssw, ncols = 2, nrows = 1;
    int ht = 0, y = 0, n = 0;

    // For a top bar
    y = (STATUS_BAR == 0 && topbar == 0 && show_bar == 0) ? sb_height+4 : 0; ypos = y;

    // If only one window
    if(mode != 4 && head != NULL && head->next == NULL) {
        if(mode == 1) XMapWindow(dis, current->win);
        XMoveResizeWindow(dis,head->win,0,y,sw+bdw,sh+bdw);
    } else {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
            	if(nmaster < 1)
                    XMoveResizeWindow(dis,head->win,0,y,master_size - bdw,sh - bdw);
                else {
                    for(d=head;d;d=d->next) {
                        XMoveResizeWindow(dis,d->win,0,ypos,master_size - bdw,sh/(nmaster+1) - bdw);
                        if(x == nmaster) break;
                        ypos += sh/(nmaster+1); x++;
                    }
                }

                // Stack
                if(d == NULL) d = head;
                n = desktops[current_desktop].numwins - (nmaster+1);
                XMoveResizeWindow(dis,d->next->win,master_size + bdw,y,sw-master_size-(2*bdw),(sh/n)+growth - bdw);
                y += (sh/n)+growth;
                for(c=d->next->next;c;c=c->next) {
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
            	if(nmaster < 1)
                    XMoveResizeWindow(dis,head->win,xpos,ypos,sw-bdw,master_size-bdw);
                else {
                    for(d=head;d;d=d->next) {
                        XMoveResizeWindow(dis,d->win,xpos,ypos,sw/(nmaster+1)-bdw,master_size-bdw);
                        if(x == nmaster) break;
                        xpos += sw/(nmaster+1); x++;
                    }
                }

                // Stack
                if(d == NULL) d = head;
                n = desktops[current_desktop].numwins - (nmaster+1);
                XMoveResizeWindow(dis,d->next->win,0,y+master_size + bdw,(sw/n)+growth-bdw,sh-master_size-(2*bdw));
                msw = (sw/n)+growth;
                for(c=d->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,msw,y+master_size + bdw,(sw/n)-(growth/(n-1)) - bdw,sh-master_size-(2*bdw));
                    msw += (sw/n)-(growth/(n-1));
                }
                break;
            case 3: { // Grid
                x = desktops[current_desktop].numwins;
                for(xpos=0;xpos<=x;xpos++) {
                    if(xpos == 3 || xpos == 7 || xpos == 10 || xpos == 17) nrows++;
                    if(xpos == 5 || xpos == 13 || xpos == 21) ncols++;
                }
                msw = (ncols > 2) ? ((master_size*2)/ncols) : master_size;
                ssw = (sw - msw)/(ncols-1); ht = sh/nrows;
                xpos = msw+(ssw*(ncols-2)); ypos = y+((nrows-1)*ht);
                for(c=head;c->next;c=c->next);
                for(d=c;d;d=d->prev) {
                    x--;
                    if(n == nrows) {
                        xpos -= (xpos == msw) ? msw : ssw;
                        ypos = y+((nrows-1)*ht);
                        n = 0;
                    }
                    if(x == 0) {
                        ht = (ypos-y+ht);
                        ypos = y;
                    }
                    if(x == 2 && xpos == msw && ypos != y) {
                        ht -= growth;
                        ypos += growth;
                    }
                    if(x == 1) {
                        ht += growth;
                        ypos -= growth;
                    }
                    wdt = (xpos > 0) ? ssw : msw;
                    XMoveResizeWindow(dis,d->win,xpos,ypos,wdt-bdw,ht-bdw);
                    ht = sh/nrows;
                    ypos -= ht; n++;
                }
                break;
            case 4: // Stacking
                for(c=head;c;c=c->next)
                    XMoveResizeWindow(dis,c->win,c->x,c->y,c->width,c->height);
                break;
            }
            default:
                break;
        }
    }
}

void update_current() {
    client *c; unsigned int border;
    unsigned long opacity = (ufalpha/100.00) * 0xffffffff;

    if(head == NULL) return;
    border = ((head->next == NULL) || (mode == 1)) ? 0 : bdw;
    for(c=head;c;c=c->next) {
        XSetWindowBorderWidth(dis,c->win,border);
        if(c != current || transient != NULL) {
            if(c->order < current->order) c->order++;
            if(ufalpha < 100) XChangeProperty(dis, c->win, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &opacity, 1l);
            XSetWindowBorder(dis,c->win,theme[1].wincolor);
            if(clicktofocus == 0)
                XGrabButton(dis, AnyButton, AnyModifier, c->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
        else {
            // "Enable" current window
            if(ufalpha < 100) XDeleteProperty(dis, c->win, alphaatom);
            XSetWindowBorder(dis,c->win,theme[0].wincolor);
            XSetInputFocus(dis,c->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,c->win);
            if(clicktofocus == 0)
                XUngrabButton(dis, AnyButton, AnyModifier, c->win);
        }
    }
    current->order = 0;
    if(transient != NULL) {
        XSetInputFocus(dis,transient->win,RevertToParent,CurrentTime);
        XRaiseWindow(dis,transient->win);
    }
    if(STATUS_BAR == 0 && show_bar == 0) getwindowname();

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
    master_size = (mode == 2) ? (sh*msize)/100 : (sw*msize)/100;
    if(mode == 1 && head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis, c->win);

    save_desktop(current_desktop);
    tile();
    update_current();
    if(STATUS_BAR == 0 && show_bar == 0) update_bar();
}

void resize_master(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->width += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    } else if(arg.i > 0) {
        if((mode != 2 && sw-master_size > 70) || (mode == 2 && sh-master_size > 70))
            master_size += arg.i;
    } else if(master_size > 70) master_size += arg.i;
    tile();
}

void resize_stack(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->height += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height+arg.i);
    } else if(mode == 3) {
        if(arg.i > 0 && ((sh/2+growth) < (sh-100))) growth += arg.i;
        else if(arg.i < 0 && ((sh/2+growth) > 80)) growth += arg.i;
        tile();
    } else if(desktops[current_desktop].numwins > 2) {
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
    unsigned int i;
    int j;
    XModifierKeymap *modmap;
    KeyCode code;

    XUngrabKey(dis, AnyKey, AnyModifier, root);
    read_keys_file();
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
    for(i=0;i<keycount;++i) {
        code = XKeysymToKeycode(dis,XStringToKeysym(keys[i].keysym));
        XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | LockMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask | LockMask, root, True, GrabModeAsync, GrabModeAsync);
    }
    for(i=1;i<4;i+=2) {
        XGrabButton(dis, i, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | numlockmask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | numlockmask | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    }
}

void keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XkbKeycodeToKeysym(dis, (KeyCode)ev->keycode, 0, 0);
    for(i = 0; i < keycount; i++) {
        if(keysym == XStringToKeysym(keys[i].keysym) && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)) {
            if(keys[i].myfunction)
                keys[i].myfunction(keys[i].arg);
        }
    }
}

void warp_pointer() {
    // Move cursor to the center of the current window
    if(followmouse != 0) return;
    if(dowarp < 1 && current != NULL) {
        XGetWindowAttributes(dis, current->win, &attr);
        XWarpPointer(dis, None, current->win, 0, 0, 0, 0, attr.width/2, attr.height/2);
        return;
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
    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4;
    wc.y = ev->y + y;
    wc.width = (ev->width < sw-bdw) ? ev->width : sw+bdw;
    wc.height = (ev->height < sh-bdw) ? ev->height : sh+bdw;
    wc.border_width = 0;
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

    int y=0;
    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4;
    // For fullscreen mplayer (and maybe some other program)
    client *c;
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            //XMoveResizeWindow(dis,c->win,0,y,sw+bdw,sh-y+bdw);
            return;
        }

    Window trans = None;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
        add_window(ev->window, 1); 
        if((attr.y + attr.height) > sh)
            XMoveResizeWindow(dis,ev->window,attr.x,y,attr.width,attr.height-10);
        XSetWindowBorderWidth(dis,ev->window,bdw);
        XSetWindowBorder(dis,ev->window,theme[0].wincolor);
        update_current();
        XMapRaised(dis,ev->window);
        XSetInputFocus(dis,ev->window,RevertToParent,CurrentTime);
        return;
    }

    XClassHint ch = {0};
    unsigned int i=0, j=0, tmp = current_desktop;
    if(XGetClassHint(dis, ev->window, &ch))
        for(i=0;i<dtcount;i++)
            if(strcmp(ch.res_class, convenience[i].class) == 0) {
                save_desktop(tmp);
                select_desktop(convenience[i].preferredd-1);
                for(c=head;c;c=c->next)
                    if(ev->window == c->win)
                        ++j;
                if(j < 1) add_window(ev->window, 0);
                if(tmp == convenience[i].preferredd-1) {
                    tile();
                    XMapWindow(dis, ev->window);
                    update_current();
                } else select_desktop(tmp);
                if(convenience[i].followwin == 0) {
                    Arg a = {.i = convenience[i].preferredd-1};
                    change_desktop(a);
                }
                if(STATUS_BAR == 0) update_bar();
                if(ch.res_class) XFree(ch.res_class);
                if(ch.res_name) XFree(ch.res_name);
                return;
            }
    if(ch.res_class) XFree(ch.res_class);
    if(ch.res_name) XFree(ch.res_name);

    add_window(ev->window, 0);
    if(mode != 4) tile();
    if(mode != 1) XMapWindow(dis,ev->window);
    warp_pointer();
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void destroynotify(XEvent *e) {
    unsigned int i = 0, tmp = current_desktop;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if(transient != NULL) {
        for(c=transient;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0, 1);
                update_current();
                return;
            }
    }
    save_desktop(tmp);
    for(i=0;i<TABLENGTH(desktops);++i) {
        select_desktop(i);
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0, 0);
                select_desktop(tmp);
                return;
            }
    }
    select_desktop(tmp);
}

void enternotify(XEvent *e) {
    if(followmouse != 0) return;

    int i;
    XCrossingEvent *ev = &e->xcrossing;
    for(i=0;i<DESKTOPS;i++)
        if(sb_bar[i].sb_win == ev->window) {
            dowarp = 1;
            return;
        }
    if(ev->window == sb_area) {
        dowarp = 1;
        return;
    }
}

void leavenotify(XEvent *e) {
    if(followmouse != 0) return;

    int i;
    XCrossingEvent *ev = &e->xcrossing;
    for(i=0;i<DESKTOPS;i++)
        if(sb_bar[i].sb_win == ev->window) {
            dowarp = 0;
            return;
        }
    if(ev->window == sb_area) {
        dowarp = 0;
        return;
    }
}

void buttonpress(XEvent *e) {
    XButtonEvent *ev = &e->xbutton;
    client *c;
    int i;

    if(STATUS_BAR == 0) {
        if(sb_area == ev->subwindow || sb_area == ev->window) {
            Arg a = {.i = previous_desktop};
            change_desktop(a);
            return;
        }
        for(i=0;i<DESKTOPS;i++)
            if((sb_bar[i].sb_win == ev->window || sb_bar[i].sb_win == ev->subwindow) && i != current_desktop) {
                Arg a = {.i = i};
                change_desktop(a);
                return;
            } else {
                if((sb_bar[i].sb_win == ev->window || sb_bar[i].sb_win == ev->subwindow) && i == current_desktop) {
                    next_win();
                    return;
                }
            }
    }

    // change focus with LMB
    if(clicktofocus == 0 && ev->window != current->win && ev->button == Button1)
        for(c=head;c;c=c->next) {
            if(ev->window == c->win) {
                current = c;
                update_current();
                XSendEvent(dis, PointerWindow, False, 0xfff, e);
                XFlush(dis);
                return;
            }
        }

    if(ev->subwindow == None) return;

    i = 0;
    if(mode == 4) {
        for(c=head;c;c=c->next)
            if(ev->subwindow == c->win) i = 1;
        if(i == 0) return;
        XGrabPointer(dis, ev->subwindow, True,
            PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
            GrabModeAsync, None, None, CurrentTime);
        XGetWindowAttributes(dis, ev->subwindow, &attr);
        starter = e->xbutton; doresize = 1;
    }
}

void motionnotify(XEvent *e) {
    int xdiff, ydiff;
    client *c;
    XMotionEvent *ev = &e->xmotion;

    if(ev->window != current->win) {
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                current = c;
                update_current();
                dowarp = 0;
                return;
           }
    }
    if(doresize < 1) return;
    while(XCheckTypedEvent(dis, MotionNotify, e));
    xdiff = ev->x_root - starter.x_root;
    ydiff = ev->y_root - starter.y_root;
    XMoveResizeWindow(dis, ev->window,
        attr.x + (starter.button==1 ? xdiff : 0),
        attr.y + (starter.button==1 ? ydiff : 0),
        MAX(1, attr.width + (starter.button==3 ? xdiff : 0)),
        MAX(1, attr.height + (starter.button==3 ? ydiff : 0)));
}

void buttonrelease(XEvent *e) {
    client *c;
    XButtonEvent *ev = &e->xbutton;

    if(doresize < 1) {
        XSendEvent(dis, PointerWindow, False, 0xfff, e);
        XFlush(dis);
        return;
    }
    XUngrabPointer(dis, CurrentTime);
    if(mode == 4) {
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                XGetWindowAttributes(dis, c->win, &attr);
                c->x = attr.x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height;
            }
        update_current();
    }
    doresize = 0;
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
    unsigned int i = 0, tmp = current_desktop;
    client *c;

    if(ev->send_event == 1) {
        if(transient != NULL) {
            for(c=transient;c;c=c->next)
                if(ev->window == c->win) {
                    remove_window(ev->window, 1, 1);
                    select_desktop(tmp);
                    return;
                }
        }
        save_desktop(tmp);
        for(i=0;i<TABLENGTH(desktops);++i) {
            select_desktop(i);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    remove_window(ev->window, 1, 0);
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
    kill_client_now(current->win);
    remove_window(current->win, 0, 0);
}

void kill_client_now(Window w) {
    int n, i;
    XEvent ke;

    if (XGetWMProtocols(dis, w, &protocols, &n) != 0) {
        for(i=n;i>=0;i--) {
            if (protocols[i] == wm_delete_window) {
                ke.type = ClientMessage;
                ke.xclient.window = w;
                ke.xclient.message_type = protos;
                ke.xclient.format = 32;
                ke.xclient.data.l[0] = wm_delete_window;
                ke.xclient.data.l[1] = CurrentTime;
                XSendEvent(dis, w, False, NoEventMask, &ke);
            }
        }
    } else XKillClient(dis, w);
    XFree(protocols);
}

void quit() {
    unsigned int i;
    client *c;

    for(i=0;i<DESKTOPS;++i) {
        if(desktops[i].head != NULL) select_desktop(i);
        else continue;
        for(c=head;c;c=c->next)
            kill_client_now(c->win);
    }
    XClearWindow(dis, root);
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    for(i=0;i<7;i++)
        XFreeGC(dis, theme[i].gc);
    XFreeGC(dis, bggc);
    XFreePixmap(dis, area_sb);
    XSync(dis, False);
    XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);
    logger("\033[0;34mYou Quit : Bye!");
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

    // Initialize variables
    DESKTOPS = 4;
    mode = DEFAULT_MODE;
    msize = MASTER_SIZE;
    ufalpha = UF_ALPHA;
    bdw = BORDER_WIDTH;
    attachaside = ATTACH_ASIDE;
    top_stack = TOP_STACK;
    followmouse = FOLLOW_MOUSE;
    clicktofocus = CLICK_TO_FOCUS;
    windownamelength = WINDOW_NAME_LENGTH;
    topbar = TOP_BAR;
    showopen = SHOW_NUM_OPEN;
    LA_WINDOWNAME = 0;
    dowarp = doresize = 0;

    char *loc;
    loc = setlocale(LC_ALL, "");
    if (!loc || !strcmp(loc, "C") || !strcmp(loc, "POSIX") || !XSupportsLocale())
        logger("LOCALE FAILED");
    // Read in RC_FILE
    sprintf(RC_FILE, "%s/.config/snapwm/rc.conf", getenv("HOME"));
    sprintf(KEY_FILE, "%s/.config/snapwm/key.conf", getenv("HOME"));
    sprintf(APPS_FILE, "%s/.config/snapwm/apps.conf", getenv("HOME"));
    set_defaults();
    read_rcfile();
    if(STATUS_BAR == 0) {
        show_bar = STATUS_BAR;
        setup_status_bar();
        status_bar();
        update_output(1);
        if(SHOW_BAR > 0) toggle_bar();
    }
    read_apps_file();

    // Shortcuts
    grabkeys();

    // For exiting
    bool_quit = 0;

    // Master size
    master_size = (mode == 2) ? (sh*msize)/100 : (sw*msize)/100;

    // Set up all desktop
    unsigned int i;
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].nmaster = 0;
        desktops[i].mode = mode;
        desktops[i].growth = 0;
        desktops[i].numwins = 0;
        desktops[i].head = NULL;
        desktops[i].current = NULL;
        desktops[i].transient = NULL;
    }

    // Select first dekstop by default
    select_desktop(0);
    alphaatom = XInternAtom(dis, "_NET_WM_WINDOW_OPACITY", False);
    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", False);
    protos = XInternAtom(dis, "WM_PROTOCOLS", False);
    update_current();

    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask|PropertyChangeMask);
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


int main() {
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
