/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include "misc.h"
#include "err.h"

/* Subtract the `struct timeval' values X and Y, storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0. */
int difftimeval(struct timeval *result, struct timeval *x, struct timeval *y)
{
	int usec;

	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec)
	{
		usec = (y->tv_usec - x->tv_usec) / USECS_PER_SEC + 1;
		y->tv_usec -= USECS_PER_SEC * usec;
		y->tv_sec += usec;
	}

	if (x->tv_usec - y->tv_usec > USECS_PER_SEC)
	{
		usec = (x->tv_usec - y->tv_usec) / USECS_PER_SEC;
		y->tv_usec += USECS_PER_SEC * usec;
		y->tv_sec -= usec;
	}

	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/* Subtract the `struct timespec' values X and Y, storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0. */
int difftimespec(struct timespec *r, struct timespec *x, struct timespec *y)
{
	int nsec;

	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec)
	{
		nsec = (y->tv_nsec - x->tv_nsec) / NSECS_PER_SEC + 1;
		y->tv_nsec -= NSECS_PER_SEC * nsec;
		y->tv_sec += nsec;
	}

	if (x->tv_nsec - y->tv_nsec > NSECS_PER_SEC)
	{
		nsec = (x->tv_nsec - y->tv_nsec) / NSECS_PER_SEC;
		y->tv_nsec += NSECS_PER_SEC * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * tv_nsec is certainly positive. */
	r->tv_sec = x->tv_sec - y->tv_sec;
	r->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if r is negative. */
	return x->tv_sec < y->tv_sec;
}

/* Subtract the `struct timespec' values X and Y by computing X - Y. If the
 * difference is negative or zero, return 0. Otherwise, return 1 and store the
 * difference in DIFF. X and Y must have valid ts_nsec values, in the range 0 to
 * 999999999. If the difference would overflow, store the maximum possible
 * difference.
 *
 * from xnanosleep.c (coreutils-5.0.1)
 */
int subtimespec(struct timespec *diff, struct timespec *x, struct timespec *y)
{
	time_t sec = x->tv_sec - y->tv_sec;
	long nsec = x->tv_nsec - y->tv_nsec;

	if (x->tv_sec < y->tv_sec)
		return 0;

	if (sec < 0)
	{
		/* The difference has overflowed.  */
		sec = TIME_T_MAX;
		nsec = NSECS_PER_SEC - 1;
	}
	else if (sec == 0 && nsec <= 0)
		return 0;

	if (nsec < 0)
	{
		sec--;
		nsec += NSECS_PER_SEC;
	}

	diff->tv_sec = sec;
	diff->tv_nsec = nsec;

	return 1;
}

inline suseconds_t timeval2usec(struct timeval *t)
{
	return t->tv_usec + t->tv_sec * USECS_PER_SEC;
}

inline long long timespec2nsec(struct timespec *t)
{
	return t->tv_nsec + t->tv_sec * NSECS_PER_SEC;
}

int strnsplit(const char *buffer, const ssize_t length, char **pieces,
		ssize_t *pieceslen, char delim, int n)
{
	int i;
	char *buf, *bufnxt;
	ssize_t buflen;

	buf = bufnxt = (char *) buffer;
	buflen = length;
	i = 0;

	while (i < n
			&& (bufnxt = memchr(bufnxt, delim, buffer + length - bufnxt))
					!= NULL )
	{
		buflen = bufnxt - buf;
		pieces[i] = buf;
		pieceslen[i] = buflen;
		buf = ++bufnxt;
		i++;
	}

	if (i < n)
	{
		if (bufnxt == NULL )
			buflen = buffer + length - buf;

		pieces[i] = buf;
		pieceslen[i] = buflen;
		i++;
	}

	while (bufnxt
			&& (bufnxt = memchr(bufnxt, delim, buffer + length - bufnxt))
					!= NULL )
	{
		buflen = bufnxt - buf;
		buf = ++bufnxt;
		i++;
	}

	return i;
}

