// readrc.c [ 0.3.1 ]

/* *********************** Read Config File ************************ */
void read_rcfile() {
    FILE *rcfile ;
    char buffer[80]; /* Way bigger that neccessary */
    char dummy[80];
    char *dummy2;
    char *dummy3;
    int i;

    rcfile = fopen( RCFILE, "r" ) ;
    if ( rcfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,RCFILE);
        set_defaults();
        return;
    } else {
        while(fgets(buffer,sizeof buffer,rcfile) != NULL) {
            /* Now look for info */
            if(strstr(buffer, "THEME" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                for(i=0;i<9;i++) {
                    dummy3 = strsep(&dummy2, ",");
                    if(getcolor(dummy3) == 1) {
                        theme[i].color = getcolor(defaultcolor[i]);
                        logger("Default colour");
                    } else
                        theme[i].color = getcolor(dummy3);
                }
                for(i=0;i<81;i++) dummy[i] = '\0';
                continue;
            }
            if(strstr(buffer, "UF_WIN_ALPHA" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                ufalpha = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "BORDERWIDTH" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                bdw = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "MASTERSIZE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                msize = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "ATTACHASIDE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                attachaside = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "FOLLOWMOUSE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                followmouse = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "CLICKTOFOCUS" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                clicktofocus = atoi(dummy);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(strstr(buffer, "DEFAULTMODE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                mode = atoi(dummy);
                for(i=0;i<DESKTOPS;i++) desktops[i].mode = mode;
                Arg a = {.i = mode}; switch_mode(a);
                for(i=0;i<81;i++) dummy[i] = '\0'; continue;
            }
            if(STATUS_BAR == 0) {
                if(strstr(buffer, "TOPBAR" ) != NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    topbar = atoi(dummy);
                    for(i=0;i<81;i++) dummy[i] = '\0'; continue;
                }
                if(strstr(buffer, "MODENAME" ) != NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<4;i++) {
                        dummy3 = strsep(&dummy2, ",");
                        if(strlen(dummy3) < 1)
                            theme[i].modename = strdup(defaultmodename[i]);
                        else
                            theme[i].modename = strdup(dummy3);
                    }
                    continue;
                }
                if(strstr(buffer,"FONTNAME" ) != NULL) {
                    strncpy(fontbarname, strstr(buffer, " ")+2, strlen(strstr(buffer, " ")+2)-2);
                    fontbar = XLoadQueryFont(dis, fontbarname);
                    if (!fontbar) {
                        fprintf(stderr,"\033[0;34m :: snapwm :\033[0;31m unable to load preferred fontbar: %s using fixed", fontbarname);
                        fontbar = XLoadQueryFont(dis, "fixed");
                    } else {
                        logger("\033[0;32m fontbar Loaded");
                    }
                    sb_height = fontbar->ascent+fontbar->descent+2;
                    continue;
                }
                if(strstr(buffer, "DESKTOP_NAMES") !=NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<DESKTOPS;i++) {
                        dummy3 = strsep(&dummy2, ",");
                        if(strlen(dummy3) < 1)
                            sb_bar[i].label = strdup("?");
                        else
                            sb_bar[i].label = strdup(dummy3);
                    }
                    continue;
                }
            }
        }
        fclose(rcfile);
    }
    if(STATUS_BAR == 0) {
        // Screen height
        sh = (XDisplayHeight(dis,screen) - (sb_height+4+bdw));
        sw = XDisplayWidth(dis,screen) - bdw;
    } else {
        sh = (XDisplayHeight(dis,screen) - bdw);
        sw = XDisplayWidth(dis,screen) - bdw;
    }
    return;
}

void set_defaults() {
    int i;

    logger("\033[0;32m Setting default values");
    for(i=0;i<9;i++)
        theme[i].color = getcolor(defaultcolor[i]);
    if(STATUS_BAR == 0) {
        for(i=0;i<4;i++)
            theme[i].modename = strdup(defaultmodename[i]);
        for(i=0;i<DESKTOPS;i++)
            sb_bar[i].label = strdup("?");
        fprintf(stderr,"\033[0;34m :: snapwm :\033[0;31m no preferred font: *%s* using default fixed\n", fontbarname);
        fontbar = XLoadQueryFont(dis, "fixed");
        sb_height = fontbar->ascent+fontbar->descent+2;
        sh = (XDisplayHeight(dis,screen) - (sb_height+4+bdw));
        sw = XDisplayWidth(dis,screen) - bdw;
    } else {
        sh = (XDisplayHeight(dis,screen) - bdw);
        sw = XDisplayWidth(dis,screen) - bdw;
    }
    return;
}

void update_config() {
    int i, y;
    
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    XFreeFont(dis, fontbar);
    fontbar = NULL;
    for(i=0;i<81;i++) fontbarname[i] = '\0';
    read_rcfile();
    if(topbar == 0) y = 0;
    else y = sh+bdw;
    if(STATUS_BAR == 0) {
        setup_status_bar();
        for(i=0;i<DESKTOPS;i++) {
            XSetWindowBorder(dis,sb_bar[i].sb_win,theme[3].color);
            XMoveResizeWindow(dis, sb_bar[i].sb_win, i*sb_width, y,sb_width-2,sb_height);
        }
        XSetWindowBorder(dis,sb_area,theme[3].color);
        XSetWindowBackground(dis, sb_area, theme[1].color);
        XMoveResizeWindow(dis, sb_area, sb_desks, y, sw-(sb_desks+2),sb_height);
    }
    for(i=0;i<DESKTOPS;i++) {
        if(desktops[i].mode == 2) desktops[i].master_size = (sh*msize)/100;
        else desktops[i].master_size = (sw*msize)/100;
    }
    tile();
    update_current();
    if(STATUS_BAR == 0) update_bar();
    grabkeys();
}
