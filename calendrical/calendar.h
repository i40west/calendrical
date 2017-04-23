
#include <stdbool.h>
#include <time.h>

#if !defined(__GNUC__) \
 || !( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
#undef __attribute__
#define __attribute__(x)
#endif

int fixed_from_struct_tm(struct tm *time);
int fixed_from_unixtime(time_t time);
double moment_from_unixtime(time_t time);
int day_of_week_from_fixed(int date) __attribute__((const));
int kday_on_or_before(int date, int k) __attribute__((const));
int kday_nearest(int date, int k) __attribute__((const));
int kday_on_or_after(int date, int k) __attribute__((const));
int kday_before(int date, int k) __attribute__((const));
int kday_after(int date, int k) __attribute__((const));
int nth_kday(int n, int k, int year, int month, int day) __attribute__((const));
int nth_kday_in_month(int n, int k, int year, int month) __attribute__((const));

double moment_from_jd(double jd) __attribute__((const));
double jd_from_moment(double t) __attribute__((const));
int fixed_from_jd(double jd) __attribute__((const));
double jd_from_fixed(int date) __attribute__((const));
int fixed_from_mjd(int mjd) __attribute__((const));
int mjd_from_fixed(int date) __attribute__((const));

bool gregorian_leap_year(int year) __attribute__((const));
int last_day_of_gregorian_month(int month, int year) __attribute__((const));
int gregorian_year_from_fixed(int date) __attribute__((const));
int fixed_from_gregorian(int year, int month, int day) __attribute__((const));
void gregorian_from_fixed(int date, int *ryear, int *rmonth, int *rday);

bool julian_leap_year(int year) __attribute__((const));
int last_day_of_julian_month(int month, int year) __attribute__((const));
int fixed_from_julian(int year, int month, int day) __attribute__((const));
void julian_from_fixed(int date, int *ryear, int *rmonth, int *rday);

bool islamic_leap_year(int year) __attribute__((const));
int last_day_of_islamic_month(int month, int year) __attribute__((const));
int fixed_from_islamic(int month, int day, int year) __attribute__((const));
void islamic_from_fixed(int date, int *rmonth, int *rday, int *ryear);

bool hebrew_leap_year(int year) __attribute__((const));
int last_month_of_hebrew_year(int year) __attribute__((const));
int last_day_of_hebrew_month(int month, int year) __attribute__((const));
int hebrew_calendar_elapsed_days(int year) __attribute__((const));
int hebrew_year_length_correction(int year) __attribute__((const));
int days_in_hebrew_year(int year) __attribute__((const));
bool long_marheshvan(int year) __attribute__((const));
bool short_kislev(int year) __attribute__((const));
int fixed_from_hebrew(int year, int month, int day) __attribute__((const));
void hebrew_from_fixed(int date, int *ryear, int *rmonth, int *rday);
int hebrew_birthday(int birth_month, int birth_day, int birth_year, int year) __attribute__((const));
int yahrzeit(int death_month, int death_day, int death_year, int year) __attribute__((const));

int advent(int year) __attribute__((const));
int eastern_orthodox_christmas(int year) __attribute__((const));
int nicaean_rule_easter(int year) __attribute__((const));
int easter(int year) __attribute__((const));
int easter_offset(int year, int month, int day) __attribute__((const));

struct Locale {
    double latitude;    // north is positive
    double longitude;   // EAST is positive!!
    double elevation;   // meters
    double timezone;    // difference in hours from UTC
};

struct ChineseDate {
    int cycle;
    int year;
    int month;
    bool leap;
    int day;
};

struct ChineseStem {
    char *chinese;
    char *pinyin;
    char *yinyang;
    char *element;
};
extern struct ChineseStem Stems[];

struct ChineseBranch {
    char *chinese;
    char *pinyin;
    char *zodiac;
    char *zsymbol;
};
extern struct ChineseBranch Branches[];

struct SolarTerm {
    int index;
    int longitude;
    char *chinese;
    char *pinyin;
    char *english;
};
extern struct SolarTerm MajorSolarTerms[];
extern struct SolarTerm MinorSolarTerms[];

char *chinese_stem(int x);
struct Locale *chinese_location(double t);
double midnight_in_china(int date) __attribute__((const));
int current_major_solar_term(int date) __attribute__((const));
int current_minor_solar_term(int date) __attribute__((const));
int chinese_winter_solstice_on_or_before(int date) __attribute__((const));
int chinese_new_moon_before(int date) __attribute__((const));
int chinese_new_moon_on_or_after(int date) __attribute__((const));
bool no_major_solar_term(int date) __attribute__((const));
bool prior_leap_month(int date1, int date2) __attribute__((const));
int chinese_new_year_in_sui(int date) __attribute__((const));
int chinese_new_year_on_or_before(int date) __attribute__((const));
int chinese_new_year(int gyear) __attribute__((const));
void chinese_sexagesimal_name(int cyear, int *stem, int *branch);
char *chinese_zodiac_animal(int date);
void chinese_from_fixed(int date, struct ChineseDate *cdate);
int fixed_from_chinese(struct ChineseDate cdate);

extern char *HaabMonths[];
extern char *TzolkinNames[];
int fixed_from_mayan_long_count(int baktun, int katun, int tun, int uinal, int kin) __attribute__((const));
void mayan_long_count_from_fixed(int date, int *rbaktun, int *rkatun, int *rtun, int *ruinal, int *rkin);
int mayan_haab_ordinal(int month, int day) __attribute__((const));
void mayan_haab_from_fixed(int date, int *rmonth, int *rday);
int mayan_haab_on_or_before(int date, int haab_month, int haab_day) __attribute__((const));
int mayan_tzolkin_ordinal(int number, int name) __attribute__((const));
void mayan_tzolkin_from_fixed(int date, int *rnumber, int *rname);
int mayan_tzolkin_on_or_before(int date, int number, int name) __attribute__((const));

int fixed_from_iso(int year, int week, int day) __attribute__((const));
void iso_from_fixed(int date, int *ryear, int *rweek, int *rday);

struct Zodiac {
    int longitude;
    char *symbol;
    char *name;
};
extern struct Zodiac Zodiacs[];

double ephemeris_correction(double t) __attribute__((const));
double aberration(double t) __attribute__((const));
double nuation(double t) __attribute__((const));
double obliquity(double t) __attribute__((const));
double solar_longitude(double t) __attribute__((const));
double solar_longitude_after(double t, double target) __attribute__((const));
double estimate_prior_solar_longitude(double t, double target) __attribute__((const));
double nth_new_moon(int n) __attribute__((const));
double new_moon_before(double t) __attribute__((const));
double new_moon_after(double t) __attribute__((const));
int current_zodiac(int date) __attribute__((const));

double universal_from_local(double t_local, struct Locale locale);
double local_from_universal(double t_universal, struct Locale locale);
double standard_from_universal(double t_universal, struct Locale locale);
double universal_from_standard(double t_standard, struct Locale locale);
double standard_from_local(double t, struct Locale locale);
double local_from_standard(double t, struct Locale locale);
double dynamical_from_universal(double t) __attribute__((const));
double universal_from_dynamical(double t) __attribute__((const));
double julian_centuries(double t) __attribute__((const));
double equation_of_time(double t) __attribute__((const));