int strnsplittrim(const char *buffer, const ssize_t length, char **pieces,
		ssize_t *pieceslen, char delim, int n)
{
	int i;
	char *buf, *bufnxt;
	ssize_t buflen;

	buf = (char *) buffer;
	buflen = length;

	while (buflen > 0 && isspace (buf[0]))
	{
		buf++;
		buflen--;
	}

	bufnxt = buf;
	i = 0;

	while (i < n
			&& (bufnxt = memchr(bufnxt, delim, buffer + length - bufnxt))
					!= NULL )
	{
		buflen = bufnxt - buf;
		pieces[i] = buf;
		pieceslen[i] = buflen;
		buf = ++bufnxt;

		while (buflen > 0 && isspace (buf[0]))
		{
			buf++;
			buflen--;
		}

		bufnxt = buf;
		i++;
	}

	if (i < n)
	{
		if (bufnxt == NULL )
			buflen = buffer + length - buf;

		pieces[i] = buf;
		pieceslen[i] = buflen;
		i++;
	}

	return i;
}

int atoilen(char *buf, ssize_t len)
{
	int c;
	long x = 0;

	if (len > 0)
	{
		c = buf[len];
		buf[len] = '\0';
		x = atoi(buf);
		buf[len] = c;
	}

	return x;
}

long atollen(char *buf, ssize_t len)
{
	int c;
	long x = 0;

	if (len > 0)
	{
		c = buf[len];
		buf[len] = '\0';
		x = atol(buf);
		buf[len] = c;
	}

	return x;
}

double atoflen(char *buf, ssize_t len)
{
	int c;
	double x = 0.0;

	if (len > 0)
	{
		c = buf[len];
		buf[len] = '\0';
		x = atof(buf);
		buf[len] = c;
	}

	return x;
}

unsigned long long strtoulllen(char *buf, ssize_t len)
{
	int c;
	unsigned long long x = 0;

	if (len > 0)
	{
		c = buf[len];
		buf[len] = '\0';
		x = strtoull(buf, NULL, 10);
		buf[len] = c;
	}

	return x;
}

int pointer_cmp(const void *a, const void *b)
{
	return !(a == b);
}

int string_cmp(const void *ain, const void *bin)
{
#define a ((char *) ain)
#define b ((char *) bin)
	return strcmp(a, b );
#undef a
#undef b
}

int integer_cmp(const void *ain, const void *bin)
{
#define a ((int *) ain)
#define b ((int *) bin)
	return *a - *b ;
#undef a
#undef b
}

int strtotimespec(struct timespec *t, char *s)
{
	char *p = NULL;
	double x, y, z;

	t->tv_sec = 0;
	t->tv_nsec = 0;

	if (s != NULL )
	{
		errno = 0;
		x = strtod(s, &p);

		if (p != NULL && *p != '\0')
		{
			errno = EINVAL;
			return -1;
		}

		if ((errno == ERANGE && (x == HUGE_VAL || x == -HUGE_VAL))
		|| (errno != 0 && x == 0))return -1;

		if (s == p)
		{
			errno = EINVAL;
			return -1;
		}

		z = modf(x, &y);

		t->tv_sec = (time_t) y;
		t->tv_nsec = (long) (NSECS_PER_SEC * z);
	}

	return 0;
}

int strtotimeval(struct timeval *t, char *s)
{
	char *p;
	double x, y, z;

	t->tv_sec = 0;
	t->tv_usec = 0;

	if (s != NULL )
	{
		errno = 0;
		x = strtod(s, &p);

		if (p != NULL && *p != '\0')
		{
			errno = EINVAL;
			return -1;
		}

		if ((errno == ERANGE && (x == HUGE_VAL || x == -HUGE_VAL))
		|| (errno != 0 && x == 0))return -1;

		if (s == p)
		{
			errno = EINVAL;
			return -1;
		}

		z = modf(x, &y);

		t->tv_sec = (time_t) y;
		t->tv_usec = (long) (USECS_PER_SEC * z);
	}

	return 0;
}

