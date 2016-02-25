#include "ftp.h"

/*! @fn static int DELE(ftp_session_t *session, const char *args)
 *
 *  @brief delete a file
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(DELE) {
    int rc;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    REJECT_WRITE_CHK;

    /* build the file path */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* return an error immediately if this is a config file. */
    if (check_is_config(session->buffer, 1)) {
        console_print(RED "Config '%s' overwrite rejected by policy.\n" RESET, session->buffer);
        return ftp_send_response(session, 550, "delete was rejected by policy\r\n");
    }

    /* try to unlink the path */
    rc = unlink(session->buffer);
    if (rc != 0) {
        /* error unlinking the file */
        console_print(RED "unlink: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 550, "failed to delete file\r\n");
    }

    return ftp_send_response(session, 250, "OK\r\n");
}

