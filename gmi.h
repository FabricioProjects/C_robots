#ifndef __GMI_H__
#define __GMI_H__

#ifndef __ARGS_H__
# include "args.h"
#endif

#ifndef __STOCKS_H__
# include "stocks.h"
#endif

struct gmi
{
	int trades_num;
	int last_day;
	int last_sec;
	int count_sec;
	int count_day;
	long int sum_sec;
	int sum_day;
	double value;
	double norm_value;
	long int tot_neg;
};

extern void gmi(struct args *, struct stock *stock, int id);

#endif /* __GMI_H__ */
