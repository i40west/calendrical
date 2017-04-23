/*
 * Moon phase code.
 * Adapted from moontool.c by John Walker
 *
 * Code in this file is released into the public domain,
 * because the original was.
 */

#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "moonphase.h"

#define epoch       2444238.5      /* 1980 January 0.0 */

/*  Constants defining the Sun's apparent orbit  */

#define elonge      278.833540     /* Ecliptic longitude of the Sun at epoch 1980.0 */
#define elongp      282.596403     /* Ecliptic longitude of the Sun at perigee */
#define eccent      0.016718       /* Eccentricity of Earth's orbit */
#define sunsmax     1.495985e8     /* Semi-major axis of Earth's orbit, km */
#define sunangsiz   0.533128       /* Sun's angular size, degrees, at semi-major axis distance */

/*  Elements of the Moon's orbit, epoch 1980.0  */

#define mmlong      64.975464      /* Moon's mean lonigitude at the epoch */
#define mmlongp     349.383063     /* Mean longitude of the perigee at the epoch */
#define mlnode      151.950429     /* Mean longitude of the node at the epoch */
#define minc        5.145396       /* Inclination of the Moon's orbit */
#define mecc        0.054900       /* Eccentricity of the Moon's orbit */
#define mangsiz     0.5181         /* Moon's angular size at distance a from Earth */
#define msmax       384401.0       /* Semi-major axis of Moon's orbit in km */
#define mparallax   0.9507         /* Parallax at distance a from Earth */
#define synmonth    29.53058868    /* Synodic month (new Moon to new Moon) */
#define lunatbase   2423436.0      /* Base date for E. W. Brown's numbered series of lunations (1923 January 16) */

/*  Properties of the Earth  */

#define earthrad    6378.16    /* Radius of Earth in kilometres */

#define PI 3.14159265358979323846

/*  Handy mathematical functions  */

#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))      /* Extract sign */
#define abs(x) ((x) < 0 ? (-(x)) : (x))                  /* Absolute val */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0))) /* Fix angle    */
#define torad(d) ((d) * (PI / 180.0))                    /* Deg->Rad     */
#define todeg(d) ((d) * (180.0 / PI))                    /* Rad->Deg     */
#define dsin(x) (sin(torad((x))))                        /* Sin from deg */
#define dcos(x) (cos(torad((x))))                        /* Cos from deg */

static double meanphase(double sdate, double k);
static double truephase(double k, double ph);
static double kepler(double m, double ecc);

/* Convert internal GMT date and time to Julian day and fraction. */
long jdate(struct tm *t)
{
    long c, m, y;

    y = t->tm_year + 1900;
    m = t->tm_mon + 1;
    if (m > 2) {
        m = m - 3;
    } else {
        m = m + 9;
        y--;
    }
    c = y / 100L;          /* Compute century */
    y -= 100L * c;
    return (t->tm_mday + (c * 146097L) / 4 + (y * 1461L) / 4 +
        (m * 153L + 2) / 5 + 1721119L);
}

/* Convert internal GMT date and time to astronomical Julian
   time (i.e. Julian date plus day fraction, expressed as a double). */
double jtime(struct tm *t)
{
    return (jdate(t) - 0.5) + 
       (t->tm_sec + 60 * (t->tm_min + 60 * t->tm_hour)) / 86400.0;
}

/* Convert Julian date to year, month, day, which are returned via
   integer pointers to integers. */
void jyear(double td, int *yy, int *mm, int *dd)
{
    double j, d, y, m;

    td += 0.5;           /* Astronomical to civil */
    j = floor(td);
    j = j - 1721119.0;
    y = floor(((4 * j) - 1) / 146097.0);
    j = (j * 4.0) - (1.0 + (146097.0 * y));
    d = floor(j / 4.0);
    j = floor(((4.0 * d) + 3.0) / 1461.0);
    d = ((4.0 * d) + 3.0) - (1461.0 * j);
    d = floor((d + 4.0) / 4.0);
    m = floor(((5.0 * d) - 3) / 153.0);
    d = (5.0 * d) - (3.0 + (153.0 * m));
    d = floor((d + 5.0) / 5.0);
    y = (100.0 * y) + j;
    if (m < 10.0)
        m = m + 3;
    else {
        m = m - 9;
        y = y + 1;
    }
    *yy = y;
    *mm = m;
    *dd = d;
}

