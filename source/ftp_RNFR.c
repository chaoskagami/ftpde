#include "ftp.h"

/*! @fn static int RNFR(ftp_session_t *session, const char *args)
 *
 *  @brief rename from
 *
 *  @note Must be followed by RNTO
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(RNFR) {
    int rc;
    struct stat st;
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    if ((session->auth_level & AUTH_WRITE) != AUTH_WRITE) {
        // Invalid. Lacking proper auth.
        return ftp_send_response(session, 530, "Not permitted.\r\n");
    }

    /* build the path to rename from */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* make sure the path exists */
    rc = lstat(session->buffer, &st);
    if (rc != 0) {
        /* error getting path status */
        console_print(RED "lstat: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 450, "no such file or directory\r\n");
    }

    /* we are ready for RNTO */
    session->flags |= SESSION_RENAME;
    return ftp_send_response(session, 350, "OK\r\n");
}

