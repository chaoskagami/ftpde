#include "ftp.h"

/*! @fn static int MKD(ftp_session_t *session, const char *args)
 *
 *  @brief create a directory
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(MKD) {
    int rc;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    REJECT_WRITE_CHK;

    /* build the path */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* try to create the directory */
    rc = mkdir(session->buffer, 0755);
    if (rc != 0 && errno != EEXIST) {
        /* mkdir failure */
        console_print(RED "mkdir: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 550,
                                 "failed to create directory\r\n");
    }

    return ftp_send_response(session, 250, "OK\r\n");
}

