
/******************************************************************************
*
* makros.h -- Some useful makros for the imd Package
*
******************************************************************************/

/******************************************************************************
* $Revision$
* $Date$
******************************************************************************/

/* these macros should allow to avoid MONOLJ in many places */
#ifdef MONOLJ
#define SORTE(cell,i) 0
#define NUMMER(cell,i) 0
#define MASSE(cell,i) 1.0
#define POTENG(cell,i) 0.0
#else
#define SORTE(cell,i) (((cell)->sorte[(i)])%ntypes)
#define NUMMER(cell,i) (cell)->nummer[(i)]
#define MASSE(cell,i) (cell)->masse[(i)]
#define POTENG(cell,i) (cell)->pot_eng[(i)]
#endif
#ifdef ORDPAR
#define NBANZ(cell,i) (cell)->nbanz[(i)]
#endif

#ifdef MPI
#define CELLS(k) cells[k]
#else
#define CELLS(k) k
#endif

/* Max gibt den groesseren von zwei Werten */
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Min gibt den kleineren  von zwei Werten */
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* Sqr quadriert sein Argument */
#if defined(__GNUC__) || defined(__SASC)
inline static real SQR(real x)
{
  return x*x;
}
#else
#define SQR(a) ((a)*(a))
#endif

/* Abs berechnet den Betrag einer Zahl */
#define ABS(a) ((a) >0 ? (a) : -(a))

/* How many dimension are there? */
#ifdef TWOD
#define DIM 2
#else
#define DIM 3
#endif

/* Sometimes we use array where we should use vectors but... */
#define X(i) [DIM*(i)  ]
#define Y(i) [DIM*(i)+1]
#define Z(i) [DIM*(i)+2]

/* Skalarprodukt */
/* Vectors */
#define SPROD3D(a,b) (((a).x * (b).x) + ((a).y * (b).y) + ((a).z * (b).z))
#define SPROD2D(a,b) (((a).x * (b).x) + ((a).y * (b).y))
/* Arrays */
#define SPRODN3D(a,i,b,j) (((a)X(i) * (b)X(j)) + ((a)Y(i) * (b)Y(j)) + ((a)Z(i) * (b)Z(j)))
#define SPRODN2D(a,i,b,j) (((a)X(i) * (b)X(j)) + ((a)Y(i) * (b)Y(j)) )
/* Mixed Arrray, Vector */
#define SPRODX3D(a,i,b) (((a)X(i) * (b).x) + ((a)Y(i) * (b).y) + ((a)Z(i) * (b).z))
#define SPRODX2D(a,i,b) (((a)X(i) * (b).x) + ((a)Y(i) * (b).y) )
                           

#ifdef TWOD
#define SPROD(a,b)         SPROD2D(a,b)
#define SPRODN(a,i,b,j)    SPRODN2D(a,i,b,j)
#define SPRODX(a,i,b)      SPRODX2D(a,i,b)
#else
#define SPROD(a,b) SPROD3D(a,b)
#define SPRODN(a,i,b,j)    SPRODN3D(a,i,b,j)
#define SPRODX(a,i,b)      SPRODX3D(a,i,b)
#endif

/* Dynamically allocated 3D arrray -- sort of */
#define PTR_3D(var,i,j,k,dim_i,dim_j,dim_k) (((var) + \
                                            ((i)*(dim_j)*(dim_k)) + \
                                            ((j)*(dim_k)) + \
                                             (k)))

/* Dynamically allocated 3D arrray -- half vector version */
#define PTR_3D_V(var,i,j,k,dim) (((var) + \
                                 ((i)*(dim.y)*(dim.z)) + \
                                 ((j)*(dim.z)) + \
                                 (k)))

/* Dynamically allocated 3D arrray -- full vector version */
#define PTR_3D_VV(var,coord,dim) (((var) + \
                                 ((coord.x)*(dim.y)*(dim.z)) + \
                                 ((coord.y)*(dim.z)) + \
                                 (coord.z)))


/* Dynamically allocated 2D arrray -- sort of */
#define PTR_2D(var,i,j,dim_i,dim_j) (((var) + \
                                   ((i)*(dim_j)) + \
                                     (j)))
                                         

/* Dynamically allocated 2D arrray -- half vector version */
#define PTR_2D_V(var,i,j,dim) (((var) + \
                               ((i)*(dim.y)) + \
                                (j)))
                                

/* Dynamically allocated 2D arrray -- full vector version */
#define PTR_2D_VV(var,coord,dim) (((var) + \
                                 ((coord.x)*(dim.y)) + \
                                  (coord.y)))



#ifdef TWOD

#define PTR     PTR_2D
#define PTR_V   PTR_2D_V
#define PTR_VV  PTR_2D_VV

#else

#define PTR     PTR_3D
#define PTR_V   PTR_3D_V
#define PTR_VV  PTR_3D_VV

#endif


/* simulation ensembles */
#define ENS_EMPTY     0
#define ENS_NVE       1
#define ENS_MIK       2
#define ENS_NVT       3
#define ENS_NPT_ISO   4
#define ENS_NPT_AXIAL 5
#define ENS_MC        7
#define ENS_FRAC      8
#define ENS_NVX      11
#define ENS_STM      13

/* All the logic in this program */
#define TRUE         1
#define FALSE        0

/* Some constants for Message passing, should all have unique values */
#ifdef MPI
#define CELL_TAG   100
#define SIZE_TAG   200
#define BUFFER_TAG 300
#define ORT_TAG      1
#define SORTE_TAG    2
#define MASSE_TAG    3 
#define NUMMER_TAG   4
#define IMPULS_TAG   5
#define KRAFT_TAG    6
#define POT_TAG      7
#define POT_INIT_TAG 8 /* obsolete */
#define POT_REF_TAG  9
#define ORT_REF_TAG  10
#define REFPOS_TAG   11
#define TRAEG_MOMENT_TAG 12
#define ACHSE_TAG 13
#define SHAPE_TAG 14
#define POT_WELL_TAG 15
#define DREH_IMPULS_TAG 16
#define DREH_MOMENT_TAG 17
#define NBA_TAG 18
#endif

/* some systems have different versions of trunc and floor float and double */
#if sgi || t3e
#ifdef DOUBLE
#define FLOOR floor
#else
#define FLOOR floorf
#endif
#else
#define FLOOR floor
#endif

#define TRUNC (int)

#ifndef M_PI
#define M_PI 4.0*atan(1.0)
#endif

/* macros for potential table modes */
#define FUNTAB 0
#define POTTAB 1
