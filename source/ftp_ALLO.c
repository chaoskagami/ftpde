#include "ftp.h"

/*! @fn static int ALLO(ftp_session_t *session, const char *args)
 *
 *  @brief allocate space
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(ALLO) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    if ((session->auth_level & AUTH_WRITE) != AUTH_WRITE) {
        // Invalid. Lacking proper auth.
        ftp_session_set_state(session, COMMAND_STATE, 0);

        return ftp_send_response(session, 530, "Not permitted.\r\n");
    }

    ftp_session_set_state(session, COMMAND_STATE, 0);

    return ftp_send_response(session, 202, "superfluous command\r\n");
}

