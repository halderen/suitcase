/*
 * Copyright (c) 2022 A.W. van Halderen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "utilities.h"
#include "iso8601.h"

/* Implements basic handling of ISO8601 (repeating) periods
 *     https://en.wikipedia.org/wiki/ISO_8601
 *     https://www.iso.org/obp/ui/#iso:std:iso:8601:-1:ed-1:v1:en
 */

static void
normalizeperiod(struct tm* tm)
{
    int count;
    if(tm->tm_mday >= 0)
        tm->tm_mday -= 1;
    for(;;) {
        if(tm->tm_mon < 0) {
            count = (11 - tm->tm_mon) / 12;
            tm->tm_year -= count;
            tm->tm_mon += count * 12;
            continue;
        } else if(tm->tm_mon > 11) {
            count = tm->tm_mon / 12;
            tm->tm_year += count;
            tm->tm_mon -= count * 12;
            continue;
        }
        
        if(tm->tm_mday < 0) {
            if(tm->tm_mon == 0 || tm->tm_mon == 1 || tm->tm_mon == 3 || tm->tm_mon == 5 || tm->tm_mon == 7 || tm->tm_mon == 8 || tm->tm_mon == 10) {
                tm->tm_mday += 31;
                tm->tm_mon -= 1;
                continue;
            } else if(tm->tm_mon == 4 || tm->tm_mon == 6 || tm->tm_mon == 8 || tm->tm_mon == 9 || tm->tm_mon == 11) {
                tm->tm_mday += 30;
                tm->tm_mon -= 1;
                continue;
            } else if(tm->tm_mon == 2) {
                if(tm->tm_year % 4 == 0 && (tm->tm_year % 100 != 0 || tm->tm_year % 400 == 0)) {
                    tm->tm_mday += 29;
                    tm->tm_mon -= 1;
                    continue;
                } else {
                    tm->tm_mday += 28;
                    tm->tm_mon -= 1;
                    continue;
                }
            }
        } else if((tm->tm_mon == 0 || tm->tm_mon == 2 || tm->tm_mon == 4 || tm->tm_mon == 6 || tm->tm_mon == 7 || tm->tm_mon == 9 || tm->tm_mon == 11) && tm->tm_mday >= 31) {
            tm->tm_mday -= 31;
            tm->tm_mon += 1;
            continue;
        } else if((tm->tm_mon == 3 || tm->tm_mon == 5 || tm->tm_mon == 8 || tm->tm_mon == 10) && tm->tm_mday >= 30) {
            tm->tm_mday -= 30;
            tm->tm_mon += 1;
            continue;
        } else if(tm->tm_mon == 1) {
            if(tm->tm_year % 4 == 0 && (tm->tm_year % 100 != 0 || tm->tm_year % 400 == 0)) {
                if(tm->tm_mday >= 29) {
                    tm->tm_mday -= 29;
                    tm->tm_mon += 1;
                    continue;
                }
            } else {
                if(tm->tm_mday >= 28) {
                    tm->tm_mday -= 28;
                    tm->tm_mon += 1;
                    continue;
                }
            }
        }
        
        if(tm->tm_hour < 0) {
            count = (23 - tm->tm_hour) / 24;
            tm->tm_mday -= count;
            tm->tm_hour += count * 24;
            continue;
        } else if(tm->tm_hour >= 24) {
            count = tm->tm_hour / 24;
            tm->tm_mday += count;
            tm->tm_hour -= count * 24;
            continue;
        }
        
        if(tm->tm_min < 0) {
            count = (59 - tm->tm_min) / 60;
            tm->tm_hour -= count;
            tm->tm_min += count * 60;
            continue;
        } else if (tm->tm_min >= 60) {
            count = tm->tm_min / 60;
            tm->tm_hour += count;
            tm->tm_min -= count * 60;
            continue;
        }
        
        if(tm->tm_sec < 0) {
            count = (59 - tm->tm_sec) / 60;
            tm->tm_min -= count;
            tm->tm_sec += count * 60;
            continue;
        } else if (tm->tm_sec >= 60) {
            count = tm->tm_sec / 60;
            tm->tm_min += count;
            tm->tm_sec -= count * 60;
            continue;
        }

        break;
    }
    tm->tm_mday += 1;
}

