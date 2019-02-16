#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// the database time used in WebWSPR references to the year 2010

time_t t2010 = 1262300400;  // senconds of year 2010

unsigned long convertLinuxtimeToDBtime(time_t t)
{
    return t - t2010;  // seconds since 2010
}

unsigned long getActTime()
{
    time_t currt = time(NULL);  // seconds since 1970
    return convertLinuxtimeToDBtime(currt);
}

unsigned long getMonthStartTime()
{
    time_t currt = time(NULL);
    struct tm *t = gmtime(&currt);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    t->tm_mday = 1;
    time_t tt = timegm(t);
    return convertLinuxtimeToDBtime(tt);
}

// substract diff_h from actual time
unsigned long getSTime(int diff_h)
{
    time_t currt = time(NULL);
    unsigned long dbt = convertLinuxtimeToDBtime(currt);
    dbt -= (((unsigned long)diff_h)*3600);
    return dbt;
}

// print database time
char *printDBtime(unsigned long t)
{
static char s[256];

    time_t tt = (time_t)t + t2010;
    struct tm *tm = gmtime(&tt);
    snprintf(s,sizeof(s),"now: %d-%d-%d %d:%d:%d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return s;
}
