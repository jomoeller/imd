/******************************************************************************
*
* imd_mpi_misc.c -- misc mpi routines
*
******************************************************************************/

/******************************************************************************
* $RCSfile$
* $Revision$
* $Date$
******************************************************************************/

#include "imd.h"

/******************************************************************************
*
* isend_buf lean version of send_cell for force_loop
*
******************************************************************************/

int isend_buf(msgbuf *b, int to_cpu, MPI_Request *req)
{
#ifdef PACX
  return MPI_Isend(b->data, b->n, MPI_REAL, to_cpu, b->n, cpugrid, req);
#else
  return MPI_Isend(b->data, b->n, MPI_REAL, to_cpu, BUFFER_TAG, cpugrid, req);
#endif
}


/******************************************************************************
*
* irecv_buf lean version of recv_cell for force_loop
*
******************************************************************************/

int irecv_buf(msgbuf *b, int from, MPI_Request *req)
{
#ifdef PACX
  return MPI_Irecv(b->data,b->n_max,MPI_REAL,from,MPI_ANY_TAG,cpugrid,req);
#else
  return MPI_Irecv(b->data,b->n_max,MPI_REAL,from,BUFFER_TAG,cpugrid,req);
#endif
}


/******************************************************************************
*
* mpi_addtime  Increments the various timers
*
******************************************************************************/

void mpi_addtime(double *timer)
{
  double time_now;

  time_now  = MPI_Wtime();
  *timer   += (time_now - time_last);
  time_last = time_now;
}


/******************************************************************************
*
* copy_atoms copies atoms into the mpi message buffer
*
******************************************************************************/

#ifdef TWOD
void copy_atoms(msgbuf *b, int k, int l)
#else
void copy_atoms(msgbuf *b, int k, int l, int m)
#endif
{
  int i,j;
  real *ptr;
  cell *from;

#ifdef TWOD
  from = PTR_2D_V(cell_array, k, l, cell_dim);
#else
  from = PTR_3D_V(cell_array, k, l, m, cell_dim);
#endif
  
  if ((b->n_max - BINC * CSTEP) < (b->n + BINC * from->n)) 
    error("Buffer overflow in copy_atoms.");

  for (i=0; i<from->n; ++i) {
    b->data[ b->n++ ] = from->ort X(i);
    b->data[ b->n++ ] = from->ort Y(i);
#ifndef TWOD
    b->data[ b->n++ ] = from->ort Z(i);
#endif
#ifndef MONOLJ
    b->data[ b->n++ ] = from->sorte[i];
#endif
#ifdef EAM
    b->data[ b->n++ ] = from->nummer[i];
#endif
  }
}


/**********************************************************************
*
* moves an atom from one cell into a buffer
*
***********************************************************************/

void copy_one_atom(msgbuf *to, cell *from, int index)
{
  /* Check the parameters */
  if ((0 > index) || (index >= from->n)) {
    printf("%d: i %d n %d\n",myid,index,from->n);
    error("copy_one_atom: index argument out of range.");
  };
  
  /* See if we need some space */
  if (to->n >= to->n_max) error("copy_one_atom: buffer overflow.");

  /* copy atom */
  to->data[ to->n++ ] = from->ort X(index); 
  to->data[ to->n++ ] = from->ort Y(index); 
#ifndef TWOD
  to->data[ to->n++ ] = from->ort Z(index); 
#endif
#ifndef MONOLJ
  to->data[ to->n++ ] = from->sorte[index];
  to->data[ to->n++ ] = from->masse[index];
  to->data[ to->n++ ] = from->nummer[index];
#ifdef REFPOS
  to->data[ to->n++ ] = from->refpos X(index);
  to->data[ to->n++ ] = from->refpos Y(index);
#ifndef TWOD
  to->data[ to->n++ ] = from->refpos Z(index);
#endif
#endif
#ifdef DISLOC
  to->data[ to->n++ ] = from->Epot_ref[index];
  to->data[ to->n++ ] = from->ort_ref X(index); 
  to->data[ to->n++ ] = from->ort_ref Y(index); 
#ifndef TWOD
  to->data[ to->n++ ] = from->ort_ref Z(index); 
#endif
#endif
#endif

  to->data[ to->n++ ] = from->impuls X(index); 
  to->data[ to->n++ ] = from->impuls Y(index); 
#ifndef TWOD
  to->data[ to->n++ ] = from->impuls Z(index); 
#endif

  /* Delete atom in original cell */

  --from->n;

  if (0 < from->n) {

    from->ort X(index) = from->ort X(from->n); 
    from->ort Y(index) = from->ort Y(from->n); 
#ifndef TWOD
    from->ort Z(index) = from->ort Z(from->n); 
#endif

    from->kraft X(index) = from->kraft X(from->n); 
    from->kraft Y(index) = from->kraft Y(from->n); 
#ifndef TWOD
    from->kraft Z(index) = from->kraft Z(from->n); 
#endif

    from->impuls X(index) = from->impuls X(from->n); 
    from->impuls Y(index) = from->impuls Y(from->n); 
#ifndef TWOD
    from->impuls Z(index) = from->impuls Z(from->n); 
#endif
#ifndef MONOLJ    
    from->pot_eng[index] = from->pot_eng[from->n]; 
    from->masse[index] = from->masse[from->n]; 
    from->sorte[index] = from->sorte[from->n];
    from->nummer[index] = from->nummer[from->n]; 
#ifdef REFPOS
    from->refpos X(index) = from->refpos X(from->n);
    from->refpos Y(index) = from->refpos Y(from->n);
#ifndef TWOD
    from->refpos Z(index) = from->refpos Z(from->n);
#endif
#endif
#ifdef DISLOC
    from->Epot_ref[index]    = from->Epot_ref[from->n]; 
    from->ort_ref X(index)   = from->ort_ref X(from->n); 
    from->ort_ref Y(index)   = from->ort_ref Y(from->n); 
#ifndef TWOD
    from->ort_ref Z(index)   = from->ort_ref Z(from->n); 
#endif
#endif
#endif
  }
}


