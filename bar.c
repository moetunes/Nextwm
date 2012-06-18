// bar.c [ 0.5.4 ]

static void draw_numopen(int cd, int gc);
static Drawable area_sb;
static GC bggc;
static int total_w;
static int pos;
/* ************************** Status Bar *************************** */
void setup_status_bar() {
    int i;
    XGCValues values;

    logger(" \033[0;33mStatus Bar called ...");

    for(i=0;i<7;i++) {
        values.background = theme[1].barcolor;
        values.foreground = theme[i].textcolor;
        values.line_width = 2;
        values.line_style = LineSolid;
        if(font.fontset)
            theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle,&values);
        else {
            values.font = font.font->fid;
            theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
        }
    }
    values.background = theme[1].barcolor;
    values.foreground = theme[1].barcolor;
    values.line_width = 2;
    values.line_style = LineSolid;
    if(font.fontset)
        bggc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle,&values);
    else {
        values.font = font.font->fid;
        bggc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
    }
    sb_width = 0;
    for(i=0;i<DESKTOPS;i++) {
        if(font.fontset)
            sb_bar[i].width = wc_size(sb_bar[i].label);
        else
            sb_bar[i].width = XTextWidth(font.font, sb_bar[i].label, strlen(sb_bar[i].label));
        if(sb_bar[i].width > sb_width)
            sb_width = sb_bar[i].width;
    }
    sb_width += 4;
    if(sb_width < sb_height*3/2) sb_width = sb_height*3/2;
    sb_desks = (DESKTOPS*sb_width)+2;
}

void status_bar() {
    int i, y;

    y = (topbar == 0) ? 0 : sh+bdw;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].sb_win = XCreateSimpleWindow(dis, root, i*sb_width, y,
                                            sb_width-2,sb_height,2,theme[3].barcolor,theme[0].barcolor);

        XSelectInput(dis, sb_bar[i].sb_win, ButtonPressMask|EnterWindowMask|LeaveWindowMask);
        XMapRaised(dis, sb_bar[i].sb_win);
    }
    sb_area = XCreateSimpleWindow(dis, root, sb_desks, y,
             sw-(sb_desks+2),sb_height,2,theme[3].barcolor,theme[1].barcolor);

    XSelectInput(dis, sb_area, ExposureMask|EnterWindowMask|LeaveWindowMask);
    XMapRaised(dis, sb_area);
    XGetWindowAttributes(dis, sb_area, &attr);
    total_w = attr.width;
    area_sb = XCreatePixmap(dis, root, total_w, sb_height, DefaultDepth(dis, screen));
    XFillRectangle(dis, area_sb, bggc, 0, 0, total_w, sb_height+4);
    status_text("");
    update_bar();
}

