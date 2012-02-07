// bar.c [ 0.3.6 ]

/* ************************** Status Bar *************************** */
static int sb_end;
void setup_status_bar() {
    int i;
    XGCValues values;

    logger(" \033[0;33mStatus Bar called ...");

    for(i=0;i<5;i++) {
        values.background = theme[1].color;
        values.foreground = theme[i+4].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        values.font = fontbar->fid;
        theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
    }

    sb_width = 0;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].width = XTextWidth(fontbar, sb_bar[i].label, strlen(sb_bar[i].label)+1);
        if(sb_bar[i].width > sb_width)
            sb_width = sb_bar[i].width;
    }
    sb_width += 4;
    if(sb_width < sb_height) sb_width = sb_height;
    sb_desks = (DESKTOPS*sb_width)+2;
    sb_end = XTextWidth(fontbar, " ", (strlen(theme[mode].modename)+40));
}

void status_bar() {
    int i, y;

    if(topbar == 0) y = 0;
    else y = sh+bdw;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].sb_win = XCreateSimpleWindow(dis, root, i*sb_width, y,
                                            sb_width-2,sb_height,2,theme[3].color,theme[0].color);

        XSelectInput(dis, sb_bar[i].sb_win, ButtonPressMask|EnterWindowMask);
        XMapRaised(dis, sb_bar[i].sb_win);
    }
    sb_area = XCreateSimpleWindow(dis, root, sb_desks, y,
             sw-(sb_desks+2)+bdw,sb_height,2,theme[3].color,theme[1].color);

    XSelectInput(dis, sb_area, ExposureMask|EnterWindowMask);
    XMapRaised(dis, sb_area);
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

        tile();
        update_current();
        update_bar();
    }
}

void getwindowname() {
    char *win_name;

    if(head != NULL) {
        XFetchName(dis, current->win, &win_name);
        status_text(win_name);
        XFree(win_name);
    } else status_text("");
}

void status_text(const char *sb_text) {
    int text_length, text_start;

    if(sb_text == NULL) sb_text = "snapwm";
    if(head == NULL) sb_text = "snapwm";
    if(strlen(sb_text) >= 35)
        text_length = 35;
    else
        text_length = strlen(sb_text);
    text_start = 10+(XTextWidth(fontbar, theme[mode].modename, strlen(theme[mode].modename)))+(XTextWidth(fontbar, " ", 35))-(XTextWidth(fontbar, sb_text, text_length));

    XClearArea(dis, sb_area,0,0,XTextWidth(fontbar, " ", (strlen(theme[mode].modename)+40)), sb_height, False);
    XDrawString(dis, sb_area, theme[0].gc, 5, fontbar->ascent+1, theme[mode].modename, strlen(theme[mode].modename));
    XDrawString(dis, sb_area, theme[0].gc, text_start, fontbar->ascent+1, sb_text, text_length);
}

void update_bar() {
    int i;
    char busylabel[20];

    for(i=0;i<DESKTOPS;i++)
        if(i != current_desktop) {
            if(desktops[i].head != NULL) {
                strcpy(busylabel, "*"); strcat(busylabel, sb_bar[i].label);
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[2].color);
                XClearWindow(dis, sb_bar[i].sb_win);
                XDrawString(dis, sb_bar[i].sb_win, theme[1].gc, (sb_width-XTextWidth(fontbar, busylabel,strlen(busylabel)))/2, fontbar->ascent+1, busylabel, strlen(busylabel));
            } else {
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[1].color);
                XClearWindow(dis, sb_bar[i].sb_win);
                XDrawString(dis, sb_bar[i].sb_win, theme[1].gc, (sb_width-sb_bar[i].width)/2, fontbar->ascent+1, sb_bar[i].label, strlen(sb_bar[i].label));
            }
        } else {
            XSetWindowBackground(dis, sb_bar[i].sb_win, theme[0].color);
            XClearWindow(dis, sb_bar[i].sb_win);
            XDrawString(dis, sb_bar[i].sb_win, theme[1].gc, (sb_width-sb_bar[i].width)/2, fontbar->ascent+1, sb_bar[i].label, strlen(sb_bar[i].label));
        }
}

void update_output(int messg) {
    int text_length, text_start, i, j=2, k=0;
    int text_space;
    char output[256];
    char *win_name;

    if(!(XFetchName(dis, root, &win_name))) {
        strcpy(output, "What's going on here then?");
        if(messg == 0)
            logger("\033[0;31m Failed to get status output. \n");
    } else {
        strncpy(output, win_name, strlen(win_name));
        output[strlen(win_name)] = '\0';
    }
    XFree(win_name);

    if(strlen(output) > 255) text_length = 255;
    else text_length = strlen(output);
    for(i=0;i<text_length;i++) {
        k++;
        if(strncmp(&output[i], "&", 1) == 0)
            i += 2;
    }
    text_space = 2+(sw-(sb_desks+sb_end+XTextWidth(fontbar, " ", k)+20))/XTextWidth(fontbar, " ", 1);
    if(text_space > 0)
        text_start = sb_end+(sw-(sb_desks+sb_end+XTextWidth(fontbar, output, k)+20));
    else
        text_start = sb_end;

    for(i=1;i<text_space;i++)
        XDrawImageString(dis, sb_area, theme[1].gc, sb_end+XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    k = 0;
    for(i=0;i<text_length;i++) {
        k++;
        if(strncmp(&output[i], "&", 1) == 0) {
            j = output[i+1]-'0';
            i += 2;
        }
        XDrawImageString(dis, sb_area, theme[j].gc, text_start+XTextWidth(fontbar, " ", k), fontbar->ascent+1, &output[i], 1);
    }
    output[0] ='\0';
    XSync(dis, False);
    return;
}
