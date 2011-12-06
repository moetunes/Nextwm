/* snapwm.c [ 0.0.7 ]
*
*  I started this from catwm 31/12/10
*  Bad window error checking and numlock checking used from
*  2wm at http://hg.suckless.org/2wm/
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    int mode;
    int growth;
    client *head;
    client *current;
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
} Theme;

typedef enum {
    ATOM_NET_WM_WINDOW_TYPE,
    ATOM_NET_WM_WINDOW_TYPE_UTILITY,
    ATOM_NET_WM_WINDOW_TYPE_DOCK,
    ATOM_NET_WM_WINDOW_TYPE_SPLASH,
    ATOM_NET_WM_WINDOW_TYPE_DIALOG,
    ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION,
    ATOM_COUNT
} AtomType;

typedef struct {
   Atom *atom;
   const char *name;
} AtomNode;

Atom atoms[ATOM_COUNT];

static const AtomNode atomList[] = {
    { &atoms[ATOM_NET_WM_WINDOW_TYPE],              "_NET_WM_WINDOW_TYPE"             },
    { &atoms[ATOM_NET_WM_WINDOW_TYPE_UTILITY],      "_NET_WM_WINDOW_TYPE_UTILITY"     },
    { &atoms[ATOM_NET_WM_WINDOW_TYPE_DOCK],         "_NET_WM_WINDOW_TYPE_DOCK"        },
    { &atoms[ATOM_NET_WM_WINDOW_TYPE_SPLASH],       "_NET_WM_WINDOW_TYPE_SPLASH"      },
    { &atoms[ATOM_NET_WM_WINDOW_TYPE_DIALOG],       "_NET_WM_WINDOW_TYPE_DIALOG"      },
    { &atoms[ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION], "_NET_WM_WINDOW_TYPE_NOTIFICATION"},
};

// Functions
static void add_window(Window w);
static void buttonpressed(XEvent *e);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void enternotify(XEvent *e);
static unsigned long getcolor(const char* color);
static void getwindowname();
static void grabkeys();
static void keypress(XEvent *e);
static void kill_client();
static void last_desktop();
static void leavenotify(XEvent *e);
static void logger(const char* e);
static void maprequest(XEvent *e);
static void move_down();
static void move_up();
static void next_win();
static void prev_win();
static void propertynotify(XEvent *e);
static void quit();
static void remove_window(Window w);
static void read_rcfile();
static void resize_master(const Arg arg);
static void resize_stack(const Arg arg);
static void rotate_desktop(const Arg arg);
static void save_desktop(int i);
static void select_desktop(int i);
static void send_kill_signal(Window w);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();
static void status_bar();
static void status_text(const char* sb_text);
static void swap_master();
static void tile();
static void toggle_bar();
static void switch_fullscreen();
static void switch_grid();
static void switch_horizontal();
static void switch_vertical();
static void update_bar();
static void update_config();
static void update_current();

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static int bool_quit;
static int current_desktop;
static int previous_desktop;
static int growth;
static int master_size;
static int mode;
static int sb_desks;        // width of the desktop switcher
static int sb_height;
static int sb_width;
static int sh;
static int sw;
static int screen;
static int show_bar;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);
//static unsigned int colors[0].color, colors[1].color, colors[2].color, colors[3].color, colors[4].color;
unsigned int numlockmask;		/* dynamic key lock mask */
static Window root;
static Window sb_area;
static client *head;
static client *current;
static char fontname[80];
static XFontStruct *font;
static GC sb_b, sb_c;
// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [EnterNotify] = enternotify,
    [LeaveNotify] = leavenotify,
    [ButtonPress] = buttonpressed,
    [DestroyNotify] = destroynotify,
    [PropertyNotify] = propertynotify,
    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest
};

// Desktop array
static desktop desktops[DESKTOPS];
static Barwin sb_bar[DESKTOPS];
static Theme colors[5];

