#ifdef HAVE_CONFIG_H
#  include <config.h>
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
#include "estimate_bender.h"

static char *str_directions[] = {
    "LONG",
    "SHORT"
};

static bool estimate_initialize_bender(struct estimate *estimate, struct args *args,
        struct stock *stock, int t_i) {
    
    int i, ma_up, ma_down;
    double price_above_ma, price_below_ma, price, inf_band_lim, sup_band_lim, perc_band_lim;
    double sg, min_sl, max_sl, tmp_sl;
    double *p0;
    time_t time, trade_start = -1, trade_stop = -1;
    bool initialize = false, interval = false, out = false;
    struct candle *candle, *candle_prev;
    struct ma *ma;
    struct tm tm;
    char buftime[18]; // YYYY-MM-DD HHMMSS 
    
    inf_band_lim = args->inf_band_lim / 1000.0;
    sup_band_lim = args->sup_band_lim / 1000.0;
    perc_band_lim = args->perc_band_lim / 1000.0;
    
    p0= (double *)malloc(sizeof(double));
    estimate->p0 = *p0;

    if (t_i == 0)
        return initialize;

    candle = &stock->trades.candle[t_i];

    if (candle->last < 0)
        return initialize;
    // debug("dif %ld arg %ld", (stock->trades.time[t_i] - stock->trades.time[0]), args->ma[args->ma_num - 1]);
    if (stock->trades.time[t_i] - stock->trades.time[0] < args->ma[args->ma_num - 1]) 
    {
        //debug("NO CORTE DO TEMPO AS  %ld", time);
        return initialize;
    }

    candle_prev = &stock->trades.candle[candle->last];
    ma = &stock->trades.ma[t_i];
    price = stock->trades.price[t_i];
    time = stock->trades.time[t_i];
    
    if (args->sma_trade_start != NULL && args->sma_trade_stop != NULL) 
    {
        interval = interval_time(time, args->sma_trade_start, &trade_start, 
                                 args->sma_trade_stop, &trade_stop);
        
        if (interval == false) 
        {
            return initialize;
        } else 
        {
            if (trade_stop > trade_start) 
            {
                /* 15 é o número de segundos antes de trade_stop que ainda pode  iniciar uma operação. */
                if (trade_stop - time < args->block_time * 60)
                    return initialize;
            } else {
                if (time - trade_stop < 15)
                    return initialize;
            }
        }
    }

    sg = args->sma_stop_gain;
    min_sl = price * args->sma_min_stop_loss / 1000.0;
    max_sl = price * args->sma_max_stop_loss / 1000.0;
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

    // condições de início de operação na direção short
    if (ma_up != args->ma_num - 1 && price_above_ma > price_below_ma && args->dir != 0) 
    {
        debug("PASSOU CONDICAO DAS MEDIAS SHORT 1 e 2 as %ld", time);
        estimate->direction = ESTIMATE_DIRECTIONS_SHORT;

        bool cond1 = ((price >= (1 + perc_band_lim) * stock->trades.candle[t_i].high || price >= (1 + perc_band_lim) * candle_prev->high));
        bool cond2 = (fabs(stock->trades.candle[t_i].high - candle_prev->high) < price * sup_band_lim);
        bool cond3 = (price >= stock->trades.candle[t_i].high && price >= candle_prev->high);
        debug("SHORT cond1 cond2 cond3 %d %d %d", cond1, cond2, cond3);

        if ((cond1 && cond2)) 
        {
            debug("1+perc_band_lim * price %5.2f %lg %lg", (1 + perc_band_lim) * price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("sup_band_lim * diff %5.2f %lg ", price * sup_band_lim, fabs(stock->trades.candle[t_i].low - candle_prev->low));
            debug("PASSOU CONDICAO DO CANDLE CASO SHORT 1 e 2 as %ld", time);
            //sdebug("PASSOU CONDICAO DO CANDLE CASO SHORT 1 e 2 as %ld", time);
            debug("price %lg max. atual: %lg max. anterior %lg", price, stock->trades.candle[t_i].high, candle_prev->high);
            debug("price %lg min. atual: %lg min. anterior %lg", price, stock->trades.candle[t_i].low, candle_prev->low);
            initialize = true;
            tmp_sl = 1.5 * (price - stock->trades.candle[t_i].low);

            if (tmp_sl > max_sl)
                estimate->stop_loss = price + max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price + min_sl;
            else
                estimate->stop_loss = price + tmp_sl;

            estimate->sell_price = price;
            if (price - stock->trades.candle[t_i].low < price * inf_band_lim) {
                initialize = false;
                debug("banda pequena demais: price = %lg min = %lg", price, stock->trades.candle[t_i].low);
            } else {
                estimate->stop_gain = stock->trades.candle[t_i].low;
                estimate->p0 = price - estimate->stop_gain;
            }
        } else if ((cond3 && !cond2)) {
            debug("1+perc_band_lim * price %5.2f %lg %lg", (1 + perc_band_lim) * price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("sup_band_lim * diff %5.2f %lg ", price * sup_band_lim, fabs(stock->trades.candle[t_i].low - candle_prev->low));
            debug("PASSOU CONDICAO DO CANDLE CASO SHORT 3 e !2 as %ld", time);
           // sdebug("PASSOU CONDICAO DO CANDLE CASO SHORT 3 e !2 as %ld", time);
            debug("price %lg max. atual: %lg max. anterior %lg", price, stock->trades.candle[t_i].high, candle_prev->high);
            debug("price %lg min. atual: %lg min. anterior %lg", price, stock->trades.candle[t_i].low, candle_prev->low);
            initialize = true;
            tmp_sl = 1.5 * (price - stock->trades.candle[t_i].low);

            if (tmp_sl > max_sl)
                estimate->stop_loss = price + max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price + min_sl;
            else
                estimate->stop_loss = price + tmp_sl;

            estimate->sell_price = price;
            if (price - stock->trades.candle[t_i].low < price * inf_band_lim) {
                initialize = false;
                debug("banda pequena demais: price = %lg min = %lg", price, stock->trades.candle[t_i].low);
            } else {
                estimate->stop_gain = stock->trades.candle[t_i].low;
                estimate->p0 = price - estimate->stop_gain;
            }
        }
    }
    
    // condições de início de operação na direção long
    if (ma_down != args->ma_num - 1 && price_below_ma > price_above_ma && args->dir != 1) 
    {    
        debug("PASSOU CONDICAO DAS MEDIAS LONG 1 e 2 as %ld", time);
        estimate->direction = ESTIMATE_DIRECTIONS_LONG;

        bool cond1 = (((1 + perc_band_lim) * price <= stock->trades.candle[t_i].low || (1 + perc_band_lim) * price <= candle_prev->low));
        bool cond2 = (fabs(stock->trades.candle[t_i].low - candle_prev->low) < price * sup_band_lim);
        bool cond3 = (price <= stock->trades.candle[t_i].low && price <= candle_prev->low);
        debug("LONG cond1 cond2 cond3 %d %d %d", cond1, cond2, cond3);
        if ((cond1 && cond2)) 
        {
            debug("1+perc_band_lim * price %5.2f %lg %lg", (1 + perc_band_lim) * price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("sup_band_lim * diff %5.2f %lg ", price * sup_band_lim, fabs(stock->trades.candle[t_i].low - candle_prev->low));
            debug("PASSOU CONDICAO DO CANDLE caso LONG 1 e 2 as %ld", time);
           // sdebug("PASSOU CONDICAO DO CANDLE caso LONG 1 e 2 as %ld", time);
            debug("price %lg min. atual: %lg min. anterior %lg", price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("price %lg max. atual: %lg max. anterior %lg", price, stock->trades.candle[t_i].high, candle_prev->high);
            initialize = true;
            tmp_sl = 1.5 * (stock->trades.candle[t_i].high - price);

            if (tmp_sl > max_sl)
                estimate->stop_loss = price - max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price - min_sl;
            else
                estimate->stop_loss = price - tmp_sl;
            estimate->buy_price = price;
            if (stock->trades.candle[t_i].high - price < price * inf_band_lim) {
                debug("banda pequena demais");
                initialize = false;
            } else {
                estimate->stop_gain = stock->trades.candle[t_i].high;
                estimate->p0 = estimate->stop_gain - price;
                debug("p0: %lg sg: %lg price: %lg", estimate->p0, estimate->stop_gain, price);
                
            }
        } else if ((cond3 && !cond2)) {
            debug("1+perc_band_lim * price %5.2f %lg %lg", (1 + perc_band_lim) * price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("sup_band_lim * diff %5.2f %lg ", price * sup_band_lim, fabs(stock->trades.candle[t_i].low - candle_prev->low));
            //if (price <= stock->trades.candle[t_i].low   || price <= candle_prev->low)
            debug("PASSOU CONDICAO DO CANDLE caso LONG 3 e !2 as %ld", time);
           // sdebug("PASSOU CONDICAO DO CANDLE caso LONG 3 e !2 as %ld", time);
            debug("price %lg min. atual: %lg min. anterior %lg", price, stock->trades.candle[t_i].low, candle_prev->low);
            debug("price %lg max. atual: %lg max. anterior %lg", price, stock->trades.candle[t_i].high, candle_prev->high);
            initialize = true;
            tmp_sl = 1.5 * (stock->trades.candle[t_i].high - price);

            if (tmp_sl > max_sl)
                estimate->stop_loss = price - max_sl;
            else if (tmp_sl < min_sl)
                estimate->stop_loss = price - min_sl;
            else
                estimate->stop_loss = price - tmp_sl;
            estimate->buy_price = price;
            if (stock->trades.candle[t_i].high - price < price * inf_band_lim) {
                debug("banda pequena demais");
                initialize = false;
            } else {
                estimate->stop_gain = stock->trades.candle[t_i].high;                
                estimate->p0 = estimate->stop_gain - price;
                debug("p0: %lg sg: %lg price: %lg", estimate->p0, estimate->stop_gain, price);
                
            }
        }
    }
    if (stock->trades.gmi[t_i].value >= args->gmi_max || stock->trades.gmi[t_i].value <= args->gmi_min) 
    {
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
        // estimate->p0 = stock->trades.price[t_i] -  stock->trades.candle[t_i].average;
    }

    return initialize;
}

static time_t estimate_stop_loss_update_bender(struct estimate *estimate, struct args *args,
        struct stock *stock, time_t time_ref, int t_i) 
{  
    struct candle *candle_prev;
    time_t time;
    double price, min_sl, max_sl, tmp_sl, temp_new_loss, wgap;
    int c_i_prev;
    
    time = stock->trades.time[t_i];
    debug("primeira entrada em loss_update as %ld", time);
    price = stock->trades.price[t_i];
    min_sl = price * args->sma_min_stop_loss / 1000.0;
    max_sl = price * args->sma_max_stop_loss / 1000.0;
    wgap = price * args->sma_wgap / 1000.0;
    debug("max_sl %lg", max_sl);
    debug("min_sl %lg", min_sl);
    if(args->candle_out_loss){
    	c_i_prev = stock->trades.candle_out_loss[t_i].last;
 	candle_prev = &stock->trades.candle_out_loss[c_i_prev];
    }
    else{
	c_i_prev = stock->trades.candle[t_i].last;
    	candle_prev = &stock->trades.candle[c_i_prev];
    }     

    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
        tmp_sl = price - candle_prev->low;
        debug("temp_sl %lg e max_sl %lg no intante  (UNIXTIME) %ld", tmp_sl, max_sl, time);
        if (tmp_sl > max_sl)
            temp_new_loss = price - max_sl;
        else if (tmp_sl < min_sl)
            temp_new_loss = price - min_sl;
        else
            temp_new_loss = candle_prev->low;

        if (temp_new_loss > estimate->stop_loss
                && temp_new_loss > estimate->buy_price + wgap
		&& temp_new_loss < price) {
            estimate->stop_loss = temp_new_loss;
            debug("novo stop loss %lg no instante %ld", temp_new_loss, time);
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
                && temp_new_loss < estimate->sell_price - wgap
		&& temp_new_loss > price) {
            estimate->stop_loss = temp_new_loss;
            debug("novo stop loss %lg no instante %ld", temp_new_loss, time);
            estimate->loss_status = ESTIMATE_FINALIZE_WGAP;
            time_ref = time;
        }
    }

    return time_ref;
}

static bool estimate_finalize_status_bender(struct estimate *estimate, struct args *args, struct stock *stock,
        double price, time_t time, int *t_i) 
{
    bool finalize = false;
    bool next_day = false;

    if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) 
    {
        if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) {
            (*t_i)--;
	    if(time - stock->trades.time[(*t_i)] > 3600 * 10)
                next_day=true;
            finalize = true;
            estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
            debug("Finalizou por TIME no instante %ld", time);
            sdebug("Finalizou por TIME no instante %ld", time);
        } else if (price <= estimate->stop_loss) {
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
            
        } else if (price >= estimate->stop_gain) {
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
            
        } 

        if (finalize == true)
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
        
    } else if (estimate->direction == ESTIMATE_DIRECTIONS_SHORT) 
    {
       if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) {
            (*t_i)--;
            if(time - stock->trades.time[(*t_i)] > 3600 * 10)
                next_day=true;
            finalize = true;
            estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
            debug("Finalizou por TIME no instante %ld", time);
            sdebug("Finalizou por TIME no instante %ld", time);
        } else if (price >= estimate->stop_loss) {
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
        } else if (price <= estimate->stop_gain) {
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
            
        } else if (estimate->trade_stop >= 0 && time >= estimate->trade_stop) 
        {
            (*t_i)--;
            finalize = true;
            estimate->finalize_status = ESTIMATE_FINALIZE_TIME;
            debug("Finalizou por TIME no instante %ld", time);
            sdebug("Finalizou por TIME no instante %ld", time);
        }

        if (finalize == true)
        {
                if(next_day){
                        estimate->finalize_time = stock->trades.time[(*t_i)];
                        estimate->buy_price = stock->trades.price[(*t_i)];
                }
                else{
                        estimate->finalize_time = time;
                        estimate->buy_price = price;
                }
        }
    }

    return finalize;
}

static bool estimate_sma_p_bender(struct estimate *estimate, struct args *args,
        struct stock *stock, int t_i) 
{    
    bool finalize = false;
    double p, a, b, c, price, tmp_ret_price, ff, sgl, sgs, beta, t_max, t_min, min_ret, amp_factor;
    double last, neg_lim, cut, margin1, margin2, min_factor;
    time_t time, t;
    int region;
    int candle_time;

    ff = 1.0;

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

    t = time - estimate->initialize_time;
    min_factor = args->min_factor * price / 1000.0;
    debug("entrou na smp_p com %ld segundos de operação", t);
    
    if (t > t_min) 
    {
        if (t < 1000 * candle_time) 
        {
            tmp_ret_price = (estimate->direction == ESTIMATE_DIRECTIONS_LONG ?
                    last - estimate->buy_price :
                    estimate->sell_price - last);
        } else {
            tmp_ret_price = (estimate->direction == ESTIMATE_DIRECTIONS_LONG ?
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
        neg_lim = -1.0 * args->neg_lim;
        margin1 = args->margin1 * price / 1000.0 * stock->trades.gmi[t_i].value / args->gmi_min;
        margin2 = args->margin2 * price / 1000.0 * stock->trades.gmi[t_i].value / args->gmi_min;
        cut = -1.0 * args->cut * price / 1000.0;
        debug("margin1=%lg margin2=%lg", margin1, margin2);
        
        if (p > 0.0) // && (double) t > 120)
        {
            ff = tmp_ret_price / p;
            if (ff > amp_factor)
                ff = amp_factor;
            if (ff < min_ret)
                ff = min_ret;
            debug("T MAIOR QUE tmin, p(%ld)=%lg ff=%lg (ret_price:%lg) time:%ld  finalize:%s", t, p, ff,
                    tmp_ret_price, time, (finalize == true ? "true" : "false"));
            if (tmp_ret_price >= min_ret * p) 
            {
                debug("ENTROU NO PRIMEIRO IF DO LUCRO (lucro > min_ret)");
                if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) {
                    debug("fator de correcao do gain: %lg fator de gain inicial: %lg produto %lg", ff, estimate->p0, (estimate->p0) * ff);
                    estimate->stop_gain = estimate->buy_price + (estimate->p0 * ff);
                } else {
                    debug("fator de correcao do gain: %lg fator de gain inicial: %lg produto %lg", ff, estimate->p0, (estimate->p0) * ff);
                    estimate->stop_gain = estimate->sell_price - (estimate->p0 * ff);
                }
            } else if (tmp_ret_price < (min_ret * p) && tmp_ret_price >= cut + p * (stock->trades.gmi[t_i].value / args->gmi_max)) {
                estimate->finalize_status = ESTIMATE_FINALIZE_POST;
                if (estimate->direction == ESTIMATE_DIRECTIONS_LONG) 
                {
                    if (tmp_ret_price + min_factor >= estimate->p0)
                        estimate->stop_gain = estimate->buy_price + (estimate->p0);
                    else
                        estimate->stop_gain = estimate->buy_price + tmp_ret_price + min_factor;
                    debug("ENTROU NO SEGUNDO IF DO LUCRO (POST)");
                    debug("preco de compra:%lg retorno:%lg", estimate->buy_price, tmp_ret_price);
                }
                else 
                {
                    //estimate->stop_gain = estimate->sell_price -  sg * ff;
                    if (tmp_ret_price + min_factor >= (estimate->p0))
                        estimate->stop_gain = estimate->sell_price - (estimate->p0);
                    else
                        estimate->stop_gain = estimate->sell_price - tmp_ret_price - min_factor;
                    debug("ENTROU NO SEGUNDO IF DO LUCRO (POST)");
                    debug("preco de venda:%lg retorno:%lg", estimate->sell_price, tmp_ret_price);
                }
            } else if (tmp_ret_price < cut + p * (stock->trades.gmi[t_i].value / args->gmi_max)) {
                estimate->finalize_status = ESTIMATE_FINALIZE_ZERT;
                region = 2;
                debug("ENTROU NO TERCEIRO IF DO LUCRO (neg_lim<lucro<0)");

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

            debug("depois dos ifs de correcao do gain: sg=%lg", estimate->stop_gain);
        }

        // debug ("p(%ld)=%lg (ret_price:%lg) ff %lg REGION:%d finalize:%s", t, p, ff, 
        //    tmp_ret_price, region, (finalize == true ? "true" : "false"));
    }
    // debug ("FINALIZE = %d", finalize);
    return finalize;
}

static bool estimate_finalize_bender(struct estimate *estimate, struct args *args,
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

   // while (t_i < stock->trades.num && finalize != true) { 
        time = stock->trades.time[t_i];
        price = stock->trades.price[t_i];

        if (args->str_sma_p != NULL) 
        {
            if ((estimate->last_calc_p < 0) || (estimate->last_calc_p > 0 && time - estimate->last_calc_p > args->sma_p_update_time))
            {
               finalize = estimate_sma_p_bender(estimate, args, stock, t_i);
            }
        }

        if (finalize != true)
            finalize = estimate_finalize_status_bender (estimate, args, stock, price, time, &t_i);
       
        
	if (finalize != true) {
        	diff_time = time - time_ref;
    	if(args->candle_out_loss){
        	if (diff_time > args->candle_out_loss)
            	   time_ref = estimate_stop_loss_update_bender(estimate, args, stock,
                	time_ref, t_i);
    	}
    	else
    	{
		if (diff_time > args->candle)
            	   time_ref = estimate_stop_loss_update_bender(estimate, args, stock,
             		time_ref, t_i);
	}
    }	

 
/*
        else {
            *t_f = t_i;
        }
*/

        /* Esse caso protege caso haja corte por tempo de finalização de tempo
         * de operação no mesmo negócio do inicio. Isso pode ocorrer pois na
         * hora que acontece o corte por TIME, utiliza-se o negócio anterior
         * ao corte. */
        if (finalize == true && estimate->finalize_status == ESTIMATE_FINALIZE_TIME && t_i <= estimate->t_i_buy)
            return false;

    //}

    return finalize;
}

static volatile bool finalized = false;

void estimate_bender(struct args *args, struct stock *stock) 
{    
    struct estimate estimate;
    bool initialized = false;
    int i = 0;
    int daycount = 1;
    //printf("Entrou em estimate_bender \n");
    
    estimate_reset(&estimate);
    stock_read(args, stock);

    while ((args->daemon && !finalized) || (i < stock->trades.num)) {
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


/*
        if (args->print_trades)
            estimate_print_trade(args, stock, i);
*/

        if (!initialized) 
        {
            initialized = estimate_initialize_bender(&estimate, args, stock, i);
            if (initialized && !args->simulation)
                estimate_print(&estimate, args);
        } else 
        {
            if (estimate_finalize_bender(&estimate, args, stock, i )){ //, &i_f)) {
                estimate_print(&estimate, args);
                estimate_reset(&estimate);
                initialized = false;
            }
        }
}
        i++;
        
        if (args->daemon) 
        {
            while ((i == stock->trades.num) && !finalized) 
            {
                debug("ESTIMATE: Lendo (%d/%d)...", i, stock->trades.num);
                stock_read(args, stock);
                if (i == stock->trades.num) 
                {
                    debug("ESTIMATE: Dormindo (%d/%d)...", i, stock->trades.num);
                    sleep(1);
                }
            }
        }
    }
}
