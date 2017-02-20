/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifndef __ERR_H__
#define __ERR_H__

#ifndef _STDLIB_H
# include <stdlib.h>
#endif

#ifndef _STDIO_H
# include <stdio.h>
#endif

#ifndef _STDBOOL_H
# include <stdbool.h>
#endif

#ifndef _SYS_TYPES_H
# include <sys/types.h>
#endif

#ifndef _SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifndef _SYS_STAT_H
# include <sys/stat.h>
#endif

#ifndef _PTHREAD_H
# include <pthread.h>
#endif

#ifndef _TIME_H
# include <time.h>
#endif

#ifndef _SYS_TIME_H
# include <sys/time.h>
#endif

#ifndef _REGEX_H
# include <regex.h>
#endif

#ifndef __DEBUG_H__
# include "debug.h"
#endif

extern char *err_strdup(__FLF_PROTO__, const char *);
extern char *err_strndup(__FLF_PROTO__, const char *, size_t);
extern void *err_malloc(__FLF_PROTO__, size_t);
extern void *err_calloc(__FLF_PROTO__, size_t, size_t);
extern void *err_realloc(__FLF_PROTO__, void *, size_t);
extern int err_mkdir(__FLF_PROTO__, const char *, mode_t);
extern FILE *err_fopen(__FLF_PROTO__, const char *, const char *);
extern int err_open(__FLF_PROTO__, const char *, int, mode_t);
extern void err_setsockopt(__FLF_PROTO__, int, int, int, const void *,
		socklen_t);
extern void err_fstat(__FLF_PROTO__, int, struct stat *);
extern void err_stat(__FLF_PROTO__, const char *, struct stat *);
extern off_t err_lseek(__FLF_PROTO__, int, off_t, int);
extern ssize_t err_send(__FLF_PROTO__, int, const void *, size_t, int);
extern FILE *err_fdopen(__FLF_PROTO__, int, const char *);
extern int err_select(__FLF_PROTO__, int, fd_set *, fd_set *, fd_set *,
		struct timeval *);
extern int err_accept(__FLF_PROTO__, int, struct sockaddr *, socklen_t *);
extern int err_vsnprintf(__FLF_PROTO__, char *, size_t, const char *, va_list);
extern struct tm *err_localtime_r(__FLF_PROTO__, const time_t *, struct tm *);
extern int err_atoi(__FLF_PROTO__, const char *);
extern double err_atof(__FLF_PROTO__, const char *);
extern ssize_t errx_getdelim(__FLF_PROTO__, char **, size_t *, int, FILE *);
extern ssize_t errx_getline(__FLF_PROTO__, char **, size_t *, FILE *);
extern int err_regcomp(__FLF_PROTO__, regex_t *, const char *, int);

extern int err_pthread_join(__FLF_PROTO__, pthread_t, void **);
extern int err_pthread_cancel(__FLF_PROTO__, pthread_t);
extern int err_pthread_setcancelstate(__FLF_PROTO__, int, int *);
extern int err_pthread_create(__FLF_PROTO__, pthread_t *, pthread_attr_t *,
		void *(*)(void *), void *);
extern int err_pthread_attr_init(__FLF_PROTO__, pthread_attr_t *);
extern int err_pthread_attr_destroy(__FLF_PROTO__, pthread_attr_t *);
extern int err_pthread_attr_setdetachstate(__FLF_PROTO__, pthread_attr_t *,
		int);
extern int err_pthread_mutex_init(__FLF_PROTO__, pthread_mutex_t *,
		const pthread_mutexattr_t *);
extern int err_pthread_mutex_destroy(__FLF_PROTO__, pthread_mutex_t *);
extern int err_pthread_mutex_lock(__FLF_PROTO__, pthread_mutex_t *);
extern int err_pthread_mutex_unlock(__FLF_PROTO__, pthread_mutex_t *);
extern int err_pthread_cond_destroy(__FLF_PROTO__, pthread_cond_t *);

#define err_strdup(args...) err_strdup(__FLF__,args)
#define err_strndup(args...) err_strndup(__FLF__,args)
#define err_malloc(args...) err_malloc(__FLF__,args)
#define err_calloc(args...) err_calloc(__FLF__,args)
#define err_realloc(args...) err_realloc(__FLF__,args)
#define err_mkdir(args...) err_mkdir(__FLF__,args)
#define err_fopen(args...) err_fopen(__FLF__,args)
#define err_open(args...) err_open(__FLF__,args)
#define err_setsockopt(args...) err_setsockopt(__FLF__,args)
#define err_fstat(args...) err_fstat(__FLF__,args)
#define err_stat(args...) err_stat(__FLF__,args)
#define err_lseek(args...) err_lseek(__FLF__,args)
#define err_send(args...) err_send(__FLF__,args)
#define err_fdopen(args...) err_fdopen(__FLF__,args)
#define err_select(args...) err_select(__FLF__,args)
#define err_accept(args...) err_accept(__FLF__,args)
#define err_clock_getres(args...) err_clock_getres(__FLF__,args)
#define err_clock_gettime(args...) err_clock_gettime(__FLF__,args)
#define err_vsnprintf(args...) err_vsnprintf(__FLF__,args)
#define err_localtime_r(args...) err_localtime_r(__FLF__,args)
#define err_atoi(args...) err_atoi(__FLF__,args)
#define err_atof(args...) err_atof(__FLF__,args)
#define errx_getdelim(args...) errx_getdelim(__FLF__,args)
#define errx_getline(args...) errx_getline(__FLF__,args)
#define err_regcomp(args...) err_regcomp(__FLF__,args)
#define err_pthread_join(args...) err_pthread_join(__FLF__,args)
#define err_pthread_cancel(args...) err_pthread_cancel(__FLF__,args)
#define err_pthread_setcancelstate(args...) \
    err_pthread_setcancelstate(__FLF__,args)
#define err_pthread_create(args...) err_pthread_create(__FLF__,args)
#define err_pthread_attr_init(args...) err_pthread_attr_init(__FLF__,args)
#define err_pthread_attr_destroy(args...) \
    err_pthread_attr_destroy(__FLF__,args)
#define err_pthread_attr_setdetachstate(args...) \
    err_pthread_attr_setdetachstate(__FLF__,args)
#define err_pthread_mutex_init(args...) err_pthread_mutex_init(__FLF__,args)
#define err_pthread_mutex_destroy(args...) \
    err_pthread_mutex_destroy(__FLF__,args)
#define err_pthread_mutex_lock(args...) err_pthread_mutex_lock(__FLF__,args)
#define err_pthread_mutex_unlock(args...) err_pthread_mutex_unlock(__FLF__,args)
#define err_pthread_cond_destroy(args...) err_pthread_cond_destroy(__FLF__,args)

#endif /* __ERR_H__ */
