#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <error.h>
#include <argp.h>
#include <unistd.h>
#include <string.h>
#include "args.h"
#include "err.h"
#include "misc.h"
#include "candle.h"
#include "estimate.h"
#include "estimate_bender.h"
#include "estimate_quarks.h"
//#include "distributions.h"

static void args_str_sma_p(struct args *);
static void args_ma_values(struct args *);

/* Keys for options. */
#define OPT_STOCK_CODE       's'
#define OPT_MA               1
#define OPT_CANDLE           2
#define OPT_CANDLE_OUT_GAIN  3
#define OPT_DATA_DIR         4
#define OPT_PRINT_TRADES     5
#define OPT_CANDLE_OUT_LOSS  6
#define OPT_ROCKET           7
#define OPT_BENDER           8
#define OPT_QUARKS           9
#define OPT_SMA_SG           11
#define OPT_SMA_MIN_SL       12
#define OPT_SMA_MAX_SL       13
#define OPT_SMA_WGAP         15
#define OPT_SMA_TRADE_START  16
#define OPT_SMA_TRADE_STOP   17
#define OPT_SMA_P            18
#define OPT_SMA_P_UPD_TIME   19
#define OPT_T_INIT           22
#define OPT_T_MAX            23
#define OPT_MIN_RET          24
#define OPT_AMP_FACTOR       25
#define OPT_MIN_FACTOR       26
#define OPT_TIME_FACTOR      27
#define OPT_TIME_FACTOR_MIN  28
#define OPT_MARGIN1          29
#define OPT_MARGIN2          30
#define OPT_CUT              31
#define OPT_TIME_FACTOR_BETA  32
#define OPT_TIME_FACTOR_GAMMA 33
#define OPT_BETA_MIN         34

#define OPT_DIR              36
#define OPT_BLOCK_TIME       37
#define OPT_PROFILE          38
// comandos exclusivos do bender e quarks
#define OPT_INF_BAND_LIM     39
#define OPT_SUP_BAND_LIM     40
#define OPT_PERC_BAND_LIM    41
#define OPT_GMI_MAX          43
#define OPT_NO_TRADES        48
#define OPT_NEG_LIM          49

#define OPT_REPLACE_DATA    539
#define OPT_INPUT_FORMAT    540
#define OPT_SIMULATION      541
#define OPT_DAEMON          542

#define OPT_GMI_TFRAME      600
#define OPT_GMI_AVG_DAYS    601
#define OPT_GMI_MIN         602
#define OPT_GMI_FACTOR      603
#define OPT_STOP_CLONE	    604
#define OPT_DATE_START      605
#define OPT_DATE_FINISH     606
#ifndef NDEBUG
# define OPT_DEBUG          'x'
# define OPT_SDEBUG         'y'
#endif

static char *str_dirs[] =
{ "long", "short", "all" };

/* Understood command line global options.
 * All options must have long-option because of input file. */
