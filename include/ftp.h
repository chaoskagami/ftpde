#pragma once

#include "ftp.h"
#include "sha256.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef _3DS
#include <3ds.h>
#define lstat stat
#else
#include <stdbool.h>
#define BIT(x) (1 << (x))
#endif
#include "console.h"

#define POLL_UNKNOWN (~(POLLIN | POLLPRI | POLLOUT))

#define XFER_BUFFERSIZE 32768
#define SOCK_BUFFERSIZE 32768
#define FILE_BUFFERSIZE 65536
#define CMD_BUFFERSIZE 4096
#define SOCU_ALIGN 0x1000
#define SOCU_BUFFERSIZE 0x100000

typedef struct ftp_session_t ftp_session_t;

#ifdef _3DS
  #define LISTEN_PORT 21
  #define DATA_PORT (LISTEN_PORT + 1)
#else
  #ifndef NO_EXTERN_FTP
    extern int LISTEN_PORT; // Use the FTP port default standard, please.
  #endif
  #define DATA_PORT 0 /* ephemeral port */
#endif

#ifndef NO_EXTERN_FTP
  #ifdef _3DS
    /*! SOC service buffer */
    extern u32 *SOCU_buffer;
  #endif

  /*! server listen address */
  extern struct sockaddr_in serv_addr;

  /*! listen file descriptor */
  extern int listenfd;

  #ifdef _3DS
    /*! current data port */
    extern in_port_t data_port;
  #endif

  /*! list of ftp sessions */
  extern ftp_session_t *sessions;

  /*! socket buffersize */
  extern int sock_buffersize;

  /*! server start time */
  extern time_t start_time;

  extern char* ftp_root_dir;
#endif

#define FTP_DECLARE(x) int x(ftp_session_t *session, const char *args)
FTP_DECLARE(ABOR);
FTP_DECLARE(ALLO); // AUTH_WRITE
FTP_DECLARE(APPE); // AUTH_WRITE
FTP_DECLARE(CDUP); // Chained to root_path
FTP_DECLARE(CWD);  // Chained to root_path
FTP_DECLARE(DELE); // AUTH_WRITE
FTP_DECLARE(FEAT);
FTP_DECLARE(HELP);
FTP_DECLARE(LIST);
FTP_DECLARE(MDTM);
FTP_DECLARE(MKD);  // AUTH_WRITE
FTP_DECLARE(MODE);
FTP_DECLARE(NLST);
FTP_DECLARE(NOOP);
FTP_DECLARE(OPTS);
FTP_DECLARE(PASS);
FTP_DECLARE(PASV);
FTP_DECLARE(PORT);
FTP_DECLARE(PWD);
FTP_DECLARE(QUIT);
FTP_DECLARE(REST);
FTP_DECLARE(RETR);
FTP_DECLARE(RMD);  // AUTH_WRITE
FTP_DECLARE(RNFR); // AUTH_WRITE
FTP_DECLARE(RNTO); // AUTH_WRITE
FTP_DECLARE(STAT);
FTP_DECLARE(STOR); // AUTH_WRITE
FTP_DECLARE(STOU); // AUTH_WRITE
FTP_DECLARE(STRU);
FTP_DECLARE(SYST);
FTP_DECLARE(TYPE);
FTP_DECLARE(USER);
FTP_DECLARE(SITE); // Possibly AUTH_WRITE, depending on args
FTP_DECLARE(SIZE);

/*! session state */
typedef enum {
    COMMAND_STATE,       /*!< waiting for a command */
    DATA_CONNECT_STATE,  /*!< waiting for connection after PASV command */
    DATA_TRANSFER_STATE, /*!< data transfer in progress */
} session_state_t;

/*! ftp_session_set_state flags */
typedef enum {
    CLOSE_PASV = BIT(0), /*!< Close the pasv_fd */
    CLOSE_DATA = BIT(1), /*!< Close the data_fd */
} set_state_flags_t;

/*! ftp_session_t flags */
typedef enum {
    SESSION_BINARY = BIT(0), /*!< data transfers in binary mode */
    SESSION_PASV =
        BIT(1), /*!< have pasv_addr ready for data transfer command */
    SESSION_PORT =
        BIT(2), /*!< have peer_addr ready for data transfer command */
    SESSION_RECV = BIT(3), /*!< data transfer in source mode */
    SESSION_SEND = BIT(4), /*!< data transfer in sink mode */
    SESSION_RENAME =
        BIT(5), /*!< last command was RNFR and buffer contains path */
    SESSION_NLST = BIT(6),   /*!< list command is NLST */
    SESSION_URGENT = BIT(7), /*!< in telnet urgent mode */
} session_flags_t;

typedef enum {
    AUTH_REJECT = 0,      // All commands will be rejected.
    AUTH_READ   = BIT(0), // Only reads will be allowed.
    AUTH_WRITE  = BIT(1), // Uploads are allowed.
} auth_level_t;

/*! ftp command descriptor */
typedef struct ftp_command {
    const char *name;                              /*!< command name */
    int (*handler)(ftp_session_t *, const char *); /*!< command callback */
} ftp_command_t;

/*! Loop status */
typedef enum {
    LOOP_CONTINUE, /*!< Continue looping */
    LOOP_RESTART,  /*!< Reinitialize */
    LOOP_EXIT,     /*!< Terminate looping */
} loop_status_t;

