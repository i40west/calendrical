# calendrical

This is a C implementation of most of the book "Calendrical Calculations"
by Edward Reingold and Nachum Dershowitz.

The library includes the functions to convert between several calendars:
Gregorian, Julian, Chinese, Mayan, Hebrew, Islamic, ISO, and Hindu.
Calculation of Easter is included, as is a full set of moon phase calculations.

Functions are named as in the book. The Hindu functions are separated into
their own file because they are weird and they redefined things and I needed
to keep them separate. They are also the least-tested part of this code.
(I have used the main library in production, but I have never used the Hindu
functions.)

Also included are two little command-line programs showing usage of the library.
mayandate outputs the date in the Mayan calendar; and cyear outputs the current
Chinese year name.

The moon phase code is adapted from moontool.c by John Walker (far be it from
me to take credit for that math).