/******************************************************************************
*
*  process buffer sorts particles from mpi buffer into buffer cells
*  buffers have been filled with copy_atoms or copy_one_atom
*
******************************************************************************/

void process_buffer(msgbuf *b, int mode)
{
  ivektor coord, coord2;
  int to_cpu;
  static cell *input = NULL;
  int j;
  
  if (NULL == input) {
    input = (cell *) malloc(sizeof(cell));
    if (0==input) error("Cannot allocate buffer cell.");
    input->n_max=0;
    alloc_cell(input, 1);
  }

  j=0;
  if (b->n > 0) do {     
    input->n        = 1;
    input->ort X(0) = b->data[j++];
    input->ort Y(0) = b->data[j++];
#ifndef TWOD
    input->ort Z(0) = b->data[j++];
#endif
#ifndef MONOLJ
    input->sorte[0] = b->data[j++];
#endif
#ifdef EAM
    if (mode == FORCE) {
      input->nummer[0] = b->data[j++];
    }
#endif

#ifdef TWOD
    coord = local_cell_coord( input->ort X(0), input->ort Y(0) );
#else
    coord = local_cell_coord( input->ort X(0), input->ort Y(0), 
                                               input->ort Z(0) );
#endif

    if (FORCE!=mode) {

      /* get the rest of the data */
#ifndef MONOLJ
      input->masse[0]        = b->data[j++];
      input->nummer[0]       = b->data[j++];
#ifdef REFPOS
      input->refpos X(0)     = b->data[j++];
      input->refpos Y(0)     = b->data[j++];
#ifndef TWOD
      input->refpos Z(0)     = b->data[j++];
#endif
#endif
#ifdef DISLOC
      input->Epot_ref[0]     = b->data[j++];
      input->ort_ref X(0)    = b->data[j++];
      input->ort_ref Y(0)    = b->data[j++];
#ifndef TWOD
      input->ort_ref Z(0)    = b->data[j++];
#endif
#endif
#endif
      input->impuls X(0)     = b->data[j++];
      input->impuls Y(0)     = b->data[j++];
#ifndef TWOD
      input->impuls Z(0)     = b->data[j++];
#endif

#ifdef TWOD
    coord2 = cell_coord( input->ort X(0), input->ort Y(0) );
#else
    coord2 = cell_coord( input->ort X(0), input->ort Y(0), input->ort Z(0) );
#endif

      to_cpu = cpu_coord( coord2 );
      if (to_cpu == myid) move_atom(coord, input, 0);

    } else {

      /* Apply pbc */
      if (coord.x < 0)                  coord.x += global_cell_dim.x;
      if (coord.x >= global_cell_dim.x) coord.x -= global_cell_dim.x;
      if (coord.y < 0)                  coord.y += global_cell_dim.y;
      if (coord.y >= global_cell_dim.y) coord.y -= global_cell_dim.y;
#ifndef TWOD
      if (coord.z < 0)                  coord.z += global_cell_dim.z;
      if (coord.z >= global_cell_dim.z) coord.z -= global_cell_dim.z;
#endif
      /* Check if atom is on my CPU */
      if (
	  /* Check for buffer cell */
	  ((0 == coord.x) || ((cell_dim.x-1) == coord.x) 
	   || (0 == coord.y) || ((cell_dim.y-1) == coord.y) 
#ifndef TWOD
	   || (0 == coord.z) || ((cell_dim.z-1) == coord.z)
#endif
	   )

	  &&

	  /* Check for my CPU */
	  ((0 <= coord.x) && (cell_dim.x > coord.x) 
	   && (0 <= coord.y) && (cell_dim.y > coord.y) 
#ifndef TWOD
	   && (0 <= coord.z) && (cell_dim.z > coord.z)
#endif
	  )) move_atom(coord, input, 0);
    };
  } while (j<b->n);

}


