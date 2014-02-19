// readrc.c [ 0.7.9 ]

/* *********************** Read Config File ************************ */
void read_rcfile() {
    FILE *rcfile ;
    char buffer[256];
    char dummy[256];
    char *dummy2, *dummy3;
    unsigned int i; int j=-1;

    rcfile = fopen( RC_FILE, "r" ) ;
    if ( rcfile == NULL ) {
        fprintf(stderr, "\033[0;34m:: snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,RC_FILE);
        set_defaults();
        return;
    } else {
        while(fgets(buffer,sizeof buffer,rcfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#') continue;
            /* Now look for info */
            if(strstr(buffer, "WINDOWTHEME" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                for(i=0;i<2;++i) {
                    dummy3 = strsep(&dummy2, ";");
                    if(getcolor(dummy3) == 1) {
                        theme[i].wincolor = getcolor(defaultwincolor[i]);
                        logger("Default colour");
                    } else
                        theme[i].wincolor = getcolor(dummy3);
                }
            } else if(strstr(buffer, "RESIZEMOVEKEY" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                //dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                if(strncmp(dummy2, "Super", 5) == 0)
                    resizemovekey = Mod4Mask;
                else
                    resizemovekey = Mod1Mask;
            } else if(strstr(buffer, "DESKTOPS" ) != NULL) {
                DESKTOPS = atoi(strstr(buffer, " ")+1);
                if(DESKTOPS > 10) DESKTOPS = 10;
            } else if(strstr(buffer, "UF_WIN_ALPHA" ) != NULL) {
                ufalpha = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "BAR_ALPHA" ) != NULL) {
                baralpha = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "CENTER_STACK" ) != NULL) {
                cstack = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "BORDERWIDTH" ) != NULL) {
                bdw = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "MASTERSIZE" ) != NULL) {
                msize = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "ATTACHASIDE" ) != NULL) {
                attachaside = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "TOPSTACK" ) != NULL) {
                top_stack = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "FOLLOWMOUSE" ) != NULL) {
                followmouse = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "LEFT_WINDOWNAME" ) != NULL) {
                LA_WINDOWNAME = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "CLICKTOFOCUS" ) != NULL) {
                clicktofocus = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "NMASTER" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                for(i=0;i<DESKTOPS; ++i) {
                    j = atoi(strsep(&dummy2, ";"));
                    if(j > -1 && j < 10) desktops[i].nmaster = j;
                    else desktops[i].nmaster = 0;
                    if(dummy2 == NULL) break;
                }
            } else if(strstr(buffer, "AUTO_NUM_OPEN" ) != NULL) {
                auto_num = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "AUTO_MODE" ) != NULL) {
                auto_mode = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "DEFAULTMODE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                for(i=0;i<DESKTOPS; ++i) {
                    if(dummy2 == NULL) break;
                    j = atoi(strsep(&dummy2, ";"));
                    if(j > -1 && j < 5)
                        desktops[i].mode = j;
                }
            } else if(STATUS_BAR == 0) {
                if(strstr(buffer, "SWITCHERTHEME" ) != NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<4;++i) {
                        dummy3 = strsep(&dummy2, ";");
                        if(getcolor(dummy3) == 1) {
                            theme[i].barcolor = getcolor(defaultbarcolor[i]);
                            logger("Default bar colour");
                        } else
                            theme[i].barcolor = getcolor(dummy3);
                    }
                } else if(strstr(buffer, "STATUSTHEME" ) != NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<10;++i) {
                        dummy3 = strsep(&dummy2, ";");
                        if(getcolor(dummy3) == 1) {
                            theme[i].textcolor = getcolor(defaulttextcolor[i]);
                            logger("Default text colour");
                        } else
                            theme[i].textcolor = getcolor(dummy3);
                    }
                } else if(strstr(buffer, "BAR_MONITOR" ) != NULL) {
                    barmonchange = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "BAR_SHORT" ) != NULL) {
                    lessbar = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "SHOWNUMOPEN" ) != NULL) {
                    showopen = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "WNAMEBG" ) != NULL) {
                    wnamebg = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "TOPBAR" ) != NULL) {
                    topbar = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "SHOW_BAR" ) != NULL) {
                    show_bar = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "MODENAME" ) != NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<5;++i) {
                        dummy3 = strsep(&dummy2, ";");
                        if(strlen(dummy3) < 1)
                            theme[i].modename = strdup(defaultmodename[i]);
                        else
                            theme[i].modename = strdup(dummy3);
                    }
                } else if(strstr(buffer,"FONTNAME" ) != NULL) {
                    memset(font_list, '\0', 256);
                    strncpy(font_list, strstr(buffer, " ")+2, strlen(strstr(buffer, " ")+2)-2);
                    get_font();
                    sb_height = font.height+2;
                    font.fh = ((sb_height - font.height)/2) + font.ascent;
                } else if(strstr(buffer, "WINDOWNAMELENGTH" ) != NULL) {
                    windownamelength = atoi(strstr(buffer, " ")+1);
                } else if(strstr(buffer, "DESKTOP_NAMES") !=NULL) {
                    strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                    dummy[strlen(dummy)-1] = '\0';
                    dummy2 = strdup(dummy);
                    for(i=0;i<DESKTOPS;++i) {
                        dummy3 = strsep(&dummy2, ";");
                        if(!(dummy3)) {
                            if(!(defaultdesktopnames[i]))
                                sb_bar[i].label = strdup("?");
                            else sb_bar[i].label = strdup(defaultdesktopnames[i]);
                        } else sb_bar[i].label = strdup(dummy3);
                    }
                }
            }
            memset(buffer, '\0', 256);
            memset(dummy, '\0', 256);
        }
        fclose(rcfile);
        opacity = (ufalpha/100.00) * 0xffffffff;
        baropacity = (baralpha/100.00) * 0xffffffff;
    }
    return;
}

