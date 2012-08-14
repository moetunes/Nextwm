/* readkeysapps.c [ 0.6.0 ] */

void read_keys_file() {
    FILE *keyfile ;
    char buffer[100];
    char dummy[100];
    char *dummy2, *dummy3, *dummy4;
    keycount = cmdcount = 0;
    int i, j;

    keyfile = fopen( KEY_FILE, "r" ) ;
    if ( keyfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,KEY_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,keyfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#') continue;
            /* Now look for commands */
            if(strstr(buffer, "CMD" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy2 = strdup(dummy);
                cmds[cmdcount].name = strsep(&dummy2, ";");
                cmds[cmdcount].list[0] = strsep(&dummy2, ";");
                i=1;
                while(dummy2) {
                    cmds[cmdcount].list[i] = strsep(&dummy2, ";");
                    if(strcmp(cmds[cmdcount].list[i], "NULL") == 0) break;
                    i++;
                }
                cmdcount++;
                continue;
            } else if(strstr(buffer, "KEY" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1));
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                dummy3 = strsep(&dummy2, ";");
                if(strncmp(dummy3, "DESKTOPCHANGE", 12) == 0) {
                    keys[keycount].keysym = strsep(&dummy2, ";");
                    dummy4 = strsep(&dummy2, ";");
                    keys[keycount].mod = Mod1Mask;
                    keys[keycount].myfunction = change_desktop;
                    keys[keycount].arg.i = atoi(dummy4);
                    keycount++;
                    keys[keycount].mod = Mod4Mask|ShiftMask;
                    keys[keycount].keysym = keys[keycount-1].keysym;
                    keys[keycount].myfunction = client_to_desktop;
                    keys[keycount].arg.i = atoi(dummy4);
                    keycount++;
                    keys[keycount].mod = Mod1Mask|ShiftMask;
                    keys[keycount].keysym = keys[keycount-1].keysym;
                    keys[keycount].myfunction = follow_client_to_desktop;
                    keys[keycount].arg.i = atoi(dummy4);
                    keycount++;
                    continue;
                } else if(strncmp(dummy3, "Alt", 3) == 0) keys[keycount].mod = Mod1Mask;
                else if(strncmp(dummy3, "CtrlAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ControlMask;
                else if(strncmp(dummy3, "ShftAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ShiftMask;
                else if(strncmp(dummy3, "Super", 5) == 0) keys[keycount].mod = Mod4Mask;
                keys[keycount].keysym = strsep(&dummy2, ";");
                dummy3 = strsep(&dummy2, ";");
                if(strcmp(dummy3, "kill_client") == 0) keys[keycount].myfunction = kill_client;
                else if(strcmp(dummy3, "last_desktop") == 0) keys[keycount].myfunction = last_desktop;
                else if(strcmp(dummy3, "more_master") == 0) {
                    keys[keycount].myfunction = more_master;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_down") == 0) {
                    keys[keycount].myfunction = move_down;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_up") == 0) {
                    keys[keycount].myfunction = move_up;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_left") == 0) {
                    keys[keycount].myfunction = move_left;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_right") == 0) {
                    keys[keycount].myfunction = move_right;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "switch_mode") == 0) {
                    keys[keycount].myfunction = switch_mode;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "resize_master") == 0) {
                    keys[keycount].myfunction = resize_master;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "resize_stack") == 0) {
                    keys[keycount].myfunction = resize_stack;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "rotate_desktop") == 0) {
                    keys[keycount].myfunction = rotate_desktop;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "rotate_mode") == 0) {
                    keys[keycount].myfunction = rotate_mode;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "quit") == 0) keys[keycount].myfunction = quit;
                else if(strcmp(dummy3, "next_win") == 0) keys[keycount].myfunction = next_win;
                else if(strcmp(dummy3, "prev_win") == 0) keys[keycount].myfunction = prev_win;
                else if(strcmp(dummy3, "swap_master") == 0) keys[keycount].myfunction = swap_master;
                else if(strcmp(dummy3, "toggle_bar") == 0) keys[keycount].myfunction = toggle_bar;
                else if(strcmp(dummy3, "update_config") == 0) keys[keycount].myfunction = update_config;
                else if(strcmp(dummy3, "spawn") == 0) {
                    keys[keycount].myfunction = spawn;
                    dummy4 = strsep(&dummy2, ";");
                    for(i=0;i<cmdcount;i++) {
                        if(strcmp(dummy4, cmds[i].name) == 0) {
                            keys[keycount].arg.com[0] = cmds[i].list[0];
                            j=1;
                            while(cmds[i].list[j]) {
                                if(strcmp(cmds[i].list[j], "NULL") == 0) break;
                                keys[keycount].arg.com[j] = cmds[i].list[j];
                                j++;
                            }
                            break;
                        }
                    }
                }
                keycount++;
                continue;
            }
        }
    }
    fclose(keyfile);
}

void read_apps_file() {
    FILE *appsfile ;
    char buffer[100];
    char dummy[100];
    char *dummy2;
    dtcount = pcount = 0;

    appsfile = fopen( APPS_FILE, "r" ) ;
    if ( appsfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,APPS_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,appsfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#') continue;
            /* Now look for commands */
            if(strstr(buffer, "DESKTOP" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy2 = strdup(dummy);
                convenience[dtcount].class = strsep(&dummy2, ";");
                convenience[dtcount].preferredd = atoi(strsep(&dummy2, ";"));
                convenience[dtcount].followwin = atoi(strsep(&dummy2, ";"));
                dtcount++;
                continue;
            } else if(strstr(buffer, "POSITION" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy2 = strdup(dummy);
                positional[pcount].class = strsep(&dummy2, ";");
                positional[pcount].x = atoi(strsep(&dummy2, ";"));
                positional[pcount].y = atoi(strsep(&dummy2, ";"));
                positional[pcount].width = atoi(strsep(&dummy2, ";"));
                positional[pcount].height = atoi(strsep(&dummy2, ";"));
                pcount++;
                continue;
            } else continue;
        }
    }
    fclose(appsfile);
}