/* ***************************** Window Management ******************************* */
void add_window(Window w) {
    client *c,*t;

    if(!(c = (client *)calloc(1,sizeof(client)))) {
        logger("\033[0;31mError calloc!");
        exit(1);
    }

    if(head == NULL) {
        c->next = NULL;
        c->prev = NULL;
        c->win = w;
        head = c;
    }
    else {
        if(ATTACH_ASIDE == 0) {
            for(t=head;t->next;t=t->next);

            c->next = NULL;
            c->prev = t;
            c->win = w;

            t->next = c;
        }
        else {
            for(t=head;t->prev;t=t->prev);

            c->prev = NULL;
            c->next = t;
            c->win = w;

            t->prev = c;

            head = c;
        }
    }

    current = c;
    save_desktop(current_desktop);
    // for folow mouse and statusbar updates
    if(FOLLOW_MOUSE == 0 && show_bar == 0)
        XSelectInput(dis, c->win, EnterWindowMask|PropertyChangeMask);
    else if(FOLLOW_MOUSE == 0)
        XSelectInput(dis, c->win, EnterWindowMask);
    else if(show_bar == 0)
        XSelectInput(dis, c->win, PropertyChangeMask);
}

void remove_window(Window w) {
    client *c;

    // CHANGE THIS UGLY CODE
    for(c=head;c;c=c->next) {

        if(c->win == w) {
            if(c->prev == NULL && c->next == NULL) {
                free(head);
                head = NULL;
                current = NULL;
                save_desktop(current_desktop);
                status_text("");
                return;
            }

            if(c->prev == NULL) {
                head = c->next;
                c->next->prev = NULL;
                current = c->next;
            }
            else if(c->next == NULL) {
                c->prev->next = NULL;
                current = c->prev;
            }
            else {
                c->prev->next = c->next;
                c->next->prev = c->prev;
                current = c->prev;
            }

            free(c);
            save_desktop(current_desktop);
            tile();
            update_current();
            return;
        }
    }
}

void kill_client() {
    if(current != NULL) {
        //send delete signal to window
        XEvent ke;
        ke.type = ClientMessage;
        ke.xclient.window = current->win;
        ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
        ke.xclient.format = 32;
        ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
        ke.xclient.data.l[1] = CurrentTime;
        XSendEvent(dis, current->win, False, NoEventMask, &ke);
        send_kill_signal(current->win);
        remove_window(current->win);
	}
}

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        if(mode == 1)
            tile();
        update_current();
    }
}

void prev_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->prev == NULL)
            for(c=head;c->next;c=c->next);
        else
            c = current->prev;

        current = c;
        if(mode == 1)
            tile();
        update_current();
    }
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
    if(head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis,c->win);

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(head != NULL)
        for(c=head;c;c=c->next)
            XMapWindow(dis,c->win);
    else
        status_text("");

    tile();
    update_current();
    update_bar();
}

void last_desktop() {
    Arg a = {.i = previous_desktop};
    change_desktop(a);
}

void rotate_desktop(const Arg arg) {
    int ndesktops = TABLENGTH(desktops);
    Arg a = {.i = (current_desktop + ndesktops + arg.i) % ndesktops};
     change_desktop(a);
}

void client_to_desktop(const Arg arg) {
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    XUnmapWindow(dis,tmp->win);
    remove_window(tmp->win);
    save_desktop(tmp2);
    tile();
    update_current();
    if(FOLLOW_WINDOW == 0)
        change_desktop(arg);
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].head = head;
    desktops[i].current = current;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    head = desktops[i].head;
    current = desktops[i].current;
    current_desktop = i;
}