void get_font() {
	char *def, **missing;
	int i, n;

	missing = NULL;
	if(strlen(font_list) > 0)
	    font.fontset = XCreateFontSet(dis, (char *)font_list, &missing, &n, &def);
	if(missing) {
		if(FONTS_ERROR < 1)
            while(--n)
                fprintf(stderr, ":: snapwm :: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(font.fontset) {
		XFontStruct **xfonts;
		char **font_names;

		font.ascent = font.descent = 0;
		n = XFontsOfFontSet(font.fontset, &xfonts, &font_names);
		for(i = 0, font.ascent = 0, font.descent = 0; i < n; ++i) {
			if (font.ascent < (*xfonts)->ascent) font.ascent = (*xfonts)->ascent;
            if (font.descent < (*xfonts)->descent) font.descent = (*xfonts)->descent;
			++xfonts;
		}
		font.width = XmbTextEscapement(font.fontset, " ", 1);
	} else {
		fprintf(stderr, ":: snapwm :: Font '%s' Not Found\n:: snapwm :: Trying Font 'Fixed'\n", font_list);
		if(!(font.font = XLoadQueryFont(dis, font_list))
		&& !(font.font = XLoadQueryFont(dis, "fixed")))
			fprintf(stderr, ":: snapwm :: Error, cannot load font: '%s'\n", font_list);
		font.ascent = font.font->ascent;
		font.descent = font.font->descent;
		font.width = XTextWidth(font.font, " ", 1);
	}
	font.height = font.ascent + font.descent;
}

void set_defaults() {
    unsigned int i;

    for(i=0;i<2;++i)
        theme[i].wincolor = getcolor(defaultwincolor[i]);
    if(STATUS_BAR == 0) {
        for(i=0;i<4;++i)
            theme[i].barcolor = getcolor(defaultbarcolor[i]);
        for(i=0;i<5;++i)
            theme[i].modename = strdup(defaultmodename[i]);
        for(i=0;i<10;++i) {
            if(!(defaultdesktopnames[i]))
                sb_bar[i].label = strdup("?");
            else sb_bar[i].label = strdup(defaultdesktopnames[i]);
            theme[i].textcolor = getcolor(defaulttextcolor[i]);
            desktops[i].mode = mode;
        }
        strncpy(font_list, defaultfontlist, strlen(defaultfontlist));
        get_font();
        sb_height = font.height+2;
        font.fh = ((sb_height - font.height)/2) + font.ascent;
    }
    logger("\033[0;32m Setting default values");
    return;
}

void update_config() {
    unsigned int i, y, old_desktops = DESKTOPS, tmp = current_desktop;
    
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    XUngrabButton(dis, AnyButton, AnyModifier, root);
    if(font.fontset) XFreeFontSet(dis, font.fontset);
    read_rcfile();
    y = (topbar == 0) ? 0 : desktops[barmon].h+bdw;
    if(DESKTOPS < old_desktops) {
        save_desktop(current_desktop);
        Arg a = {.i = DESKTOPS-1};
        for(i=DESKTOPS;i<old_desktops;++i) {
            select_desktop(i);
            if(head != NULL) {
                while(desktops[current_desktop].numwins > 0) {
                    client_to_desktop(a);
                }
            }
        }
        select_desktop(tmp);
        if(current_desktop > (DESKTOPS-1)) change_desktop(a);
    }
    if(old_desktops < DESKTOPS || barmon != barmonchange)
        init_desks();
    if(STATUS_BAR == 0) {
        setup_status_bar();
        if(DESKTOPS != old_desktops) {
            for(i=0;i<old_desktops;++i)
                XDestroyWindow(dis, sb_bar[i].sb_win);
            XDestroyWindow(dis, sb_area);
            status_bar();
        } else {
            sb_width = 0;
            for(i=0;i<DESKTOPS;++i) {
                XSetWindowBorder(dis,sb_bar[i].sb_win,theme[3].barcolor);
                XMoveResizeWindow(dis, sb_bar[i].sb_win, desktops[barmon].x+sb_width, y,sb_bar[i].width-2,sb_height);
                sb_width += sb_bar[i].width;
            }
            XSetWindowBorder(dis,sb_area,theme[3].barcolor);
            XSetWindowBackground(dis, sb_area, theme[1].barcolor);
            XMoveResizeWindow(dis, sb_area, desktops[barmon].x+sb_desks, y, desktops[barmon].w-(sb_desks+4)+bdw-lessbar,sb_height);
            XGetWindowAttributes(dis, sb_area, &attr);
            total_w = attr.width;
            if(area_sb != 0) {
                XFreePixmap(dis, area_sb);
            }
            area_sb = XCreatePixmap(dis, root, total_w, sb_height+4, DefaultDepth(dis, screen));
            XFillRectangle(dis, area_sb, theme[0].gc, 0, 0, total_w, sb_height+4);
        }
    }
    if(mode == 1 && head != NULL) {
        client *c;
        XUnmapWindow(dis, current->win);
        for(c=head;c;c=c->next) XMapWindow(dis, c->win);
    }
    for(i=0;i<DESKTOPS;++i)
        desktops[i].master_size = (desktops[i].mode == 2) ? (desktops[i].h*msize)/100 : (desktops[i].w*msize)/100;
    if(numwins < 1) {
        Arg a = {.i = desktops[current_desktop].mode};
        switch_mode(a);
        select_desktop(current_desktop);
    }
    if(STATUS_BAR == 0) update_bar();
    update_current();
    setbaralpha();

    read_apps_file();
    memset(keys, 0, keycount * sizeof(key));
    memset(cmds, 0, cmdcount * sizeof(Commands));
    grabkeys();
}
