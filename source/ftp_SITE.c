#include "ftp.h"

FTP_DECLARE(ST_HELP) {
    console_print(CYAN " -> %s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* list our accepted commands */
    return ftp_send_response(session, -214,
                             "The following SITE commands are recognized\r\n"
                             " HELP CHMOD\r\n"
                             " Server is running " STATUS_STRING "\r\n"
                             "214 End\r\n");
}

FTP_DECLARE(ST_CHMOD) {
    console_print(CYAN " -> %s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

#ifndef _3DS
    /* We only do this stuff on POSIX builds, since the SD card of a 3DS
     * has no concept of permissions. */

    char* mode_str = args;
    char* filename = NULL;
    for (int i=0; i < strlen(args)-1; i++) {
        if (args[i] == ' ') {
            filename = args + i + 1;
        }
    }
    uint32_t mode;

    // Now, scan the mode into a string and remove any problematic bits (e.g. suid).
    sscanf(mode_str, "%u", &mode);
    // By far the simplest way...
    mode = mode % 1000;

    REJECT_WRITE_CHK;

    // Now test if said file exists. It should. We use build_path here.

    /* build the file path */
    if (build_path(session, session->cwd, filename) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* return an error immediately if this is a config file. */
    if (check_is_config(session->buffer, 1)) {
        console_print(RED "Config '%s' chmod rejected by policy.\n" RESET, session->buffer);
        return ftp_send_response(session, 550, "chmod was rejected by policy\r\n");
    }

    // Now session->buffer is a filename to chmod - do so.
    if (chmod(session->buffer, mode) == -1)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));
#endif

    return ftp_send_response(session, 200, "OK\r\n");
}

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

    ftp_session_set_state(session, COMMAND_STATE, 0);

    return ftp_send_response(session, 502, "Invalid site command.\r\n");
}

