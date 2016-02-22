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
        // TODO - Salt the password before hashing it.
        Sha256_Data((const unsigned char*)args, strlen(args), (unsigned char*)session->password_buf);

        char password[65];
        password[64] = 0;
        // This is a hexdumping algorithm. I use it all the time.
        for(int i=0; i < 32; i++) {
            password[(i*2)]   = ("0123456789abcdef")[(session->password_buf[i] & 0x0F)];
            password[(i*2)+1] = ("0123456789abcdef")[((session->password_buf[i] >> 4) & 0x0F)];
        }

        // Log the login attempt.
        console_print(RED "LOGIN: USER:%s PASS_SHA256:%s\n" RESET, session->username_buf, password);

        // TODO - implement access list
        session->auth_level = AUTH_READ|AUTH_WRITE; // Read/Write.

        return ftp_send_response(session, 230, "OK\r\n");
    }

    // We only reach this on failure.
    return ftp_send_response(session, 430, "Invalid username or password.\r\n");
}