void tile() {
    client *c;
    int n = 0;
    int x = 0;
    int y = 0;

    // For a top panel
    if(TOP_PANEL == 0)
        y = PANEL_HEIGHT;

    // If only one window
    if(head != NULL && head->next == NULL) {
        XMoveResizeWindow(dis,head->win,0,y,sw+BORDER_WIDTH,sh+BORDER_WIDTH);
    }
    else if(head != NULL) {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,master_size - BORDER_WIDTH,sh - BORDER_WIDTH);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                if(n == 1) growth = 0;
                XMoveResizeWindow(dis,head->next->win,master_size + BORDER_WIDTH,y,sw-master_size-(2*BORDER_WIDTH),(sh/n)+growth - BORDER_WIDTH);
                y += (sh/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,master_size + BORDER_WIDTH,y,sw-master_size-(2*BORDER_WIDTH),(sh/n)-(growth/(n-1)) - BORDER_WIDTH);
                    y += (sh/n)-(growth/(n-1));
                }
                break;
            case 1: /* Fullscreen */
                for(c=head;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,0,y,sw+2*BORDER_WIDTH,sh+2*BORDER_WIDTH);
                }
                break;
            case 2: /* Horizontal */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,sw-BORDER_WIDTH,master_size - BORDER_WIDTH);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                if(n == 1) growth = 0;
                XMoveResizeWindow(dis,head->next->win,0,y+master_size + BORDER_WIDTH,(sw/n)+growth-BORDER_WIDTH,sh-master_size-(2*BORDER_WIDTH));
                x = (sw/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,x,y+master_size + BORDER_WIDTH,(sw/n)-(growth/(n-1)) - BORDER_WIDTH,sh-master_size-(2*BORDER_WIDTH));
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
                        if((n == 1) || (n == 4) || (n == 7))
                            xpos = 0;
                        if((n == 2) || (n == 5) || (n == 8))
                            xpos = (sw/3) + BORDER_WIDTH;
                        if((n == 3) || (n == 6) || (n == 9))
                            xpos = (2*(sw/3)) + BORDER_WIDTH;
                        if((n == 4) || (n == 7))
                            y += (sh/3) + BORDER_WIDTH;
                        if((n == x) && (n == 7))
                            wdt = sw - BORDER_WIDTH;
                        if((n == x) && (n == 8))
                            wdt = 2*sw/3 - BORDER_WIDTH;
                    } else
                    if(x >= 5) {
                        wdt = (sw/3) - BORDER_WIDTH;
                        ht  = (sh/2) - BORDER_WIDTH;
                        if((n == 1) || (n == 4))
                            xpos = 0;
                        if((n == 2) || (n == 5))
                            xpos = (sw/3) + BORDER_WIDTH;
                        if((n == 3) || (n == 6))
                            xpos = (2*(sw/3)) + BORDER_WIDTH;
                        if(n == 4)
                            y += (sh/2); // + BORDER_WIDTH;
                        if((n == x) && (n == 5))
                            wdt = 2*sw/3 - BORDER_WIDTH;

                    } else {
                        if(x > 2) {
                            if((n == 1) || (n == 2))
                                ht = (sh/2) + growth - BORDER_WIDTH;
                            if(n >= 3)
                                ht = (sh/2) - growth - 2*BORDER_WIDTH;
                        }
                        else
                            ht = sh - BORDER_WIDTH;
                        if((n == 1) || (n == 3)) {
                            xpos = 0;
                            wdt = master_size - BORDER_WIDTH;
                        }
                        if((n == 2) || (n == 4)) {
                            xpos = master_size+BORDER_WIDTH;
                            wdt = (sw - master_size) - 2*BORDER_WIDTH;
                        }
                        if(n == 3)
                            y += (sh/2) + growth + BORDER_WIDTH;
                        if((n == x) && (n == 3))
                            wdt = sw - BORDER_WIDTH;
                    }
                    XMoveResizeWindow(dis,c->win,xpos,y,wdt,ht);
                }
            }
            break;
            default:
                break;
        }
    }
}

