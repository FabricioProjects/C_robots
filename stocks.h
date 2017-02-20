#ifndef __STOCKS_H__
#define __STOCKS_H__

#ifndef __ARGS_H__
# include "args.h"
#endif

enum STOCK_FILE
{	STOCK_FILE_OPERATION,
	STOCK_FILE_DATETIME,
	STOCK_FILE_STOCK,
	STOCK_FILE_DATE,
	STOCK_FILE_RESERVED,
	STOCK_FILE_PRICE,
	STOCK_FILE_QUANTITY,
	STOCK_FILE_NUM
};

struct trades
{
	int num;
	size_t size;

	time_t *time;

	struct ma *ma;
	struct candle *candle;
	struct candle *candle_out_loss;
	struct candle *candle_out_gain;
	struct gmi *gmi;
	struct gmi *gminorm;

	double *price;
	long *quantity;
};

struct days
{
	int num;
	size_t size;

	struct trades *trades;
	double *open;
	double *close;
	double *high;
	double *low;
	double *average;
};

struct stock
{
	char *name;
	size_t namelen;

	FILE *file;
	char filename[FILENAME_MAX];

	struct days days;
	struct trades trades;
};

extern struct stock * stock_create(struct args *);
extern void stock_destroy(struct stock *);
extern int stock_read(struct args *args, struct stock *stock);

#endif /* __STOCKS_H__ */
