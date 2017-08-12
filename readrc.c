// readrc.c [ 0.9.1 ]

unsigned int i, k=0, c=0;
int j=-1;

int get_value() {
    memset(dummy, 0, 256); c = 0;
    while(buffer[k] != ';' && buffer[k] != 0 && buffer[k] != '\n') {
        if(buffer[k] == '"') {
            ++k; continue;
        }
        dummy[c] = buffer[k];
        ++c; ++k;
    }
    ++k;
    if(c > 0) {
        ++c; dummy[c] = '\0';
        return 0;
    } else return 1;
}

/* *********************** Read Config File ************************ */
void read_rcfile() {
    FILE *rcfile;

    rcfile = fopen( RC_FILE, "r" );
    if (rcfile == NULL) {
        fprintf(stderr, "\033[0;34m:: readrc : \033[0;31m Couldn't find %s\033[0m \n", RC_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,rcfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n') {
                memset(buffer, '\0', 256);
                continue;
            }
            if(buffer[strlen(buffer)-1] == '\n')
                buffer[strlen(buffer)-1] = 0;
            /* Now look for info */
            if(strstr(buffer, "WINDOWTHEME" ) != NULL) {
                k = 12; // eleven letters in windowtheme plus a space
                for(i=0;i<2;++i) {
                    if(get_value() == 0) {
                        theme[i].wincolor = getcolor(dummy);
                        if(theme[i].wincolor == 1) {
                            theme[i].wincolor = getcolor(defaultwincolor[i]);
                            logger("Default Window Border Colour", "");
                        }
                    }
                }
            } else if(strstr(buffer, "RESIZEMOVEKEY" ) != NULL) {
                k = 14;
                if(get_value() == 0) {
                    if(strncmp(dummy, "Super", 5) == 0)
                        resizemovekey = Mod4Mask;
                    else
                        resizemovekey = Mod1Mask;
                }
            } else if(strstr(buffer, "DESKTOPS" ) != NULL) {
                k = 9;
                if(get_value() == 0) {
                    DESKTOPS = strtol(dummy, NULL, 10);
                    if(DESKTOPS > 12) DESKTOPS = 12;
                }
            } else if(strstr(buffer, "DEFAULT_DESK" ) != NULL) {
                k = 13;
                if(get_value() == 0)
                    default_desk = strtol(dummy, NULL, 10);
                if(default_desk > DESKTOPS) default_desk = DESKTOPS;
            } else if(strstr(buffer, "UF_WIN_ALPHA" ) != NULL) {
                k = 13;
                if(get_value() == 0) {
                    ufalpha = strtol(dummy, NULL, 10);
                    if(ufalpha < 1 || ufalpha > 100)
                        ufalpha = 100;
                }
            } else if(strstr(buffer, "BAR_ALPHA" ) != NULL) {
                k = 10;
                if(get_value() == 0) {
                    baralpha = strtol(dummy, NULL, 10);
                    if(baralpha < 1 || baralpha > 100)
                        baralpha = 100;
                }
            } else if(strstr(buffer, "CENTER_STACK" ) != NULL) {
                k = 13;
                if(get_value() == 0) {
                    if(dummy[0] == '0' || dummy[0] == '1')
                        cstack = strtol(dummy, NULL, 10);
                }
            } else if(strstr(buffer, "BORDERWIDTH" ) != NULL) {
                k = 12;
                if(get_value() == 0)
                    bdw = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "MASTERSIZE" ) != NULL) {
                k = 11;
                if(get_value() == 0)
                    msize = strtol(dummy, NULL, 10);
                if(msize > 80 || msize < 20) msize = 50;
            } else if(strstr(buffer, "ATTACHASIDE" ) != NULL) {
                k = 12;
                if(get_value() == 0) 
                    if(dummy[0] == '0' || dummy[0] == '1')
                        attachaside = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "TOPSTACK" ) != NULL) {
                k = 9;
                if(get_value() == 0)
                    if(dummy[0] == '0' || dummy[0] == '1')
                        top_stack = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "FOLLOWMOUSE" ) != NULL) {
                k = 12;
                if(get_value() == 0)
                    if(dummy[0] == '0' || dummy[0] == '1')
                        followmouse = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "LEFT_WINDOWNAME" ) != NULL) {
                k = 16;
                if(get_value() == 0)
                    if(dummy[0] == '0' || dummy[0] == '1')
                        LA_WINDOWNAME = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "CLICKTOFOCUS" ) != NULL) {
                k = 13;
                if(get_value() == 0)
                    if(dummy[0] == '0' || dummy[0] == '1')
                        clicktofocus = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "AUTO_NUM_OPEN" ) != NULL) {
                k = 14;
                if(get_value() == 0)
                    auto_num = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "AUTO_MODE" ) != NULL) {
                k = 10;
                if(get_value() == 0) {
                    k = strtol(dummy, NULL, 10);
                    if(k >= 0 && k < 5)
                        auto_mode = k;
                }
            } else if(strstr(buffer, "DEFAULTMODE" ) != NULL) {
                k = 12;
                for(i=0;i<DESKTOPS; ++i) {
                    if(get_value() == 0) {
                        j = strtol(dummy, NULL, 10);
                        if(j > -1 && j < 5)
                            desktops[i].mode = j;
                    } else desktops[i].mode = desktops[0].mode;
                }
            } else if(strstr(buffer, "NMASTER" ) != NULL) {
                k = 8;
                for(i=0;i<DESKTOPS; ++i) {
                    if(get_value() == 0) {
                        j = strtol(dummy, NULL, 10);
                        if(j > -1 && j < 10) desktops[i].nmaster = j;
                        else desktops[i].nmaster = 0;
                    }
                }
            } else if(strstr(buffer, "MINW_H" ) != NULL) {
                k = 7;
                if(get_value() == 0)
                    minww = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "MINW_W" ) != NULL) {
                k = 7;
                if(get_value() == 0)
                    minwh = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "UG_OUT" ) != NULL) {
                k = 7;
                if(get_value() == 0)
                    ug_out = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "UG_IN" ) != NULL) {
                k = 6;
                if(get_value() == 0)
                    ug_in = strtol(dummy, NULL, 10);
            } else if(strstr(buffer, "UG_BAR" ) != NULL) {
                k = 6;
                if(get_value() == 0)
                    ug_bar = strtol(dummy, NULL, 10);
            } else if(STATUS_BAR == 0) {
                if(strstr(buffer, "SWITCHERTHEME" ) != NULL) {
                    k = 14; XGCValues values;
                    for(i=0;i<5;++i) {
                        if(get_value() == 0)
                            theme[i].barcolor = (getcolor(dummy) == 1)? getcolor(defaultbarcolor[i]):getcolor(dummy);
                        values.foreground = theme[i].barcolor;
                        theme[i].swgc = XCreateGC(dis, root, GCForeground,&values);
                    }
                } else if(strstr(buffer, "STATUSTHEME" ) != NULL) {
                    k = 12;
                    for(i=0;i<10;++i) {
                        if(get_value() == 0) {
                            theme[i].textcolor = getcolor(dummy);
                            if(theme[i].textcolor == 1) {
                                theme[i].textcolor = getcolor(defaulttextcolor[i]);
                                logger("Default text colour", "");
                            }
                        }
                    }
                } else if(strstr(buffer, "BAR_MONITOR" ) != NULL) {
                    k = 12;
                    if(get_value() == 0)
                        barmonchange = strtol(dummy, NULL, 10);
                } else if(strstr(buffer, "BAR_SHORT" ) != NULL) {
                    k = 10;
                    if(get_value() == 0)
                        lessbar = strtol(dummy, NULL, 10);
                } else if(strstr(buffer, "SHOWNUMOPEN" ) != NULL) {
                    k = 12;
                    if(get_value() == 0)
                        if(dummy[0] == '0' || dummy[0] == '1')
                            showopen = strtol(dummy, NULL, 10);
                } else if(strstr(buffer, "WNAMEBG" ) != NULL) {
                    k = 8;
                    if(get_value() == 0) {
                        wnamebg = strtol(dummy, NULL, 10);
                        wnamebg = (wnamebg < 5) ? wnamebg:0;
                    }
                } else if(strstr(buffer, "TOPBAR" ) != NULL) {
                    k = 7;
                    if(get_value() == 0)
                       if(dummy[0] == '0' || dummy[0] == '1')
                           topbar = strtol(dummy, NULL, 10);
                } else if(strstr(buffer, "SHOW_BAR" ) != NULL) {
                    k = 9;
                    for(i=0;i<DESKTOPS; ++i) {
                        if(get_value() == 0) {
                            if(dummy[0] == '0')
                                desktops[i].showbar = 0;
                            else
                                desktops[i].showbar = 1;
                        } else desktops[i].showbar = 0;
                    }
                } else if(strstr(buffer, "WINDOWNAMELENGTH" ) != NULL) {
                    k = 17;
                    if(get_value() == 0)
                        windownamelength = strtol(dummy, NULL, 10);
                } else if(strstr(buffer,"FONTNAME" ) != NULL) {
                    k = 9;
                    if(get_value() == 0) {
                        memset(font_list, '\0', 256);
                        strncpy(font_list, dummy, c);
                        get_font();
                        sb_height = font.height+2;
                        font.fh = ((sb_height - font.height)/2) + font.ascent;
                    }
                } else if(strstr(buffer, "MODENAME" ) != NULL) {
                    k = 9;
                    for(i=0;i<5;++i) {
                        if(get_value() == 0)
                            theme[i].modename = strdup(dummy);
                        else
                            theme[i].modename = strdup(defaultmodename[i]);
                    }
                } else if(strstr(buffer, "DESKTOP_NAMES") !=NULL) {
                    k = 14;
                    for(i=0;i<DESKTOPS;++i) {
                        if(get_value() == 0)
                            sb_bar[i].label = strdup(dummy);
                        else {
                            if(!(defaultdesktopnames[i]))
                                sb_bar[i].label = strdup("?");
                            else sb_bar[i].label = strdup(defaultdesktopnames[i]);
                        }
                    }
                }
            }
            memset(buffer, '\0', 256);
            memset(dummy, '\0', 256);
            k=0; c=0;
        }
    }
    fclose(rcfile);
    opacity = (ufalpha/100.00) * 0xffffffff;
    baropacity = (baralpha/100.00) * 0xffffffff;
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
            while(--n) {
                fprintf(stderr, ":: snapwm :: missing fontset: %s\n", missing[n]);
            }
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
        for(i=0;i<5;++i)
            theme[i].barcolor = getcolor(defaultbarcolor[i]);
        for(i=0;i<5;++i)
            theme[i].modename = strdup(defaultmodename[i]);
        for(i=0;i<12;++i) {
            if(!(defaultdesktopnames[i]))
                sb_bar[i].label = strdup("?");
            else sb_bar[i].label = strdup(defaultdesktopnames[i]);
            desktops[i].mode = mode;
        }
        for(i=0;i<10;++i)
            theme[i].textcolor = getcolor(defaulttextcolor[i]);
        strncpy(font_list, defaultfontlist, strlen(defaultfontlist));
        get_font();
        sb_height = font.height+2;
        font.fh = ((sb_height - font.height)/2) + font.ascent;
    }
    logger("\033[0;32m Setting default values", "");
    return;
}

