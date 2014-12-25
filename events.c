// events.c [ 0.8.4 ]

void configurenotify(XEvent *e) {
    // Do nothing for the moment
}

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
    if(check_dock(ev->window) == 0) {
         XMapWindow(dis,ev->window);
         return;
    }

    // For fullscreen mplayer (and maybe some other program)
    client *c;
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            return;
        }

    Window w;
    w = (numwins > 0) ? current->win:0;

    Window trans = None; unsigned int tranny = 1;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None)
        tranny = 0;

    char *this = getwindowname(ev->window);
    XClassHint ch = {0};
    unsigned int i=0, j=0, tmp = current_desktop, tmp2;
    if(XGetClassHint(dis, ev->window, &ch)) {
        for(i=0;i<pcount;++i) {
            if((strcmp(this, positional[i].class) == 0) ||
              (tranny == 1 && strcmp(ch.res_class, positional[i].class) == 0) ||
              (tranny == 1 && strcmp(ch.res_name, positional[i].class) == 0)) {
                XMoveResizeWindow(dis,ev->window,positional[i].x,positional[i].y,positional[i].width,positional[i].height);
                ++j;
                break;
            }
        }
        if(tranny == 1 && j == 0 && cstack == 0) {
            XGetWindowAttributes(dis, ev->window, &attr);
            XMoveWindow(dis, ev->window,desktops[current_desktop].w/2-(attr.width/2),(desktops[current_desktop].h+sb_height+4)/2-(attr.height/2));
        }
        j = 0;
        for(i=0;i<dtcount;++i) {
            if((strcmp(this, convenience[i].class) == 0) ||
              (tranny == 1 && strcmp(ch.res_class, convenience[i].class) == 0) ||
              (tranny == 1 && strcmp(ch.res_name, convenience[i].class) == 0)) {
                tmp2 = (convenience[i].preferredd > DESKTOPS) ? DESKTOPS-1 : convenience[i].preferredd-1;
                save_desktop(tmp);
                if(convenience[i].followwin == 0) {
                    Arg a = {.i = tmp2};
                    change_desktop(a);
                } else select_desktop(tmp2);
                for(c=head;c;c=c->next)
                    if(ev->window == c->win)
                        ++j;
                if(tranny == 1 && j < 1) add_window(ev->window, 0, NULL);
                if(tranny == 0) add_window(ev->window, 1, NULL);
                for(j=0;j<num_screens;++j) {
                    if(view[j].cd == convenience[i].preferredd-1) {
                        if(tranny == 1) {
                            tile();
                            if(mode == 1 && numwins > 1) XUnmapWindow(dis, w);
                        }
                        if(mode != 1) XMapWindow(dis, ev->window);
                        update_current();
                    }
                }
                if(convenience[i].followwin != 0) select_desktop(tmp);
                if(STATUS_BAR == 0) update_bar();
                if(ch.res_class) XFree(ch.res_class);
                if(ch.res_name) XFree(ch.res_name);
                return;
            }
        }
    }

    if(tranny == 1) {
        add_window(ev->window, 0, NULL);
        tile();
        if(mode != 1) XMapWindow(dis,ev->window);
        if(mode == 1 && numwins > 1) XUnmapWindow(dis, w);
    } else {
        add_window(ev->window, 1, NULL);
        XMapWindow(dis, ev->window);
    }
    update_current();
    if(STATUS_BAR == 0) update_bar();
    if(ch.res_class) XFree(ch.res_class);
    if(ch.res_name) XFree(ch.res_name);
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
    int i=0, cd = current_desktop, foundit=0;

    if(STATUS_BAR == 0) {
        if(sb_area == (ev->subwindow | ev->window)) {
            Arg a = {.i = previous_desktop};
            dowarp = 1;
            change_desktop(a);
            return;
        }
        for(i=0;i<DESKTOPS;++i)
            if(sb_bar[i].sb_win == (ev->window | ev->subwindow)) {
                if(i != current_desktop) {
                    Arg a = {.i = i};
                    if(ev->button == Button1) {
                        change_desktop(a);
                        return;
                    } else if(ev->button == Button3) {
                        follow_client_to_desktop(a);
                        return;
                    }
                } else {
                    if(ev->button == Button1) {
                        next_win();
                        return;
                    }
                }
            }
    }

    // change focus with LMB
    unsigned int cds = desktops[cd].screen;
    if(clicktofocus == 0) {
        save_desktop(cd);
        for(i=cds;i<cds+num_screens;++i) {
            select_desktop(view[i%num_screens].cd);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    if(cd != current_desktop || c != focus) {
                        foundit = 1;
                        focus = c; current = c;
                        break;
                    }
                }
            if(foundit == 0)
                for(c=transient;c;c=c->next)
                    if(ev->window == c->win) {
                        if(c != focus) {
                            foundit = 1;
                            focus = c;
                        } else {
                            foundit = 2; break;
                        }
                    }
            if(foundit == 2) break;
            if(foundit == 1) {
                if(cd != current_desktop) {
                    save_desktop(current_desktop);
                    Arg a = {.i = current_desktop}; dowarp = 1;
                    select_desktop(cd);
                    change_desktop(a); dowarp = 0;
                }
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
    if(mode == 4)
        for(c=head;c;c=c->next)
            if(ev->subwindow == c->win) i = 1;
    if(i == 0 && transient != NULL)
        for(c=transient;c;c=c->next)
            if(ev->subwindow == c->win) i = 1;
    if(i == 0) return;
    XGrabPointer(dis, ev->subwindow, True,
        PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
        GrabModeAsync, None, None, CurrentTime);
    XGetWindowAttributes(dis, ev->subwindow, &attr);
    starter = e->xbutton; doresize = 1;
}

void motionnotify(XEvent *e) {
    client *c;
    XMotionEvent *ev = &e->xmotion;

    if(ev->window != current->win) {
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                current = focus = c;
                update_current();
                dowarp = 0;
                return;
           }
    }
    if(doresize == 0) return;
    while(XCheckTypedEvent(dis, MotionNotify, e) && doresize == 1);
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

    if(doresize == 0) {
        XSendEvent(dis, PointerWindow, False, 0xfff, e);
        XFlush(dis);
        return;
    }
    doresize = 0;
    XUngrabPointer(dis, CurrentTime);
    if(mode == 4) {
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                XGetWindowAttributes(dis, c->win, &attr);
                c->x = attr.x - desktops[current_desktop].x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height; return;
            }
    }
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            if(ev->window == c->win) {
                XGetWindowAttributes(dis, c->win, &attr);
                c->x = attr.x - desktops[current_desktop].x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height;
                return;
            }
}

