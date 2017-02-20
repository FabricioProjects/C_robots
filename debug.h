/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <libgen.h>

#define __FLF__       __FILE__,__LINE__,__FUNCTION__
#define __FLF_ARGS__  __file__,__line__,__function__
#define __FLF_PROTO__ const char *__file__,int __line__,const char *__function__

#ifdef NDEBUG
# define debug_init(args...)
# define debug_finish()
# define debug_begin()
# define debug_end()
# define dputnc(args...)
# define dprint(args...)
# define dprintn(args...)
# define debug(args...)
# define debugn(args...)
# define err_error(s,e,args...) error(s,e,args)
# define error(s,e,args...)     error(s,e,args)
#else /* não NDEBUG */

bool                debug_mode;
bool                sdebug_mode;
bool                debug_time;
pthread_mutex_t     debug_mutex;
pthread_mutexattr_t debug_mutexattr;
FILE                *debug_file;
FILE                *sdebug_file;
int                 debug_errno;
time_t              debug_t;
struct tm           debug_tm;
char                debug_tbuf[256];
struct timeval      debug_tv;

#ifdef __linux__
# define getprogname() program_invocation_short_name
#endif

# define debug_init(mode,filename)                                     \
    do {                                                               \
        debug_mode = mode;                                             \
        if (debug_mode == true) {                                      \
            if (filename == NULL) {                                    \
                debug_file = stderr;                                   \
            } else if (strcmp (filename, "stderr") == 0) {             \
                debug_file = stderr;                                   \
            } else if ((debug_file = fopen (filename, "w")) == NULL) { \
                fprintf (stderr, "%s: PROGRAM ERROR: %s: %s\n",        \
                         getprogname(), filename,            \
                         strerror (errno));                            \
                exit (EXIT_FAILURE);                                   \
            }                                                          \
            setlinebuf (debug_file);                                   \
            pthread_mutexattr_init (&debug_mutexattr);                 \
            pthread_mutexattr_settype (&debug_mutexattr,               \
                                       PTHREAD_MUTEX_RECURSIVE);    \
            pthread_mutex_init (&debug_mutex, &debug_mutexattr);       \
            pthread_mutexattr_destroy (&debug_mutexattr);              \
            debug_errno = 0;                                           \
            if (getenv ("STOKER_DEBUG_TIME") != NULL)                  \
                debug_time = atoi (getenv ("STOKER_DEBUG_TIME"));      \
            else                                                       \
                debug_time = false;                                    \
        }                                                              \
    } while (0)

# define sdebug_init(mode,filename)                                     \
    do {                                                               \
        sdebug_mode = mode;                                             \
        if (sdebug_mode == true) {                                      \
            if (filename == NULL) {                                    \
                sdebug_file = stderr;                                   \
            } else if (strcmp (filename, "stderr") == 0) {             \
               sdebug_file = stderr;                                   \
            } else if ((sdebug_file = fopen (filename, "w")) == NULL) { \
                fprintf (stderr, "%s: PROGRAM ERROR: %s: %s\n",        \
                         getprogname(), filename,            \
                         strerror (errno));                            \
                exit (EXIT_FAILURE);                                   \
            }                                                          \
            setlinebuf (sdebug_file);                                   \
            pthread_mutexattr_init (&debug_mutexattr);                 \
            pthread_mutexattr_settype (&debug_mutexattr,               \
                                       PTHREAD_MUTEX_RECURSIVE);    \
            pthread_mutex_init (&debug_mutex, &debug_mutexattr);       \
            pthread_mutexattr_destroy (&debug_mutexattr);              \
            debug_errno = 0;                                           \
            if (getenv ("STOKER_DEBUG_TIME") != NULL)                  \
                debug_time = atoi (getenv ("STOKER_DEBUG_TIME"));      \
            else                                                       \
                debug_time = false;                                    \
        }                                                              \
    } while (0)

# define debug_finish()                                 \
    do {                                                \
        if (debug_mode == true && debug_file != NULL) { \
            debug_begin ();                             \
            fclose (debug_file);                        \
            debug_end ();                               \
        }                                               \
        pthread_mutex_destroy (&debug_mutex);           \
    } while (0)

# define debug_begin()                     \
    do {                                   \
        pthread_mutex_lock (&debug_mutex); \
    } while (0)

# define debug_end()                         \
    do {                                     \
        pthread_mutex_unlock (&debug_mutex); \
    } while (0)

# define dputnc(c,n)                                 \
    do {                                             \
        if (debug_mode == true) {                    \
            pthread_mutex_lock (&debug_mutex);       \
            int _i;                                  \
            for (_i = 0; _i < n; _i++)               \
                fprintf (debug_file, "%c", c);       \
            pthread_mutex_unlock (&debug_mutex);     \
        }                                            \
    } while (0)

