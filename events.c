// events.c [ 0.8.6 ]

void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XWindowChanges wc;
    int y = 0;
    if(STATUS_BAR == 0 && topbar == 0 && show_bar == 0) y = sb_height+4+ug_out;

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

    unsigned int i=0, j=0, tmp = current_desktop, tmp2, move = 0;
    save_desktop(tmp);
    Window trans = None; unsigned int tranny = 0;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
        tranny = 1;
        for(i=current_desktop;i<current_desktop+DESKTOPS;++i) {
            select_desktop(i%DESKTOPS);
            for(c=head;c;c=c->next)
                if(c->win == trans) {
                    j = 1; break;
                }
            if(j == 1) break;
        }
    }

    getwindowname(ev->window, 1);
    XClassHint ch = {0};
    if(XGetClassHint(dis, ev->window, &ch)) {
        for(i=0;i<dtcount;++i) {
            if((strcmp(winname, convenience[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_class, convenience[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_name, convenience[i].class) == 0)) {
                tmp2 = (convenience[i].preferredd > DESKTOPS) ? DESKTOPS-1 : convenience[i].preferredd-1;
                if(convenience[i].followwin == 0) {
                    Arg a = {.i = tmp2};
                    change_desktop(a);
                    move += 1;
                } else {
                    select_desktop(tmp2);
                    break;
                }
            }
        }
        j = 0;
        for(i=0;i<pcount;++i) {
            if((strcmp(winname, positional[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_class, positional[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_name, positional[i].class) == 0)) {
                XMoveResizeWindow(dis,ev->window,positional[i].x,positional[i].y,positional[i].width,positional[i].height);
                ++j;
                break;
            }
        }
        for(i=0;i<tcount;++i) {
            if((strcmp(winname, popped[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_class, popped[i].class) == 0) ||
              (tranny == 0 && strcmp(ch.res_name, popped[i].class) == 0)) {
                ++j;
                tranny = 1;
                XMoveResizeWindow(dis, ev->window, desktops[current_desktop].x+attr.x, attr.y, attr.width, attr.height);
                break;
            }
        }
        if(tranny == 0 && j == 0 && cstack == 0) {
            ++j;
            XMoveWindow(dis, ev->window,desktops[current_desktop].w/2-(attr.width/2),(desktops[current_desktop].h+sb_height+4+ug_bar)/2-(attr.height/2));
        }
    }
    if(ch.res_class) XFree(ch.res_class);
    if(ch.res_name) XFree(ch.res_name);
    if(j > 0) XGetWindowAttributes(dis, ev->window, &attr);

    add_window(ev->window, tranny, NULL, attr.x, attr.y, attr.width, attr.height);
    if(mode == 1 && numwins > 1) XUnmapWindow(dis, w);
    for(i=0;i<num_screens;++i)
        if(current_desktop == view[i].cd) {
            if(mode != 1 ) XMapWindow(dis,ev->window);
            if(mode == 1 && tranny == 1) XMapWindow(dis,ev->window);
            tile();
        }
    if(move == 0) select_desktop(tmp);
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
        if(foundit == 1) break;
    }
    if(foundit == 1)
        for(i=0;i<num_screens;++i)
            if(current_desktop == view[i].cd) {
                if(mode != 4) tile();
            }
    if(foundit == 1 && current_desktop == tmp) update_current();
    select_desktop(tmp);
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
                        if(c->trans == 0) focus = current = c;
                        else focus = c;
                        break;
                    }
                }
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
    for(c=head;c;c=c->next)
        if(ev->subwindow == c->win) {
            if(c->trans == 1 || mode == 4) i = 1;
            break;
        }
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

    if(ev->window != focus->win) {
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                if(c->trans == 0) current = focus = c;
                else focus = c;
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

    XUngrabPointer(dis, CurrentTime);
    if(doresize == 0) {
        XSendEvent(dis, PointerWindow, False, 0xfff, e);
        XFlush(dis);
        return;
    }
    doresize = 0;
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XGetWindowAttributes(dis, c->win, &attr);
            c->x = attr.x - desktops[current_desktop].x;
            c->y = attr.y;
            c->w = attr.width;
            c->h = attr.height; return;
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

void propertynotify(XEvent *e) {
    XPropertyEvent *ev = &e->xproperty;
    unsigned int i, tmp = current_desktop; client *c;

    if(ev->state == PropertyDelete) return;
    else if(ev->atom == XA_WM_NAME && ev->window == root) update_output(0);
    else if(ev->atom == XA_WM_NAME && (head != NULL))
        getwindowname(focus->win, 0);
    else if(ev->atom == XA_WM_HINTS) {
        save_desktop(tmp);
        for(i=tmp;i<tmp+DESKTOPS;++i) {
            select_desktop(i%DESKTOPS);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    XWMHints *wmh = XGetWMHints(dis, c->win);
                    if(wmh && (wmh->flags & XUrgencyHint)) {
                        if(c->trans == 0) current = focus = c;
                        else focus = c;
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
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_client(c, 1, 0);
                if(mode != 4) tile();
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
        if(head != NULL) getwindowname(focus->win, 0);
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