void update_current() {
    client *c;

    for(c=head;c;c=c->next) {
        if((head->next == NULL) || (mode == 1))
            XSetWindowBorderWidth(dis,c->win,0);
        else
            XSetWindowBorderWidth(dis,c->win,BORDER_WIDTH);

        if(current == c) {
            // "Enable" current window
            XSetWindowBorder(dis,c->win,colors[0].color);
            XSetInputFocus(dis,c->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,c->win);
            if(CLICK_TO_FOCUS == 0)
                XUngrabButton(dis, AnyButton, AnyModifier, c->win);
        }
        else {
            XSetWindowBorder(dis,c->win,colors[1].color);
            if(CLICK_TO_FOCUS == 0)
                XGrabButton(dis, AnyButton, AnyModifier, c->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
    }
    if(show_bar == 0 && head != NULL) {
        getwindowname();
    }
    XSync(dis, False);
}

void switch_vertical() {
    if(mode != 0) {
        mode = 0;
        master_size = sw * MASTER_SIZE;
	tile();
        update_current();
    }
}

void switch_fullscreen() {
    if(mode != 1) {
        mode = 1;
        tile();
        update_current();
    }
}

void switch_horizontal() {
    if(mode != 2) {
        mode = 2;
        master_size = sh * MASTER_SIZE;
        tile();
        update_current();
    }
}

void switch_grid() {
    if(mode != 3) {
        mode = 3;
        master_size = sw * MASTER_SIZE;
        tile();
        update_current();
    }
}

void resize_master(const Arg arg) {
        master_size += arg.i;
        tile();
}

void resize_stack(const Arg arg) {
    growth += arg.i;
    tile();
}

/* ************************** Status Bar *************************** */
void status_bar() {
    int i;
   	XGCValues values;

    logger(" \033[0;33mStatus Bar called ...");

   	/* create the sb_b GC to draw the font */
	values.foreground = colors[4].color;
	values.line_width = 2;
	values.line_style = LineSolid;
	values.font = font->fid;
	sb_b = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);

   	/* create the sb_c GC to draw the  */
	values.foreground = colors[3].color;
	values.line_width = 2;
	values.line_style = LineSolid;
	values.font = font->fid;
	sb_c = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);

    sb_width = 0;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].width = XTextWidth(font, sb_bar[i].label, strlen(sb_bar[i].label));
        if(sb_bar[i].width > sb_width)
            sb_width = sb_bar[i].width;
    }
    sb_width += 4;
    if(sb_width < 20) sb_width = 20;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].sb_win = XCreateSimpleWindow(dis, root, i*sb_width, sh+PANEL_HEIGHT+BORDER_WIDTH,
                                            sb_width-BORDER_WIDTH,sb_height-2*BORDER_WIDTH,BORDER_WIDTH,colors[3].color,colors[0].color);

        XSelectInput(dis, sb_bar[i].sb_win, ButtonPressMask|EnterWindowMask|LeaveWindowMask );

        XMapWindow(dis, sb_bar[i].sb_win);
    }
	
	sb_desks = (i*sb_width)+BORDER_WIDTH;
    sb_area = XCreateSimpleWindow(dis, root, sb_desks, sh+PANEL_HEIGHT+BORDER_WIDTH,
             sw-(sb_desks+BORDER_WIDTH),sb_height-2*BORDER_WIDTH,BORDER_WIDTH,colors[3].color,colors[1].color);

    XSelectInput(dis, sb_area, ButtonPressMask|EnterWindowMask|LeaveWindowMask );
    XMapWindow(dis, sb_area);

	status_text("");
	update_bar();
}

void toggle_bar() {
    int i;

    if(STATUS_BAR == 0) {
        if(show_bar == 1) {
            show_bar = 0;
            sh -= sb_height;
            for(i=0;i<DESKTOPS;i++) {
                XMapWindow(dis, sb_bar[i].sb_win);
                XMapWindow(dis, sb_area);
            }
        } else {
            show_bar = 1;
            sh += sb_height;
            for(i=0;i<DESKTOPS;i++) {
                XUnmapWindow(dis,sb_bar[i].sb_win);
                XUnmapWindow(dis, sb_area);
            }
        }

        tile();
        update_current();
        update_bar();
        getwindowname();
    }
}

void getwindowname() {
    char *win_name;

    if(head != NULL) {
        XFetchName(dis, current->win, &win_name);
        status_text(win_name);
        XFree(win_name);
    }
}

void status_text(const char *sb_text) {
	int text_length, text_start;

	if(sb_text == NULL) sb_text = "snapwm";
	if(head == NULL) sb_text = "snapwm";
	if(strlen(sb_text) >= 45)
	    text_length = 45;
	else
	    text_length = strlen(sb_text);
	text_start = (sb_width+(XTextWidth(font, sb_text, 45)))-(XTextWidth(font, sb_text, text_length));

	XClearWindow(dis, sb_area);
	XDrawString(dis, sb_area, sb_b, text_start, font->ascent+2, sb_text, text_length);
}

