/** \file particle_data.c
    This file contains everything related to particle storage. If you want to add a new
    property to the particles, it is probably a good idea to modify \ref Particle to give
    scripts access to that property. You always have to modify two positions: first the
    print section, where you should add your new data at the end, and second the read
    section where you have to find a nice and short name for your property to appear in
    the Tcl code. Then you just parse your part out of argc and argv.

    The corresponding header file is \ref particle_data.h "particle_data.h".
*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "particle_data.h"
#include "global.h"
#include "communication.h"
#include "grid.h"
#include "interaction_data.h" 
#include "integrate.h"
#include "debug.h"
#include "utils.h"
#include "cells.h"

/* cwz-build-command: make all
 */

/************************************************
 * defines
 ************************************************/

/** granularity of the particle buffers in particles */
#define PART_INCREMENT 32

/************************************************
 * variables
 ************************************************/

int max_seen_particle = -1;
int n_total_particles = 0;
int max_particle_node = 0;
int *particle_node = NULL;
int max_local_particles = 0;
Particle **local_particles = NULL;
Particle *partCfg = NULL;
int partCfgSorted = 0;

/************************************************
 * functions
 ************************************************/


void updatePartCfg()
{
  int j;
  if(partCfg)
    return;

  partCfg = malloc(n_total_particles*sizeof(Particle));
  mpi_get_particles(partCfg); 
  for(j=0; j<n_total_particles; j++)
    unfold_particle(partCfg[j].r.p,partCfg[j].i);

  partCfgSorted = 0;
}

int sortPartCfg()
{
  int i;
  Particle *sorted;

  if (!partCfg)
    updatePartCfg();

  if (partCfgSorted)
    return 1;

  if (n_total_particles != max_seen_particle + 1)
    return 0;

  sorted = malloc(n_total_particles*sizeof(Particle));
  for(i = 0; i < n_total_particles; i++) {
    COMM_TRACE(printf("Sort particle %d from %d to gdata\n",partCfg[i].r.identity, i));
    memcpy(&sorted[partCfg[i].r.identity], &partCfg[i], sizeof(Particle));
  }
  free(partCfg);
  partCfg = sorted;

  partCfgSorted = 1;

  return 1;
}

/** resize \ref local_particles.
    \param part the highest existing particle
*/
void realloc_local_particles(int part)
{
  if (part >= max_local_particles) {
    /* round up part + 1 in granularity PART_INCREMENT */
    max_local_particles = PART_INCREMENT*((part + PART_INCREMENT)/PART_INCREMENT);
    local_particles = (Particle **)realloc(local_particles, sizeof(Particle *)*max_local_particles);
  }
}

/** resize \ref particle_node.
    This procedure is only used on the master node in Tcl mode.
    \param part the highest existing particle
*/
static void realloc_particle_node(int part)
{
  if (part >= max_particle_node) {
    /* round up part + 1 in granularity PART_INCREMENT */
    max_particle_node = PART_INCREMENT*((part + PART_INCREMENT)/PART_INCREMENT);
    particle_node = (int *)realloc(particle_node, sizeof(int)*max_particle_node);
  }
}

void particle_invalidate_part_node()
{
  /* invalidate particle->node data */
  if (particle_node) {
    free(particle_node);
    particle_node = NULL;
    max_particle_node = 0;
  }
}

void build_particle_node()
{
  realloc_particle_node(max_seen_particle);
  mpi_who_has();
}

void init_particleList(ParticleList *pList)
{
  pList->n    = 0;
  pList->max  = 0;
  pList->part = NULL;
}

void init_particle(Particle *part) 
{
  part->r.identity = 0;
  part->r.type     = 0;
  part->r.q        = 0.0;
  part->r.p[0]     = 0.0;
  part->r.p[1]     = 0.0;
  part->r.p[2]     = 0.0;

  part->p_old[0]   = 0.0;
  part->p_old[1]   = 0.0;
  part->p_old[2]   = 0.0;
  part->i[0]       = 0;
  part->i[1]       = 0;
  part->i[2]       = 0;
  part->f[0]       = 0.0;
  part->f[1]       = 0.0;
  part->f[2]       = 0.0;
  part->v[0]       = 0.0;
  part->v[1]       = 0.0;
  part->v[2]       = 0.0;

#ifdef EXTERNAL_FORCES
  part->ext_flag   = 0;
  part->ext_force[0] = 0.0;
  part->ext_force[1] = 0.0;
  part->ext_force[2] = 0.0;
#endif

  init_intlist(&(part->bl));
}

