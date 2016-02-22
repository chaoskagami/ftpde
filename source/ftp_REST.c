#include "ftp.h"

/*! @fn static int REST(ftp_session_t *session, const char *args)
 *
 *  @brief restart a transfer
 *
 *  @note sets file position for a subsequent STOR operation
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(REST) {
    const char *p;
    uint64_t pos = 0;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* make sure an argument is provided */
    if (args == NULL)
        return ftp_send_response(session, 504, "invalid argument\r\n");

    /* parse the offset */
    for (p = args; *p; ++p) {
        if (!isdigit((int)*p))
            return ftp_send_response(session, 504, "invalid argument\r\n");

        if (UINT64_MAX / 10 < pos)
            return ftp_send_response(session, 504, "invalid argument\r\n");

        pos *= 10;

        if (UINT64_MAX - (*p - '0') < pos)
            return ftp_send_response(session, 504, "invalid argument\r\n");

        pos += (*p - '0');
    }

    /* set the restart offset */
    session->filepos = pos;
    return ftp_send_response(session, 200, "OK\r\n");
}

