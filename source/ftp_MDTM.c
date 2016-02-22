#include "ftp.h"

/*! @fn static int MDTM(ftp_session_t *session, const char *args)
 *
 *  @brief get last modification time
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(MDTM) {
    int rc;
#ifdef _3DS
    uint64_t mtime;
#else
    struct stat st;
#endif
    time_t t_mtime;
    struct tm *tm;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* build the path */
    if (build_path(session, session->cwd, args) != 0)
        return ftp_send_response(session, 553, "%s\r\n", strerror(errno));

#ifdef _3DS
    rc = sdmc_getmtime(session->buffer, &mtime);
    if (rc != 0)
        return ftp_send_response(session, 550, "Error getting mtime\r\n");
    t_mtime = mtime;
#else
    rc = stat(session->buffer, &st);
    if (rc != 0)
        return ftp_send_response(session, 550, "Error getting mtime\r\n");
    t_mtime = st.st_mtime;
#endif

    tm = gmtime(&t_mtime);
    if (tm == NULL)
        return ftp_send_response(session, 550, "Error getting mtime\r\n");

    session->buffersize =
        strftime(session->buffer, sizeof(session->buffer), "%Y%m%d%H%M%S", tm);
    if (session->buffersize == 0)
        return ftp_send_response(session, 550, "Error getting mtime\r\n");
    session->buffer[session->buffersize] = 0;

    return ftp_send_response(session, 213, "%s\r\n", session->buffer);
}

