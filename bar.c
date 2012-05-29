// bar.c [ 0.4.8 ]

/* ************************** Status Bar *************************** */
static int total_w;
static int pos;
void setup_status_bar() {
    int i, extra_width;
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
            values.font = fontbar->fid;
            theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
        }
    }

    if(showopen < 1) extra_width = 2;
    else extra_width = 0;
    sb_width = 0;
    for(i=0;i<DESKTOPS;i++) {
        sb_bar[i].width = (wc_size(sb_bar[i].label) + extra_width)*font.width;
        //sb_bar[i].width = XTextWidth(fontbar, sb_bar[i].label, strlen(sb_bar[i].label)+extra_width);
        if(sb_bar[i].width > sb_width)
            sb_width = sb_bar[i].width;
    }
    sb_width += 4;
    if(sb_width < sb_height) sb_width = sb_height*3/2;
    sb_desks = (DESKTOPS*sb_width)+2;
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
    XGetWindowAttributes(dis, sb_area, &attr);
    //printf("SCREEN_WIDTH = %d\n", attr.width);
    total_w = attr.width/font.width;
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
        XFetchName(dis, current->win, &win_name);
        status_text(win_name);
        XFree(win_name);
    } else status_text("");
}

void update_bar() {
    int i, j=0;
    char busylabel[20];

    for(i=0;i<DESKTOPS;i++) {
        if(showopen == 0 && (desktops[i].numwins < 2 || i == current_desktop)) j = font.width;
        if(i != current_desktop) {
            if(desktops[i].head != NULL) {
                if(showopen < 1 && desktops[i].numwins > 1)
                    sprintf(busylabel, "%d:%s", desktops[i].numwins, sb_bar[i].label);
                else {
                    j = font.width;
                    sprintf(busylabel, "%s", sb_bar[i].label);
                }
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[2].barcolor);
                XClearWindow(dis, sb_bar[i].sb_win);
                XmbDrawString(dis, sb_bar[i].sb_win, font.fontset, theme[2].gc, (sb_width-sb_bar[i].width)/2, font.fh, busylabel, strlen(busylabel));
                //XDrawString(dis, sb_bar[i].sb_win, theme[2].gc, (sb_width-XTextWidth(fontbar, busylabel,strlen(busylabel)))/2, fontbar->ascent+1, busylabel, strlen(busylabel));
            } else {
                sprintf(busylabel, "%s", sb_bar[i].label);
                XSetWindowBackground(dis, sb_bar[i].sb_win, theme[1].barcolor);
                XClearWindow(dis, sb_bar[i].sb_win);
                XmbDrawString(dis, sb_bar[i].sb_win, font.fontset, theme[1].gc, ((sb_width-sb_bar[i].width)/2)+j, font.fh, busylabel, strlen(busylabel));
                //XDrawString(dis, sb_bar[i].sb_win, theme[1].gc, (sb_width-XTextWidth(fontbar, busylabel,strlen(busylabel)))/2, fontbar->ascent+1, busylabel, strlen(busylabel));
            }
        } else {
            if(showopen < 1 && desktops[i].mode == 1 && desktops[i].numwins > 1)
                sprintf(busylabel, "%d:%s", desktops[i].numwins, sb_bar[i].label);
            else
                sprintf(busylabel, "%s", sb_bar[i].label);
            XSetWindowBackground(dis, sb_bar[i].sb_win, theme[0].barcolor);
            XClearWindow(dis, sb_bar[i].sb_win);
            XmbDrawString(dis, sb_bar[i].sb_win, font.fontset, theme[0].gc, ((sb_width-sb_bar[i].width)/2)+j, font.fh, busylabel, strlen(busylabel));
            //XDrawString(dis, sb_bar[i].sb_win, theme[0].gc, (sb_width-XTextWidth(fontbar, busylabel,strlen(busylabel)))/2, fontbar->ascent+1, busylabel, strlen(busylabel));
        }
    }
}

void status_text(char *sb_text) {
    int text_length, text_start, blank_start, i;

    if(sb_text == NULL) sb_text = "snapwm";
    if(head == NULL) sb_text = "snapwm";
    if(strlen(sb_text) >= windownamelength)
        text_length = windownamelength;
    else
        text_length = wc_size(sb_text);
    blank_start = wc_size(theme[mode].modename);
    pos = blank_start+4+windownamelength;
    text_start = pos - text_length;
    
    XmbDrawImageString(dis, sb_area, font.fontset, theme[3].gc, font.width*2, font.fh, theme[mode].modename, strlen(theme[mode].modename));
    for(i=blank_start+2;i<text_start;i++)
        XmbDrawImageString(dis, sb_area, font.fontset, theme[0].gc, font.width*i, font.fh, " ", 1);
    XmbDrawImageString(dis, sb_area, font.fontset, theme[3].gc, font.width*text_start, font.fh, sb_text, text_length);
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
    //printf("out=%s,k=%d,n=%d\n", astring,k,n);
    //printf("pos=%d,start=%d,length=%d,total=%d,desks=%d,swdth=%d\n", pos, text_start, n, total_w, sb_desks, sw/font.width);
    k = 0; // i=pos on screen k=pos in text
    for(i=pos;i<total_w;i++) {
        if(i < text_start || i > pos+text_start+p_length) {
            XmbDrawImageString(dis, sb_area, font.fontset, theme[0].gc, i*font.width, font.fh, " ", 1);

        } else { 
            while(output[k] == '&') {
                if(output[k+1]-'0' < 7 && output[k+1]-'0' > 0) {
                    j = output[k+1]-'0';
                    if(j > 1 || j < 7) {
                        j--;
                    } else  j = 2;
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
            if(n < 1) return;
            astring[n] = '\0';
            wsize = wc_size(astring);
            if(font.fontset)
                XmbDrawImageString(dis, sb_area, font.fontset, theme[j].gc, i*font.width, font.fh, astring, n);
                //XmbDrawImageString(dis, sb_area, font.fontset, theme[j].gc, (total_w-10)*font.width, font.fh, "####*####*", 10);
           else
                XDrawImageString(dis, sb_area, theme[1].gc, i*font.width, font.fh, astring, n);
            i += wsize-1;
            for(n=0;n<256;n++)
                astring[n] = '\0';
        }
    }

    output[0] ='\0';
    XSync(dis, False);
    return;
}

unsigned int wc_size(char *string) {
    wchar_t *wp;
    int num, len, wlen, wsize;

    num = strlen(string);
    len = num * sizeof(wchar_t);
    if(!(wp = (wchar_t *)malloc(1+len))) {
        logger("WC Malloc Failed");
        return num;
    }
    wlen = mbstowcs(wp, string, len);
    wsize = wcswidth(wp, wlen);
    if(wsize < 1) {
        wsize = num;
        logger("\twsize fail");
    }
    //printf("astr=%s,wsz=%d", string, wsize);
    free(wp);
    return wsize;
}
