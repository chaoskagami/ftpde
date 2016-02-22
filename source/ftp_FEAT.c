#include "ftp.h"

/*! @fn static int FEAT(ftp_session_t *session, const char *args)
 *
 *  @brief list server features
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(FEAT) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* list our features */
    return ftp_send_response(session, -211, "\r\n"
                                            " MDTM\r\n"
                                            " UTF8\r\n"
                                            "\r\n"
                                            "211 End\r\n");
}