struct argp_option options[] =
		{
		{ 0, 0, 0, 0, "Program options:", 1 },
		{ "data-dir", OPT_DATA_DIR, "DIR", 0, "Data output directory.\n" },
		{ "stock", OPT_STOCK_CODE, "CODE", 0, "Stock code.\n" },
		{ "profile", OPT_PROFILE, "CODE", 0, "Profile name.\n" },
                { "rocket", OPT_ROCKET, 0, 0, "Choose the robot rocket.\n" },
                { "bender", OPT_BENDER, 0, 0, "Choose the robot bender.\n" },
                { "quarks", OPT_QUARKS, 0, 0, "Choose the robot quarks.\n" },
				{ "ma", OPT_MA, "INT[,INT[,...]]", 0,
						"Moving average lenght in seconds.\n" },
				{ "candle", OPT_CANDLE, "INT", 0, "Candle size (in seconds).\n" },
				{ "candle-out-gain", OPT_CANDLE_OUT_GAIN, "INT", 0, 
					"Tamanho em segundos do candle usado para atualizar o valor de stop gain.\n" },	
				{ "candle-out-loss", OPT_CANDLE_OUT_LOSS, "INT", 0, 
					"Tamanho em segundos do candle usado para atualizar o valor de stop loss.\n" },	
				{ "print-trades", OPT_PRINT_TRADES, 0, 0,
						"Print all trades data file.\n" },
				{ "replace-data", OPT_REPLACE_DATA, 0, 0,
						"Replace data files instead of append.\n" },
				{ "simulation", OPT_SIMULATION, 0, 0,
						"Enable simulation mode (if not defined, production is used).\n" },
                                { "no-trades", OPT_NO_TRADES, 0, 0, "Don't print all_trades.dat\n" },
				{ "daemon", OPT_DAEMON, 0, 0,
						"Run in background (as Linux daemon).\n" },
#ifndef NDEBUG
				{ "debug", OPT_DEBUG, "FILE", OPTION_ARG_OPTIONAL,
						"Print debug messages\n" },
                                { "sdebug", OPT_SDEBUG, "FILE", OPTION_ARG_OPTIONAL,
						"Print sdebug (strategic debug) messages.\n" },                             
				{ "sma-sg", OPT_SMA_SG, "STOP GAIN", 0,
						"Valor de referência inicial. Só é estritamente válido antes "
                                                "de transcorrerem t-init segundos. Após t-init segundos, o valor de stop gain "
                                                "ganha um fator multiplicativo, calculado pela função p.\n " },
				{ "sma-min-sl", OPT_SMA_MIN_SL, "STOP LOSS", 0,
						"Valor mínimo para o stop loss. O valor de stop loss é calculado "
                                                "no início da operação, com base em alguns dados de mercado de momento. Porém, "
                                                "se o valor calculado for menor que sma-min-sl, o valor usado será sma-min-sl.\n " },
				{ "sma-max-sl", OPT_SMA_MAX_SL, "STOP LOSS", 0,
						"Complemento do sma-min-sl, este é o limite máximo para o stop loss.\n " },
				{ "sma-wgap", OPT_SMA_WGAP, "STOP LOSS", 0,
						"Controle de stop loss móvel. Por exemplo, digamos que o primeiro stop seja s1. Isto "
                                                "signifca que com -s1 pontos de perda a operação é finalizada. Com um stop móvel em "
                                                "funcionamento,  um novo stop s2 é calculado. O comportamento normal seria: se s2 < s1, "
                                                "move o stop para s2. Porém, ainda é feita uma nova comparação: se s2 < wgap, o stop é "
                                                "MANTIDO em s1. Deve-se tomar cuidado com os sinais, mas se wgap for escolhido positivo, "
                                                "por exemplo 250, o stop só será movido quando representar um GANHO de 250 pontos. "
                                                "Naturalmente, isto só irá ocorrer para operações que já acumularam um lucro razoavelmente "
                                                "MAIOR que 250.\n " },
				{ "sma-trade-start", OPT_SMA_TRADE_START, "HHMMSS", 0,
						"Hora inicial a partir da qual operações podem ser iniciadas. Deve-se lembrar "
                                                "que todas as médias e candles também já devem ter sido preenchidos.\n" },                       
				{ "sma-trade-stop", OPT_SMA_TRADE_STOP, "HHMMSS", 0,
  					        "Hora onde todas as operações em aberto são encerradas.\n" },                                             
				{ "sma-p", OPT_SMA_P, "A,B,C", 0, 
                                                "SMA P Function parameters.\n" },                                             
				{ "sma-p-upd-time", OPT_SMA_P_UPD_TIME, "SECS", 0,
						"Tempo de update da função P, ou seja, de quantos em quantos segundos esta função é "
                                                "calculada. (DEFAULT: 60)\n" },
				{ "t-init", OPT_T_INIT, "DOUBLE", 0,
						"Tempo minimo para uso da funcao P, antes deste tempo a função não é calculada e os stops"
                                                " não são re-avaliados. A função P é menor igual a zero para t < 1/C, e portanto mesmo que "
                                                "t-init seja zero a função não é calculada para tempos menores que 1/C.\n" },
				{ "t-max", OPT_T_MAX, "DOUBLE", 0,
						"Tempo de espera que uma vez superado diminui a expectativa de ganho em operações no modo NEGT.\n " },
				{ "min-ret", OPT_MIN_RET, "DOUBLE", 0,
						"Fator multiplicativo que pode ser aplicado ao retorno esperado de P.\n " },
				{ "amp-factor", OPT_AMP_FACTOR, "DOUBLE", 0,
						"Limitador superior do fator multiplicativo do stop gain. Nas operações em que o retorno é maior "
                                                "do que P, o stop gain ganha um fator multiplicativo. Este amp-factor limita este fator. Deve-se "
                                                "notar que o retorno da operação não é seu retorno instantâneo, mas sim seu retorno médio, calculado "
                                                "num intervalo de tempo de um candle.\n " },
				{ "min-factor", OPT_MIN_FACTOR, "DOUBLE", 0,
						"Stop gain - Um dos parâmetros ao qual o algoritmo é mais sensível. Este valor é a referência de stop"
                                                " gain quando a operação está nos modos POST e ZERT. Nestes casos, a estratégia avalia o retorno médio "
                                                "no intervalo de tempo do último candle. Então ela define inicialmente o novo stop gain como sendo o valor"
                                                " que gerará um retorno igual o retorno atual mais min_factor.\n " },
				{ "time-factor", OPT_TIME_FACTOR, "DOUBLE", 0,
						"Intervalo de tempo (em segundos) de referência, neste valor de intervalo de tempo WGAP já foi "
                                                "modificado para seu mínimo.\n " },
				{ "time-factor-min", OPT_TIME_FACTOR_MIN, "DOUBLE", 0,
						"valor minimo em segundos de um fator de atenuação de WGAP.\n " },
				{ "margin1", OPT_MARGIN1, "DOUBLE", 0,
						"Valores menores de stop gain dinâmico para operações ruins - modo NEGT.\n " },
				{ "margin2", OPT_MARGIN2, "DOUBLE", 0,
						"Valores menores de stop gain dinâmico para operações ruins - modo NEGT.\n " },
				{ "cut", OPT_CUT, "DOUBLE", 0,
						"Valor mínimo de perdas a patir do qual entra-se no modo NEGT.\n " },
                                                
                                { "inf-band-lim", OPT_INF_BAND_LIM, "DOUBLE", 0,
                                                "Valor, em pontos por mil, mínimo para a amplitude da banda.\n "},
                                { "sup-band-lim", OPT_SUP_BAND_LIM, "DOUBLE", 0,
                                                "Valor, em pontos por mil,  da  amplitude da banda para o qual muda o regime de entrada.\n "},
                                { "perc-band-lim", OPT_PERC_BAND_LIM, "DOUBLE", 0,
                                                "Valor, em pontos por mil, de quanto a banda deve ser superada para entrar na operação.\n "},
                                { "gmi-max", OPT_GMI_MAX  , " DOUBLE ", 0, 
                                                "Valor máximo de gmi.\n "},
                                { "neg-lim", OPT_NEG_LIM, "DOUBLE", 0,
                                                "Limite do fator multiplicativo das op negativas.\n "},
				{ "time-factor-beta", OPT_TIME_FACTOR_BETA, "DOUBLE", 0,
						"Valor de tempo a partir do qual começa a valer a redução de min_factor.\n " },
				{ "time-factor-gamma", OPT_TIME_FACTOR_GAMMA, "DOUBLE", 0,
						"Valor de tempo quando a redução de min_factor é máxima.\n " },
				{ "beta-min", OPT_BETA_MIN, "DOUBLE", 0,
						"Valor mínimo do multiplicador de min_factor.\n " },
                                { "gmi-min", OPT_GMI_MIN, "DOUBLE", 0, 
                                                "Valor mínimo do índice gmi.\n "},
				{ "gmi-tframe", OPT_GMI_TFRAME, "DOUBLE", 0,
						"Janela de tempo da média do gmi.\n " },
				{ "gmi-avg-days", OPT_GMI_AVG_DAYS, "DOUBLE ", 0,
						"Número de dias da normalização do gmi.\n " },
				{ "gmi-factor", OPT_GMI_FACTOR, "DOUBLE ", 0,
						"Fator de normalização do gmi.\n " },
				{ "dir", OPT_DIR, "INT", 0, 
                                                "0=long, 1=short, 2=ambas " },
				{ "stop-clone", OPT_STOP_CLONE, 0, 0,
						"Se a opção não é passada, initialize_clone não bloqueia por block_time\n" },
				{ "date-start", OPT_DATE_START, "YYYYMMDD", 0,
						"Data de ínicio. Qualquer trade anterior a essa data será ignorado. Se não for passado,"
						" lê o arquivo de dados desde seu início.\n" },
				{ "date-finish", OPT_DATE_FINISH, "YYYYMMDD", 0,
						"Data de fim. Qualquer trade posterior a essa data será ignorado. Se não for passado,"
						" lê o arquivo de dados até o seu final.\n" },	
				{ "block-time", OPT_BLOCK_TIME, " INT ", 0,
						"Intervalo de tempo, em minutos para bloqueio de abertura de operações. Operações não"
                                                "serão abertas a menos de block-time minutos  de sma-trade-stop  A,B,C - parâmetros da"
                                                "função p --> p(t) = (b*t + a) * 0.4343 * log (c*t);  Esta função dá uma espécie de ganho "
                                                "esperado da operação, que depende do tempo.  o fator antes de log é de conversão de base. "
                                                "Operações ganhando mais do que p(t) são as boas, etc. Ou seja, os diferentes status GAIN, "
                                                "POST, ZERT e NEGT usam p(t) como referência.\n " },
#endif
				{ 0, 0, 0, 0, "Help options:", -1 },

				{ 0 } };

