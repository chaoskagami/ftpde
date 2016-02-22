#include "ftp.h"

/*! @fn static int NLST(ftp_session_t *session, const char *args)
 *
 *  @brief retrieve a name list
 *
 *  @note Requires a PASV or PORT connection
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(NLST) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* open the path in NLST mode */
    return ftp_xfer_dir(session, args, XFER_DIR_NLST, false);
}

