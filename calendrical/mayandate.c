/* Output the date in the Mayan fashion. */

#include <stdio.h>
#include <time.h>
#include "calendar.h"

int main(int argc, char *argv[])
{
    int baktun, katun, tun, uinal, kin;
    int h_month, h_day;
    int t_number, t_name;

    time_t now = time(NULL);
    struct tm *today;
    today = localtime(&now);

    int rd = fixed_from_struct_tm(today);

    mayan_long_count_from_fixed(rd, &baktun, &katun, &tun, &uinal, &kin);
    mayan_haab_from_fixed(rd, &h_month, &h_day);
    mayan_tzolkin_from_fixed(rd, &t_number, &t_name);

    printf("Long Count = %d.%d.%d.%d.%d; Tzolkin = %d %s; Haab = %d %s\n",
        baktun, katun, tun, uinal, kin,
        t_number, TzolkinNames[t_name],
        h_day, HaabMonths[h_month]);

}
