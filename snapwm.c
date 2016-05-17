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
#define _DEFAULT_SOURCE
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
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    unsigned int x, y, w, h, order, trans;
};

typedef struct desktop desktop;
struct desktop{
    unsigned int master_size, screen;
    unsigned int mode, growth, numwins, numorder, nmaster, showbar;
    unsigned int x, y, w, h;
    client *head, *current, *focus;
};

typedef struct {
    Window win;
    unsigned int numstuck;
} Stuckd;

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
    const char *class;
} Popped;

typedef struct {
    XFontStruct *font;
    XFontSet fontset;
    int height, width;
    unsigned int fh; /* Y coordinate to draw characters */
    int ascent, descent;
} Iammanyfonts;

typedef struct {
    Window sb_win;
    char *label;
    unsigned int width, labelwidth;
} Barwin;

typedef struct {
    char *modename;
    unsigned long barcolor, wincolor, textcolor;
    GC gc, swgc;
} Theme;

typedef struct {
    char *name;
    char *list[15];
} Commands;

// Functions
static void add_window(Window win, int tw, client *cl, int x, int y, int w, int h);
static void bar_rt_click();
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void change_desktop(const Arg arg);
static int check_dock(Window w);
static void client_to_desktop(const Arg arg);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void draw_desk(Window win, unsigned int barcolor, unsigned int gc, unsigned int x, char *string, unsigned int len);
static void draw_text(Window win, unsigned int gc, unsigned int x, char *string, unsigned int len);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void follow_client_to_desktop(const Arg arg);
static unsigned long getcolor(const char* color);
static void get_font();
static void getwindowname(Window win, unsigned int stext);
static void grabkeys();
static void init_desks();
static void init_start();
static void keypress(XEvent *e);
static void kill_client();
static void kill_client_now(Window w);
static void last_desktop();
static void last_win();
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
static void plugnplay(XEvent *e);
static void pop_window();
static void prev_win();
static void propertynotify(XEvent *e);
static void quit();
static void remove_client(client *cl, unsigned int dr);
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
static void sticky_win(const Arg arg);
static void swap_master();
static void switch_mode(const Arg arg);
static void terminate(const Arg arg);
static void tile();
static void toggle_bar();
static void unmapbar();
static void unmapnotify(XEvent *e);    // Thunderbird's write window just unmaps...
static void unsticky_win();
static void update_bar();
static void update_config();
static void update_current();
static void update_output(unsigned int messg);
static void warp_pointer();
static unsigned int wc_size(char *string);
static int get_value();

// Include configuration file
#include "config.h"

// Variable
static Display *dis;
static unsigned int attachaside, bdw, bool_quit, clicktofocus, current_desktop, doresize, dowarp, cstack;
static unsigned int screen, followmouse, mode, msize, previous_desktop, DESKTOPS, STATUS_BAR, numwins, numorder;
static unsigned int auto_mode, auto_num, shutting_down, default_desk, numstuck;
static int num_screens, growth, sh, sw, master_size, nmaster, randr_ev;
static unsigned int sb_desks;        // width of the desktop switcher
static unsigned int sb_height, sb_width, screen, show_bar, has_bar, wnamebg, barmon, barmonchange, lessbar;
static unsigned int showopen;        // whether the desktop switcher shows number of open windows
static unsigned int topbar, top_stack, windownamelength, keycount, cmdcount, dtcount, pcount, tcount, LA_WINDOWNAME;
static unsigned int ug_out, ug_in, ug_bar;
static int ufalpha, baralpha;
static unsigned long opacity, baropacity;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int numlockmask, resizemovekey;        /* dynamic key lock mask */
static Window root;
static Window sb_area;
static client *head, *current, *focus;
static char font_list[256], buffer[256], dummy[256];
static char RC_FILE[100], KEY_FILE[100], APPS_FILE[100];
static char winname[101];
static Atom alphaatom, wm_delete_window, protos, *protocols, dockatom, typeatom;
static XWindowAttributes attr;
static XButtonEvent starter;
static Arg barrtclkarg;