const char *argp_program_version = "DOD "VERSION;

static char doc[] =
		"\DOD. \v\
SMA: Simple Moving Average\n\n\
SMA P FUNCTION: \n\
P(t) = (1.0e-6*A*t + 1.0e-2*B) * log (1.0e-2*C*t)\n";

/* A description of the arguments we accept. */
static char args_doc[] = "";

time_t datetime_to_unixtime(char *datetime)
{

	char year[4];
	char month[2];
	char day[2];
	struct tm t;
        time_t unixtime;
	debug("entrei datetime");
	strncpy(year,datetime,4);
	strncpy(month,&datetime[4],2);
	strncpy(day,&datetime[6],2);
	t.tm_year = atoi(year)-1900;
        t.tm_mon = atoi(month)-1; // Month, 0 - jan
        t.tm_mday = atoi(day); // Day of the month
        t.tm_hour = 0;
        t.tm_min = 0;
        t.tm_sec = 1;
        t.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
        unixtime = mktime(&t);
	return unixtime;
}


/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	 * know is a pointer to our arguments structure. */
	struct args *args = state->input;

	switch (key)
	{
	case OPT_STOCK_CODE:
		args->stock = arg;
		break;
	case OPT_MA:
		args->str_ma = arg;
		break;
	case OPT_CANDLE:
		args->candle = atoi(arg);
		break;
	case OPT_CANDLE_OUT_GAIN:
		args->candle_out_gain = atoi(arg);
		break;	
	case OPT_CANDLE_OUT_LOSS:
		args->candle_out_loss = atoi(arg);
		break;	
	case OPT_DATA_DIR:
		args->data_dir = arg;
		break;
	case OPT_PROFILE:
		args->profile = arg;
		break;
	case OPT_PRINT_TRADES:
		args->print_trades = true;
		break;
	case OPT_REPLACE_DATA:
		args->replace_data = true;
		break;
	case OPT_SIMULATION:
		args->simulation = true;
		break;
	case OPT_DAEMON:
		args->daemon = true;
		break;
        case OPT_ROCKET:
                args->rocket = true;
                break;     
        case OPT_BENDER:
                args->bender = true;
                break;    
        case OPT_QUARKS:
                args->quarks = true;
                break;
	case OPT_STOP_CLONE:
                args->stop_clone = true;
                break;    
	case OPT_DATE_START:
		args->date_start = datetime_to_unixtime(arg);
		break;
	case OPT_DATE_FINISH:
                args->date_finish = datetime_to_unixtime(arg);
		break;
        case OPT_NO_TRADES:
                args->print_trades = false;
                break;
	case OPT_SMA_SG:
		args->sma_stop_gain = atof(arg)/100;
		break;
	case OPT_SMA_MIN_SL:
		args->sma_min_stop_loss = atof(arg)/100;
		break;
	case OPT_SMA_MAX_SL:
		args->sma_max_stop_loss = atof(arg)/100;
		break;
	case OPT_SMA_WGAP:
		args->sma_wgap = atof(arg)/100;
		break;
	case OPT_SMA_TRADE_START:
		args->sma_trade_start = arg;
		break;
	case OPT_SMA_TRADE_STOP:
		args->sma_trade_stop = arg;
		break;
	case OPT_SMA_P:
		args->str_sma_p = arg;
		break;
	case OPT_SMA_P_UPD_TIME:
		args->sma_p_update_time = atoi(arg);
		break;
	case OPT_T_INIT:
		args->t_init = atof(arg);
		break;
	case OPT_T_MAX:
		args->t_max = atof(arg);
		break;
	case OPT_MIN_RET:
		args->min_ret = atof(arg);
		break;
	case OPT_AMP_FACTOR:
		args->amp_factor = atof(arg);
		break;
	case OPT_MIN_FACTOR:
		args->min_factor = atof(arg)/100;
		break;
	case OPT_TIME_FACTOR:
		args->time_factor = atof(arg);
		break;
	case OPT_TIME_FACTOR_MIN:
		args->time_factor_min = atof(arg);
		break;
	case OPT_MARGIN1:
		args->margin1 = atof(arg)/100;
		break;
	case OPT_MARGIN2:
		args->margin2 = atof(arg)/100;
		break;
	case OPT_CUT:
		args->cut = atof(arg)/100;
		break;          
        case OPT_INF_BAND_LIM:
                args->inf_band_lim = atof (arg);
                break;
        case OPT_SUP_BAND_LIM:
                args->sup_band_lim = atof (arg);
                break;
        case OPT_PERC_BAND_LIM:
                args->perc_band_lim = atof (arg);
                break;  
        case OPT_GMI_MAX:
                args->gmi_max = atof (arg);
                break;   
        case OPT_NEG_LIM:
                args->neg_lim = atof (arg);
                break;
	case OPT_TIME_FACTOR_BETA:
		args->time_factor_beta = atof(arg);
		break;
	case OPT_TIME_FACTOR_GAMMA:
		args->time_factor_gamma = atof(arg);
		break;
	case OPT_BETA_MIN:
		args->beta_min = atof(arg);
		break;
	case OPT_GMI_MIN:
		args->gmi_min = atof(arg);
		break;
	case OPT_GMI_TFRAME:
		args->gmi_tframe = atoi(arg);
		break;
	case OPT_GMI_AVG_DAYS:
		args->gmi_avg_days = atoi(arg);
		break;
	case OPT_GMI_FACTOR:
		args->gmi_factor = atof(arg);
		break;
	case OPT_DIR:
		args->dir = atoi(arg);
		args->dir_name = str_dirs[args->dir];
		break;
	case OPT_BLOCK_TIME:
		args->block_time = atoi(arg);
		break;
#ifndef NDEBUG
	case OPT_DEBUG:
		if (arg != NULL )
			args->debug_file = arg;                   
		args->debug = true;
                
	case OPT_SDEBUG:
		if (arg != NULL )
                        args->sdebug_file = arg;     
                args->sdebug = true;
		break;
#endif
	default:
		return ARGP_ERR_UNKNOWN;
		break;
	}

	return 0;
}

