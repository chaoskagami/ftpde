#include "ftp.h"

/*! @fn static int CWD(ftp_session_t *session, const char *args)
 *
 *  @brief change working directory
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(CWD) {
    struct stat st;
    int rc;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* .. is equivalent to CDUP */
    if (strcmp(args, "..") == 0) {
        cd_up(session);
        return ftp_send_response(session, 200, "OK\r\n");
    }

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* build the new cwd path */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

    /* get the path status */
    rc = stat(session->buffer, &st);
    if (rc != 0) {
        console_print(RED "stat '%s': %d %s\n" RESET, session->buffer, errno,
                      strerror(errno));
        return ftp_send_response(session, 550, "unavailable\r\n");
    }

    /* make sure it is a directory */
    if (!S_ISDIR(st.st_mode))
        return ftp_send_response(session, 553, "not a directory\r\n");

    /* copy the path into the cwd */
    strncpy(session->cwd, session->buffer, sizeof(session->cwd));

    return ftp_send_response(session, 200, "OK\r\n");
}

