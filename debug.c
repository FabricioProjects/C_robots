#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifndef __linux__

void error (int status, int errnum, const char *message, ...)
{
	fprintf (stderr, "%s: ", getprogname());
}

#endif