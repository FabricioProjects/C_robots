#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include "args.h"
#include "err.h"
#include "stats.h"
#include "misc.h"
#include "debug.h"
#include "ma.h"
#include "candle.h"
#include "gmi.h"
#include "estimate.h"

static char *str_directions[] = {
    "LONG",
    "SHORT"
};

static char *str_finalize[] = {
    "LOSS",
    "GAIN",
    "TIME",
    "PCUT",
    "NULL",
    "NUM",
    "WGAP",
    "POST",
    "ZERT",
    "NEGT"
};

time_t estimate_get_last_hint(struct estimate *estimate, struct args *args) 
{
    FILE *file;
    char filename[FILENAME_MAX];
    time_t timestamp = 0;

    //sdebug("em estimate get last hint");
    
    if(args->rocket)
      snprintf(filename, FILENAME_MAX, "%s/output/rocket/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);
    
    if(args->bender)
      snprintf(filename, FILENAME_MAX, "%s/output/bender/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);
    
    if(args->quarks)
      snprintf(filename, FILENAME_MAX, "%s/output/quarks/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);

    file = fopen(filename, "r");
    if (file) {
        if (fscanf(file, "%lld", (long long *) &timestamp))
            fclose(file);
    }

    return timestamp;
}

static time_t estimate_set_last_hint(struct estimate *estimate,
        struct args *args) {
    
    FILE *file;
    char filename[FILENAME_MAX];
    time_t timestamp;
    
    //sdebug("em estimate_set last hint");
    
    if(args->rocket)
    snprintf(filename, FILENAME_MAX, "%s/output/rocket/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);
    
    if(args->bender)
    snprintf(filename, FILENAME_MAX, "%s/output/bender/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);
    
    if(args->quarks)
    snprintf(filename, FILENAME_MAX, "%s/output/quarks/%s-%s-%s.last",
            args->data_dir, args->stock, args->profile, args->dir_name);

    if (estimate->finalize_time == -1)
        timestamp = estimate->initialize_time;
    else
    {
        timestamp = estimate->finalize_time;
    }

    file = err_fopen(filename, "w");
    fprintf(file, "%ld\n", timestamp);
    fclose(file);

    return timestamp;
}

void estimate_print(struct estimate *estimate, struct args *args) 
{
    FILE *file;
    char filename[FILENAME_MAX];
    char buftime[18]; /* YYYY-MM-DD HHMMSS */
    struct stat st;
    struct tm tm;
    struct timeval tv;
    static time_t last_hint = 0;
    
    //sdebug("entrou no estimate_print");
    
    // simulation mode, don't need to much checking and use just one file for all hints
    if (args->simulation) 
    {
        
        if(args->rocket)
        snprintf(filename, FILENAME_MAX, "%s/output/rocket/%s-%s-%s",
                args->data_dir, args->stock, args->profile, args->dir_name);
        
        if(args->bender)
        snprintf(filename, FILENAME_MAX, "%s/output/bender/%s-%s-%s",
                args->data_dir, args->stock, args->profile, args->dir_name);
        
        if(args->quarks)
        snprintf(filename, FILENAME_MAX, "%s/output/quarks/%s-%s-%s",
                args->data_dir, args->stock, args->profile, args->dir_name);
        
    }   // production mode needs to check last hint to prevent emit it again
    else 
    {
        // read last hint file only for the first hint
        if (last_hint == 0)
            last_hint = estimate_get_last_hint(NULL, args);

        // if hint is already emited, just return
        if ((estimate->initialize_time < last_hint)
                && (estimate->finalize_time <= last_hint))
            return;

        gettimeofday(&tv, NULL);
        
        if(args->rocket)
          snprintf(filename, FILENAME_MAX,
                "%s/output/rocket/%s-%s-%s-%ld.%06d.%d", args->data_dir,
                args->stock, args->profile, args->dir_name,
                estimate->initialize_time, tv.tv_usec, (estimate->finalize_time > 0));
        
        if(args->bender)
          snprintf(filename, FILENAME_MAX,
                "%s/output/bender/%s-%s-%s-%ld.%06d.%d", args->data_dir,
                args->stock, args->profile, args->dir_name,
                estimate->initialize_time, tv.tv_usec, (estimate->finalize_time > 0));
        
        if(args->quarks)
          snprintf(filename, FILENAME_MAX,
                "%s/output/quarks/%s-%s-%s-%ld.%06d.%d", args->data_dir,
                args->stock, args->profile, args->dir_name,
                estimate->initialize_time, tv.tv_usec, (estimate->finalize_time > 0));
    }

    if ((args->replace_data) || (stat(filename, &st) < 0 && errno == ENOENT)) 
    {
        args->replace_data = false;

        file = err_fopen(filename, "w");

        fprintf(file, "#1|INITIALIZE UNIXTIME\t");
        fprintf(file, "2|INITIALIZE DATETIME\t");
        fprintf(file, "3|FINALIZE UNIXTIME\t");
        fprintf(file, "4|FINALIZE DATETIME\t");
        fprintf(file, "5|DIRECTION\t");
        fprintf(file, "6|STATUS\t");
        fprintf(file, "7|BUY PRICE\t");
        fprintf(file, "8|SELL PRICE\t");
        fprintf(file, "9|STOP LOSS\t");
        fprintf(file, "10|STOP GAIN\t");
        fprintf(file, "11|QUANTITY\t");
        fprintf(file, "12|TICKS\t");
        fprintf(file, "--DOD version %s\n",VERSION);
    } else {
        file = err_fopen(filename, "a");
    }

    fprintf(file, "%ld\t", estimate->initialize_time);
    localtime_r(&estimate->initialize_time, &tm);
    strftime(buftime, sizeof (buftime), "%F %H%M%S", &tm);
    fprintf(file, "%s\t", buftime);

    fprintf(file, "%ld\t", estimate->finalize_time);
    localtime_r(&estimate->finalize_time, &tm);
    strftime(buftime, sizeof (buftime), "%F %H%M%S", &tm);
    fprintf(file, "%s\t", buftime);

    fprintf(file, "%s\t", str_directions[estimate->direction]);
    fprintf(file, "%s\t", str_finalize[estimate->finalize_status]);
    fprintf(file, "%lg\t", estimate->buy_price);
    fprintf(file, "%lg\t", estimate->sell_price);
    fprintf(file, "%lg\t", estimate->stop_loss);
    fprintf(file, "%lg\t", estimate->stop_gain);
    fprintf(file, "%ld\t", estimate->quantity);
    fprintf(file, "%ld\n", estimate->ticks);

    fclose(file);

    // in prodution, update last hint file
    if (!args->simulation)
        last_hint = estimate_set_last_hint(estimate, args);
}

static void estimate_print_trade(struct args *args, struct stock *stock, int i) {
    int j, k;
    static FILE *file;
    char filename[FILENAME_MAX];
    char buftime[18]; /* YYYY-MM-DD HHMMSS */
    struct stat st;
    struct tm tm;

    snprintf(filename, FILENAME_MAX, "%s/output/rocket/%s-%s-%s.trades",
            args->data_dir, args->stock, args->profile, args->dir_name);

    if (stat(filename, &st) < 0 && errno == ENOENT) {
        file = err_fopen(filename, "w");

        fprintf(file, "#1|UNIXTIME"
                "\t2|DATETIME"
                "\t3|PRICE"
                "\t4|QUANTITY");

        j = 4;

        if (args->ma != NULL)
            for (k = 0; k < args->ma_num; k++) {
                fprintf(file, "\t%d|MA %d", j++, k);
                fprintf(file, "\t%d|MA %d TRADES", j++, k);
            }

        if (args->candle >= 0) {
            fprintf(file, "\t%d|CANDLE TRADES", ++j);
            fprintf(file, "\t%d|CANDLE OPEN", ++j);
            fprintf(file, "\t%d|CANDLE HIGH", ++j);
            fprintf(file, "\t%d|CANDLE AVERAGE", ++j);
            fprintf(file, "\t%d|CANDLE LOW", ++j);
            fprintf(file, "\t%d|CANDLE CLOSE", ++j);
            fprintf(file, "\t%d|CANDLE QUANTITY", ++j);
            fprintf(file, "\t%d|CANDLE TICKS", ++j);
	    fprintf(file, "\t%d|GMI", ++j);
	    fprintf(file, "\n");
        }
    } else {
        file = err_fopen(filename, "a");
    }

    localtime_r(&stock->trades.time[i], &tm);
    strftime(buftime, sizeof (buftime), "%F %H%M%S", &tm);

    fprintf(file, "%ld\t%s\t%lg\t%ld", stock->trades.time[i], buftime,
            stock->trades.price[i], stock->trades.quantity[i]);

    if (args->ma != NULL)
        for (j = 0; j < args->ma_num; j++) {
            fprintf(file, "\t%lg\t%d", stock->trades.ma[i].price[j],
                    stock->trades.ma[i].trades_num[j]);
        }

    if (args->candle >= 0)
        fprintf(file, "\t%d\t%lg\t%lg\t%lg\t%lg\t%lg\t%ld\t%ld\t%lg",
            stock->trades.candle[i].trades_num,
            stock->trades.candle[i].open, stock->trades.candle[i].high,
            stock->trades.candle[i].average, stock->trades.candle[i].low,
            stock->trades.candle[i].close, stock->trades.candle[i].quantity,
            stock->trades.candle[i].ticks, stock->trades.gmi[i].value);

    fprintf(file, "\n");

    fclose(file);
}

void estimate_reset(struct estimate *estimate) {
    estimate->direction = -1;
    estimate->initialize_time = -1;
    estimate->finalize_time = -1;
    estimate->buy_price = 0.0;
    estimate->sell_price = 0.0;
    estimate->quantity = 0;
    estimate->ticks = 0;
    estimate->finalize_status = 4;
    estimate->loss_status = -1;
    estimate->last_calc_p = -1;
    estimate->t_i_buy = -1;

    estimate->stop_loss = 0.0;
    estimate->stop_gain = 0.0;
    estimate->p0 = 0.0;
}

static bool estimate_initialize_rocket(struct estimate *estimate, 
                                       struct args *args,
                                       struct stock *stock, 
                                       int t_i) 
{    
    int i, ma_up, ma_down;
    double price_above_ma, price_below_ma, price;
    double sg, min_sl, max_sl, tmp_sl;
    time_t time, trade_start = -1, trade_stop = -1;
    bool initialize = false;
    bool interval = false;
    struct candle *candle, *candle_prev;
    struct ma *ma;
    struct tm tm;
    char buftime[18]; // YYYY-MM-DD HHMMSS 
    
    if (t_i == 0)
        return initialize;

    candle = &stock->trades.candle[t_i];

    if (candle->last < 0)
        return initialize;

    if (stock->trades.time[t_i] - stock->trades.time[0]
            < args->ma[args->ma_num - 1])
        return initialize;

    candle_prev = &stock->trades.candle[candle->last];
    ma = &stock->trades.ma[t_i];

    price = stock->trades.price[t_i];
    time = stock->trades.time[t_i];

    if (args->sma_trade_start != NULL && args->sma_trade_stop != NULL) 
    {
        interval = interval_time (time, args->sma_trade_start, &trade_start,
                                  args->sma_trade_stop, &trade_stop);

        if (interval == false) 
        {
            return initialize;
        } 
        else 
        {
            if (trade_stop > trade_start) {
                /* 15 é o número de segundos antes de trade_stop que ainda pode
                 * iniciar uma operação. */
                if (trade_stop - time < args->block_time * 60)
                    return initialize;
            } 
            else 
            {
                if (time - trade_stop < 120)
                    return initialize;
            }
        }
    }
    sg = (args->sma_stop_gain) * price;
    min_sl = (args->sma_min_stop_loss) * price;
    max_sl = (args->sma_max_stop_loss) * price;

    ma_up = ma_down = price_above_ma = price_below_ma = 0;

    for (i = 0; i < args->ma_num; i++) {
        if (i != 0) {
            if (ma->price[i - 1] > ma->price[i])
                ma_up++;
            else if (ma->price[i - 1] < ma->price[i])
                ma_down++;
        }

        if (price > ma->price[i])
            price_above_ma++;
        else if (price < ma->price[i])
            price_below_ma++;
    }

    if (price_above_ma == args->ma_num && ma_up == args->ma_num - 1
            && args->dir != 1) {
        debug("PASSOU CONDICAO DAS MEDIAS");
        estimate->direction = ESTIMATE_DIRECTIONS_LONG;

        if (price > candle->open && price >= candle->high) {
            debug("PASSOU CONDICAO DO CANDLE CASO LONG as %ld", time);
           // sdebug("PASSOU CONDICAO DO CANDLE CASO LONG as %ld", time);
            initialize = true;
            tmp_sl = price - candle_prev->low;

            if (tmp_sl > max_sl)
                estimate->stop_loss = price - max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price - min_sl;
            else
                estimate->stop_loss = candle_prev->low;

            estimate->buy_price = price;
            estimate->stop_gain = price + sg;
        }
    } else if (price_below_ma == args->ma_num && ma_down == args->ma_num - 1
            && args->dir != 0) {
        estimate->direction = ESTIMATE_DIRECTIONS_SHORT;

        if (price < candle->open && price <= candle->low) {
            debug("PASSOU CONDICAO DO CANDLE CASO SHORT as %ld", time);
            //sdebug("PASSOU CONDICAO DO CANDLE CASO SHORT as %ld", time);
            initialize = true;
            tmp_sl = candle_prev->high - price;

            if (tmp_sl > max_sl)
                estimate->stop_loss = price + max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price + min_sl;
            else
                estimate->stop_loss = candle_prev->high;

            estimate->sell_price = price;
            estimate->stop_gain = price - sg;
        }
    }
    if (stock->trades.gmi[t_i].value >= 1000000000
            || stock->trades.gmi[t_i].value <= args->gmi_min) {
        if (initialize == true) {
            debug("bloqueado por vol as: %ld", time);
        }
        initialize = false;
    }
    if (initialize == true) 
    {
        if (trade_stop >= 0 && trade_start >= 0) {
            estimate->trade_stop = trade_stop;

            if (trade_stop > trade_start) {
                if (time > trade_stop)
                    estimate->trade_stop += 3600 * 24;
            } else {
                if (time > trade_start)
                    estimate->trade_stop += 3600 * 24;
            }
        } else {
            estimate->trade_stop = -1;
        }

        estimate->t_i_buy = t_i;
        estimate->initialize_time = time;
        estimate->re_initialize_time = time;
        estimate->quantity = candle->quantity;
        estimate->ticks = candle->ticks;
    }

    if (initialize == true) 
    {
        localtime_r(&estimate->initialize_time, &tm);
        strftime(buftime, sizeof (buftime), "%F %H%M%S", &tm);
        debug("inicio de OP %s no intante %s (UNIXTIME) %ld", str_directions[estimate->direction], buftime, time);
        sdebug("inicio de OP %s no intante %s (UNIXTIME) %ld", str_directions[estimate->direction], buftime, time);
        debug("gatilho: %lg stop gain inicial: %lg stop loss inicial %lg", price, estimate->stop_gain, estimate->stop_loss);
        sdebug("preço inicial: %lg stop gain inicial: %lg stop loss inicial %lg", price, estimate->stop_gain, estimate->stop_loss);
    }

    return initialize;
}

static int estimate_initialize_clone(struct estimate *estimate,
        struct args *args, struct stock *stock, int t_i) {
    int i, ma_up, ma_down;
    double price_above_ma, price_below_ma, price;
    time_t time, trade_start = -1, trade_stop = -1;
    struct candle *candle, *candle_prev;
    struct ma *ma;
    int initialize = 0;
    bool interval = false;

    if (t_i == 0)
        return initialize;

    candle = &stock->trades.candle[t_i];

    if (candle->last < 0)
        return initialize;

    if (stock->trades.time[t_i] - stock->trades.time[0]
            < args->ma[args->ma_num - 1])
        return initialize;

    time = stock->trades.time[t_i];

    if (args->sma_trade_start != NULL && args->sma_trade_stop != NULL)
    {
        interval = interval_time (time, args->sma_trade_start, &trade_start,
                args->sma_trade_stop, &trade_stop);

        if (interval == true)
        {
            
	    if (trade_stop > trade_start)
            {
		
                if (trade_stop - time < args->block_time * 60 && args->stop_clone)
                {
                    debug("parou a clone por estar depois de blocktime %ld ",time);
                    return initialize;
                }
            }
        }
    }

    candle_prev = &stock->trades.candle[candle->last];
    ma = &stock->trades.ma[t_i];

    price = stock->trades.price[t_i];
    time = stock->trades.time[t_i];

    ma_up = ma_down = price_above_ma = price_below_ma = 0;

    for (i = 0; i < args->ma_num; i++) {
        if (i != 0) {
            if (ma->price[i - 1] > ma->price[i])
                ma_up++;
            else if (ma->price[i - 1] < ma->price[i])
                ma_down++;
        }

        if (price > ma->price[i])
            price_above_ma++;
        else if (price < ma->price[i])
            price_below_ma++;
    }

    debug(
            "antes dos ifs finais da clone no instante %ld, initialize %d ", time, initialize);
    if (price_above_ma == args->ma_num && ma_up == args->ma_num - 1) {
        if (price > candle->open && price >= candle_prev->high) {
            initialize = 1;
        }
    } else if (price_below_ma == args->ma_num && ma_down == args->ma_num - 1) {
        if (price < candle->open && price <= candle_prev->low) {
            initialize = -1;
            debug(
                    "clone chamada e dando condição de venda no  instante %ld", time);
        }
    }

    /* A clone não tem check de vol e olha para candle ANTERIOR, enquanto 
     a que confere a condição de entrada real olha o candle ATUAL */
return initialize;
}

static time_t estimate_stop_loss_update_rocket(struct estimate *estimate,
        struct args *args, struct stock *stock, time_t time_ref, int t_i) 
{
    struct candle *candle_prev;
    time_t time;
    double price, min_sl, max_sl, tmp_sl, temp_new_loss, t, fator, wgap;
    int c_i_prev;

    time = stock->trades.time[t_i];
    price = stock->trades.price[t_i];
    t = time - estimate->initialize_time; // tempo real de operação
    
    // sdebug("tempo real de operação %lg ", t);
    
    fator = (1 - t / args->time_factor);
    if (fator < args->time_factor_min)
        fator = args->time_factor_min;

    min_sl = (args->sma_min_stop_loss) * price;
    max_sl = (args->sma_max_stop_loss) * price;
    
    if(args->candle_out_loss){
    	c_i_prev = stock->trades.candle_out_loss[t_i].last;
 	candle_prev = &stock->trades.candle_out_loss[c_i_prev];
    }
    else{
	c_i_prev = stock->trades.candle[t_i].last;
    	candle_prev = &stock->trades.candle[c_i_prev];
    } 	
    wgap = (args->sma_wgap) * price;

    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
        tmp_sl = price - candle_prev->low;

        if (tmp_sl > max_sl)
            temp_new_loss = price - max_sl;
        else if (tmp_sl < min_sl)
            temp_new_loss = price - min_sl;
        else
            temp_new_loss = candle_prev->low;

        if (temp_new_loss > estimate->stop_loss
                && temp_new_loss > estimate->buy_price + fator * wgap
		&& temp_new_loss < price) {
            estimate->stop_loss = temp_new_loss;
            debug("Novo stop loss %lg no instante %ld", temp_new_loss, time);
            //sdebug("Novo stop loss %lg no instante %ld", temp_new_loss, time);
            estimate->loss_status = ESTIMATE_FINALIZE_WGAP;
            time_ref = time;
        }
    } else if (estimate->direction == ESTIMATE_DIRECTIONS_SHORT) {
        tmp_sl = candle_prev->high - price;

        if (tmp_sl > max_sl)
            temp_new_loss = price + max_sl;
        else if (tmp_sl < min_sl)
            temp_new_loss = price + min_sl;
        else
            temp_new_loss = candle_prev->high;

        if (temp_new_loss < estimate->stop_loss
                && temp_new_loss < estimate->sell_price - fator * wgap
		&& temp_new_loss > price) {
            estimate->stop_loss = temp_new_loss;
            estimate->loss_status = ESTIMATE_FINALIZE_WGAP;
            debug("Novo stop loss %lg no instante %ld", temp_new_loss, time);
            time_ref = time;
        }
    }

    return time_ref;
}

static bool estimate_finalize_status_rocket(struct estimate *estimate,
        struct args *args, struct stock *stock, double price, time_t time,
        int *t_i) {
    
    int variavel, v2;
    bool finalize = false;
    bool aux_finalize = true;
    bool next_day = false;
    variavel = estimate_initialize_clone(estimate, args, stock, *t_i);
    v2 = estimate->direction;
    v2 = (1 - v2) / (1 + v2) - v2;
    if (variavel * v2 == -1) {
    } else if (variavel * v2 == 1) {
        aux_finalize = false;
        estimate->re_initialize_time = time;
        debug("zerou o tempo");
        debug("re_init %ld", estimate->re_initialize_time);
        debug("direção: %s", str_directions[estimate->direction]);
        return finalize;
    }

    /* No else if acima é onde é criado o contador que zera a cada
     pseudo-entrada (quando a clone dá condição de entrada) */
    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) 
    {
        if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) {
            (*t_i)--;
            if(time - stock->trades.time[(*t_i)] > 3600 * 10)
                next_day=true;
            finalize = true;
            estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
        } else if (price <= estimate->stop_loss && aux_finalize == true) {
            if (estimate->loss_status == ESTIMATE_FINALIZE_WGAP) {
                estimate->finalize_status = ESTIMATE_FINALIZE_WGAP;
                debug("Finalizou com WGAP no instante %ld", time);
                sdebug("Finalizou com WGAP no instante %ld", time);
            } else {
                debug("Finalizou com LOSS no instante %ld", time);
                sdebug("Finalizou com LOSS no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_LOSS;
            }
            finalize = true;
            
        } else if (price >= estimate->stop_gain && aux_finalize == true) {
            finalize = true;
            if (estimate->finalize_status == ESTIMATE_FINALIZE_POST) {
                estimate->finalize_status = ESTIMATE_FINALIZE_POST;
                debug("Finalizou com POST no instante %ld", time);
                sdebug("Finalizou com POST no instante %ld", time);
            } else if (estimate->finalize_status == ESTIMATE_FINALIZE_NEGT) {
                debug("Finalizou com NEGT no instante %ld", time);
                sdebug("Finalizou com NEGT no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_NEGT;
            } else if (estimate->finalize_status == ESTIMATE_FINALIZE_ZERT) {
                debug("Finalizou com ZERT no instante %ld", time);
                sdebug("Finalizou com ZERT no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_ZERT;
            } else {
                estimate->finalize_status = ESTIMATE_FINALIZE_GAIN;
                debug("Finalizou com GAIN no instante %ld", time);
                sdebug("Finalizou com GAIN no instante %ld", time);
            }
            
        } else if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) {
            (*t_i)--;
            finalize = true;
	    if(time - stock->trades.time[(*t_i)] > 3600 * 10)
            	next_day=true;
	    estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
        }

        if (finalize == true && aux_finalize == true) 
        {
            if(next_day){
		estimate->finalize_time = stock->trades.time[(*t_i)];
		estimate->sell_price = stock->trades.price[(*t_i)];
	    }
	    else{
	    	estimate->finalize_time = time;
		estimate->sell_price = price;
	    }
        }
        
    } else if (estimate->direction == ESTIMATE_DIRECTIONS_SHORT) {
        if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) {
            (*t_i)--;
	    if(time - stock->trades.time[(*t_i)] > 3600 * 10)
                next_day=true;
            finalize = true;
            estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
        } else if (price >= estimate->stop_loss && aux_finalize == true) {
            finalize = true;
            if (estimate->loss_status == ESTIMATE_FINALIZE_WGAP) {
                estimate->finalize_status = ESTIMATE_FINALIZE_WGAP;
                debug("Finalizou com WGAP no instante %ld", time);
                sdebug("Finalizou com WGAP no instante %ld", time);
            } else {
                debug("Finalizou com LOSS no instante %ld", time);
                sdebug("Finalizou com LOSS no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_LOSS;
            }
        } else if (price <= estimate->stop_gain && aux_finalize == true) {
            finalize = true;
            if (estimate->finalize_status == ESTIMATE_FINALIZE_POST) {
                estimate->finalize_status = ESTIMATE_FINALIZE_POST;
                debug("Finalizou com POST no instante %ld", time);
                sdebug("Finalizou com POST no instante %ld", time);
            } else if (estimate->finalize_status == ESTIMATE_FINALIZE_NEGT) {
                debug("Finalizou com NEGT no instante %ld", time);
                sdebug("Finalizou com NEGT no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_NEGT;
            } else if (estimate->finalize_status == ESTIMATE_FINALIZE_ZERT) {
                debug("Finalizou com ZERT no instante %ld", time);
                sdebug("Finalizou com ZERT no instante %ld", time);
                estimate->finalize_status = ESTIMATE_FINALIZE_ZERT;
            } else {
                estimate->finalize_status = ESTIMATE_FINALIZE_GAIN;
                debug("Finalizou com GAIN no instante %ld", time);
                sdebug("Finalizou com GAIN no instante %ld", time);
            }
        } else if (price <= estimate->stop_gain && aux_finalize == false) {
            debug("Não deixou sair as %ld", time);
        } 
        if (finalize == true && aux_finalize == true) {
            if(next_day){
                estimate->finalize_time = stock->trades.time[(*t_i)-1];
                estimate->buy_price = stock->trades.price[(*t_i)-1];
            }
            else{
                estimate->finalize_time = time;
                estimate->buy_price = price;
            }

        }
    }

    return finalize;
}

static bool estimate_sma_p_rocket(struct estimate *estimate, struct args *args,
        struct stock *stock, int t_i) //, double *p0)
{
    bool finalize = false;
    bool interval = false;
    double p, a, b, c, price, tmp_ret_price, ff, sg, beta, t_max, t_min,
            min_ret, amp_factor, last , min_factor;
    double cut, margin1, margin2, t1, tfb, tfg, mb;
    time_t time, t, t_aux;
    time_t trade_start = -1, trade_stop = -1;
    int candle_time;

    ff = 1.0;
    if(estimate->direction == ESTIMATE_DIRECTIONS_LONG)
    	sg = (args->sma_stop_gain) * estimate->buy_price;
    else
	sg = (args->sma_stop_gain) * estimate->sell_price;
    
    if (args->t_init < (100.0 / args->sma_p_c))
        t_min = (100.0 / args->sma_p_c) + 1.0;
    else
        t_min = args->t_init;

    time = stock->trades.time[t_i];
    last = stock->trades.price[t_i];
    if(args->candle_out_gain){
    	price = stock->trades.candle_out_gain[t_i].average;
	candle_time = args->candle_out_gain;
    }
    else {
	price = stock->trades.candle[t_i].average;
	candle_time = args->candle;
    }
	
    t = time - estimate->re_initialize_time;
    t_aux = time - estimate->initialize_time;
    interval = interval_time (time, args->sma_trade_start, &trade_start,
            args->sma_trade_stop, &trade_stop);
    /* Dentro da função p são usados os dois tempos. Em geral o tempo real de op
     só controla se já há tempo suficiente para usar o preço médio como referência.
     O tempo depois da pseudo-entrada é o usado na função p */
    if (t_aux > t_min)
    {
        if (t_aux < candle_time) {
            tmp_ret_price = (
                    estimate->direction == ESTIMATE_DIRECTIONS_LONG ?
                    last - estimate->buy_price :
                    estimate->sell_price - last);
        } else {
            tmp_ret_price = (
                    estimate->direction == ESTIMATE_DIRECTIONS_LONG ?
                    price - estimate->buy_price :
                    estimate->sell_price - price);
        }
        estimate->last_calc_p = time;
        /* p(t) = (10^2 * a  + 10^-2 * b * t) * log (10^-2 * c * t)
         * o 0.4343 vem da conversao de base
         * */
        a = 1.0e2 * args->sma_p_a;
        b = 1.0e-2 * args->sma_p_b;
        c = 1.0e-2 * args->sma_p_c;
        p = (b * (double) t + a) * 0.4343 * log(c * (double) t);
        t_max = args->t_max;
        min_ret = args->min_ret;
        amp_factor = args->amp_factor;
        margin1 = (args->margin1) * last;
        margin2 = (args->margin2) * last;
	min_factor = (args->min_factor) * last;
        cut = -1.0 * ((args->cut) * last);
        beta = 1.0;
    tfg = trade_stop - time;
    tfb = args->time_factor_beta;
    mb = args->beta_min;
        if (tfg < tfb)
                {
                        beta = tfg/tfb;
        if (beta < mb)
                        beta = mb;
                }
    if (p > 0.0)
        {
            ff = tmp_ret_price / p;
            debug(
                    "Tempo maior que tmin, P MAIOR QUE ZERO p(%ld)=%lg ff=%lg (ret_price:%lg)", t, p, ff, tmp_ret_price);
            debug(
                    "time:%ld stop gain antes da correcao=%lg finalize:%s", time, estimate->stop_gain, (finalize == true ? "true" : "false"));
            if (tmp_ret_price >= min_ret * p)
            {
                debug("ENTROU NO PRIMEIRO IF DO LUCRO (lucro > min_ret)");
                debug("correcao do gain %lg", (sg * ff));
                if (ff > amp_factor)
                    ff = amp_factor;
                if (estimate->direction == ESTIMATE_DIRECTIONS_LONG)
                    estimate->stop_gain = estimate->buy_price + (sg * ff);
                else
                    estimate->stop_gain = estimate->sell_price - (sg * ff);
            } else if (tmp_ret_price < (min_ret * p) && tmp_ret_price >= (-1.0 * cut) + p * (stock->trades.gmi[t_i].value / args->gmi_min)) {
                estimate->finalize_status = ESTIMATE_FINALIZE_POST;
                debug("ENTROU NO SEGUNDO IF DO LUCRO (POST)");
                if (estimate->direction == ESTIMATE_DIRECTIONS_LONG)
                    estimate->stop_gain = estimate->buy_price + tmp_ret_price + beta * min_factor;
                else
                    estimate->stop_gain = estimate->sell_price - tmp_ret_price - beta * min_factor;
                debug("correcao do gain %lg", (sg * ff));
            } else if (tmp_ret_price < (-1.0 * cut) + p * (stock->trades.gmi[t_i].value / args->gmi_min) && tmp_ret_price >= cut) {
                debug("ENTROU NO TERCEIRO IF DO LUCRO (ZERT)");
                estimate->finalize_status = ESTIMATE_FINALIZE_ZERT;
                if (estimate->direction == ESTIMATE_DIRECTIONS_LONG)
                    estimate->stop_gain = estimate->buy_price + tmp_ret_price
                        + beta * min_factor;
                else
                    estimate->stop_gain = estimate->sell_price - tmp_ret_price
                        - beta * min_factor;
                debug("correcao do gain %lg", (sg * ff));
            } else if (tmp_ret_price < cut) {
                debug("ENTROU NO QUARTO IF DO LUCRO (NEGT) ");
                estimate->finalize_status = ESTIMATE_FINALIZE_NEGT;
                margin1 = beta * margin1;
        margin2 = beta * margin2;
                if (t < t_min + t_max)
                {
                    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
                        if (margin1 > tmp_ret_price + margin2)
                            estimate->stop_gain = estimate->buy_price + margin1;
                        else
                            estimate->stop_gain = estimate->buy_price + tmp_ret_price + margin2;
                    } else {
                        if (margin1 > tmp_ret_price + margin2)
                            estimate->stop_gain = estimate->sell_price - margin1;
                        else
                            estimate->stop_gain = estimate->sell_price - tmp_ret_price - margin2;
                    }
                } else
                {
                    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
                        if (margin1 < tmp_ret_price + margin2)
                            estimate->stop_gain = estimate->buy_price + margin1;
                        else
                            estimate->stop_gain = estimate->buy_price + tmp_ret_price + margin2;
                    } else {
                        if (margin1 < tmp_ret_price + margin2)
                            estimate->stop_gain = estimate->sell_price - margin1;
                        else
                            estimate->stop_gain = estimate->sell_price - tmp_ret_price - margin2;
                    }
                }
            }
            debug("depois dos ifs de correcao do gain: stop gain= %lg", estimate->stop_gain);
        }

    }
    return finalize;
}

static bool estimate_finalize_rocket(struct estimate *estimate, struct args *args,
        struct stock *stock, int t_i) 
{  
    double price, last;
    time_t diff_time, time_ref, time;
    bool finalize = false;
    struct tm tm;
    char buftime[18]; /* YYYY-MM-DD HHMMSS */

    time_ref = estimate->initialize_time;
    last = stock->trades.price[t_i];
    localtime_r(&stock->trades.time[t_i], &tm);
    strftime(buftime, sizeof (buftime), "%F %H%M%S", &tm);

    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
        debug("Pl: %lg\t Price %lg Time %s\t", last - estimate->buy_price, last, buftime);
        sdebug("Pl: %lg\t Price %lg Time %s\t", last - estimate->buy_price, last, buftime);
    }
    if (estimate->direction == ESTIMATE_DIRECTIONS_SHORT) {
        debug("Pl: %lg\t Price %lg Time %s\t", estimate->sell_price - last, last, buftime);
        sdebug("Pl: %lg\t Price %lg Time %s\t", estimate->sell_price - last, last, buftime);
    }
    
    time = stock->trades.time[t_i];
    price = stock->trades.price[t_i];

    if (args->str_sma_p != NULL) {
        if ((estimate->last_calc_p < 0)
                || (estimate->last_calc_p > 0
                && time - estimate->last_calc_p
                > args->sma_p_update_time))
            finalize = estimate_sma_p_rocket(estimate, args, stock, t_i); //, &p0);
    }

    if (finalize != true) 
    {
        debug("antes de chamar a finalize time: %ld fin: %d stop gain: %lg price: %lg", time, finalize, estimate->stop_gain, price);
        finalize = estimate_finalize_status_rocket(estimate, args, stock, price, time, &t_i);
        debug("depois de chamar a finalize fin: %d stop gain: %lg price: %lg", finalize, estimate->stop_gain, price);
    }

    if (finalize != true) {
        diff_time = time - time_ref;
    	if(args->candle_out_loss){
        	if (diff_time > args->candle_out_loss)
            	   time_ref = estimate_stop_loss_update_rocket(estimate, args, stock,
                	time_ref, t_i);
    	}
    	else
    	{
		if (diff_time > args->candle)
            	   time_ref = estimate_stop_loss_update_rocket(estimate, args, stock,
             		time_ref, t_i);
	}
    }

    /* Esse caso protege caso haja corte por tempo de finalização de tempo
     * de operação no mesmo negócio do inicio. Isso pode ocorrer pois na
     * hora que acontece o corte por TIME, utiliza-se o negócio anterior
     * ao corte. */
    if (finalize == true && estimate->finalize_status == ESTIMATE_FINALIZE_TIME
            && t_i <= estimate->t_i_buy)
        return false;

    return finalize;
}

static volatile bool finalized = false;

void terminate(int signo) {
    finalized = true;
}

void estimate_rocket(struct args *args, struct stock *stock) {
    struct estimate estimate;
    bool initialized = false;
    int i = 0;
    int daycount = 1;
    //printf("Entrou em estimate rocket\n");
    
    if (args->daemon)
        signal(SIGTERM, terminate);

    estimate_reset(&estimate);

    stock_read(args, stock);
  
    while ((args->daemon && !finalized) || (i < stock->trades.num)) 
    {
        debug("ESTIMATE: Processando (%d/%d)...", i, stock->trades.num);
	
	if(i>0){	
		if ( !is_same_day(stock->trades.time[i],stock->trades.time[i-1] ))
			daycount++;
		}
	
	if (args->gmi_tframe > 0)
            gmi(args, stock, i);

	if(daycount > args->gmi_avg_days){  
		if (args->ma != NULL)
            		ma(args, stock, i);

        	if (args->candle >= 0)
            		candle(args, stock, i);
        
        	if (args->print_trades)
            		estimate_print_trade(args, stock, i);

        	if (!initialized) {
            		initialized = estimate_initialize_rocket(&estimate, args, stock, i);
            	if (initialized && !args->simulation)
                	estimate_print(&estimate, args);
        	} else {
            		if (estimate_finalize_rocket(&estimate, args, stock, i)) 
            		{
                		estimate_print(&estimate, args);
                		estimate_reset(&estimate);
                		initialized = false;
            		}
        	}
	}
        i++;
      
        if (args->daemon) {
            while ((i == stock->trades.num) && !finalized) {
                debug("ESTIMATE: Lendo (%d/%d)...", i, stock->trades.num);
                stock_read(args, stock);
                if (i == stock->trades.num) {
                    debug("ESTIMATE: Dormindo (%d/%d)...", i, stock->trades.num);
                    sleep(1);
                }
            }
        }
    }
}
