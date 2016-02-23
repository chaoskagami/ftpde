#include "ftp.h"

FTP_DECLARE(ST_HELP) {
    console_print(CYAN " -> %s %s\n" RESET, __func__, args ? args : "");

    /* list our accepted commands */
    return ftp_send_response(session, -214,
                             "The following SITE commands are recognized\r\n"
                             " HELP CHMOD\r\n"
                             "Server is running " STATUS_STRING "\r\n"
                             "214 End\r\n");
}

FTP_DECLARE(ST_CHMOD) {
    console_print(CYAN " -> %s %s\n" RESET, __func__, args ? args : "");

#ifndef _3DS
    /* We only do this stuff on POSIX builds, since the SD card of a 3DS
     * has no concept of permissions. */
#endif

    return ftp_send_response(session, 200, "OK\r\n");
}

#define MIN(x, y) ()

/*! @fn static int SITE(ftp_session_t *session, const char *args)
 *
 *  @brief Execute site-specific commmands.
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(SITE) {
    char* args_new = NULL;

    console_print(CYAN "%s %s" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    // This is a site-specific command.
    if (args && strlen(args) > 0) {
        for(int i=0; i < strlen(args) - 1; i++) {
            if(args[i] == ' ') { // Space
                args_new = args+i+1; // Save new args.
                break;
            }
        }
    }

    if (args && !strncmp(args, "CHMOD", 5)) {
        return ST_CHMOD(session, args_new);
    } else if (args && !strncmp(args, "HELP", 4)) {
        return ST_HELP(session, args_new);
    }

    console_print("\n");

    return ftp_send_response(session, 504, "Invalid site command.\r\n");
}