/******************************************************************************
*
* copy_atoms_buf copies atoms from one message buffer to another
*
******************************************************************************/

void copy_atoms_buf(msgbuf *to, msgbuf *from)
{
  int i;

  if ((to->n_max - BINC * CSTEP) < (to->n + from->n)) 
      error("Buffer overflow in copy_buf.");

  for (i=0; i<from->n; ++i) 
    to->data[ to->n++ ] = from->data[i];
}


/******************************************************************************
*
* setup_buffers sets up the send/receive buffers
*
* This is called periodically to check the buffers are large enough
*
* The buffers never shrink. 
* 
******************************************************************************/

void setup_buffers(void)
{
  int largest_cell, largest_local_cell=0;
  int k;
  int size_east;
  int size_north;
  int size_up;

  /* Find largest cell */
  for (k=0; k<ncells; ++k) {
    int n;
    n = (cell_array + CELLS(k))->n;
    if (largest_local_cell < n) largest_local_cell = n;
  }

  MPI_Allreduce( &largest_local_cell, &largest_cell, 1, 
                 MPI_INT, MPI_MAX, cpugrid);

  /* Add security */
  largest_cell += CSTEP;

#ifndef TWOD
  size_east  = largest_cell * cell_dim.y * cell_dim.z * BINC;
  size_north = largest_cell * cell_dim.x * cell_dim.z * BINC;
  size_up    = largest_cell * cell_dim.x * cell_dim.y * BINC;
#ifdef DEBUG
  if (0==myid) 
     printf("Max. cell is %d size east %d size north %d size up %d.\n", 
		      largest_cell,size_east,size_north,size_up);
#endif
#else
  size_east  = largest_cell * cell_dim.y * BINC;
  size_north = largest_cell * cell_dim.x * BINC;
#ifdef DEBUG
  if (0==myid) printf("Max. cell is %d size east %d size north %d.\n", 
		      largest_cell,size_east,size_north);
#endif
#endif

  /* Allocate east/west buffers */
  if (size_east > send_buf_east.n_max) {

    /* Buffer size is zero only at startup */
    if (0 != send_buf_east.n_max) {
      free(send_buf_east.data);
      free(send_buf_west.data);
      free(recv_buf_east.data);
      free(recv_buf_west.data);
    };

    /* Allocate buffers */
    send_buf_east.data = (real *) malloc( size_east * sizeof(real) );
    send_buf_west.data = (real *) malloc( size_east * sizeof(real) );
    recv_buf_east.data = (real *) malloc( size_east * sizeof(real) );
    recv_buf_west.data = (real *) malloc( size_east * sizeof(real) );

    /* Always check malloc results */
    if ((NULL == send_buf_east.data ) ||
	(NULL == send_buf_west.data ) ||
	(NULL == recv_buf_east.data ) ||
	(NULL == recv_buf_west.data )) 
        error("Can't allocate east/west buffer.");

    send_buf_east.n_max = size_east;
    send_buf_west.n_max = size_east;
    recv_buf_east.n_max = size_east;
    recv_buf_west.n_max = size_east;
  };

  /* Allocate north/south buffers */
  if (size_north > send_buf_north.n_max) {

    /* Buffer size is zero only at startup */
    if (0 != send_buf_north.n_max) {
      free(send_buf_north.data);
      free(send_buf_south.data);
      free(recv_buf_north.data);
      free(recv_buf_south.data);
    };

    /* Allocate buffers */
    send_buf_north.data = (real *) malloc( size_north * sizeof(real) );
    send_buf_south.data = (real *) malloc( size_north * sizeof(real) );
    recv_buf_north.data = (real *) malloc( size_north * sizeof(real) );
    recv_buf_south.data = (real *) malloc( size_north * sizeof(real) );

    /* Always check malloc results */
    if ((NULL == send_buf_north.data ) ||
	(NULL == send_buf_south.data ) ||
	(NULL == recv_buf_north.data ) ||
	(NULL == recv_buf_south.data )) 
        error("Can't allocate north/south buffer.");

    send_buf_north.n_max = size_north;
    send_buf_south.n_max = size_north;
    recv_buf_north.n_max = size_north;
    recv_buf_south.n_max = size_north;

  };

#ifndef TWOD
  /* Allocate up/down buffers */
  if (size_up > send_buf_up.n_max) {

    /* Buffer size is zero only at startup */
    if (0 != send_buf_up.n_max) {
      free(send_buf_up.data);
      free(send_buf_down.data);
      free(recv_buf_up.data);
      free(recv_buf_down.data);
    };

    /* Allocate buffers */
    send_buf_up.data   = (real *) malloc( size_up * sizeof(real) );
    send_buf_down.data = (real *) malloc( size_up * sizeof(real) );
    recv_buf_up.data   = (real *) malloc( size_up * sizeof(real) );
    recv_buf_down.data = (real *) malloc( size_up * sizeof(real) );

    /* Always check malloc results */
    if ((NULL == send_buf_up.data ) ||
	(NULL == send_buf_down.data ) ||
	(NULL == recv_buf_up.data ) ||
	(NULL == recv_buf_down.data )) 
        error("Can't allocate up/down buffer.");

    send_buf_up.n_max   = size_up;
    send_buf_down.n_max = size_up;
    recv_buf_up.n_max   = size_up;
    recv_buf_down.n_max = size_up;

  }

#endif

}


