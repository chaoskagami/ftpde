#include "ftp.h"

/*! @fn static int STRU(ftp_session_t *session, const char *args)
 *
 *  @brief set file structure
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(STRU) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* we only support F (no structure) mode */
    if (strcasecmp(args, "F") == 0)
        return ftp_send_response(session, 200, "OK\r\n");

    return ftp_send_response(session, 504, "unavailable\r\n");
}

