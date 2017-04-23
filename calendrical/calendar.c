/*
 Copyright (c) 2015 Jeremy Nixon. All rights reserved.

 Developed by: Jeremy Nixon

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal with the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimers.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimers in the documentation
 and/or other materials provided with the distribution.

 Neither the names of the developer nor the names of its contributors may
 be used to endorse or promote products derived from this Software without
 specific prior written permission.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS WITH THE SOFTWARE.
 */
#include <stddef.h>
#include <math.h>
#include <time.h>
#include "moonphase.h"
#include "calendar.h"

#define MEAN_TROPICAL_YEAR 365.242189
#define MEAN_SYNODIC_MONTH 29.530588853

#define LONGITUDE_SPRING 0
#define LONGITUDE_SUMMER 90
#define LONGITUDE_AUTUMN 180
#define LONGITUDE_WINTER 270

#define EPOCH_JD -1721424.5
#define EPOCH_MJD 678576
#define EPOCH_UNIXTIME 719163
#define EPOCH_HEBREW -1373427
#define EPOCH_CHINESE -963099
#define EPOCH_ISLAMIC 227015
#define EPOCH_MAYAN -1137142
#define EPOCH_MAYAN_HAAB -1137490
#define EPOCH_MAYAN_TZOLKIN -1137301

#pragma mark Math

#undef oddp
#define oddp(n) ((n) % 2)

#undef angle
#define angle(d, m, s) (d + (m + s / 60.0) / 60.0)

/* The standard fmod() function behaves differently for negative numbers,
   so we define this. */
static double mod(double x, double y)
{
    return x - y * floor(x / y);
}

static double amod(double x, double y)
{
    return y + mod(x,-y);
}

static double poly(double x, int argc, double a[])
{
    double result = a[0];
    for (int i = 1; i < argc; i++) {
        result += a[i] * pow(x,i);
    }
    return result;
}

static double deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

static int signum(double num)
{
    if (num < 0) return -1;
    if (num > 0) return 1;
    return 0;
}

#pragma mark Basics

int fixed_from_struct_tm(struct tm *time)
{
    return fixed_from_gregorian(time->tm_year+1900,time->tm_mon+1,time->tm_mday);
}

int fixed_from_unixtime(time_t time)
{
    return floor(time / 86400) + EPOCH_UNIXTIME;
}

double moment_from_unixtime(time_t time)
{
    return (time / 86400) + EPOCH_UNIXTIME;
}

/* Sunday is 0 */
int day_of_week_from_fixed(int date)
{
    return (int)mod(date,7);
}

int kday_on_or_before(int date, int k)
{
    return date - mod(date - k, 7);
}

int kday_nearest(int date, int k)
{
    return kday_on_or_before(date + 3, k);
}

int kday_on_or_after(int date, int k)
{
    return kday_on_or_before(date + 6, k);
}

int kday_before(int date, int k)
{
    return kday_on_or_before(date - 1, k);
}

int kday_after(int date, int k)
{
    return kday_on_or_before(date + 7, k);
}

/* Sunday is 0. */
int nth_kday(int n, int k, int year, int month, int day)
{
    if (n > 0)
        return 7 * n + kday_before(fixed_from_gregorian(year,month,day),k);

    return 7 * n + kday_after(fixed_from_gregorian(year,month,day),k);
}

/* Negative values of n count back from the end of the month. */
int nth_kday_in_month(int n, int k, int year, int month)
{
    if (n > 0)
        return kday_on_or_before(fixed_from_gregorian(year,month,7), k)
            + (7 * (n - 1));
    return
        kday_on_or_before(fixed_from_gregorian(year,month,
            last_day_of_gregorian_month(month, year)),k) + (7 * (1 + n));
}

double moment_from_jd(double jd)
{
    return jd + EPOCH_JD;
}

double jd_from_moment(double t)
{
    return t - EPOCH_JD;
}

int fixed_from_jd(double jd)
{
    return floor(jd + EPOCH_JD);
}

double jd_from_fixed(int date)
{
    return jd_from_moment((double)date);
}

int fixed_from_mjd(int mjd)
{
    return mjd + EPOCH_MJD;
}

int mjd_from_fixed(int date)
{
    return date - EPOCH_MJD;
}

#pragma mark Gregorian

bool gregorian_leap_year(int year)
{
    return (
        mod(year, 4) == 0 &&
        !(mod(year, 400) == 100 ||
          mod(year, 400) == 200 ||
          mod(year, 400) == 300)) ? true : false;
}

