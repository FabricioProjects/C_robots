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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include "debug.h"

char *
err_strdup(__FLF_PROTO__, const char *s)
{
	char *new = strdup(s);

	if (new == NULL )
		err_error(EXIT_FAILURE, errno, "strdup(%s)", s);

	return new;
}

char *
err_strndup(__FLF_PROTO__, const char *s, size_t n)
{
	char *new = strndup(s, n);

	if (new == NULL )
		err_error(EXIT_FAILURE, errno, "strndup(%s, %zd)", s, n);

	return new;
}

void *
err_malloc(__FLF_PROTO__, size_t n)
{
	void *p = malloc(n);

	if (p == NULL && n != 0)
		err_error(EXIT_FAILURE, errno, "malloc(%zd)", n);

	return p;
}

void *
err_calloc(__FLF_PROTO__, size_t n, size_t size)
{
	void *p = calloc(n, size);

	if (p == NULL && n != 0)
		err_error(EXIT_FAILURE, errno, "calloc(%zd)", n);

	return p;
}

void *
err_realloc(__FLF_PROTO__, void *p, size_t n)
{
	p = realloc(p, n);

	if (p == NULL && n != 0)
		err_error(EXIT_FAILURE, errno, "realloc(%zd)", n);

	return p;
}

int err_mkdir(__FLF_PROTO__, const char *pathname, mode_t mode)
{
	int retval = mkdir(pathname, mode);

	if (retval < 0)
	{
		if (errno == EEXIST)
			err_error(0, errno, "%s", pathname);
		else
			err_error(EXIT_FAILURE, errno, "%s", pathname);
	}

	return retval;
}

FILE *
err_fopen(__FLF_PROTO__, const char *filename, const char *mode)
{
	FILE *file = fopen(filename, mode);

	if (file == NULL )
		err_error(EXIT_FAILURE, errno, "%s", filename);

	return file;
}

int err_open(__FLF_PROTO__, const char *filename, int flags, mode_t mode)
{
	int fd = open(filename, flags, mode);

	if (fd < 0)
		err_error(EXIT_FAILURE, errno, "%s", filename);

	return fd;
}

void err_setsockopt(__FLF_PROTO__, int s, int level, int optname,
		const void *optval, socklen_t optlen)
{
	if (setsockopt(s, level, optname, optval, optlen) < 0)
		err_error(EXIT_FAILURE, errno, "setsockopt(%d)", s);
}

void err_fstat(__FLF_PROTO__, int fd, struct stat *buf)
{
	if (fstat(fd, buf) < 0)
		err_error(EXIT_FAILURE, errno, "fstat(%d)", fd);
}

void err_stat(__FLF_PROTO__, const char *path, struct stat *buf)
{
	if (stat(path, buf) < 0)
		err_error(EXIT_FAILURE, errno, "stat(%s)", path);
}

off_t err_lseek(__FLF_PROTO__, int fd, off_t offset, int whence)
{
	off_t n = lseek(fd, offset, whence);

	if (n < 0)
		err_error(EXIT_FAILURE, errno, "lseek(%d)", fd);

	return n;
}

ssize_t err_send(__FLF_PROTO__, int s, const void *buf, size_t len, int flags)
{
	ssize_t n = send(s, buf, len, flags);

	if (n < 0)
	{
		if (errno == ECONNRESET)
			err_error(0, errno, "send(): connection reset by peer");
		else
			err_error(EXIT_FAILURE, errno, "send()");
	}

	return n;
}

FILE *
err_fdopen(__FLF_PROTO__, int fd, const char *mode)
{
	FILE *file = fdopen(fd, mode);

	if (file == NULL )
		err_error(EXIT_FAILURE, errno, "fdopen()");

	return file;
}

int err_select(__FLF_PROTO__, int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	int n = select(nfds, readfds, writefds, exceptfds, timeout);

	if (n < 0)
		err_error(EXIT_FAILURE, errno, "select()");

	return n;
}

int err_accept(__FLF_PROTO__, int sockfd, struct sockaddr *addr,
		socklen_t * addrlen)
{
	int n = accept(sockfd, addr, addrlen);

	if (n < 0)
	{
		/* XXX Essa condição não é um erro. EWOULDBLOCK ou EAGAIN
		 * acontecem quando o SOCKFD é não-bloqueante e não existe
		 * nenhuma conexão pendente. Imprimir essa mensagem pode confundir o
		 * usuário. */
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			/* XXX How to solve per-process error?
			 * The per-process limit of open file descriptors has
			 * been reached or the system limit on the total number of
			 * open files has been reached. */
			if (errno == EMFILE || errno == ENFILE)
				err_error(0, errno, "accept(): connection refused");
			else if (errno == ECONNABORTED)
				err_error(0, errno, "accept(): connection aborted");
			else
				err_error(EXIT_FAILURE, errno, "accept()");
		}
	}

	return n;
}

int err_vsnprintf(__FLF_PROTO__, char *str, size_t size, const char *format,
		va_list ap)
{
	int retval = vsnprintf(str, size, format, ap);

	if (retval < 0)
		err_error(EXIT_FAILURE, errno, "vsnprintf()");

	return retval;
}