/* Convert Julian time to Unixtime. */
double jdaytosecs(double jday)
{
    return (jday - 2440587.5) * 86400;
}

/* Convert Julian time to hour, minutes, and seconds. */
void jhms(double j, int *h, int *m, int *s)
{
    long ij;

    j += 0.5;           /* Astronomical to civil */
    ij = (j - floor(j)) * 86400.0;
    *h = (int)(ij / 3600L);
    *m = (ij / 60L) % 60L;
    *s = ij % 60L;
}

/* Calculates time of the mean new Moon for a given
 * base date.  This argument K to this function is
 * the precomputed synodic month index, given by:
 *
 *     K = (year - 1900) * 12.3685
 *
 * where year is expressed as a year and fractional year. */
static double meanphase(double sdate, double k)
{
    double  t, t2, t3, nt1;

    /* Time in Julian centuries from 1900 January 0.5 */
    t = (sdate - 2415020.0) / 36525;
    t2 = t * t;         /* Square for frequent use */
    t3 = t2 * t;        /* Cube for frequent use */

    nt1 = 2415020.75933 + synmonth * k
        + 0.0001178 * t2
        - 0.000000155 * t3
        + 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

    return nt1;
}

/* Given a K value used to determine the mean phase of the new moon,
   and a phase selector (0.0, 0.25, 0.5, 0.75),
   obtain the true, corrected phase time. */
static double truephase(double k, double ph)
{
    double t, t2, t3, pt, m, mprime, f;
    bool apcor = false;

    k += ph;                     /* Add phase to new moon time */
    t = k / 1236.85;             /* Time in Julian centuries from 1900 January 0.5 */
    t2 = t * t;                  /* Square for frequent use */
    t3 = t2 * t;                 /* Cube for frequent use */
    pt = 2415020.75933           /* Mean time of phase */
         + synmonth * k
         + 0.0001178 * t2
         - 0.000000155 * t3
         + 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

        m = 359.2242             /* Sun's mean anomaly */
        + 29.10535608 * k
        - 0.0000333 * t2
        - 0.00000347 * t3;
        mprime = 306.0253        /* Moon's mean anomaly */
        + 385.81691806 * k
        + 0.0107306 * t2
        + 0.00001236 * t3;
        f = 21.2964              /* Moon's argument of latitude */
        + 390.67050646 * k
        - 0.0016528 * t2
        - 0.00000239 * t3;
    if ((ph < 0.01) || (abs(ph - 0.5) < 0.01)) {

       /* Corrections for New and Full Moon */

       pt += (0.1734 - 0.000393 * t) * dsin(m)
            + 0.0021 * dsin(2 * m)
            - 0.4068 * dsin(mprime)
            + 0.0161 * dsin(2 * mprime)
            - 0.0004 * dsin(3 * mprime)
            + 0.0104 * dsin(2 * f)
            - 0.0051 * dsin(m + mprime)
            - 0.0074 * dsin(m - mprime)
            + 0.0004 * dsin(2 * f + m)
            - 0.0004 * dsin(2 * f - m)
            - 0.0006 * dsin(2 * f + mprime)
            + 0.0010 * dsin(2 * f - mprime)
            + 0.0005 * dsin(m + 2 * mprime);
       apcor = true;
    } else if ((abs(ph - 0.25) < 0.01 || (abs(ph - 0.75) < 0.01))) {
       pt += (0.1721 - 0.0004 * t) * dsin(m)
            + 0.0021 * dsin(2 * m)
            - 0.6280 * dsin(mprime)
            + 0.0089 * dsin(2 * mprime)
            - 0.0004 * dsin(3 * mprime)
            + 0.0079 * dsin(2 * f)
            - 0.0119 * dsin(m + mprime)
            - 0.0047 * dsin(m - mprime)
            + 0.0003 * dsin(2 * f + m)
            - 0.0004 * dsin(2 * f - m)
            - 0.0006 * dsin(2 * f + mprime)
            + 0.0021 * dsin(2 * f - mprime)
            + 0.0003 * dsin(m + 2 * mprime)
            + 0.0004 * dsin(m - 2 * mprime)
            - 0.0003 * dsin(2 * m + mprime);
       if (ph < 0.5)
          /* First quarter correction */
          pt += 0.0028 - 0.0004 * dcos(m) + 0.0003 * dcos(mprime);
       else
          /* Last quarter correction */
          pt += -0.0028 + 0.0004 * dcos(m) - 0.0003 * dcos(mprime);
       apcor = true;
    }
    if (!apcor) {
        pt = NAN;
    }
    return pt;
}