/******************************************************************************
*
* empty buffer cells -- set number of atoms in buffer cells to zero
*
******************************************************************************/

void empty_buffer_cells(void)
{
  cell *p;
  int i,j;

#ifdef TWOD
  /* xz surface */
  for (i=0; i < cell_dim.x; ++i) {
    p = PTR_2D_V(cell_array, i, cell_dim.y-1, cell_dim);
    p->n = 0;
    p = PTR_2D_V(cell_array, i,            0, cell_dim);
    p->n = 0;
  };

  /* yz surface */
  for (i=0; i < cell_dim.y; ++i) {
    p = PTR_2D_V(cell_array, cell_dim.x-1, i, cell_dim);
    p->n = 0;
    p = PTR_2D_V(cell_array,            0, i, cell_dim);
    p->n = 0;
  };
#else
  /* empty the buffer cells */
  /* xy surface */
  for (i=0; i < cell_dim.x; ++i) 
    for (j=0; j < cell_dim.y; ++j) {
      p = PTR_3D_V(cell_array, i, j, cell_dim.z-1, cell_dim);
      p->n = 0;
      p = PTR_3D_V(cell_array, i, j,            0, cell_dim);
      p->n = 0;
    };

  /* xz surface */
  for (i=0; i < cell_dim.x; ++i) 
    for (j=0; j < cell_dim.z; ++j) {
      p = PTR_3D_V(cell_array, i, cell_dim.y-1, j, cell_dim);
      p->n = 0;
      p = PTR_3D_V(cell_array, i,            0, j, cell_dim);
      p->n = 0;
    };

  /* yz surface */
  for (i=0; i < cell_dim.y; ++i) 
    for (j=0; j < cell_dim.z; ++j) {
      p = PTR_3D_V(cell_array, cell_dim.x-1, i, j, cell_dim);
      p->n = 0;
      p = PTR_3D_V(cell_array,            0, i, j, cell_dim);
      p->n = 0;
    };
#endif

}


/******************************************************************************
*
* empty mpi buffers -- set number of atoms in mpi buffers to zero
*
******************************************************************************/

void empty_mpi_buffers(void)
{
 /* Empty MPI buffers */
  send_buf_north.n = 0;
  send_buf_south.n = 0;
  send_buf_east.n  = 0;
  send_buf_west.n  = 0;

  recv_buf_north.n = 0;
  recv_buf_south.n = 0;
  recv_buf_east.n  = 0;
  recv_buf_west.n  = 0;

#ifndef TWOD
  recv_buf_down.n  = 0;
  recv_buf_up.n    = 0;
  send_buf_down.n  = 0;
  send_buf_up.n    = 0;
#endif

}