int last_day_of_gregorian_month(int month, int year)
{
    if (month == 2 && gregorian_leap_year(year))
        return 29;
    switch (month) {
        case  1: return 31;
        case  2: return 28;
        case  3: return 31;
        case  4: return 30;
        case  5: return 31;
        case  6: return 30;
        case  7: return 31;
        case  8: return 31;
        case  9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

int fixed_from_gregorian(int year, int month, int day)
{
    int correction,f;
    if (month <= 2) correction = 0;
    else if (month > 2 && gregorian_leap_year(year)) correction = -1;
    else correction = -2;

    f = 365 * (year - 1) +
        floor((year - 1) / 4.0) -
        floor((year - 1) / 100.0) +
        floor((year - 1) / 400.0) +
        floor((367 * month - 362) / 12.0) +
        correction + day;
    return f;
}

int gregorian_year_from_fixed(int date)
{
    int d0 = date - 1;
    int n400 = (int)floor(d0 / 146097.0);
    int d1 = mod(d0, 146097);
    int n100 = (int)floor(d1 / 36524.0);
    int d2 = mod(d1, 36524);
    int n4 = (int)floor(d2 / 1461.0);
    int d3 = mod(d2, 1461);
    int n1 = (int)floor(d3 / 365.0);
    int year = 400 * n400 + 100 * n100 + 4 * n4 + n1;
    if (n100 == 4 || n1 == 4) return year;
    return year + 1;
}

void gregorian_from_fixed(int date, int *ryear, int *rmonth, int *rday)
{
    int year = gregorian_year_from_fixed(date);
    int prior_days = date - fixed_from_gregorian(year,1,1);

    int correction;
    if (date < fixed_from_gregorian(year,3,1))
        correction = 0;
    else if (gregorian_leap_year(year))
        correction = 1;
    else
        correction = 2;

    int month = floor((12 * (prior_days + correction) + 373) / 367.0);
    int day = date - fixed_from_gregorian(year,month,1) + 1;
    if (ryear) *ryear = year;
    if (rmonth) *rmonth = month;
    if (rday) *rday = day;
}

#pragma mark Julian

bool julian_leap_year(int year)
{
    return (year != 4 && mod(year, 4) == 0) ? true : false;
}

int last_day_of_julian_month(int month, int year)
{
    if (month == 2 && julian_leap_year(year))
        return 29;
    switch (month) {
        case  1: return 31;
        case  2: return 28;
        case  3: return 31;
        case  4: return 30;
        case  5: return 31;
        case  6: return 30;
        case  7: return 31;
        case  8: return 31;
        case  9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

int fixed_from_julian(int year, int month, int day)
{
    int correction,y,f;
    if (month <= 2) correction = 0;
    else if (julian_leap_year(year)) correction = -1;
    else correction = -2;

    if (year < 0) y = year + 1;
    else y = year;

    f = -2 + (365 * (y - 1)) +
        floor((y - 1) / 4.0) +
        floor((367 * month - 362) / 12.0) +
        correction + day;
    return f;
}

void julian_from_fixed(int date, int *ryear, int *rmonth, int *rday)
{
    int approx = floor((4 * (date + 1) + 1464) / 1461.0);
    int year = (approx <= 0 ? approx - 1 : approx);
    int prior_days = date - fixed_from_julian(year, 1, 1);

    int correction;
    if (date < fixed_from_julian(year, 3, 1)) correction = 0;
    else if (julian_leap_year(year)) correction = 1;
    else correction = 2;

    int month = floor((12 * (prior_days + correction) + 373) / 367.0);
    int day = date - fixed_from_julian(year,month,1) + 1;
    if (ryear) *ryear = year;
    if (rmonth) *rmonth = month;
    if (rday) *rday = day;
}

#pragma mark Islamic

bool islamic_leap_year(int year)
{
    return mod(14 + (11 * year), 30) < 11 ? true : false;
}

int last_day_of_islamic_month(int month, int year)
{
    return (oddp(month) ||
        ((month == 12) && islamic_leap_year(year))) ?
        30 : 29;
}

int fixed_from_islamic(int year, int month, int day) {
    return day + (29 * (month - 1)) + floor((6 * month - 1) / 11.0) +
            (year - 1) * 354 +
            floor((3 + (11 * year)) / 30.0) +
            EPOCH_ISLAMIC - 1;
}

void islamic_from_fixed(int date, int *ryear, int *rmonth, int *rday)
{
    int prior_days, year, month, day;
    year = (int)floor((30 * (date - EPOCH_ISLAMIC) + 10646) / 10631.0);
    prior_days = date - fixed_from_islamic(year,1,1);
    month = (int)floor((11 * prior_days + 330) / 325.0);
    day = date - fixed_from_islamic(year,month,1) + 1;

    if (ryear) *ryear = year;
    if (rmonth) *rmonth = month;
    if (rday) *rday = day;
}

#pragma mark Hebrew

bool hebrew_leap_year(int year)
{
    return (mod(1 + (7 * year), 19) < 7) ? true : false;
}

int last_month_of_hebrew_year(int year)
{
    return (hebrew_leap_year(year)) ? 13 : 12;
}

int last_day_of_hebrew_month(int month, int year)
{
    return
        ((month == 2 ||
          month == 4 ||
          month == 6 ||
          month == 10 ||
          month == 13) ||
         (month == 12 && !hebrew_leap_year(year)) ||
         (month == 8 && !long_marheshvan(year)) ||
         (month == 9 && short_kislev(year))) ?
         29 : 30;
}

int hebrew_calendar_elapsed_days(int year)
{
    int months_elapsed = floor((235 * year - 234) / 19.0);
    double parts_elapsed = 12084 + 13753 * months_elapsed;
    int day = 29 * months_elapsed + floor(parts_elapsed / 25920.0);
    return mod(3 * (day + 1), 7) < 3 ? day + 1 : day;
}

int hebrew_year_length_correction(int year)
{
    int ny0 = hebrew_calendar_elapsed_days(year - 1);
    int ny1 = hebrew_calendar_elapsed_days(year);
    int ny2 = hebrew_calendar_elapsed_days(year + 1);
    if (ny2 - ny1 == 356) return 2;
    if (ny1 - ny0 == 382) return 1;
    return 0;
}

int hebrew_new_year(int year)
{
    return EPOCH_HEBREW + hebrew_calendar_elapsed_days(year) +
                hebrew_year_length_correction(year);
}

int days_in_hebrew_year(int year)
{
    return hebrew_new_year(year + 1) - hebrew_new_year(year);
}

bool long_marheshvan(int year)
{
    int days = days_in_hebrew_year(year);
    return (days == 355 || days == 385) ? true : false;
}

bool short_kislev(int year)
{
    int days = days_in_hebrew_year(year);
    return (days == 353 || days == 383) ? true : false;
}

int fixed_from_hebrew(int year, int month, int day)
{
    int date = hebrew_new_year(year) + day - 1;
    if (month < 7) {
        int lm = last_month_of_hebrew_year(year);
        for (int i = 7; i <= lm; i++) {
            date += last_day_of_hebrew_month(i, year);
        }
        for (int i = 1; i < month; i++) {
            date += last_day_of_hebrew_month(i, year);
        }
    } else {
        for (int i = 7; i < month; i++) {
            date += last_day_of_hebrew_month(i, year);
        }
    }
    return date;
}

void hebrew_from_fixed(int date, int *ryear, int *rmonth, int *rday)
{
    int approx = 1 + floor((98496.0 / 35975351.0) * (date - EPOCH_HEBREW));
    int year = approx - 1;
    while (hebrew_new_year(year) <= date) year++;
    year--;

    int start = date < fixed_from_hebrew(year,1,1) ? 7 : 1;
    int month = start;
    while (date > fixed_from_hebrew(year,month,last_day_of_hebrew_month(month, year))) month++;

    int day = date + 1 - fixed_from_hebrew(year,month,1);

    if (ryear) *ryear = year;
    if (rmonth) *rmonth = month;
    if (rday) *rday = day;
}

int hebrew_birthday(int birth_month, int birth_day, int birth_year, int year)
{
    return
        (birth_month == last_month_of_hebrew_year(birth_year)) ?
        fixed_from_hebrew(year, last_month_of_hebrew_year(year), birth_day) :
        fixed_from_hebrew(year, birth_month, birth_day);
}

int yahrzeit(int death_month, int death_day, int death_year, int year)
{
    if (death_month == 8 &&
        death_day == 30 &&
        !long_marheshvan(1 + death_year))
            return fixed_from_hebrew(year, 9, 1) - 1;
    if (death_month == 9 &&
        death_day == 30 &&
        !short_kislev(1 + death_year))
            return fixed_from_hebrew(year, 10, 1) - 1;
    if (death_month == 13)
        return fixed_from_hebrew
            (year, last_month_of_hebrew_year(year), death_day);
    if (death_day == 30 &&
        death_month == 12 &&
        !hebrew_leap_year(year))
        return fixed_from_hebrew(year, 11, 30);
    return fixed_from_hebrew(year, death_month, death_day);
}

#pragma mark Christian Dates

int advent(int year)
{
    return kday_nearest(fixed_from_gregorian(year,11,30), 0);
//    return kday_on_or_before(fixed_from_gregorian(year,12,3), 0);
}

int eastern_orthodox_christmas(int year)
{
    int jan1, dec31, y, c1, c2;

    jan1 = fixed_from_gregorian(year,1,1);
    dec31 = fixed_from_gregorian(year,12,31);
    julian_from_fixed(jan1, &y, NULL, NULL);
    c1 = fixed_from_julian(y, 12, 25);
    c2 = fixed_from_julian((1 + y), 12, 25);
    if (jan1 <= c1 && c1 <= dec31)
        return c1;
    else if (jan1 <= c2 && c2 <= dec31)
        return c2;
    return 0;
}

int nicaean_rule_easter(int year)
{
    int shifted_epact, paschal_moon;

    shifted_epact = (int)mod(14 + (11 * mod(year, 19)), 30);
    paschal_moon = fixed_from_julian(year, 4, 19) - shifted_epact;
    return kday_on_or_before(paschal_moon + 7, 0);
}

int easter(int year)
{
    int century, shifted_epact, adjusted_epact, paschal_moon;

    century = 1 + floor(year / 100.0);
    shifted_epact = (int)mod(14 + (11 * mod(year, 19))
                  - floor((3 * century) / 4.0)
                  + floor((5 + (8 * century)) / 25.0)
                  + (30 * century),
                  30);
    adjusted_epact = ((shifted_epact == 0)
               || ((shifted_epact == 1) && (10 < mod(year, 19)))) ?
                1 + shifted_epact : shifted_epact;
    paschal_moon = fixed_from_gregorian(year,4,19)
        - adjusted_epact;
    return kday_on_or_before(paschal_moon + 7, 0);
}

/* Offset in days from Easter. Negative numbers are before Easter.
   Assume combined Julian/Gregorian calendar with 1582 reform, beause
   that's how Apple provides it. */
int easter_offset(int year, int month, int day)  // order of args should change.
{
    int e, fixed, off;

    e = easter(year);
    if (year > 1582) {
        fixed = fixed_from_gregorian(year,month,day);
    } else {
        fixed = fixed_from_julian(year,month,day);
    }
    off = fixed - e;

    return off;
}

#pragma mark Chinese

struct ChineseStem Stems[] = {
    { NULL, NULL, NULL, NULL },
    { "甲", "jiǎ",  "yang", "wood" },
    { "乙", "yǐ",   "yin",  "wood" },
    { "丙", "bǐng", "yang", "fire" },
    { "丁", "dīng", "yin",  "fire" },
    { "戊", "wù",   "yang", "earth" },
    { "己", "jǐ",   "yin",  "earth" },
    { "庚", "gēng", "yang", "metal" },
    { "辛", "xīn",  "yin",  "metal" },
    { "壬", "rén",  "yang", "water" },
    { "癸", "guǐ",  "yin",  "water" }
};

struct ChineseBranch Branches[] = {
    { NULL, NULL, NULL },
    { "子", "zǐ",   "Rat",     "鼠" },
    { "丑", "chǒu", "Ox",      "牛" },
    { "寅", "yín",  "Tiger",   "虎" },
    { "卯", "mǎo",  "Rabbit",  "兔" },
    { "辰", "chén", "Dragon",  "龍" },
    { "巳", "sì",   "Snake",   "蛇" },
    { "午", "wǔ",   "Horse",   "馬" },
    { "未", "wèi",  "Goat",    "羊" },
    { "申", "shēn", "Monkey",  "猴" },
    { "酉", "yǒu",  "Rooster", "雞" },
    { "戌", "xū",   "Dog",     "狗" },
    { "亥", "hài",  "Pig",     "豬" }
};

struct SolarTerm MajorSolarTerms[] = {
    { 0, -1, NULL, NULL, NULL },
    { 1,  330, "雨水", "yǔshuǐ",      "Rain Water" },
    { 2,  0,   "春分", "chūnfēn",     "Vernal Equinox" },
    { 3,  30,  "穀雨", "gǔyǔ",        "Grain Rains" },
    { 4,  60,  "小滿", "xiǎomǎn",     "Grain Full" },
    { 5,  90,  "夏至", "xiàzhì",      "Summer Solstice" },
    { 6,  120, "大暑", "dàshǔ",       "Major Heat" },
    { 7,  150, "處暑", "chùshǔ",      "Limit of Heat" },
    { 8,  180, "秋分", "qiūfēn",      "Autumnal Equinox" },
    { 9,  210, "霜降", "shuāngjiàng", "Descent of Frost" },
    { 10, 240, "小雪", "xiǎoxuě",     "Minor Snow" },
    { 11, 270, "冬至", "dōngzhì",     "Winter Solstice" },
    { 12, 300, "大寒", "dàhán",       "Major Cold" }
};

struct SolarTerm MinorSolarTerms[] = {
    { 0, -1, NULL, NULL, NULL },
    { 1,  315, "立春", "lìchūn",      "Start of Spring" },
    { 2,  345, "驚蟄", "jīngzhé",     "Awakening of Insects" },
    { 3,  15,  "清明", "qīngmíng",    "Clear and Bright" },
    { 4,  45,  "立夏", "lìxià",       "Start of Summer" },
    { 5,  75,  "芒種", "mángzhòng",   "Grain in Ear" },
    { 6,  105, "小暑", "xiǎoshǔ",     "Minor Heat" },
    { 7,  135, "立秋", "lìqiū",       "Start of Autumn" },
    { 8,  165, "白露", "báilù",       "White Dew" },
    { 9,  195, "寒露", "hánlù",       "Cold Dew" },
    { 10, 225, "立冬", "lìdōng",      "Start of Winter"  },
    { 11, 255, "大雪", "dàxuě",       "Major Snow" },
    { 12, 285, "小寒", "xiǎohán",     "Minor Cold" },
};

char *chinese_stem(int x)
{
    return Stems[x].chinese;
}

char *chinese_branch(int x)
{
    return Branches[x].chinese;
}

static struct Locale BejingOld = { angle(39,55,0), angle(116,25,0), 43.5, 1397/180 };
static struct Locale Bejing = { angle(39,55,0), angle(116,25,0), 43.5, 8.0 };
struct Locale *chinese_location(double t)
{
    double year = gregorian_year_from_fixed((int)floor(t));
    if (year < 1929)
        return &BejingOld;
    else
        return &Bejing;
}

double midnight_in_china(int date)
{
    return universal_from_standard((double)date, *chinese_location(date));
}

/* This assumes the date is being passed in the Chinese time zone.
   Pass date+1 for UTC. */
int current_major_solar_term(int date)
{
    struct Locale *b = chinese_location((double)date);
    double s = solar_longitude(universal_from_standard(date,*b));
    return amod(2 + floor(s / 30.0), 12);
}

/* This assumes the date is being passed in the Chinese time zone.
   Pass date+1 for UTC. */
int current_minor_solar_term(int date)
{
    struct Locale *b = chinese_location((double)date);
    double s = solar_longitude(universal_from_standard(date,*b));
    return amod(3 + floor((s - 15) / 30.0), 12);
}

int chinese_winter_solstice_on_or_before(int date)
{
    double approx = estimate_prior_solar_longitude(midnight_in_china(date + 1), LONGITUDE_WINTER);
    int i = (int)floor(approx) - 1;
    while (LONGITUDE_WINTER > solar_longitude(midnight_in_china(i + 1))) {
        i++;
    }
    return i;
}

int chinese_new_moon_on_or_after(int date)
{
    double t = new_moon_after(midnight_in_china(date));
    return (int)floor(standard_from_universal(t,*chinese_location(t)));
}

int chinese_new_moon_before(int date)
{
    double t = new_moon_before(midnight_in_china(date));
    return (int)floor(standard_from_universal(t,*chinese_location(t)));
}

/* True if lunar month starting on date has no major solar term. */
bool no_major_solar_term(int date)
{
    return current_major_solar_term(date) ==
            current_major_solar_term(chinese_new_moon_on_or_after(date + 1)) ?
            true : false;
}

/* True if there is a leap month on or after date1 and at or before date2. */
bool prior_leap_month(int date1, int date2)
{
    return (date2 >= date1 && (no_major_solar_term(date2) ||
            prior_leap_month(date1,chinese_new_moon_before(date2)))) ? true : false;
}

int chinese_new_year_in_sui(int date)
{
    int s1 = chinese_winter_solstice_on_or_before(date);
    int s2 = chinese_winter_solstice_on_or_before(s1 + 370);
    int m12 = chinese_new_moon_on_or_after(s1 + 1);
    int m13 = chinese_new_moon_on_or_after(m12 + 1);
    int next_m11 = chinese_new_moon_before(s2 + 1);
    if (round((next_m11 - m12) / MEAN_SYNODIC_MONTH) == 12 &&
            (no_major_solar_term(m12) || no_major_solar_term(m13))) {
        return chinese_new_moon_on_or_after(m13 + 1);
    } else {
        return m13;
    }
}

int chinese_new_year_on_or_before(int date)
{
    int new_year = chinese_new_year_in_sui(date);
    return date >= new_year ? new_year : chinese_new_year_in_sui(date - 180);
}

/* Return Chinese New Year during the given Gregorian year. */
int chinese_new_year(int gyear)
{
    return chinese_new_year_on_or_before(fixed_from_gregorian(gyear, 7, 1));
}

/* Indices for stem and branch for the chinese year (within a cycle) */
void chinese_sexagesimal_name(int cyear, int *stem, int *branch)
{
    *stem = amod(cyear,10);
    *branch = amod(cyear,12);
}

char *chinese_zodiac_animal(int date)
{
    struct ChineseDate cdate;
    chinese_from_fixed(date,&cdate);
    int stem, branch;
    chinese_sexagesimal_name(cdate.year, &stem, &branch);
    return Branches[branch].zodiac;
}

void chinese_from_fixed(int date, struct ChineseDate *cdate)
{
    int s1 = chinese_winter_solstice_on_or_before(date);
    int s2 = chinese_winter_solstice_on_or_before(s1 + 370);
    int m12 = chinese_new_moon_on_or_after(s1 + 1);
    int next_m11 = chinese_new_moon_before(s2 + 1);
    int m = chinese_new_moon_before(date + 1);
    bool leap_year = (int)round((next_m11 - m12) / MEAN_SYNODIC_MONTH) == 12
                                                            ? true : false;
    int adj = (leap_year && prior_leap_month(m12,m)) ? 1 : 0;
    cdate->month = amod(round((m - m12) / MEAN_SYNODIC_MONTH) - adj, 12);
    cdate->leap = leap_year &&
            no_major_solar_term(m) &&
            ! prior_leap_month(m12, chinese_new_moon_before(m)) ? true : false;
    int ep = fixed_from_gregorian(-2636,2,15);
    int elapsed_years = (int)floor(1.5 - (cdate->month / 12.0) +
                        (date - ep) / MEAN_TROPICAL_YEAR);
    cdate->cycle = (int)floor((elapsed_years - 1) / 60.0) + 1;
    cdate->year = amod(elapsed_years,60);
    cdate->day = date - m + 1;
}

int fixed_from_chinese(struct ChineseDate cdate)
{
    int mid_year = (int)floor(EPOCH_CHINESE + ((cdate.cycle - 1) *
            60 + (cdate.year - 1) + 0.5) * MEAN_TROPICAL_YEAR);
    int new_year = chinese_new_year_on_or_before(mid_year);
    int p = chinese_new_moon_on_or_after(new_year + (cdate.month - 1) * 29);

    struct ChineseDate d;
    chinese_from_fixed(p,&d);

    int prior_new_moon;
    if (cdate.month == d.month && cdate.leap == d.leap) prior_new_moon = p;
    else prior_new_moon = chinese_new_moon_on_or_after(p + 1);
    return prior_new_moon + cdate.day - 1;
}

#pragma mark Mayan

char *HaabMonths[] = { NULL, "Pop", "Uo", "Zip", "Zotz", "Tzec", "Xul",
    "Yaxkin", "Mol", "Chen", "Yax", "Zac", "Ceh", "Mac", "Kankin",
    "Muan", "Pax", "Kayab", "Cumku", "Uayeb" };

char *TzolkinNames[] = { NULL, "Imix", "Ik", "Akbal", "Kan", "Chicchan",
    "Cimi", "Manik", "Lamat", "Muluc", "Oc", "Chuen", "Eb", "Ben", "Ix",
    "Men", "Cib", "Caban", "Etznab", "Cauac", "Ahau" };

int fixed_from_mayan_long_count(int baktun, int katun, int tun, int uinal, int kin)
{
    return EPOCH_MAYAN + baktun * 144000 + katun * 7200 +
            tun * 360 + uinal * 20 + kin;
}

void mayan_long_count_from_fixed(int date, int *rbaktun, int *rkatun,
                                    int *rtun, int *ruinal, int *rkin)
{
    int long_count = date - EPOCH_MAYAN;
    int baktun = (int)floor(long_count / 144000.0);
    int day_of_baktun = mod(long_count,144000);
    int katun = (int)floor(day_of_baktun / 7200.0);
    int day_of_katun = mod(day_of_baktun,7200);
    int tun = (int)floor(day_of_katun / 360.0);
    int day_of_tun = mod(day_of_katun,360);
    int uinal = (int)floor(day_of_tun / 20.0);
    int kin = mod(day_of_tun,20);

    if (rbaktun) *rbaktun = baktun;
    if (rkatun) *rkatun = katun;
    if (rtun) *rtun = tun;
    if (ruinal) *ruinal = uinal;
    if (rkin) *rkin = kin;
}

int mayan_haab_ordinal(int month, int day)
{
    return ((month - 1) * 20 + day);
}

void mayan_haab_from_fixed(int date, int *rmonth, int *rday)
{
    int count = mod(date - EPOCH_MAYAN_HAAB, 365);
    int day = mod(count,20);
    int month = floor(count / 20.0) + 1;

    if (rmonth) *rmonth = month;
    if (rday) *rday = day;
}

int mayan_haab_on_or_before(int date, int haab_month, int haab_day)
{
    return date - mod((date - EPOCH_MAYAN_HAAB -
                mayan_haab_ordinal(haab_month,haab_day)),365);
}

int mayan_tzolkin_ordinal(int number, int name)
{
    return mod(number - 1 + 39 * (number - name), 260);
}

void mayan_tzolkin_from_fixed(int date, int *rnumber, int *rname)
{
    int count = date - EPOCH_MAYAN_TZOLKIN + 1;
    int number = amod(count,13);
    int name = amod(count,20);
    if (rnumber) *rnumber = number;
    if (rname) *rname = name;
}

int mayan_tzolkin_on_or_before(int date, int number, int name)
{
    return date - mod(date - EPOCH_MAYAN_TZOLKIN -
                        mayan_tzolkin_ordinal(number,name), 260);
}

#pragma mark ISO

int fixed_from_iso(int year, int week, int day)
{
    return nth_kday(week, 0, year - 1, 12, 28) + day;
}

void iso_from_fixed(int date, int *ryear, int *rweek, int *rday)
{
    int approx = gregorian_year_from_fixed(date - 3);
    int year;
    if (date >= fixed_from_iso(approx + 1, 1, 1)) year = approx + 1;
    else year = approx;
    int week = 1 + floor((date - fixed_from_iso(year, 1, 1)) / 7.0);
    int day = amod(date,7);

    if (ryear) *ryear = year;
    if (rweek) *rweek = week;
    if (rday) *rday = day;
}

#pragma mark Astronomical

static double c19[] = { -0.00002, 0.000297, 0.025184, -0.181133, 0.553040,
                        -0.861938, 0.677066, -0.212591 };
static double c18[] = { -0.000009, 0.003844, 0.083563, 0.865736, 4.867575, 15.845535,
                        31.332267, 38.291999, 28.316289, 11.636204, 2.043794 };
static double c17[] = { 196.58333, -4.0675, 0.0219167 };
static int c19c = 8;
static int c18c = 11;
static int c17c = 3;
double ephemeris_correction(double t)
{
    int year = gregorian_year_from_fixed((int)floor(t));
    double c = (double)(fixed_from_gregorian(year,7,1)
                    - fixed_from_gregorian(1900,1,1))
                    / 36525.0;
    double result;
    if (1988 <= year && year <= 2019) {
        result = (double)(year - 1933) / (24.0 * 60.0 * 60.0);
    } else if (1900 <= year && year <= 1987) {
        result = poly(c,c19c,c19);
    } else if (1800 <= year && year <= 1899) {
        result = poly(c,c18c,c18);
    } else if (1620 <= year && year <= 1799) {
        result = poly((double)(year - 1600), c17c, c17) / (24 * 60 * 60);
    } else {
        double x = 0.5 + (double)(fixed_from_gregorian(year,1,1)
                            - fixed_from_gregorian(1810,1,1));
        result = (x * x / 41048480.0 - 15) / (24 * 60 * 60);
    }

    return result;
}

double aberration(double t)
{
    double c = julian_centuries(t);
    return 0.0000974 * cos(deg2rad(177.63 + 35999.01848 * c)) - 0.0005575;
}

static double avec[] = { 124.90, -1934.134, 0.002063 };
static double bvec[] = { 201.11, 72001.5377, 0.00057 };
double nuation(double t)
{
    double c = julian_centuries(t);
    return -0.004778 * sin(deg2rad(poly(c,3,avec)))
                + -0.0003667 * sin(deg2rad(poly(c,3,bvec)));
}

static double oblvec[] = {
    0, angle(0, 0, -46.8150), angle(0, 0, -0.00059), angle(0, 0, 0.001813)
};
double obliquity(double t)
{
    double c = julian_centuries(t);
    return angle(23,26,21.448) + poly(c,3,oblvec);
}

static int xvec[] = {
    403406, 195207, 119433, 112392, 3891, 2819, 1721, 660, 350, 334, 314,
    268, 242, 234, 158, 132, 129, 114, 99, 93, 86, 78, 72, 68, 64, 46, 38,
    37, 32, 29, 28, 27, 27, 25, 24, 21, 21, 20, 18, 17, 14, 13, 13, 13, 12,
    10, 10, 10, 10
};
static double yvec[] = {
    270.54861, 340.19128, 63.91854, 331.26220,
    317.843, 86.631, 240.052, 310.26, 247.23,
    260.87, 297.82, 343.14, 166.79, 81.53,
    3.50, 132.75, 182.95, 162.03, 29.8,
    266.4, 249.2, 157.6, 257.8, 185.1, 69.9,
    8.0, 197.1, 250.4, 65.3, 162.7, 341.5,
    291.6, 98.5, 146.7, 110.0, 5.2, 342.6,
    230.9, 256.1, 45.3, 242.9, 115.2, 151.8,
    285.3, 53.3, 126.6, 205.7, 85.9, 146.1
};
static double zvec[] = {
    0.9287892, 35999.1376958, 35999.4089666,
    35998.7287385, 71998.20261, 71998.4403,
    36000.35726, 71997.4812, 32964.4678,
    -19.4410, 445267.1117, 45036.8840, 3.1008,
    22518.4434, -19.9739, 65928.9345,
    9038.0293, 3034.7684, 33718.148, 3034.448,
    -2280.773, 29929.992, 31556.493, 149.588,
    9037.750, 107997.405, -4444.176, 151.771,
    67555.316, 31556.080, -4561.540,
    107996.706, 1221.655, 62894.167,
    31437.369, 14578.298, -31931.757,
    34777.243, 1221.999, 62894.511,
    -4442.039, 107997.909, 119.066, 16859.071,
    -4.578, 26895.292, -39.127, 12297.536, 90073.778
};
static int vlen = 49;
double solar_longitude(double t)
{
    double c = julian_centuries(t);
    double sigma = 0.0;
    for (int i = 0; i < vlen; i++) {
        sigma += xvec[i] * sin(deg2rad(yvec[i] + (zvec[i] * c)));
    }
    double longitude = 282.7771834 + 36000.76953744 * c
                        + 0.000005729577951308232 * sigma;
    return mod(longitude + aberration(t) + nuation(t), 360);
}

/* Moment at or after t when solar longitude will be target degrees. */
double solar_longitude_after(double t, double target)
{
    double v = 0.00001;
    double rate = MEAN_TROPICAL_YEAR / 360.0;
    double tau = t + rate * mod(target - solar_longitude(t), 360);
    double l = fmax(t, tau - 5);
    double u = tau + 5;

    double lo = l, hi = u, x = (hi + lo) / 2;
    while (hi - lo > v) {
        if (mod(solar_longitude(x) - target, 360) < 180) hi = x;
        else lo = x;
        x = (hi + lo) / 2;
    }
    return x;
}

/* Approximate moment at or before t when solar longitude was target. */
double estimate_prior_solar_longitude(double t, double target)
{
    double rate = MEAN_TROPICAL_YEAR / 360;
    double tau = t - rate * mod(solar_longitude(t) - target, 360);
    double d = mod(solar_longitude(tau) - target + 180, 360) - 180;
    return fmin(t,tau - rate * d);
}

/* Calculate the Nth new moon after (or before if negative) the
   first new moon after RD 0, which was Jan 11, 1. */
static double nm_approx_vec[] = { 730125.59765, MEAN_SYNODIC_MONTH * 1236.85,
        0.0001337, -0.000000150, 0.00000000073 };
static double nm_E_vec[] = { 1, -0.002516, -0.0000074 };
static double nm_solaranom_vec[] = { 2.5534, 29.10535669 * 1236.85, -0.0000218, -0.00000011 };
static double nm_lunaranom_vec[] = { 201.5643, 385.81693528 * 1236.85, 0.0107438, 0.00001239, -0.000000058 };
static double nm_moonarg_vec[] = { 160.7108, 390.67050274 * 1236.85, -0.0016341, -0.00000227, 0.000000011 };
static double nm_omega_vec[] = { 124.7746, -1.56375580 * 1236.85, 0.0020691, 0.00000215 };
static int nm_w_vec[] = { 0, 1, 0, 0, 1, 1, 2, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int nm_x_vec[] = { 0, 1, 0, 0, -1, 1, 2, 0, 0, 1, 0, 1, 1, -1, 2, 0, 3, 1, 0, 1, -1, -1, 1, 0 };
static int nm_y_vec[] = { 1, 0, 2, 0, 1, 1, 0, 1, 1, 2, 3, 0, 0, 2, 1, 2, 0, 1, 2, 1, 1, 1, 3, 4 };
static int nm_z_vec[] = { 0, 0, 0, 2, 0, 0, 0, -2, 2, 0, 0, 2, -2, 0, 0, -2, 0, -2, 2, 2, 2, -2, 0, 0 };
static double nm_v_vec[] = { -0.40720, 0.17241, 0.01608, 0.01039, 0.00739, -0.00514, 0.00208,
        -0.00111, -0.00057, 0.00056, -0.00042, 0.00042, 0.00038, -0.00024,
        -0.00007, 0.00004, 0.00004, 0.00003, 0.00003, -0.00003, 0.00003,
        -0.00002, -0.00002, 0.00002 };
static double nm_i_vec[] = { 251.88, 251.83, 349.42, 84.66, 141.74, 207.14, 154.84,
        34.52, 207.19, 291.34, 161.72, 239.56, 331.55 };
static double nm_j_vec[] = { 0.016321, 26.641886, 36.412478, 18.206239, 53.303771,
        2.453732, 7.306860, 27.261239, 0.121824, 1.844379, 24.198154,
        25.513099, 3.592518 };
static double nm_l_vec[] = { 0.000165, 0.000164, 0.000126, 0.000110, 0.000062, 0.000060,
        0.000056, 0.000047, 0.000042, 0.000040, 0.000037, 0.000035, 0.000023 };
static double nm_extra_vec[] = { 299.77, 132.8475848, -0.009173 };
double nth_new_moon(int n)
{
    double k = n - 24724;
    double c = k / 1236.85;
    double approx = poly(c, 5, nm_approx_vec);
    double E = poly(c, 3, nm_E_vec);
    double solar_anomaly = poly(c, 4, nm_solaranom_vec);
    double lunar_anomaly = poly(c, 5, nm_lunaranom_vec);
    double moon_argument = poly(c, 5, nm_moonarg_vec);
    double omega = poly(c, 4, nm_omega_vec);
    double correction = -0.00017 * sin(deg2rad(omega));
    for (int i = 0; i < 24; i++) {
        correction += nm_v_vec[i] * pow(E, nm_w_vec[i]) *
            sin(deg2rad(nm_x_vec[i] * solar_anomaly +
                        nm_y_vec[i] * lunar_anomaly +
                        nm_z_vec[i] * moon_argument));
    }
    double additional = 0.0;
    for (int i = 0; i < 13; i++) {
        additional += nm_l_vec[i] * sin(deg2rad(nm_i_vec[i] + nm_j_vec[i] * k));
    }
    double extra = 0.000325 * sin(deg2rad(poly(c, 3, nm_extra_vec)));
    return universal_from_dynamical(approx + correction + extra + additional);
}

double new_moon_before(double t)
{
    double jd = jd_from_moment(t);
    int n = (int)round(t / MEAN_SYNODIC_MONTH - phase(jd, NULL,NULL,NULL,NULL,NULL,NULL));
    return nth_new_moon(n);
}

double new_moon_after(double t)
{
    double jd = jd_from_moment(t);
    int n = (int)round(t / MEAN_SYNODIC_MONTH - phase(jd, NULL,NULL,NULL,NULL,NULL,NULL));
    return nth_new_moon(n + 1);
}

struct Zodiac Zodiacs[] = {
    { -1, NULL, NULL },
    { 0,   "♈", "Aries" },
    { 30,  "♉", "Taurus" },
    { 60,  "♊", "Gemini" },
    { 90,  "♋", "Cancer" },
    { 120, "♌", "Leo" },
    { 150, "♍", "Virgo" },
    { 180, "♎", "Libra" },
    { 210, "♏", "Scorpio" },
    { 240, "♐", "Sagittarius" },
    { 270, "♑", "Capricorn" },
    { 300, "♒", "Aquarius" },
    { 330, "♓", "Pisces" }
};

int current_zodiac(int date)
{
    double s = solar_longitude(date);
    return amod(floor(s / 30.0), 12);
}

#pragma mark Time

/* "local" is local mean time, not the local timezone */
double universal_from_local(double t, struct Locale locale)
{
    return t - locale.longitude / 360;
}

double local_from_universal(double t, struct Locale locale)
{
    return t + locale.longitude / 360;
}

/* "standard" is the time in the local timezone */
double standard_from_universal(double t, struct Locale locale)
{
    return t + locale.timezone / 24;
}

double universal_from_standard(double t, struct Locale locale)
{
    return t - locale.timezone / 24;
}

double standard_from_local(double t, struct Locale locale)
{
    return standard_from_universal(universal_from_local(t,locale), locale);
}

double local_from_standard(double t, struct Locale locale)
{
    return local_from_universal(universal_from_standard(t,locale), locale);
}

/* Dynamical (astronomical) time */
double dynamical_from_universal(double t)
{
    return t + ephemeris_correction(t);
}

double universal_from_dynamical(double t)
{
    return t - ephemeris_correction(t);
}

/* Number (and fraction) of Dynamical Time centuries before/after 2000-01-01. */
double julian_centuries(double t)
{
    return (dynamical_from_universal(t) - 730120.5) / 36525.0;
}

static double etlongvec[] = { 280.46645, 36000.76983, 0.0003032 };
static double etanomvec[] = { 357.52910, 35999.05030, -0.0001559, -0.00000048 };
static double eteccvec[] = { 0.016708617, -0.000042037, -0.0000001236 };
double equation_of_time(double t)
{
    double c = julian_centuries(t);
    double longitude = poly(c, 3, etlongvec);
    double anomaly = poly(c, 4, etanomvec);
    double eccentricity = poly(c, 3, eteccvec);
    double squiggly = obliquity(t);
    double y = pow(tan(deg2rad(squiggly / 2.0)), 2);
    double eq = (1 / (2 * M_PI)) *
                (y * sin(deg2rad(2 * longitude)) +
                -2 * eccentricity * sin(deg2rad(anomaly)) +
                4 * eccentricity * y * sin(deg2rad(anomaly)) *
                cos(deg2rad(2 * longitude)) +
                -0.5 * y * y * sin(deg2rad(4 * longitude)) +
                -1.25 * eccentricity * eccentricity * sin(deg2rad(2 * anomaly)));
    return signum(eq) * fmin(fabs(eq), 0.5);
}
