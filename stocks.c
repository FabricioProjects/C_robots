#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "err.h"
#include "stocks.h"
#include "misc.h"
#include "debug.h"
#include "ma.h"
#include "candle.h"
#include "gmi.h"

static void trades_resize(struct trades *trades, long size)
{
	int i;

	if (size <= 0)
		size = 2 * trades->size;

	trades->time = err_realloc (trades->time, size * sizeof (*trades->time));
	trades->price = err_realloc (trades->price, size * sizeof (*trades->price));
	trades->quantity =
			err_realloc (trades->quantity, size * sizeof (*trades->quantity));

	trades->ma = err_realloc (trades->ma, size * sizeof (*trades->ma));
	trades->gmi = err_realloc (trades->gmi, size * sizeof (*trades->gmi));
	trades->candle =
			err_realloc (trades->candle, size * sizeof (*trades->candle));
	trades->candle_out_gain =
			err_realloc (trades->candle_out_gain, size * sizeof (*trades->candle_out_gain));
	trades->candle_out_loss =
			err_realloc (trades->candle_out_loss, size * sizeof (*trades->candle_out_loss));

	for (i = trades->size; i < size; i++)
	{
		trades->time[i] = 0;
		trades->price[i] = 0.0;
		trades->quantity[i] = 0;

		candle_init(&trades->candle[i]);
		candle_init(&trades->candle_out_gain[i]);	
		candle_init(&trades->candle_out_loss[i]);	
	}

	trades->size = size;
}

static void trades_create(struct trades *trades, long size)
{
	trades->num = 0;
	trades->size = 0;
	trades->time = NULL;
	trades->price = NULL;
	trades->quantity = NULL;

	trades->ma = NULL;
	trades->gmi = NULL;
	trades->candle = NULL;
	trades->candle_out_gain = NULL;
	trades->candle_out_loss = NULL;

	trades_resize(trades, size);
}

int stock_read(struct args *args, struct stock *stock)
{
	int i;
	time_t unixtime;
	time_t currtime;
	ssize_t length, *plen;
	size_t size;
	char **p, *buffer;

	if (!stock->file)
	{
		sdebug("Dod versão %s",VERSION);
		debug("Dod versão %s",VERSION);
		debug("Abrindo arquivo %s", stock->filename);
		stock->file = fopen(stock->filename, "r");
	}

	if (!stock->file)
		return 0;

	p = err_malloc (STOCK_FILE_NUM * sizeof (*p));
	plen = err_malloc (STOCK_FILE_NUM * sizeof (*plen));

	buffer = NULL;
	size = i = 0;
	currtime = time(0);

	while ((length = getline(&buffer, &size, stock->file)) > 0)
	{
		if (LINE_IS_COMMENT_OR_BLANK (buffer))
			continue;

		if (strncmp(buffer, "T", 1) != 0)
			continue;

		TRIMMING_TRAILING(buffer, length);
		strnsplit(buffer, length, p, plen, ',', STOCK_FILE_NUM);

		/* -3 is used because the file uses additional 3 digits that we don't need */
		unixtime = (time_t) atoilen(p[STOCK_FILE_DATETIME],
				plen[STOCK_FILE_DATETIME] - 3);
                
        //        sdebug("unixtime: %ld", unixtime );
		if (unixtime > args->date_finish)
			break;
		else if ( unixtime >= args->date_start){
			if ((i != 0) && (unixtime < stock->trades.time[i]))
			{
				debug("Erro de horário. time_ant:%ld time_now:%ld [%s]",
                                	stock->trades.time[i], unixtime, ctime(&unixtime));
				continue;
			}
		}
		else 
			continue;
		
		i = stock->trades.num++;

		if (stock->trades.num > stock->trades.size)
			trades_resize(&stock->trades, -1);

		stock->trades.time[i] = unixtime;
		stock->trades.price[i] = atoflen(p[STOCK_FILE_PRICE],
				plen[STOCK_FILE_PRICE]);
		stock->trades.quantity[i] = atollen(p[STOCK_FILE_QUANTITY],
				plen[STOCK_FILE_QUANTITY]);

		ma_create(&stock->trades.ma[i], args->ma_num);

		/* previne contra looping com duração maior que um segundo, isso é
		 útil para os casos de execução como daemon. */
		if ((args->daemon) && (currtime != time(0)))
			break;
	}

	free(buffer);
	free(plen);
	free(p);

	if (stock->trades.num < stock->trades.size)
		trades_resize(&stock->trades, stock->trades.num);

	return 1;
}

struct stock * stock_create(struct args *args)
{
	struct stock *stock;

	stock = err_malloc (sizeof (*stock));

	stock->name = args->stock;
	stock->namelen = strlen(args->stock);

	stock->file = NULL;
	snprintf(stock->filename, FILENAME_MAX, "%s/input/%s", args->data_dir,
			stock->name);

	trades_create(&stock->trades, BUFSIZ);

	return stock;
}

static void trades_destroy(struct trades *trades)
{
	int i;

	free(trades->time);
	free(trades->price);
	free(trades->quantity);
	free(trades->gmi);
	free(trades->candle);
	free(trades->candle_out_gain);
	free(trades->candle_out_loss);

	for (i = 0; i < trades->num; i++)
		ma_destroy(&trades->ma[i]);

	free(trades->ma);
}

void stock_destroy(struct stock *stock)
{
	if (stock->file)
		fclose(stock->file);

	trades_destroy(&stock->trades);
	free(stock);
}
