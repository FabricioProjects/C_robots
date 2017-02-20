#ifndef __ARGS_H__
#define __ARGS_H__

#ifndef _STDBOOL_H
# include <stdbool.h>
#endif

#define DEFAULT_DATA_DIR "/var/lib/stoker"

/* Used by main to communicate with parse_opt. */
struct args
{
        char *data_dir;
	char *stock;
	char *profile;
	char *str_ma;
        bool estimate;  
        bool rocket;
        bool bender;
        bool quarks;
	int *ma;
	int ma_num;
	int candle;
	int candle_out_gain;
	int candle_out_loss;
        bool print_trades;
        bool replace_data;
        bool simulation;
        bool daemon;
	int dir;
	char *dir_name;
	int block_time;

	double sma_stop_gain;
	double sma_min_stop_loss;
	double sma_max_stop_loss;
	double sma_wgap;
	char *str_sma_p;
	double sma_p_a;
	double sma_p_b;
	double sma_p_c;
	time_t sma_p_update_time;
	char *sma_trade_start;
	char *sma_trade_stop;  

	double t_init;
	double t_max;
	double min_ret;
	double amp_factor;
	double min_factor;
	double time_factor;
	double time_factor_min;
	double neg_lim;
	double margin1;
	double margin2;
	double cut;
        double inf_band_lim;
        double sup_band_lim;
        double perc_band_lim;
        double gmi_max;
	double time_factor_beta;
	double time_factor_gamma;
	double beta_min;
	double gmi_min;
	double gmi_factor;
	int gmi_tframe;
	int gmi_avg_days;
	bool stop_clone;
	time_t date_start;
	time_t date_finish;
	/* Program options */
#ifndef NDEBUG
	bool debug;
        bool sdebug;
	char *debug_file;
        char *sdebug_file;
#endif
};

extern struct args *args_create(int *, char ***);
extern void args_destroy(struct args *);

#endif /* __ARGS_H__ */