/*! ftp_xfer_file mode */
typedef enum {
    XFER_FILE_RETR, /*!< Retrieve a file */
    XFER_FILE_STOR, /*!< Store a file */
    XFER_FILE_APPE, /*!< Append a file */
} xfer_file_mode_t;

/*! ftp_xfer_dir mode */
typedef enum {
    XFER_DIR_LIST, /*!< Long list */
    XFER_DIR_NLST, /*!< Short list */
    XFER_DIR_STAT, /*!< Stat command */
} xfer_dir_mode_t;

/*! ftp command */
#define FTP_COMMAND(x)                                                         \
    { #x, x, }

/*! ftp alias */
#define FTP_ALIAS(x, y)                                                        \
    { #x, y, }

/*! ftp session */
struct ftp_session_t {
	char root_dir[4096];          /*!< root directory to list */
    char cwd[4096];               /*!< current working directory, relative to root_dir */
    char lwd[4096];               /*!< list working directory */
    struct sockaddr_in peer_addr; /*!< peer address for data connection */
    struct sockaddr_in pasv_addr; /*!< listen address for PASV connection */
    int cmd_fd;                   /*!< socket for command connection */
    int pasv_fd;                  /*!< listen socket for PASV */
    int data_fd;                  /*!< socket for data transfer */
    time_t timestamp;             /*!< time from last command */
    session_flags_t flags;        /*!< session flags */
    session_state_t state;        /*!< session state */
    ftp_session_t *next;          /*!< link to next session */
    ftp_session_t *prev;          /*!< link to prev session */

    loop_status_t (*transfer)(ftp_session_t *); /*! data transfer callback */
    char buffer[XFER_BUFFERSIZE];      /*! persistent data between callbacks */
    char tmp_buffer[XFER_BUFFERSIZE];  /*! persistent data between callbacks */
    char file_buffer[FILE_BUFFERSIZE]; /*! stdio file buffer */
    char cmd_buffer[CMD_BUFFERSIZE];   /*! command buffer */
    size_t bufferpos;  /*! persistent buffer position between callbacks */
    size_t buffersize; /*! persistent buffer size between callbacks */
    size_t cmd_buffersize;
    uint64_t filepos;  /*! persistent file position between callbacks */
    uint64_t filesize; /*! persistent file size between callbacks */
    FILE *fp;          /*! persistent open file pointer between callbacks */
    DIR *dp; /*! persistent open directory pointer between callbacks */

    /*! added in ftpde */
    char username_buf[64];   /*!< Username buffer. Any longer is overkill. */
    auth_level_t auth_level; /*!< Auth */
};

#define REJECT_WRITE_CHK { \
    int REJ_CHK = ftp_check_reject_write(session); \
    if (REJ_CHK != 0) \
        return REJ_CHK; \
}

#define CODE_TO_STATBOOL(x) ((x >= 400 && x < 600) ? (0) : (1))

int ftp_check_reject_write(ftp_session_t* session);

int ftp_command_cmp(const void *p1, const void *p2);

in_port_t next_data_port(void);

int ftp_set_socket_nonblocking(int fd);

int ftp_set_socket_options(int fd);

void ftp_closesocket(int fd, bool connected);

void ftp_session_close_cmd(ftp_session_t *session);

void ftp_session_close_pasv(ftp_session_t *session);

void ftp_session_close_data(ftp_session_t *session);

void ftp_session_close_file(ftp_session_t *session);

int ftp_session_open_file_read(ftp_session_t *session);

ssize_t ftp_session_read_file(ftp_session_t *session);

int ftp_session_open_file_write(ftp_session_t *session, bool append);

ssize_t ftp_session_write_file(ftp_session_t *session);

void ftp_session_close_cwd(ftp_session_t *session);

int ftp_session_open_cwd(ftp_session_t *session);

void ftp_session_set_state(ftp_session_t *session, session_state_t state, set_state_flags_t flags);

void ftp_session_transfer(ftp_session_t *session);

char *encode_path(const char *path, size_t *len, bool quotes);

void decode_path(ftp_session_t *session, size_t len);

ssize_t ftp_send_response_buffer(ftp_session_t *session, const char *buffer, size_t len, int success);

__attribute__((format(printf, 3, 4)))
    ssize_t ftp_send_response(ftp_session_t *session, int code, const char *fmt, ...);

ftp_session_t *ftp_session_destroy(ftp_session_t *session);

void ftp_session_new(int listen_fd);

int ftp_session_accept(ftp_session_t *session);

int ftp_session_connect(ftp_session_t *session);

void ftp_session_read_command(ftp_session_t *session, int events);

ftp_session_t *ftp_session_poll(ftp_session_t *session);

int ftp_init(int port, char* root_d, int readonly);

void ftp_exit(void);

loop_status_t ftp_loop(void);

void cd_up(ftp_session_t *session);

int validate_path(const char *args);

int build_path(ftp_session_t *session, const char *cwd, const char *args);

loop_status_t list_transfer(ftp_session_t *session);

loop_status_t retrieve_transfer(ftp_session_t *session);

loop_status_t store_transfer(ftp_session_t *session);

int ftp_xfer_file(ftp_session_t *session, const char *args, xfer_file_mode_t mode);

int ftp_xfer_dir(ftp_session_t *session, const char *args, xfer_dir_mode_t mode, bool workaround);
