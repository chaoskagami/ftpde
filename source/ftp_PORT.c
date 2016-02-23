#include "ftp.h"

/*! @fn static int PORT(ftp_session_t *session, const char *args)
 *
 *  @brief provide an address for the server to connect to
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(PORT) {
    char *addrstr, *p, *portstr;
    int commas = 0, rc;
    unsigned short port = 0; // Ports are uint16_t.
    unsigned long val;
    struct sockaddr_in addr;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    /* reset the state */
    ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
    session->flags &= ~(SESSION_PASV | SESSION_PORT);

    /* dup the args since they are const and we need to change it */
    addrstr = strdup(args);
    if (addrstr == NULL)
        return ftp_send_response(session, 425, "%s\r\n", strerror(ENOMEM));

    /* replace a,b,c,d,e,f with a.b.c.d\0e.f */
    for (p = addrstr; *p; ++p) {
        if (*p == ',') {
            if (commas != 3)
                *p = '.';
            else {
                *p = 0;
                portstr = p + 1;
            }
            ++commas;
        }
    }

    /* make sure we got the right number of values */
    if (commas != 5) {
        free(addrstr);
        return ftp_send_response(session, 501, "%s\r\n", strerror(EINVAL));
    }

    /* parse the address */
    rc = inet_aton(addrstr, &addr.sin_addr);
    if (rc == 0) {
        free(addrstr);
        return ftp_send_response(session, 501, "%s\r\n", strerror(EINVAL));
    }

    /* parse the port */
    val = 0;
    port = 0;
    for (p = portstr; *p; ++p) {
        if (!isdigit((int)*p)) {
            if (p == portstr || *p != '.' || val > 0xFF) {
                free(addrstr);
                return ftp_send_response(session, 501, "%s\r\n",
                                         strerror(EINVAL));
            }
            port <<= 8;
            port += val;
            val = 0;
        } else {
            val *= 10;
            val += *p - '0';
        }
    }

    /* validate the port */
    if (val > 0xFF || port > 0xFF) {
        free(addrstr);
        return ftp_send_response(session, 501, "%s\r\n", strerror(EINVAL));
    }

    port <<= 8;
    port += val;

    // Security consideration.
    if (port < 1024) { // We should immediately reject low port redirects.
        free(addrstr);
        return ftp_send_response(session, 501, "Rejected port %d for security.\r\n", port);
    }

    /* fill in the address port and family */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    free(addrstr);

    // Another security check - the IP should be the same as cmd.
    struct sockaddr_in addr_cmd;
    socklen_t addrlen_cmd = sizeof(addr_cmd);
    getpeername(session->cmd_fd, (struct sockaddr *)&addr_cmd, &addrlen_cmd);

    if(addr_cmd.sin_addr.s_addr != addr.sin_addr.s_addr) {
        // Reject non-identical address.
        free(addrstr);
        return ftp_send_response(session, 501, "IP does not match command port.\r\n");
    }

    memcpy(&session->peer_addr, &addr, sizeof(addr));

    /* we are ready to connect to the client */
    session->flags |= SESSION_PORT;
    return ftp_send_response(session, 200, "OK\r\n");
}

