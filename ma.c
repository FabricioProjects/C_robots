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
#include "ma.h"

static void ma_resize(struct ma *ma, long size)
{
	int i;

	if (size <= 0)
		size = 2 * ma->size;

	ma->price = err_realloc (ma->price, size * sizeof (*ma->price));
	ma->trades_num = err_realloc (ma->trades_num, size *
			sizeof (*ma->trades_num));

	for (i = ma->size; i < size; i++)
	{
		ma->price[i] = 0.0;
		ma->trades_num[i] = 0;
	}

	ma->size = size;
}

void ma_create(struct ma *ma, long size)
{
	ma->num = 0;
	ma->size = 0;

	ma->price = NULL;
	ma->trades_num = NULL;

	ma_resize(ma, size);
}

int ma_sort_cmp(const void *ain, const void *bin)
{
#define a ((const int *) ain)
#define b ((const int *) bin)
	if (*a > *b )
		return 1;
	else if (*a < *b )
		return -1;
	else
		return 0;
#undef a
#undef b
}

void ma_days(struct args *args, struct stock *stock)
{
	int i, j, k, l, m, ma_num, ma_i, trades_num, trades_tot;
	struct trades *trades, *trades_tmp;
	struct ma *ma = NULL;
	double ma_sum, last_price, last_qtt;
	time_t diff_time;
	bool ma_calc;

	ma_num = args->ma_num;

	/* Ordenando os valores das médias móveis em ordem crescente. */
	qsort(args->ma, ma_num, sizeof(*args->ma), ma_sort_cmp);

	for (i = 0; i < stock->days.num; i++)
	{
		trades = &stock->days.trades[i];

		for (j = 0; j < trades->num; j++)
		{
			ma = &stock->days.trades[i].ma[j];
			trades_tmp = trades;

			k = i;
			l = j;
			ma_sum = 0;
			trades_num = 0;
			trades_tot = 0;
			last_price = trades_tmp->price[l];
			last_qtt = trades_tmp->quantity[l];
			diff_time = trades->time[j] - trades_tmp->time[l];

			for (ma_i = 0; ma_i < ma_num; ma_i++)
			{
				ma_calc = false;

				if (diff_time > args->ma[ma_i])
				{
					ma_calc = true;
				}
				else
				{
					ma_sum += last_price * last_qtt;
					trades_num++;
				}

				while (ma_calc != true)
				{
					if (l == 0)
					{
						if (k == 0)
						{
							ma_calc = true;
						}
						else
						{
							k--;
							trades_tmp = &stock->days.trades[k];
							l = trades_tmp->num - 1;
						}
					}
					else
					{
						l--;
					}

					if (ma_calc != true)
					{
						last_price = trades_tmp->price[l];
						diff_time = trades->time[j] - trades_tmp->time[l];

						if (diff_time > args->ma[ma_i])
						{
							ma_calc = true;
						}
						else
						{
							ma_sum += last_price;
							trades_num++;
						}
					}
				}
				ma->price[ma_i] = ma_sum / (double) trades_tot;
				ma->trades_num[ma_i] = trades_num;

				/* Se chegou até o fim da série de trades e days e ainda há
				 * médias a serem calculadas, elas não precisam ser
				 * calculadas, apenas uso o valor da média atual que, pela
				 * ordenação do vetor ARGS->MA é mais curta que as
				 * próximas. */
				if (ma_i < args->ma_num && k == 0 && l == 0)
				{
					m = ma_i;

					while (++ma_i < args->ma_num)
					{
						ma->price[ma_i] = ma->price[m];
						ma->trades_num[ma_i] = ma->trades_num[m];
					}
				}
			}
		}
	}
}

void ma(struct args *args, struct stock *stock, int i)
{
	int j, m, ma_num, ma_i, trades_num;
	struct ma *ma = NULL;
	double ma_money_sum, last_price;
	double last_quantity, ma_quantity_sum;
	time_t diff_time;
	bool ma_calc;

	ma_num = args->ma_num;

	/* Ordenando os valores das médias móveis em ordem crescente. */
	qsort(args->ma, ma_num, sizeof(*args->ma), ma_sort_cmp);

	ma = &stock->trades.ma[i];

	trades_num = 0;
	j = i;
	last_price = stock->trades.price[j];
	last_quantity = stock->trades.quantity[j];
	diff_time = stock->trades.time[i] - stock->trades.time[j];
	ma_money_sum = 0;
	ma_quantity_sum = 0;

	for (ma_i = 0; ma_i < ma_num; ma_i++)
	{
		ma_calc = false;

		if (diff_time > args->ma[ma_i])
		{
			ma_calc = true;
		}
		else
		{
			ma_money_sum += last_price * last_quantity;
			ma_quantity_sum += last_quantity;
			trades_num++;
		}

		while (ma_calc != true)
		{
			if (j > 0)
				j--;
			else
				ma_calc = true;

			if (ma_calc != true)
			{
				last_price = stock->trades.price[j];
				last_quantity = stock->trades.quantity[j];
				diff_time = stock->trades.time[i] - stock->trades.time[j];

				if (diff_time > args->ma[ma_i])
				{
					ma_calc = true;
				}
				else
				{
					ma_money_sum += last_price * last_quantity;
					ma_quantity_sum += last_quantity;
					trades_num++;
				}
			}
		}

		ma->price[ma_i] = ma_money_sum / (double) ma_quantity_sum;
		ma->trades_num[ma_i] = trades_num;
		/* Se chegou até o fim da série de trades e days e ainda há
		 * médias a serem calculadas, elas não precisam ser
		 * calculadas, apenas uso o valor da média atual que, pela
		 * ordenação do vetor ARGS->MA é mais curta que as
		 * próximas. */
		if (ma_i < args->ma_num && j == 0)
		{
			m = ma_i;

			while (++ma_i < args->ma_num)
			{
				ma->price[ma_i] = ma->price[m];
				ma->trades_num[ma_i] = ma->trades_num[m];
			}
		}
	}
}

void ma_destroy(struct ma *ma)
{
	free(ma->trades_num);
	free(ma->price);
}
