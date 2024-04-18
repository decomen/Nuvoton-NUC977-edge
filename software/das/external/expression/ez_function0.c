
#include <stdlib.h>
#include <time.h>
#include "ez_function0.h"
#include "das_util.h"

static double _rand(void)
{
    static int _srand = 0;
    time_t n = time(0);
    if (_srand != n) {
        srand(n);
        _srand = n;
    }
    return (double)rand();
}

static double _reboot(void)
{
    return (double)system("reboot");
}

static void __localtime_r(struct tm *tm)
{
    time_t t = time(0);
    das_localtime_r(&t, tm);
}

static double _time(void)
{
    return (double)time(0);
}

static double _timezone(void)
{
    time_t t1 = time(0), t2 = 0;
    struct tm tm;
    das_localtime_r(&t1, &tm);
    t1 = mktime(&tm);
    gmtime_r(&t1, &tm);
    t2 = mktime(&tm);
    return (double)(t1 - t2);
}

static double _year(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_year + 1900);
}

static double _month(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_mon + 1);
}

static double _day(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_mday);
}

static double _hour(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_hour);
}

static double _minute(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_min);
}

static double _second(void)
{
    struct tm tm;
    __localtime_r(&tm);
    return (double)(tm.tm_sec);
}

static const function_0 token_to_functions_0[T_FUNC_MAX_TOKEN] = {
    [T_FUNC_RAND]       = _rand,
    [T_FUNC_REBOOT]     = _reboot,
    [T_FUNC_TIME]       = _time,
    [T_FUNC_TIMEZONE]   = _timezone,
    [T_FUNC_YEAR]       = _year,
    [T_FUNC_MONTH]      = _month,
    [T_FUNC_DAY]        = _day,
    [T_FUNC_HOUR]       = _hour,
    [T_FUNC_MINUTE]     = _minute,
    [T_FUNC_SECOND]     = _second,
};

bool is_function0_token(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN && token_to_functions_0[t] != NULL);
}

function_0 get_function0(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN ? token_to_functions_0[t] : NULL);
}


