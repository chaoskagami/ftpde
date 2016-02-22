/* This is FTP server implementation is based on RFC 959
 * (https://tools.ietf.org/html/rfc959) and suggested implementation details
 * from https://cr.yp.to/ftp/filesystem.html
 */

#define NO_EXTERN_FTP
  #include "ftp.h"
#undef NO_EXTERN_FTP

#ifndef _3DS
  int LISTEN_PORT = 21;
#endif

#ifdef _3DS
  /*! SOC service buffer */
  u32 *SOCU_buffer = NULL;
#endif

/*! server listen address */
struct sockaddr_in serv_addr;

/*! listen file descriptor */
int listenfd = -1;

#ifdef _3DS
  /*! current data port */
  in_port_t data_port = DATA_PORT;
#endif

/*! list of ftp sessions */
ftp_session_t *sessions = NULL;

/*! socket buffersize */
int sock_buffersize = SOCK_BUFFERSIZE;

/*! server start time */
time_t start_time = 0;

/*! root directory to jail to. */
char* ftp_root_dir = NULL;

/*! Whether to disallow writes. */
int ftp_readonly_mode = 0;

/*! ftp command list */
ftp_command_t ftp_commands[] = {
    FTP_COMMAND(ABOR),     FTP_COMMAND(ALLO),    FTP_COMMAND(APPE),
    FTP_COMMAND(CDUP),     FTP_COMMAND(CWD),     FTP_COMMAND(DELE),
    FTP_COMMAND(FEAT),     FTP_COMMAND(HELP),    FTP_COMMAND(LIST),
    FTP_COMMAND(MDTM),     FTP_COMMAND(MKD),     FTP_COMMAND(MODE),
    FTP_COMMAND(NLST),     FTP_COMMAND(NOOP),    FTP_COMMAND(OPTS),
    FTP_COMMAND(PASS),     FTP_COMMAND(PASV),    FTP_COMMAND(PORT),
    FTP_COMMAND(PWD),      FTP_COMMAND(QUIT),    FTP_COMMAND(REST),
    FTP_COMMAND(RETR),     FTP_COMMAND(RMD),     FTP_COMMAND(RNFR),
    FTP_COMMAND(RNTO),     FTP_COMMAND(SIZE),    FTP_COMMAND(STAT),
    FTP_COMMAND(STOR),     FTP_COMMAND(STOU),    FTP_COMMAND(STRU),
    FTP_COMMAND(SYST),     FTP_COMMAND(TYPE),     FTP_COMMAND(USER),
    FTP_ALIAS(XCUP, CDUP), FTP_ALIAS(XCWD, CWD), FTP_ALIAS(XMKD, MKD),
    FTP_ALIAS(XPWD, PWD),  FTP_ALIAS(XRMD, RMD),
};

/*! number of ftp commands */
const size_t num_ftp_commands =
    sizeof(ftp_commands) / sizeof(ftp_commands[0]);

/*! check whether the session should reject a write command
 *
 */
int ftp_check_reject_write(ftp_session_t* session) {
    if ((session->auth_level & AUTH_WRITE) != AUTH_WRITE || ftp_readonly_mode == 1) {
        // Invalid. Lacking proper auth.
        return ftp_send_response(session, 530, "Not permitted.\r\n");
    }
    return 0;
}

/*! compare ftp command descriptors
 *
 *  @param[in] p1 left side of comparison (ftp_command_t*)
 *  @param[in] p2 right side of comparison (ftp_command_t*)
 *
 *  @returns <0 if p1 <  p2
 *  @returns 0 if  p1 == p2
 *  @returns >0 if p1 >  p2
 */
int ftp_command_cmp(const void *p1, const void *p2) {
    ftp_command_t *c1 = (ftp_command_t *)p1;
    ftp_command_t *c2 = (ftp_command_t *)p2;

    /* ordered by command name */
    return strcasecmp(c1->name, c2->name);
}

/*! Allocate a new data port
 *
 *  @returns next data port
 */
in_port_t next_data_port(void) {
#ifdef _3DS
    if (++data_port >= 10000)
        data_port = DATA_PORT;
    return data_port;
#else
    return 0; /* ephemeral port */
#endif
}

/*! set a socket to non-blocking
 *
 *  @param[in] fd socket
 *
 *  @returns error
 */
int ftp_set_socket_nonblocking(int fd) {
    int rc, flags;

    /* get the socket flags */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        console_print(RED "fcntl: %d %s\n" RESET, errno, strerror(errno));
        return -1;
    }

    /* add O_NONBLOCK to the socket flags */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc != 0) {
        console_print(RED "fcntl: %d %s\n" RESET, errno, strerror(errno));
        return -1;
    }

    return 0;
}

/*! set socket options
 *
 *  @param[in] fd socket
 *
 *  @returns failure
 */
int ftp_set_socket_options(int fd) {
    int rc;

    /* increase receive buffer size */
    rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sock_buffersize,
                    sizeof(sock_buffersize));
    if (rc != 0) {
        console_print(RED "setsockopt: SO_RCVBUF %d %s\n" RESET, errno,
                      strerror(errno));
        return -1;
    }

    /* increase send buffer size */
    rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sock_buffersize,
                    sizeof(sock_buffersize));
    if (rc != 0) {
        console_print(RED "setsockopt: SO_SNDBUF %d %s\n" RESET, errno,
                      strerror(errno));
        return -1;
    }

    return 0;
}

/*! close a socket
 *
 *  @param[in] fd        socket to close
 *  @param[in] connected whether this socket is connected
 */
