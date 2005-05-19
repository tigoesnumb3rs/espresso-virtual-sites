// This file is part of the ESPResSo distribution (http://www.espresso.mpg.de).
// It is therefore subject to the ESPResSo license agreement which you accepted upon receiving the distribution
// and by which you are legally bound while utilizing this file in any form or way.
// There is NO WARRANTY, not even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// You should have received a copy of that license along with this program;
// if not, refer to http://www.espresso.mpg.de/license.html where its current version can be found, or
// write to Max-Planck-Institute for Polymer Research, Theory Group, PO Box 3148, 55021 Mainz, Germany.
// Copyright (c) 2002-2005; all rights reserved unless otherwise stated.
#ifndef CONFIG_H
#define CONFIG_H

/** \file config.h
 
   This file contains all Precompiler Flags deciding which features
   of Espresso to turn on/off. It is recommended to turn everything
   off which you do not need in order to optimize the performance of
   Espresso for your problem. 
 
   There are also quite a number of features which are turned off by
   default since they are used only rarely.
 
   You can get information on the compilation status of the code you
   are working with by using the tcl command \ref tcl_code_info "code_info"
   within your tcl_script. It is highly recommended to
   store this information with your simulation data in order to
   maintain the reproducibility of your results.
 
   If you add a new compile flag you also have to add the
   corresponding lines in the function \ref compilation_callback.
 
   <b>Responsible:</b>
   <a href="mailto:arnolda@mpip-mainz.mpg.de">Axel</a>

   <h1> Configuration switches in this file </h1>

   <h2> Switches for basic features </h2>
    <ul>
    <li> \verbatim #define PARTIAL_PERIODIC \endverbatim
    if defined, the code will be slower, but with the \ref #periodic
    array you can choose which coordinates are bound to p.b.c and
    which are not. If not defined, all coordinates are bound to
    p.b.c.

    Has effect on: \ref per_callback, \ref #fields, and functions in
    \ref domain_decomposition.c, \ref grid.c, \ref interaction_data.c,
    \ref layered.c, \ref statistics_chain.c

    <li> \verbatim #define ELECTROSTATICS \endverbatim
    Compiler flag to enable charges and the various electrostatics algorithms.

    <li> \verbatim #define ROTATION \endverbatim
    Compiler flag to enable describing and processing particle orientations.
    This will allow to use such particle properties as quart, omega, and torque.
    
    <li> \verbatim #define EXTERNAL_FORCES \endverbatim
    Compiler flag to enable external forces. E.g. apply a fixed external force
    to a particle or fix a particle in space.

    <li> \verbatim #define CONSTRAINTS \endverbatim
    Compiler Flag to enable constraints, eg walls, spheres.
    See \ref constraint.h and \ref interaction_data.h

    <li> \verbatim #define MASS \endverbatim
    allow particles to have different masses.

    <li> \verbatim #define EXCLUSIONS \endverbatim
    exclusion of nonbonded interactions for specific particle pairs
    currently works only with domain decomposition and Verlet lists

    <li> \verbatim #define COMFORCE \endverbatim
    Compiler Flag to enable COMFIXED potential

    <li> \verbatim #define COMFIXED \endverbatim
    Compiler Flag to enable COMFIXED potential

    <li> \verbatim #define BOND_CONSTRAINT \endverbatim
    Compiler Flag to enable bond constraint.
    See \ref rattle.h, merged but not tested. 
    If you need this, I wish you happy debugging.

    </ul>

    <h2> Switches for the available short ranged potentials </h2>
    For optimization it might be useful to switch off the ones you don't need
    <ul>
    <li> \verbatim #define  LENNARD_JONES \endverbatim
    Lennard-Jones potential

    <li> \verbatim #define  LJ_WARN_WHEN_CLOSE \endverbatim
    if defined, you will get a warning when particles approach nearer than
    0.9 sigma, because then it's likely the integration will blow up.
<<<<<<< config.h
*/
#define LJ_WARN_WHEN_CLOSE
=======
>>>>>>> 2.88

    <li> \verbatim #define MORSE \endverbatim
    Morse potential

    <li> \verbatim #define LJCOS \endverbatim
    Lennard-Jones potential with cosine tail

    <li> \verbatim #define BUCKINGHAM \endverbatim
    Buckingham potential
    
    <li> \verbatim #define SOFT_SPHERE \endverbatim
    Soft sphere potential

    </ul>

    <h2> Options for the style of the angle potential </h2>
    <ul>
    <li> \verbatim #define BOND_ANGLE_HARMONIC \endverbatim
    Harmonic bond angle potential:      V = 1/2 k (phi - phi0)^2

    <li> \verbatim #define BOND_ANGLE_COSINE \endverbatim
    Cosine bond angle potential:        V = k (1+cos(phi-phi0))

    <li> \verbatim #define BOND_ANGLE_COSSQUARE \endverbatim
    Cosine square bond angle potential: V = 1/2 k (cos(phi)-cos(phi0))^2

    </ul>

    <h2> Switches for simulation methods, Integrators and Thermostats </h2>
    <ul>
    <li> \verbatim #define NEMD \endverbatim
    NEMD (Non Eqilibrium Molecular Dynamics).
    This is used to perform shear simulations

    <li> \verbatim #define NPT \endverbatim
    Allows to use (N,p,T)-ensembles during integration as well

    <li> \verbatim #define DPD \endverbatim
    DPD Thermostat (Dissipative Particle Dynamics) 
    Flag needed only because DPD acts like a short range potential

