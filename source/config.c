#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <libconfig.h>

#ifdef _3DS
  #include <3ds.h>
#endif

#include "hexd.h"
#include "console.h"
#include "ftp.h"

#ifdef _3DS
  #define SYSCONFDIR ""
#endif

char* config_file = SYSCONFDIR "/ftpde.conf";
int sett_port                = 21;
int sett_enable_anon         = 1;
int sett_blank_is_anon       = 1;
int sett_login_to_anon       = 0;
int sett_enable_ip_whitelist = 0;
int sett_disable_color       = 0;

#ifdef _3DS
  int sett_high_clock_rate = 0;
#endif

int sett_poll_rate = -1;

int check_login_info(char* username, char* password) {
    config_t config_obj;
	config_setting_t *setting;

    if (password == NULL || strlen(password) == 0)
        return 0; // It isn't valid. Don't bother.
    if (username == NULL || strlen(username) == 0)
        return 0; // It isn't valid. Don't bother.

    config_init(&config_obj);

    if (!config_read_file(&config_obj, config_file)) {
        // Failed to load config file.
        console_print(RED "Opening config file failed.\n" RESET);

        config_destroy(&config_obj);
        return 0;
    }

    // Read user list
    setting = config_lookup(&config_obj, "users");

    int success = 0; // If the login was successful, 1. Otherwise, 0.

    int len = config_setting_length(setting);

    for(int i = 0; i < len; i++) {
        config_setting_t* elem = config_setting_get_elem(setting, i);

        char *user, *pass_str, *pass_alg, *pass_salt, *pass_hash;

        user = config_setting_name(elem);

        if (strcmp(user, username))
            continue; // Keep going.

        pass_str = strdup(config_setting_get_string(elem));
        pass_alg = pass_str;

        // Find the ':' separating alg from salt.
        for(int j=0; j < strlen(pass_str); j++) {
            if (pass_str[j] == ':') {
                pass_str[j] = 0;
                pass_salt = pass_str+j+1;
                break;
            }
        }

        // Find the '$' separating salt from hash.
        for(int j=0; j < strlen(pass_salt); j++) {
            if (pass_salt[j] == '$') {
                pass_salt[j] = 0;
                pass_hash = pass_salt+j+1;
                break;
            }
        }

        int s_pass_len = (strlen(pass_salt)/2) + strlen(password);

        // We have all the data needed to process whether our credentials match, so now...
        char *salted_buf = malloc(s_pass_len + 1);
        memset(salted_buf, 0, s_pass_len);

        // Create the salted password.
        unhexdump(pass_salt, salted_buf);
        memcpy(salted_buf + (strlen(pass_salt) / 2), password, strlen(password));

        // Sha256 is the only algorithm implemented, but check anyways.
        if (!strcmp(pass_alg, "sha256")) {
            char hash_bin[32];
            char hash_out[65];
            hash_out[64] = 0;

            Sha256_Data((const unsigned char*)salted_buf, s_pass_len, (unsigned char*)hash_bin);

            hexdump(hash_bin, hash_out, 32);

            if (!strncmp(hash_out, pass_hash, 64)) // Pass is correct for user. Permit login.
                success = 1;
        }

        free(salted_buf);
        free(pass_str);

        if (success == 1)
            break;
    }

    config_destroy(&config_obj);

    return success;
}

/* This only loads general info. Not users/passwords. */
int load_config_file() {
    config_t config_obj;

    config_init(&config_obj);

    if (!config_read_file(&config_obj, config_file)) {
        // Failed to load config file.
        console_print(RED "Opening config file failed.\n" RESET);

        // Failed to load config file.
        config_destroy(&config_obj);
        return -1;
    }

    // Port
    config_lookup_int(&config_obj, "port", &sett_port);

    // Enable 'anonymous' user which can only read.
    config_lookup_bool(&config_obj, "enable_anon", &sett_enable_anon);

    // Whether blank username should be treated as 'anonymous'
    config_lookup_bool(&config_obj, "blank_is_anon", &sett_blank_is_anon);

    // Whether invalid login credentials should fall back to 'anonymous'.
    config_lookup_bool(&config_obj, "login_to_anon", &sett_login_to_anon);

    // Enable IP whitelist. Only hosts on it will be allowed to connect.
    config_lookup_bool(&config_obj, "enable_ip_whitelist", &sett_enable_ip_whitelist);

    // Disable colorized output.
    config_lookup_bool(&config_obj, "disable_color", &sett_disable_color);

#ifdef _3DS
    // Enable n3DS clock rate.
    config_lookup_bool(&config_obj, "high_clock_rate", &sett_high_clock_rate);
#endif

    // Poll rate for hosts. Default is platform-specific.
    config_lookup_int(&config_obj, "poll_rate", &sett_poll_rate);
    if (sett_poll_rate == -1) {
#ifdef _3DS
        sett_poll_rate = 0;
#else
        sett_poll_rate = 100;
#endif
    }

    config_destroy(&config_obj);

    return 0;
}

