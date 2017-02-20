#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include "stats.h"
#include "err.h"

double stats_mean(double *data, size_t size)
{
	int i;
	double mean = 0.0;

	for (i = 0; i < size; i++)
		mean += data[i];

	return mean / (double) size;
}

double stats_wllmean(double *price, unsigned long long *quantity, size_t size)
{
	int i;
	double mean = 0.0;
	double sum_price = 0.0;
	unsigned long long sum_quantity = 0;

	for (i = 0; i < size; i++)
	{
		sum_price += price[i] * quantity[i];
		sum_quantity += quantity[i];
	}

	mean = sum_price / (double) sum_quantity;

	return mean;
}

double stats_wlmean(double *price, long *quantity, size_t size)
{
	int i;
	double mean = 0.0;
	double sum_price = 0.0;
	long sum_quantity = 0;

	for (i = 0; i < size; i++)
	{
		sum_price += price[i] * quantity[i];
		sum_quantity += quantity[i];
	}

	mean = sum_price / (double) sum_quantity;

	return mean;
}

double stats_meani(int *data, size_t size)
{
	int i;
	double mean = 0.0;

	for (i = 0; i < size; i++)
		mean += data[i];

	return mean / (double) size;
}

double stats_meanll(unsigned long long *data, size_t size)
{
	int i;
	double mean = 0.0;

	for (i = 0; i < size; i++)
		mean += data[i];

	return mean / (double) size;
}

double stats_variance(double *data, size_t size, double mean)
{
	int i;
	double delta = 0.0;
	double variance = 0.0;

	for (i = 0; i < size; i++)
	{
		delta = data[i] - mean;
		variance += (delta * delta - variance) / (i + 1);
	}

	return variance;
}

double stats_variance_price(double *price, unsigned long long *quantity,
		size_t size, double mean)
{
	int i;
	double delta = 0.0;
	double variance = 0.0;
	unsigned long long sum_quantity = 0;

	for (i = 0; i < size; i++)
	{
		sum_quantity += quantity[i];
		delta = (price[i] - mean) * (quantity[i] / (double) sum_quantity);
		variance += (delta * delta - variance) / (i + 1);
	}

	return variance;
}

double stats_sd(double *data, size_t size, double mean)
{
	return sqrt(stats_variance(data, size, mean));
}

double stats_sd_price(double *price, unsigned long long *quantity, size_t size,
		double mean)
{
	return sqrt(stats_variance_price(price, quantity, size, mean));
}

double stats_variancei(int *data, size_t size, double mean)
{
	int i;
	double delta = 0.0;
	double variance = 0.0;

	for (i = 0; i < size; i++)
	{
		delta = (double) data[i] - mean;
		variance += (delta * delta - variance) / (i + 1);
	}

	return variance;
}

double stats_sdi(int *data, size_t size, double mean)
{
	return sqrt(stats_variancei(data, size, mean));
}

double stats_variancell(unsigned long long *data, size_t size, double mean)
{
	int i;
	double delta = 0.0;
	double variance = 0.0;

	for (i = 0; i < size; i++)
	{
		delta = (double) data[i] - mean;
		variance += (delta * delta - variance) / (i + 1);
	}

	return variance;
}

double stats_sdll(unsigned long long *data, size_t size, double mean)
{
	return sqrt(stats_variancell(data, size, mean));
}

/**
 * gsl_stats_correlation()
 *
 *   Calculate Pearson correlation = cov(X, Y) / (sigma_X * sigma_Y)
 *   This routine efficiently computes the correlation in one pass of the
 *   data and makes use of the algorithm described in:
 *   
 *   B. P. Welford, "Note on a Method for Calculating Corrected Sums of
 *   Squares and Products", Technometrics, Vol 4, No 3, 1962.
 *   
 *   This paper derives a numerically stable recurrence to compute a sum
 *   of products
 *   
 *   S = sum_{i=1..N} [ (x_i - mu_x) * (y_i - mu_y) ]
 *   
 *   with the relation
 *   
 *   S_n = S_{n-1} + ((n-1)/n) * (x_n - mu_x_{n-1}) * (y_n - mu_y_{n-1})
 **/
double stats_correlation(double data1[], double data2[], size_t n)
{
	size_t i;
	long double sum_xsq = 0.0;
	long double sum_ysq = 0.0;
	long double sum_cross = 0.0;
	long double ratio;
	long double delta_x, delta_y;
	long double mean_x, mean_y;
	long double r;

	/*
	 * Compute:
	 * sum_xsq = Sum [ (x_i - mu_x)^2 ],
	 * sum_ysq = Sum [ (y_i - mu_y)^2 ] and
	 * sum_cross = Sum [ (x_i - mu_x) * (y_i - mu_y) ]
	 * using the above relation from Welford's paper
	 */

	mean_x = data1[0];
	mean_y = data2[0];

	for (i = 1; i < n; ++i)
	{
		ratio = i / (i + 1.0);
		delta_x = data1[i] - mean_x;
		delta_y = data2[i] - mean_y;
		sum_xsq += delta_x * delta_x * ratio;
		sum_ysq += delta_y * delta_y * ratio;
		sum_cross += delta_x * delta_y * ratio;
		mean_x += delta_x / (i + 1.0);
		mean_y += delta_y / (i + 1.0);
	}

	r = sum_cross / (sqrt(sum_xsq) * sqrt(sum_ysq));

	return r;
}
