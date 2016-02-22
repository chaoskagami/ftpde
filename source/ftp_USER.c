#include "ftp.h"
#include "configread.h"

/*! @fn static int USER(ftp_session_t *session, const char *args)
 *
 *  @brief provide user name
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(USER) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    strncpy(session->username_buf, args, 64); // Copy username into buffer.

    ftp_session_set_state(session, COMMAND_STATE, 0);

    // TODO - Allow disabling the anonymous user.
    if (sett_enable_anon && !strcmp(session->username_buf, "anonymous")) {
        // Anon is a special user that accepts any password should have only
        // read permissions. Blank is the same thing as anonymous.
        session->auth_level = AUTH_READ;

        console_print(RED "Session logged in as 'anonymous'. Read-only.\n" RESET);

        return ftp_send_response(session, 230, "OK\r\n");
    }

    return ftp_send_response(session, 331, "OK\r\n");
}

