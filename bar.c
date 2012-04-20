// bar.c [ 0.4.5 ]

/* ************************** Status Bar *************************** */
static int sb_end;
void setup_status_bar() {
    int i, extra_width;
    XGCValues values;

    logger(" \033[0;33mStatus Bar called ...");

    for(i=0;i<5;i++) {
        values.background = theme[1].barcolor;
        values.foreground = theme[i+4].barcolor;
        values.line_width = 2;
        values.line_style = LineSolid;
        values.font = fontbar->fid;
        theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
    }

    if(showopen < 1) extra_width = 2;
    else extra_width = 0;
    sb_width = 0;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].width = XTextWidth(fontbar, sb_bar[i].label, strlen(sb_bar[i].label)+extra_width);
        if(sb_bar[i].width > sb_width)
            sb_width = sb_bar[i].width;
    }
    sb_width += 4;
    if(sb_width < sb_height) sb_width = sb_height;
    sb_desks = (DESKTOPS*sb_width)+2;
    sb_end = XTextWidth(fontbar, " ", (strlen(theme[mode].modename)+4+windownamelength));
    //printf("\tSB END == %d\n", sb_end);
}

void status_bar() {
    int i, y;

    if(topbar == 0) y = 0;
    else y = sh+bdw;
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

void update_bar() {
    int i;
    char busylabel[20];

    for(i=0;i<DESKTOPS;i++) {
        if(i != current_desktop) {
            if(desktops[i].head != NULL) {
                if(showopen < 1 && desktops[i].numwins > 1)
                    sprintf(busylabel, "%d:%s", desktops[i].numwins, sb_bar[i].label);
                else sprintf(busylabel, "%s", sb_bar[i].label);
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[2].barcolor);
            } else {
                sprintf(busylabel, "%s", sb_bar[i].label);
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[1].barcolor);
            }
        } else {
            if(showopen < 1 && desktops[i].mode == 1 && desktops[i].numwins > 1)
                sprintf(busylabel, "%d:%s", desktops[i].numwins, sb_bar[i].label);
            else sprintf(busylabel, "%s", sb_bar[i].label);
            XSetWindowBackground(dis, sb_bar[i].sb_win, theme[0].barcolor);
        }
        XClearWindow(dis, sb_bar[i].sb_win);
        XDrawString(dis, sb_bar[i].sb_win, theme[1].gc, (sb_width-XTextWidth(fontbar, busylabel,strlen(busylabel)))/2, fontbar->ascent+1, busylabel, strlen(busylabel));
    }
}

void status_text(const char *sb_text) {
    int text_length, text_start, blank_start, text_total, i;

    if(sb_text == NULL) sb_text = "snapwm";
    if(head == NULL) sb_text = "snapwm";
    if(strlen(sb_text) >= windownamelength)
        text_length = windownamelength;
    else
        text_length = strlen(sb_text);
    text_total = (strlen(theme[mode].modename)+4+windownamelength); //sb_end/XTextWidth(fontbar, " ", 1);
    text_start = text_total - text_length;
    blank_start = strlen(theme[mode].modename)+2;

    XDrawImageString(dis, sb_area, theme[0].gc, XTextWidth(fontbar, " ", 2), fontbar->ascent+1, theme[mode].modename, strlen(theme[mode].modename));
    for(i=blank_start;i<text_start;i++)
        XDrawImageString(dis, sb_area, theme[0].gc, XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    XDrawImageString(dis, sb_area, theme[0].gc, XTextWidth(fontbar, " ", text_start), fontbar->ascent+1, sb_text, text_length);
    //XDrawImageString(dis, sb_area, theme[0].gc, sb_end, fontbar->ascent+1, "H", 1);
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
    text_space = (sw-(sb_desks+sb_end+bdw+XTextWidth(fontbar, " ", k)))/XTextWidth(fontbar, " ", 1);
    if(text_space > 0)
        text_start = sb_end+XTextWidth(fontbar, " ", text_space);
    else
        text_start = sb_end;

    for(i=0;i<text_space;i++)
        XDrawImageString(dis, sb_area, theme[1].gc, sb_end+XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    k = 0;
    for(i=0;i<text_length;i++) {
        if(strncmp(&output[i], "&", 1) == 0) {
            j = output[i+1]-'0';
            i += 2;
        }
        XDrawImageString(dis, sb_area, theme[j].gc, text_start+XTextWidth(fontbar, " ", k), fontbar->ascent+1, &output[i], 1);
        k++;
    }
    output[0] ='\0';
    XSync(dis, False);
    return;
}