static char*
parseperiod(char* s, struct tm* tm)
{
    int value = 0;
    char lastUnit = '\0';
    char lastChar = '\0';
    memset(tm, 0, sizeof(struct tm));
    for(; *s; s++) {
        if(isdigit(*s)) {
            value = value * 10 + (*s - '0');
            lastChar = *s;
        } else {
            switch(toupper(*s)) {
                case 'R':
                case '/':
                case 'P':
                case 'T':
                    switch(lastUnit) {
                        case '\0':
                        case '/':
                        case 'P':
                        case 'Y':
                        case 'M':
                     case 'D':
                            break;
                        case 'R':
                        case 'T':
                        case 'H':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                case 'Y':
                    switch(lastUnit) {
                        case '\0':
                        case '/':
                        case 'P':
                            if(isdigit(lastChar))
                                tm->tm_year = value;
                            else
                                return "missing value";
                            break;
                        case 'Y':
                        case 'R':
                        case 'T':
                        case 'H':
                        case 'M':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                case 'M':
                    switch(lastUnit) {
                        case '\0':
                        case '/':
                        case 'P':
                        case 'Y':
                            if(isdigit(lastChar))
                                tm->tm_mon = value;
                            else
                                return "missing value";
                            break;
                        case 'T':
                        case 'H':
                            if(isdigit(lastChar))
                                tm->tm_min = value;
                            else
                                return "missing value";
                            break;
                        case 'R':
                        case 'M':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                case 'D':
                    switch(lastUnit) {
                        case '\0':
                        case '/':
                        case 'P':
                        case 'Y':
                            if(isdigit(lastChar))
                                tm->tm_mday = value;
                            else
                                return "missing value";
                            break;
                        case 'R':
                        case 'T':
                        case 'H':
                        case 'M':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                case 'H':
                    switch(lastUnit) {
                        case 'T':
                            if(isdigit(lastChar))
                                tm->tm_hour = value;
                            else
                                return "missing value";
                            break;
                        case '\0':
                        case '/':
                        case 'P':
                        case 'Y':
                        case 'R':
                        case 'H':
                        case 'M':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                case 'S':
                    switch(lastUnit) {
                        case 'T':
                        case 'H':
                        case 'M':
                            if(isdigit(lastChar))
                                tm->tm_sec = value;
                            else
                                return "missing value";
                            break;
                        case '\0':
                        case '/':
                        case 'P':
                        case 'Y':
                        case 'R':
                        case 'S':
                        default:
                            return "bad";
                    }
                    break;
                default:
                    return "bad";
            }
            value = 0;
            lastChar = lastUnit = toupper(*s);
        }
    }
    return NULL;
}

static char*
periodstring(struct tm* tm)
{
    char* s;
    if(tm->tm_year == 0 && tm->tm_mon == 0 && tm->tm_mday == 0 &&
       tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0 &&
       tm->tm_mday % 7 == 0) {
        asprintf(&s, "P%dW", tm->tm_mday / 7);
    } else {
        asprintf(&s, "P%d%s%d%s%d%s%s%d%s%d%s%d%s",
                tm->tm_year, (tm->tm_year>0?"Y":""),
                tm->tm_mon,  (tm->tm_mon>0?"M":""),
                tm->tm_mday, (tm->tm_mday>0?"D":""),
                (tm->tm_hour != 0 || tm->tm_min != 0 || tm->tm_sec != 0 ? "T" : ""),
                tm->tm_hour, (tm->tm_hour>0?"H":""),
                tm->tm_min,  (tm->tm_min>0?"M":""),
                tm->tm_sec,  (tm->tm_sec>0?"S":""));
        /* next loop will remove leading zeros to multiple numbers in the string */
        int i = 0, p = 0;
        while(s[i]) {
            if(isdigit(s[i])) {
                if(s[i]!='0') {
                    do {
                        s[p++] = s[i++];
                    } while(isdigit(s[i]));
                } else
                    ++i;
            } else
                s[i++] = s[p++];
        }
        alloc(s, 1, NULL, strlen(s)+1);
    }
    return s;
}

time_t
intrvl_add(time_t t, char* period)
{
    struct tm t1;
    struct tm t2;
    char* err;
    localtime_r(&t, &t1);
    normalizeperiod(&t1);
    err = parseperiod(period, &t2);
    if(err)
        fprintf(stderr,"ERR %s\n",err);
    printf("%d-%d-%d %d:%d:%d\n",t2.tm_year,t2.tm_mon,t2.tm_mday,t2.tm_hour,t2.tm_min,t2.tm_sec);
    t1.tm_year += t2.tm_year;
    t1.tm_mon  += t2.tm_mon;
    t1.tm_mday += t2.tm_mday;
    t1.tm_hour += t2.tm_hour;
    t1.tm_min  += t2.tm_min;
    t1.tm_sec  += t2.tm_sec;
    normalizeperiod(&t1);
    t = mktime(&t1);
    return t;
}

char*
intrvl_concat(char* period1, char* period2)
{
    struct tm t1;
    struct tm t2;
    parseperiod(period1, &t1);
    parseperiod(period2, &t2);
    t1.tm_year += t2.tm_year;
    t1.tm_mon  += t2.tm_mon;
    t1.tm_mday += t2.tm_mday;
    t1.tm_hour += t2.tm_hour;
    t1.tm_min  += t2.tm_min;
    t1.tm_sec  += t2.tm_sec;
    return periodstring(&t1);
}

int
intrvl_verify(char* period)
{
    char* err;
    struct tm t;
    err = parseperiod(period, &t);
    if(err)
        return -1;
    else
        return 0;
}
// The smallest value used may also have a decimal fraction
// tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0
// PYYYYMMDDThhmmss or in the extended format P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss
