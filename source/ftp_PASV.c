#include "ftp.h"

/*! @fn static int PASV(ftp_session_t *session, const char *args)
 *
 *  @brief request an address to connect to
 *
 *  @param[in] session ftp session
 *  @param[in] args    arguments
 *
 *  @returns error
 */
FTP_DECLARE(PASV) {
    int rc;
    char buffer[INET_ADDRSTRLEN + 10];
    char *p;
    in_port_t port;

    console_print(CYAN "%s %s\n" RESET, __func__, args ? args : "");

    memset(buffer, 0, sizeof(buffer));

    /* reset the state */
    ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
    session->flags &= ~(SESSION_PASV | SESSION_PORT);

    /* create a socket to listen on */
    session->pasv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (session->pasv_fd < 0) {
        console_print(RED "socket: %d %s\n" RESET, errno, strerror(errno));
        return ftp_send_response(session, 451, "\r\n");
    }

    /* set the socket options */
    rc = ftp_set_socket_options(session->pasv_fd);
    if (rc != 0) {
        /* failed to set socket options */
        ftp_session_close_pasv(session);
        return ftp_send_response(session, 451, "\r\n");
    }

    /* grab a new port */
    session->pasv_addr.sin_port = htons(next_data_port());

#ifdef _3DS
    console_print(YELLOW "binding to %s:%u\n" RESET,
                  inet_ntoa(session->pasv_addr.sin_addr),
                  ntohs(session->pasv_addr.sin_port));
#endif

    /* bind to the port */
    rc = bind(session->pasv_fd, (struct sockaddr *)&session->pasv_addr,
              sizeof(session->pasv_addr));
    if (rc != 0) {
        /* failed to bind */
        console_print(RED "bind: %d %s\n" RESET, errno, strerror(errno));
        ftp_session_close_pasv(session);
        return ftp_send_response(session, 451, "\r\n");
    }

    /* listen on the socket */
    rc = listen(session->pasv_fd, 1);
    if (rc != 0) {
        /* failed to listen */
        console_print(RED "listen: %d %s\n" RESET, errno, strerror(errno));
        ftp_session_close_pasv(session);
        return ftp_send_response(session, 451, "\r\n");
    }

#ifndef _3DS
    {
        /* get the socket address since we requested an ephemeral port */
        socklen_t addrlen = sizeof(session->pasv_addr);
        rc = getsockname(session->pasv_fd,
                         (struct sockaddr *)&session->pasv_addr, &addrlen);
        if (rc != 0) {
            /* failed to get socket address */
            console_print(RED "getsockname: %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_session_close_pasv(session);
            return ftp_send_response(session, 451, "\r\n");
        }
    }
#endif

    /* we are now listening on the socket */
    console_print(YELLOW "listening on %s:%u\n" RESET,
                  inet_ntoa(session->pasv_addr.sin_addr),
                  ntohs(session->pasv_addr.sin_port));
    session->flags |= SESSION_PASV;

    /* print the address in the ftp format */
    port = ntohs(session->pasv_addr.sin_port);
    strcpy(buffer, inet_ntoa(session->pasv_addr.sin_addr));
    sprintf(buffer + strlen(buffer), ",%u,%u", port >> 8, port & 0xFF);
    for (p = buffer; *p; ++p) {
        if (*p == '.')
            *p = ',';
    }

    return ftp_send_response(session, 227, "%s\r\n", buffer);
}