void update_bar() {
    int i;

    for(i=0;i<DESKTOPS;i++)
        if(i != current_desktop) {
            XSetWindowBackground(dis, sb_bar[i].sb_win, colors[1].color);
            XClearWindow(dis, sb_bar[i].sb_win);
            if(desktops[i].head != NULL)
                XDrawString(dis, sb_bar[i].sb_win, sb_b, (sb_width-XTextWidth(font, "#",1))/2, font->ascent+2, "#", 1);
            else
                XDrawString(dis, sb_bar[i].sb_win, sb_b, (sb_width-sb_bar[i].width)/2, font->ascent+2, sb_bar[i].label, sb_bar[i].width);
        } else {
            XSetWindowBackground(dis, sb_bar[i].sb_win, colors[0].color);
            XClearWindow(dis, sb_bar[i].sb_win);
            XDrawString(dis, sb_bar[i].sb_win, sb_b, (sb_width-sb_bar[i].width)/2, font->ascent+2, sb_bar[i].label, sb_bar[i].width);
        }
}

/* *********************** Read Config File ************************ */
void read_rcfile() {
    FILE *rcfile ; 
    char buffer[80]; /* Way bigger that neccessary */
    char dummy[50];
    char *dummy2;
    char *dummy3;
    int i;

    show_bar = STATUS_BAR;
    rcfile = fopen( RCFILE, "r" ) ; 
    if ( rcfile == NULL ) { 
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,RCFILE);
        return; 
    } else { 
        while(fgets(buffer,sizeof buffer,rcfile) != NULL) {
            /* Now look for info */ 
            if(strstr(buffer, "THEME" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                //printf("\tTHIS IS DUMMY %s \n", dummy);
                dummy2 = strdup(dummy);
                for(i=0;i<5;i++) {
                    dummy3 = strsep(&dummy2, ",");
                    if(getcolor(dummy3) == 1)
                        colors[i].color = getcolor(defaultcolor[4]);
                    else
                        colors[i].color = getcolor(dummy3);
                }
            }
            if(strstr(buffer, "DESKTOP_NAMES") !=NULL) {
                //printf("\t FOUND DESK NAMES \n");
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                //dummy[strlen(dummy)-1] = '\0';
                //printf("\tTHIS IS DUMMY %s \n", dummy);
                dummy2 = strdup(dummy);
                for(i=0;i<DESKTOPS;i++)
                    sb_bar[i].label = strsep(&dummy2, ",");
            }
           if(STATUS_BAR == 0) {
                if(strstr(buffer,"FONTNAME" ) != NULL) {
                    strncpy(fontname, strstr(buffer, " ")+2, strlen(strstr(buffer, " ")+2)-2);
                    font = XLoadQueryFont(dis, fontname);
                    if (!font) {
                        fprintf(stderr,"\033[0;34m :: snapwm :\033[0;31m unable to load preferred font: %s using fixed", fontname);
                        font = XLoadQueryFont(dis, "fixed");
                    } else {
                        logger("\033[0;32m Font Loaded");
                    }
                    sb_height = font->ascent+10;

                    // Screen height
                    sh = (XDisplayHeight(dis,screen) - (sb_height+PANEL_HEIGHT+BORDER_WIDTH));
                    sw = XDisplayWidth(dis,screen) - BORDER_WIDTH;
                }
            } else {
                sh = (XDisplayHeight(dis,screen) - (PANEL_HEIGHT+BORDER_WIDTH));
                sw = XDisplayWidth(dis,screen) - BORDER_WIDTH;
            }
        }
        fclose(rcfile);
        return;
    }
}
void update_config() {
    int i;

    read_rcfile();
    update_current();
    for(i=0;i<DESKTOPS;i++)
        XDestroyWindow(dis, sb_bar[i].sb_win);
    XDestroyWindow(dis, sb_area);
    status_bar();
    tile();
    update_current();
}