struct tm *
err_localtime_r(__FLF_PROTO__, const time_t *timep, struct tm *result)
{
	result = localtime_r(timep, result);

	if (result == NULL )
		err_error(EXIT_FAILURE, errno, "localtime_r()");

	return result;
}

int err_atoi(__FLF_PROTO__, const char *str)
{
	char *endptr;
	int val;

	errno = 0;
	val = strtol(str, &endptr, 10);

	if ((errno == ERANGE && (val == INT_MAX || val == INT_MIN))
	|| errno != 0)err_error(EXIT_FAILURE, errno, "strtol(): `%s':", str)
		;

	if (endptr == str)
		err_error(EXIT_FAILURE, EINVAL, "strtol(): `%s':", str);

	return val;
}

double err_atof(__FLF_PROTO__, const char *str)
{
	char *endptr;
	double val;

	errno = 0;
	val = strtod(str, &endptr);

	if ((errno == ERANGE && (val == HUGE_VAL || val == 0)) || errno != 0)
		err_error(EXIT_FAILURE, errno, "strtod(): `%s':", str);

	if (endptr == str)
		err_error(EXIT_FAILURE, EINVAL, "strtod(): `%s':", str);

	return val;
}

ssize_t errx_getdelim(__FLF_PROTO__, char **buffer, size_t *size, int delim,
		FILE *stream)
{
	ssize_t bytes = getdelim(buffer, size, delim, stream);

	if (bytes < 0)
	{
		free(*buffer);
		err_error(0, errno, "getdelim()");
	}

	return bytes;
}

ssize_t errx_getline(__FLF_PROTO__, char **buffer, size_t *size, FILE *stream)
{
	ssize_t bytes = getline(buffer, size, stream);

	if (bytes < 0)
	{
		free(*buffer);
		err_error(0, errno, "getline()");
	}

	return bytes;
}

int err_regcomp(__FLF_PROTO__, regex_t *preg, const char *regex, int cflags)
{
	int retval;
	size_t length;
	char *buffer = NULL;

	retval = regcomp(preg, regex, cflags);

	if (retval < 0)
	{
		length = regerror(retval, preg, NULL, 0);
		buffer = err_malloc(__FLF_ARGS__, length * sizeof(*buffer));
		regerror(retval, preg, buffer, length);
		err_error(EXIT_FAILURE, 0, "%s", buffer);
		free(buffer);
	}

	return retval;
}

int err_pthread_join(__FLF_PROTO__, pthread_t th, void **thread_return)
{
	errno = pthread_join(th, thread_return);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_join()");

	return errno;
}

int err_pthread_cancel(__FLF_PROTO__, pthread_t thread)
{
	errno = pthread_cancel(thread);

	if (errno == ESRCH)
		err_error(0, errno, "pthread_cancel(): no thread could be found");
	else if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_cancel()");

	return errno;
}

int err_pthread_setcancelstate(__FLF_PROTO__, int state, int *oldstate)
{
	errno = pthread_setcancelstate(state, oldstate);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_setcancelstate()");

	return errno;
}

int err_pthread_create(__FLF_PROTO__, pthread_t *thread, pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg)
{
	errno = pthread_create(thread, attr, start_routine, arg);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_create()");

	return errno;
}

int err_pthread_attr_init(__FLF_PROTO__, pthread_attr_t *attr)
{
	errno = pthread_attr_init(attr);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_attr_init()");

	return errno;
}

int err_pthread_attr_destroy(__FLF_PROTO__, pthread_attr_t *attr)
{
	errno = pthread_attr_destroy(attr);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_attr_destroy()");

	return errno;
}

int err_pthread_attr_setdetachstate(__FLF_PROTO__, pthread_attr_t *attr,
		int detachstate)
{
	errno = pthread_attr_setdetachstate(attr, detachstate);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_attr_setdetachstate()");

	return errno;
}

int err_pthread_mutex_init(__FLF_PROTO__, pthread_mutex_t *mutex,
		const pthread_mutexattr_t *mutexattr)
{
	errno = pthread_mutex_init(mutex, mutexattr);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_mutex_init()");

	return errno;
}

int err_pthread_mutex_destroy(__FLF_PROTO__, pthread_mutex_t *mutex)
{
	errno = pthread_mutex_destroy(mutex);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_mutex_destroy()");

	return errno;
}

int err_pthread_mutex_lock(__FLF_PROTO__, pthread_mutex_t *mutex)
{
	errno = pthread_mutex_lock(mutex);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_lock()");

	return errno;
}

int err_pthread_mutex_unlock(__FLF_PROTO__, pthread_mutex_t *mutex)
{
	errno = pthread_mutex_unlock(mutex);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_mutex_unlock()");

	return errno;
}

int err_pthread_cond_destroy(__FLF_PROTO__, pthread_cond_t *cond)
{
	errno = pthread_cond_destroy(cond);

	if (errno != 0)
		err_error(EXIT_FAILURE, errno, "pthread_cond_destroy()");

	return errno;
}

