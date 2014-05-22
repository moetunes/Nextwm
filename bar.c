// bar.c [ 0.8.1 ]

static void draw_numopen(unsigned int cd, unsigned int gc);
static Drawable area_sb;
static unsigned int total_w, pos;
/* ************************** Status Bar *************************** */
void setup_status_bar() {
    unsigned int i;
    XGCValues values;

    for(i=0;i<10;++i) {
        values.foreground = theme[i].textcolor;
        values.line_width = 2;
        values.line_style = LineSolid;
        if(font.fontset)
            theme[i].gc = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle,&values);
        else {
            values.font = font.font->fid;
            theme[i].gc = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
        }
    }
    sb_desks = 0;
    for(i=0;i<DESKTOPS;++i) {
        sb_bar[i].labelwidth = wc_size(sb_bar[i].label);
        sb_bar[i].width = sb_bar[i].labelwidth + 6;
        if(sb_bar[i].width < sb_height*3/2) sb_bar[i].width = sb_height*3/2;
        sb_desks += sb_bar[i].width;
    }
    sb_desks += 2;
}

void status_bar() {
    unsigned int i, y;

    y = (topbar == 0) ? 0 : desktops[barmon].h+bdw;
    sb_width = 0;
    for(i=0;i<DESKTOPS;++i) {
        sb_bar[i].sb_win = XCreateSimpleWindow(dis, root, desktops[barmon].x+sb_width, y,
                                            sb_bar[i].width-2,sb_height,2,theme[3].barcolor,theme[0].barcolor);

        XSelectInput(dis, sb_bar[i].sb_win, ButtonPressMask|EnterWindowMask|LeaveWindowMask);
        XMapWindow(dis, sb_bar[i].sb_win);
        sb_width += sb_bar[i].width;
    }
    sb_area = XCreateSimpleWindow(dis, root, desktops[barmon].x+sb_desks, y,
             desktops[barmon].w-lessbar-(sb_desks+2),sb_height,2,theme[3].barcolor,theme[1].barcolor);

    XSelectInput(dis, sb_area, ButtonPressMask|ExposureMask|EnterWindowMask|LeaveWindowMask);
    XMapWindow(dis, sb_area);
    XGetWindowAttributes(dis, sb_area, &attr);
    total_w = attr.width;
    area_sb = XCreatePixmap(dis, root, total_w, sb_height, DefaultDepth(dis, screen));
    XFillRectangle(dis, area_sb, theme[0].gc, 0, 0, total_w, sb_height+4);
    status_text("");
    update_bar();
}

void toggle_bar() {
    if(desktops[current_desktop].screen != barmon) return;
    if(STATUS_BAR == 0) {
        if(has_bar == 0) {
            show_bar = 1;
            unmapbar();
            desktops[current_desktop].h += sb_height+4;
        } else {
            show_bar = 0;
            mapbar();
            desktops[current_desktop].h -= sb_height+4;
            update_bar();
        }

        sh = desktops[current_desktop].h;
        tile();
    }
}

void unmapbar() {
    for(int i=0;i<DESKTOPS;++i) XUnmapWindow(dis,sb_bar[i].sb_win);
    XUnmapWindow(dis, sb_area);
    has_bar = 1;
}

void mapbar() {
    for(int i=0;i<DESKTOPS;++i) XMapWindow(dis, sb_bar[i].sb_win);
    XMapWindow(dis, sb_area);
    has_bar = 0;
}

void setbaralpha() {
    unsigned int i;
    if(baralpha < 100) {
        XChangeProperty(dis, sb_area, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &baropacity, 1l);
        for(i=0;i<DESKTOPS;++i)
            XChangeProperty(dis, sb_bar[i].sb_win, alphaatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &baropacity, 1l);
    }
}

void getwindowname() {
    char *win_name;

    if(head != NULL) {
        (XFetchName(dis, current->win, &win_name) != 0) ? status_text(win_name) : status_text("");
        XFree(win_name);
    } else status_text("");
}

void update_bar() {
    unsigned int i;

    for(i=0;i<DESKTOPS;++i) {
        if(i != current_desktop) {
            if(desktops[i].head != NULL) {
                draw_desk(sb_bar[i].sb_win, 2, 3, (sb_bar[i].width-sb_bar[i].labelwidth)/2, sb_bar[i].label, strlen(sb_bar[i].label));
                if(showopen < 1 && desktops[i].numwins > 1) {
                    draw_numopen(i, 3);
                }
            } else {
                draw_desk(sb_bar[i].sb_win, 1, 2, (sb_bar[i].width-sb_bar[i].labelwidth)/2, sb_bar[i].label, strlen(sb_bar[i].label));
            }
            if(i == previous_desktop)
                XFillRectangle(dis, sb_bar[i].sb_win, theme[3].gc, 0, 0, 2, 2);
        } else {
            draw_desk(sb_bar[i].sb_win, 0, 1, (sb_bar[i].width-sb_bar[i].labelwidth)/2, sb_bar[i].label, strlen(sb_bar[i].label));
            if(showopen < 1 && (desktops[i].mode == 1 || desktops[i].mode == 4) && desktops[i].numwins > 1) {
                draw_numopen(i, 1);
            }
        }
    }
    getwindowname();
}

