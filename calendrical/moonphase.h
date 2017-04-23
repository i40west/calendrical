#include <time.h>

/* convert struct tm to Julian date */
long jdate(struct tm *t);

/* convert struct tm to Julian date/time */
double jtime(struct tm *t);

/* convert Julian date to Y, M, D */
void jyear(double td, int *yy, int *mm, int *dd);

/* convert Julian date to H, M, S */
void jhms(double j, int *h, int *m, int *s);

/* convert Julian date to Unixtime */
double jdaytosecs(double jday);

/* Find time of moon phases surrounding the given date.
 * Five phases are found, starting and ending with the new moons. */
void phasehunt(double sdate, double phases[5]);

/* Find time of pcount moon phases after the given date.
 * Caller must provide an array of at least pcount elements.
 * startphase is set to the phase of the first returned entry,
 * where 0 = new, 1 = first, 2 = full, 3 = last. */
void phaselist(double sdate, int pcount, double ph[], int *startphase);

/* Calculate phase of moon as a fraction.
 * Returns the terminator phase angle as a percentage of a full circle (0 to 1),
 * Returned in pointers:
 * illuminated fraction of the moon's disc,
 * the moon's age in days and fraction,
 * distance of the moon from the Earth's center,
 * angular diameter subtended by the moon as seen from the center of the Earth. */
double phase(double pdate, double *pphase, double *mage, double *dist,
             double *angdia, double *sudist, double *suangdia);
