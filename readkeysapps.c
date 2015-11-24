/* readkeysapps.c [ 0.8.8 ] */

void read_keys_file() {
    FILE *keyfile ;
    keycount = cmdcount = 0;
    unsigned int i, j;

    keyfile = fopen( KEY_FILE, "r" ) ;
    if ( keyfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,KEY_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,keyfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n') continue;
            /* Now look for commands */
            if(strstr(buffer, "CMD" ) != NULL) {
                k = 4;
                if(get_value() == 0)
                    cmds[cmdcount].name = strdup(dummy);
                i=0;
                while(get_value() == 0){
                    cmds[cmdcount].list[i] = strdup(dummy);
                    if(strcmp(cmds[cmdcount].list[i], "NULL") == 0) break;
                    ++i;
                }
                ++cmdcount;
                continue;
            } else if(strstr(buffer, "KEY" ) != NULL) {
                k = 4;
                if(get_value() == 0) {
                    if(strncmp(dummy, "AltSuper", 8) == 0) keys[keycount].mod = Mod1Mask|Mod4Mask;
                    else if(strncmp(dummy, "Control", 7) == 0) keys[keycount].mod = ControlMask;
                    else if(strncmp(dummy, "CtrlAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ControlMask;
                    else if(strncmp(dummy, "ShftAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ShiftMask;
                    else if(strncmp(dummy, "Super", 5) == 0) keys[keycount].mod = Mod4Mask;
                    else if(strncmp(dummy, "CtrlSuper", 9) == 0) keys[keycount].mod = Mod4Mask|ControlMask;
                    else if(strncmp(dummy, "ShftSuper", 9) == 0) keys[keycount].mod = Mod4Mask|ShiftMask;
                    else if(strncmp(dummy, "Alt", 3) == 0) keys[keycount].mod = Mod1Mask;
                    else if(strncmp(dummy, "NULL", 4) == 0) keys[keycount].mod = 0;
                    else continue;
                }
                if(get_value() == 0)
                    keys[keycount].keysym = strdup(dummy);
                if(get_value() == 0) {
                    if(strcmp(dummy, "kill_client") == 0) keys[keycount].myfunction = kill_client;
                    else if(strcmp(dummy, "last_desktop") == 0) keys[keycount].myfunction = last_desktop;
                    else if(strcmp(dummy, "change_desktop") == 0) {
                        keys[keycount].myfunction = change_desktop;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "follow_client_to_desktop") == 0) {
                        keys[keycount].myfunction = follow_client_to_desktop;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "client_to_desktop") == 0) {
                        keys[keycount].myfunction = client_to_desktop;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "more_master") == 0) {
                        keys[keycount].myfunction = more_master;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "move_down") == 0) {
                        keys[keycount].myfunction = move_down;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "move_up") == 0) {
                        keys[keycount].myfunction = move_up;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "move_left") == 0) {
                        keys[keycount].myfunction = move_left;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "move_right") == 0) {
                        keys[keycount].myfunction = move_right;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "switch_mode") == 0) {
                        keys[keycount].myfunction = switch_mode;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "resize_master") == 0) {
                        keys[keycount].myfunction = resize_master;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "resize_stack") == 0) {
                        keys[keycount].myfunction = resize_stack;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "rotate_desktop") == 0) {
                        keys[keycount].myfunction = rotate_desktop;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "rotate_mode") == 0) {
                        keys[keycount].myfunction = rotate_mode;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "terminate") == 0) {
                        keys[keycount].myfunction = terminate;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "sticky_win") == 0) {
                        keys[keycount].myfunction = sticky_win;
                        get_value();
                        keys[keycount].arg.i = strtol(dummy, NULL, 10);
                    } else if(strcmp(dummy, "unsticky_win") == 0) keys[keycount].myfunction = unsticky_win;
                    else if(strcmp(dummy, "quit") == 0) keys[keycount].myfunction = quit;
                    else if(strcmp(dummy, "next_win") == 0) keys[keycount].myfunction = next_win;
                    else if(strcmp(dummy, "prev_win") == 0) keys[keycount].myfunction = prev_win;
                    else if(strcmp(dummy, "swap_master") == 0) keys[keycount].myfunction = swap_master;
                    else if(strcmp(dummy, "pop_window") == 0) keys[keycount].myfunction = pop_window;
                    else if(strcmp(dummy, "toggle_bar") == 0) keys[keycount].myfunction = toggle_bar;
                    else if(strcmp(dummy, "update_config") == 0) keys[keycount].myfunction = update_config;
                    else if(strcmp(dummy, "spawn") == 0) {
                        keys[keycount].myfunction = spawn;
                        get_value();
                        for(i=0;i<cmdcount;++i) {
                            if(strcmp(dummy, cmds[i].name) == 0) {
                                j=0;
                                while(strcmp(cmds[i].list[j], "NULL") != 0) {
                                    keys[keycount].arg.com[j] = cmds[i].list[j];
                                    ++j;
                                }
                                break;
                            }
                        }
                    } else continue;
                    ++keycount; k=0;
                }
            }
        }
    }
    fclose(keyfile);
}

void read_apps_file() {
    FILE *appsfile ;
    dtcount = pcount = tcount = 0;

    appsfile = fopen( APPS_FILE, "r" ) ;
    if ( appsfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,APPS_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,appsfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n') continue;
            /* Now look for commands */
            if(strstr(buffer, "DESKTOP" ) != NULL) {
                k = 8;
                if(get_value() == 0) convenience[dtcount].class = strdup(dummy);
                if(get_value() == 0) convenience[dtcount].preferredd = strtol(dummy, NULL, 10);
                if(get_value() == 0) convenience[dtcount].followwin = strtol(dummy, NULL, 10);
                ++dtcount;
                continue;
            } else if(strstr(buffer, "POSITION" ) != NULL) {
                k = 9;
                if(get_value() == 0) positional[pcount].class = strdup(dummy);
                if(get_value() == 0) positional[pcount].x = strtol(dummy, NULL, 10);
                if(get_value() == 0) positional[pcount].y = strtol(dummy, NULL, 10);
                if(get_value() == 0) positional[pcount].width = strtol(dummy, NULL, 10);
                if(get_value() == 0) positional[pcount].height = strtol(dummy, NULL, 10);
                ++pcount;
                continue;
            } else if(strstr(buffer, "POPPED" ) != NULL) {
                k = 7;
                if(get_value() == 0) popped[tcount].class = strdup(dummy);
                ++tcount;
                continue;
            } else continue;
        }
    }
    fclose(appsfile);
}

