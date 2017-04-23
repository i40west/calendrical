/*
 *  hindu.c
 *  Includes the hindu stuff I'm not using, because it's weird and redefines stuff
 *  and I want it out of the way.
 */

#include <stddef.h>
#include <math.h>
#include <calendar.h>

double solar_longitude(double days) __attribute__ ((const));
double zodiac(double days) __attribute__ ((const));
double lunar_longitude(double days) __attribute__ ((const));
double lunar_phase(double days) __attribute__ ((const));
double new_moon(double days) __attribute__ ((const));

void old_hindu_solar_from_absolute(int date, int *rmonth, int *rday, int *ryear);
int absolute_from_old_hindu_solar(int month, int day, int year) __attribute__ ((const));
void old_hindu_lunar_from_absolute(int date, int *rmonth, int *rleapmonth, int *rday, int *ryear);
int old_hindu_lunar_precedes(int month1, int leap1, int day1, int year1,
     int month2, int leap2, int day2, int year2) __attribute__ ((const));
int absolute_from_old_hindu_lunar(int month, int leapmonth, int day, int year) __attribute__ ((const));

#undef quotient
#define quotient(m, n) (floor(((double)(m)) / ((double)(n))))
#undef mod
#define mod(m, n) f_mod(m, n)
static double f_mod(double m, double n)
{
    double x;

    x = fmod(m, n);
    if ((n < 0) ? (x > 0) : (x < 0))
        x += n;
    return x;
}
#undef oddp
#define oddp(n) (((int)(n)) % 2)

#define solar_sidereal_year (365 + 279457.0 / 1080000)
#define solar_month (solar_sidereal_year / 12)
#define lunar_sidereal_month (27 + 4644439.0 / 14438334)
#define lunar_synodic_month (29 + 7087771.0 / 13358334)

double solar_longitude(double days)
{
    return mod(days / solar_sidereal_year, 1) * 360;
}

double zodiac(double days)
{
    return 1 + quotient(solar_longitude(days), 30);
}

double lunar_longitude(double days)
{
    return mod (days / lunar_sidereal_month, 1) * 360;
}

double lunar_phase(double days)
{
    return 1
        + quotient
        (mod (lunar_longitude(days) - solar_longitude(days),
        360),
        12);
}

double new_moon(double days)
{
    return days - mod(days, lunar_synodic_month);
}

// Hindu
void old_hindu_solar_from_absolute(int date, int *rmonth, int *rday, int *ryear)
{
    double hdate;
    int year, month, day;

    hdate = date + 1132959 + 1.0 / 4;
    year = quotient(hdate, solar_sidereal_year);
    month = zodiac(hdate);
    day = 1 + floor(mod(hdate, solar_month));
    if (rmonth)
        *rmonth = month;
    if (rday)
        *rday = day;
    if (ryear)
        *ryear = year;
}

int absolute_from_old_hindu_solar(int month, int day, int year)
{
    return floor ((year * solar_sidereal_year)
        + ((month - 1) * solar_month)
        + day - 1.0 / 4
        - 1132959);
}

void old_hindu_lunar_from_absolute(int date, int *rmonth, int *rleapmonth, int *rday, int *ryear)
{
    double hdate, sunrise, last_new_moon, next_new_moon, next_month;
    int day, month, leapmonth, year;

    hdate = date + 1132959;
    sunrise = hdate + 1.0 / 4;
    last_new_moon = new_moon(sunrise);
    next_new_moon = last_new_moon + lunar_synodic_month;
    day = lunar_phase(sunrise);
    month = adjusted_mod(1 + zodiac(last_new_moon), 12);
    leapmonth = zodiac(last_new_moon) == zodiac(next_new_moon);
    next_month = next_new_moon + (leapmonth ? lunar_synodic_month : 0);
    year = quotient(next_month, solar_sidereal_year);
    if (rmonth)
        *rmonth = month;
    if (rleapmonth)
        *rleapmonth = leapmonth;
    if (rday)
        *rday = day;
    if (ryear)
        *ryear = year;
}

int old_hindu_lunar_precedes
    (int month1, int leap1, int day1, int year1,
     int month2, int leap2, int day2, int year2)
{
    return ((year1 < year2) ||
      ((year1 == year2) &&
       ((month1 < month2) ||
        ((month1 == month2) &&
         ((leap1 && !leap2) ||
          ((leap1 == leap2) &&
           (day1 < day2)))))));
}

int absolute_from_old_hindu_lunar(int month, int leapmonth, int day, int year)
{
    int years, months, approx, try,
        month1, leapmonth1, day1, year1;
    int sumres;

    years = year;
    months = month - 2;
    approx = floor (years * solar_sidereal_year)
        + floor (months * lunar_synodic_month)
        - 1132959;
    {
        int temp, i;
        for (temp = 0, i = approx;
            (old_hindu_lunar_from_absolute
            (i, &month1, &leapmonth1, &day1, &year1),
            old_hindu_lunar_precedes
            (month1, leapmonth1, day1, year1,
            month, leapmonth, day, year));
            temp = temp + 1, i++);
        sumres = temp;
    }
    try = approx + sumres;
    old_hindu_lunar_from_absolute(try, &month1, &leapmonth1, &day1, &year);
    if (month1 == month &&
        leapmonth1 == leapmonth &&
        day1 == day &&
        year1 == year)
        return try;
    return 0;
}
