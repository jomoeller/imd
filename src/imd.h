/******************************************************************************
*
* IMD -- The ITAP Molecular Dynamics Program
*
* Copyright 1996-2007 Institute for Theoretical and Applied Physics,
* University of Stuttgart, D-70550 Stuttgart
*
******************************************************************************/

/******************************************************************************
*
* imd.h -- Header file for all modules of IMD
*
******************************************************************************/

/******************************************************************************
* $Revision$
* $Date$
******************************************************************************/

/* C stuff */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef CBE
#define USE_WALLTIME
#endif

/* support for timers */
#ifndef MPI
#if defined(USE_RUSAGE) || defined(USE_WALLTIME)
#include <sys/time.h>
#include <sys/resource.h>
#else
#include <sys/times.h>
#include <sys/types.h>
#endif
#endif

/* Machine specific headers */
#ifdef MPI
#include <mpi.h>
#ifdef MPE
#include <mpe.h>
#endif
#endif
#ifdef OMP
#include <omp.h>
#endif

/* FFT for diffraction patterns */
#ifdef DIFFPAT
#include <fftw3.h>
#endif

/* IMD version */
#include "version.h"

/* Configuration */
#include "config.h"

/* Data types */
#include "types.h"

/* Some makros */
#include "makros.h"

/* Function Prototypes */
#include "prototypes.h"

/* Global Variables */
#include "globals.h"
