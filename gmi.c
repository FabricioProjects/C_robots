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
#include "gmi.h"

bool is_same_day(const time_t time_ini, const time_t time_end)
{
	struct tm tm_ini; // current date
	struct tm tm_end; // previous date

	gmtime_r(&time_ini, &tm_ini);
	gmtime_r(&time_end, &tm_end);
	
	return ((tm_ini.tm_mday == tm_end.tm_mday)
			&& (tm_ini.tm_mon == tm_end.tm_mon)
			&& (tm_ini.tm_year == tm_end.tm_year));
}

void gmi(struct args *args, struct stock *stock, int id)
{
	struct gmi *gmicur; // current  gmi
	struct gmi *gmipre; // previous gmi
	struct gmi *gmipro; // on processing gmi (loop)

	time_t timecur; // current  time
	time_t timepre; // previous time

	bool samesec; // current and previous tick is same second
	bool sameday; // current and previous tick is same day

	int index;
	int current = id;
	int previous = id - 1;
	
	int daycount_out = 1;
	// if first tick then set default values and return
	if (current == 0)
	{
		gmicur = &stock->trades.gmi[id];

		gmicur->last_day = -1;
		gmicur->last_sec = -1;

		gmicur->count_sec = 1;
		gmicur->count_day = 1;

		gmicur->sum_sec = 0;
		gmicur->sum_day = 0;

		return;
	}

	// current and previous gmi
	gmicur = &stock->trades.gmi[current];
		
	gmipre = &stock->trades.gmi[previous];
	// current and previous time
	timecur = stock->trades.time[current];
	timepre = stock->trades.time[previous];

	// is same second and is same day
	samesec = (timecur == timepre);
	sameday = (samesec || is_same_day(timecur, timepre));

	// same second
	if (samesec)
	{
		gmicur->last_sec = gmipre->last_sec;
		gmicur->count_sec = gmipre->count_sec + 1;
	}
	else
	{
		gmicur->last_sec = previous;
		gmicur->count_sec = 1;
	}
	// same day
	if (sameday)
	{
		gmicur->last_day = gmipre->last_day;
		gmicur->count_day = gmipre->count_day + 1;
		gmicur->norm_value = gmipre->norm_value;
	}
	else
	{
		daycount_out=daycount_out+1;		
		gmicur->last_day = previous;
		gmicur->count_day = 1;
		if ( !args->gmi_factor )
		{
			int daycount_in = 1;		
			gmicur->sum_sec = 0;	
			
			for (index = gmicur->last_sec; index > 0; index = gmipro->last_sec)
			{
				gmipro = &stock->trades.gmi[index];				
				if ( !is_same_day(stock->trades.time[index],stock->trades.time[gmipro->last_sec] ))
					daycount_in ++;
					
				else
					gmicur->norm_value = gmipre->norm_value;
				if (gmipro->count_sec > 1)
				{
					gmicur->sum_sec += gmipro->count_sec;
					gmicur->tot_neg ++;
				}
				if ( daycount_in > args->gmi_avg_days )
				{
					gmicur->norm_value = (double) gmicur->sum_sec / (double) gmicur->tot_neg;
					break;
				}
			}	
						
			
		}
	}

	// now calculate value for seconds
	gmicur->sum_sec = 0;
	for (index = gmicur->last_sec; index > 0; index = gmipro->last_sec)
	{
				
		gmipro = &stock->trades.gmi[index];
		if ((timecur - stock->trades.time[index]) > args->gmi_tframe)
			break;

		// only sum if count > 1
		if (gmipro->count_sec > 1)
			gmicur->sum_sec += gmipro->count_sec;
	}
    	if (args->gmi_factor == 0)
			gmicur->value = (double) gmicur->sum_sec
			/ (args->gmi_tframe * gmicur->norm_value);
	else
		gmicur->value = (double) gmicur->sum_sec
			/ (args->gmi_tframe * args->gmi_factor);	
}
