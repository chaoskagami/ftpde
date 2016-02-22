#pragma once

extern char* config_file;
extern int sett_port;
extern int sett_enable_anon;
extern int sett_blank_is_anon;
extern int sett_login_to_anon;
extern int sett_enable_ip_whitelist;

int check_login_info(char* username, char* password);
int load_config_file();
