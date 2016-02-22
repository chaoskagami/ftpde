#include "ftp.h"

/*! @fn static int RNTO(ftp_session_t *session, const char *args)
 *
 *  @brief rename to
 *
 *  @note Must be preceded by RNFR
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(RNTO) {
    int rc;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    if ((session->auth_level & AUTH_WRITE) != AUTH_WRITE) {
        // Invalid. Lacking proper auth.
        return ftp_send_response(session, 530, "Not permitted.\r\n");
    }

    /* make sure the previous command was RNFR */
    if (!(session->flags & SESSION_RENAME))
        return ftp_send_response(session, 503, "Bad sequence of commands\r\n");

    /* clear the rename state */
    session->flags &= ~SESSION_RENAME;

    /* copy the RNFR path */
    memcpy(session->tmp_buffer, session->buffer, XFER_BUFFERSIZE);

    /* build the path to rename to */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 554, "%s\r\n", strerror(errno));

    /* rename the file */
    rc = rename(session->tmp_buffer, session->buffer);
    if (rc != 0) {
        /* rename failure */
        console_print(RED "rename: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 550,
                                 "failed to rename file/directory\r\n");
    }

    return ftp_send_response(session, 250, "OK\r\n");
}

