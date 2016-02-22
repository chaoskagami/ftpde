#include "ftp.h"

/*! @fn static int RETR(ftp_session_t *session, const char *args)
 *
 *  @brief retrieve a file
 *
 *  @note Requires a PASV or PORT connection
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(RETR) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* open the file to retrieve */
    return ftp_xfer_file(session, args, XFER_FILE_RETR);
}