void toggle_bar() {
    int i;

    if(STATUS_BAR == 0) {
        if(show_bar == 0) {
            show_bar = 1;
            sh += sb_height+4;
            for(i=0;i<DESKTOPS;i++) XUnmapWindow(dis,sb_bar[i].sb_win);
            XUnmapWindow(dis, sb_area);
        } else {
            show_bar = 0;
            sh -= sb_height+4;
            for(i=0;i<DESKTOPS;i++) XMapRaised(dis, sb_bar[i].sb_win);
            XMapRaised(dis, sb_area);
        }

        if(mode != 4) tile();
        update_current();
        update_bar();
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
    int i;

    for(i=0;i<DESKTOPS;i++) {
        if(i != current_desktop) {
            if(desktops[i].head != NULL) {
                if(showopen < 1 && desktops[i].numwins > 1) {
                    draw_desk(sb_bar[i].sb_win, 2, 2, (sb_width-sb_bar[i].width)/2, sb_bar[i].label, strlen(sb_bar[i].label));
                    draw_numopen(i, 2);
                } else {
                    draw_desk(sb_bar[i].sb_win, 2, 2, (sb_width-sb_bar[i].width)/2, sb_bar[i].label, strlen(sb_bar[i].label));
                }
            } else {
                draw_desk(sb_bar[i].sb_win, 1, 1, (sb_width-sb_bar[i].width)/2, sb_bar[i].label, strlen(sb_bar[i].label));
            }
        } else {
            if(showopen < 1 && (desktops[i].mode == 1 || desktops[i].mode == 4) && desktops[i].numwins > 1) {
                draw_desk(sb_bar[i].sb_win, 0, 0, (sb_width-sb_bar[i].width)/2, sb_bar[i].label, strlen(sb_bar[i].label));
                draw_numopen(i, 0);
            } else {
                draw_desk(sb_bar[i].sb_win, 0, 0, (sb_width-sb_bar[i].width)/2, sb_bar[i].label, strlen(sb_bar[i].label));
            }
        }
    }
}

void draw_desk(Window win, int barcolor, int gc, int x, char *string, int len) {
    XSetWindowBackground(dis, win, theme[barcolor].barcolor);
    XClearWindow(dis, win);
    if(font.fontset)
        XmbDrawString(dis, win, font.fontset, theme[gc].gc, x, font.fh, string, len);
    else
        XDrawString(dis, win, theme[gc].gc, x, font.fh, string, len);
}

void draw_numopen(int cd, int gc) {
    int i, x=0, y=sb_height-2;

    for(i=0;i<desktops[cd].numwins; i++) {
        XFillRectangle(dis, sb_bar[cd].sb_win, theme[gc].gc, x, y, 2, 2);
        x += 3;
        if((x+3) >= sb_width) return;
    }
}

void draw_text(Window win, int gc, int x, char *string, int len) {
    if(font.fontset)
        XmbDrawString(dis, win, font.fontset, theme[gc].gc, x, font.fh, string, len);
    else
        XDrawString(dis, win, theme[gc].gc, x, font.fh, string, len);
}

void status_text(char *sb_text) {
    int text_length, text_start, blank_start, wsize, count = 0, wnl;
    char win_name[256];

    XFillRectangle(dis, area_sb, bggc, 0, 0, pos, sb_height+4);
    if(strlen(sb_text) < 1) sb_text = "snapwm";
    if(head == NULL) sb_text = "snapwm";
    while(sb_text[count] != '\0' && count < windownamelength) {
        win_name[count] = sb_text[count];
        count++;
    }
    win_name[count] = '\0';
    wnl = windownamelength*font.width;
    wsize = wc_size(win_name);
    text_length = (wsize >= wnl) ? wnl : wsize;
    blank_start = wc_size(theme[mode].modename)+(2*font.width);
    pos = blank_start+(2*font.width)+wnl;
    text_start = (LA_WINDOWNAME < 1) ? blank_start+(2*font.width) : pos - text_length;

    draw_text(area_sb, 3, font.width*2, theme[mode].modename, strlen(theme[mode].modename));
    draw_text(area_sb, 3, text_start, win_name, count);
    XCopyArea(dis, area_sb, sb_area, theme[1].gc, 0, 0, pos, sb_height+4, 0, 0);
}

void update_output(int messg) {
    unsigned int text_length=0, p_length, text_start, i, j=2, k=0;
    unsigned int wsize, n;
    char output[256];
    char astring[256];
    int status;
    XTextProperty text_prop;

    status = XGetWMName(dis, root, &text_prop);
    if (!status || !text_prop.value || !text_prop.nitems) {
        strcpy(astring, "What's going on here then?");
        if(messg == 0)
            logger("\033[0;31m Failed to get status output. \n");
    } else {
        strncpy(astring, (char *)text_prop.value, 255);
    }
    while(astring[text_length] != '\0' && text_length < 256) {
        output[text_length] = astring[text_length];
        ++text_length;
    }
    output[text_length] = '\0';

    if(text_prop.value) XFree(text_prop.value);

    for(n=0;n<text_length;n++) {
        while(output[n] == '&') {
            if(output[n+1]-'0' < 7 && output[n+1]-'0' > 0) {
                n += 2;
            } else break;
        }
        astring[k] = output[n]; k++;
    }
    astring[k] = '\0';
    p_length = wc_size(astring);
    text_start = total_w - p_length;
    XFillRectangle(dis, area_sb, bggc, pos, 0, total_w-pos, sb_height+4);
    k = 0; // i=pos on screen k=pos in text
    for(i=pos;i<total_w;i++) {
        if(i+font.width < text_start) {
            //draw_text(area_sb, 0, i, " ", 1);
            i += font.width-1;
        } else if(k <= text_length) { 
            while(output[k] == '&') {
                if(output[k+1]-'0' < 7 && output[k+1]-'0' >= 0) {
                    j = output[k+1]-'0';
                    k += 2;
                } else break;
            }
            n = 0;
            if(output[k] == '&') {
                astring[0] = '&';
                n++;k++;
            }
            while(output[k] != '&' && output[k] != '\0' && output[k] != '\n' && output[k] != '\r') {
                astring[n] = output[k];
                n++;k++;
            }
            if(n < 1)
                continue;
            astring[n] = '\0';
            wsize = wc_size(astring);
            draw_text(area_sb, j, i, astring, n);
            i += wsize-1;
            for(n=0;n<256;n++)
                astring[n] = '\0';
        }
    }

    XCopyArea(dis, area_sb, sb_area, theme[1].gc, pos, 0, (total_w - pos), sb_height+4, pos, 0);
    for(n=0;n<256;n++)
        output[n] ='\0';
    XSync(dis, False);
    return;
}

unsigned int wc_size(char *string) {
    int num;
    XRectangle rect;

    num = strlen(string);
    XmbTextExtents(font.fontset, string, num, NULL, &rect);
    return rect.width;
}
