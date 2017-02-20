/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifndef __MISC_H__
#define __MISC_H__

#ifndef _STDLIB_H
# include <stdlib.h>
#endif

#ifndef _STDBOOL_H
# include <stdbool.h>
#endif

#ifndef _TIME_H
# include <time.h>
#endif

#ifndef _SYS_TIME_H
# include <sys/time.h>
#endif

#ifndef _CTYPE_H
# include <ctype.h>
#endif

#ifndef _SYS_SELECT_H
# include <sys/select.h>
#endif

#ifndef _LIMITS_H
# include <limits.h>
#endif

#ifndef _FLOAT_H
# include <float.h>
#endif

#ifndef _MATH_H
# include <math.h>
#endif

#ifndef _STDARG_H
# include <stdarg.h>
#endif

#define FDSET_INSERT(set,new,max) \
    do {                          \
        FD_SET (new, &set);       \
        if (new > max)            \
            max = new;            \
    } while (0)

#define FDSET_REMOVE(set,fd,max)             \
    do {                                     \
        if (fd == max)                       \
            while (FD_ISSET (fd, &set) == 0) \
                (max)--;                     \
        FD_CLR (fd, &set);                   \
    } while (0)

#define TIMEVAL_COPY(src,dst)          \
    do {                               \
        (dst).tv_sec = (src).tv_sec;   \
        (dst).tv_usec = (src).tv_usec; \
    } while (0)

#define TIMESPEC_CMP(a,b)           \
    ((a).tv_sec < (b).tv_sec ? -1 : \
     (a).tv_sec > (b).tv_sec ? 1 :  \
     (a).tv_nsec - (b).tv_nsec)

#define TRIMMING_TRAILING(s,n)                  \
    while ((n) > 0 && isspace ((s)[(n) - 1])) { \
        (s)[--(n)] = '\0';                      \
    }

#define TRIMMING_LEADING(s,n)             \
    while ((n) > 0 && isspace ((s)[0])) { \
        (s)++;                            \
        (n)--;                            \
    }

#define LINE_IS_COMMENT_OR_BLANK(a) \
    ((a)[0] == '#' || (a)[0] == '\n')

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define VARPERC(a,b) ((a) / (double) (b) - 1.0) * 100.0

/* Fonte: http://www.htdig.org */
/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
 It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
                              ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

#ifndef TIME_T_MIN
# define TIME_T_MIN TYPE_MINIMUM (time_t)
#endif
#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
#endif

#define BAD_DATE (time_t)0

#define ROUND2(x) (double)(floor (x * 100 + 0.05) * 0.01)

#define EQ(x,y) (fabs((x)-(y)) < DBL_EPSILON)
#define GT(x,y) ((x)-(y) > DBL_EPSILON)
#define LT(x,y) ((y)-(x) > DBL_EPSILON)
#define GE(x,y) (GT(x,y) || EQ(x,y))
#define LE(x,y) (LT(x,y) || EQ(x,y))

#define PRICE_PRECISION 2

#define USECS_PER_SEC 1000000
#define NSECS_PER_SEC 1000000000

extern int difftimeval(struct timeval *, struct timeval *, struct timeval *);
extern int difftimespec(struct timespec *, struct timespec *,
		struct timespec *);
extern int subtimespec(struct timespec *, struct timespec *, struct timespec *);
extern inline suseconds_t timeval2usec(struct timeval *);
extern inline long long timespec2nsec(struct timespec *);
extern int strnsplit(const char *, const ssize_t, char **, ssize_t *, char,
		int);
extern int strnsplittrim(const char *, const ssize_t, char **, ssize_t *, char,
		int);
extern int atoilen(char *, ssize_t);
extern long atollen(char *, ssize_t);
extern double atoflen(char *, ssize_t);
extern int atoilen(char *, ssize_t);
extern unsigned long long strtoulllen(char *, ssize_t);
extern int pointer_cmp(const void *, const void *);
extern int string_cmp(const void *, const void *);
extern int integer_cmp(const void *, const void *);
extern int strtotimespec(struct timespec *, char *);
extern int strtotimeval(struct timeval *, char *);
extern int msprintf(char **, ssize_t *, const char *, ...);
extern int mvsprintf(char **, ssize_t *, const char *, va_list);
extern int parse_time(char *, struct tm *, char **);
extern int parse_date(char *, struct tm *, char **);
extern int parse_datetime(char *, struct tm *, char **);
extern time_t tm2sec(const struct tm *);
extern double roundp(double, int);
extern double truncp(double, int);
extern char *remove_trailing_slashes(char *);
extern void timestamptostr(char *, ssize_t, time_t);
extern int interger_places_count(unsigned long);
extern double d_rand(double, double);
extern bool interval_time(time_t, char *, time_t *, char *, time_t *);

#endif /* __MISC_H__ */