void draw_desk(Window win, unsigned int barcolor, unsigned int gc, unsigned int x, char *string, unsigned int len) {
    XSetWindowBackground(dis, win, theme[barcolor].barcolor);
    XClearWindow(dis, win);
    if(font.fontset)
        XmbDrawString(dis, win, font.fontset, theme[gc].gc, x, font.fh, string, len);
    else
        XDrawString(dis, win, theme[gc].gc, x, font.fh, string, len);
}

void draw_numopen(unsigned int cd, unsigned int gc) {
    unsigned int i, x=0, y=sb_height-2;

    for(i=0;i<desktops[cd].numwins; ++i) {
        XFillRectangle(dis, sb_bar[cd].sb_win, theme[gc].gc, x, y, 2, 2);
        x += 3;
        if((x+3) >= sb_bar[cd].width) return;
    }
}

void draw_text(Window win, unsigned int gc, unsigned int x, char *string, unsigned int len) {
    if(font.fontset)
        XmbDrawString(dis, win, font.fontset, theme[gc].gc, x, font.fh, string, len);
    else
        XDrawString(dis, win, theme[gc].gc, x, font.fh, string, len);
}

void status_text(char *sb_text) {
    unsigned int text_length, text_start, blank_start, wsize, count = 0, wnl;
    char win_name[256];

    XFillRectangle(dis, area_sb, theme[0].gc, 0, 0, pos, sb_height+4);
    if(strlen(sb_text) < 1) sb_text = ":";
    while(sb_text[count] != '\0' && count < windownamelength) {
        win_name[count] = sb_text[count];
        ++count;
    }
    win_name[count] = '\0';
    wnl = windownamelength*font.width;
    wsize = wc_size(win_name);
    text_length = (wsize >= wnl) ? wnl : wsize;
    blank_start = wc_size(theme[mode].modename)+(4*font.width);
    pos = blank_start+wnl;
    text_start = (LA_WINDOWNAME < 1) ? blank_start : pos - text_length;

    draw_text(area_sb, 4, font.width*2, theme[mode].modename, strlen(theme[mode].modename));
    XFillRectangle(dis, area_sb, theme[wnamebg].gc, text_start, 0, text_length, sb_height+4);
    draw_text(area_sb, 4, text_start, win_name, count);
    XCopyArea(dis, area_sb, sb_area, theme[1].gc, 0, 0, pos, sb_height+4, 0, 0);
}

void update_output(unsigned int messg) {
    unsigned int text_length=0, p_length, text_start, i, j=2, k=0;
    unsigned int wsize, n, bg=0;
    char output[256];
    char astring[256];
    char *win_name;

    if(!(XFetchName(dis, root, &win_name))) {
        strcpy(output, "&3 snapwm inc. ");
        text_length = 15;
    } else {
        while(win_name[text_length] != '\0' && text_length < 256) {
            output[text_length] = win_name[text_length];
            ++text_length;
        }
        text_length += 1;
        output[text_length] = '\0';
    }
    XFree(win_name);

    for(n=0;n<text_length;++n) {
        while(output[n] == '&') {
            if(output[n+1]-'0' < 10 && output[n+1]-'0' >= 0) {
                n += 2;
            } else if(output[n+1] == 'B' && output[n+2]-'0' < 10 && output[n+2]-'0' >= 0) {
                bg = output[n+2]-'0';
                n += 3;
            } else break;
        }
        astring[k] = output[n]; ++k;
    }
    astring[k] = '\0';
    p_length = wc_size(astring);
    text_start = total_w - p_length;
    XFillRectangle(dis, area_sb, theme[0].gc, pos, 0, total_w-pos, sb_height+4);
    k = 0; // i=pos on screen k=pos in text
    for(i=text_start;i<total_w;++i) {
        if(k <= text_length) { 
            while(output[k] == '&') {
                if(output[k+1]-'0' < 10 && output[k+1]-'0' >= 0) {
                    j = output[k+1]-'0';
                    k += 2;
                } else if(output[k+1] == 'B' && output[k+2]-'0' < 10 && output[k+2]-'0' >= 0) {
                    bg = output[k+2]-'0';
                    k += 3;
                } else break;
            }
            n = 0;
            if(output[k] == '&') {
                astring[0] = '&';
                ++n;++k;
            }
            while(output[k] != '&' && output[k] != '\0' && output[k] != '\n' && output[k] != '\r') {
                astring[n] = output[k];
                ++n;++k;
            }
            if(n < 1)
                continue;
            astring[n] = '\0';
            wsize = wc_size(astring);
            XFillRectangle(dis, area_sb, theme[bg].gc, i, 0, wsize, sb_height+4);
            draw_text(area_sb, j, i, astring, n);
            i += wsize-1;
            for(n=0;n<256;++n)
                astring[n] = '\0';
        }
    }

    XCopyArea(dis, area_sb, sb_area, theme[1].gc, pos, 0, (total_w - pos), sb_height+4, pos, 0);
    for(n=0;n<256;++n)
        output[n] ='\0';
    XSync(dis, False);
    return;
}

unsigned int wc_size(char *string) {
    unsigned int num;
    XRectangle rect;
    num = strlen(string);

    if(font.fontset) {
        XmbTextExtents(font.fontset, string, num, NULL, &rect);
        return rect.width;
    } else {
        return XTextWidth(font.font, string, num);
    }
}
