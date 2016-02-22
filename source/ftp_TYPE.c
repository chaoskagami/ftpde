#include "ftp.h"

/*! @fn static int TYPE(ftp_session_t *session, const char *args)
 *
 *  @brief set transfer mode
 *
 *  @note transfer mode is always binary
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(TYPE) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* FIXME - Not the correct way to handle this. The server needs to
       refuse settings another mode, not silently OK. */

    /* we always transfer in binary mode */
    return ftp_send_response(session, 200, "OK\r\n");
}

