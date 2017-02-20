#ifndef __STATS_H__
#define __STATS_H__

#define LINEAR_INTERPOLATE_POINT(a,b,x) ((a * x) + b)

extern double stats_mean(double *, size_t);
extern double stats_wllmean(double *, unsigned long long *, size_t);
extern double stats_wlmean(double *, long *, size_t);
extern double stats_meani(int *, size_t);
extern double stats_meanll(unsigned long long *, size_t);
extern double stats_variance(double *, size_t, double);
extern double stats_variance_price(double *, unsigned long long *, size_t,
		double);
extern double stats_sd(double *, size_t, double);
extern double stats_sd_price(double *, unsigned long long *, size_t, double);
extern double stats_variancei(int *, size_t, double);
extern double stats_sdi(int *, size_t, double);
extern double stats_variancell(unsigned long long *, size_t, double);
extern double stats_sdll(unsigned long long *, size_t, double);
extern double stats_correlation(double[], double[], size_t);

#endif /* __STATS_H__ */