void propertynotify(XEvent *e) {
    XPropertyEvent *ev = &e->xproperty;
    unsigned int i, tmp = current_desktop; client *c;

    if(ev->state == PropertyDelete) return;
    else if(ev->atom == XA_WM_NAME && ev->window == root && sb_area) update_output(0);
    else if(ev->atom == XA_WM_NAME && (head != NULL || transient != NULL)) status_text(getwindowname(focus->win));
    else if(ev->atom == XA_WM_HINTS) {
        save_desktop(tmp);
        for(i=tmp;i<tmp+DESKTOPS;++i) {
            select_desktop(i%DESKTOPS);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    XWMHints *wmh = XGetWMHints(dis, c->win);
                    if(wmh && (wmh->flags & XUrgencyHint)) {
                        current = focus = c;
                        if(current_desktop == tmp) update_current();
                        draw_desk(sb_bar[current_desktop].sb_win, 4, 3, (sb_bar[current_desktop].width-sb_bar[current_desktop].labelwidth)/2,\
                          sb_bar[current_desktop].label, strlen(sb_bar[current_desktop].label));
                        XFree(wmh);
                    }
                    break;
                }
        }
        select_desktop(tmp);
    }
}

void unmapnotify(XEvent *e) { // for thunderbird's write window and maybe others
    XUnmapEvent *ev = &e->xunmap;
    client *c;

    if(ev->send_event == 1) {
        if(transient != NULL) {
            for(c=transient;c;c=c->next)
                if(ev->window == c->win) {
                    remove_client(c, 1, 1);
                    update_current();
                    return;
                }
        }
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_client(c, 1, 0);
                update_current();
                if(STATUS_BAR == 0) update_bar();
                return;
            }
    }
}

void expose(XEvent *e) {
    XExposeEvent *ev = &e->xexpose;

    if(STATUS_BAR == 0 && ev->count == 0 && ev->window == sb_area) {
        update_output(1);
        if(head != NULL || transient != NULL) status_text(getwindowname(focus->win));
        update_bar();
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
