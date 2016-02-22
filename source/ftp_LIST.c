#include "ftp.h"

/*! @fn static int LIST(ftp_session_t *session, const char *args)
 *
 *  @brief retrieve a directory listing
 *
 *  @note Requires a PORT or PASV connection
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(LIST) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* open the path in LIST mode */
    return ftp_xfer_dir(session, args, XFER_DIR_LIST, true);
}

