#include "ftp.h"

/*! @fn static int RMD(ftp_session_t *session, const char *args)
 *
 *  @brief remove a directory
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(RMD) {
    int rc;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    REJECT_WRITE_CHK;

    /* build the path to remove */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* remove the directory */
    rc = rmdir(session->buffer);
    if (rc != 0) {
        /* rmdir error */
        console_print(RED "rmdir: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 550,
                                 "failed to delete directory\r\n");
    }

    return ftp_send_response(session, 250, "OK\r\n");
}