// Desktop array
static desktop desktops[12];
static Stuckd stuck[12];
static MonitorView view[5];
static Barwin sb_bar[12];
static Theme theme[10];
static Iammanyfonts font;
static key keys[80];
static Commands cmds[50];
static Convenience convenience[40];
static Positional positional[40];
static Popped popped[40];
#include "bar.c"
#include "readrc.c"
#include "readkeysapps.c"

#include "events.c"
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
    [ConfigureRequest] = configurerequest
};

/* ***************************** Window Management ******************************* */
void add_window(Window win, int tw, client *cl, int x, int y, int w, int h) {
    client *c,*t;

    if(cl != NULL) c = cl;
    else {
        if(!(c = (client *)calloc(1,sizeof(client)))) {
            logger("\033[0;31mError calloc!");
            exit(1);
        }
        c->x = x;
        if(topbar == 0 && y < sb_height+4+bdw+ug_bar) c->y = sb_height+4+bdw+ug_bar;
        else c->y = y;
        c->w = w;
        c->h = h;
    }

    if(head != NULL)
        for(t=head;t;t=t->next)
            ++t->order;

    c->win = win; c->order = 0;
    c->trans = tw;
    if(head == NULL) {
        c->next = NULL; c->prev = NULL;
        head = c;
    } else {
        if(attachaside == 0) {
            if(top_stack == 0) {
                c->next = head->next; c->prev = head;
                head->next = c;
            } else {
                for(t=head;t->next;t=t->next); // Start at the last in the stack
                t->next = c; c->next = NULL;
                c->prev = t;
            }
        } else {
            c->prev = NULL; c->next = head;
            c->next->prev = c;
            head = c;
        }
    }

    focus = c;
    ++numorder;
    if(c->trans == 0) {
        numwins += 1;
        current = c;
    }
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

void remove_client(client *cl, unsigned int dr) {
    client *t;

    if(cl->prev == NULL && cl->next == NULL) {
        head = NULL;
    } else if(cl->prev == NULL) {
        head = cl->next;
        cl->next->prev = NULL;
    } else if(cl->next == NULL) {
        cl->prev->next = NULL;
    } else {
        cl->prev->next = cl->next;
        cl->next->prev = cl->prev;
    }
    
    if(cl->trans == 0) numwins -= 1;
    numorder -= 1;
    if(head != NULL) {
        for(t=head;t;t=t->next) {
            if(t->order > cl->order) --t->order;
            if(t->order == 0) {
                focus = t;
                if(t->trans == 0) current = t;
            }
        }
        if(cl == current && numwins > 0) {
            for(i=0;i<numorder;++i) {
                for(t=head;t;t=t->next)
                    if(t->order == i && t->trans == 0) {
                        current = t;
                        break;
                    }
                if(current == t) break;
            }
        }
    } else focus = current = NULL;
    if(numwins == 0) current = NULL;
    if(dr == 0) free(cl);
    if((numwins - nmaster) < 3) growth = 0;
    save_desktop(current_desktop);
    return;
}

void next_win() {
    if(numorder < 2) return;

    client *c = focus;

    focus = (focus->next == NULL) ? head : focus->next;
    if(focus->trans == 0) current = focus;

    save_desktop(current_desktop);
    if(mode == 1) {
        tile();
        if(c->trans == 0 && focus->trans == 0) XUnmapWindow(dis, c->win);
    }
    update_current();
}

void prev_win() {
    if(numorder < 2) return;

    client *d = focus;
    client *c;

    if(focus->prev == NULL) {
        for(c=head;c->next;c=c->next);
        focus = c;
    } else focus = focus->prev;
    if(focus->trans == 0) current = focus;

    save_desktop(current_desktop);
    if(mode == 1) {
        tile();
        if(d->trans == 0 && focus->trans == 0) XUnmapWindow(dis, d->win);
    }
    update_current();
}

void last_win() {
    client *c;
    for(c=head;c;c=c->next)
        if(c->order == 1) {
            if(c->trans == 0) focus = current = c;
            else focus = c;
            update_current();
            return;
        }
}

void move_down(const Arg arg) {
    if(focus != NULL && (mode == 4 || focus->trans == 1)) {
        focus->y += arg.i;
        XMoveResizeWindow(dis,focus->win,desktops[current_desktop].x+focus->x,focus->y,focus->w,focus->h);
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
    if(focus != NULL && (mode == 4 || focus->trans == 1)) {
        focus->y += arg.i;
        XMoveResizeWindow(dis,focus->win,desktops[current_desktop].x+focus->x,focus->y,focus->w,focus->h);
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
    if(focus != NULL && (mode == 4 || focus->trans == 1)) {
        focus->x += arg.i;
        XMoveResizeWindow(dis,focus->win,desktops[current_desktop].x+focus->x,focus->y,focus->w,focus->h);
    }
}

void move_right(const Arg arg) {
    move_left(arg);
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
        current = focus = head;
    }
    save_desktop(current_desktop);
    tile();
    update_current();
}

void pop_window() {
    if(focus == NULL || mode == 4) return;
    client *c;
    if(focus->trans == 0) {
        focus->trans = 1;
        XMoveResizeWindow(dis, focus->win, desktops[current_desktop].x+focus->x, focus->y, focus->w, focus->h);
        numwins -= 1;
        if((numwins - nmaster) < 3) growth = 0;
        if(focus == current) {
            for(i=0;i<numorder;++i) {
                for(c=head;c;c=c->next)
                    if(c->trans == 0 && c->order == i) {
                        current = c;
                        break;
                    }
                if(current != focus) break;
            }
        }
        tile();
    } else {
        Window win = (current == NULL) ? 0:current->win;
        focus->trans = 0;
        numwins += 1;
        current = focus;
        tile();
        if(mode == 1 && numwins > 1) XUnmapWindow(dis, win);
    }
    update_current();
}

void sticky_win(const Arg arg) {
    if((arg.i-1) == current_desktop || focus == NULL) return;
    unsigned int tmp = current_desktop, i, j, stickied = 0;

    client *t, *c = focus;
    select_desktop(arg.i - 1);
    for(t=head;t;t=t->next)
        if(t->win == c->win) stickied = 1;
    if(stickied == 1) {
        select_desktop(tmp);
        return;
    } else {
        stickied = 0;
        for(i=0;i<numstuck;++i)
            if(c->win == stuck[i].win) {
                stickied = 1;
                stuck[i].numstuck += 1;
            }
        if(stickied == 0) {
            stuck[numstuck].win = c->win;
            stuck[numstuck].numstuck = 1;
            numstuck += 1;
        }
        add_window(c->win, c->trans, NULL, c->x, c->y, c->w, c->h);
        for(j=0;j<num_screens;++j)
            if(current_desktop == view[j].cd)
                if(mode != 4) tile();
        select_desktop(tmp);
        if(STATUS_BAR == 0) update_bar();
    }
}

void unsticky_win() {
    if(focus == NULL) return;
    unsigned int i, j, stickied = 0;
    for(i=0;i<numstuck;++i) {
        if(stuck[i].win == focus->win) {
            stuck[i].numstuck -= 1;
            if(stuck[i].numstuck == 0) {
                for(j=0;j+i<numstuck-1;++j) {
                    stuck[j].win = stuck[j+1].win;
                    stuck[j].numstuck = stuck[j+1].numstuck;
                }
                numstuck -= 1;
            }
            stickied = 1;
        }
    }
    if(stickied == 0) return;
    client *c = focus;
    XUnmapWindow(dis, c->win);
    remove_client(c, 0);
    if(focus != NULL) {
        if(mode != 4) tile();
        update_current();
    }
    if(STATUS_BAR == 0) update_bar();
}

/* **************************** Desktop Management ************************************* */

void change_desktop(const Arg arg) {
    if(arg.i >= DESKTOPS || arg.i == current_desktop) return;
    client *c;

    int next_view = desktops[arg.i].screen;
    if(next_view != desktops[current_desktop].screen && dowarp == 0) {
        XWarpPointer(dis, None, root, 0, 0, 0, 0,
          desktops[arg.i].x+(desktops[arg.i].w/2),
            desktops[arg.i].y+(desktops[arg.i].h/2));
    }

    // Save current "properties"
    save_desktop(current_desktop);
    previous_desktop = current_desktop;
    if(head != NULL) {
        XSetWindowBorder(dis,focus->win,theme[1].wincolor);
        if(clicktofocus == 0)
            XGrabButton(dis, AnyButton, AnyModifier, focus->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
    }

    if(arg.i != view[next_view].cd) {
        select_desktop(view[next_view].cd);
        // Unmap all window
        if(head != NULL)
            for(c=head;c;c=c->next)
                XUnmapWindow(dis,c->win);
    }

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(head != NULL) {
        for(c=head;c;c=c->next)
            if(c->trans == 1) {
                XMoveResizeWindow(dis,c->win,desktops[current_desktop].x+c->x,desktops[current_desktop].y+c->y,c->w,c->h);
                XMapWindow(dis,c->win);
            } else if(mode != 1) XMapWindow(dis,c->win);
        tile();
    }

    if(next_view == barmon && has_bar == 1 && show_bar == 0) mapbar();
    if(next_view == barmon && has_bar == 0 && show_bar == 1) unmapbar();

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
    if(focus == NULL || arg.i == current_desktop || arg.i >= DESKTOPS) return;
    client_to_desktop(arg);
    change_desktop(arg);
}

void client_to_desktop(const Arg arg) {
    if(focus == NULL || arg.i == current_desktop || arg.i >= DESKTOPS) return;

    client *c = focus;
    unsigned int tmp2 = current_desktop, j, cd = desktops[current_desktop].screen;
    unsigned int stickied = 0;

    select_desktop(arg.i);
    for(j=0;j<numstuck;++j)
        if(c->win == stuck[j].win)
            stickied = 1;
    select_desktop(tmp2);

    // Remove client from current desktop
    if(stickied == 1) unsticky_win();
    else {
        XUnmapWindow(dis, c->win);
        remove_client(c, 1);
        if(mode != 4) tile();

        // Add client to desktop
        select_desktop(arg.i);
        add_window(c->win, c->trans, c, c->x, c->y, c->w, c->h);
        save_desktop(arg.i);

        if(c->trans == 1) XMoveResizeWindow(dis, c->win,
          desktops[current_desktop].x+c->x,c->y,c->w,c->h);
        for(j=cd;j<cd+num_screens;++j) {
            if(view[j%num_screens].cd == arg.i) {
                if(c->trans == 0) tile();
                XMapWindow(dis, c->win);
            }
        }
        select_desktop(tmp2);
        update_current();

        if(STATUS_BAR == 0) update_bar();
    }
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].nmaster = nmaster;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].showbar = show_bar;
    desktops[i].head = head;
    desktops[i].current = current;
    desktops[i].focus = focus;
    desktops[i].numwins = numwins;
    desktops[i].numorder = numorder;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    nmaster = desktops[i].nmaster;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    show_bar = desktops[i].showbar;
    head = desktops[i].head;
    current = desktops[i].current;
    focus = desktops[i].focus;
    numwins = desktops[i].numwins;
    numorder = desktops[i].numorder;
    current_desktop = i;
    sw = desktops[i].w;
    sh = desktops[i].h;
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
    if(head == NULL || current == NULL || (head->next == NULL && head->trans == 1)) return;
    client *c, *d=NULL;
    unsigned int x = 0, xpos = 0, ypos=0, wdt = 0, msw, ssw, ncols = 2, nrows = 1;
    int ht = 0, y, n = 0, nm = (numwins < 3) ? 0: (numwins-2 < nmaster) ? (numwins-2):nmaster;
    int scrx = desktops[current_desktop].x;
    int scry = desktops[current_desktop].y;

    // For a top bar
    y = (STATUS_BAR == 0 && topbar == 0 && show_bar == 0) ? sb_height+4+ug_bar : 0; ypos = y;

    // If only one window
    if(numwins == 1 && mode != 4 && mode != 1) {
        for(c=head;c;c=c->next)
            if(c->trans == 0) {
                XMoveResizeWindow(dis,c->win,scrx+ug_out,scry+y+ug_out,sw+bdw-2*ug_out,sh+bdw-2*ug_out);
                if(mode == 1) XMapWindow(dis, c->win);
            }
    } else {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
            	scrx += ug_out; scry += ug_out;
            	sw = desktops[current_desktop].w-2*ug_out; sh = desktops[current_desktop].h-2*ug_out;
                for(d=head;d;d=d->next) {
                    if(d->trans == 1) continue;
                    XMoveResizeWindow(dis,d->win,scrx,scry+ypos,master_size-bdw-(ug_in/2),sh/(nm+1)-bdw-(nm*(ug_in/2)));
                    if(x == nm) break;
                    ypos += sh/(nm+1)+(ug_in/2); ++x;
                }

                // Stack
                if(d == NULL) d = head;
                n = numwins - (nm+1);
                for(c=d->next;c;c=c->next)
                    if(c->trans == 0) break;
                XMoveResizeWindow(dis,c->win,scrx+master_size+ug_in,scry+y,sw-master_size-bdw-ug_in,(sh/n)+growth-bdw-((n-1)*(ug_in/2)));
                y += (sh/n)+growth+(ug_in/2);
                for(d=c->next;d;d=d->next) {
                    if(d->trans == 1) continue;
                    XMoveResizeWindow(dis,d->win,scrx+master_size+ug_in,scry+y,sw-master_size-bdw-ug_in,(sh/n)-(growth/(n-1))-bdw-((n-1)*(ug_in/2)));
                    y += (sh/n)-(growth/(n-1))+(ug_in/2);
                }
                break;
            case 1: /* Fullscreen */
                XMoveResizeWindow(dis,current->win,scrx,scry+y,sw+bdw,sh+bdw);
                XMapWindow(dis, current->win);
                break;
            case 2: /* Horizontal */
            	// Master window
            	scrx += ug_out; scry += ug_out;
            	sw = desktops[current_desktop].w-2*ug_out; sh = desktops[current_desktop].h-2*ug_out;
                for(d=head;d;d=d->next) {
                    if(d->trans == 1) continue;
                    XMoveResizeWindow(dis,d->win,scrx+xpos,scry+ypos,sw/(nm+1)-bdw-(nm*(ug_in/2)),master_size-bdw-ug_in/2);
                    xpos += sw/(nm+1)+(ug_in/2);
                    if(x == nm) break; ++x;
                }

                // Stack
                if(d == NULL) d = head;
                n = numwins - (nm+1); y += ug_in/2;
                for(c=d->next;c;c=c->next)
                    if(c->trans == 0) break;
                XMoveResizeWindow(dis,c->win,scrx,scry+y+master_size,(sw/n)+growth-bdw-ug_in/2,sh-master_size-bdw-ug_in/2);
                msw = (sw/n)+growth+(ug_in/2);
                for(d=c->next;d;d=d->next) {
                    if(d->trans == 1) continue;
                    XMoveResizeWindow(dis,d->win,scrx+msw,scry+y+master_size,(sw/n)-(growth/(n-1))-bdw-((n-1)*(ug_in/2)),sh-master_size-bdw-ug_in/2);
                    msw += (sw/n)-(growth/(n-1));
                }
                break;
            case 3: { // Grid
            	scrx += ug_out; scry += ug_out;
            	sw = desktops[current_desktop].w-2*ug_out; sh = desktops[current_desktop].h-2*ug_out;
                if(numwins < 5) {
                    for(c=head;c;c=c->next) {
                        if(c->trans == 1) continue;
                        ++n;
                        if(numwins > 2) {
                            if((n == 1) || (n == 2))
                                ht = (sh/2)+growth-bdw-ug_in/2;
                            if(n > 2)
                                ht = (sh/2)-growth-bdw-ug_in/2;
                        } else ht = sh - bdw;
                        if((n == 1) || (n == 3)) {
                            xpos = 0;
                            wdt = master_size-bdw-ug_in/2;
                        }
                        if((n == 2) || (n == 4)) {
                            xpos = master_size+ug_in/2;
                            wdt = (sw-master_size)-bdw-ug_in/2;
                        }
                        if(n == 3)
                            ypos += (sh/2)+growth+ug_in/2;
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
                        if(d->trans == 1) continue;
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
                    for(c=head;c;c=c->next) {
                        if(c->trans == 1) continue;
                        XMoveResizeWindow(dis,c->win,scrx+c->x,scry+c->y,c->w,c->h);
                    }
                break;
            }
            default:
                break;
        }
    }
}

void update_current() {
    if(head == NULL) return;

    client *c; unsigned int border, i;

    border = ((numwins == 1 && mode != 4) || (mode == 1)) ? 0 : bdw;
    for(c=head;c;c=c->next) {
        XSetWindowBorderWidth(dis,c->win,border);
        if(c != focus) {
            if(c->order <= focus->order) ++c->order;
            if(ufalpha < 100) XChangeProperty(dis, c->win, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &opacity, 1l);
            XSetWindowBorder(dis,c->win,theme[1].wincolor);
            if(clicktofocus == 0)
                XGrabButton(dis, AnyButton, AnyModifier, c->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
    }
    // "Enable" current window
    focus->order = 0;
    if(ufalpha < 100) XDeleteProperty(dis, focus->win, alphaatom);
    XSetWindowBorder(dis,focus->win,theme[0].wincolor);
    XSetInputFocus(dis,focus->win,RevertToParent,CurrentTime);
    if(clicktofocus == 0)
        XUngrabButton(dis, AnyButton, AnyModifier, focus->win);
    XRaiseWindow(dis,focus->win);

    if(numorder != numwins)
        for(i=numorder;i>0;--i)
            for(c=head;c;c=c->next) {
                if(c->order != i-1) continue;
                if(c->trans == 1) {
                    if(mode == 1 || numwins == 1) XSetWindowBorderWidth(dis,c->win,bdw);
                    if(mode != 4) XRaiseWindow(dis,c->win);
                }
            }

    if(STATUS_BAR == 0) getwindowname(focus->win, 0);
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
        for(c=head;c;c=c->next) {
            if(c->trans == 1) continue;
            XUnmapWindow(dis, c->win);
        }

    save_desktop(current_desktop);
    tile();
    update_current();
    if(STATUS_BAR == 0) status_text("");
}

void resize_master(const Arg arg) {
    if(focus != NULL && (mode == 4 || focus->trans == 1)) {
        focus->w = (focus->w + arg.i > 50) ? focus->w + arg.i: 50;
        XMoveResizeWindow(dis,focus->win,desktops[current_desktop].x+focus->x,
          focus->y,focus->w,focus->h);
    } else if(mode == 1 || numwins < 2) return;
    else {
        if(arg.i > 0) {
            if((mode != 2 && sw-master_size > 70) || (mode == 2 && sh-master_size > 70))
                master_size += arg.i;
        } else if(master_size > 70) master_size += arg.i;
        tile();
    }
}

void resize_stack(const Arg arg) {
    if(focus != NULL && (mode == 4 || focus->trans == 1)) {
        focus->h = (focus->h + arg.i > 50) ? focus->h + arg.i: 50;
        XMoveResizeWindow(dis,focus->win,desktops[current_desktop].x+focus->x,
          focus->y,focus->w,focus->h);
    }
    if((numwins - nmaster) < 3) return;
    if(mode == 3) {
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

    // For each shortcut
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

void warp_pointer() {
    // Move cursor to the center of the current window
    if(followmouse != 0) return;
    if(dowarp < 1 && current != NULL) {
        XGetWindowAttributes(dis, current->win, &attr);
        XWarpPointer(dis, None, current->win, 0, 0, 0, 0, attr.width/2, attr.height/2);
        return;
    }
}

/* ********************** Signal Management ************************** */
int check_dock(Window w) {
    unsigned long count, j, extra;
    Atom realType;
    int realFormat, ret = 1;
    unsigned char *temp;
    Atom *type;

    if(XGetWindowProperty(dis, w, typeatom, 0, 32,
       False, XA_ATOM, &realType, &realFormat, &count, &extra,
        &temp) == Success) {
        if(count > 0) {
            type = (unsigned long*)temp;
            for(j=0; j<count; j++)
                if(type[j] == dockatom) ret = 0;
        }
        XFree(temp);
    }
    return ret;
}

void kill_client() {
    if(head == NULL) return;
    Window w = focus->win;
    kill_client_now(w);
    if(w) return;
    remove_client(focus, 0);
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
    unsigned int i, j;

    for(i=0;i<DESKTOPS;++i) {
        if(desktops[i].head != NULL) select_desktop(i);
        else continue;
        for(j=0;j<numorder;++j)
            kill_client();
    }
    XClearWindow(dis, root);
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    for(i=0;i<10;++i)
        XFreeGC(dis, theme[i].gc);
    XFreePixmap(dis, area_sb);
    XSync(dis, False);
    XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);
    logger("\033[0;34mYou Quit : Bye!");
    if(shutting_down > 0) return;
    else bool_quit = 1;
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(XAllocNamedColor(dis,map,color,&c,&c)) return c.pixel;
    else {
        logger("\033[0;31mError parsing color!");
        return 1;
    }
    
}

void terminate(const Arg arg) {
    unsigned int i, j=0;
    char *search, *msg;
    Arg a;
    
    shutting_down = 1;
    quit();
    if(arg.i == 1) {
        search = "shutdowncmd";
        msg = "SHUTTING DOWN";
    } else {
        search = "rebootcmd";
        msg = "REBOOTING";
    }
    for(i=0;i<cmdcount;++i) {
        if(strcmp(search, cmds[i].name) == 0) {
            while(strncmp(cmds[i].list[j], "NULL", 4) != 0) {
                a.com[j] = cmds[i].list[j];
                ++j;
            }
            a.com[j] = NULL;
            logger(msg);
            bool_quit = 1;
            spawn(a);
            //execvp((char*)a.com[0],(char**)a.com);
        }
    }
}

void logger(const char* e) {
    fprintf(stderr,"\n\033[0;34m:: snapwm : %s \033[0;m\n", e);
    fflush(stderr);
}

void plugnplay(XEvent *e) {
    client *c; unsigned int tmp = current_desktop;
    XRRUpdateConfiguration(e);
    if(font.fontset) XFreeFontSet(dis, font.fontset);
    memset(font_list, '\0', 256);
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    for(i=0;i<10;++i)
        XFreeGC(dis, theme[i].gc);
            if(i < 5) XFreeGC(dis, theme[i].swgc);
    XFreePixmap(dis, area_sb);
    for(i=0;i<num_screens;++i) {
        select_desktop(view[i].cd);
        for(c=head;c;c=c->next) XUnmapWindow(dis, c->win);
    }
    select_desktop(tmp);
    unmapbar();
    XSetInputFocus(dis, root, RevertToPointerRoot, CurrentTime);
    XSync(dis, False);
    XCloseDisplay(dis);
    dis = XOpenDisplay(NULL);
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);
    read_rcfile();
    init_desks();
    if(STATUS_BAR == 0) {
        setup_status_bar();
        status_bar();
        if(show_bar > 0) unmapbar();
    }
    grabkeys();
    init_start();
    for(i=0;i<num_screens;++i) {
        select_desktop(view[i].cd);
        for(c=head;c;c=c->next) XMapWindow(dis, c->win);
        tile();
    }
    Arg a = {.i = tmp};
    change_desktop(a);
}

void init_start() {
    alphaatom = XInternAtom(dis, "_NET_WM_WINDOW_OPACITY", False);
    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", False);
    protos = XInternAtom(dis, "WM_PROTOCOLS", False);
    typeatom = XInternAtom(dis, "_NET_WM_WINDOW_TYPE", False);
    dockatom = XInternAtom(dis, "_NET_WM_WINDOW_TYPE_DOCK", False);
    // To catch maprequest and destroynotify (if other wm running)
    XSetWindowAttributes at;
    at.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|PropertyChangeMask;
    XChangeWindowAttributes(dis,root,CWEventMask,&at);
    XRRSelectInput(dis,root,RRScreenChangeNotifyMask);
    int err;
    XRRQueryExtension(dis,&randr_ev,&err);
}

void init_desks() {
    int last_width=0, i, j, have_Xin = 0;

    XineramaScreenInfo *info = NULL;
    if(!(info = XineramaQueryScreens(dis, &num_screens))) {
        logger("XINERAMA Fail");
        num_screens = 1;
        have_Xin = 1;
    }
    //fprintf(stderr, "Number of screens is %d -- HAVE_XIN = %d\n", num_screens, have_Xin);

    if(barmonchange >= 0 && barmonchange < num_screens)
        barmon = barmonchange;
    else barmon = (num_screens - 1);
    for (i = 0; i < num_screens; ++i) {
        for(j=i;j<DESKTOPS;j+=num_screens) {
            if(i == barmon && desktops[j].showbar != 1 && STATUS_BAR == 0) {
                desktops[j].h = ((have_Xin == 0) ? info[i].height:XDisplayHeight(dis, screen)) - (sb_height+4+bdw+ug_bar);
            } else {
                desktops[j].h = ((have_Xin == 0) ? info[i].height:XDisplayHeight(dis, screen)) - bdw;
                desktops[j].showbar = 1;
            }
            if(!(desktops[j].mode)) desktops[j].mode = mode;
            if(!(desktops[j].nmaster)) desktops[j].nmaster = nmaster;
            //fprintf(stderr, "**screen is %d - desktop is %d **\n", i, j);
            desktops[j].x = (have_Xin == 0) ? info[i].x_org + last_width:0;
            desktops[j].y = (have_Xin == 0) ? info[i].y_org:0;
            desktops[j].w = (have_Xin == 0) ? info[i].width - bdw:XDisplayWidth(dis, screen);
            //fprintf(stderr, " x=%d - y=%d - w=%d - h=%d \n", desktops[j].x, desktops[j].y, desktops[j].w, desktops[j].h);
            desktops[j].master_size = (desktops[j].mode == 2) ? (desktops[j].h*msize)/100 : (desktops[j].w*msize)/100;
            if(!(desktops[j].growth)) desktops[j].growth = 0;
            if(!(desktops[j].numwins)) desktops[j].numwins = 0;
            if(!(desktops[j].numorder)) desktops[j].numorder = 0;
            if(!(desktops[j].head)) desktops[j].head = NULL;
            if(!(desktops[j].current)) desktops[j].current = NULL;
            if(!(desktops[j].focus)) desktops[j].focus = NULL;
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

    // Initialize variables
    DESKTOPS = 4;
    topbar = followmouse = top_stack = mode = cstack = default_desk = 0;
    LA_WINDOWNAME = wnamebg = dowarp = doresize = nmaster = has_bar = 0;
    auto_mode = auto_num = shutting_down = numstuck = 0;
    msize = 55;
    ufalpha = 75; baralpha = 90;
    bdw = 2;
    showopen = clicktofocus = attachaside = 1;
    resizemovekey = Mod1Mask;
    windownamelength = 35;
    show_bar = STATUS_BAR = barmon = barmonchange = lessbar = 0;
    ug_out = ug_in = ug_bar = 0;

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
        if(show_bar > 0) unmapbar();
    }
    read_apps_file();

    // Shortcuts
    grabkeys();

    // For exiting
    bool_quit = 0;

    // Select default desktop
    select_desktop(0);
    if(default_desk > 0) {
        Arg a = {.i = default_desk};
        change_desktop(a);
    }
    init_start();
    update_current();
    setbaralpha();

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


void start() {
    XEvent ev;

    while(!bool_quit && !XNextEvent(dis,&ev)) {
        if(ev.type == randr_ev) plugnplay(&ev);
        else if(events[ev.type])
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