/* Solve the equation of Kepler. */
static double kepler(double m, double ecc)
{
    double e, delta;
#define EPSILON 1E-6

    e = m = torad(m);
    do {
        delta = e - ecc * sin(e) - m;
        e -= delta / (1 - ecc * cos(e));
    } while (abs(delta) > EPSILON);
    return e;
}

/* Find time of phases of the moon which surround the current date.
   Five phases are found, starting and ending with the new moons
   which bound the current lunation. */
void phasehunt(double sdate, double phases[5])
{
    double  adate, k1, k2, nt1, nt2;
    int yy, mm, dd;

    adate = sdate - 45;

    jyear(adate, &yy, &mm, &dd);
    k1 = floor((yy + ((mm - 1) * (1.0 / 12.0)) - 1900) * 12.3685);

    adate = nt1 = meanphase(adate, k1);
    while (1) {
        adate += synmonth;
        k2 = k1 + 1;
        nt2 = meanphase(adate, k2);
        if (nt1 <= sdate && nt2 > sdate)
            break;
        nt1 = nt2;
        k1 = k2;
    }
    phases[0] = truephase(k1, 0.0);
    phases[1] = truephase(k1, 0.25);
    phases[2] = truephase(k1, 0.5);
    phases[3] = truephase(k1, 0.75);
    phases[4] = truephase(k2, 0.0);
}

/* List the phases that come after the given date,
   to a maximum of pcount phases, stored in the array ph to
   be provided by the caller, startphase stored 0 == new moon.
   This is based on Astro::MoonPhase. */
void phaselist(double sdate, int pcount, double ph[], int *startphase)
{
    int yy, mm, dd;
    *startphase = -1;
    jyear(sdate, &yy, &mm, &dd);
    double k = floor((yy + ((mm - 1) * (1.0 / 12.0)) - 1900) * 12.3685) - 2;

    int c = 0, i = 0;
    do {
        if (c % 4 == 0) k++;
        double psel = (c % 4) * 0.25;
        double d = truephase(k, psel);

        if (d >= sdate) {
            if (*startphase == -1) *startphase = (c % 4);
            ph[i] = d;
            i++;
        }
        c++;
    } while (i < pcount);
}

/* Calculate phase of moon as a fraction:

   The argument is the time for which the phase is requested,
   expressed as a Julian date and fraction.  Returns the terminator
   phase angle as a percentage of a full circle (i.e., 0 to 1),
   and stores into pointer arguments the illuminated fraction of
   the Moon's disc, the Moon's age in days and fraction, the
   distance of the Moon from the centre of the Earth, and the
   angular diameter subtended by the Moon as seen by an observer
   at the centre of the Earth. */
