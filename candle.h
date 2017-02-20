#ifndef __CANDLE_H__
#define __CANDLE_H__

#ifndef __ARGS_H__
# include "args.h"
#endif

#ifndef __STOCKS_H__
# include "stocks.h"
#endif

struct candle
{
	int trades_num;
	int last;

	double open;
	double high;
	double average;
	double low;
	double close;

	long quantity;
	long ticks;
	time_t close_time;
};

extern void candle_init(struct candle *);
extern void candle(struct args *, struct stock *stock, int i);

#endif /* __CANDLE_H__ */
