#pragma once

extern char* config_file;
extern int sett_port;
extern int sett_enable_anon;
extern int sett_blank_is_anon;
extern int sett_login_to_anon;
extern int sett_enable_ip_whitelist;
extern int sett_poll_rate;
extern int sett_disable_color;

#ifdef _3DS
  extern int sett_high_clock_rate;
#endif

int check_login_info(char* username, char* password);
int load_config_file();
