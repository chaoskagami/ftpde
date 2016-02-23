#include "ftp.h"
#include "configread.h"

/*! @fn static int PASS(ftp_session_t *session, const char *args)
 *
 *  @brief provide password
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(PASS) {
    console_print(CYAN "%s ****\n" RESET, __func__); // Deliberately censored. Passwords do NOT get printed to consoles.

    ftp_session_set_state(session, COMMAND_STATE, 0);

    if (sett_enable_anon && !strcmp(session->username_buf, "anonymous")) {
        // Just return. Anon is Read-only access and needs no password.
        return ftp_send_response(session, 230, "OK\r\n");
    } else if (check_login_info(session->username_buf, args) ) {
        // Username and password are valid.

        // Log the succeeded login attempt.
        console_print(RED "Session opened as '%s'\n" RESET, session->username_buf);

        // TODO - implement permissions.
        session->auth_level = AUTH_READ|AUTH_WRITE; // Read/Write.

        return ftp_send_response(session, 230, "OK\r\n");
    }

    // We only reach this on failure.
    return ftp_send_response(session, 430, "Invalid username or password.\r\n");
}