void update_config() {
    unsigned int i, y, old_desktops = DESKTOPS, tmp = current_desktop;
    unsigned int tmpview = desktops[current_desktop].screen;

    XUngrabKey(dis, AnyKey, AnyModifier, root);
    XUngrabButton(dis, AnyButton, AnyModifier, root);
    if(font.fontset) XFreeFontSet(dis, font.fontset);
    read_rcfile();
    y = (topbar == 0) ? ug_bar : desktops[barmon].h+bdw;
    if(DESKTOPS < old_desktops) {
        save_desktop(current_desktop);
        Arg a = {.i = DESKTOPS-1};
        for(i=DESKTOPS;i<old_desktops;++i) {
            select_desktop(i);
            if(head != NULL) {
                while(desktops[current_desktop].numorder > 0) {
                    client_to_desktop(a);
                }
            }
        }
        select_desktop(tmp);
        if(current_desktop > (DESKTOPS-1)) change_desktop(a);
    }
    init_desks();
    view[tmpview].cd = tmp;
    if(STATUS_BAR == 0) {
        setup_status_bar();
        if(DESKTOPS != old_desktops) {
            for(i=0;i<old_desktops;++i)
                XDestroyWindow(dis, sb_bar[i].sb_win);
            XDestroyWindow(dis, sb_area);
            status_bar();
        } else {
            sb_width = ug_bar;
            for(i=0;i<DESKTOPS;++i) {
                XSetWindowBorder(dis,sb_bar[i].sb_win,theme[3].barcolor);
                XMoveResizeWindow(dis, sb_bar[i].sb_win, desktops[barmon].x+sb_width, y,sb_bar[i].width-2,sb_height);
                sb_width += sb_bar[i].width;
            }
            XSetWindowBorder(dis,sb_area,theme[3].barcolor);
            XSetWindowBackground(dis, sb_area, theme[1].barcolor);
            XMoveResizeWindow(dis, sb_area, desktops[barmon].x+sb_width, y, desktops[barmon].w-(sb_desks+4)+bdw-lessbar-2*ug_bar,sb_height);
            XWindowAttributes attr;
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
        XMoveWindow(dis,current->win,current->x,2*desktops[DESKTOPS-1].h);
    }
    for(i=0;i<DESKTOPS;++i) {
        desktops[i].master_size = (desktops[i].mode == 2) ? (desktops[i].h*msize)/100 : (desktops[i].w*msize)/100;
        if(numwins < 1) {
            Arg a = {.i = desktops[current_desktop].mode};
            switch_mode(a);
        }
    }
    select_desktop(tmp);

    if(STATUS_BAR == 0) update_bar();
    for(i=0;i<num_screens;++i) {
        select_desktop(view[i].cd);
        tile();
    }
    select_desktop(tmp);
    update_current();
    setbaralpha();

    read_apps_file();
    memset(keys, 0, keycount * sizeof(key));
    memset(cmds, 0, cmdcount * sizeof(Commands));
    grabkeys();
}
