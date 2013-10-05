 /* snapwm.c
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
/* For a multimedia keyboard */
#include <X11/XF86keysym.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
// #include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union {
    char *com[15];
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
    unsigned int master_size, screen;
    unsigned int mode, growth, numwins, nmaster, showbar;
    unsigned int x, y, w, h;
    client *head;
    client *current;
    client *transient;
};

typedef struct {
    int cd;
} MonitorView;

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
    unsigned int fh; /* Y coordinate to draw characters */
    int ascent;
    int descent;
} Iammanyfonts;

typedef struct {
    Window sb_win;
    char *label;
    unsigned int width;
    unsigned int labelwidth;
} Barwin;

typedef struct {
    unsigned long barcolor, wincolor, textcolor;
    char *modename;
    GC gc;
} Theme;

typedef struct {
    char *name;
    char *list[15];
} Commands;

// Functions
static void add_window(Window w, int tw, client *cl);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void draw_desk(Window win, unsigned int barcolor, unsigned int gc, unsigned int x, char *string, unsigned int len);
static void draw_text(Window win, unsigned int gc, unsigned int x, char *string, unsigned int len);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void follow_client_to_desktop(const Arg arg);
static unsigned long getcolor(const char* color);
static void get_font();
static void getwindowname();
static void grabkeys();
static void init_desks();
static void keypress(XEvent *e);
static void kill_client();
static void kill_client_now(Window w);
static void last_desktop();
static void leavenotify(XEvent *e);
static void logger(const char* e);
static void mapbar();
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
static void remove_client(client *cl, unsigned int dr, unsigned int tw);
static void read_apps_file();
static void read_keys_file();
static void read_rcfile();
static void resize_master(const Arg arg);
static void resize_stack(const Arg arg);
static void rotate_desktop(const Arg arg);
static void rotate_mode(const Arg arg);
static void save_desktop(int i);
static void select_desktop(int i);
static void setbaralpha();
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
static void unmapbar();
static void unmapnotify(XEvent *e);    // Thunderbird's write window just unmaps...
static void update_bar();
static void update_config();
static void update_current();
static void update_output(unsigned int messg);
static void warp_pointer();
static unsigned int wc_size(char *string);

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static unsigned int attachaside, bdw, bool_quit, clicktofocus, current_desktop, doresize, dowarp, cstack;
static unsigned int screen, followmouse, mode, msize, previous_desktop, DESKTOPS, STATUS_BAR, numwins;
static unsigned int auto_mode, auto_num;
static int num_screens, growth, sh, sw, master_size, nmaster;
static unsigned int sb_desks;        // width of the desktop switcher
static unsigned int sb_height, sb_width, screen, show_bar, has_bar, wnamebg, barmon, barmonchange;
static unsigned int showopen;        // whether the desktop switcher shows number of open windows
static unsigned int topbar, top_stack, windownamelength, keycount, cmdcount, dtcount, pcount, LA_WINDOWNAME;
static int ufalpha, baralpha;
static unsigned long opacity, baropacity;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask, resizemovekey;        /* dynamic key lock mask */
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
static MonitorView view[5];
static Barwin sb_bar[10];
static Theme theme[10];
static Iammanyfonts font;
static key keys[80];
static Commands cmds[50];
static Convenience convenience[20];
static Positional positional[20];
#include "bar.c"
#include "readrc.c"
#include "readkeysapps.c"

