#include "ftp.h"

/*! @fn static int PASS(ftp_session_t *session, const char *args)
 *
 *  @brief provide password
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(PASS) {
    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    ftp_session_set_state(session, COMMAND_STATE, 0);

    if (!strcmp(session->username_buf, "anonymous")) {
        // Just return. Anon is Read-only access and needs no password.
        return ftp_send_response(session, 230, "OK\r\n");
    } else {
        // actual user login. First - sha256 the password.
        // This is also a binary buffer, not plaintext.
        Sha256_Data((const unsigned char*)args, strlen(args), (unsigned char*)session->password_buf);

        // Log the login attempt.
        console_print(RED "login attempt: user:%s ph:", session->username_buf);
        for(int i=0; i < 64; i++) {
            char low  = ("0123456789abcdef")[(session->password_buf[i] & 0x0F)];
            char high = ("0123456789abcdef")[((session->password_buf[i] >> 4) & 0x0F)];
            console_print("%c%c", high, low);
        }
        console_print("\n" RESET);

        // TODO - implement access list
        session->auth_level = AUTH_READ|AUTH_WRITE; // Read/Write.

        return ftp_send_response(session, 230, "OK\r\n");
    }

    // We only reach this on failure.
    return ftp_send_response(session, 430, "Invalid username or password.\r\n");
}
