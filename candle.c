#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "args.h"
#include "err.h"
#include "stats.h"
#include "misc.h"
#include "debug.h"
#include "candle.h"

void candle_init(struct candle *candle)
{
	candle->trades_num = 0;
	candle->open = 0.0;
	candle->high = 0.0;
	candle->average = 0.0;
	candle->low = 0.0;
	candle->close = 0.0;
	candle->quantity = 0;
	candle->ticks = 0;
	candle->close_time = 0;
}

void candle(struct args *args, struct stock *stock, int i)
{
	int count_candle, count_candle_gain, count_candle_loss;
	struct candle *candle, *candle_out_gain, *candle_out_loss;
	time_t diff_time;
	bool candle_calc, candle_calc_out_gain, candle_calc_out_loss;
	double price;

	count_candle = i;
	count_candle_gain = i;
	count_candle_loss = i;
	
	candle = &stock->trades.candle[i];

	price = stock->trades.price[i];
	if (args->candle_out_gain){
		candle_out_gain = &stock->trades.candle_out_gain[i];
		candle_calc_out_gain = false;
		candle_out_gain->last = -1;
		candle_out_gain->close = price;
		candle_out_gain->close_time = stock->trades.time[i];
		candle_out_gain->high = price;
		candle_out_gain->low = price;
		candle_out_gain->quantity = stock->trades.quantity[i];
		candle_out_gain->trades_num = 1;
	}
	else
		candle_calc_out_gain = true;
	
	if (args->candle_out_loss){
		candle_out_loss = &stock->trades.candle_out_loss[i];
		candle_calc_out_loss = false;
		candle_out_loss->last = -1;
		candle_out_loss->close = price;
		candle_out_loss->close_time = stock->trades.time[i];
		candle_out_loss->high = price;
		candle_out_loss->low = price;
		candle_out_loss->quantity = stock->trades.quantity[i];
		candle_out_loss->trades_num = 1;
	}
	else
		candle_calc_out_loss = true;			
	
	
	candle->last = -1;
	candle->close = price;
	candle->close_time = stock->trades.time[i];
	candle->high = price;
	candle->low = price;
	candle->quantity = stock->trades.quantity[i];
	candle->trades_num = 1;

	candle_calc = false;

	while (candle_calc == false || candle_calc_out_gain == false || candle_calc_out_loss == false)
	{
		if (count_candle > 0)
			count_candle--;
		else
			candle_calc = true;
		
		if (count_candle_gain > 0)
			count_candle_gain--;
		else
			candle_calc_out_gain = true;

		if (count_candle_gain > 0)
			count_candle_loss--;
		else
			candle_calc_out_loss = true;			

		if (!candle_calc)
		{
			diff_time = stock->trades.time[i] - stock->trades.time[count_candle];

			if (diff_time > args->candle)
			{
				candle_calc = true;
				count_candle++;
			}
			else
			{
				candle->trades_num++;
				price = stock->trades.price[count_candle];

				if (price > candle->high)
					candle->high = price;

				if (price < candle->low)
					candle->low = price;

				candle->quantity += stock->trades.quantity[count_candle];
			}
		}

		if (!candle_calc_out_gain)
		{
			diff_time = stock->trades.time[i] - stock->trades.time[count_candle_gain];

			if (diff_time > args->candle_out_gain)
			{
				candle_calc_out_gain = true;
				count_candle_gain++;
			}
			else
			{
				candle_out_gain->trades_num++;
				price = stock->trades.price[count_candle_gain];

				if (price > candle_out_gain->high)
					candle_out_gain->high = price;

				if (price < candle_out_gain->low)
					candle_out_gain->low = price;

				candle_out_gain->quantity += stock->trades.quantity[count_candle_gain];
			}
		}

		if (!candle_calc_out_loss)
		{
			diff_time = stock->trades.time[i] - stock->trades.time[count_candle_loss];

			if (diff_time > args->candle_out_loss)
			{
				candle_calc_out_loss = true;
				count_candle_loss++;
			}
			else
			{
				candle_out_loss->trades_num++;
				price = stock->trades.price[count_candle_loss];

				if (price > candle_out_loss->high)
					candle_out_loss->high = price;

				if (price < candle_out_loss->low)
					candle_out_loss->low = price;

				candle_out_loss->quantity += stock->trades.quantity[count_candle_loss];
			}
		}		
	}

	/* candle->last é o último candle não sobreposto. */
	if (count_candle > 0)
		candle->last = count_candle - 1;
	candle->open = stock->trades.price[count_candle];
	candle->average = stats_wlmean(stock->trades.price + count_candle,
			stock->trades.quantity + count_candle, candle->trades_num);
	candle->ticks = candle->trades_num;
	
	if (args->candle_out_gain){	
		if (count_candle_gain >0)
			candle_out_gain->last = count_candle_gain - 1;
		candle_out_gain->open = stock->trades.price[count_candle_gain];
		candle_out_gain->average = stats_wlmean(stock->trades.price + count_candle_gain,
			stock->trades.quantity + count_candle_gain, candle_out_gain->trades_num);
		candle_out_gain->ticks = candle_out_gain->trades_num;	
	}
	
	if (args->candle_out_loss){	
		if (count_candle_loss > 0)
			candle_out_loss->last = count_candle_loss - 1;		
		
		candle_out_loss->open = stock->trades.price[count_candle_loss];
		candle_out_loss->ticks = candle_out_loss->trades_num;	
	}

}
