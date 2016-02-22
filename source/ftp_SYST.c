#include "ftp.h"

/*! @fn static int SYST(ftp_session_t *session, const char *args)
 *
 *  @brief identify system
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(SYST) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* we are UNIX compliant with 8-bit characters */
    return ftp_send_response(session, 215, "UNIX Type: L8\r\n");
}