double phase(double pdate,
             double *pphase,        /* Illuminated fraction */
             double *mage,          /* Age of moon in days */
             double *dist,          /* Distance in kilometres */
             double *angdia,        /* Angular diameter in degrees */
             double *sudist,        /* Distance to Sun */
             double *suangdia)      /* Sun's angular diameter */
{

    double  Day, N, M, Ec, Lambdasun, ml, MM, MN, Ev, Ae, A3, MmP,
        mEc, A4, lP, V, lPP, NP, y, x, Lambdamoon, BetaM,
        MoonAge, MoonPhase,
        MoonDist, MoonDFrac, MoonAng, MoonPar,
        F, SunDist, SunAng;

        /* Calculation of the Sun's position */

    Day = pdate - epoch;                    /* Date within epoch */
    N = fixangle((360 / 365.2422) * Day);   /* Mean anomaly of the Sun */
    M = fixangle(N + elonge - elongp);      /* Convert from perigee co-ordinates to epoch 1980.0 */
    Ec = kepler(M, eccent);                 /* Solve equation of Kepler */
    Ec = sqrt((1 + eccent) / (1 - eccent)) * tan(Ec / 2);
    Ec = 2 * todeg(atan(Ec));               /* True anomaly */
        Lambdasun = fixangle(Ec + elongp);  /* Sun's geocentric ecliptic
                                               longitude */
    /* Orbital distance factor */
    F = ((1 + eccent * cos(torad(Ec))) / (1 - eccent * eccent));
    SunDist = sunsmax / F;                  /* Distance to Sun in km */
    SunAng = F * sunangsiz;                 /* Sun's angular size in degrees */


        /* Calculation of the Moon's position */

    /* Moon's mean longitude */
    ml = fixangle(13.1763966 * Day + mmlong);

    /* Moon's mean anomaly */
    MM = fixangle(ml - 0.1114041 * Day - mmlongp);

    /* Moon's ascending node mean longitude */
    MN = fixangle(mlnode - 0.0529539 * Day);

    /* Evection */
    Ev = 1.2739 * sin(torad(2 * (ml - Lambdasun) - MM));

    /* Annual equation */
    Ae = 0.1858 * sin(torad(M));

    /* Correction term */
    A3 = 0.37 * sin(torad(M));

    /* Corrected anomaly */
    MmP = MM + Ev - Ae - A3;

    /* Correction for the equation of the centre */
    mEc = 6.2886 * sin(torad(MmP));

    /* Another correction term */
    A4 = 0.214 * sin(torad(2 * MmP));

    /* Corrected longitude */
    lP = ml + Ev + mEc - Ae + A4;

    /* Variation */
    V = 0.6583 * sin(torad(2 * (lP - Lambdasun)));

    /* True longitude */
    lPP = lP + V;

    /* Corrected longitude of the node */
    NP = MN - 0.16 * sin(torad(M));

    /* Y inclination coordinate */
    y = sin(torad(lPP - NP)) * cos(torad(minc));

    /* X inclination coordinate */
    x = cos(torad(lPP - NP));

    /* Ecliptic longitude */
    Lambdamoon = todeg(atan2(y, x));
    Lambdamoon += NP;

    /* Ecliptic latitude */
    BetaM = todeg(asin(sin(torad(lPP - NP)) * sin(torad(minc))));

    /* Calculation of the phase of the Moon */

    /* Age of the Moon in degrees */
    MoonAge = lPP - Lambdasun;

    /* Phase of the Moon */
    MoonPhase = (1 - cos(torad(MoonAge))) / 2;

    /* Calculate distance of moon from the centre of the Earth */

    MoonDist = (msmax * (1 - mecc * mecc)) /
       (1 + mecc * cos(torad(MmP + mEc)));

    /* Calculate Moon's angular diameter */

    MoonDFrac = MoonDist / msmax;
    MoonAng = mangsiz / MoonDFrac;

    /* Calculate Moon's parallax */

    MoonPar = mparallax / MoonDFrac;

    if (pphase) *pphase = MoonPhase;
    if (mage) *mage = synmonth * (fixangle(MoonAge) / 360.0);
    if (dist) *dist = MoonDist;
    if (angdia) *angdia = MoonAng;
    if (sudist) *sudist = SunDist;
    if (suangdia) *suangdia = SunAng;
    return fixangle(MoonAge) / 360.0;
}
