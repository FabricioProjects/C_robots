#ifndef __MA_H__
#define __MA_H__

#ifndef __ARGS_H__
# include "args.h"
#endif

#ifndef __STOCKS_H__
# include "stocks.h"
#endif

struct ma
{
	int num;
	size_t size;

	int *trades_num;
	double *price;
};

extern void ma_create(struct ma *, long);
extern void ma_days(struct args *, struct stock *);
extern void ma(struct args *, struct stock *, int i);
extern void ma_destroy(struct ma *);
extern int  ma_sort_cmp(const void *ain, const void *bin);

#endif /* __MA_H__ */