/* Argp parser. */
static struct argp argp =
{ options, parse_opt, args_doc, doc };

static int args_parse(int *argc, char ***argv, struct args *args)
{
	argp_parse(&argp, *argc, *argv, 0, 0, args);

	if (args->str_ma != NULL )
		args_ma_values(args);

	if (args->str_sma_p != NULL )
		args_str_sma_p(args);

	if (args->stock == NULL )
		error(EXIT_FAILURE, 0, "Missing %c argument", "stock");

	return 0;
}

static void args_init(struct args *args)
{
	/* Program options */
	args->data_dir = DEFAULT_DATA_DIR;
	args->stock = NULL;
	args->str_ma = NULL;
	args->ma_num = 0;
	args->candle = -1;
	args->candle_out_gain = 0;
	args->candle_out_loss = 0;
	args->estimate = true;  
	args->ma = NULL; 
        
        args->rocket = false;
        args->bender = false;
        args->quarks = false;
        args->print_trades = false;
        args->inf_band_lim = 0.75;
        args->sup_band_lim = 3.0;
        args->perc_band_lim = 1.5;
        args->gmi_max = 0.05;
        args->neg_lim = 0.4;
    
	args->replace_data = false;
	args->simulation = false;
	args->daemon = false;
	args->sma_stop_gain = -1.0;
	args->sma_min_stop_loss = -1.0;
	args->sma_max_stop_loss = -1.0;
	args->sma_wgap = 0.0;
	args->sma_trade_start = NULL;
	args->sma_trade_stop = NULL;
	args->str_sma_p = NULL;
	args->sma_p_a = 0.0;
	args->sma_p_b = 0.0;
	args->sma_p_c = 0.0;
	args->sma_p_update_time = 1;
	args->t_init = 100.0;
	args->t_max = 200.0;
	args->min_ret = 0.50;
	args->amp_factor = 2.0;
	args->min_factor = 0.1;
	args->time_factor = 3600;
	args->time_factor_min = 0.3;
	args->cut = 20.0;
	args->margin1 = 20.0;
	args->margin2 = 20.0;
	args->time_factor_beta = 1200;
	args->time_factor_gamma = 4500;
	args->beta_min = 0.2;
	args->gmi_tframe = 0;
	args->gmi_avg_days = 0;
	args->gmi_min = 0.1;
	args->gmi_factor = 0.0;
	args->dir = 2;
	args->block_time = 30;
	args->stop_clone = false;
	args->date_start = 1;
	args->date_finish = 9999999999;
#ifndef NDEBUG
	args->debug = false;
        args->sdebug = false;
	args->debug_file = NULL;
        args->sdebug_file = NULL;
#endif
}

