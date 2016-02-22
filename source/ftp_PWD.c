#include "ftp.h"

/*! @fn static int PWD(ftp_session_t *session, const char *args)
 *
 *  @brief print working directory
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(PWD) {
    static char buffer[CMD_BUFFERSIZE];
    size_t len = sizeof(buffer), i;
    char *path;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* encode the cwd */
    len = strlen(session->cwd);
    path = encode_path(session->cwd, &len, true);

    if (path != NULL) {
        i = sprintf(buffer, "257 \"");
        if (i + len + 3 > sizeof(buffer)) {
            /* buffer will overflow */
            free(path);
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            ftp_send_response(session, 550, "unavailable\r\n");
            return ftp_send_response(session, 425, "%s\r\n",
                                     strerror(EOVERFLOW));
        }
        memcpy(buffer + i, path, len);
        free(path);
        len += i;
        buffer[len++] = '"';
        buffer[len++] = '\r';
        buffer[len++] = '\n';

        return ftp_send_response_buffer(session, buffer, len, 1);
    }
    return ftp_send_response(session, 425, "%s\r\n", strerror(ENOMEM));
}

