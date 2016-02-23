#include "ftp.h"

/*! @fn static int STAT(ftp_session_t *session, const char *args)
 *
 *  @brief get status
 *
 *  @note If no argument is supplied, and a transfer is occurring, get the
 *        current transfer status. If no argument is supplied, and no transfer
 *        is occurring, get the server status. If an argument is supplied, this
 *        is equivalent to LIST, except the data is sent over the command
 *        socket.
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(STAT) {
    time_t uptime = time(NULL) - start_time;
    int hours = uptime / 3600;
    int minutes = (uptime / 60) % 60;
    int seconds = uptime % 60;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    if (session->state == DATA_CONNECT_STATE) {
        /* we are waiting to connect to the client */
        return ftp_send_response(session, -211,
                                 "FTP server status\r\n"
                                 " Running: " STATUS_STRING "\r\n"
                                 " Logged in as: '%s'\r\n"
                                 " TYPE: Binary, MODE: Stream\r\n"
                                 " No data connection\r\n"
                                 "211 End\r\n",
                                 session->username_buf);
    } else if (session->state == DATA_TRANSFER_STATE) {
        /* we are in the middle of a transfer */
        return ftp_send_response(session, -211,
                                 "FTP server status\r\n"
                                 " Running: " STATUS_STRING "\r\n"
                                 " Logged in as: '%s'\r\n"
                                 " TYPE: Binary, MODE: Stream\r\n"
                                 " Transferred %" PRIu64 " bytes\r\n"
                                 "211 End\r\n",
                                 session->username_buf, session->filepos);
    }

    if (strlen(args) == 0) {
        /* no argument provided, send the server status */
        return ftp_send_response(session, -211, "FTP server status\r\n"
                                                " Running: " STATUS_STRING "\r\n"
                                                " Logged in as: '%s'\r\n"
                                                " TYPE: Binary, MODE: Stream\r\n"
                                                " Uptime: %02d:%02d:%02d\r\n"
                                                "211 End\r\n",
                                 session->username_buf, hours, minutes, seconds);
    }

    /* argument provided, open the path in STAT mode */
    return ftp_xfer_dir(session, args, XFER_DIR_STAT, false);
}