# define dprint(args...)                             \
    do {                                             \
        if (debug_mode == true) {                    \
            pthread_mutex_lock (&debug_mutex);       \
            fprintf (debug_file, ## args);           \
            fputc ('\n', debug_file);                \
            pthread_mutex_unlock (&debug_mutex);     \
        }                                            \
    } while (0)

# define dprintn(args...)                            \
    do {                                             \
        if (debug_mode == true) {                    \
            pthread_mutex_lock (&debug_mutex);       \
            fprintf (debug_file, ## args);           \
            pthread_mutex_unlock (&debug_mutex);     \
        }                                            \
    } while (0)

# define debug(args...)                                                \
    do {                                                               \
        if (debug_mode == true) {                                      \
            pthread_mutex_lock (&debug_mutex);                         \
            if (debug_time != 0) {                                     \
                gettimeofday (&debug_tv, NULL);                        \
                debug_t = debug_tv.tv_sec;                             \
                localtime_r (&debug_t, &debug_tm);                     \
                strftime (debug_tbuf, sizeof (debug_tbuf), "%H%M%S",   \
                          &debug_tm);                                  \
                fprintf (debug_file, "%s.%06ld %s:%d: %s(): ",         \
                         debug_tbuf, debug_tv.tv_usec, __FLF__);       \
            }                                                          \
            else                                                       \
                fprintf (debug_file, "%s:%d: %s(): ", __FLF__);        \
            fprintf (debug_file, ## args);                             \
            if (debug_errno != 0)                                      \
                fprintf (debug_file, " [%s]", strerror (debug_errno)); \
            fputc ('\n', debug_file);                                  \
            pthread_mutex_unlock (&debug_mutex);                       \
        }                                                              \
    } while (0)

# define sdebug(args...)                                                \
    do {                                                               \
        if (sdebug_mode == true) {                                      \
            pthread_mutex_lock (&debug_mutex);                         \
            if (debug_time != 0) {                                     \
                gettimeofday (&debug_tv, NULL);                        \
                debug_t = debug_tv.tv_sec;                             \
                localtime_r (&debug_t, &debug_tm);                     \
                strftime (debug_tbuf, sizeof (debug_tbuf), "%H%M%S",   \
                          &debug_tm);                                  \
                fprintf (sdebug_file, "%s.%06ld %s:%d: %s(): ",         \
                         debug_tbuf, debug_tv.tv_usec, __FLF__);       \
            }                                                          \
            else                                                       \
                fprintf (sdebug_file, "%s:%d: %s(): ", __FLF__);        \
            fprintf (sdebug_file, ## args);                             \
            if (debug_errno != 0)                                      \
                fprintf (sdebug_file, " [%s]", strerror (debug_errno)); \
            fputc ('\n', sdebug_file);                                  \
            pthread_mutex_unlock (&debug_mutex);                       \
        }                                                              \
    } while (0)

# define debugn(args...)                                               \
    do {                                                               \
        if (debug_mode == true) {                                      \
            pthread_mutex_lock (&debug_mutex);                         \
            if (debug_time != 0) {                                     \
                gettimeofday (&debug_tv, NULL);                        \
                debug_t = debug_tv.tv_sec;                             \
                localtime_r (&debug_t, &debug_tm);                     \
                strftime (debug_tbuf, sizeof (debug_tbuf), "%H%M%S",   \
                          &debug_tm);                                  \
                fprintf (debug_file, "%s.%06ld %s:%d: %s(): ",         \
                         debug_tbuf, debug_tv.tv_usec, __FLF__);       \
            }                                                          \
            else                                                       \
                fprintf (debug_file, "%s:%d: %s(): ", __FLF__);        \
            fprintf (debug_file, ## args);                             \
            if (debug_errno != 0)                                      \
                fprintf (debug_file, " [%s]", strerror (debug_errno)); \
            pthread_mutex_unlock (&debug_mutex);                       \
        }                                                              \
    } while (0)

# define err_error(s,e,args...)                                        \
    do {                                                               \
        debug_errno = e;                                               \
        if (debug_mode == true) {                                      \
            pthread_mutex_lock (&debug_mutex);                         \
            if (debug_time != 0) {                                     \
                gettimeofday (&debug_tv, NULL);                        \
                debug_t = debug_tv.tv_sec;                             \
                localtime_r (&debug_t, &debug_tm);                     \
                strftime (debug_tbuf, sizeof (debug_tbuf), "%H%M%S",   \
                          &debug_tm);                                  \
                fprintf (debug_file, "%s.%06ld %s:%d: %s(): ",         \
                         debug_tbuf, debug_tv.tv_usec, __FLF_ARGS__);  \
            }                                                          \
            else                                                       \
                fprintf (debug_file, "%s:%d: %s(): ", __FLF_ARGS__);   \
            fprintf (debug_file, ## args);                             \
            if (debug_errno != 0)                                      \
                fprintf (debug_file, " [%s]", strerror (debug_errno)); \
            fputc ('\n', debug_file);                                  \
            pthread_mutex_unlock (&debug_mutex);                       \
        }                                                              \
        debug_errno = 0;                                               \
        error(s,e,args);                                               \
    } while (0)

# define error(s,e,args...) \
  do {                      \
      debug_errno = e;      \
      debug(args);          \
      debug_errno = 0;      \
      error(s,e,args);      \
  } while (0)

#endif /* NDEBUG */

#endif /* __DEBUG_H__ */
