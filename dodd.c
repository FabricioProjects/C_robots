#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include "stocks.h"
#include "err.h"
#include "debug.h"
#include "misc.h"
#include "args.h"
#include "ma.h"
#include "candle.h"
#include "estimate.h"
#include "estimate_bender.h"
#include "estimate_quarks.h"

static void dir_create(char *dir, mode_t mode)
{
	struct stat st;

	if (stat(dir, &st) < 0 && errno == ENOENT)
		err_mkdir(dir, mode);
}

int main(int argc, char *argv[])
{
	struct args *args;
	struct stock *stock;

	args = args_create(&argc, &argv);

	debug_init(args->debug, args->debug_file);
        sdebug_init(args->sdebug, args->sdebug_file);

	dir_create(args->data_dir, 0777);

	stock = stock_create(args);

        if(args->rocket)
	    estimate_rocket(args, stock);

        if(args->bender)
            estimate_bender(args, stock); 

        if(args->quarks)
            estimate_quarks(args, stock);


	stock_destroy(stock);
	args_destroy(args);

	return EXIT_SUCCESS;
}