<<<<<<< config.h
This will allow to use such particle properties as quart, omega, and torque. */
#define ROTATION
=======
    <li> \verbatim #define LB \endverbatim
    LB Thermostat (fluctuating Lattice Boltzmann fluid)
>>>>>>> 2.88

    </ul>
*/

#include <archconfig.h>

#define PARTIAL_PERIODIC
#define ELECTROSTATICS
/* #define ROTATION */
#define EXTERNAL_FORCES
#define CONSTRAINTS
<<<<<<< config.h

/** allow particles to have different masses. */
#define MASS

/** exclusion of nonbonded interactions for specific particle pairs
    currently works only with domain decomposition and Verlet lists */
#define EXCLUSIONS

/** Compiler Flag to enable COMFORCE potential */
#define COMFORCE

/** Compiler Flag to enable COMFIXED potential */
#define COMFIXED

/** Compiler Flag to enable bond constraint.
    See \ref rattle.h, merged but not tested. 
    If you need this, I wish you happy debugging. */
#define BOND_CONSTRAINT
=======
/* #define MASS */
/* #define EXCLUSIONS */
/* #define COMFORCE */
/* #define COMFIXED */
/* #define BOND_CONSTRAINT */
>>>>>>> 2.88

<<<<<<< config.h
/************************************************/
/** \name available short--ranged potentials
    For optimization it might be useful to switch
    off the ones you don't need */
/*@{*/

/** to use tabulated potential*/
#define TABULATED

/** Lennard-Jones */
=======
/* #define TABULATED */
>>>>>>> 2.88
#define LENNARD_JONES
<<<<<<< config.h

/** Morse */
#define MORSE

/** Lennard-Jones with cosine tail */
#define LJCOS

/** BUCKINGHAM potential */
#define BUCKINGHAM

/** SOFT-SPHERE potential */
#define SOFT_SPHERE
/*@}*/
=======
/* #define LJ_WARN_WHEN_CLOSE */
/* #define MORSE */
/* #define LJCOS */
/* #define BUCKINGHAM */
/* #define SOFT_SPHERE */
>>>>>>> 2.88

/* #define BOND_ANGLE_HARMONIC */
<<<<<<< config.h
/** Cosine bond angle potential:        V = k (1+cos(phi-phi0)) */
/* #define BOND_ANGLE_COSINE */
/** Cosine square bond angle potential: V = 1/2 k (cos(phi)-cos(phi0))^2 */
#define BOND_ANGLE_COSSQUARE
=======
#define BOND_ANGLE_COSINE
/* #define BOND_ANGLE_COSSQUARE */
>>>>>>> 2.88

<<<<<<< config.h
/*@}*/

/***********************************************************/
/** \name Simulation methods, Integrators and Thermostats  */
/***********************************************************/
/*@{*/

/** NEMD (Non Eqilibrium Molecular Dynamics).
    This is used to perform shear simulations */
#define NEMD

/** Allows to use (N,p,T)-ensembles during integration as well */
#define NPT

/** DPD Thermostat (Dissipative Particle Dynamics) 
    Flag needed only because DPD acts like a short range potential
*/
#define DPD

/** LB Thermostat (fluctuating Lattice Boltzmann fluid) 
*/
#define LB
=======
/* #define NEMD */
/* #define NPT */
/* #define DPD */
/* #define LB */
>>>>>>> 2.88

/************************************************/
/** \name Default Parameter Settings            */
/************************************************/
/*@{*/

/** CELLS: Default value for the maximal number of cells per node. */
#define CELLS_MAX_NUM_CELLS 216

/** P3M: Default for number of interpolation points of the charge
    assignment function. */
#define P3M_N_INTERPOL 32768

/** P3M: Default for boundary condition: Epsilon of the surrounding
    medium. */
#define P3M_EPSILON 0.0

/** P3M: Default for offset of first mesh point from the origin (left
    down corner of the simulation box. */
#define P3M_MESHOFF 0.5

/** P3M: Default for the number of Brillouin zones taken into account
    in the calculation of the optimal influence function (aliasing
    sums). */
#define P3M_BRILLOUIN 1

/** P3M: Maximal mesh size that will be checked. The current setting
         limits the memory consumption to below 1GB, which is probably
	 reasonable for a while. */
#define P3M_MAX_MESH 128

/** Precision for capture of round off errors. */
#define ROUND_ERROR_PREC 1.0e-14

/** Tiny angle cutoff for sinus calculations */
#define TINY_SIN_VALUE 1e-10
/** Tiny angle cutoff for cosine calculations */
#define TINY_COS_VALUE 0.9999999999

/** maximal number of iterations in the RATTLE algorithm before it bails out. */
#define SHAKE_MAX_ITERATIONS 1000

/*@}*/

/********************************************/
/* \name exported functions of config.c     */
/********************************************/
/*@{*/
#include <tcl.h>

/** callback for version status. */
int version_callback(Tcl_Interp *interp);

/** callback for compilation status. */
int compilation_callback(Tcl_Interp *interp);
/*@}*/

#endif
