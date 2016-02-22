#include "ftp.h"

/*! @fn static int HELP(ftp_session_t *session, const char *args)
 *
 *  @brief print server help
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(HELP) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    /* list our accepted commands */
    return ftp_send_response(session, -214,
                             "The following commands are recognized\r\n"
                             " ABOR ALLO APPE CDUP CWD DELE FEAT HELP LIST "
                             "MDTM MKD MODE NLST NOOP\r\n"
                             " OPTS PASS PASV PORT PWD QUIT REST RETR RMD RNFR "
                             "RNTO SIZE STAT STOR\r\n"
                             " STOU STRU SYST TYPE USER XCUP XCWD XMKD XPWD XRMD\r\n"
                             "214 End\r\n");
}