int realloc_particles(ParticleList *l, int size)
{
  int old_max = l->max, i;
  Particle *old_start = l->part;
  if (size < l->max) {
    /* shrink not as fast, just lose half, rounded up */
    l->max = PART_INCREMENT*(((l->max + size + 1)/2 +
			      PART_INCREMENT - 1)/PART_INCREMENT);
  }
  else
    /* round up */
    l->max = PART_INCREMENT*((size + PART_INCREMENT - 1)/PART_INCREMENT);
  if (l->max != old_max)
    l->part = (Particle *) realloc(l->part, sizeof(Particle)*l->max);
  for (i = old_max; i < l->max; i++)
    l->part[i].r.identity = -1;
  return l->part != old_start;
}

void update_local_particles(ParticleList *pl)
{
  Particle *p = pl->part;
  int n = pl->n, i;
  for (i = 0; i < n; i++)
    local_particles[p[i].r.identity] = &p[i];
}

void init_redParticleList(RedParticleList *pList)
{
  pList->n    = 0;
  pList->max  = 0;
  pList->part = NULL;
}

void realloc_redParticles(RedParticleList *pList, int size)
{
  int old_max = pList->max, i;
  if (size < pList->max) {
    /* shrink not as fast, just lose half, rounded up */
    pList->max = PART_INCREMENT*(((pList->max + size + 1)/2 +
				  PART_INCREMENT - 1)/PART_INCREMENT);
  }
  else
    /* round up */
    pList->max = PART_INCREMENT*((size + PART_INCREMENT - 1)/PART_INCREMENT);
  if (pList->max != old_max)
    pList->part = (ReducedParticle *) realloc(pList->part, sizeof(ReducedParticle)*pList->max);
  for (i = old_max; i < pList->max; i++)
    pList->part[i].identity = -1;
}

int try_delete_bond(Particle *part, int *bond)
{
  IntList *bl = &part->bl;
  int i, j, type, partners;
  if (!bond) {
    realloc_intlist(bl, bl->n = 0);
    return TCL_OK;
  }

  for (i = 0; i < bl->n;) {
    type = bond[i];
    partners = bonded_ia_params[type].num;
    if (type != bond[0])
      i += 1 + partners;
    else {
      for(j = 1; j <= partners; j++)
	if (bond[j] != bl->e[i + j])
	  break;
      if (j > partners) {
	bl->n -= 1 + partners;
	memcpy(bl->e + i, bl->e + i + 1 + partners,
	       bl->n - i);
	realloc_intlist(bl, bl->n);
	return TCL_OK;
      }
      i += 1 + partners;
    }
  }
  return TCL_ERROR;
}

Particle *got_particle(ParticleList *l, int id)
{
  int i;

  for (i = 0; i < l->n; i++)
    if (l->part[i].r.identity == id)
      break;
  if (i == l->n)
    return NULL;
  return &(l->part[i]);
}

Particle *append_unindexed_particle(ParticleList *l, Particle *part)
{
  Particle *p;

  realloc_particles(l, ++l->n);
  p = &l->part[l->n - 1];

  memcpy(p, part, sizeof(Particle));
  return p;
}

Particle *append_indexed_particle(ParticleList *l, Particle *part)
{
  int re;
  Particle *p;
 
  re = realloc_particles(l, ++l->n);
  p  = &l->part[l->n - 1];

  memcpy(p, part, sizeof(Particle));
  if (re)
    update_local_particles(l);
  else
    local_particles[p->r.identity] = p;
  return p;
}

Particle *move_unindexed_particle(ParticleList *dl, ParticleList *sl, int i)
{
  Particle *dst, *src, *end;

  realloc_particles(dl, ++dl->n);
  dst = &dl->part[dl->n - 1];
  src = &sl->part[i];
  end = &sl->part[sl->n - 1];
  memcpy(dst, src, sizeof(Particle));
  if ( src != end )
    memcpy(src, end, sizeof(Particle));
  sl->n -= 1;
  realloc_particles(sl, sl->n);
  return dst;
}

