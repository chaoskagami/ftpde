#include "ftp.h"

/*! @fn static int OPTS(ftp_session_t *session, const char *args)
 *
 *  @brief set options
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(OPTS) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* we only accept the following options */
    if (strcasecmp(args, "UTF8") == 0 || strcasecmp(args, "UTF8 ON") == 0 ||
        strcasecmp(args, "UTF8 NLST") == 0)
        return ftp_send_response(session, 200, "OK\r\n");

    return ftp_send_response(session, 504, "invalid argument\r\n");
}

