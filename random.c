#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "global.h"
#include "debug.h"
#include "random.h"
#include "tcl.h"
#include "communication.h"
#include "utils.h"

/** \file random.c A random generator. Be sure to run init_random() before
    you use any of the generators. */

# define NTAB_RANDOM  32


const long     IA = 16807;
const long     IM = 2147483647;
const double   AM = (1.0/2147483647. );
const long     IQ = 127773;
const long     IR = 2836;
const double NDIV = (double) (1+(2147483647-1)/NTAB_RANDOM);
const double RNMX = (1.0-1.2e-7);


static long  idum = 1;
static long  iy=0;
static long  iv[NTAB_RANDOM];

/*----------------------------------------------------------------------*/

long l_random(void)
{
  /* 
   *    from Numerical Recipes in C by Press et al.,
   *    N O T E   T H A T   T H E R E   A R E   N O   S A F E T Y   C H E C K S  !!!
   */
  
  int    j;
  long   k;
  
  k = (idum) / IQ;
  idum = IA * (idum - k * IQ) - IR * k;
  if (idum < 0) idum += IM;
  j = iy / NDIV;
  iy = iv[j];
  iv[j] = idum;
  return iy;
}

/*----------------------------------------------------------------------*/

int i_random(int maxint)
{
  /* delivers an integer between 0 and maxint-1 */
  int temp;
  temp =  (int)( ( (double) maxint * l_random() )* AM );
  return temp;
}
  

/*----------------------------------------------------------------------*/

double d_random(void)
{
  /* delivers a uniform double between 0 and 1 */
  double temp;
  iy = l_random();
  if ((temp = AM * iy) > RNMX) 
    temp = RNMX;
  return temp;
}

/*----------------------------------------------------------------------*/

void init_random(void)
{
  /* initializes the random number generator. You MUST NOT FORGET THIS! */

  int    j;
  long   k;
  unsigned long seed;

  /* This random generator is bad I know, thats why its only used
     for the seed (see Num. Rec. 7.1.) */
  seed = (10*this_node+1)*1103515245 + 12345;
  seed = (seed/65536) % 32768;
  idum = (long) seed;
  RANDOM_TRACE(fprintf(stderr, "%d init random with seed %ld\n",this_node,idum));
  for (j = NTAB_RANDOM + 7;j >= 0; j--) {
    k = (idum) / IQ;
    idum = IA * (idum - k * IQ) - IR * k;
    if (idum < 0) idum += IM;
    if (j < NTAB_RANDOM) iv[j] = idum;
  }
  iy = iv[0];
}

/*----------------------------------------------------------------------*/

/**  A random generator for tcl.
 Usage: tcl_rand() for uniform double in ]0;1[
 tcl_rand(i <n>) for integer between 0 and n-1*/

int tcl_rand(ClientData data, Tcl_Interp *interp,
	  int argc, char **argv)
{
  int i_out;
  double d_out;
  char   buffer[TCL_DOUBLE_SPACE + 5];

  if (argc > 1) {
    switch(argv[1][0])
      {
      case 'i':
	if(argc < 3){
	  Tcl_AppendResult(interp, "wrong # args:  should be \"",
			   argv[0], " ?variable type? ?parameter?\"",
			   (char *) NULL);
	  return (TCL_ERROR);
	}else {
	  Tcl_GetInt(interp, argv[2], &i_out);
	  i_out = i_random(i_out);
	  sprintf(buffer, "%d", i_out);
	  Tcl_AppendResult(interp, buffer, (char *) NULL);
	  return (TCL_OK);
	} 
      case 'd':
	d_out = d_random();
	sprintf(buffer, "%f", d_out);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	return (TCL_OK);
      default:
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
			 argv[0], " ?variable type? ?parameter?\"",
			 (char *) NULL);
	return (TCL_ERROR);
      }
  }else {
   d_out = d_random();
   sprintf(buffer, "%f", d_out);
   Tcl_AppendResult(interp, buffer, (char *) NULL); 
   return (TCL_OK);
  }
  printf("Error in tcl_rand(); this should never been shown\n");
  return(TCL_ERROR);
}