Particle *move_indexed_particle(ParticleList *dl, ParticleList *sl, int i)
{
  int re = realloc_particles(dl, ++dl->n);
  Particle *dst = &dl->part[dl->n - 1];
  Particle *src = &sl->part[i];
  Particle *end = &sl->part[sl->n - 1];

  memcpy(dst, src, sizeof(Particle));
  if (re)
    update_local_particles(dl);
  else
    local_particles[dst->r.identity] = dst;
    
  if ( src != end )
    memcpy(src, end, sizeof(Particle));

  sl->n -= 1;
  if (realloc_particles(sl, sl->n))
    update_local_particles(sl);
  else
    local_particles[src->r.identity] = src;
  return dst;
}

void fold_particle(double pos[3],int image_box[3])
{
  int i;
  int tmp;
  for(i=0;i<3;i++) {
    if (periodic[i]) {
      image_box[i] += (tmp = (int)floor(pos[i]/box_l[i]));
      pos[i]       = pos[i] - tmp*box_l[i];    
      if(pos[i] < 0 || pos[i] > box_l[i]) {
	fprintf(stderr,"%d: fold_particle: Particle out of"
		" range image_box[%d] = %d, exiting\n",
		this_node,i,image_box[i]);
	errexit();
      }
    }
  }
}

void fold_coordinate(double pos[3], int image_box[3], int dir)
{
  int tmp;
  if (periodic[dir]) {
    image_box[dir] += (tmp = (int)floor(pos[dir]/box_l[dir]));
    pos[dir]        = pos[dir] - tmp*box_l[dir];    
    if(pos[dir] < 0 || pos[dir] > box_l[dir]) {
      fprintf(stderr,"%d: fold_particle: Particle out of"
	      " range image_box[%d] = %d, exiting\n",
	      this_node,dir,image_box[dir]);
      errexit();
    }
  }
}

void unfold_particle(double pos[3],int image_box[3])
{
  int i;
  for(i=0;i<3;i++) {
    pos[i]       = pos[i] + image_box[i]*box_l[i];    
    image_box[i] = 0;
  }
}

/** append particle data in ASCII form to the Tcl result.
    @param part_num the particle which data is appended
    @param interp   the Tcl interpreter to which result to add to */
int printParticleToResult(Tcl_Interp *interp, int part_num)
{
  char buffer[50 + TCL_DOUBLE_SPACE + TCL_INTEGER_SPACE];
  Particle part;
  IntList *bl = &(part.bl);

  if (!particle_node)
    build_particle_node();

  if (get_particle_data(part_num, &part) == TCL_ERROR)
    return (TCL_ERROR);

  unfold_particle(part.r.p, part.i);

  sprintf(buffer, "%d", part.r.identity);
  Tcl_AppendResult(interp, buffer, (char *)NULL);
  Tcl_PrintDouble(interp, part.r.p[0], buffer);
  Tcl_AppendResult(interp, " pos ", buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.r.p[1], buffer);
  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.r.p[2], buffer);
  Tcl_AppendResult(interp, buffer, " type ", (char *)NULL);
  sprintf(buffer, "%d", part.r.type);
  Tcl_AppendResult(interp, buffer, " q ", (char *)NULL);
  Tcl_PrintDouble(interp, part.r.q, buffer);
  Tcl_AppendResult(interp, buffer, " v ", (char *)NULL);
  Tcl_PrintDouble(interp, part.v[0], buffer);
  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.v[1], buffer);
  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.v[2], buffer);
  Tcl_AppendResult(interp, buffer, " f ", (char *)NULL);
  Tcl_PrintDouble(interp, part.f[0], buffer);
  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.f[1], buffer);
  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
  Tcl_PrintDouble(interp, part.f[2], buffer);
  Tcl_AppendResult(interp, buffer, (char *)NULL);

#ifdef EXTERNAL_FORCES
  /* print external force information. */
  if(part.ext_flag == PARTICLE_EXT_FORCE) {
      Tcl_AppendResult(interp, " ext_force ", (char *)NULL);
      Tcl_PrintDouble(interp, part.ext_force[0], buffer);
      Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
      Tcl_PrintDouble(interp, part.ext_force[1], buffer);
      Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
      Tcl_PrintDouble(interp, part.ext_force[2], buffer);
      Tcl_AppendResult(interp, buffer, (char *)NULL);
  }
  else if (part.ext_flag == PARTICLE_FIXED) {
    Tcl_AppendResult(interp, " fix ", (char *)NULL);
  }
