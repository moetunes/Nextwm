/* bluckywm.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <xcb/xcb.h>
#include "config.h"

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union {
    const char** com;
    const int i;
} Arg;

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;

    // The window
    xcb_window_t win;
};

typedef struct desktop desktop;
struct desktop{
    int master_size;
    int mode;
    client *head;
    client *current;
};

static void add_window(xcb_window_t w);
static void remove_window(xcb_window_t w);
static void save_desktop(int i);
static void select_desktop(int i);
static void tile();
static void update_current();

static void events();
static void sigcatch(int sig);
static void cleanup(int code);

static void logger(const char* e);

static int current_desktop;
static int master_size;
static int mode;
static int sh;
static int sw;
static int sigcode;
xcb_connection_t *conn;         /* Connection to X server. */
xcb_screen_t *screen;           /* Our current screen.  */
xcb_colormap_t colormap;     /* our colourmap connection */
static client *head;
static client *current;

static desktop desktops[5];

/* ***************************** Window Management ******************************* */
void add_window(xcb_window_t w) {
    client *c,*t;

    c = malloc(sizeof (c));
    if (NULL == c) {
        logger("Error calloc!");
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
}

void remove_window(xcb_window_t w) {
    client *c;

    for(c=head;c;c=c->next) {
        if(c->win == w) {
            if(c->prev == NULL && c->next == NULL) {
                free(head);
                head = NULL;
                current = NULL;
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
            return;
        }
    }
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].head = head;
    desktops[i].current = current;
}

void select_desktop(int i) {
    head = desktops[i].head;
    current = desktops[i].current;
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    current_desktop = i;
}

void tile() {
    client *c;
    int n = 0;
    int x = 0;
    uint32_t mask = 0;    
    uint32_t values[4];

    // If only one window
    if(head != NULL && head->next == NULL) {
        mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        values[0] = 0;
        values[1] = 0;
        values[2] = sw;
        values[3] = sh;
        xcb_configure_window (conn, head->win, mask, values);
    }
    else if(head != NULL) {
        switch(mode) {
            case 0: // Horizontal
                // Master window
                mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                values[0] = 0;
                values[1] = 0;
                values[2] = sw;
                values[3] = master_size-2*BORDER_WIDTH;
                xcb_configure_window (conn, head->win, mask, values);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                for(c=head->next;c;c=c->next) {
                    mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                    values[0] = x;
                    values[1] = master_size;
                    values[2] = sw/n-BORDER_WIDTH;
                    values[3] = sh-master_size;
                    xcb_configure_window (conn, c->win, mask, values);
                    x += sw/n;
                }
                break;
            case 1: // Fullscreen
                for(c=head;c;c=c->next) {
                    mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                    values[0] = 0;
                    values[1] = 0;
                    values[2] = sw;
                    values[3] = sh;
                    xcb_configure_window (conn, c->win, mask, values);
                }
                break;
            case 2: // Vertical
            	// Master window
                mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                values[0] = 0;
                values[1] = 0;
                values[2] = master_size-2*BORDER_WIDTH;
                values[3] = sh;
                xcb_configure_window (conn, head->win, mask, values);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                for(c=head->next;c;c=c->next) {
                    mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                    values[0] = master_size;
                    values[1] = x;
                    values[2] = sw-master_size;
                    values[3] = (sh/n);
                    xcb_configure_window (conn, c->win, mask, values);
                    x += sh/n;
                }
                break;
            default:
                break;
        }
    }
}

void update_current() {
    client *c;
    xcb_alloc_color_cookie_t colcookie;
    xcb_alloc_color_reply_t *reply;
    xcb_generic_error_t *error;
    uint32_t values[2];

    for(c=head;c;c=c->next)
        if(current == c) {
            // "Enable" current window
            colcookie = xcb_alloc_color (conn, colormap, 33000, 20000, 10000);
            reply = xcb_alloc_color_reply (conn, colcookie, &error);
            if (NULL != error)
            {
                logger("Couldn't get pixel value for colour : Exiting.\n");
                xcb_disconnect(conn);
                exit(1);
            }
            values[0] = reply->pixel;
            xcb_change_window_attributes(conn, c->win, XCB_CW_BORDER_PIXEL, values);
            values[0] = BORDER_WIDTH;
            xcb_configure_window(conn, c->win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
            values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
            xcb_change_window_attributes_checked(conn, c->win, XCB_CW_EVENT_MASK, values);
            values[0] = ( XCB_STACK_MODE_ABOVE );
            xcb_configure_window(conn, c->win, XCB_CONFIG_WINDOW_STACK_MODE, values);
        }
        else {
            colcookie = xcb_alloc_color (conn, colormap, 0, 16000, 19000);
            reply = xcb_alloc_color_reply (conn, colcookie, NULL);
            values[0] = reply->pixel;
            xcb_change_window_attributes(conn, c->win, XCB_CW_BORDER_PIXEL, values);
            free (reply);
        }

    xcb_flush(conn);
}

/********************************* Signals *****************************************/

void logger(const char* e) {
    fprintf(stdout, " :: bluckywm : %s\n", e);
}

void cleanup(int code)
{
    xcb_set_input_focus(conn, XCB_NONE,
                        XCB_INPUT_FOCUS_POINTER_ROOT,
                        XCB_CURRENT_TIME);
    xcb_flush(conn);
    xcb_disconnect(conn);    
    exit(code);
}

void events()
{
    xcb_generic_event_t *ev;

    int fd;                         /* Our X file descriptor */
    fd_set in;                      /* For select */
    int found;                      /* Ditto. */

    /* Get the file descriptor so we can do select() on it. */
    fd = xcb_get_file_descriptor(conn);

    for (sigcode = 0; 0 == sigcode;)
    {
        /* Prepare for select(). */
        FD_ZERO(&in);
        FD_SET(fd, &in);

        /* Check for events, again and again. When poll returns NULL, we block on select() until the event file descriptor gets readable again. We do it this way instead of xcb_wait_for_event() since select() will return if we we're interrupted by a signal. We like that.*/
        ev = xcb_poll_for_event(conn);
        if (NULL == ev)
        {
            found = select(fd + 1, &in, NULL, NULL, NULL);
            if (-1 == found)
            {
                if (EINTR == errno)
                { /* We received a signal. Break out of loop. */
                    break;
                }
                else
                { /* Something was seriously wrong with select(). */
                    logger(" select failed.");
                    cleanup(0);
                    exit(1);
                }
            }
            else
            {/* We found more events. Goto start of loop. */
                continue;
            }
        }
        switch (ev->response_type & ~0x80)
        {
        case XCB_MAP_REQUEST:
        {
            xcb_map_request_event_t *e;

            e = (xcb_map_request_event_t *) ev;
            add_window(e->window);
            xcb_map_window(conn, e->window);
            tile();
            update_current();
        }
        break;
        
        case XCB_DESTROY_NOTIFY:
        {
            int i=0;
            client *c;
            xcb_destroy_notify_event_t *e;

            e = (xcb_destroy_notify_event_t *) ev;
            for(c=head;c;c=c->next)
                if(e->window == c->win)
                    i++;

            if(i == 0)
                return;

            remove_window(e->window);
            tile();
            update_current();

        }
        break;
        
        case XCB_ENTER_NOTIFY:
        {
            xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;
            client *c;

            if (e->mode != XCB_NOTIFY_MODE_NORMAL || e->mode != XCB_NOTIFY_MODE_UNGRAB) {
                for(c=head;c;c=c->next)
                    if(e->event == c->win) {
                        current = c;
                        update_current();
                    }
            }
        }
        break;

        }
    }
}

void sigcatch(int sig)
{
    sigcode = sig;
}

int main ()
{
    uint32_t mask = 0;
    uint32_t values[2];
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;
    xcb_drawable_t root;

    /* Install signal handlers. */
    if (SIG_ERR == signal(SIGINT, sigcatch))
    {
        logger("signal");
        exit(1);
    }
    if (SIG_ERR == signal(SIGTERM, sigcatch))
    {
        logger("signal");
        exit(1);
    }
    
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn))
    {
        logger("xcb_connect");
        exit(1);
    }
    
    /* Get the first screen */
    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    root = screen->root;
    sw = screen->width_in_pixels-2*BORDER_WIDTH;
    sh = (screen->height_in_pixels-PANEL_HEIGHT)-2*BORDER_WIDTH;
    colormap = screen->default_colormap;

    // Default tiling
    mode = DEFAULT_MODE;
    head = NULL;
    current = NULL;
    if(mode == 0)
        master_size = sh*MASTER_SIZE;
    else
        master_size = sw*MASTER_SIZE;

    // initiate desktops
    int i;
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].head = head;
        desktops[i].current = current;
    }
    const Arg arg = {.i = 0};
    current_desktop = arg.i;
    //change_desktop(arg);

    fprintf(stdout, " :: bluckywm : %dX%d \n", sw, sh);

    // initiate events
    mask = XCB_CW_EVENT_MASK;
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    // Check if there's another wm running
    cookie = xcb_change_window_attributes_checked(conn, root, mask, values);
    error = xcb_request_check(conn, cookie);
    xcb_flush(conn);
    if (NULL != error)
    {
        fprintf(stderr, " :: bluckywm : Can't get SUBSTRUCTURE REDIRECT. "
                "Error code: %d\n"
                "Another window manager running? Exiting.\n",
                error->error_code);

        xcb_disconnect(conn);
        exit(1);
    }

    while (1)
    {
    /* Loop over events. */
    events();
    }
    /* Die gracefully. */
    cleanup(sigcode);

    exit(0);
}
