/* Output the current Chinese year name. */

#include <stdio.h>
#include <time.h>
#include "calendar.h"

int main(int argc, char *argv[])
{
    time_t now = time(NULL);
    struct tm *today;
    today = localtime(&now);

    struct ChineseDate cdate;
    int rd = fixed_from_struct_tm(today);
    chinese_from_fixed(rd,&cdate);

    int stem,branch;
    chinese_sexagesimal_name(cdate.year, &stem, &branch);
    printf("%s%s Year of the %s\n", Stems[stem].chinese,
                                    Branches[branch].chinese,
                                    Branches[branch].zodiac);
    return 0;
}
