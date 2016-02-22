#include "ftp.h"

/*! @fn static int APPE(ftp_session_t *session, const char *args)
 *
 *  @brief append data to a file
 *
 *  @note requires a PASV or PORT connection
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(APPE) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* open the file in append mode */
    return ftp_xfer_file(session, args, XFER_FILE_APPE);
}

