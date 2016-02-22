#include "ftp.h"

/*! @fn static int CDUP(ftp_session_t *session, const char *args)
 *
 *  @brief CWD to parent directory
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(CDUP) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* change to parent directory */
    cd_up(session);

    return ftp_send_response(session, 200, "OK\r\n");
}