/******************************************************************************
*
* send_cell send cell data to another process
*
******************************************************************************/

void send_cell(cell *p, int to_cpu, int tag)
{
#ifdef PACX
  MPI_Send( &(p->n), 1,           MPI_INT,  to_cpu, tag + SIZE_TAG,   cpugrid);
#endif

  MPI_Ssend( p->ort, DIM*p->n,    MPI_REAL, to_cpu, tag + ORT_TAG,    cpugrid);

#ifndef MONOLJ
  MPI_Ssend( p->nummer, p->n,     INTEGER,  to_cpu, tag + NUMMER_TAG, cpugrid);
  MPI_Ssend( p->sorte,  p->n,     SHORT,    to_cpu, tag + SORTE_TAG,  cpugrid);
  MPI_Ssend( p->masse,  p->n,     MPI_REAL, to_cpu, tag + MASSE_TAG,  cpugrid);
  MPI_Ssend( p->pot_eng,p->n,     MPI_REAL, to_cpu, tag + POT_TAG,    cpugrid);
#ifdef REFPOS
  MPI_Ssend( p->refpos, DIM*p->n, MPI_REAL, to_cpu, tag + REFPOS_TAG, cpugrid);
#endif
#ifdef DISLOC
  MPI_Ssend( p->Epot_ref,p->n,    MPI_REAL, to_cpu, tag + POT_REF_TAG,cpugrid);
  MPI_Ssend( p->ort_ref,DIM*p->n, MPI_REAL, to_cpu, tag + ORT_REF_TAG,cpugrid);
#endif
#endif

  MPI_Ssend( p->impuls, DIM*p->n, MPI_REAL, to_cpu, tag + IMPULS_TAG, cpugrid);
  MPI_Ssend( p->kraft,  DIM*p->n, MPI_REAL, to_cpu, tag + KRAFT_TAG,  cpugrid);
}


/******************************************************************************
*
* recv_cell receive cell data for cell
*
******************************************************************************/

void recv_cell(cell *p, int from_cpu,int tag)
{
  int size;
  int newsize;
  MPI_Status status;

#ifdef PACX
  MPI_Recv( &size, 1, MPI_INT, from_cpu, tag + SIZE_TAG , cpugrid, &status );
#else
  MPI_Probe( from_cpu, tag + ORT_TAG, cpugrid, &status );
  MPI_Get_count( &status, MPI_REAL, &size );
  size /= DIM;
#endif

  /* realloc cell if necessary */
  newsize = p->n_max; 
  while ( newsize < size ) newsize += incrsz;
  if (newsize > p->n_max) {
    /* Inihibit superfluous copy operation */
    p->n = 0;
    alloc_cell(p,newsize);
  }
  p->n = size;

  MPI_Recv(p->ort,     DIM * size, MPI_REAL, from_cpu, tag + ORT_TAG,
                                             cpugrid, &status );
#ifndef MONOLJ
  MPI_Recv(p->nummer,        size, INTEGER,  from_cpu, tag + NUMMER_TAG, 
                                             cpugrid, &status);
  MPI_Recv(p->sorte,         size, SHORT,    from_cpu, tag + SORTE_TAG , 
                                             cpugrid, &status);
  MPI_Recv(p->masse,         size, MPI_REAL, from_cpu, tag + MASSE_TAG , 
                                             cpugrid, &status);
  MPI_Recv(p->pot_eng,       size, MPI_REAL, from_cpu, tag + POT_TAG, 
                                             cpugrid, &status);
#ifdef REFPOS
  MPI_Recv(p->refpos,  DIM * size, MPI_REAL, from_cpu, tag + REFPOS_TAG, 
                                             cpugrid, &status);
#endif
#ifdef DISLOC
  MPI_Recv(p->Epot_ref,      size, MPI_REAL, from_cpu, tag + POT_REF_TAG , 
                                             cpugrid, &status);
  MPI_Recv(p->ort_ref, DIM * size, MPI_REAL, from_cpu, tag + ORT_REF_TAG, 
                                             cpugrid, &status);
#endif
#endif
  MPI_Recv(p->impuls,  DIM * size, MPI_REAL, from_cpu, tag + IMPULS_TAG, 
                                             cpugrid, &status);
  MPI_Recv(p->kraft,   DIM * size, MPI_REAL, from_cpu, tag + KRAFT_TAG,  
                                             cpugrid, &status);

}






