#include "ftp.h"

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
    if (!strcmp(session->username_buf, "anonymous") || !strcmp(session->username_buf, "")) {
        // Anon is a special user that accepts any password should have only
        // read permissions. Blank is the same thing as anonymous.
        session->auth_level = AUTH_READ; // Remove AUTH_WRITE once configs are implmented in PASS.

        return ftp_send_response(session, 230, "OK\r\n");
    }

    return ftp_send_response(session, 331, "OK\r\n");
}