/* ********************** Keyboard Management ********************** */
void grabkeys() {
    int i;
    KeyCode code;

    XUngrabKey(dis, AnyKey, AnyModifier, root);
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

void configurenotify(XEvent *e) {
    // Do nothing for the moment
}

/* ********************** Signal Management ************************** */
void configurerequest(XEvent *e) {
    // Paste from DWM, thx again \o/
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x = ev->x;
    wc.y = ev->y;
    if(ev->width < sw-BORDER_WIDTH)
        wc.width = ev->width;
    else
        wc.width = sw-BORDER_WIDTH;
    if(ev->height < sh-BORDER_WIDTH)
        wc.height = ev->height;
    else
        wc.height = sh-BORDER_WIDTH;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
    XSync(dis, False);
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    // For fullscreen mplayer (and maybe some other program)
    client *c;

    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            XMoveResizeWindow(dis,c->win,-BORDER_WIDTH,-BORDER_WIDTH,sw+BORDER_WIDTH,sh+BORDER_WIDTH);
            return;
        }

   	Window trans = None;
   	if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
   	    add_window(ev->window);
        XMapWindow(dis, ev->window);
        XSetInputFocus(dis,ev->window,RevertToParent,CurrentTime);
        XRaiseWindow(dis,ev->window);
        return;
    }

    unsigned long count, j, extra;
    Atom realType;
    int realFormat;
    unsigned char *temp;
    Atom *type;

    if(XGetWindowProperty(dis, ev->window, atoms[ATOM_NET_WM_WINDOW_TYPE], 0, 32, False, XA_ATOM, &realType, &realFormat, &count, &extra, &temp) == Success) {
        if(count > 0) {
            type = (unsigned long*)temp;
            for(j=0; j<count; j++) {
                if((type[j] == atoms[ATOM_NET_WM_WINDOW_TYPE_UTILITY]) ||
                  (type[j] == atoms[ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION]) ||
                  (type[j] == atoms[ATOM_NET_WM_WINDOW_TYPE_SPLASH]) ||
                  (type[j] == atoms[ATOM_NET_WM_WINDOW_TYPE_DIALOG]) ||
                  (type[j] == atoms[ATOM_NET_WM_WINDOW_TYPE_DOCK])) {
                    add_window(ev->window);
                    XMapWindow(dis, ev->window);
                    XSetInputFocus(dis,ev->window,RevertToParent,CurrentTime);
                    XRaiseWindow(dis,ev->window);
                    return;
                }
            }
        }
        if(temp)
         XFree(temp);
    }

    XClassHint ch = {0};
    static unsigned int len = sizeof convenience / sizeof convenience[0];
    int i = 0;
    int tmp = current_desktop;
    if(XGetClassHint(dis, ev->window, &ch))
        for(i=0;i<len;i++)
            if(strcmp(ch.res_class, convenience[i].class) == 0) {
                save_desktop(tmp);
                select_desktop(convenience[i].preferredd-1);
                add_window(ev->window);
                if(tmp == convenience[i].preferredd-1) {
                    XMapWindow(dis, ev->window);
                    tile();
                    update_current();
                } else {
                    select_desktop(tmp);
                }
                if(convenience[i].followwin != 0) {
                    Arg a = {.i = convenience[i].preferredd-1};
                    change_desktop(a);
                }
                if(ch.res_class)
                    XFree(ch.res_class);
                if(ch.res_name)
                    XFree(ch.res_name);
                update_bar();
                return;
            }

    add_window(ev->window);
    XMapWindow(dis,ev->window);
    tile();
    update_current();
}

void destroynotify(XEvent *e) {
    int i = 0;
    int j = 0;
    int tmp = current_desktop;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    save_desktop(tmp);
    for(j=0;j<TABLENGTH(desktops);++j) {
        select_desktop(j);
        for(c=head;c;c=c->next)
            if(ev->window == c->win)
                i++;

        if(i != 0) {
            remove_window(ev->window);
            select_desktop(tmp);
            return;
        }

        i = 0;
    }
    select_desktop(tmp);
    update_bar();
}

void enternotify(XEvent *e) {
    client *c;
    XCrossingEvent *ev = &e->xcrossing;

    if(FOLLOW_MOUSE == 0) {
        if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
            return;
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                current = c;
                update_current();
                return;
       }
   }
    if(STATUS_BAR == 0) {
        int i;
        for(i=0;i<DESKTOPS;i++)
            if(sb_bar[i].sb_win == ev->window) {
                XSetWindowBackground(dis, sb_bar[i].sb_win, colors[2].color);
                XClearWindow(dis, sb_bar[i].sb_win);
                XDrawString(dis, sb_bar[i].sb_win, sb_b, (sb_width-sb_bar[i].width)/2, font->ascent, sb_bar[i].label, sb_bar[i].width);
                }
   }
}

void leavenotify(XEvent *e) {
    if(STATUS_BAR == 0) {
        XCrossingEvent *ev = &e->xcrossing;
        int i;
        for(i=0;i<DESKTOPS;i++)
            if(sb_bar[i].sb_win == ev->window)
                update_bar();
    }
}

