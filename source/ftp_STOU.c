#include "ftp.h"

/*! @fn static int STOU(ftp_session_t *session, const char *args)
 *
 *  @brief store a unique file
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(STOU) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* we do not support this yet */
    ftp_session_set_state(session, COMMAND_STATE, 0);

    return ftp_send_response(session, 502, "unavailable\r\n");
}

