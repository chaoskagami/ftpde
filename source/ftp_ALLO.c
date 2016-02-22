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

    ftp_session_set_state(session, COMMAND_STATE, 0);

    REJECT_WRITE_CHK;

    return ftp_send_response(session, 202, "superfluous command\r\n");
}

