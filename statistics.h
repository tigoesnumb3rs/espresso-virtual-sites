#ifndef STATISTICS_H
#define STATISTICS_H
/** \file statistics.h
    This file contains the code for simply statistics on the data.
*/

#include <tcl.h>

/** Implements the Tcl command mindist. It returns the minimal distance of two particles
    (in minimum image convention). */
int mindist(ClientData data, Tcl_Interp *interp, int argc, char **argv);


#endif
