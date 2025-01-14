/* libtac.h
 * 
 * Copyright (C) 2010, Pawel Krawczyk <pawel.krawczyk@hush.com> and
 * Jeroen Nijhof <jeroen@jeroennijhof.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program - see the file COPYING.
 *
 * See `CHANGES' file for revision history.
 */

#ifndef _LIB_TAC_H
#define _LIB_TAC_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __linux__

#include <cdefs.h>

#else
#include "cdefs.h"
#endif

// from gnulib-tool --import
// see lib/Makefile.am and lib/Makefile.gnulib for details of modules imported
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_GETRANDOM
#include <sys/random.h>
#endif
#include "gl_array_list.h"
#include "gl_list.h"
#include "gl_xlist.h"
#include "md5.h"
#include "xalloc.h"

// TACACS+ types and constants
#include "tacplus.h"

#ifndef HAVE_ABORT
#define abort exit(EXIT_FAILURE)
#endif


#if defined(__clang__)
#define __CLANG_PREREQ(maj, min) ((__clang_major__ > (maj)) || (__clang_major__ == (maj) && __clang_minor__ >= (min)))
#else
#define __CLANG_PREREQ(maj, min) (0)
#endif

#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(ma, mi) 0
#endif

#if __GNUC_PREREQ(3, 2) || __CLANG_PREREQ(4, 0)
#define __Unused __attribute__((unused))
#else
#define __Unused /* unused */
#endif

#if defined(DEBUGTAC) && !defined(TACDEBUG)
#ifdef __GNUC__
#define TACDEBUG(level, fmt, ...) syslog(level, fmt, ##__VA_ARGS__)
#else
#define TACDEBUG(level, fmt, ...) syslog(level, fmt, __VA_ARGS__)
#endif
#else
#define TACDEBUG(level, fmt, ...) (void)0
#endif

#ifdef __GNUC__
#define TACSYSLOG(level, fmt, ...) syslog(level, fmt, ##__VA_ARGS__)
#else
#define TACSYSLOG(level, fmt, ...) syslog(level, fmt, __VA_ARGS__)
#endif

