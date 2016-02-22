#include "ftp.h"

/*! @fn static int SIZE(ftp_session_t *session, const char *args)
 *
 *  @brief returns file size for argument
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(SIZE) {
    int rc;
    struct stat st;
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

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

    /* return filesize */
    return ftp_send_response(session, 200, "%lu\r\n", st.st_size);
}
