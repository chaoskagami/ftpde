#pragma once

extern char* config_file;
extern int sett_port;
extern int sett_enable_anon;
extern int sett_blank_is_anon;
extern int sett_login_to_anon;
extern int sett_enable_ip_whitelist;
extern int sett_poll_rate;
extern int sett_poll_rate_two;
extern int sett_disable_color;
extern int sett_paranoid_port;

#ifdef _3DS
  extern int   sett_high_clock_rate;
  extern char* sett_app_bottom_path;
#endif

int check_login_info(char* username, char* password);
int load_config_file();