/* ***************************** Window Management ******************************* */
void add_window(Window w, int tw, client *cl) {
    client *c,*t, *dummy;

    if(cl != NULL) c = cl;
    else if(!(c = (client *)calloc(1,sizeof(client)))) {
        logger("\033[0;31mError calloc!");
        exit(1);
    }

    if(tw == 0 && cl == NULL) {
        XClassHint chh = {0};
        unsigned int i, j=0;
        if(XGetClassHint(dis, w, &chh)) {
            for(i=0;i<pcount;++i)
                if((strcmp(chh.res_class, positional[i].class) == 0) ||
                  (strcmp(chh.res_name, positional[i].class) == 0)) {
                    XMoveResizeWindow(dis,w,desktops[current_desktop].x+positional[i].x,desktops[current_desktop].y+positional[i].y,positional[i].width,positional[i].height);
                    ++j;
                }
            if(chh.res_class) XFree(chh.res_class);
            if(chh.res_name) XFree(chh.res_name);
        } 
        if(j == 0 && cstack == 0) {
            XGetWindowAttributes(dis, w, &attr);
            XMoveWindow(dis, w,desktops[current_desktop].x+desktops[current_desktop].w/2-(attr.width/2),desktops[current_desktop].y+(desktops[current_desktop].h+sb_height+4)/2-(attr.height/2));
        }
    }
    XGetWindowAttributes(dis, w, &attr);
    c->x = attr.x;
    if(topbar == 0 && attr.y < sb_height+4+bdw) c->y = sb_height+4+bdw;
    else c->y = attr.y;
    c->width = attr.width;
    c->height = attr.height;
    XMoveWindow(dis, w,c->x,c->y);

    c->win = w; c->order = 0;
    dummy = (tw == 1) ? transient : head;
    for(t=dummy;t;t=t->next)
        ++t->order;

    if(dummy == NULL) {
        c->next = NULL; c->prev = NULL;
        dummy = c;
    } else {
        if(attachaside == 0) {
            if(top_stack == 0) {
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
    numwins += 1;
    if(growth > 0) growth = growth*(numwins-1)/numwins;
    else growth = 0;
    save_desktop(current_desktop);
    
    if(mode == 4 && auto_num > 0 && numwins >= auto_num)
            mode = auto_mode;

    // for folow mouse and statusbar updates
    if(followmouse == 0 && STATUS_BAR == 0)
        XSelectInput(dis, c->win, PointerMotionMask|PropertyChangeMask);
    else if(followmouse == 0)
        XSelectInput(dis, c->win, PointerMotionMask);
    else if(STATUS_BAR == 0)
        XSelectInput(dis, c->win, PropertyChangeMask);
}

void remove_client(client *cl, unsigned int dr, unsigned int tw) {
    client *t, *dummy;

    dummy = (tw == 1) ? transient : head;
    if(cl->prev == NULL && cl->next == NULL) {
        dummy = NULL;
    } else if(cl->prev == NULL) {
        dummy = cl->next;
        cl->next->prev = NULL;
    } else if(cl->next == NULL) {
        cl->prev->next = NULL;
    } else {
        cl->prev->next = cl->next;
        cl->next->prev = cl->prev;
    }
    if(tw == 1) {
        transient = dummy;
        free(cl);
        save_desktop(current_desktop);
        return;
    } else {
        head = dummy;
        XUngrabButton(dis, AnyButton, AnyModifier, cl->win);
        numwins -= 1;
        if(head != NULL) {
            for(t=head;t;t=t->next) {
                if(t->order > cl->order) --t->order;
                if(t->order == 0) current = t;
            }
        } else current = NULL;
        if(dr == 0) free(cl);
        if(numwins < 3) growth = 0;
        save_desktop(current_desktop);
        if(mode != 4) tile();
        XUnmapWindow(dis, cl->win);
        return;
    }
}

void next_win() {
    if(numwins < 2) return;
    Window w = current->win;

    current = (current->next == NULL) ? head : current->next;
    save_desktop(current_desktop);
    if(mode == 1) {
        tile();
        XUnmapWindow(dis, w);
    }
    update_current();
}

void prev_win() {
    if(numwins < 2) return;
    client *c; Window w = current->win;

    if(current->prev == NULL) for(c=head;c->next;c=c->next);
    else c = current->prev;
    current = c;
    save_desktop(current_desktop);
    if(mode == 1) {
        tile();
        XUnmapWindow(dis, w);
    }
    update_current();
}

void move_down(const Arg arg) {
    if(mode == 4 && current != NULL) {
        current->y += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
        return;
    }
    if(current == NULL || current->next == NULL || current->win == head->win || current->prev == NULL)
        return;

    Window tmp = current->win;
    current->win = current->next->win;
    current->next->win = tmp;
    //keep the moved window activated
    next_win();
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
    if(current == NULL || current->prev == head || current->win == head->win)
        return;

    Window tmp = current->win;
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

    if(numwins < 2 || mode == 1) return;
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

/* **************************** Desktop Management ************************************* */

void change_desktop(const Arg arg) {
    if(arg.i >= DESKTOPS || arg.i == current_desktop) return;
    client *c;

    int next_view = desktops[arg.i].screen;
    if(next_view != desktops[current_desktop].screen && dowarp == 0) {
        // Find cursor position on current monitor, adapt it to next monitor
        Window dummy;
        int cx=0, cy=0, dx, dy, rx, ry;
        unsigned int mask;
        XQueryPointer(dis, root, &dummy, &dummy, &rx, &ry, &cx, &cy, &mask);
        dx = cx - desktops[current_desktop].x + desktops[arg.i].x;
        if(dx > (desktops[arg.i].x+desktops[arg.i].w-10))
            dx = (desktops[arg.i].x+desktops[arg.i].w)-10;
        dy = cy - desktops[current_desktop].y + desktops[arg.i].y;
        if(dy > (desktops[arg.i].y+desktops[arg.i].h-10))
            dy = (desktops[arg.i].y+desktops[arg.i].h)-10;
        XWarpPointer(dis, None, root, 0, 0, 0, 0, dx, dy);
    }

    // Save current "properties"
    save_desktop(current_desktop);
    previous_desktop = current_desktop;

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    if(next_view == barmon && has_bar == 1 && show_bar == 0) mapbar();
    if(next_view == barmon && has_bar == 0 && show_bar == 1) unmapbar();

    // Map all windows
    if(head != NULL) {
        if(mode != 1)
            for(c=head;c;c=c->next)
                XMapWindow(dis,c->win);
        tile();
    }
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XMapWindow(dis,c->win);

    select_desktop(previous_desktop);
    if(arg.i != view[next_view].cd) {
        select_desktop(view[next_view].cd);
        // Unmap all window
        if(transient != NULL)
            for(c=transient;c;c=c->next)
                XUnmapWindow(dis,c->win);

        if(head != NULL)
            for(c=head;c;c=c->next)
                XUnmapWindow(dis,c->win);
    }

    select_desktop(arg.i);
    view[next_view].cd = current_desktop;
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
    if(current == NULL || arg.i == current_desktop || arg.i >= DESKTOPS) return;
    client_to_desktop(arg);
    change_desktop(arg);
}

void client_to_desktop(const Arg arg) {
    if(current == NULL || arg.i == current_desktop || arg.i >= DESKTOPS) return;

    client *tmp = current;
    unsigned int tmp2 = current_desktop, j, cd = desktops[current_desktop].screen;

    tmp->x -= desktops[current_desktop].x;
    tmp->y -= desktops[current_desktop].y;
    // Remove client from current desktop
    remove_client(current, 1, 0);

    // Add client to desktop
    select_desktop(arg.i);
    tmp->x += desktops[current_desktop].x;
    tmp->y += desktops[current_desktop].y;
    add_window(tmp->win, 0, tmp);
    save_desktop(arg.i);

    for(j=cd;j<cd+num_screens;++j) {
        if(view[j%num_screens].cd == arg.i) {
            tile();
            XMapWindow(dis, current->win);
        }
    }
    select_desktop(tmp2);
    update_current();

    if(STATUS_BAR == 0) update_bar();
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].nmaster = nmaster;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].showbar = show_bar;
    desktops[i].head = head;
    desktops[i].current = current;
    desktops[i].transient = transient;
    desktops[i].numwins = numwins;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    nmaster = desktops[i].nmaster;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    show_bar = desktops[i].showbar;
    head = desktops[i].head;
    current = desktops[i].current;
    transient = desktops[i].transient;
    numwins = desktops[i].numwins;
    current_desktop = i;
    sw = desktops[current_desktop].w;
    sh = desktops[current_desktop].h;
}

void more_master (const Arg arg) {
    if(arg.i > 0) {
        if((numwins < 3) || (nmaster == (numwins-2))) return;
        nmaster += 1;
    } else {
        if(nmaster == 0) return;
        nmaster -= 1;
    }
    save_desktop(current_desktop);
    tile();
}

void tile() {
    if(head == NULL) return;
    client *c, *d=NULL;
    unsigned int x = 0, xpos = 0, ypos=0, wdt = 0, msw, ssw, ncols = 2, nrows = 1;
    int ht = 0, y, n = 0, nm = (numwins < 3) ? 0: (numwins-2 < nmaster) ? (numwins-2):nmaster;
    int scrx = desktops[current_desktop].x;
    int scry = desktops[current_desktop].y;

    // For a top bar
    y = (STATUS_BAR == 0 && topbar == 0 && show_bar == 0) ? sb_height+4 : 0; ypos = y;

    // If only one window
    if(mode != 4 && head != NULL && head->next == NULL) {
        if(mode == 1) XMapWindow(dis, current->win);
        XMoveResizeWindow(dis,head->win,scrx,scry+y,sw+bdw,sh+bdw);
    } else {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
            	if(nm < 1)
                    XMoveResizeWindow(dis,head->win,scrx,scry+y,master_size - bdw,sh - bdw);
                else {
                    for(d=head;d;d=d->next) {
                        XMoveResizeWindow(dis,d->win,scrx,scry+ypos,master_size - bdw,sh/(nm+1) - bdw);
                        if(x == nm) break;
                        ypos += sh/(nm+1); ++x;
                    }
                }

                // Stack
                if(d == NULL) d = head;
                n = numwins - (nm+1);
                XMoveResizeWindow(dis,d->next->win,scrx+master_size,scry+y,sw-master_size-bdw,(sh/n)+growth - bdw);
                y += (sh/n)+growth;
                for(c=d->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,scrx+master_size,scry+y,sw-master_size-bdw,(sh/n)-(growth/(n-1)) - bdw);
                    y += (sh/n)-(growth/(n-1));
                }
                break;
            case 1: /* Fullscreen */
                XMoveResizeWindow(dis,current->win,scrx,scry+y,sw+bdw,sh+bdw);
                XMapWindow(dis, current->win);
                break;
            case 2: /* Horizontal */
            	// Master window
            	if(nm < 1)
                    XMoveResizeWindow(dis,head->win,scrx+xpos,scry+ypos,sw-bdw,master_size-bdw);
                else {
                    for(d=head;d;d=d->next) {
                        XMoveResizeWindow(dis,d->win,scrx+xpos,scry+ypos,sw/(nm+1)-bdw,master_size-bdw);
                        if(x == nm) break;
                        xpos += sw/(nm+1); ++x;
                    }
                }

                // Stack
                if(d == NULL) d = head;
                n = numwins - (nm+1);
                XMoveResizeWindow(dis,d->next->win,scrx,scry+y+master_size,(sw/n)+growth-bdw,sh-master_size-bdw);
                msw = (sw/n)+growth;
                for(c=d->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,scrx+msw,scry+y+master_size,(sw/n)-(growth/(n-1)) - bdw,sh-master_size-bdw);
                    msw += (sw/n)-(growth/(n-1));
                }
                break;
            case 3: { // Grid
                if(numwins < 5) {
                    for(c=head;c;c=c->next) {
                        ++n;
                        if(numwins > 2) {
                            if((n == 1) || (n == 2))
                                ht = (sh/2) + growth - bdw;
                            if(n > 2)
                                ht = (sh/2) - growth - bdw;
                        } else ht = sh - bdw;
                        if((n == 1) || (n == 3)) {
                            xpos = 0;
                            wdt = master_size - bdw;
                        }
                        if((n == 2) || (n == 4)) {
                            xpos = master_size;
                            wdt = (sw - master_size) - bdw;
                        }
                        if(n == 3)
                            ypos += (sh/2) + growth;
                        if((n == numwins) && (n == 3))
                            wdt = sw - bdw;
                        XMoveResizeWindow(dis,c->win,scrx+xpos,scry+ypos,wdt,ht);
                    }
                } else {
                    x = numwins;
                    for(xpos=0;xpos<=x;++xpos) {
                        if(xpos == 3 || xpos == 7 || xpos == 10 || xpos == 17) ++nrows;
                        if(xpos == 5 || xpos == 13 || xpos == 21) ++ncols;
                    }
                    msw = (ncols > 2) ? ((master_size*2)/ncols) : master_size;
                    ssw = (sw - msw)/(ncols-1); ht = sh/nrows;
                    xpos = msw+(ssw*(ncols-2)); ypos = y+((nrows-1)*ht);
                    for(c=head;c->next;c=c->next);
                    for(d=c;d;d=d->prev) {
                        --x;
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
                        XMoveResizeWindow(dis,d->win,scrx+xpos,scry+ypos,wdt-bdw,ht-bdw);
                        ht = sh/nrows;
                        ypos -= ht; ++n;
                    }
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
    client *c, *d; unsigned int border, tmp = current_desktop, i;

    save_desktop(current_desktop);
    for(i=0;i<num_screens;++i) {
        if(view[i].cd != current_desktop) {
            select_desktop(view[i].cd);
            if(head != NULL) {
                XSetInputFocus(dis,root,RevertToParent,CurrentTime);
                XSetWindowBorder(dis,current->win,theme[1].wincolor);
                if(clicktofocus == 0)
                    XGrabButton(dis, AnyButton, AnyModifier, current->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
            }
        }
    }
    select_desktop(tmp);

    if(head == NULL) return;

    border = ((head->next == NULL && mode != 4) || (mode == 1)) ? 0 : bdw;
    for(c=head;c->next;c=c->next);
    for(d=c;d;d=d->prev) {
        XSetWindowBorderWidth(dis,d->win,border);
        if(d != current) {
            if(d->order < current->order) ++d->order;
            if(ufalpha < 100) XChangeProperty(dis, d->win, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &opacity, 1l);
            XSetWindowBorder(dis,d->win,theme[1].wincolor);
            if(clicktofocus == 0)
                XGrabButton(dis, AnyButton, AnyModifier, d->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
        else {
            // "Enable" current window
            if(ufalpha < 100) XDeleteProperty(dis, d->win, alphaatom);
            XSetWindowBorder(dis,d->win,theme[0].wincolor);
            if(transient == NULL)
                XSetInputFocus(dis,d->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,d->win);
            if(clicktofocus == 0)
                XUngrabButton(dis, AnyButton, AnyModifier, d->win);
        }
    }
    current->order = 0;
    if(transient != NULL) {
        for(c=transient;c->next;c=c->next);
        for(d=c;d;d=d->prev) {
            XMoveResizeWindow(dis,d->win,d->x,d->y,d->width,d->height);
            XRaiseWindow(dis,d->win);
        }
        XSetInputFocus(dis,transient->win,RevertToParent,CurrentTime);
    }
    if(STATUS_BAR == 0 && show_bar == 0) getwindowname();
    warp_pointer();
    XSync(dis, False);
}

void switch_mode(const Arg arg) {
    if(mode == arg.i) return;

    client *c;
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
    if(STATUS_BAR == 0) update_bar();
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
    } else if(numwins > 2) {
        int n = numwins-1;
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
    XUngrabButton(dis, AnyButton, AnyModifier, root);
    read_keys_file();
    // numlock workaround
    numlockmask = 0;
    modmap = XGetModifierMapping(dis);
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < modmap->max_keypermod; ++j) {
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
        XGrabButton(dis, i, resizemovekey, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, resizemovekey | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, resizemovekey | numlockmask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, resizemovekey | numlockmask | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    }
}

void keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XkbKeycodeToKeysym(dis, (KeyCode)ev->keycode, 0, 0);
    for(i = 0; i < keycount; ++i) {
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

    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4;
    wc.x = ev->x;
    wc.y = ev->y + y;
    if(mode == 4) {
        wc.width = (ev->width < sw-2*bdw) ? ev->width : sw-2*bdw;
        wc.height = (ev->height < sh-2*bdw) ? ev->height : sh-2*bdw;
    } else {
        wc.width = (ev->width < sw+bdw) ? ev->width : sw+bdw;
        wc.height = (ev->height < sh+bdw) ? ev->height : sh+bdw;
    }
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

    unsigned int y=0;
    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4;
    // For fullscreen mplayer (and maybe some other program)
    client *c; Window w;
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            //XMoveResizeWindow(dis,c->win,0,y,sw+bdw,sh-y+bdw);
            return;
        }

    w = (numwins > 0) ? current->win:0;

    Window trans = None;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
        add_window(ev->window, 1, NULL); 
        if((attr.y + attr.height) > sh)
            XMoveResizeWindow(dis,ev->window,attr.x,y,attr.width,attr.height-10);
        XSetWindowBorderWidth(dis,ev->window,bdw);
        XSetWindowBorder(dis,ev->window,theme[0].wincolor);
        XMapRaised(dis,ev->window);
        XSetInputFocus(dis,ev->window,RevertToParent,CurrentTime);
        return;
    }

    XClassHint ch = {0};
    unsigned int i=0, j=0, tmp = current_desktop, tmp2;
    if(XGetClassHint(dis, ev->window, &ch))
        for(i=0;i<dtcount;++i)
            if((strcmp(ch.res_class, convenience[i].class) == 0) ||
              (strcmp(ch.res_name, convenience[i].class) == 0)) {
                tmp2 = (convenience[i].preferredd > DESKTOPS) ? DESKTOPS-1 : convenience[i].preferredd-1;
                save_desktop(tmp);
                select_desktop(tmp2);
                for(c=head;c;c=c->next)
                    if(ev->window == c->win)
                        ++j;
                if(j < 1) add_window(ev->window, 0, NULL);
                for(j=0;j<num_screens;++j) {
                    if(view[j].cd == convenience[i].preferredd-1) {
                        tile();
                        XMapWindow(dis, ev->window);
                        if(mode == 1 && numwins > 1) XUnmapWindow(dis, w);
                        update_current();
                    }
                }
                select_desktop(tmp);
                if(convenience[i].followwin == 0) {
                    Arg a = {.i = tmp2};
                    change_desktop(a);
                }
                if(STATUS_BAR == 0) update_bar();
                if(ch.res_class) XFree(ch.res_class);
                if(ch.res_name) XFree(ch.res_name);
                return;
            }
    if(ch.res_class) XFree(ch.res_class);
    if(ch.res_name) XFree(ch.res_name);

    add_window(ev->window, 0, NULL);
    if(mode != 4) tile();
    if(mode != 1) XMapWindow(dis,ev->window);
    if(mode == 1 && numwins > 1) XUnmapWindow(dis, w);
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void destroynotify(XEvent *e) {
    unsigned int i = 0, tmp = current_desktop, foundit = 0;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    save_desktop(tmp);
    for(i=tmp;i<tmp+DESKTOPS;++i) {
        select_desktop(i%DESKTOPS);
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_client(c, 0, 0);
                foundit = 1;
                break;
            }
        if(transient != NULL && foundit < 1) {
            for(c=transient;c;c=c->next)
                if(ev->window == c->win) {
                    remove_client(c, 0, 1);
                    foundit = 1;
                    break;
                }
        }
        if(foundit == 1) break;
    }
    select_desktop(tmp);
    if(foundit > 0) update_current();
    if(STATUS_BAR == 0) update_bar();
}

void enternotify(XEvent *e) {
    int i;
    XCrossingEvent *ev = &e->xcrossing;
    if(ev->window == sb_area) {
        dowarp = 1;
        return;
    }
    for(i=0;i<DESKTOPS;++i)
        if(sb_bar[i].sb_win == ev->window) {
            dowarp = 1;
            return;
        }
}

void leavenotify(XEvent *e) {
    int i;
    XCrossingEvent *ev = &e->xcrossing;
    if(ev->window == sb_area) {
        dowarp = 0;
        return;
    }
    for(i=0;i<DESKTOPS;++i)
        if(sb_bar[i].sb_win == ev->window) {
            dowarp = 0;
            return;
        }
}

void buttonpress(XEvent *e) {
    XButtonEvent *ev = &e->xbutton;
    client *c;
    int i=0, cd = current_desktop;

    if(STATUS_BAR == 0) {
        if(sb_area == ev->subwindow || sb_area == ev->window) {
            Arg a = {.i = previous_desktop};
            dowarp = 1;
            change_desktop(a);
            return;
        }
        for(i=0;i<DESKTOPS;++i)
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

    for(c=transient;c;c=c->next)
        if(ev->window == c->win) {
            XSetInputFocus(dis,ev->window,RevertToParent,CurrentTime);
            return;
        }
    // change focus with LMB
    unsigned int cds = desktops[cd].screen;
    if(clicktofocus == 0 && ev->button == Button1)
        save_desktop(cd);
        for(i=cds;i<cds+num_screens;++i) {
            select_desktop(view[i%num_screens].cd);
            for(c=head;c;c=c->next) {
                if(ev->window == c->win) {
                    Arg a = {.i = current_desktop};
                    select_desktop(cd); dowarp = 1;
                    change_desktop(a); dowarp = 0;
                    current = c;
                    update_current();
                    XSendEvent(dis, PointerWindow, False, 0xfff, e);
                    update_bar();
                    XFlush(dis);
                    return;
                }
            }
        }

    select_desktop(cd);
    if(ev->subwindow == None) return;

    i = 0;
    for(c=transient;c;c=c->next)
        if(ev->subwindow == c->win) i = 1;
    if(mode == 4 || i > 0) {
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
    int xdiff = ev->x_root - starter.x_root;
    int ydiff = ev->y_root - starter.y_root;
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
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            if(ev->window == c->win) {
                XGetWindowAttributes(dis, c->win, &attr);
                c->x = attr.x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height;
            }
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

    if(ev->state == PropertyDelete) return;
    else
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
                    remove_client(c, 1, 1);
                    select_desktop(tmp);
                    update_current();
                    return;
                }
        }
        save_desktop(tmp);
        for(i=0;i<TABLENGTH(desktops);++i) {
            select_desktop(i);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    remove_client(c, 1, 0);
                    select_desktop(tmp);
                    update_current();
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
    remove_client(current, 0, 0);
    update_current();
    if(STATUS_BAR == 0) update_bar();
}

void kill_client_now(Window w) {
    int n, i;
    XEvent ke;

    if (XGetWMProtocols(dis, w, &protocols, &n) != 0) {
        for(i=n;i>=0;--i) {
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
    for(i=0;i<10;++i)
        XFreeGC(dis, theme[i].gc);
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

void init_desks() {
    int last_width=0, i, j, have_Xin = 0;

    XineramaScreenInfo *info = NULL;
    if(!(info = XineramaQueryScreens(dis, &num_screens))) {
        logger("XINERAMA Fail");
        num_screens = 1;
        have_Xin = 1;
    }
    //printf("\t \nNumber of screens is %d\n\n", num_screens);

    if(barmon != barmonchange && barmonchange >= 0 && barmonchange < num_screens)
        barmon = barmonchange;
    for (i = 0; i < num_screens; ++i) {
        for(j=i;j<DESKTOPS;j+=num_screens) {
            if(i == barmon && STATUS_BAR == 0 && show_bar == 0) {
                desktops[j].h = ((have_Xin < 1) ? info[i].height:XDisplayHeight(dis, screen)) - (sb_height+4+bdw);
                desktops[j].showbar = 0;
            } else {
                desktops[j].h = ((have_Xin < 1) ? info[i].height:XDisplayHeight(dis, screen)) - bdw;
                desktops[j].showbar = 1;
            }
            if(!(desktops[j].mode)) desktops[j].mode = mode;
            if(!(desktops[j].nmaster)) desktops[j].nmaster = nmaster;
            if(desktops[j].w > 0) continue;
            //printf("**screen is %d - desktop is %d **\n", i, j);
            desktops[j].x = (have_Xin < 1) ? info[i].x_org + last_width:0;
            desktops[j].y = (have_Xin < 1) ? info[i].y_org:0;
            desktops[j].w = (have_Xin < 1) ? info[i].width - bdw:XDisplayWidth(dis, screen);
            //printf(" x=%d - y=%d - w=%d - h=%d \n", desktops[j].x, desktops[j].y, desktops[j].w, desktops[j].h);
            desktops[j].master_size = (desktops[j].mode == 2) ? (desktops[j].h*msize)/100 : (desktops[j].w*msize)/100;
            desktops[j].growth = 0;
            desktops[j].numwins = 0;
            desktops[j].head = NULL;
            desktops[j].current = NULL;
            desktops[j].transient = NULL;
            desktops[j].screen = i;
        }
        last_width += desktops[j].w;
        view[i].cd = i;
    }
    XFree(info);
}

void setup() {
    // Install a signal
    sigchld(0);

    // Screen and root window
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);
    XSelectInput(dis,root,SubstructureRedirectMask);

    // Initialize variables
    DESKTOPS = 4;
    topbar = followmouse = top_stack = mode = cstack = 0;
    LA_WINDOWNAME = wnamebg = dowarp = doresize = nmaster = 0;
    auto_mode = auto_num = 0;
    msize = 55;
    ufalpha = 75; baralpha = 90;
    bdw = 2;
    showopen = clicktofocus = attachaside = 1;
    resizemovekey = Mod1Mask;
    windownamelength = 35;
    show_bar = STATUS_BAR = barmon = barmonchange = 0;

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

    // Set up all desktops
    init_desks();

    if(STATUS_BAR == 0) {
        setup_status_bar();
        status_bar();
        update_output(1);
        if(show_bar > 0) toggle_bar();
    }
    read_apps_file();

    // Shortcuts
    grabkeys();

    // For exiting
    bool_quit = 0;

    // Select first dekstop by default
    select_desktop(0);
    alphaatom = XInternAtom(dis, "_NET_WM_WINDOW_OPACITY", False);
    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", False);
    protos = XInternAtom(dis, "WM_PROTOCOLS", False);
    update_current();
    setbaralpha();

    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask|PropertyChangeMask);
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
    if(ee->error_code == BadAccess) {
        logger("\033[0;31mIs Another Window Manager Running? Exiting!");
        exit(1);
    } else logger("\033[0;31mBad Window Error!");
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

    XSetErrorHandler(xerror);

    // Setup env
    setup();

    // Start wm
    start();

    // Close display
    XCloseDisplay(dis);

    exit(0);
}
