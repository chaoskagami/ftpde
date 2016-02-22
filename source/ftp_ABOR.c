#include "ftp.h"

/*! @fn static int ABOR(ftp_session_t *session, const char *args)
 *
 *  @brief abort a transfer
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(ABOR) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    if (session->state == COMMAND_STATE)
        return ftp_send_response(session, 225, "No transfer to abort\r\n");

    /* abort the transfer */
    ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);

    /* send response for this request */
    ftp_send_response(session, 225, "Aborted\r\n");

    /* send response for transfer */
    return ftp_send_response(session, 425, "Transfer aborted\r\n");
}