int msprintf(char **buffer, ssize_t *size_user, const char *fmt, ...)
{
	int n;
	ssize_t *size, sizex;
	va_list ap;

	/* Function can be called with SIZE as NULL when the caller doens't care
	 * about how many bytes are allocated. */
	if (size_user != NULL )
		size = size_user;
	else
	{
		sizex = 256;
		size = &sizex;
	}

	/* Guess we need no more than 128 bytes. */
	if (*buffer == NULL )
	{
		*size = 128;
		*buffer = err_malloc (*size);
	}
	else if (*size == 0)
	{
		*size = 128;
		*buffer = err_realloc (*buffer, *size);
	}

	while (1)
	{
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(*buffer, *size, fmt, ap);
		va_end(ap);

		/* If that worked, return the string. */
		if (n > -1 && n < *size)
			return n;

		/* Else try again with more space. */
		if (n > -1) /* glibc 2.1 */
			*size = n + 1; /* precisely what is needed */
		else
			/* glibc 2.0 */
			*size *= 2; /* twice the old size */

		*buffer = err_realloc (*buffer, *size);
	}
}

int mvsprintf(char **buffer, ssize_t *size_user, const char *fmt, va_list ap)
{
	int n;
	ssize_t *size, sizex;

	/* Function can be called with SIZE as NULL when the caller doens't care
	 * about how many bytes are allocated. */
	if (size_user != NULL )
		size = size_user;
	else
	{
		sizex = 256;
		size = &sizex;
	}

	/* Guess we need no more than 128 bytes. */
	if (*buffer == NULL )
	{
		*size = 128;
		*buffer = err_malloc (*size);
	}
	else if (*size == 0)
	{
		*size = 128;
		*buffer = err_realloc (*buffer, *size);
	}

	while (1)
	{
		/* Try to print in the allocated space. */
		n = vsnprintf(*buffer, *size, fmt, ap);

		/* If that worked, return the string. */
		if (n > -1 && n < *size)
			return n;

		/* Else try again with more space. */
		if (n > -1) /* glibc 2.1 */
			*size = n + 1; /* precisely what is needed */
		else
			/* glibc 2.0 */
			*size *= 2; /* twice the old size */

		*buffer = err_realloc (*buffer, *size);
	}
}

int parse_time(char *s, struct tm *tm, char **endptr)
{
	int i = 0;
	struct tm copy;
	time_t t;
	char *c;
	char *format[] =
	{ "%H:%M:%S", "%H%M%S", "%H:%M", "%H%M", "%H", 0 };

	memcpy(&copy, tm, sizeof(copy));
	strptime("000000", "%H%M%S", &copy);
	copy.tm_isdst = -1;
	t = mktime(&copy);

	while (format[i] != NULL )
	{
		if ((c = strptime(s, format[i], &copy)) != NULL && *c == '\0')
		{
			if (endptr != NULL )
				*endptr = c;

			memcpy(tm, &copy, sizeof(copy));
			return mktime(&copy) - t;
		}

		i++;
	}

	return -1;
}

int parse_date(char *s, struct tm *tm, char **endptr)
{
	int i = 0;
	struct tm copy;
	char *c;
	char *format[] =
	{ "%y-%m-%d", "%y%m%d", "%Y-%m-%d", "%Y%m%d", 0 };

	memcpy(&copy, tm, sizeof(copy));
	copy.tm_isdst = -1;

	while (format[i] != NULL )
	{
		if ((c = strptime(s, format[i], &copy)) != NULL && *c == '\0')
		{
			if (endptr != NULL )
				*endptr = c;

			memcpy(tm, &copy, sizeof(copy));
			return mktime(&copy);
		}

		i++;
	}

	return -1;
}

int parse_datetime(char *s, struct tm *tm, char **endptr)
{
	int i = 0;
	struct tm copy;
	char *c;
	char *format[] =
	{ "%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H%M%S", "%y-%m-%d %H:%M:%S",
			"%y-%m-%d %H%M%S", "%y%m%d %H:%M:%S", "%y%m%d %H%M%S",
			"%Y%m%d %H:%M:%S", "%Y%m%d %H%M%S", 0 };

	memcpy(&copy, tm, sizeof(copy));
	copy.tm_isdst = -1;

	while (format[i] != NULL )
	{
		if ((c = strptime(s, format[i], &copy)) != NULL && *c == '\0')
		{
			if (endptr != NULL )
				*endptr = c;

			memcpy(tm, &copy, sizeof(copy));
			return mktime(&copy);
		}

		i++;
	}

	return -1;
}