void ftp_closesocket(int fd, bool connected) {
    int rc;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct pollfd pollinfo;

    if (connected) {
        /* get peer address and print */
        rc = getpeername(fd, (struct sockaddr *)&addr, &addrlen);
        if (rc != 0) {
            console_print(RED "getpeername: %d %s\n" RESET, errno,
                          strerror(errno));
            console_print(YELLOW "closing connection to fd=%d\n" RESET, fd);
        } else
            console_print(YELLOW "closing connection to %s:%u\n" RESET,
                          inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        /* shutdown connection */
        rc = shutdown(fd, SHUT_WR);
        if (rc != 0)
            console_print(RED "shutdown: %d %s\n" RESET, errno,
                          strerror(errno));

        /* wait for client to close connection */
        pollinfo.fd = fd;
        pollinfo.events = POLLIN;
        pollinfo.revents = 0;
        rc = poll(&pollinfo, 1, 250);
        if (rc < 0)
            console_print(RED "poll: %d %s\n" RESET, errno, strerror(errno));
    }

    /* close socket */
    rc = close(fd);
    if (rc != 0)
        console_print(RED "close: %d %s\n" RESET, errno, strerror(errno));
}

/*! close command socket on ftp session
 *
 *  @param[in] session ftp session
 */
void ftp_session_close_cmd(ftp_session_t *session) {
    /* close command socket */
    if (session->cmd_fd >= 0)
        ftp_closesocket(session->cmd_fd, true);
    session->cmd_fd = -1;
}

/*! close listen socket on ftp session
 *
 *  @param[in] session ftp session
 */
void ftp_session_close_pasv(ftp_session_t *session) {
    /* close pasv socket */
    if (session->pasv_fd >= 0) {
        console_print(YELLOW "stop listening on %s:%u\n" RESET,
                      inet_ntoa(session->pasv_addr.sin_addr),
                      ntohs(session->pasv_addr.sin_port));

        ftp_closesocket(session->pasv_fd, false);
    }

    session->pasv_fd = -1;
}

/*! close data socket on ftp session
 *
 *  @param[in] session ftp session
 */
void ftp_session_close_data(ftp_session_t *session) {
    /* close data connection */
    if (session->data_fd >= 0 && session->data_fd != session->cmd_fd)
        ftp_closesocket(session->data_fd, true);
    session->data_fd = -1;

    /* clear send/recv flags */
    session->flags &= ~(SESSION_RECV | SESSION_SEND);
}

/*! close open file for ftp session
 *
 *  @param[in] session ftp session
 */
void ftp_session_close_file(ftp_session_t *session) {
    int rc;

    if (session->fp != NULL) {
        rc = fclose(session->fp);
        if (rc != 0)
            console_print(RED "fclose: %d %s\n" RESET, errno, strerror(errno));
    }

    session->fp = NULL;
    session->filepos = 0;
}

/*! open file for reading for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns -1 for error
 */
int ftp_session_open_file_read(ftp_session_t *session) {
    int rc;
    struct stat st;

    /* open file in read mode */
    session->fp = fopen(session->buffer, "rb");
    if (session->fp == NULL) {
        console_print(RED "fopen '%s': %d %s\n" RESET, session->buffer, errno,
                      strerror(errno));
        return -1;
    }

    /* it's okay if this fails */
    errno = 0;
    rc = setvbuf(session->fp, session->file_buffer, _IOFBF, FILE_BUFFERSIZE);
    if (rc != 0) {
        console_print(RED "setvbuf: %d %s\n" RESET, errno, strerror(errno));
    }

    /* get the file size */
    rc = fstat(fileno(session->fp), &st);
    if (rc != 0) {
        console_print(RED "fstat '%s': %d %s\n" RESET, session->buffer, errno,
                      strerror(errno));
        return -1;
    }
    session->filesize = st.st_size;

    if (session->filepos != 0) {
        rc = fseek(session->fp, session->filepos, SEEK_SET);
        if (rc != 0) {
            console_print(RED "fseek '%s': %d %s\n" RESET, session->buffer,
                          errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

/*! read from an open file for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns bytes read
 */
ssize_t ftp_session_read_file(ftp_session_t *session) {
    ssize_t rc;

    /* read file at current position */
    rc = fread(session->buffer, 1, sizeof(session->buffer), session->fp);
    if (rc < 0) {
        console_print(RED "fread: %d %s\n" RESET, errno, strerror(errno));
        return -1;
    }

    /* adjust file position */
    session->filepos += rc;

    return rc;
}

/*! open file for writing for ftp session
 *
 *  @param[in] session ftp session
 *  @param[in] append  whether to append
 *
 *  @returns -1 for error
 *
 *  @note truncates file
 */
int ftp_session_open_file_write(ftp_session_t *session, bool append) {
    int rc;
    const char *mode = "wb";

    if (append)
        mode = "ab";
    else if (session->filepos != 0)
        mode = "r+b";

    /* open file in write mode */
    session->fp = fopen(session->buffer, mode);
    if (session->fp == NULL) {
        console_print(RED "fopen '%s': %d %s\n" RESET, session->buffer, errno,
                      strerror(errno));
        return -1;
    }

    /* it's okay if this fails */
    errno = 0;
    rc = setvbuf(session->fp, session->file_buffer, _IOFBF, FILE_BUFFERSIZE);
    if (rc != 0) {
        console_print(RED "setvbuf: %d %s\n" RESET, errno, strerror(errno));
    }

    /* check if this had REST but not APPE */
    if (session->filepos != 0 && !append) {
        /* seek to the REST offset */
        rc = fseek(session->fp, session->filepos, SEEK_SET);
        if (rc != 0) {
            console_print(RED "fseek '%s': %d %s\n" RESET, session->buffer,
                          errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

/*! write to an open file for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns bytes written
 */
ssize_t ftp_session_write_file(ftp_session_t *session) {
    ssize_t rc;

    /* write to file at current position */
    rc = fwrite(session->buffer + session->bufferpos, 1,
                session->buffersize - session->bufferpos, session->fp);
    if (rc < 0) {
        console_print(RED "fwrite: %d %s\n" RESET, errno, strerror(errno));
        return -1;
    } else if (rc == 0)
        console_print(RED "fwrite: wrote 0 bytes\n" RESET);

    /* adjust file position */
    session->filepos += rc;

    return rc;
}

/*! close current working directory for ftp session
 *
 *   @param[in] session ftp session
 */
void ftp_session_close_cwd(ftp_session_t *session) {
    int rc;

    /* close open directory pointer */
    if (session->dp != NULL) {
        rc = closedir(session->dp);
        if (rc != 0)
            console_print(RED "closedir: %d %s\n" RESET, errno,
                          strerror(errno));
    }
    session->dp = NULL;
}

/*! open current working directory for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @return -1 for failure
 */
int ftp_session_open_cwd(ftp_session_t *session) {
    /* open current working directory */
    session->dp = opendir(session->cwd);
    if (session->dp == NULL) {
        console_print(RED "opendir '%s': %d %s\n" RESET, session->cwd, errno,
                      strerror(errno));
        return -1;
    }

    return 0;
}

/*! set state for ftp session
 *
 *  @param[in] session ftp session
 *  @param[in] state   state to set
 *  @param[in] flags   flags
 */
void ftp_session_set_state(ftp_session_t *session, session_state_t state,
                                  set_state_flags_t flags) {
    session->state = state;

    /* close pasv and data sockets */
    if (flags & CLOSE_PASV)
        ftp_session_close_pasv(session);
    if (flags & CLOSE_DATA)
        ftp_session_close_data(session);

    if (state == COMMAND_STATE) {
        /* close file/cwd */
        ftp_session_close_file(session);
        ftp_session_close_cwd(session);
    }
}

/*! transfer loop
 *
 *  Try to transfer as much data as the sockets will allow without blocking
 *
 *  @param[in] session ftp session
 */
void ftp_session_transfer(ftp_session_t *session) {
    int rc;
    do {
        rc = session->transfer(session);
    } while (rc == 0);
}

/*! encode a path
 *
 *  @param[in]     path   path to encode
 *  @param[in,out] len    path length
 *  @param[in]     quotes whether to encode quotes
 *
 *  @returns encoded path
 *
 *  @note The caller must free the returned path
 */
char *encode_path(const char *path, size_t *len, bool quotes) {
    bool enc = false;
    size_t i, diff = 0;
    char *out, *p = (char *)path;

    /* check for \n that needs to be encoded */
    if (memchr(p, '\n', *len) != NULL)
        enc = true;

    if (quotes) {
        /* check for " that needs to be encoded */
        p = (char *)path;
        do {
            p = memchr(p, '"', path + *len - p);
            if (p != NULL) {
                ++p;
                ++diff;
            }
        } while (p != NULL);
    }

    /* check if an encode was needed */
    if (!enc && diff == 0)
        return strdup(path);

    /* allocate space for encoded path */
    p = out = (char *)malloc(*len + diff);
    if (out == NULL)
        return NULL;

    /* copy the path while performing encoding */
    for (i = 0; i < *len; ++i) {
        if (*path == '\n') {
            /* encoded \n is \0 */
            *p++ = 0;
        } else if (quotes && *path == '"') {
            /* encoded " is "" */
            *p++ = '"';
            *p++ = '"';
        } else
            *p++ = *path;
        ++path;
    }

    *len += diff;
    return out;
}

/*! decode a path
 *
 *  @param[in] session ftp session
 *  @param[in] len     command length
 */
void decode_path(ftp_session_t *session, size_t len) {
    size_t i;

    /* decode \0 from the first command */
    for (i = 0; i < len; ++i) {
        /* this is an encoded \n */
        if (session->cmd_buffer[i] == 0)
            session->cmd_buffer[i] = '\n';
    }
}

/*! send a response on the command socket
 *
 *  @param[in] session ftp session
 *  @param[in] buffer  buffer to send
 *  @param[in] len     buffer length
 *
 *  @returns bytes sent to peer
 */
ssize_t ftp_send_response_buffer(ftp_session_t *session,
                                        const char *buffer, size_t len, int succeed) {
    ssize_t rc, to_send;

    /* send response */
    to_send = len;
    if (succeed)
        console_print(GREEN "%s" RESET, buffer);
    else
        console_print(RED "%s" RESET, buffer);
    rc = send(session->cmd_fd, buffer, to_send, 0);
    if (rc < 0)
        console_print(RED "send: %d %s\n" RESET, errno, strerror(errno));
    else if (rc != to_send)
        console_print(RED "only sent %u/%u bytes\n" RESET, (unsigned int)rc,
                      (unsigned int)to_send);

    return rc;
}

__attribute__((format(printf, 3, 4)))
/*! send ftp response to ftp session's peer
 *
 *  @param[in] session ftp session
 *  @param[in] code    response code
 *  @param[in] fmt     format string
 *  @param[in] ...     format arguments
 *
 *  returns bytes sent to peer
 */
ssize_t
ftp_send_response(ftp_session_t *session, int code, const char *fmt, ...) {
    static char buffer[CMD_BUFFERSIZE];
    ssize_t rc;
    va_list ap;

    /* print response code and message to buffer */
    va_start(ap, fmt);
    if (code > 0)
        rc = sprintf(buffer, "%d ", code);
    else
        rc = sprintf(buffer, "%d-", -code);
    rc += vsnprintf(buffer + rc, sizeof(buffer) - rc, fmt, ap);
    va_end(ap);

    if (rc >= sizeof(buffer)) {
        /* couldn't fit message; just send code */
        console_print(RED "%s: buffersize too small\n" RESET, __func__);
        if (code > 0)
            rc = sprintf(buffer, "%d \r\n", code);
        else
            rc = sprintf(buffer, "%d-\r\n", -code);
    }

    return ftp_send_response_buffer(session, buffer, rc, CODE_TO_STATBOOL(code));
}

/*! destroy ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns the next session in the list
 */
ftp_session_t *ftp_session_destroy(ftp_session_t *session) {
    ftp_session_t *next = session->next;

    /* close all sockets/files */
    ftp_session_close_cmd(session);
    ftp_session_close_pasv(session);
    ftp_session_close_data(session);
    ftp_session_close_file(session);
    ftp_session_close_cwd(session);

    /* unlink from sessions list */
    if (session->next)
        session->next->prev = session->prev;
    if (session == sessions)
        sessions = session->next;
    else {
        session->prev->next = session->next;
        if (session == sessions->prev)
            sessions->prev = session->prev;
    }

    /* deallocate */
    free(session);

    return next;
}

/*! allocate new ftp session
 *
 *  @param[in] listen_fd socket to accept connection from
 */
void ftp_session_new(int listen_fd) {
    ssize_t rc;
    int new_fd;
    ftp_session_t *session;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char host[NI_MAXHOST + 1];
    char serv[NI_MAXSERV + 1];

    /* accept connection */
    new_fd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
    if (new_fd < 0) {
        console_print(RED "accept: %d %s\n" RESET, errno, strerror(errno));
        return;
    }

    memset(host, 0, sizeof(host));
    memset(serv, 0, sizeof(serv));

    /* reverse dns lookup */
    rc = getnameinfo((struct sockaddr *)&addr, addrlen, host, sizeof(host),
                     serv, sizeof(serv), 0);
    if (rc != 0)
        console_print(CYAN "accepted connection from %s:%u\n" RESET,
                      inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    else
        console_print(CYAN "accepted connection from %s:%s\n" RESET, host,
                      serv);

    /* allocate a new session */
    session = (ftp_session_t *)calloc(1, sizeof(ftp_session_t));
    if (session == NULL) {
        console_print(RED "failed to allocate session\n" RESET);
        ftp_closesocket(new_fd, true);
        return;
    }

	// Support for a 'root' directory other than /.
    if (ftp_root_dir != NULL)
        strcpy(session->root_dir, ftp_root_dir);
    else {
#ifdef _3DS
        strcpy(session->root_dir, "/"); // Just do the root on 3DS. Nothing fancy.
#else
        getcwd(session->root_dir, 4096); // The default on linux is 'pwd' if not set.
#endif
    }

    /* initialize session */
    strcpy(session->cwd, session->root_dir);
    session->peer_addr.sin_addr.s_addr = INADDR_ANY;
    session->cmd_fd = new_fd;
    session->pasv_fd = -1;
    session->data_fd = -1;
    session->state = COMMAND_STATE;

    /* link to the sessions list */
    if (sessions == NULL) {
        sessions = session;
        session->prev = session;
    } else {
        sessions->prev->next = session;
        session->prev = sessions->prev;
        sessions->prev = session;
    }

    /* copy socket address to pasv address */
    addrlen = sizeof(session->pasv_addr);
    rc = getsockname(new_fd, (struct sockaddr *)&session->pasv_addr, &addrlen);
    if (rc != 0) {
        console_print(RED "getsockname: %d %s\n" RESET, errno, strerror(errno));
        ftp_send_response(session, 451, "Failed to get connection info\r\n");
        ftp_session_destroy(session);
        return;
    }

    session->cmd_fd = new_fd;

    /* send initiator response */
    rc = ftp_send_response(session, 220, "Hello!\r\n");
    if (rc <= 0)
        ftp_session_destroy(session);
}

/*! accept PASV connection for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns -1 for failure
 */
int ftp_session_accept(ftp_session_t *session) {
    int rc, new_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (session->flags & SESSION_PASV) {
        /* clear PASV flag */
        session->flags &= ~SESSION_PASV;

        /* tell the peer that we're ready to accept the connection */
        ftp_send_response(session, 150, "Ready\r\n");

        /* accept connection from peer */
        new_fd = accept(session->pasv_fd, (struct sockaddr *)&addr, &addrlen);
        if (new_fd < 0) {
            console_print(RED "accept: %d %s\n" RESET, errno, strerror(errno));
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            ftp_send_response(session, 425,
                              "Failed to establish connection\r\n");
            return -1;
        }

        /* set the socket to non-blocking */
        rc = ftp_set_socket_nonblocking(new_fd);
        if (rc != 0) {
            ftp_closesocket(new_fd, true);
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            ftp_send_response(session, 425,
                              "Failed to establish connection\r\n");
            return -1;
        }

        console_print(CYAN "accepted connection from %s:%u\n" RESET,
                      inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        /* we are ready to transfer data */
        ftp_session_set_state(session, DATA_TRANSFER_STATE, CLOSE_PASV);
        session->data_fd = new_fd;

        return 0;
    } else {
        /* peer didn't send PASV command */
        ftp_send_response(session, 503, "Bad sequence of commands\r\n");
        return -1;
    }
}

/*! connect to peer for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns -1 for failure
 */
int ftp_session_connect(ftp_session_t *session) {
    int rc;

    /* clear PORT flag */
    session->flags &= ~SESSION_PORT;

    /* create a new socket */
    session->data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (session->data_fd < 0) {
        console_print(RED "socket: %d %s\n" RESET, errno, strerror(errno));
        return -1;
    }

    /* set socket options */
    rc = ftp_set_socket_options(session->data_fd);
    if (rc != 0) {
        ftp_closesocket(session->data_fd, false);
        session->data_fd = -1;
        return -1;
    }

    /* connect to peer */
    rc = connect(session->data_fd, (struct sockaddr *)&session->peer_addr,
                 sizeof(session->peer_addr));
    if (rc != 0) {
        console_print(RED "connect: %d %s\n" RESET, errno, strerror(errno));
        ftp_closesocket(session->data_fd, false);
        session->data_fd = -1;
        return -1;
    }

    /* set socket to non-blocking */
    rc = ftp_set_socket_nonblocking(session->data_fd);
    if (rc != 0)
        return -1;

    console_print(CYAN "connected to %s:%u\n" RESET,
                  inet_ntoa(session->peer_addr.sin_addr),
                  ntohs(session->peer_addr.sin_port));

    return 0;
}

/*! read command for ftp session
 *
 *  @param[in] session ftp session
 *  @param[in] events  poll events
 */
void ftp_session_read_command(ftp_session_t *session, int events) {
    char *buffer, *args, *next = NULL;
    size_t i, len;
    int atmark;
    ssize_t rc;
    ftp_command_t key, *command;

    /* check out-of-band data */
    if (events & POLLPRI) {
        session->flags |= SESSION_URGENT;

        /* check if we are at the urgent marker */
        atmark = sockatmark(session->cmd_fd);
        if (atmark < 0) {
            console_print(RED "sockatmark: %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_session_close_cmd(session);
            return;
        }

        if (!atmark) {
            /* discard in-band data */
            rc = recv(session->cmd_fd, session->cmd_buffer,
                      sizeof(session->cmd_buffer), 0);
            if (rc < 0 && errno != EWOULDBLOCK) {
                console_print(RED "recv: %d %s\n" RESET, errno,
                              strerror(errno));
                ftp_session_close_cmd(session);
            }

            return;
        }

        /* retrieve the urgent data */
        rc = recv(session->cmd_fd, session->cmd_buffer,
                  sizeof(session->cmd_buffer), MSG_OOB);
        if (rc < 0) {
            /* EWOULDBLOCK means out-of-band data is on the way */
            if (errno == EWOULDBLOCK)
                return;

            /* error retrieving out-of-band data */
            console_print(RED "recv (oob): %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_session_close_cmd(session);
            return;
        }

        /* reset the command buffer */
        session->cmd_buffersize = 0;
        return;
    }

    /* prepare to receive data */
    buffer = session->cmd_buffer + session->cmd_buffersize;
    len = sizeof(session->cmd_buffer) - session->cmd_buffersize;
    if (len == 0) {
        /* error retrieving command */
        console_print(RED "Exceeded command buffer size\n" RESET);
        ftp_session_close_cmd(session);
        return;
    }

    /* retrieve command data */
    rc = recv(session->cmd_fd, buffer, len, 0);
    if (rc < 0) {
        /* error retrieving command */
        console_print(RED "recv: %d %s\n" RESET, errno, strerror(errno));
        ftp_session_close_cmd(session);
        return;
    }
    if (rc == 0) {
        /* peer closed connection */
        ftp_session_close_cmd(session);
        return;
    } else {
        session->cmd_buffersize += rc;
        len = sizeof(session->cmd_buffer) - session->cmd_buffersize;

        if (session->flags & SESSION_URGENT) {
            /* look for telnet data mark */
            for (i = 0; i < session->cmd_buffersize; ++i) {
                if ((unsigned char)session->cmd_buffer[i] == 0xF2) {
                    /* ignore all data that precedes the data mark */
                    if (i < session->cmd_buffersize - 1)
                        memmove(session->cmd_buffer,
                                session->cmd_buffer + i + 1, len - i - 1);
                    session->cmd_buffersize -= i + 1;
                    session->flags &= ~SESSION_URGENT;
                    break;
                }
            }
        }

        /* loop through commands */
        while (true) {
            /* must have at least enough data for the delimiter */
            if (session->cmd_buffersize < 1)
                return;

            /* look for \r\n or \n delimiter */
            for (i = 0; i < session->cmd_buffersize; ++i) {
                if (i < session->cmd_buffersize - 1 &&
                    session->cmd_buffer[i] == '\r' &&
                    session->cmd_buffer[i + 1] == '\n') {
                    /* we found a \r\n delimiter */
                    session->cmd_buffer[i] = 0;
                    next = &session->cmd_buffer[i + 2];
                    break;
                } else if (session->cmd_buffer[i] == '\n') {
                    /* we found a \n delimiter */
                    session->cmd_buffer[i] = 0;
                    next = &session->cmd_buffer[i + 1];
                    break;
                }
            }

            /* check if a delimiter was found */
            if (i == session->cmd_buffersize)
                return;

            /* decode the command */
            decode_path(session, i);

            /* split command from arguments */
            args = buffer = session->cmd_buffer;
            while (*args && !isspace((int)*args))
                ++args;
            if (*args)
                *args++ = 0;

            /* look up the command */
            key.name = buffer;
            command = bsearch(&key, ftp_commands, num_ftp_commands,
                              sizeof(ftp_command_t), ftp_command_cmp);

            /* update command timestamp */
            session->timestamp = time(NULL);

            /* execute the command */
            if (command == NULL) {
                /* send header */
                ftp_send_response(session, 502, "Invalid command \"");

                /* send command */
                len = strlen(buffer);
                buffer = encode_path(buffer, &len, false);
                if (buffer != NULL)
                    ftp_send_response_buffer(session, buffer, len, 1);
                else
                    ftp_send_response_buffer(session, key.name,
                                             strlen(key.name), 1);
                free(buffer);

                /* send args (if any) */
                if (*args != 0) {
                    len = strlen(args);
                    buffer = encode_path(args, &len, false);
                    if (buffer != NULL)
                        ftp_send_response_buffer(session, buffer, len, 1);
                    else
                        ftp_send_response_buffer(session, args, strlen(args), 1);
                    free(buffer);
                }

                /* send footer */
                ftp_send_response_buffer(session, "\"\r\n", 3, 1);
            } else if (session->state != COMMAND_STATE) {
                /* only some commands are available during data transfer */
                if (strcasecmp(command->name, "ABOR") != 0 &&
                    strcasecmp(command->name, "STAT") != 0 &&
                    strcasecmp(command->name, "QUIT") != 0) {
                    ftp_send_response(session, 503,
                                      "Invalid command during transfer\r\n");
                    ftp_session_set_state(session, COMMAND_STATE,
                                          CLOSE_PASV | CLOSE_DATA);
                    ftp_session_close_cmd(session);
                } else
                    command->handler(session, args);
            } else {
                /* clear RENAME flag for all commands except RNTO */
                if (strcasecmp(command->name, "RNTO") != 0)
                    session->flags &= ~SESSION_RENAME;

                command->handler(session, args);
            }

            /* remove executed command from the command buffer */
            len = session->cmd_buffer + session->cmd_buffersize - next;
            if (len > 0)
                memmove(session->cmd_buffer, next, len);
            session->cmd_buffersize = len;
        }
    }
}

/*! poll sockets for ftp session
 *
 *  @param[in] session ftp session
 *
 *  @returns next session
 */
ftp_session_t *ftp_session_poll(ftp_session_t *session) {
    int rc;
    struct pollfd pollinfo[2];
    nfds_t nfds = 1;

    /* the first pollfd is the command socket */
    pollinfo[0].fd = session->cmd_fd;
    pollinfo[0].events = POLLIN | POLLPRI;
    pollinfo[0].revents = 0;

    switch (session->state) {
    case COMMAND_STATE:
        /* we are waiting to read a command */
        break;

    case DATA_CONNECT_STATE:
        /* we are waiting for a PASV connection */
        pollinfo[1].fd = session->pasv_fd;
        pollinfo[1].events = POLLIN;
        pollinfo[1].revents = 0;
        nfds = 2;
        break;

    case DATA_TRANSFER_STATE:
        /* we need to transfer data */
        pollinfo[1].fd = session->data_fd;
        if (session->flags & SESSION_RECV)
            pollinfo[1].events = POLLIN;
        else
            pollinfo[1].events = POLLOUT;
        pollinfo[1].revents = 0;
        nfds = 2;
        break;
    }

    /* poll the selected sockets */
#ifdef _3DS
    rc = poll(pollinfo, nfds, 0);   // Waiting is pointless on 3DS, single task system and all that.
#else
    rc = poll(pollinfo, nfds, 100); // 100ms for CPU usage safety.
#endif
    if (rc < 0) {
        console_print(RED "poll: %d %s\n" RESET, errno, strerror(errno));
        ftp_session_close_cmd(session);
    } else if (rc > 0) {
        /* check the command socket */
        if (pollinfo[0].revents != 0) {
            /* handle command */
            if (pollinfo[0].revents & POLL_UNKNOWN)
                console_print(YELLOW "cmd_fd: revents=0x%08X\n" RESET,
                              pollinfo[0].revents);

            /* we need to read a new command */
            if (pollinfo[0].revents & (POLLERR | POLLHUP))
                ftp_session_close_cmd(session);
            else if (pollinfo[0].revents & (POLLIN | POLLPRI))
                ftp_session_read_command(session, pollinfo[0].revents);
        }

        /* check the data/pasv socket */
        if (nfds > 1 && pollinfo[1].revents != 0) {
            switch (session->state) {
            case COMMAND_STATE:
                /* this shouldn't happen? */
                break;

            case DATA_CONNECT_STATE:
                if (pollinfo[1].revents & POLL_UNKNOWN)
                    console_print(YELLOW "pasv_fd: revents=0x%08X\n" RESET,
                                  pollinfo[1].revents);

                /* we need to accept the PASV connection */
                if (pollinfo[1].revents & (POLLERR | POLLHUP)) {
                    ftp_session_set_state(session, COMMAND_STATE,
                                          CLOSE_PASV | CLOSE_DATA);
                    ftp_send_response(session, 426,
                                      "Data connection failed\r\n");
                } else if (pollinfo[1].revents & POLLIN) {
                    if (ftp_session_accept(session) != 0)
                        ftp_session_set_state(session, COMMAND_STATE,
                                              CLOSE_PASV | CLOSE_DATA);
                }
                break;

            case DATA_TRANSFER_STATE:
                if (pollinfo[1].revents & POLL_UNKNOWN)
                    console_print(YELLOW "data_fd: revents=0x%08X\n" RESET,
                                  pollinfo[1].revents);

                /* we need to transfer data */
                if (pollinfo[1].revents & (POLLERR | POLLHUP)) {
                    ftp_session_set_state(session, COMMAND_STATE,
                                          CLOSE_PASV | CLOSE_DATA);
                    ftp_send_response(session, 426,
                                      "Data connection failed\r\n");
                } else if (pollinfo[1].revents & (POLLIN | POLLOUT))
                    ftp_session_transfer(session);
                break;
            }
        }
    }

    /* still connected to peer; return next session */
    if (session->cmd_fd >= 0)
        return session->next;

    /* disconnected from peer; destroy it and return next session */
    return ftp_session_destroy(session);
}

/*! initialize ftp subsystem */
int ftp_init(int port, char* root_d, int readonly) {
    int rc;

    start_time = time(NULL);

    if (readonly == 1)
        ftp_readonly_mode = readonly;

#ifdef _3DS
    Result ret = 0;
    u32 wifi = 0;
    bool loop;

    console_print(GREEN "Waiting for wifi...\n" RESET);

    /* wait for wifi to be available */
    while ((loop = aptMainLoop()) && !wifi && (ret == 0 || ret == 0xE0A09D2E)) {
        ret = 0;

        hidScanInput();
        if (hidKeysDown() & KEY_B) {
            /* user canceled */
            loop = false;
            break;
        }

        /* update the wifi status */
        ret = ACU_GetWifiStatus(&wifi);
        if (ret != 0)
            wifi = 0;
    }

    /* check if there was a wifi error */
    if (ret != 0)
        console_print(RED "ACU_GetWifiStatus returns 0x%lx\n" RESET, ret);

    /* check if we need to exit */
    if (!loop || ret != 0)
        return -1;

    console_print(GREEN "Ready!\n" RESET);

#ifdef ENABLE_LOGGING
    /* open log file */
    FILE *fp = freopen("/ftpde.log", "wb", stderr);
    if (fp == NULL) {
        console_print(RED "freopen: 0x%08X\n" RESET, errno);
        goto log_fail;
    }

    /* truncate log file */
    if (ftruncate(fileno(fp), 0) != 0) {
        console_print(RED "ftruncate: 0x%08X\n" RESET, errno);
        goto ftruncate_fail;
    }
#endif

    /* allocate buffer for SOC service */
    SOCU_buffer = (u32 *)memalign(SOCU_ALIGN, SOCU_BUFFERSIZE);
    if (SOCU_buffer == NULL) {
        console_print(RED "memalign: failed to allocate\n" RESET);
        goto memalign_fail;
    }

    /* initialize SOC service */
    ret = socInit(SOCU_buffer, SOCU_BUFFERSIZE);
    if (ret != 0) {
        console_print(RED "socInit: 0x%08X\n" RESET, (unsigned int)ret);
        goto soc_fail;
    }
#endif

    /* allocate socket to listen for clients */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        console_print(RED "socket: %d %s\n" RESET, errno, strerror(errno));
        ftp_exit();
        return -1;
    }

#ifndef _3DS
    if (port != 0)
        LISTEN_PORT = port;
    if (root_d != NULL)
        ftp_root_dir = root_d;
#endif

    /* get address to listen on */
    serv_addr.sin_family = AF_INET;
#ifdef _3DS
    serv_addr.sin_addr.s_addr = gethostid();
    serv_addr.sin_port = htons(LISTEN_PORT);
#else
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(LISTEN_PORT);
#endif

    /* reuse address */
    {
        int yes = 1;
        rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (rc != 0) {
            console_print(RED "setsockopt: %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_exit();
            return -1;
        }
    }

    /* bind socket to listen address */
    rc = bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc != 0) {
        console_print(RED "bind: %d %s\n" RESET, errno, strerror(errno));
        ftp_exit();
        return -1;
    }

    /* listen on socket */
    rc = listen(listenfd, 5);
    if (rc != 0) {
        console_print(RED "listen: %d %s\n" RESET, errno, strerror(errno));
        ftp_exit();
        return -1;
    }

/* print server address */
#ifdef _3DS
    console_set_status("\n" GREEN STATUS_STRING " " YELLOW "Host:" CYAN
                       "%s " YELLOW "Port:" CYAN "%u" RESET,
                       inet_ntoa(serv_addr.sin_addr),
                       ntohs(serv_addr.sin_port));
#else
    {
        char hostname[128];
        socklen_t addrlen = sizeof(serv_addr);
        rc = getsockname(listenfd, (struct sockaddr *)&serv_addr, &addrlen);
        if (rc != 0) {
            console_print(RED "getsockname: %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_exit();
            return -1;
        }

        rc = gethostname(hostname, sizeof(hostname));
        if (rc != 0) {
            console_print(RED "gethostname: %d %s\n" RESET, errno,
                          strerror(errno));
            ftp_exit();
            return -1;
        }

        console_set_status(GREEN STATUS_STRING " " YELLOW "Host:" CYAN
                                               "%s " YELLOW "Port:" CYAN
                                               "%u" RESET,
                           hostname, ntohs(serv_addr.sin_port));
    }
#endif

    return 0;

#ifdef _3DS
soc_fail:
    free(SOCU_buffer);

memalign_fail:
#ifdef ENABLE_LOGGING
ftruncate_fail:
    if (fclose(stderr) != 0)
        console_print(RED "fclose: 0x%08X\n" RESET, errno);

log_fail:
#endif
    return -1;

#endif
}

/*! deinitialize ftp subsystem */
void ftp_exit(void) {
#ifdef _3DS
    Result ret;
#endif

    /* clean up all sessions */
    while (sessions != NULL)
        ftp_session_destroy(sessions);

    /* stop listening for new clients */
    if (listenfd >= 0)
        ftp_closesocket(listenfd, false);

#ifdef _3DS
    /* deinitialize SOC service */
    console_render();
    console_print(CYAN "Waiting for socExit()...\n" RESET);
    ret = socExit();
    if (ret != 0)
        console_print(RED "socExit: 0x%08X\n" RESET, (unsigned int)ret);
    free(SOCU_buffer);

#ifdef ENABLE_LOGGING
    /* close log file */
    if (fclose(stderr) != 0)
        console_print(RED "fclose: 0x%08X\n" RESET, errno);

#endif

#endif
}

/*! ftp loop
 *
 *  @returns whether to keep looping
 */
loop_status_t ftp_loop(void) {
    int rc;
    struct pollfd pollinfo;
    ftp_session_t *session;

    /* we will poll for new client connections */
    pollinfo.fd = listenfd;
    pollinfo.events = POLLIN;
    pollinfo.revents = 0;

    /* poll for a new client */
#ifdef _3DS
    rc = poll(&pollinfo, 1, 0);
#else
    // TODO - Make the poll delay an exposed ftpde.conf property.
    rc = poll(&pollinfo, 1, 100); // Timeout should *NOT* be 0. EVER. 1/10s is a safe poll, and fixes the 100% CPU issue.
#endif
    if (rc < 0) {
        /* wifi got disabled */
        if (errno == ENETDOWN)
            return LOOP_RESTART;

        console_print(RED "poll: %d %s\n" RESET, errno, strerror(errno));
        return LOOP_EXIT;
    } else if (rc > 0) {
        if (pollinfo.revents & POLLIN) {
            /* we got a new client */
            ftp_session_new(listenfd);
        } else {
            console_print(YELLOW "listenfd: revents=0x%08X\n" RESET,
                          pollinfo.revents);
        }
    }

    /* poll each session */
    session = sessions;
    while (session != NULL)
        session = ftp_session_poll(session);

#ifdef _3DS
    /* check if the user wants to exit */
    hidScanInput();
    if (hidKeysDown() & KEY_B)
        return LOOP_EXIT;
#endif

    return LOOP_CONTINUE;
}

/*! change to parent directory
 *
 *  @param[in] session ftp session
 */
void cd_up(ftp_session_t *session) {
    char *slash = NULL, *p;

    /* remove basename from cwd */
    for (p = session->cwd; *p; ++p) {
        if (*p == '/')
            slash = p;
    }

    *slash = 0;
    if (strncmp(session->cwd, session->root_dir, strlen(session->root_dir)))
        strcpy(session->cwd, session->root_dir);
}

/*! validate a path
 *
 *  @param[in] args path to validate
 */
int validate_path(const char *args) {
    const char *p;

    /* make sure no path components are '..' */
    p = args;
    while ((p = strstr(p, "/..")) != NULL) {
        if (p[3] == 0 || p[3] == '/')
            return -1;
    }

    /* make sure there are no '//' */
    if (strstr(args, "//") != NULL)
        return -1;

    return 0;
}

/*! get a path relative to cwd
 *
 *  @param[in] session ftp session
 *  @param[in] cwd     working directory
 *  @param[in] args    path to make
 *
 *  @returns error
 *
 *  @note the output goes to session->buffer
 */
int build_path(ftp_session_t *session, const char *cwd,
                      const char *args) {
    int rc;
    char *p;

    memset(session->buffer, 0, sizeof(session->buffer));

    /* make sure the input is a valid path */
    if (validate_path(args) != 0) {
        errno = EINVAL;
        return -1;
    }

    if (args[0] == '/') {
        /* this is an absolute path */
        if (strlen(args) > sizeof(session->buffer) - 1) {
            errno = ENAMETOOLONG;
            return -1;
        }
        strncpy(session->buffer, args, sizeof(session->buffer));
    } else {
        /* this is a relative path */
        if (!strcmp(cwd, "/"))
            rc =
                snprintf(session->buffer, sizeof(session->buffer), "/%s", args);
        else
            rc = snprintf(session->buffer, sizeof(session->buffer), "%s/%s",
                          cwd, args);

        if (rc >= sizeof(session->buffer)) {
            errno = ENAMETOOLONG;
            return -1;
        }
    }

    /* remove trailing / */
    p = session->buffer + strlen(session->buffer);
    while (p > session->buffer && *--p == '/')
        *p = 0;

    /* if we ended with a path not containing the root dir...jail it. */
    if (strncmp(session->buffer, session->root_dir, strlen(session->root_dir)))
        strcpy(session->buffer, session->root_dir);

    return 0;
}

/*! transfer a directory listing
 *
 *  @param[in] session ftp session
 *
 *  @returns whether to call again
 */
loop_status_t list_transfer(ftp_session_t *session) {
    ssize_t rc;
    size_t len;
    uint64_t mtime;
    time_t t_mtime;
    struct tm *tm;
    char *buffer;
    struct stat st;
    struct dirent *dent;

    /* check if we sent all available data */
    if (session->bufferpos == session->buffersize) {
        /* check if this was a STAT */
        if (session->data_fd == session->cmd_fd)
            rc = 213;
        else
            rc = 226;

        /* check if this was for a file */
        if (session->dp == NULL) {
            /* we already sent the file's listing */
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            ftp_send_response(session, rc, "OK\r\n");
            return LOOP_EXIT;
        }

        /* get the next directory entry */
        dent = readdir(session->dp);
        if (dent == NULL) {
            /* we have exhausted the directory listing */
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            ftp_send_response(session, rc, "OK\r\n");
            return LOOP_EXIT;
        }

        /* TODO I think we are supposed to return entries for . and .. */
//        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
//            return LOOP_CONTINUE;

        /* check if this was a NLST */
        if (session->flags & SESSION_NLST) {
            /* NLST gives the whole path name */
            session->buffersize = 0;
            if (build_path(session, session->lwd, dent->d_name) == 0) {
                /* encode \n in path */
                len = strlen(session->buffer);
                buffer = encode_path(session->buffer, &len, false);
                if (buffer != NULL) {
                    /* copy to the session buffer to send */
                    memcpy(session->buffer, buffer, len);
                    free(buffer);
                    session->buffer[len++] = '\r';
                    session->buffer[len++] = '\n';
                    session->buffersize = len;
                }
            }
        } else {
#ifdef _3DS
            /* the sdmc directory entry already has the type and size, so no
             * need to do a slow stat */
            sdmc_dir_t *dir = (sdmc_dir_t *)session->dp->dirData->dirStruct;

            if (dir->entry_data.attributes & FS_ATTRIBUTE_DIRECTORY)
                st.st_mode = S_IFDIR;
            else
                st.st_mode = S_IFREG;

            st.st_size = dir->entry_data.fileSize;

            if ((rc = build_path(session, session->lwd, dent->d_name)) != 0)
                console_print(RED "build_path: %d %s\n" RESET, errno,
                              strerror(errno));
            else if ((rc = sdmc_getmtime(session->buffer, &mtime)) != 0) {
                console_print(RED "sdmc_getmtime '%s': 0x%x\n" RESET,
                              session->buffer, rc);
                mtime = 0;
            }
#else
            /* lstat the entry */
            if ((rc = build_path(session, session->lwd, dent->d_name)) != 0)
                console_print(RED "build_path: %d %s\n" RESET, errno,
                              strerror(errno));
            else if ((rc = lstat(session->buffer, &st)) != 0)
                console_print(RED "stat '%s': %d %s\n" RESET, session->buffer,
                              errno, strerror(errno));

            if (rc != 0) {
                /* an error occurred */
                ftp_session_set_state(session, COMMAND_STATE,
                                      CLOSE_PASV | CLOSE_DATA);
                ftp_send_response(session, 550, "unavailable\r\n");
                return LOOP_EXIT;
            }

            mtime = st.st_mtime;
#endif
            /* encode \n in path */
            len = strlen(dent->d_name);
            buffer = encode_path(dent->d_name, &len, false);
            if (buffer != NULL) {
                /* copy to the session buffer to send */
                session->buffersize = sprintf(
                    session->buffer, "%crwxrwxrwx 1 ftp ftp %lld ",
                    S_ISREG(st.st_mode) ? '-' :
                        S_ISDIR(st.st_mode) ? 'd' :
                        S_ISLNK(st.st_mode) ? 'l' :
                        S_ISCHR(st.st_mode) ? 'c' :
                        S_ISBLK(st.st_mode) ? 'b' :
                        S_ISFIFO(st.st_mode) ? 'p' :
                        S_ISSOCK(st.st_mode) ? 's' : '?',
                    (signed long long)st.st_size);
                t_mtime = mtime;
                tm = gmtime(&t_mtime);
                if (tm != NULL) {
                    const char *fmt = "%b %e %Y ";
                    if (session->timestamp > mtime &&
                        session->timestamp - mtime < (60 * 60 * 24 * 365 / 2))
                        fmt = "%b %e %H:%M ";
                    session->buffersize += strftime(
                        session->buffer + session->buffersize,
                        sizeof(session->buffer) - session->buffersize, fmt, tm);
                } else {
                    session->buffersize += sprintf(session->buffer + session->buffersize,
                                                   "Jan 1 1970 "); // AKA Unix time 0.
                }

                if (session->buffersize + len + 2 > sizeof(session->buffer)) {
                    /* buffer will overflow */
                    free(buffer);
                    ftp_session_set_state(session, COMMAND_STATE,
                                          CLOSE_PASV | CLOSE_DATA);
                    ftp_send_response(session, 425, "%s\r\n",
                                      strerror(EOVERFLOW));
                    return LOOP_EXIT;
                }
                memcpy(session->buffer + session->buffersize, buffer, len);
                free(buffer);
                len = session->buffersize + len;
                session->buffer[len++] = '\r';
                session->buffer[len++] = '\n';
                session->buffersize = len;
            } else
                session->buffersize = 0;
        }
        session->bufferpos = 0;
    }

    /* send any pending data */
    rc = send(session->data_fd, session->buffer + session->bufferpos,
              session->buffersize - session->bufferpos, 0);
    if (rc <= 0) {
        /* error sending data */
        if (rc < 0) {
            if (errno == EWOULDBLOCK)
                return LOOP_EXIT;
            console_print(RED "send: %d %s\n" RESET, errno, strerror(errno));
        } else
            console_print(YELLOW "send: %d %s\n" RESET, ECONNRESET,
                          strerror(ECONNRESET));

        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        ftp_send_response(session, 426,
                          "Connection broken during transfer\r\n");
        return LOOP_EXIT;
    }

    /* we can try to send more data */
    session->bufferpos += rc;
    return LOOP_CONTINUE;
}

/*! send a file to the client
 *
 *  @param[in] session ftp session
 *
 *  @returns whether to call again
 */
loop_status_t retrieve_transfer(ftp_session_t *session) {
    ssize_t rc;

    if (session->bufferpos == session->buffersize) {
        /* we have sent all the data so read some more */
        rc = ftp_session_read_file(session);
        if (rc <= 0) {
            /* can't read any more data */
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            if (rc < 0)
                ftp_send_response(session, 451, "Failed to read file\r\n");
            else
                ftp_send_response(session, 226, "OK\r\n");
            return LOOP_EXIT;
        }

        /* we read some data so reset the session buffer to send */
        session->bufferpos = 0;
        session->buffersize = rc;
    }

    /* send any pending data */
    rc = send(session->data_fd, session->buffer + session->bufferpos,
              session->buffersize - session->bufferpos, 0);
    if (rc <= 0) {
        /* error sending data */
        if (rc < 0) {
            if (errno == EWOULDBLOCK)
                return LOOP_EXIT;
            console_print(RED "send: %d %s\n" RESET, errno, strerror(errno));
        } else
            console_print(YELLOW "send: %d %s\n" RESET, ECONNRESET,
                          strerror(ECONNRESET));

        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        ftp_send_response(session, 426,
                          "Connection broken during transfer\r\n");
        return LOOP_EXIT;
    }

    /* we can try to send more data */
    session->bufferpos += rc;
    return LOOP_CONTINUE;
}

/*! send a file to the client
 *
 *  @param[in] session ftp session
 *
 *  @returns whether to call again
 */
loop_status_t store_transfer(ftp_session_t *session) {
    ssize_t rc;

    if (session->bufferpos == session->buffersize) {
        /* we have written all the received data, so try to get some more */
        rc =
            recv(session->data_fd, session->buffer, sizeof(session->buffer), 0);
        if (rc <= 0) {
            /* can't read any more data */
            if (rc < 0) {
                if (errno == EWOULDBLOCK)
                    return LOOP_EXIT;
                console_print(RED "recv: %d %s\n" RESET, errno,
                              strerror(errno));
            }

            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);

            if (rc == 0)
                ftp_send_response(session, 226, "OK\r\n");
            else
                ftp_send_response(session, 426,
                                  "Connection broken during transfer\r\n");
            return LOOP_EXIT;
        }

        /* we received some data so reset the session buffer to write */
        session->bufferpos = 0;
        session->buffersize = rc;
    }

    rc = ftp_session_write_file(session);
    if (rc <= 0) {
        /* error writing data */
        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        ftp_send_response(session, 451, "Failed to write file\r\n");
        return LOOP_EXIT;
    }

    /* we can try to receive more data */
    session->bufferpos += rc;
    return LOOP_CONTINUE;
}

/*! Transfer a file
 *
 *  @param[in] session ftp session
 *  @param[in] args    ftp arguments
 *  @param[in] mode    transfer mode
 *
 *  @returns failure
 */
int ftp_xfer_file(ftp_session_t *session, const char *args,
                         xfer_file_mode_t mode) {
    int rc;

    // Only RETR is allowed without write privs.
    if (((session->auth_level & AUTH_WRITE) != AUTH_WRITE || ftp_readonly_mode == 1) &&
        mode != XFER_FILE_RETR) {
        // Invalid. Lacking proper auth.
        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        return ftp_send_response(session, 530, "Not permitted.\r\n");
    }

    /* build the path of the file to transfer */
    if (build_path(session, session->cwd, args) != 0) {
        rc = errno;
        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        return ftp_send_response(session, 553, "%s\r\n", strerror(rc));
    }

    /* open the file for retrieving or storing */
    if (mode == XFER_FILE_RETR)
        rc = ftp_session_open_file_read(session);
    else
        rc = ftp_session_open_file_write(session, mode == XFER_FILE_APPE);

    if (rc != 0) {
        /* error opening the file */
        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        return ftp_send_response(session, 450, "failed to open file\r\n");
    }

    if (session->flags & SESSION_PORT) {
        /* connect to the client */
        ftp_session_set_state(session, DATA_TRANSFER_STATE, CLOSE_PASV);
        rc = ftp_session_connect(session);
        if (rc != 0) {
            /* error connecting */
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            return ftp_send_response(session, 425,
                                     "can't open data connection\r\n");
        }

        /* set up the transfer */
        session->flags &= ~(SESSION_RECV | SESSION_SEND);
        if (mode == XFER_FILE_RETR) {
            session->flags |= SESSION_SEND;
            session->transfer = retrieve_transfer;
        } else {
            session->flags |= SESSION_RECV;
            session->transfer = store_transfer;
        }

        session->bufferpos = 0;
        session->buffersize = 0;

        return ftp_send_response(session, 150, "Ready\r\n");
    } else if (session->flags & SESSION_PASV) {
        /* set up the transfer */
        session->flags &= ~(SESSION_RECV | SESSION_SEND);
        if (mode == XFER_FILE_RETR) {
            session->flags |= SESSION_SEND;
            session->transfer = retrieve_transfer;
        } else {
            session->flags |= SESSION_RECV;
            session->transfer = store_transfer;
        }

        session->bufferpos = 0;
        session->buffersize = 0;

        ftp_session_set_state(session, DATA_CONNECT_STATE, CLOSE_DATA);
        return 0;
    }

    ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
    return ftp_send_response(session, 503, "Bad sequence of commands\r\n");
}

/*! Transfer a directory
 *
 *  @param[in] session    ftp session
 *  @param[in] args       ftp arguments
 *  @param[in] mode       transfer mode
 *  @param[in] workaround whether to workaround LIST -a
 *
 *  @returns failure
 */
int ftp_xfer_dir(ftp_session_t *session, const char *args,
                        xfer_dir_mode_t mode, bool workaround) {
    ssize_t rc;
    size_t len;
    struct stat st;
    const char *base;
    char *buffer;

    /* set up the transfer */
    session->flags &= ~(SESSION_RECV | SESSION_NLST);
    session->flags |= SESSION_SEND;

    session->transfer = list_transfer;
    session->buffersize = 0;
    session->bufferpos = 0;

    if (strlen(args) > 0) {
        /* an argument was provided */
        if (build_path(session, session->cwd, args) != 0) {
            /* error building path */
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            return ftp_send_response(session, 550, "%s\r\n", strerror(errno));
        }

        /* check if this is a directory */
        session->dp = opendir(session->buffer);
        if (session->dp == NULL) {
            /* not a directory; check if it is a file */
            rc = stat(session->buffer, &st);
            if (rc != 0) {
                /* error getting stat */
                rc = errno;

                /* work around broken clients that think LIST -a is a thing */
                if (workaround && mode == XFER_DIR_LIST) {
                    if (args[0] == '-' && args[1] == 'a') {
                        if (args[2] == 0)
                            buffer = strdup(args + 2);
                        else
                            buffer = strdup(args + 3);

                        if (buffer != NULL) {
                            rc = ftp_xfer_dir(session, buffer, mode, false);
                            free(buffer);
                            return rc;
                        }

                        rc = ENOMEM;
                    }
                }

                ftp_session_set_state(session, COMMAND_STATE,
                                      CLOSE_PASV | CLOSE_DATA);
                return ftp_send_response(session, 550, "%s\r\n", strerror(rc));
            } else {
                /* get the base name */
                base = strrchr(session->buffer, '/') + 1;

                /* encode \n in path */
                len = strlen(base);
                buffer = encode_path(base, &len, false);
                if (buffer != NULL) {
                    /* copy to the session buffer to send */
                    session->buffersize =
                        sprintf(session->buffer,
                                "-rwxrwxrwx 1 ftp ftp %lld Jan 1 1970 ",
                                (signed long long)st.st_size);
                    if (session->buffersize + len + 2 >
                        sizeof(session->buffer)) {
                        /* buffer will overflow */
                        free(buffer);
                        ftp_session_set_state(session, COMMAND_STATE,
                                              CLOSE_PASV | CLOSE_DATA);
                        ftp_send_response(session, 425, "%s\r\n",
                                          strerror(EOVERFLOW));
                        return LOOP_EXIT;
                    }
                    memcpy(session->buffer + session->buffersize, buffer, len);
                    free(buffer);
                    len = session->buffersize + len;
                    session->buffer[len++] = '\r';
                    session->buffer[len++] = '\n';
                    session->buffersize = len;
                } else
                    session->buffersize = 0;
            }
        } else {
            /* it was a directory, so set it as the lwd */
            strcpy(session->lwd, session->buffer);
        }
    } else if (ftp_session_open_cwd(session) != 0) {
        /* no argument, but opening cwd failed */
        ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
        return ftp_send_response(session, 550, "%s\r\n", strerror(errno));
    } else {
        /* set the lwd as the cwd */
        strcpy(session->lwd, session->cwd);
    }

    if (session->flags & SESSION_PORT) {
        /* connect to the client */
        ftp_session_set_state(session, DATA_TRANSFER_STATE, CLOSE_PASV);
        rc = ftp_session_connect(session);
        if (rc != 0) {
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            return ftp_send_response(session, 425,
                                     "can't open data connection\r\n");
        }

        /* set up the transfer */
        if (mode == XFER_DIR_NLST)
            session->flags |= SESSION_NLST;
        else if (mode != XFER_DIR_LIST) {
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            return ftp_send_response(session, 503,
                                     "Bad sequence of commands\r\n");
        }

        return ftp_send_response(session, 150, "Ready\r\n");
    } else if (session->flags & SESSION_PASV) {
        /* set up the transfer */
        if (mode == XFER_DIR_NLST)
            session->flags |= SESSION_NLST;
        else if (mode != XFER_DIR_LIST) {
            ftp_session_set_state(session, COMMAND_STATE,
                                  CLOSE_PASV | CLOSE_DATA);
            return ftp_send_response(session, 503,
                                     "Bad sequence of commands\r\n");
        }

        ftp_session_set_state(session, DATA_CONNECT_STATE, CLOSE_DATA);
        return 0;
    } else if (mode == XFER_DIR_STAT) {
        /* this is a little different; we have to send the data over the command
         * socket */
        ftp_session_set_state(session, DATA_TRANSFER_STATE,
                              CLOSE_PASV | CLOSE_DATA);
        session->data_fd = session->cmd_fd;
        session->flags |= SESSION_SEND;
        return ftp_send_response(session, -213, "Status\r\n");
    }

    /* we must have got LIST or NLST without a preceding PORT or PASV */
    ftp_session_set_state(session, COMMAND_STATE, CLOSE_PASV | CLOSE_DATA);
    return ftp_send_response(session, 503, "Bad sequence of commands\r\n");
}
