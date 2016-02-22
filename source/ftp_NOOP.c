#include "ftp.h"

/*! @fn static int NOOP(ftp_session_t *session, const char *args)
 *
 *  @brief no-op
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(NOOP) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* this is a no-op */
    return ftp_send_response(session, 200, "OK\r\n");
}

