#include "ftp.h"

/*! @fn static int MODE(ftp_session_t *session, const char *args)
 *
 *  @brief set transfer mode
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(MODE) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* we only accept S (stream) mode */
    if (strcasecmp(args, "S") == 0)
        return ftp_send_response(session, 200, "OK\r\n");

    return ftp_send_response(session, 504, "unavailable\r\n");
}