struct args *
args_create(int *argc, char ***argv)
{
	struct args *args = err_malloc (sizeof (*args));

	args_init(args);
	args_parse(argc, argv, args);

	return args;
}

void args_destroy(struct args *args)
{
	free(args->ma);
	free(args);
}

static void args_ma_values(struct args *args)
{
	int num = 0, i;
	char *tmp = args->str_ma;
	char **p;
	ssize_t *plen;

	/* Contando o número de médias móveis. */
	while (tmp != NULL )
	{
		tmp = strchr(tmp, ',');
		num++;

		if (tmp != NULL )
			tmp++;
	}

	p = err_malloc (num * sizeof (*p));
	plen = err_malloc (num * sizeof (*plen));

	strnsplittrim(args->str_ma, strlen(args->str_ma), p, plen, ',', num);

	for (i = 0; i < num; i++)
		p[i][plen[i]] = '\0';

	args->ma = err_malloc (num * sizeof (*args->ma));

	for (i = 0; i < num; i++)
	{
        args->ma[i] = atoilen (p[i], plen[i]);
        debug ("ma %d = %d seconds", i, args->ma[i]);
        }

	args->ma_num = num;

	free(p);
	free(plen);
}

static void args_str_sma_p(struct args *args)
{
	int num = 0, i;
	char *tmp = args->str_sma_p;
	char **p;
	ssize_t *plen;

	/* Contando o número de médias móveis. */
	while (tmp != NULL )
	{
		tmp = strchr(tmp, ',');
		num++;

		if (tmp != NULL )
			tmp++;
	}

	p = err_malloc (num * sizeof (*p));
	plen = err_malloc (num * sizeof (*plen));

	strnsplittrim(args->str_sma_p, strlen(args->str_sma_p), p, plen, ',', num);

	for (i = 0; i < num; i++)
		p[i][plen[i]] = '\0';

	if (num < 3)
		error(EXIT_FAILURE, 0, "Missing SMA P Function parameters (%d)", num);

        args->sma_p_a = atoflen (p[0], plen[0]);
        debug ("SMA P: A = %lg", args->sma_p_a);
        args->sma_p_b = atoflen (p[1], plen[1]);
        debug ("SMA P: B = %lg", args->sma_p_b);
        args->sma_p_c = atoflen (p[2], plen[2]);
        debug ("SMA P: C = %lg", args->sma_p_c);

	free(p);
	free(plen);
}