/*
 * tm2sec converts a GMT tm structure into the number of seconds since
 * 1st January 1970 UT.  Note that we ignore tm_wday, tm_yday, and tm_dst.
 *
 * The return value is always a valid time_t value -- (time_t)0 is returned
 * if the input date is outside that capable of being represented by time(),
 * i.e., before Thu, 01 Jan 1970 00:00:00 for all systems and
 * beyond 2038 for 32bit systems.
 *
 * This routine is intended to be very fast, much faster than mktime().
 *
 * From Apache (util_date.c)
 */
time_t tm2sec(const struct tm *t)
{
	int year;
	time_t days;
	static const int dayoffset[12] =
	{ 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275 };

	year = t->tm_year;

	if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138)))
		return BAD_DATE ;

	/* shift new year to 1st March in order to make leap year calc easy */

	if (t->tm_mon < 2)
		year--;

	/* Find number of days since 1st March 1900 (in the Gregorian calendar). */

	days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
	days += dayoffset[t->tm_mon] + t->tm_mday - 1;
	days -= 25508; /* 1 jan 1970 is 25508 days since 1 mar 1900 */

	days = ((days * 24 + t->tm_hour) * 60 + t->tm_min) * 60 + t->tm_sec;

	if (days < 0)
		return BAD_DATE ; /* must have overflowed */
	else
		return days; /* must be a valid time */
}

double roundp(double x, int precision)
{
	int i;
	long base;
	double p;

	switch (precision)
	{
	case 0:
		return round(x);
	case 1:
		return round(x * 10) * 0.1;
	case 2:
		return round(x * 100) * 0.01;
	case 3:
		return round(x * 1000) * 0.001;
	case 4:
		return round(x * 10000) * 0.0001;
	case 5:
		return round(x * 100000) * 0.00001;
	case 6:
		return round(x * 1000000) * 0.000001;
	default:
		base = 1000000;

		for (i = 0; i < precision - 6; i++)
			base *= 10;

		p = 1.0 / base;

		return round(x * base) * p;
	}
}

double truncp(double x, int precision)
{
	int i;
	long base;
	double p;

	switch (precision)
	{
	case 0:
		return trunc(x);
	case 1:
		return trunc(x * 10) * 0.1;
	case 2:
		return trunc(x * 100) * 0.01;
	case 3:
		return trunc(x * 1000) * 0.001;
	case 4:
		return trunc(x * 10000) * 0.0001;
	case 5:
		return trunc(x * 100000) * 0.00001;
	case 6:
		return trunc(x * 1000000) * 0.000001;
	default:
		base = 1000000;

		for (i = 0; i < precision - 6; i++)
			base *= 10;

		p = 1.0 / base;

		return trunc(x * base) * p;
	}
}

char *
remove_trailing_slashes(char *dir)
{
	size_t len;

	if (dir != NULL )
	{
		len = strlen(dir);

		while (len > 0 && dir[len - 1] == '/')
			dir[--len] = '\0';
	}

	return dir;
}

void timestamptostr(char *buf, ssize_t bufsiz, time_t t)
{
	struct tm tm =
	{ 0 };

	if (t != 0)
	{
		localtime_r(&t, &tm);
		strftime(buf, bufsiz, "%H:%M:%S", &tm);
	}
	else
	{
		snprintf(buf, bufsiz, " ");
	}
}

int interger_places_count(unsigned long x)
{
	int i = 0;

	do
	{
		x = x / 10;
		i++;
	} while (x != 0);

	return i;
}

/* Gera um double aleatório entre A e B. */
double d_rand(double a, double b)
{
	return a + (b - a) * rand() / (RAND_MAX + 1.0);
}

bool interval_time(time_t time, char *str_time_ini, time_t *time_ini,
		char *str_time_fin, time_t *time_fin)
{
	struct tm tm;
	int interval = false;

	localtime_r(&time, &tm);
	parse_time(str_time_ini, &tm, NULL );
	*time_ini = mktime(&tm);
	parse_time(str_time_fin, &tm, NULL );
	*time_fin = mktime(&tm);

	if (*time_fin > *time_ini)
	{
		if (*time_ini <= time && time <= *time_fin)
			interval = true;
	}
	else
	{
		if (*time_ini <= time || time <= *time_fin)
			interval = true;
	}

	return interval;
}
