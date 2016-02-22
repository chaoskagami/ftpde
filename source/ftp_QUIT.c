#include "ftp.h"

/*! @fn static int QUIT(ftp_session_t *session, const char *args)
 *
 *  @brief terminate ftp session
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(QUIT) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* disconnect from the client */
    ftp_send_response(session, 221, "disconnecting\r\n");
    ftp_session_close_cmd(session);

    return 0;
}

