
/******************************************************************************
*
* IMD -- The ITAP Molecular Dynamics Program
*
* Copyright 1996-2001 Institute for Theoretical and Applied Physics,
* University of Stuttgart, D-70550 Stuttgart
*
******************************************************************************/

/******************************************************************************
*
* imd_io_3d.c -- 3D-specific IO routines
*
******************************************************************************/

/******************************************************************************
* $Revision$
* $Date$
******************************************************************************/

#include "imd.h"

/******************************************************************************
*
* read_atoms - reads atoms and velocities into the cell-array
*
* The file format is flat ascii, one atom per line, lines beginning
* with '#' denote comments. Each line consists of
*
* number type mass x y z vx vy vz rest
*
* where
*
* number   is an arbitrary integer number assigned to each atom
* type     is the atom's index to the potenital table
* mass     is the mass of the atom
* x,y,z    are the atom's coordinates
* vx,vy,vz are the atom's velocity
* rest     is ignored until end of line
*
******************************************************************************/

void read_atoms(str255 infilename)
{
  cell *input;
  FILE *infile;
  char buf[512];
  str255 msg;
  int p;
  vektor pos;
  vektor vau;
#ifdef DISLOC
  FILE *reffile;
  int pref;
  char refbuf[512];
  real refeng=0;
  real fdummy;
  int refn, idummy;
  vektor3d refpos;
#endif
  real m;
#ifdef UNIAX
  real the;
  vektor axe;
  vektor sig;
  vektor eps;
  vektor ome;
#endif
  int i,s,n;
  cell *to;
  ivektor cellc;
  int to_cpu;
  int addnumber = 0;
#ifdef MPI
  msgbuf *input_buf, *b;
#endif

  /* allocate num_sort and num_vsort on all CPUs */
  if ((num_sort = (int *) calloc(ntypes,sizeof(int)))==NULL)
    error("cannot allocate memory for num_sort\n");
  if ((num_vsort = (int *) calloc(vtypes,sizeof(int)))==NULL)
    error("cannot allocate memory for num_vsort\n");

#ifdef MPI

  /* Try opening a per cpu file first when parallel_input is active */
  if (1==parallel_input) {
    sprintf(buf,"%s.%u",infilename,myid); 
    infile = fopen(buf,"r");
    /* When each cpu reads only part of the atoms, we have to add the
       number of atoms together to get the correct natoms. We set a
       flag here */
    if (NULL!=infile) addnumber=1;
  } else if (0!=myid) {
    recv_atoms();
    /* If CPU 0 found velocities in its data, no initialisation is done */
    MPI_Bcast( &natoms,       1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( &nactive,      1, MPI_INT, 0, MPI_COMM_WORLD);
#ifdef UNIAX
    MPI_Bcast( &nactive_rot,  1, MPI_INT, 0, MPI_COMM_WORLD);
#endif
    MPI_Bcast( num_sort, ntypes, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( num_vsort, vtypes, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( &do_maxwell,   1, MPI_INT, 0, MPI_COMM_WORLD);
    return;
  }

  if ((0==parallel_input) || (NULL==infile)) infile = fopen(infilename,"r");
#ifdef DISLOC
  if (calc_Epot_ref == 0) reffile = fopen(reffilename,"r");
#endif

  /* allocate temporary input buffer */
  if (0==parallel_input) {
    input_buf = (msgbuf *) malloc( num_cpus * sizeof(msgbuf) );
    if (NULL==input_buf) error("cannot allocate input buffers");
    input_buf[0].data  = (real *) NULL;
    input_buf[0].n_max = 0;
    input_buf[0].n     = 0;
    for (i=1; i<=num_cpus; i++) {
      input_buf[i].data  = (real *) malloc( INPUT_BUF_SIZE * sizeof(real) );
      if (NULL==input_buf[i].data) error("cannot allocate input buffer");
      input_buf[i].n_max = INPUT_BUF_SIZE;
      input_buf[i].n     = 0;
    }
  }

#else /* not MPI */

  infile = fopen(infilename,"r");
#ifdef DISLOC
  if (calc_Epot_ref == 0) reffile = fopen(reffilename,"r");
#endif

#endif /* MPI or not MPI */

  if (NULL==infile) {
    sprintf(msg,"File %s not found",infilename);
    error(msg);
  }
#ifdef DISLOC
  if ((calc_Epot_ref == 0) && (NULL==reffile)) {
    sprintf(msg,"File %s not found",reffilename);
    error(msg);
  }
#endif

 /* Set up 1 atom input cell */
  input = (cell *) malloc(sizeof(cell));
  if (0==input) error("Cannot allocate input cell.");
  input->n_max=0;
  alloc_cell(input, 1);

  natoms  = 0;
  nactive = 0;
#ifdef SHOCK
  do_maxwell=1;
#else
  do_maxwell=0;
#endif

  /* Read the input file line by line */
  while(!feof(infile)) {

    buf[0] = '\0';
    fgets(buf,sizeof(buf),infile);
    /* eat comments */
    while ('#'==buf[1]) fgets(buf,sizeof(buf),infile); 

#ifdef DISLOC
    if (calc_Epot_ref == 0) {
      refbuf[0] = '\0';
      fgets(refbuf,sizeof(refbuf),reffile);
      /* eat comments */
      while ('#'==refbuf[1]) fgets(refbuf,sizeof(refbuf),reffile);
    }
#endif

#ifdef UNIAX 

#ifdef DOUBLE
    p = sscanf(buf,"%d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
	      &n,&s,&m,&the,&pos.x,&pos.y,&pos.z,&axe.x,&axe.y,&axe.z,&sig.x,&sig.y,&sig.z,&eps.x,&eps.y,&eps.z,&vau.x,&vau.y,&vau.z,&ome.x,&ome.y,&ome.z);  
    if ( axe.x * axe.x + axe.y * axe.y + axe.z * axe.z 
	 - 1.0 > 1.0e-04 )
      error("Molecular axis not a unit vector!");
    if ( sig.x != sig.y )
      error("This is not a uniaxial molecule!");
    if ( eps.x != eps.y )
      error("This is not a uniaxial molecule!");
#else
    p = sscanf(buf,"%d %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
	      &n,&s,&m,&the,&pos.x,&pos.y,&pos.z,&axe.x,&axe.y,&axe.z,&sig.x,&sig.y,&sig.z,&eps.x,&eps.y,&eps.z,&vau.x,&vau.y,&vau.z,&ome.x,&ome.y,&ome.z);  
    if ( axe.x * axe.x + axe.y * axe.y + axe.z * axe.z 
	 - 1.0 > 1.0e-04 )
      error("Molecular axis not a unit vector!");
    if ( sig.x != sig.y )
      error("This is not a uniaxial molecule!");
    if ( eps.x != eps.y )
      error("This is not a uniaxial molecule!");
#endif

#else /* not UNIAX */

#ifdef DOUBLE
    p = sscanf(buf,"%d %d %lf %lf %lf %lf %lf %lf %lf",
	      &n,&s,&m,&pos.x,&pos.y,&pos.z,&vau.x,&vau.y,&vau.z);  
#else
    p = sscanf(buf,"%d %d %f %f %f %f %f %f %f",
	      &n,&s,&m,&pos.x,&pos.y,&pos.z,&vau.x,&vau.y,&vau.z);
#endif

#endif /* UNIAX or not UNIAX */

    if (p>0) {  /* skip empty lines at end of file */

#ifdef DISLOC
      if (calc_Epot_ref == 0) {
#ifdef DOUBLE
        pref = sscanf(refbuf,"%d %d %lf %lf %lf %lf %lf %lf %lf %lf",
	  	    &refn,&idummy,&fdummy,&refpos.x,&refpos.y,&refpos.z,
                    &fdummy,&fdummy,&fdummy,&refeng);
#else
        pref = sscanf(refbuf,"%d %d %f %f %f %f %f %f %f %f",
		    &refn,&idummy,&fdummy,&refpos.x,&refpos.y,&refpos.z,
                    &fdummy,&fdummy,&fdummy,&refeng);
#endif
        if ((ABS(refn)) != (ABS(n))) {
          printf("%d %d\n", n, refn); 
          error("Numbers in infile and reffile are different.\n");
        }
      }
#endif /* DISLOC */

      pos = back_into_box(pos);

      if (0>=m) error("Mass zero or negative.\n");
#ifdef UNIAX
      if ((p!=16) && (p<22)) error("incorrect line in configuration file.");
#else
      if ((p!=6) && (p<9)) error("incorrect line in configuration file.");
#endif

      input->n = 1;
#ifndef MONOLJ
      NUMMER(input,0) = n;
      VSORTE(input,0) = s;
      MASSE(input,0)  = m;
#endif
      ORT(input,0,X) = pos.x;
      ORT(input,0,Y) = pos.y;
      ORT(input,0,Z) = pos.z;
#ifdef UNIAX
      TRAEG_MOMENT(input,0) = the;
      ACHSE(input,0,X) = axe.x ;
      ACHSE(input,0,Y) = axe.y ;
      ACHSE(input,0,Z) = axe.z ;
      SHAPE(input,0,X) = sig.x ;
      SHAPE(input,0,Y) = sig.y ;
      SHAPE(input,0,Z) = sig.z ;
      POT_WELL(input,0,X) = eps.x ;
      POT_WELL(input,0,Y) = eps.y ;
      POT_WELL(input,0,Z) = eps.z ;
      if (p==16) {
	do_maxwell=1;
	IMPULS(input,0,X) = 0 ;
	IMPULS(input,0,Y) = 0 ;
	IMPULS(input,0,Z) = 0 ;
	DREH_IMPULS(input,0,X) = 0 ;
	DREH_IMPULS(input,0,Y) = 0 ;
	DREH_IMPULS(input,0,Z) = 0 ;
      } else {
	IMPULS(input,0,X) = vau.x * m;
	IMPULS(input,0,Y) = vau.y * m;
	IMPULS(input,0,Z) = vau.z * m;
	DREH_IMPULS(input,0,X) = ome.x * the;
	DREH_IMPULS(input,0,Y) = ome.y * the;
	DREH_IMPULS(input,0,Z) = ome.z * the;
      }
      DREH_MOMENT(input,0,X) = 0 ;
      DREH_MOMENT(input,0,Y) = 0 ;
      DREH_MOMENT(input,0,Z) = 0 ;
#else /* not UNIAX */
      if (p==6) {
        do_maxwell=1;
        IMPULS(input,0,X) = 0;
        IMPULS(input,0,Y) = 0;
        IMPULS(input,0,Z) = 0;
      } else {
        IMPULS(input,0,X) = vau.x * m * (restrictions+s)->x;
        IMPULS(input,0,Y) = vau.y * m * (restrictions+s)->y;
        IMPULS(input,0,Z) = vau.z * m * (restrictions+s)->z;
      }
#endif /* UNIAX or not UNIAX */
      KRAFT(input,0,X) = 0;
      KRAFT(input,0,Y) = 0;
      KRAFT(input,0,Z) = 0;
#ifdef DISLOC
      ORT_REF(input,0,X) = refpos.x;
      ORT_REF(input,0,Y) = refpos.y;
      ORT_REF(input,0,Z) = refpos.z;
      EPOT_REF(input,0)  = refeng;
#endif

#ifdef EPITAX
      /* determine largest atom number of substrate atoms */
      epitax_sub_n = MAX( n, epitax_sub_n );
#endif

      cellc = cell_coord(pos.x,pos.y,pos.z);

#ifdef MPI

      to_cpu = cpu_coord(cellc);
      if ((myid != to_cpu) && (0==parallel_input)) {
        natoms++;
        /* we still have s == input->sorte[0] */
        if (s < ntypes) {
          nactive += DIM;
#ifdef UNIAX
          nactive_rot += 2;
#endif
        } else {
          nactive += (int) (restrictions+s)->x;
          nactive += (int) (restrictions+s)->y;
          nactive += (int) (restrictions+s)->z;
        }
        num_sort[SORTE(input,0)]++;
        num_vsort[VSORTE(input,0)]++;
        b = input_buf + to_cpu;
        copy_one_atom(b, input, 0, 0);
        if (b->n_max - b->n < MAX_ATOM_SIZE) {
          MPI_Send(b->data, b->n, REAL, to_cpu, INBUF_TAG, cpugrid);
          b->n = 0;
        }
      } else if (to_cpu==myid) {
        natoms++;  
        /* we still have s == input->sorte[0] */
        if (s < ntypes) {
          nactive += DIM;
#ifdef UNIAX
        nactive_rot += 2;
#endif
        } else {
          nactive += (int) (restrictions+s)->x;
          nactive += (int) (restrictions+s)->y;
          nactive += (int) (restrictions+s)->z;
        }
        num_sort[SORTE(input,0)]++;
        num_vsort[VSORTE(input,0)]++;
	cellc = local_cell_coord(pos.x,pos.y,pos.z);
        to = PTR_VV(cell_array,cellc,cell_dim);
	move_atom(to, input, 0);
      }

#else /* not MPI */

      natoms++;  
      /* we still have s == input->sorte[0] */
      if (s < ntypes) {
        nactive += DIM;
#ifdef UNIAX
        nactive_rot += 2;
#endif
      } else {
        nactive += (int) (restrictions+s)->x;
        nactive += (int) (restrictions+s)->y;
        nactive += (int) (restrictions+s)->z;
      }
      num_sort[SORTE(input,0)]++;
      num_vsort[VSORTE(input,0)]++;
      to = PTR_VV(cell_array,cellc,cell_dim);
      move_atom(to, input, 0);

#endif /* MPI or not MPI */

    } /* (p>0) */
  } /* !feof(infile) */

  fclose(infile);  
#ifdef DISLOC
  if (calc_Epot_ref==0) fclose(reffile);
#endif

#ifdef MPI

  /* The last buffer is sent with a different tag, which tells the
     target CPU that reading is finished; we increase the size by
     one, so that the buffer is sent even if it is empty */
  if (0==parallel_input)
    for (s=1; s<num_cpus; s++) {
      b = input_buf + s;
      b->data[b->n++] = 0.0;
      MPI_Send(b->data, b->n, REAL, s, INBUF_TAG+1, cpugrid);
      free(b->data);
    }

  /* Add the number of atoms read (and kept) by each CPU */
  if (1==parallel_input) {
    MPI_Allreduce( &natoms,  &addnumber, 1, MPI_INT, MPI_SUM, cpugrid);
    natoms = addnumber;
    MPI_Allreduce( &nactive, &addnumber, 1, MPI_INT, MPI_SUM, cpugrid);
    nactive = addnumber;
#ifdef UNIAX
    MPI_Allreduce( &nactive_rot, &addnumber, 1, MPI_INT, MPI_SUM, cpugrid);
    nactive_rot = addnumber;
#endif
    for (i=0; i<ntypes; i++) {
      MPI_Allreduce( &num_sort[i], &addnumber, 1, MPI_INT, MPI_SUM, cpugrid);
      num_sort[i] = addnumber;
    }
    for (i=0; i<vtypes; i++) {
      MPI_Allreduce( &num_vsort[i], &addnumber, 1, MPI_INT, MPI_SUM, cpugrid);
      num_vsort[i] = addnumber;
    }
  } else { /* broadcast if serial io */
    MPI_Bcast( &natoms ,      1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( &nactive,      1, MPI_INT, 0, MPI_COMM_WORLD);
#ifdef UNIAX
    MPI_Bcast( &nactive_rot,  1, MPI_INT, 0, MPI_COMM_WORLD);
#endif
    MPI_Bcast( num_sort, ntypes, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( num_vsort, vtypes, MPI_INT, 0, MPI_COMM_WORLD);
  }

  /* If CPU 0 found velocities in its data, no initialisation is done */
  MPI_Bcast( &do_maxwell , 1, MPI_INT, 0, MPI_COMM_WORLD);

#endif /* MPI */

  /* print number of atoms */
  if (0==myid) {
    printf("Read structure with %d atoms.\n",natoms);
    addnumber=num_sort[0];
    printf("num_sort = [ %u",num_sort[0]);
    for (i=1; i<ntypes; i++) {
      printf(", %u",num_sort[i]);
      addnumber+=num_sort[i];
    }
    printf(" ],  total = %u\n",addnumber);
    addnumber=num_vsort[0];
    printf("num_vsort = [ %u",num_vsort[0]);
    for (i=1; i<vtypes; i++) {
      printf(", %u",num_vsort[i]);
      addnumber+=num_vsort[i];
    }
    printf(" ],  total = %u\n",addnumber);
  }
}


#ifdef MPI

/******************************************************************************
*
*  recveive atoms in several chunks from CPU 0
*  this is only used when parallel_input==0
*
******************************************************************************/

void recv_atoms(void)
{
  MPI_Status status;
  int finished=0;
  msgbuf b;   

  b.data  = (real *) malloc( INPUT_BUF_SIZE * sizeof(real) );
  b.n_max = INPUT_BUF_SIZE;
  if (NULL == b.data) error("cannot allocate input receive buffer");

  printf("Node %d listening.\n",myid);
  do {
    MPI_Recv(b.data, INPUT_BUF_SIZE, REAL, 0, MPI_ANY_TAG, cpugrid, &status);
    MPI_Get_count(&status, REAL, &b.n);
    if (status.MPI_TAG==INBUF_TAG+1) { b.n--; finished=1; } /* last buffer */
    process_buffer( &b, (cell *) NULL );
  } while (0==finished);
  printf("Node %d leaves listen.\n",myid);
  free(b.data);
}

#endif /* MPI */ 


/******************************************************************************
*
*  filter function for write_config_select
*  writes data of all atoms for checkpoints
*
******************************************************************************/

void write_atoms_config(FILE *out)
{
  int i, k, len=0;
  cell *p;

#ifdef HPO
#define RESOL1 " %12.16f"
#define RESOL3 " %12.16f %12.16f %12.16f"
#else
#define RESOL1 " %f"
#define RESOL3 " %f %f %f"
#endif

  for (k=0; k<ncells; k++) {
    p = cell_array + CELLS(k);
    for (i=0; i<p->n; i++) {
      len += sprintf(outbuf+len, "%d %d", NUMMER(p,i), VSORTE(p,i));
      len += sprintf(outbuf+len, RESOL1, MASSE(p,i));
#ifdef UNIAX
      len += sprintf(outbuf+len, RESOL1, TRAEG_MOMENT(p,i) );
#endif
      len += sprintf(outbuf+len, 
        RESOL3, ORT(p,i,X), ORT(p,i,Y), ORT(p,i,Z) );
#ifdef UNIAX
      len += sprintf(outbuf+len, 
        RESOL3, ACHSE(p,i,X), ACHSE(p,i,Y), ACHSE(p,i,Z) );
      len += sprintf(outbuf+len, 
        RESOL3, SHAPE(p,i,X), SHAPE(p,i,Y), SHAPE(p,i,Z) );
      len += sprintf(outbuf+len, 
        RESOL3, POT_WELL(p,i,X), POT_WELL(p,i,Y), POT_WELL(p,i,Z) );
#endif
#ifdef CG
      len += sprintf(outbuf+len, RESOL3, 0.0, 0.0, 0.0);
#else
      len += sprintf(outbuf+len, RESOL3,
        IMPULS(p,i,X) / MASSE(p,i), 
        IMPULS(p,i,Y) / MASSE(p,i), 
        IMPULS(p,i,Z) / MASSE(p,i));
#ifdef UNIAX
      len += sprintf(outbuf+len, RESOL3,
        DREH_IMPULS(p,i,X) / TRAEG_MOMENT(p,i),
        DREH_IMPULS(p,i,Y) / TRAEG_MOMENT(p,i),
        DREH_IMPULS(p,i,Z) / TRAEG_MOMENT(p,i)); 
#endif
#endif
      len += sprintf(outbuf+len, RESOL1, POTENG(p,i));
#ifdef ORDPAR
      len += sprintf(outbuf+len, " %d",  NBANZ(p,i));
#endif
      len += sprintf(outbuf+len,"\n");
      /* flush or send outbuf if it is full */
      if (len > OUTPUT_BUF_SIZE - 256) flush_outbuf(out,&len,OUTBUF_TAG);
    }
  }
  flush_outbuf(out,&len,OUTBUF_TAG+1);
}

/******************************************************************************
*
*  write iteration file
*
******************************************************************************/

void write_itr_file(int fzhlr, int steps, char *suffix)
{
  FILE *out;
  str255 fname;
  int m,n;

#ifdef SNAPSHOT
  if (fzhlr>=0) sprintf(fname,"%s.%05d.%sitr",outfilename,fzhlr,suffix);
  else          sprintf(fname,"%s-final.%sitr",outfilename,suffix);
#else
  if (fzhlr>=0) sprintf(fname,"%s.%05d.itr",outfilename,fzhlr);
  else          sprintf(fname,"%s-final.itr",outfilename);
#endif

  out = fopen(fname,"w");
  if (NULL == out) error("Cannot write iteration file.");

  fprintf(out,"# checkpoint %d\n",fzhlr);
  fprintf(out,"startstep \t%d\n",steps+1);
#ifdef SNAPSHOT
  fprintf(out,"sscount \t%d\n",sscount);
#endif
  fprintf(out,"box_x \t%.16f %.16f %.16f\n",box_x.x,box_x.y,box_x.z);
  fprintf(out,"box_y \t%.16f %.16f %.16f\n",box_y.x,box_y.y,box_y.z);
  fprintf(out,"box_z \t%.16f %.16f %.16f\n",box_z.x,box_z.y,box_z.z);

#if defined(NVT) || defined(NPT) 
  /* if we have temperature control, write external temperature and eta */
  if (((ensemble==ENS_NVT) || (ensemble==ENS_NPT_AXIAL) || 
       (ensemble==ENS_NPT_ISO)) && (isq_tau_eta>0)) {
    fprintf(out,"starttemp \t%f\n",temperature);
    fprintf(out,"eta \t%f\n",eta);
#ifdef UNIAX
    fprintf(out,"eta_rot \t%f\n",eta_rot);
#endif
  }
#endif

#ifdef FRAC 
 /* with FRAC ensemble, write actual damping factor and strainrate*/
  fprintf(out,"gamma_damp \t%f\n",gamma_damp);
  fprintf(out,"strainrate \t%f\n",dotepsilon);
#endif

#ifdef FTG
  for(m=0; m<nslices;m++)
  fprintf(out,"gamma_ftg %d\t%f\n",m , *(gamma_ftg + m));
#endif

#ifdef AND
  /* with Anderson thermostat, write external temperature */
  if (tempintv>0) fprintf(out,"starttemp \t%f\n",temperature);
#endif

#ifdef FBC
  for(n=0; n<vtypes;n++)
    fprintf(out,"extra_startforce %d %.21g %.21g %.21g \n",
            n,(fbc_forces+n)->x,(fbc_forces+n)->y,(fbc_forces+n)->z);
#endif

#ifdef NPT
  /* if we have pressure control, write external pressure and xi */
  if ((ensemble==ENS_NPT_ISO) && (isq_tau_xi>0)) {
    fprintf(out,"pressure_start \t%f\n",pressure_ext.x);
    fprintf(out,"xi \t%f\n",xi.x);
  }
  if ((ensemble==ENS_NPT_AXIAL) && (isq_tau_xi>0)) {
    fprintf(out,"pressure_start \t%f %f %f\n",
            pressure_ext.x,pressure_ext.y,pressure_ext.z);
    fprintf(out,"xi \t%f %f %f\n", xi.x,xi.y,xi.z);
  }
#endif /* NPT */

  fclose(out);
}

#ifdef AVPOS

/******************************************************************************
*
*  write avpos iteration file
*
******************************************************************************/

void write_avpos_itr_file(int fzhlr, int steps)
{
  FILE *out;
  str255 fname;
  real tmp;

  if (fzhlr>=0) sprintf(fname,"%s.%05d.avp.itr",outfilename,fzhlr);
  else          sprintf(fname,"%s-final.avp.itr",outfilename);

  out = fopen(fname,"w");
  if (NULL == out) error("Cannot write avpos iteration file.");

  fprintf(out,"# avpos checkpoint %d\n",fzhlr);
  fprintf(out,"startstep \t%d\n",steps+1);
#ifdef NPT
  /* Take average of box vectors */
  tmp = (real) avpos_res / ( avpos_int + avpos_res );
  fprintf(out,"box_x \t%.16f %.16f %.16f\n",av_box_x.x * tmp,av_box_x.y * tmp,av_box_x.z * tmp);
  fprintf(out,"box_y \t%.16f %.16f %.16f\n",av_box_y.x * tmp,av_box_y.y * tmp,av_box_y.z * tmp);
  fprintf(out,"box_z \t%.16f %.16f %.16f\n",av_box_z.x * tmp,av_box_z.y * tmp,av_box_z.z * tmp);
#else 
  fprintf(out,"box_x \t%.16f %.16f %.16f\n",box_x.x,box_x.y,box_x.z);
  fprintf(out,"box_y \t%.16f %.16f %.16f\n",box_y.x,box_y.y,box_y.z);
  fprintf(out,"box_z \t%.16f %.16f %.16f\n",box_z.x,box_z.y,box_z.z);
#endif

  fclose(out);
}

#endif