#if defined(TACDEBUG_AT_RUNTIME)
#undef TACDEBUG
#undef TACSYSLOG
#ifdef __GNUC__
#define TACDEBUG(level, fmt, ...)              \
    do                                         \
    {                                          \
        if (tac_debug_enable)                  \
            logmsg(level, fmt, ##__VA_ARGS__); \
    } while (0)
#define TACSYSLOG(level, fmt, ...) logmsg(level, fmt, ##__VA_ARGS__)
#else
#define TACDEBUG(level, fmt, ...)            \
    do                                       \
    {                                        \
        if (tac_debug_enable)                \
            logmsg(level, fmt, __VA_ARGS__); \
    } while (0)
#define TACSYSLOG(level, fmt, ...) logmsg(level, fmt, __VA_ARGS__)
#endif
extern void logmsg __P((int, const char *, ...));
#endif

/* u_int32_t support for sun */
#ifdef sun
typedef unsigned int u_int32_t;
#endif

#define TAC_PLUS_ATTRIB_MAX_LEN 255
#define TAC_PLUS_ATTRIB_MAX_CNT 255

struct areply {
    /* g_list pointer - needs to be allocated using:
     *      areply.attr = gl_list_create_empty(GL_ARRAY_LIST, NULL, NULL, NULL, false);
     * and later freed using:
     *      tac_free_attrib(arep.attr);
     */
    gl_list_t attr;
    char *msg;
    signed int status: 8;
    unsigned int flags: 8;
    unsigned int seq_no: 8;
};

#ifndef TAC_PLUS_MAXSERVERS
#define TAC_PLUS_MAXSERVERS 8
#endif

#ifndef TAC_PLUS_MAX_PACKET_SIZE
#define TAC_PLUS_MAX_PACKET_SIZE 128000 /* bytes */
#endif

#ifndef TAC_PLUS_MAX_ARGCOUNT
#define TAC_PLUS_MAX_ARGCOUNT TAC_PLUS_ATTRIB_MAX_CNT /* maximum number of arguments passed in packet */
#endif

#ifndef TAC_PLUS_PORT
#define TAC_PLUS_PORT 49
#endif

#define TAC_PLUS_READ_TIMEOUT 180  /* seconds */
#define TAC_PLUS_WRITE_TIMEOUT 180 /* seconds */

/* Internal status codes
*   all negative, tacplus status codes are >= 0
*/

#define LIBTAC_STATUS_ASSEMBLY_ERR (-1)
#define LIBTAC_STATUS_PROTOCOL_ERR (-2)
#define LIBTAC_STATUS_READ_TIMEOUT (-3)
#define LIBTAC_STATUS_WRITE_TIMEOUT (-4)
#define LIBTAC_STATUS_WRITE_ERR (-5)
#define LIBTAC_STATUS_SHORT_HDR (-6)
#define LIBTAC_STATUS_SHORT_BODY (-7)
#define LIBTAC_STATUS_CONN_TIMEOUT (-8)
#define LIBTAC_STATUS_CONN_ERR (-9)
#define LIBTAC_STATUS_ATTRIB_TOO_LONG (-10)
#define LIBTAC_STATUS_ATTRIB_TOO_MANY (-11)

/*
 * Algorithm

      The Algorithm field is one octet and indicates the authentication
      method to be used.  Up-to-date values are specified in the most
      recent "Assigned Numbers" [2].  One value is required to be
      implemented:

         5       CHAP with MD5 [3]
    https://datatracker.ietf.org/doc/html/rfc1994#section-3
 */
#define TACPLUS_ALGORITHM_CHAP_MD5 5

/* Runtime flags */

/* header.c */
extern u_int32_t session_id;
extern int tac_encryption;
extern const char *tac_secret;
extern char tac_login[64];
extern int tac_priv_lvl;
extern int tac_authen_method;
extern int tac_authen_service;

extern int tac_debug_enable;
extern int tac_readtimeout_enable;

/* connect.c */
extern unsigned long tac_timeout;

extern void tac_set_dscp(uint8_t val);

extern void tac_enable_readtimeout(int enable);

extern int tac_connect(struct addrinfo **, char **, int);

extern int tac_connect_single(const struct addrinfo *, const char *, struct addrinfo *,
                              int);

extern char *tac_ntop(const struct sockaddr *);

extern int tac_authen_send(int, const char *, const char *, const char *, const char *,
                           unsigned char);

extern int tac_authen_read(int, struct areply *);

extern int tac_authen_read_timeout(int, struct areply *, unsigned long);

extern int tac_cont_send_seq(int, const char *, int);

#define tac_cont_send(fd, pass) tac_cont_send_seq((fd), (pass), 3)

extern HDR *_tac_req_header(unsigned char, int);

extern void _tac_obfuscate(unsigned char *buf, const HDR *th);

extern int tac_add_attrib(gl_list_t attr, char *, char *);

extern void tac_free_attrib(gl_list_t attr);

extern char *tac_acct_flag2str(int);

extern int tac_acct_send(int, int, const char *, char *, char *, gl_list_t attr);

extern int tac_acct_read(int, struct areply *);

extern int tac_acct_read_timeout(int, struct areply *, unsigned long);

extern char *xstrncpy(char *dst, const char *src, size_t dst_size);

extern char *_tac_check_header(HDR *, int);

extern int tac_author_send(int, const char *, char *, char *, gl_list_t attr);

extern int tac_author_read(int, struct areply *);

extern int tac_author_read_timeout(int, struct areply *, unsigned long);

extern int tac_add_attrib_pair(gl_list_t attr, char *, char, char *);

extern int tac_add_attrib_truncate(gl_list_t attr, char *name, char *value);

extern int tac_add_attrib_pair_truncate(gl_list_t attr, char *name,
                                        char sep, char *value);

extern int tac_read_wait(int, int, int, time_t *);

extern uint32_t _get_session_id(void);

extern void digest_chap(unsigned char *digest, unsigned char id,
                        const char *pass, size_t pass_len,
                        unsigned char *challenge, size_t challenge_len);

extern char *protocol_err_msg;
extern char *authen_syserr_msg;
extern char *author_ok_msg;
extern char *author_fail_msg;
extern char *author_err_msg;
extern char *author_syserr_msg;
extern char *acct_ok_msg;
extern char *acct_fail_msg;
extern char *acct_err_msg;
extern char *acct_syserr_msg;

#ifdef __cplusplus
}
#endif

#endif