void buttonpressed(XEvent *e) {
    client *c;
    XButtonPressedEvent *ev = &e->xbutton;

    // change focus with LMB
    if(CLICK_TO_FOCUS == 0 && ev->window != current->win && ev->button == Button1)
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                current = c;
                update_current();
                return;
            }

    if(STATUS_BAR == 0) {
        int i;
        for(i=0;i<DESKTOPS;i++)
            if(i != current_desktop && sb_bar[i].sb_win == ev->window) {
                Arg a = {.i = i};
                change_desktop(a);
            }
    }
}

void propertynotify(XEvent *e) {
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete) {
        logger("prop notify delete");
        return;
    } else
        getwindowname();
}

void send_kill_signal(Window w) {
    XEvent ke;
    ke.type = ClientMessage;
    ke.xclient.window = w;
    ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
    ke.xclient.format = 32;
    ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
    ke.xclient.data.l[1] = CurrentTime;
    XSendEvent(dis, w, False, NoEventMask, &ke);
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

void quit() {
    Window root_return, parent;
    Window *children;
    int i;
    unsigned int nchildren;
    XEvent ev;

    /*
     * if a client refuses to terminate itself,
     * we kill every window remaining the brutal way.
     * Since we're stuck in the while(nchildren > 0) { ... } loop
     * we can't exit through the main method.
     * This all happens if MOD+q is pushed a second time.
     */
    if(bool_quit == 1) {
        XUngrabKey(dis, AnyKey, AnyModifier, root);
        XDestroySubwindows(dis, root);
        logger(" \033[0;33mThanks for using!");
        XCloseDisplay(dis);
        logger("\033[0;31mforced shutdown");
        exit (0);
    }

    bool_quit = 1;
    XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
    for(i = 0; i < nchildren; i++) {
        send_kill_signal(children[i]);
    }
    //keep alive until all windows are killed
    while(nchildren > 0) {
        XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
        XNextEvent(dis,&ev);
        if(events[ev.type])
            events[ev.type](&ev);
    }

    XUngrabKey(dis,AnyKey,AnyModifier,root);
    logger("\033[0;34mYou Quit : Thanks for using!");
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

    // Read in RCFILE
    setlocale(LC_CTYPE, "");
    //printf("\t Reading RCFILE\n");
    read_rcfile();
    if(STATUS_BAR == 0)
        status_bar();

    // numlock workaround
    int j, k;
    XModifierKeymap *modmap;
    numlockmask = 0;
    modmap = XGetModifierMapping(dis);
    for (k = 0; k < 8; k++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if(modmap->modifiermap[k * modmap->max_keypermod + j] == XKeysymToKeycode(dis, XK_Num_Lock))
                numlockmask = (1 << k);
        }
    }
    XFreeModifiermap(modmap);

    // Shortcuts
    grabkeys();

    // Default stack
    mode = DEFAULT_MODE;
    growth = 0;

    // For exiting
    bool_quit = 0;

    // List of client
    head = NULL;
    current = NULL;

    // Master size
    if(mode == 2)
        master_size = sh*MASTER_SIZE;
    else
        master_size = sw*MASTER_SIZE;

    // Set up all desktop
    int i;
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].growth = growth;
        desktops[i].head = head;
        desktops[i].current = current;
    }

    // Select first dekstop by default
    const Arg arg = {.i = 0};
    current_desktop = arg.i;
    change_desktop(arg);
    // Set up atoms for dialog/notification windows
    int x;
    for(x = 0; x < ATOM_COUNT; x++)
        *atomList[x].atom = XInternAtom(dis, atomList[x].name, False);
    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask);
    XSetErrorHandler(xerror);
    logger("\033[0;32mWe're up and running!");
}

void sigchld(int unused) {
    // Again, thx to dwm ;)
	if(signal(SIGCHLD, sigchld) == SIG_ERR) {
		logger("\033[0;31mCan't install SIGCHLD handler");
		exit(1);
        }
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const Arg arg) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis)
                close(ConnectionNumber(dis));

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

    // Main loop, just dispatch events (thx to dwm ;)
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

    return 0;
}
