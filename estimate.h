#ifndef __ESTIMATE_H__
#define __ESTIMATE_H__

#ifndef __ARGS_H__
# include "args.h"
#endif

#ifndef __STOCKS_H__
# include "stocks.h"
#endif

enum ESTIMATE_FINALIZE
{	ESTIMATE_FINALIZE_LOSS,
	ESTIMATE_FINALIZE_GAIN,
	ESTIMATE_FINALIZE_TIME,
	ESTIMATE_FINALIZE_PCUT,
	ESTIMATE_FINALIZE_NULL,
	ESTIMATE_FINALIZE_NUM,
	ESTIMATE_FINALIZE_WGAP,
	ESTIMATE_FINALIZE_POST,
	ESTIMATE_FINALIZE_ZERT,
	ESTIMATE_FINALIZE_NEGT
};

enum ESTIMATE_DIRECTIONS
{
	ESTIMATE_DIRECTIONS_LONG, ESTIMATE_DIRECTIONS_SHORT, ESTIMATE_DIRECTIONS_NUM
};

//enum ESTIMATE_ROBOTS
//{	ESTIMATE_ROCKET,
//        ESTIMATE_BENDER,
//        ESTIMATE_QUARKS
//};

struct estimate
{
	int direction;
	int t_i_buy;

	time_t initialize_time;
	time_t re_initialize_time;
	time_t finalize_time;
	time_t trade_stop;
	time_t last_calc_p;

	int finalize_status;
	int loss_status;

	double buy_price;
	double sell_price;
	double stop_loss;
	double stop_gain;
	double p0;

	long quantity;
	long ticks;
};

extern void estimate_rocket(struct args *, struct stock *);

#endif /* __ESTIMATE_H__ */