#endif

  /* print bonding structure */
  if(bl->n > 0) {
    int i=0,j,size;
    Tcl_AppendResult(interp, " bonds { ", (char *)NULL);
    while(i<bl->n) {
      size = bonded_ia_params[bl->e[i]].num;
      sprintf(buffer, "{%d ", bl->e[i]); i++;
      Tcl_AppendResult(interp, buffer, (char *)NULL);
      for(j=0;j<size-1;j++) {
	sprintf(buffer, "%d ", bl->e[i]); i++;
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      sprintf(buffer, "%d} ", bl->e[i]); i++;
      Tcl_AppendResult(interp, buffer, (char *)NULL);
    }
    Tcl_AppendResult(interp, "} ", (char *)NULL);
  }
  realloc_intlist(bl, 0);

  return (TCL_OK);
}

int part(ClientData data, Tcl_Interp *interp,
	 int argc, char **argv)
{
  int part_num = -1, j;

  if (!node_grid_is_set())
    setup_node_grid();

  if (!particle_node)
    build_particle_node();

  /* if no further arguments are given, print out all stored particles */
  if (argc == 1) {
    int i = 0, start = 1;
    PART_TRACE(fprintf(stderr, "max_seen %d\n", max_seen_particle));
    for (i = 0; i <= max_seen_particle ; i++) {
      PART_TRACE(fprintf(stderr, "particle %d\n", i));
      if (particle_node[i] != -1) {
	if (start) {
	  Tcl_AppendResult(interp, "{", (char *)NULL);
	  start = 0;
	}
	else
	  Tcl_AppendResult(interp, " {", (char *)NULL);
	printParticleToResult(interp, i);
	Tcl_AppendResult(interp, "}", (char *)NULL);
      }
    }
    return (TCL_OK);
  }

  if (!strncmp(argv[1], "deleteall", strlen(argv[1]))) {
    remove_all_particles();
    return TCL_OK;
  }

  part_num = atol(argv[1]);

  /* print out particle information */
  if (argc == 2) {
    if (printParticleToResult(interp, part_num) == TCL_ERROR)
      Tcl_AppendResult(interp, "na", (char *)NULL);
    return (TCL_OK);
  }

  /* print out partial particle information */
  if (!strncmp(argv[2], "print",  strlen(argv[2]))) {
    char buffer[TCL_DOUBLE_SPACE + TCL_INTEGER_SPACE];
    Particle part;
    IntList *bl = &(part.bl);
    
    if (part_num < 0 || part_num > max_seen_particle) {
      Tcl_AppendResult(interp, "na", (char *)NULL);
      return (TCL_OK);
    }
    
    if (!particle_node)
      build_particle_node();
    
    if (get_particle_data(part_num, &part) == TCL_ERROR) {
      Tcl_AppendResult(interp, "na", (char *)NULL);
      return (TCL_OK);
    }
    
    argc -= 3;
    argv += 3;
    while (argc > 0) {
      if (!strncmp(argv[0], "identity", strlen(argv[0]))) {
	sprintf(buffer, "%d", part.r.identity);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      else if (!strncmp(argv[0], "pos", strlen(argv[0]))) {
	unfold_particle(part.r.p, part.i);
	Tcl_PrintDouble(interp, part.r.p[0], buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.r.p[1], buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.r.p[2], buffer);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      else if (!strncmp(argv[0], "type", strlen(argv[0]))) {
	sprintf(buffer, "%d", part.r.type);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      else if (!strncmp(argv[0], "q", strlen(argv[0]))) {
	Tcl_PrintDouble(interp, part.r.q, buffer);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      else if (!strncmp(argv[0], "v", strlen(argv[0]))) {
	/* unscale velocities ! */
	Tcl_PrintDouble(interp, part.v[0]/time_step, buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.v[1]/time_step, buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.v[2]/time_step, buffer);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
      else if (!strncmp(argv[0], "f", strlen(argv[0]))) {
	Tcl_PrintDouble(interp, part.f[0], buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.f[1], buffer);
	Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	Tcl_PrintDouble(interp, part.f[2], buffer);
	Tcl_AppendResult(interp, buffer, (char *)NULL);
      }
#ifdef EXTERNAL_FORCES
      else if (!strncmp(argv[0], "ext_force", strlen(argv[0]))) {
	if(part.ext_flag == PARTICLE_EXT_FORCE) {
	  Tcl_PrintDouble(interp, part.ext_force[0], buffer);
	  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	  Tcl_PrintDouble(interp, part.ext_force[1], buffer);
	  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	  Tcl_PrintDouble(interp, part.ext_force[2], buffer);
	  Tcl_AppendResult(interp, buffer, (char *)NULL);
	}
	else {
	  Tcl_AppendResult(interp, buffer, "0.0 0.0 0.0 ", (char *)NULL);
	}
      }
      else if (!strncmp(argv[0], "fix", strlen(argv[0]))) {
	if(part.ext_flag == PARTICLE_FIXED) 
	  Tcl_AppendResult(interp, "fix", (char *)NULL);
	else if(part.ext_flag == PARTICLE_UNFIXED)
	  Tcl_AppendResult(interp, "unfix", (char *)NULL);
	else if(part.ext_flag == PARTICLE_EXT_FORCE) {
	  Tcl_PrintDouble(interp, part.ext_force[0], buffer);
	  Tcl_AppendResult(interp, "ext_force ", buffer, " ", (char *)NULL);
	  Tcl_PrintDouble(interp, part.ext_force[1], buffer);
	  Tcl_AppendResult(interp, buffer, " ", (char *)NULL);
	  Tcl_PrintDouble(interp, part.ext_force[2], buffer);
	  Tcl_AppendResult(interp, buffer, (char *)NULL);
	}
      }
#endif
      else if (!strncmp(argv[0], "bonds", strlen(argv[0]))) {
	int i = 0, j, size;
	Tcl_AppendResult(interp, "{", (char *)NULL);
	while(i < bl->n) {
	  size = bonded_ia_params[bl->e[i]].num;
	  sprintf(buffer, "{%d ", bl->e[i]);
	  i++;
	  Tcl_AppendResult(interp, buffer, (char *)NULL);
	  for(j = 0; j < size - 1; j++) {
	    sprintf(buffer, "%d ", bl->e[i]);
	    i++;
	    Tcl_AppendResult(interp, buffer, (char *)NULL);
	  }
	  sprintf(buffer, "%d} ", bl->e[i]);
	  i++;
	  Tcl_AppendResult(interp, buffer, (char *)NULL);
	}
	Tcl_AppendResult(interp, "}", (char *)NULL);
	realloc_intlist(bl, 0);
      }
      else {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "unknown particle data \"", argv[0], "\" requested", (char *)NULL);
      }
      if (argc > 1)
	Tcl_AppendResult(interp, " ", (char *)NULL);

      argc--;
      argv++;
    }

    return TCL_OK;
  }

  if (part_num < 0) {
    Tcl_AppendResult(interp, "particle identities must be positive", (char *)NULL);
    return (TCL_ERROR);
  }

  /* delete a particle */

  if (!strncmp(argv[2], "delete", strlen(argv[2]))) {
    if (remove_particle(part_num) == TCL_ERROR) {
      char buffer[32 + TCL_INTEGER_SPACE];
      sprintf(buffer, "particle %d", part_num);
      Tcl_AppendResult(interp, buffer, " does not exist and cannot be removed", (char *)NULL);      
      return TCL_ERROR;
    }
    if (argc != 3) {
      Tcl_AppendResult(interp, "part <id> delete does not expect parameters", (char *)NULL);      
      return TCL_ERROR;
    }
    return TCL_OK;
  }

  /* set particle data */

  argc -= 2;
  argv += 2;
  while (argc > 0) {
    int err = TCL_OK;
    if (!strncmp(argv[0], "pos", strlen(argv[0]))) {
      double pos[3];
      if (argc < 4) {
	Tcl_AppendResult(interp, "pos requires 3 arguments", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set position */
      for (j = 0; j < 3; j++) {
	if (Tcl_GetDouble(interp, argv[1 + j], &pos[j]) == TCL_ERROR)
	  return (TCL_ERROR);
      }

      if (place_particle(part_num, pos) == TCL_ERROR) {
	Tcl_AppendResult(interp, "particle could not be set", (char *) NULL);
	return TCL_ERROR;
      }

      argc -= 4;
      argv += 4;
    }
    else if (!strncmp(argv[0], "q", strlen(argv[0]))) {
      double q;
      if (argc < 2) {
	Tcl_AppendResult(interp, "q requires 1 argument", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set charge */
      if (Tcl_GetDouble(interp, argv[1], &q) == TCL_ERROR)
	return (TCL_ERROR);
      
      err = set_particle_q(part_num, q);
      
      argc -= 2;
      argv += 2;
    }
    else if (!strncmp(argv[0], "v", strlen(argv[0]))) {
      double v[3];
      if (argc < 4) {
	Tcl_AppendResult(interp, "v requires 3 arguments", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set v */
      if (Tcl_GetDouble(interp, argv[1], &v[0]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &v[1]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[3], &v[2]) == TCL_ERROR)
	return (TCL_ERROR);
      
      /* scale velocity with time step */
      v[0] *= time_step; 
      v[1] *= time_step; 
      v[2] *= time_step; 

      err = set_particle_v(part_num, v);

      argc -= 4;
      argv += 4;
    }
    else if (!strncmp(argv[0], "f", strlen(argv[0]))) {
      double f[3];
      if (argc < 4) {
	Tcl_AppendResult(interp, "f requires 3 arguments", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set v */
      if (Tcl_GetDouble(interp, argv[1], &f[0]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &f[1]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[3], &f[2]) == TCL_ERROR)
	return (TCL_ERROR);
      
      err = set_particle_f(part_num, f);
      
      argc -= 4;
      argv += 4;
    }
    else if (!strncmp(argv[0], "type", strlen(argv[0]))) {
      int type;
      if (argc < 2) {
	Tcl_AppendResult(interp, "type requires 1 argument", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set type */
      if (Tcl_GetInt(interp, argv[1], &type) == TCL_ERROR)
	return (TCL_ERROR);
      
      if (type < 0) {
	Tcl_AppendResult(interp, "invalid particle type", (char *) NULL);
	return (TCL_ERROR);	  
      } 
      
      err = set_particle_type(part_num, type);
      
      argc -= 2;
      argv += 2;
    }
#ifdef EXTERNAL_FORCES
    else if (!strncmp(argv[0], "ext_force", strlen(argv[0]))) {
      double ext_f[3];
      if (argc < 4) {
	Tcl_AppendResult(interp, "ext_force requires 3 arguments", (char *) NULL);
	return (TCL_ERROR);
      }
      /* set external force */
      if (Tcl_GetDouble(interp, argv[1], &ext_f[0]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &ext_f[1]) == TCL_ERROR)
	return (TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[3], &ext_f[2]) == TCL_ERROR)
	return (TCL_ERROR);

      err = set_particle_ext(part_num, PARTICLE_EXT_FORCE, ext_f);
      
      argc -= 4;
      argv += 4;
      }
    else if (!strncmp(argv[0], "fix", strlen(argv[0]))) {
      double ext_f[3] = {0.0,0.0,0.0};
      err = set_particle_ext(part_num, PARTICLE_FIXED, ext_f);
      argc -= 1;
      argv += 1;
    }
    else if (!strncmp(argv[0], "unfix", strlen(argv[0]))) {
      double ext_f[3] = {0.0,0.0,0.0};
      err = set_particle_ext(part_num, PARTICLE_UNFIXED, ext_f);
      argc -= 1;
      argv += 1;
    }
#endif
    else if (!strncmp(argv[0], "bond", strlen(argv[0]))) {
      int delete = 0;
      int type_num;
      int n_partners;
      /* Bond type number and the bond partner atoms are stored in this field. */
      int *bond;
      /* check number of arguments */
      if (argc < 2) {
	Tcl_AppendResult(interp, "bond requires at least 1 arguments: "
			 "[delete] { <type_num> <partner> }", (char *) NULL);
	return (TCL_ERROR);
      }
      /* parse away delete eventually */
      if (!strncmp(argv[1], "delete", strlen(argv[1]))) {
	delete = 1;
	argc--;
	argv++;
      }
      /* check type_num */
      if (argc <= 1 || Tcl_GetInt(interp, argv[1], &type_num) == TCL_ERROR) {
	if (delete == 0)
	  return (TCL_ERROR);
	bond = NULL;
	// since there is not even a type...
	n_partners = -1;
      }
      else {
	if(type_num < 0 || type_num >= n_bonded_ia) {
	  Tcl_AppendResult(interp, "invalid bonded interaction type_num"
			   "(Set bonded interaction parameters first)", (char *) NULL);
	  return (TCL_ERROR);
	}
	/* check partners */ 
	n_partners = bonded_ia_params[type_num].num;
	if(argc < 2+n_partners) {
	  char buffer[256 + 2*TCL_INTEGER_SPACE];
	  sprintf(buffer, "bond type %d requires %d arguments.",
		  type_num, n_partners+1);
	  Tcl_AppendResult(interp, buffer, (char *) NULL);
	  return (TCL_ERROR);
	}
	bond = (int *)malloc( (n_partners+1)*sizeof(int) );
	bond[0] = type_num;
	j=1;
	while(j <= n_partners) {
	  if (Tcl_GetInt(interp, argv[j+1], &(bond[j])) == TCL_ERROR) {
	    free(bond);
	    return (TCL_ERROR);
	  }
	  if(bond[j] > max_seen_particle || particle_node[bond[j]] == -1) {
	    Tcl_AppendResult(interp, "partner atom %d (identity %d) not known"
			     ,j+1,bond[j],
			     "(Set all partner atoms first)", (char *) NULL);
	    free(bond);
	    return (TCL_ERROR);
	  }
	  j++;
	}
      }
      /* set/delete bond */ 
      if (change_particle_bond(part_num, bond, delete) != TCL_OK) {
	Tcl_AppendResult(interp, "bond to delete did not exist", (char *)NULL);
	free(bond);
	return (TCL_ERROR);
      }
      free(bond);
      argc -= (2 + n_partners);
      argv += (2 + n_partners);
    }
    else {
      Tcl_AppendResult(interp, "unknown particle parameter \"", argv[0],"\"", (char *)NULL);
      return (TCL_ERROR);
    }

    if (err == TCL_ERROR) {
      Tcl_AppendResult(interp, "set particle position first", (char *)NULL);
      return (TCL_ERROR);
    }
  }

  return (TCL_OK);
}

int get_particle_data(int part, Particle *data)
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;

  pnode = particle_node[part];
  if (pnode == -1)
    return TCL_ERROR;
  mpi_recv_part(pnode, part, data);
  return TCL_OK;
}

int place_particle(int part, double p[3])
{
  int new, i;
  int pnode, retcode = TCL_OK;

  if (part < 0)
    return TCL_ERROR;

  if (!node_grid_is_set()) 
    setup_node_grid();

  if (!particle_node)      
    build_particle_node();

  pnode = (part <= max_seen_particle) ? particle_node[part] : -1;
  new = (pnode == -1);
  if (new) {
    /* new particle, node by spatial position */
    pnode = find_node(p);

    /* master node specific stuff */
    realloc_particle_node(part);
    particle_node[part] = pnode;

    /* fill up possible gap */
    for (i = max_seen_particle + 1; i < part; i++)
      particle_node[i] = -1;

    retcode = TCL_CONTINUE;
  }

  mpi_place_particle(pnode, part, new, p);

  return retcode;
}

int set_particle_v(int part, double v[3])
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  mpi_send_v(pnode, part, v);
  return TCL_OK;
}

int set_particle_f(int part, double F[3])
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  mpi_send_f(pnode, part, F);
  return TCL_OK;
}

int set_particle_q(int part, double q)
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  mpi_send_q(pnode, part, q);
  return TCL_OK;
}

int set_particle_type(int part, int type)
{
  int pnode;

  make_particle_type_exist(type);

  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  mpi_send_type(pnode, part, type);
  return TCL_OK;
}

#ifdef EXTERNAL_FORCES
int set_particle_ext(int part, int flag, double force[3])
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  mpi_send_ext(pnode, part, flag, force);
  return TCL_OK;
}
#endif

int change_particle_bond(int part, int *bond, int delete)
{
  int pnode;
  if (!particle_node)
    build_particle_node();

  if (part < 0 || part > max_seen_particle)
    return TCL_ERROR;
  pnode = particle_node[part];

  if (pnode == -1)
    return TCL_ERROR;
  return mpi_send_bond(pnode, part, bond, delete);
}

void remove_all_particles()
{
  mpi_remove_particle(-1, -1);
  realloc_particle_node(0);
}

int remove_particle(int part)
{
  int pnode;

  if (!particle_node)
    build_particle_node();

  if (part > max_seen_particle)
    return TCL_ERROR;

  pnode = particle_node[part];
  if (pnode == -1)
    return TCL_ERROR;

  particle_node[part] = -1;

  mpi_remove_particle(pnode, part);

  if (part == max_seen_particle) {
    while (max_seen_particle >= 0 && particle_node[max_seen_particle] == -1)
      max_seen_particle--;
    mpi_bcast_parameter(FIELD_MAXPART);
  }

  return TCL_OK;
}

void local_remove_particle(int part)
{
  int ind, m, n, o;
  Particle *p = local_particles[part];
  ParticleList *pl = NULL, *tmp;
  
  /* the tricky - say ugly - part: determine
     the cell the particle is located in by checking
     wether the particle address is inside the array */
  INNER_CELLS_LOOP(m, n, o) {
    tmp = &CELL_PTR(m,n,o)->pList;
    ind = p - tmp->part;
    if (ind >= 0 && ind < tmp->n) {
      pl = tmp;
      break;
    }
  }
  if (!pl) {
    fprintf(stderr, "%d: could not find cell of particle %d, exiting\n",
	    this_node, part);
    errexit();
  }

  realloc_intlist(&p->bl, 0);
  /* remove local_particles entry */
  local_particles[p->r.identity] = NULL;

  if (&pl->part[pl->n - 1] != p) {
    /* move last particle to free position */
    memcpy(p, &pl->part[pl->n - 1], sizeof(Particle));
    /* update the local_particles array for the moved particle */
    local_particles[p->r.identity] = p;
  }

  pl->n--;
}

void local_place_particle(int part, double p[3])
{
  int i[3];
  Particle *pt = (part <= max_seen_particle) ? local_particles[part] : NULL;

  i[0] = 0;
  i[1] = 0;
  i[2] = 0;
  fold_particle(p, i);
  
  if (!pt)
    pt = cells_alloc_particle(part, p);

  memcpy(pt->r.p, p, 3*sizeof(double));
  memcpy(pt->i, i, 3*sizeof(int));
}

void local_remove_all_particles()
{
  int m, n, o;
  n_total_particles = 0;
  max_seen_particle = -1;
  INNER_CELLS_LOOP(m, n, o) {
    Particle *p = CELL_PTR(m, n, o)->pList.part;
    int i,   np = CELL_PTR(m, n, o)->pList.n;
    for (i = 0; i < np; i++)
      realloc_intlist(&p[i].bl, 0);
    CELL_PTR(m, n, o)->pList.n = 0;
  }
}

void added_particle(int part) 
{
  int i;

  n_total_particles++;

  if (part > max_seen_particle) {
    realloc_local_particles(part);
    /* fill up possible gap. Part itself is ESSENTIAL!!!  */
    for (i = max_seen_particle + 1; i <= part; i++)
      local_particles[i] = NULL;
    max_seen_particle = part;
  }
}

int local_change_bond(int part, int *bond, int delete)
{
  IntList *bl;
  Particle *p;
  int bond_size;
  int i;
 
  p = local_particles[part];
  if (delete)
    return try_delete_bond(p, bond);

  bond_size = bonded_ia_params[bond[0]].num + 1;
  bl = &(p->bl);
  realloc_intlist(bl, bl->n + bond_size);
  for(i = 0; i < bond_size; i++)
    bl->e[bl->n++] = bond[i];
  return TCL_OK;
}

void remove_all_bonds_to(int identity)
{
  int m, n, o, p;
  INNER_CELLS_LOOP(m, n, o) {
    Cell *cell = CELL_PTR(m, n, o);
    int n = cell->pList.n;
    Particle *part = cell->pList.part;
    for (p = 0; p < n; p++) {
      IntList *bl = &part[p].bl;
      int i, j, partners;
      
      for (i = 0; i < bl->n;) {
	partners = bonded_ia_params[bl->e[i]].num;
	for(j = 1; j <= partners; j++)
	  if (bl->e[i + j] == identity)
	    break;
	if (j <= partners) {
	  bl->n -= 1 + partners;
	  memcpy(bl->e + i, bl->e + i + 1 + partners,
		 bl->n - i);
	  realloc_intlist(bl, bl->n);
	}
	i += 1 + partners;
      }
      if (i != bl->n) {
	fprintf(stderr, "%d: bond information corrupt for particle %d, exiting...\n",
		this_node, part[p].r.identity);
	errexit();
      }
    }
  }
}
