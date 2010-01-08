/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision$
 ***********************************************************************EHEADER*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "_hypre_utilities.h"
#include "HYPRE_sstruct_ls.h"
#include "HYPRE_krylov.h"
#include "_hypre_sstruct_mv.h"
#include "_hypre_sstruct_ls.h"
 
#define DEBUG 0

/*--------------------------------------------------------------------------
 * Data structures
 *--------------------------------------------------------------------------*/

char infile_default[50] = "sstruct.in.default";

typedef int Index[3];
typedef int ProblemIndex[9];

typedef struct
{
   /* for GridSetExtents */
   int                    nboxes;
   ProblemIndex          *ilowers;
   ProblemIndex          *iuppers;
   int                   *boxsizes;
   int                    max_boxsize;

   /* for GridSetVariables */
   int                    nvars;
   HYPRE_SStructVariable *vartypes;

   /* for GridAddVariables */
   int                    add_nvars;
   ProblemIndex          *add_indexes;
   HYPRE_SStructVariable *add_vartypes;

   /* for GridSetNeighborBox */
   int                    glue_nboxes;
   ProblemIndex          *glue_ilowers;
   ProblemIndex          *glue_iuppers;
   int                   *glue_nbor_parts;
   ProblemIndex          *glue_nbor_ilowers;
   ProblemIndex          *glue_nbor_iuppers;
   Index                 *glue_index_maps;

   /* for GraphSetStencil */
   int                   *stencil_num;

   /* for GraphAddEntries */
   int                    graph_nentries;
   ProblemIndex          *graph_ilowers;
   ProblemIndex          *graph_iuppers;
   Index                 *graph_strides;
   int                   *graph_vars;
   int                   *graph_to_parts;
   ProblemIndex          *graph_to_ilowers;
   ProblemIndex          *graph_to_iuppers;
   Index                 *graph_to_strides;
   int                   *graph_to_vars;
   Index                 *graph_index_maps;
   Index                 *graph_index_signs;
   int                   *graph_entries;
   double                *graph_values;
   int                   *graph_boxsizes;

   int                    matrix_nentries;
   ProblemIndex          *matrix_ilowers;
   ProblemIndex          *matrix_iuppers;
   Index                 *matrix_strides;
   int                   *matrix_vars;
   int                   *matrix_entries;
   double                *matrix_values;

   Index                  periodic;

} ProblemPartData;
 
typedef struct
{
   int              ndim;
   int              nparts;
   ProblemPartData *pdata;
   int              max_boxsize;

   int              nstencils;
   int             *stencil_sizes;
   Index          **stencil_offsets;
   int            **stencil_vars;
   double         **stencil_values;

   int              symmetric_nentries;
   int             *symmetric_parts;
   int             *symmetric_vars;
   int             *symmetric_to_vars;
   int             *symmetric_booleans;

   int              ns_symmetric;

   Index            rfactor;

   int              npools;
   int             *pools;   /* array of size nparts */

} ProblemData;
 
/*--------------------------------------------------------------------------
 * Read routines
 *--------------------------------------------------------------------------*/

int
SScanIntArray( char  *sdata_ptr,
               char **sdata_ptr_ptr,
               int    size,
               int   *array )
{
   int i;

   sdata_ptr += strspn(sdata_ptr, " \t\n[");
   for (i = 0; i < size; i++)
   {
      array[i] = strtol(sdata_ptr, &sdata_ptr, 10);
   }
   sdata_ptr += strcspn(sdata_ptr, "]") + 1;

   *sdata_ptr_ptr = sdata_ptr;
   return 0;
}

int
SScanProblemIndex( char          *sdata_ptr,
                   char         **sdata_ptr_ptr,
                   int            ndim,
                   ProblemIndex   index )
{
   int  i;
   char sign[3];

   /* initialize index array */
   for (i = 0; i < 9; i++)
   {
      index[i]   = 0;
   }

   sdata_ptr += strspn(sdata_ptr, " \t\n(");
   switch (ndim)
   {
      case 1:
      sscanf(sdata_ptr, "%d%c",
             &index[0], &sign[0]);
      break;

      case 2:
      sscanf(sdata_ptr, "%d%c%d%c",
             &index[0], &sign[0], &index[1], &sign[1]);
      break;

      case 3:
      sscanf(sdata_ptr, "%d%c%d%c%d%c",
             &index[0], &sign[0], &index[1], &sign[1], &index[2], &sign[2]);
      break;
   }
   sdata_ptr += strcspn(sdata_ptr, ":)");
   if ( *sdata_ptr == ':' )
   {
      /* read in optional shift */
      sdata_ptr += 1;
      switch (ndim)
      {
         case 1:
            sscanf(sdata_ptr, "%d", &index[6]);
            break;
            
         case 2:
            sscanf(sdata_ptr, "%d%d", &index[6], &index[7]);
            break;
            
         case 3:
            sscanf(sdata_ptr, "%d%d%d", &index[6], &index[7], &index[8]);
            break;
      }
      /* pre-shift the index */
      for (i = 0; i < ndim; i++)
      {
         index[i] += index[i+6];
      }
   }
   sdata_ptr += strcspn(sdata_ptr, ")") + 1;

   for (i = 0; i < ndim; i++)
   {
      if (sign[i] == '+')
      {
         index[i+3] = 1;
      }
   }

   *sdata_ptr_ptr = sdata_ptr;
   return 0;
}

int
ReadData( char         *filename,
          ProblemData  *data_ptr )
{
   ProblemData        data;
   ProblemPartData    pdata;

   int                myid;
   FILE              *file;

   char              *sdata = NULL;
   char              *sdata_line;
   char              *sdata_ptr;
   int                sdata_size;
   int                size;
   int                memchunk = 10000;
   int                maxline  = 250;

   char               key[250];

   int                part, var, entry, s, i, il, iu;

   /*-----------------------------------------------------------
    * Read data file from process 0, then broadcast
    *-----------------------------------------------------------*/
 
   MPI_Comm_rank(MPI_COMM_WORLD, &myid);

   if (myid == 0)
   {
      if ((file = fopen(filename, "r")) == NULL)
      {
         printf("Error: can't open input file %s\n", filename);
         exit(1);
      }

      /* allocate initial space, and read first input line */
      sdata_size = 0;
      sdata = hypre_TAlloc(char, memchunk);
      sdata_line = fgets(sdata, maxline, file);

      s= 0;
      while (sdata_line != NULL)
      {
         sdata_size += strlen(sdata_line) + 1;

         /* allocate more space, if necessary */
         if ((sdata_size + maxline) > s)
         {
            sdata = hypre_TReAlloc(sdata, char, (sdata_size + memchunk));
            s= sdata_size + memchunk;
         }
         
         /* read the next input line */
         sdata_line = fgets((sdata + sdata_size), maxline, file);
      }
   }

   /* broadcast the data size */
   MPI_Bcast(&sdata_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

   /* broadcast the data */
   sdata = hypre_TReAlloc(sdata, char, sdata_size);
   MPI_Bcast(sdata, sdata_size, MPI_CHAR, 0, MPI_COMM_WORLD);

   /*-----------------------------------------------------------
    * Parse the data and fill ProblemData structure
    *-----------------------------------------------------------*/

   data.max_boxsize = 0;
   data.symmetric_nentries = 0;
   data.symmetric_parts    = NULL;
   data.symmetric_vars     = NULL;
   data.symmetric_to_vars  = NULL;
   data.symmetric_booleans = NULL;
   data.ns_symmetric = 0;

   sdata_line = sdata;
   while (sdata_line < (sdata + sdata_size))
   {
      sdata_ptr = sdata_line;
      
      if ( ( sscanf(sdata_ptr, "%s", key) > 0 ) && ( sdata_ptr[0] != '#' ) )
      {
         sdata_ptr += strcspn(sdata_ptr, " \t\n");

         if ( strcmp(key, "GridCreate:") == 0 )
         {
            data.ndim = strtol(sdata_ptr, &sdata_ptr, 10);
            data.nparts = strtol(sdata_ptr, &sdata_ptr, 10);
            data.pdata = hypre_CTAlloc(ProblemPartData, data.nparts);
         }
         else if ( strcmp(key, "GridSetExtents:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            if ((pdata.nboxes % 10) == 0)
            {
               size = pdata.nboxes + 10;
               pdata.ilowers =
                  hypre_TReAlloc(pdata.ilowers, ProblemIndex, size);
               pdata.iuppers =
                  hypre_TReAlloc(pdata.iuppers, ProblemIndex, size);
               pdata.boxsizes =
                  hypre_TReAlloc(pdata.boxsizes, int, size);
            }
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.ilowers[pdata.nboxes]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.iuppers[pdata.nboxes]);
            /* check use of +- in GridSetExtents */
            il = 1;
            iu = 1;
            for (i = 0; i < data.ndim; i++)
            {
               il *= pdata.ilowers[pdata.nboxes][i+3];
               iu *= pdata.iuppers[pdata.nboxes][i+3];
            }
            if ( (il != 0) || (iu != 1) )
            {
               printf("Error: Invalid use of `+-' in GridSetExtents\n");
               exit(1);
            }
            pdata.boxsizes[pdata.nboxes] = 1;
            for (i = 0; i < 3; i++)
            {
               pdata.boxsizes[pdata.nboxes] *=
                  (pdata.iuppers[pdata.nboxes][i] -
                   pdata.ilowers[pdata.nboxes][i] + 2);
            }
            pdata.max_boxsize =
               hypre_max(pdata.max_boxsize, pdata.boxsizes[pdata.nboxes]);
            pdata.nboxes++;
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "GridSetVariables:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            pdata.nvars = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata.vartypes = hypre_CTAlloc(HYPRE_SStructVariable, pdata.nvars);
            SScanIntArray(sdata_ptr, &sdata_ptr,
                          pdata.nvars, (int *) pdata.vartypes);
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "GridAddVariables:") == 0 )
         {
            /* TODO */
            printf("GridAddVariables not yet implemented!\n");
            exit(1);
         }
         else if ( strcmp(key, "GridSetNeighborBox:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            if ((pdata.glue_nboxes % 10) == 0)
            {
               size = pdata.glue_nboxes + 10;
               pdata.glue_ilowers =
                  hypre_TReAlloc(pdata.glue_ilowers, ProblemIndex, size);
               pdata.glue_iuppers =
                  hypre_TReAlloc(pdata.glue_iuppers, ProblemIndex, size);
               pdata.glue_nbor_parts =
                  hypre_TReAlloc(pdata.glue_nbor_parts, int, size);
               pdata.glue_nbor_ilowers =
                  hypre_TReAlloc(pdata.glue_nbor_ilowers, ProblemIndex, size);
               pdata.glue_nbor_iuppers =
                  hypre_TReAlloc(pdata.glue_nbor_iuppers, ProblemIndex, size);
               pdata.glue_index_maps =
                  hypre_TReAlloc(pdata.glue_index_maps, Index, size);
            }
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.glue_ilowers[pdata.glue_nboxes]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.glue_iuppers[pdata.glue_nboxes]);
            pdata.glue_nbor_parts[pdata.glue_nboxes] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.glue_nbor_ilowers[pdata.glue_nboxes]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.glue_nbor_iuppers[pdata.glue_nboxes]);
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim,
                          pdata.glue_index_maps[pdata.glue_nboxes]);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.glue_index_maps[pdata.glue_nboxes][i] = i;
            }
            pdata.glue_nboxes++;
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "GridSetPeriodic:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim, pdata.periodic);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.periodic[i] = 0;
            }
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "StencilCreate:") == 0 )
         {
            data.nstencils = strtol(sdata_ptr, &sdata_ptr, 10);
            data.stencil_sizes   = hypre_CTAlloc(int, data.nstencils);
            data.stencil_offsets = hypre_CTAlloc(Index *, data.nstencils);
            data.stencil_vars    = hypre_CTAlloc(int *, data.nstencils);
            data.stencil_values  = hypre_CTAlloc(double *, data.nstencils);
            SScanIntArray(sdata_ptr, &sdata_ptr,
                          data.nstencils, data.stencil_sizes);
            for (s = 0; s < data.nstencils; s++)
            {
               data.stencil_offsets[s] =
                  hypre_CTAlloc(Index, data.stencil_sizes[s]);
               data.stencil_vars[s] =
                  hypre_CTAlloc(int, data.stencil_sizes[s]);
               data.stencil_values[s] =
                  hypre_CTAlloc(double, data.stencil_sizes[s]);
            }
         }
         else if ( strcmp(key, "StencilSetEntry:") == 0 )
         {
            s = strtol(sdata_ptr, &sdata_ptr, 10);
            entry = strtol(sdata_ptr, &sdata_ptr, 10);
            SScanIntArray(sdata_ptr, &sdata_ptr,
                          data.ndim, data.stencil_offsets[s][entry]);
            for (i = data.ndim; i < 3; i++)
            {
               data.stencil_offsets[s][entry][i] = 0;
            }
            data.stencil_vars[s][entry] = strtol(sdata_ptr, &sdata_ptr, 10);
            data.stencil_values[s][entry] = strtod(sdata_ptr, &sdata_ptr);
         }
         else if ( strcmp(key, "GraphSetStencil:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            var = strtol(sdata_ptr, &sdata_ptr, 10);
            s = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            if (pdata.stencil_num == NULL)
            {
               pdata.stencil_num = hypre_CTAlloc(int, pdata.nvars);
            }
            pdata.stencil_num[var] = s;
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "GraphAddEntries:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            if ((pdata.graph_nentries % 10) == 0)
            {
               size = pdata.graph_nentries + 10;
               pdata.graph_ilowers =
                  hypre_TReAlloc(pdata.graph_ilowers, ProblemIndex, size);
               pdata.graph_iuppers =
                  hypre_TReAlloc(pdata.graph_iuppers, ProblemIndex, size);
               pdata.graph_strides =
                  hypre_TReAlloc(pdata.graph_strides, Index, size);
               pdata.graph_vars =
                  hypre_TReAlloc(pdata.graph_vars, int, size);
               pdata.graph_to_parts =
                  hypre_TReAlloc(pdata.graph_to_parts, int, size);
               pdata.graph_to_ilowers =
                  hypre_TReAlloc(pdata.graph_to_ilowers, ProblemIndex, size);
               pdata.graph_to_iuppers =
                  hypre_TReAlloc(pdata.graph_to_iuppers, ProblemIndex, size);
               pdata.graph_to_strides =
                  hypre_TReAlloc(pdata.graph_to_strides, Index, size);
               pdata.graph_to_vars =
                  hypre_TReAlloc(pdata.graph_to_vars, int, size);
               pdata.graph_index_maps =
                  hypre_TReAlloc(pdata.graph_index_maps, Index, size);
               pdata.graph_index_signs =
                  hypre_TReAlloc(pdata.graph_index_signs, Index, size);
               pdata.graph_entries =
                  hypre_TReAlloc(pdata.graph_entries, int, size);
               pdata.graph_values =
                  hypre_TReAlloc(pdata.graph_values, double, size);
               pdata.graph_boxsizes =
                  hypre_TReAlloc(pdata.graph_boxsizes, int, size);
            }
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.graph_ilowers[pdata.graph_nentries]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.graph_iuppers[pdata.graph_nentries]);
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim,
                          pdata.graph_strides[pdata.graph_nentries]);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.graph_strides[pdata.graph_nentries][i] = 1;
            }
            pdata.graph_vars[pdata.graph_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            pdata.graph_to_parts[pdata.graph_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.graph_to_ilowers[pdata.graph_nentries]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.graph_to_iuppers[pdata.graph_nentries]);
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim,
                          pdata.graph_to_strides[pdata.graph_nentries]);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.graph_to_strides[pdata.graph_nentries][i] = 1;
            }
            pdata.graph_to_vars[pdata.graph_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim,
                          pdata.graph_index_maps[pdata.graph_nentries]);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.graph_index_maps[pdata.graph_nentries][i] = i;
            }
            for (i = 0; i < 3; i++)
            {
               pdata.graph_index_signs[pdata.graph_nentries][i] = 1;
               if ( pdata.graph_to_iuppers[pdata.graph_nentries][i] <
                    pdata.graph_to_ilowers[pdata.graph_nentries][i] )
               {
                  pdata.graph_index_signs[pdata.graph_nentries][i] = -1;
               }
            }
            pdata.graph_entries[pdata.graph_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            pdata.graph_values[pdata.graph_nentries] =
               strtod(sdata_ptr, &sdata_ptr);
            pdata.graph_boxsizes[pdata.graph_nentries] = 1;
            for (i = 0; i < 3; i++)
            {
               pdata.graph_boxsizes[pdata.graph_nentries] *=
                  (pdata.graph_iuppers[pdata.graph_nentries][i] -
                   pdata.graph_ilowers[pdata.graph_nentries][i] + 1);
            }
            pdata.graph_nentries++;
            data.pdata[part] = pdata;
         }
         else if ( strcmp(key, "MatrixSetSymmetric:") == 0 )
         {
            if ((data.symmetric_nentries % 10) == 0)
            {
               size = data.symmetric_nentries + 10;
               data.symmetric_parts =
                  hypre_TReAlloc(data.symmetric_parts, int, size);
               data.symmetric_vars =
                  hypre_TReAlloc(data.symmetric_vars, int, size);
               data.symmetric_to_vars =
                  hypre_TReAlloc(data.symmetric_to_vars, int, size);
               data.symmetric_booleans =
                  hypre_TReAlloc(data.symmetric_booleans, int, size);
            }
            data.symmetric_parts[data.symmetric_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            data.symmetric_vars[data.symmetric_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            data.symmetric_to_vars[data.symmetric_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            data.symmetric_booleans[data.symmetric_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            data.symmetric_nentries++;
         }
         else if ( strcmp(key, "MatrixSetNSSymmetric:") == 0 )
         {
            data.ns_symmetric = strtol(sdata_ptr, &sdata_ptr, 10);
         }
         else if ( strcmp(key, "MatrixSetValues:") == 0 )
         {
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            pdata = data.pdata[part];
            if ((pdata.matrix_nentries % 10) == 0)
            {
               size = pdata.matrix_nentries + 10;
               pdata.matrix_ilowers =
                  hypre_TReAlloc(pdata.matrix_ilowers, ProblemIndex, size);
               pdata.matrix_iuppers =
                  hypre_TReAlloc(pdata.matrix_iuppers, ProblemIndex, size);
               pdata.matrix_strides =
                  hypre_TReAlloc(pdata.matrix_strides, Index, size);
               pdata.matrix_vars =
                  hypre_TReAlloc(pdata.matrix_vars, int, size);
               pdata.matrix_entries =
                  hypre_TReAlloc(pdata.matrix_entries, int, size);
               pdata.matrix_values =
                  hypre_TReAlloc(pdata.matrix_values, double, size);
            }
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.matrix_ilowers[pdata.matrix_nentries]);
            SScanProblemIndex(sdata_ptr, &sdata_ptr, data.ndim,
                              pdata.matrix_iuppers[pdata.matrix_nentries]);
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim,
                          pdata.matrix_strides[pdata.matrix_nentries]);
            for (i = data.ndim; i < 3; i++)
            {
               pdata.matrix_strides[pdata.matrix_nentries][i] = 1;
            }
            pdata.matrix_vars[pdata.matrix_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            pdata.matrix_entries[pdata.matrix_nentries] =
               strtol(sdata_ptr, &sdata_ptr, 10);
            pdata.matrix_values[pdata.matrix_nentries] =
               strtod(sdata_ptr, &sdata_ptr);
            pdata.matrix_nentries++;
            data.pdata[part] = pdata;
         }

         else if ( strcmp(key, "rfactor:") == 0 )
         {
            SScanIntArray(sdata_ptr, &sdata_ptr, data.ndim, data.rfactor);
            for (i= data.ndim; i< 3; i++)
            {
               data.rfactor[i]= 1;
            }
         }

         else if ( strcmp(key, "ProcessPoolCreate:") == 0 )
         {
            data.npools = strtol(sdata_ptr, &sdata_ptr, 10);
            data.pools = hypre_CTAlloc(int, data.nparts);
         }
         else if ( strcmp(key, "ProcessPoolSetPart:") == 0 )
         {
            i = strtol(sdata_ptr, &sdata_ptr, 10);
            part = strtol(sdata_ptr, &sdata_ptr, 10);
            data.pools[part] = i;
         }
      }

      sdata_line += strlen(sdata_line) + 1;
   }

   data.max_boxsize = 0;
   for (part = 0; part < data.nparts; part++)
   {
      data.max_boxsize =
         hypre_max(data.max_boxsize, data.pdata[part].max_boxsize);
   }

   hypre_TFree(sdata);

   *data_ptr = data; 
   return 0;
}
 
/*--------------------------------------------------------------------------
 * Distribute routines
 *--------------------------------------------------------------------------*/

int
MapProblemIndex( ProblemIndex index,
                 Index        m )
{
   /* un-shift the index */
   index[0] -= index[6];
   index[1] -= index[7];
   index[2] -= index[8];
   /* map the index */
   index[0] = m[0]*index[0] + (m[0]-1)*index[3];
   index[1] = m[1]*index[1] + (m[1]-1)*index[4];
   index[2] = m[2]*index[2] + (m[2]-1)*index[5];
   /* pre-shift the new mapped index */
   index[0] += index[6];
   index[1] += index[7];
   index[2] += index[8];

   return 0;
}

int
IntersectBoxes( ProblemIndex ilower1,
                ProblemIndex iupper1,
                ProblemIndex ilower2,
                ProblemIndex iupper2,
                ProblemIndex int_ilower,
                ProblemIndex int_iupper )
{
   int d, size;

   size = 1;
   for (d = 0; d < 3; d++)
   {
      int_ilower[d] = hypre_max(ilower1[d], ilower2[d]);
      int_iupper[d] = hypre_min(iupper1[d], iupper2[d]);
      size *= hypre_max(0, (int_iupper[d] - int_ilower[d] + 1));
   }

   return size;
}

int
DistributeData( ProblemData   global_data,
                Index        *refine,
                Index        *distribute,
                Index        *block,
                int           num_procs,
                int           myid,
                ProblemData  *data_ptr )
{
   ProblemData      data = global_data;
   ProblemPartData  pdata;
   int             *pool_procs;
   int              np, pid;
   int              pool, part, box, entry, p, q, r, i, d, dmap, sign, size;
   Index            m, mmap, n;
   ProblemIndex     int_ilower, int_iupper;

   /* determine first process number in each pool */
   pool_procs = hypre_CTAlloc(int, (data.npools+1));
   for (part = 0; part < data.nparts; part++)
   {
      pool = data.pools[part] + 1;
      np = distribute[part][0] * distribute[part][1] * distribute[part][2];
      pool_procs[pool] = hypre_max(pool_procs[pool], np);

   }
   pool_procs[0] = 0;
   for (pool = 1; pool < (data.npools + 1); pool++)
   {
      pool_procs[pool] = pool_procs[pool - 1] + pool_procs[pool];
   }

   /* check number of processes */
   if (pool_procs[data.npools] != num_procs)
   {
      printf("Error: Invalid number of processes or process topology \n");
      exit(1);
   }

   /* modify part data */
   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      pool  = data.pools[part];
      np  = distribute[part][0] * distribute[part][1] * distribute[part][2];
      pid = myid - pool_procs[pool];

      if ( (pid < 0) || (pid >= np) )
      {
         /* none of this part data lives on this process */
         pdata.nboxes = 0;
         pdata.glue_nboxes = 0;
         pdata.graph_nentries = 0;
         pdata.matrix_nentries = 0;
      }
      else
      {
         /* refine boxes */
         m[0] = refine[part][0];
         m[1] = refine[part][1];
         m[2] = refine[part][2];
         if ( (m[0] * m[1] * m[2]) > 1)
         {
            for (box = 0; box < pdata.nboxes; box++)
            {
               MapProblemIndex(pdata.ilowers[box], m);
               MapProblemIndex(pdata.iuppers[box], m);
            }

            for (entry = 0; entry < pdata.graph_nentries; entry++)
            {
               MapProblemIndex(pdata.graph_ilowers[entry], m);
               MapProblemIndex(pdata.graph_iuppers[entry], m);
               mmap[0] = m[pdata.graph_index_maps[entry][0]];
               mmap[1] = m[pdata.graph_index_maps[entry][1]];
               mmap[2] = m[pdata.graph_index_maps[entry][2]];
               MapProblemIndex(pdata.graph_to_ilowers[entry], mmap);
               MapProblemIndex(pdata.graph_to_iuppers[entry], mmap);
            }
            for (entry = 0; entry < pdata.matrix_nentries; entry++)
            {
               MapProblemIndex(pdata.matrix_ilowers[entry], m);
               MapProblemIndex(pdata.matrix_iuppers[entry], m);
            }
         }

         /* refine and distribute boxes */
         m[0] = distribute[part][0];
         m[1] = distribute[part][1];
         m[2] = distribute[part][2];
         if ( (m[0] * m[1] * m[2]) > 1)
         {
            p = pid % m[0];
            q = ((pid - p) / m[0]) % m[1];
            r = (pid - p - q*m[0]) / (m[0]*m[1]);

            for (box = 0; box < pdata.nboxes; box++)
            {
               n[0] = pdata.iuppers[box][0] - pdata.ilowers[box][0] + 1;
               n[1] = pdata.iuppers[box][1] - pdata.ilowers[box][1] + 1;
               n[2] = pdata.iuppers[box][2] - pdata.ilowers[box][2] + 1;

               MapProblemIndex(pdata.ilowers[box], m);
               MapProblemIndex(pdata.iuppers[box], m);
               pdata.iuppers[box][0] = pdata.ilowers[box][0] + n[0] - 1;
               pdata.iuppers[box][1] = pdata.ilowers[box][1] + n[1] - 1;
               pdata.iuppers[box][2] = pdata.ilowers[box][2] + n[2] - 1;

               pdata.ilowers[box][0] = pdata.ilowers[box][0] + p*n[0];
               pdata.ilowers[box][1] = pdata.ilowers[box][1] + q*n[1];
               pdata.ilowers[box][2] = pdata.ilowers[box][2] + r*n[2];
               pdata.iuppers[box][0] = pdata.iuppers[box][0] + p*n[0];
               pdata.iuppers[box][1] = pdata.iuppers[box][1] + q*n[1];
               pdata.iuppers[box][2] = pdata.iuppers[box][2] + r*n[2];
            }

            i = 0;
            for (entry = 0; entry < pdata.graph_nentries; entry++)
            {
               MapProblemIndex(pdata.graph_ilowers[entry], m);
               MapProblemIndex(pdata.graph_iuppers[entry], m);
               mmap[0] = m[pdata.graph_index_maps[entry][0]];
               mmap[1] = m[pdata.graph_index_maps[entry][1]];
               mmap[2] = m[pdata.graph_index_maps[entry][2]];
               MapProblemIndex(pdata.graph_to_ilowers[entry], mmap);
               MapProblemIndex(pdata.graph_to_iuppers[entry], mmap);

               for (box = 0; box < pdata.nboxes; box++)
               {
                  size = IntersectBoxes(pdata.graph_ilowers[entry],
                                        pdata.graph_iuppers[entry],
                                        pdata.ilowers[box],
                                        pdata.iuppers[box],
                                        int_ilower, int_iupper);
                  if (size > 0)
                  {
                     /* if there is an intersection, it is the only one */
                     for (d = 0; d < 3; d++)
                     {
                        dmap = pdata.graph_index_maps[entry][d];
                        sign = pdata.graph_index_signs[entry][d];
                        pdata.graph_to_iuppers[i][dmap] =
                           pdata.graph_to_ilowers[entry][dmap] + sign *
                           (int_iupper[d] - pdata.graph_ilowers[entry][d]);
                        pdata.graph_to_ilowers[i][dmap] =
                           pdata.graph_to_ilowers[entry][dmap] + sign *
                           (int_ilower[d] - pdata.graph_ilowers[entry][d]);
                        pdata.graph_ilowers[i][d] = int_ilower[d];
                        pdata.graph_iuppers[i][d] = int_iupper[d];
                        pdata.graph_strides[i][d] =
                           pdata.graph_strides[entry][d];
                        pdata.graph_to_strides[i][d] =
                           pdata.graph_to_strides[entry][d];
                        pdata.graph_index_maps[i][d]  = dmap;
                        pdata.graph_index_signs[i][d] = sign;
                     }
                     for (d = 3; d < 9; d++)
                     {
                        pdata.graph_ilowers[i][d] =
                           pdata.graph_ilowers[entry][d];
                        pdata.graph_iuppers[i][d] =
                           pdata.graph_iuppers[entry][d];
                        pdata.graph_to_ilowers[i][d] =
                           pdata.graph_to_ilowers[entry][d];
                        pdata.graph_to_iuppers[i][d] =
                           pdata.graph_to_iuppers[entry][d];
                     }
                     pdata.graph_vars[i]     = pdata.graph_vars[entry];
                     pdata.graph_to_parts[i] = pdata.graph_to_parts[entry];
                     pdata.graph_to_vars[i]  = pdata.graph_to_vars[entry];
                     pdata.graph_entries[i]  = pdata.graph_entries[entry];
                     pdata.graph_values[i]   = pdata.graph_values[entry];
                     i++;
                     break;
                  }
               }
            }
            pdata.graph_nentries = i;

            i = 0;
            for (entry = 0; entry < pdata.matrix_nentries; entry++)
            {
               MapProblemIndex(pdata.matrix_ilowers[entry], m);
               MapProblemIndex(pdata.matrix_iuppers[entry], m);

               for (box = 0; box < pdata.nboxes; box++)
               {
                  size = IntersectBoxes(pdata.matrix_ilowers[entry],
                                        pdata.matrix_iuppers[entry],
                                        pdata.ilowers[box],
                                        pdata.iuppers[box],
                                        int_ilower, int_iupper);
                  if (size > 0)
                  {
                     /* if there is an intersection, it is the only one */
                     for (d = 0; d < 3; d++)
                     {
                        pdata.matrix_ilowers[i][d] = int_ilower[d];
                        pdata.matrix_iuppers[i][d] = int_iupper[d];
                        pdata.matrix_strides[i][d] =
                           pdata.matrix_strides[entry][d];
                     }
                     for (d = 3; d < 9; d++)
                     {
                        pdata.matrix_ilowers[i][d] =
                           pdata.matrix_ilowers[entry][d];
                        pdata.matrix_iuppers[i][d] =
                           pdata.matrix_iuppers[entry][d];
                     }
                     pdata.matrix_vars[i]    = pdata.matrix_vars[entry];
                     pdata.matrix_entries[i]  = pdata.matrix_entries[entry];
                     pdata.matrix_values[i]   = pdata.matrix_values[entry];
                     i++;
                     break;
                  }
               }
            }
            pdata.matrix_nentries = i;
         }

         /* refine and block boxes */
         m[0] = block[part][0];
         m[1] = block[part][1];
         m[2] = block[part][2];
         if ( (m[0] * m[1] * m[2]) > 1)
         {
            pdata.ilowers = hypre_TReAlloc(pdata.ilowers, ProblemIndex,
                                           m[0]*m[1]*m[2]*pdata.nboxes);
            pdata.iuppers = hypre_TReAlloc(pdata.iuppers, ProblemIndex,
                                           m[0]*m[1]*m[2]*pdata.nboxes);
            pdata.boxsizes = hypre_TReAlloc(pdata.boxsizes, int,
                                            m[0]*m[1]*m[2]*pdata.nboxes);
            for (box = 0; box < pdata.nboxes; box++)
            {
               n[0] = pdata.iuppers[box][0] - pdata.ilowers[box][0] + 1;
               n[1] = pdata.iuppers[box][1] - pdata.ilowers[box][1] + 1;
               n[2] = pdata.iuppers[box][2] - pdata.ilowers[box][2] + 1;

               MapProblemIndex(pdata.ilowers[box], m);

               MapProblemIndex(pdata.iuppers[box], m);
               pdata.iuppers[box][0] = pdata.ilowers[box][0] + n[0] - 1;
               pdata.iuppers[box][1] = pdata.ilowers[box][1] + n[1] - 1;
               pdata.iuppers[box][2] = pdata.ilowers[box][2] + n[2] - 1;

               i = box;
               for (r = 0; r < m[2]; r++)
               {
                  for (q = 0; q < m[1]; q++)
                  {
                     for (p = 0; p < m[0]; p++)
                     {
                        pdata.ilowers[i][0] = pdata.ilowers[box][0] + p*n[0];
                        pdata.ilowers[i][1] = pdata.ilowers[box][1] + q*n[1];
                        pdata.ilowers[i][2] = pdata.ilowers[box][2] + r*n[2];
                        pdata.iuppers[i][0] = pdata.iuppers[box][0] + p*n[0];
                        pdata.iuppers[i][1] = pdata.iuppers[box][1] + q*n[1];
                        pdata.iuppers[i][2] = pdata.iuppers[box][2] + r*n[2];
                        for (d = 3; d < 9; d++)
                        {
                           pdata.ilowers[i][d] = pdata.ilowers[box][d];
                           pdata.iuppers[i][d] = pdata.iuppers[box][d];
                        }
                        i += pdata.nboxes;
                     }
                  }
               }
            }
            pdata.nboxes *= m[0]*m[1]*m[2];

            for (entry = 0; entry < pdata.graph_nentries; entry++)
            {
               MapProblemIndex(pdata.graph_ilowers[entry], m);
               MapProblemIndex(pdata.graph_iuppers[entry], m);
               mmap[0] = m[pdata.graph_index_maps[entry][0]];
               mmap[1] = m[pdata.graph_index_maps[entry][1]];
               mmap[2] = m[pdata.graph_index_maps[entry][2]];
               MapProblemIndex(pdata.graph_to_ilowers[entry], mmap);
               MapProblemIndex(pdata.graph_to_iuppers[entry], mmap);
            }
            for (entry = 0; entry < pdata.matrix_nentries; entry++)
            {
               MapProblemIndex(pdata.matrix_ilowers[entry], m);
               MapProblemIndex(pdata.matrix_iuppers[entry], m);
            }
         }

         /* map remaining ilowers & iuppers */
         m[0] = refine[part][0] * block[part][0] * distribute[part][0];
         m[1] = refine[part][1] * block[part][1] * distribute[part][1];
         m[2] = refine[part][2] * block[part][2] * distribute[part][2];
         if ( (m[0] * m[1] * m[2]) > 1)
         {
            for (box = 0; box < pdata.glue_nboxes; box++)
            {
               MapProblemIndex(pdata.glue_ilowers[box], m);
               MapProblemIndex(pdata.glue_iuppers[box], m);
               mmap[0] = m[pdata.glue_index_maps[box][0]];
               mmap[1] = m[pdata.glue_index_maps[box][1]];
               mmap[2] = m[pdata.glue_index_maps[box][2]];
               MapProblemIndex(pdata.glue_nbor_ilowers[box], mmap);
               MapProblemIndex(pdata.glue_nbor_iuppers[box], mmap);
            }
         }

         /* compute box sizes, etc. */
         pdata.max_boxsize = 0;
         for (box = 0; box < pdata.nboxes; box++)
         {
            pdata.boxsizes[box] = 1;
            for (i = 0; i < 3; i++)
            {
               pdata.boxsizes[box] *=
                  (pdata.iuppers[box][i] - pdata.ilowers[box][i] + 2);
            }
            pdata.max_boxsize =
               hypre_max(pdata.max_boxsize, pdata.boxsizes[box]);
         }
         for (box = 0; box < pdata.graph_nentries; box++)
         {
            pdata.graph_boxsizes[box] = 1;
            for (i = 0; i < 3; i++)
            {
               pdata.graph_boxsizes[box] *=
                  (pdata.graph_iuppers[box][i] -
                   pdata.graph_ilowers[box][i] + 1);
            }
         }
      }

      if (pdata.nboxes == 0)
      {
         hypre_TFree(pdata.ilowers);
         hypre_TFree(pdata.iuppers);
         hypre_TFree(pdata.boxsizes);
         pdata.max_boxsize = 0;
      }

      if (pdata.glue_nboxes == 0)
      {
         hypre_TFree(pdata.glue_ilowers);
         hypre_TFree(pdata.glue_iuppers);
         hypre_TFree(pdata.glue_nbor_parts);
         hypre_TFree(pdata.glue_nbor_ilowers);
         hypre_TFree(pdata.glue_nbor_iuppers);
         hypre_TFree(pdata.glue_index_maps);
      }

      if (pdata.graph_nentries == 0)
      {
         hypre_TFree(pdata.graph_ilowers);
         hypre_TFree(pdata.graph_iuppers);
         hypre_TFree(pdata.graph_strides);
         hypre_TFree(pdata.graph_vars);
         hypre_TFree(pdata.graph_to_parts);
         hypre_TFree(pdata.graph_to_ilowers);
         hypre_TFree(pdata.graph_to_iuppers);
         hypre_TFree(pdata.graph_to_strides);
         hypre_TFree(pdata.graph_to_vars);
         hypre_TFree(pdata.graph_index_maps);
         hypre_TFree(pdata.graph_index_signs);
         hypre_TFree(pdata.graph_entries);
         hypre_TFree(pdata.graph_values);
         hypre_TFree(pdata.graph_boxsizes);
      }

      if (pdata.matrix_nentries == 0)
      {
         hypre_TFree(pdata.matrix_ilowers);
         hypre_TFree(pdata.matrix_iuppers);
         hypre_TFree(pdata.matrix_strides);
         hypre_TFree(pdata.matrix_vars);
         hypre_TFree(pdata.matrix_entries);
         hypre_TFree(pdata.matrix_values);
      }

      data.pdata[part] = pdata;
   }

   data.max_boxsize = 0;
   for (part = 0; part < data.nparts; part++)
   {
      data.max_boxsize =
         hypre_max(data.max_boxsize, data.pdata[part].max_boxsize);
   }

   hypre_TFree(pool_procs);

   *data_ptr = data; 
   return 0;
}

/*--------------------------------------------------------------------------
 * Destroy data
 *--------------------------------------------------------------------------*/

int
DestroyData( ProblemData   data )
{
   ProblemPartData  pdata;
   int              part, s;

   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];

      if (pdata.nboxes > 0)
      {
         hypre_TFree(pdata.ilowers);
         hypre_TFree(pdata.iuppers);
         hypre_TFree(pdata.boxsizes);
      }

      if (pdata.nvars > 0)
      {
         hypre_TFree(pdata.vartypes);
      }

      if (pdata.add_nvars > 0)
      {
         hypre_TFree(pdata.add_indexes);
         hypre_TFree(pdata.add_vartypes);
      }

      if (pdata.glue_nboxes > 0)
      {
         hypre_TFree(pdata.glue_ilowers);
         hypre_TFree(pdata.glue_iuppers);
         hypre_TFree(pdata.glue_nbor_parts);
         hypre_TFree(pdata.glue_nbor_ilowers);
         hypre_TFree(pdata.glue_nbor_iuppers);
         hypre_TFree(pdata.glue_index_maps);
      }

      if (pdata.nvars > 0)
      {
         hypre_TFree(pdata.stencil_num);
      }

      if (pdata.graph_nentries > 0)
      {
         hypre_TFree(pdata.graph_ilowers);
         hypre_TFree(pdata.graph_iuppers);
         hypre_TFree(pdata.graph_strides);
         hypre_TFree(pdata.graph_vars);
         hypre_TFree(pdata.graph_to_parts);
         hypre_TFree(pdata.graph_to_ilowers);
         hypre_TFree(pdata.graph_to_iuppers);
         hypre_TFree(pdata.graph_to_strides);
         hypre_TFree(pdata.graph_to_vars);
         hypre_TFree(pdata.graph_index_maps);
         hypre_TFree(pdata.graph_index_signs);
         hypre_TFree(pdata.graph_entries);
         hypre_TFree(pdata.graph_values);
         hypre_TFree(pdata.graph_boxsizes);
      }

      if (pdata.matrix_nentries > 0)
      {
         hypre_TFree(pdata.matrix_ilowers);
         hypre_TFree(pdata.matrix_iuppers);
         hypre_TFree(pdata.matrix_strides);
         hypre_TFree(pdata.matrix_vars);
         hypre_TFree(pdata.matrix_entries);
         hypre_TFree(pdata.matrix_values);
      }

   }
   hypre_TFree(data.pdata);

   for (s = 0; s < data.nstencils; s++)
   {
      hypre_TFree(data.stencil_offsets[s]);
      hypre_TFree(data.stencil_vars[s]);
      hypre_TFree(data.stencil_values[s]);
   }
   hypre_TFree(data.stencil_sizes);
   hypre_TFree(data.stencil_offsets);
   hypre_TFree(data.stencil_vars);
   hypre_TFree(data.stencil_values);

   if (data.symmetric_nentries > 0)
   {
      hypre_TFree(data.symmetric_parts);
      hypre_TFree(data.symmetric_vars);
      hypre_TFree(data.symmetric_to_vars);
      hypre_TFree(data.symmetric_booleans);
   }

   hypre_TFree(data.pools);

   return 0;
}

/*--------------------------------------------------------------------------
 * Compute new box based on variable type
 *--------------------------------------------------------------------------*/

int
GetVariableBox( Index  cell_ilower,
                Index  cell_iupper,
                int    int_vartype,
                Index  var_ilower,
                Index  var_iupper )
{
   int ierr = 0;
   HYPRE_SStructVariable  vartype = (HYPRE_SStructVariable) int_vartype;

   var_ilower[0] = cell_ilower[0];
   var_ilower[1] = cell_ilower[1];
   var_ilower[2] = cell_ilower[2];
   var_iupper[0] = cell_iupper[0];
   var_iupper[1] = cell_iupper[1];
   var_iupper[2] = cell_iupper[2];

   switch(vartype)
   {
      case HYPRE_SSTRUCT_VARIABLE_CELL:
      var_ilower[0] -= 0; var_ilower[1] -= 0; var_ilower[2] -= 0;
      break;
      case HYPRE_SSTRUCT_VARIABLE_NODE:
      var_ilower[0] -= 1; var_ilower[1] -= 1; var_ilower[2] -= 1;
      break;
      case HYPRE_SSTRUCT_VARIABLE_XFACE:
      var_ilower[0] -= 1; var_ilower[1] -= 0; var_ilower[2] -= 0;
      break;
      case HYPRE_SSTRUCT_VARIABLE_YFACE:
      var_ilower[0] -= 0; var_ilower[1] -= 1; var_ilower[2] -= 0;
      break;
      case HYPRE_SSTRUCT_VARIABLE_ZFACE:
      var_ilower[0] -= 0; var_ilower[1] -= 0; var_ilower[2] -= 1;
      break;
      case HYPRE_SSTRUCT_VARIABLE_XEDGE:
      var_ilower[0] -= 0; var_ilower[1] -= 1; var_ilower[2] -= 1;
      break;
      case HYPRE_SSTRUCT_VARIABLE_YEDGE:
      var_ilower[0] -= 1; var_ilower[1] -= 0; var_ilower[2] -= 1;
      break;
      case HYPRE_SSTRUCT_VARIABLE_ZEDGE:
      var_ilower[0] -= 1; var_ilower[1] -= 1; var_ilower[2] -= 0;
      break;
      case HYPRE_SSTRUCT_VARIABLE_UNDEFINED:
      break;
   }

   return ierr;
}
/*--------------------------------------------------------------------------
 * Print usage info
 *--------------------------------------------------------------------------*/

int
PrintUsage( char *progname,
            int   myid )
{
   if ( myid == 0 )
   {
      printf("\n");
      printf("Usage: %s [<options>]\n", progname);
      printf("\n");
      printf("  -in <filename> : input file (default is `%s')\n",
             infile_default);
      printf("\n");
      printf("  -pt <pt1> <pt2> ... : set part(s) for subsequent options\n");
      printf("  -r <rx> <ry> <rz>   : refine part(s)\n");
      printf("  -P <Px> <Py> <Pz>   : refine and distribute part(s)\n");
      printf("  -b <bx> <by> <bz>   : refine and block part(s)\n");
      printf("  -solver <ID>        : solver ID (default = 39)\n");
      printf("  -print             : print out the system\n");
      printf("  -v <n_pre> <n_post>: SysPFMG and Struct- # of pre and post relax\n");
      printf("  -sym <s>           : Struct- symmetric storage (1) or not (0)\n");

      printf("\n");
   }

   return 0;
}

/*--------------------------------------------------------------------------
 * Test driver for semi-structured matrix interface
 *--------------------------------------------------------------------------*/
 
int
main( int   argc,
      char *argv[] )
{
   char                 *infile;
   ProblemData           global_data;
   ProblemData           data;
   ProblemPartData       pdata;
   int                   nparts;
   int                  *parts;
   Index                *refine;
   Index                *distribute;
   Index                *block;
   int                   solver_id;
   int                   print_system;
                        
   HYPRE_SStructGrid     grid;
   HYPRE_SStructStencil *stencils;
   HYPRE_SStructGraph    graph;
   HYPRE_SStructMatrix   A;
   HYPRE_ParCSRMatrix    T, parA;
   HYPRE_SStructVector   b;
   HYPRE_SStructVector   x;
   HYPRE_ParVector       parb, parx;
   HYPRE_SStructSolver   solver;

   HYPRE_StructGrid      cell_grid;
   hypre_Box            *bounding_box;
   double                h;

   int                 **bdryRanks, *bdryRanksCnt;

   Index                 ilower, iupper;
   Index                 index, to_index;
   double               *values;

   int                   num_iterations;
   double                final_res_norm;
                         
   int                   num_procs, myid;
   int                   time_index;
                         
   int                   n_pre, n_post;

   int                   arg_index, part, box, var, entry, s, i, j, k;
                        
   /*-----------------------------------------------------------
    * Initialize some stuff
    *-----------------------------------------------------------*/

   /* Initialize MPI */
   MPI_Init(&argc, &argv);

   MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myid);

   hypre_InitMemoryDebug(myid);

   /*-----------------------------------------------------------
    * Read input file
    *-----------------------------------------------------------*/

   arg_index = 1;

   /* parse command line for input file name */
   infile = infile_default;
   if (argc > 1)
   {
      if ( strcmp(argv[arg_index], "-in") == 0 )
      {
         arg_index++;
         infile = argv[arg_index++];
      }
   }

   ReadData(infile, &global_data);

   /*-----------------------------------------------------------
    * Set defaults
    *-----------------------------------------------------------*/

   nparts = global_data.nparts;

   parts      = hypre_TAlloc(int, nparts);
   refine     = hypre_TAlloc(Index, nparts);
   distribute = hypre_TAlloc(Index, nparts);
   block      = hypre_TAlloc(Index, nparts);
   for (part = 0; part < nparts; part++)
   {
      parts[part] = part;
      for (j = 0; j < 3; j++)
      {
         refine[part][j]     = 1;
         distribute[part][j] = 1;
         block[part][j]      = 1;
      }
   }

   print_system = 0;

   n_pre  = 1;
   n_post = 1;

   /*-----------------------------------------------------------
    * Parse command line
    *-----------------------------------------------------------*/

   while (arg_index < argc)
   {
      if ( strcmp(argv[arg_index], "-pt") == 0 )
      {
         arg_index++;
         nparts = 0;
         while ( strncmp(argv[arg_index], "-", 1) != 0 )
         {
            parts[nparts++] = atoi(argv[arg_index++]);
         }
      }
      else if ( strcmp(argv[arg_index], "-r") == 0 )
      {
         arg_index++;
         for (i = 0; i < nparts; i++)
         {
            part = parts[i];
            k = arg_index;
            for (j = 0; j < 3; j++)
            {
               refine[part][j] = atoi(argv[k++]);
            }
         }
         arg_index += 3;
      }
      else if ( strcmp(argv[arg_index], "-P") == 0 )
      {
         arg_index++;
         for (i = 0; i < nparts; i++)
         {
            part = parts[i];
            k = arg_index;
            for (j = 0; j < 3; j++)
            {
               distribute[part][j] = atoi(argv[k++]);
            }
         }
         arg_index += 3;
      }
      else if ( strcmp(argv[arg_index], "-b") == 0 )
      {
         arg_index++;
         for (i = 0; i < nparts; i++)
         {
            part = parts[i];
            k = arg_index;
            for (j = 0; j < 3; j++)
            {
               block[part][j] = atoi(argv[k++]);
            }
         }
         arg_index += 3;
      }
      else if ( strcmp(argv[arg_index], "-solver") == 0 )
      {
         arg_index++;
         solver_id = atoi(argv[arg_index++]);
      }
      else if ( strcmp(argv[arg_index], "-print") == 0 )
      {
         arg_index++;
         print_system = 1;
      }
      else if ( strcmp(argv[arg_index], "-v") == 0 )
      {
         arg_index++;
         n_pre = atoi(argv[arg_index++]);
         n_post = atoi(argv[arg_index++]);
      }
      else if ( strcmp(argv[arg_index], "-help") == 0 )
      {
         PrintUsage(argv[0], myid);
         exit(1);
         break;
      }
      else
      {
         break;
      }
   }

   /*-----------------------------------------------------------
    * Distribute data
    *-----------------------------------------------------------*/

   DistributeData(global_data, refine, distribute, block,
                  num_procs, myid, &data);

   /*-----------------------------------------------------------
    * Synchronize so that timings make sense
    *-----------------------------------------------------------*/

   MPI_Barrier(MPI_COMM_WORLD);

   /*-----------------------------------------------------------
    * Set up the grid
    *-----------------------------------------------------------*/

   time_index = hypre_InitializeTiming("SStruct Interface");
   hypre_BeginTiming(time_index);

   HYPRE_SStructGridCreate(MPI_COMM_WORLD, data.ndim, data.nparts, &grid);
   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      for (box = 0; box < pdata.nboxes; box++)
      {
         HYPRE_SStructGridSetExtents(grid, part,
                                     pdata.ilowers[box], pdata.iuppers[box]);
      }

      HYPRE_SStructGridSetVariables(grid, part, pdata.nvars, pdata.vartypes);

      /* GridAddVariabes */

      /* GridSetNeighborBox */
      for (box = 0; box < pdata.glue_nboxes; box++)
      {
         printf("Error: No longer supporting SetNeighborBox\n");
#if 0
         HYPRE_SStructGridSetNeighborBox(grid, part,
                                         pdata.glue_ilowers[box],
                                         pdata.glue_iuppers[box],
                                         pdata.glue_nbor_parts[box],
                                         pdata.glue_nbor_ilowers[box],
                                         pdata.glue_nbor_iuppers[box],
                                         pdata.glue_index_maps[box]);
#endif
      }

      HYPRE_SStructGridSetPeriodic(grid, part, pdata.periodic);
   }
   HYPRE_SStructGridAssemble(grid);

   /*-----------------------------------------------------------
    * Set up the stencils
    *-----------------------------------------------------------*/

   stencils = hypre_CTAlloc(HYPRE_SStructStencil, data.nstencils);
   for (s = 0; s < data.nstencils; s++)
   {
      HYPRE_SStructStencilCreate(data.ndim, data.stencil_sizes[s],
                                 &stencils[s]);
      for (i = 0; i < data.stencil_sizes[s]; i++)
      {
         HYPRE_SStructStencilSetEntry(stencils[s], i,
                                      data.stencil_offsets[s][i],
                                      data.stencil_vars[s][i]);
      }
   }

   /*-----------------------------------------------------------
    * Set up the graph
    *-----------------------------------------------------------*/

   HYPRE_SStructGraphCreate(MPI_COMM_WORLD, grid, &graph);
   HYPRE_SStructGraphSetObjectType(graph, HYPRE_PARCSR);  

   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];

      /* set stencils */
      for (var = 0; var < pdata.nvars; var++)
      {
         HYPRE_SStructGraphSetStencil(graph, part, var,
                                      stencils[pdata.stencil_num[var]]);
      }

      /* add entries */
      for (entry = 0; entry < pdata.graph_nentries; entry++)
      {
         for (index[2] = pdata.graph_ilowers[entry][2];
              index[2] <= pdata.graph_iuppers[entry][2];
              index[2] += pdata.graph_strides[entry][2])
         {
            for (index[1] = pdata.graph_ilowers[entry][1];
                 index[1] <= pdata.graph_iuppers[entry][1];
                 index[1] += pdata.graph_strides[entry][1])
            {
               for (index[0] = pdata.graph_ilowers[entry][0];
                    index[0] <= pdata.graph_iuppers[entry][0];
                    index[0] += pdata.graph_strides[entry][0])
               {
                  for (i = 0; i < 3; i++)
                  {
                     j = pdata.graph_index_maps[entry][i];
                     k = index[i] - pdata.graph_ilowers[entry][i];
                     k /= pdata.graph_strides[entry][i];
                     k *= pdata.graph_index_signs[entry][i];
                     to_index[j] = pdata.graph_to_ilowers[entry][j] +
                        k * pdata.graph_to_strides[entry][j];
                  }
                  HYPRE_SStructGraphAddEntries(graph, part, index,
                                               pdata.graph_vars[entry],
                                               pdata.graph_to_parts[entry],
                                               to_index,
                                               pdata.graph_to_vars[entry]);
               }
            }
         }
      }
   }

   HYPRE_SStructGraphAssemble(graph);

   /*-----------------------------------------------------------
    * Set up the matrix
    *-----------------------------------------------------------*/

   values = hypre_TAlloc(double, data.max_boxsize);

   HYPRE_SStructMatrixCreate(MPI_COMM_WORLD, graph, &A);

   /* TODO HYPRE_SStructMatrixSetSymmetric(A, 1); */
   for (entry = 0; entry < data.symmetric_nentries; entry++)
   {
      HYPRE_SStructMatrixSetSymmetric(A,
                                      data.symmetric_parts[entry],
                                      data.symmetric_vars[entry],
                                      data.symmetric_to_vars[entry],
                                      data.symmetric_booleans[entry]);
   }
   HYPRE_SStructMatrixSetNSSymmetric(A, data.ns_symmetric);

   HYPRE_SStructMatrixSetObjectType(A, HYPRE_PARCSR);
   HYPRE_SStructMatrixInitialize(A);

   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      cell_grid=  hypre_SStructPGridCellSGrid(hypre_SStructGridPGrid(grid, part));
      bounding_box= hypre_StructGridBoundingBox(cell_grid);

      h= (double) (hypre_BoxIMax(bounding_box)[0]- hypre_BoxIMin(bounding_box)[0]);
      for (i= 1; i< data.ndim; i++)
      {
         if ((hypre_BoxIMax(bounding_box)[i]- hypre_BoxIMin(bounding_box)[i]) > h)
         {
            h= (double) (hypre_BoxIMax(bounding_box)[i]- hypre_BoxIMin(bounding_box)[i]);
         }
      }
      h= 1.0/h;
         
      /* set stencil values */
      for (var = 0; var < pdata.nvars; var++)
      {
         s = pdata.stencil_num[var];
         for (i = 0; i < data.stencil_sizes[s]; i++)
         {
            for (j = 0; j < pdata.max_boxsize; j++)
            {
               values[j] = h*data.stencil_values[s][i];
            }
            if (i < 9)
            {
               for (j = 0; j < pdata.max_boxsize; j++)
               {
                  values[j]+= data.stencil_values[s+data.ndim][i]/h;
               }
            }

            for (box = 0; box < pdata.nboxes; box++)
            {
               GetVariableBox(pdata.ilowers[box], pdata.iuppers[box],
                              pdata.vartypes[var], ilower, iupper);
               HYPRE_SStructMatrixSetBoxValues(A, part, ilower, iupper,
                                               var, 1, &i, values);
            }
         }
      }

      /* set non-stencil entries */
      for (entry = 0; entry < pdata.graph_nentries; entry++)
      {
         for (index[2] = pdata.graph_ilowers[entry][2];
              index[2] <= pdata.graph_iuppers[entry][2];
              index[2] += pdata.graph_strides[entry][2])
         {
            for (index[1] = pdata.graph_ilowers[entry][1];
                 index[1] <= pdata.graph_iuppers[entry][1];
                 index[1] += pdata.graph_strides[entry][1])
            {
               for (index[0] = pdata.graph_ilowers[entry][0];
                    index[0] <= pdata.graph_iuppers[entry][0];
                    index[0] += pdata.graph_strides[entry][0])
               {
                  HYPRE_SStructMatrixSetValues(A, part, index,
                                               pdata.graph_vars[entry],
                                               1, &pdata.graph_entries[entry],
                                               &pdata.graph_values[entry]);
               }
            }
         }
      }
   }

   /* reset matrix values:
    *   NOTE THAT THE matrix_ilowers & matrix_iuppers MUST BE IN TERMS OF THE
    *   CHOOSEN VAR_TYPE INDICES, UNLIKE THE EXTENTS OF THE GRID< WHICH ARE
    *   IN TEMS OF THE CELL VARTYPE INDICES.
    */
   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      for (entry = 0; entry < pdata.matrix_nentries; entry++)
      {
         for (index[2] = pdata.matrix_ilowers[entry][2];
              index[2] <= pdata.matrix_iuppers[entry][2];
              index[2] += pdata.matrix_strides[entry][2])
         {
            for (index[1] = pdata.matrix_ilowers[entry][1];
                 index[1] <= pdata.matrix_iuppers[entry][1];
                 index[1] += pdata.matrix_strides[entry][1])
            {
               for (index[0] = pdata.matrix_ilowers[entry][0];
                    index[0] <= pdata.matrix_iuppers[entry][0];
                    index[0] += pdata.matrix_strides[entry][0])
               {
                  HYPRE_SStructMatrixSetValues(A, part, index,
                                               pdata.matrix_vars[entry],
                                               1, &pdata.matrix_entries[entry],
                                               &pdata.matrix_values[entry]);
               }
            }
         }
      }
   }

   HYPRE_SStructMatrixAssemble(A);
   HYPRE_MaxwellGrad(grid, &T);

   /* eliminate the physical boundary points */
   HYPRE_SStructMatrixGetObject(A, (void **) &parA);
   HYPRE_SStructMaxwellPhysBdy(&grid, 1, data.rfactor,
                               &bdryRanks, &bdryRanksCnt);

   HYPRE_SStructMaxwellEliminateRowsCols(parA, bdryRanksCnt[0], bdryRanks[0]);

   /*{
      hypre_MaxwellOffProcRow **OffProcRows;
      hypre_SStructSharedDOF_ParcsrMatRowsComm(grid,
                                               (hypre_ParCSRMatrix *) parA,
                                               &i,
                                               &OffProcRows);
      for (j= 0; j< i; j++)
      {
         hypre_MaxwellOffProcRowDestroy((void *) OffProcRows[j]);
      }
      hypre_TFree(OffProcRows);
    }*/
                                                                                                                                    
   /*-----------------------------------------------------------
    * Set up the linear system
    *-----------------------------------------------------------*/

   HYPRE_SStructVectorCreate(MPI_COMM_WORLD, grid, &b);
   HYPRE_SStructVectorSetObjectType(b, HYPRE_PARCSR);

   HYPRE_SStructVectorInitialize(b);
   for (j = 0; j < data.max_boxsize; j++)
   {
      values[j]= sin((double)(j+1));
      values[j]= (double) rand()/RAND_MAX;
      values[j]= (double) j;
   }
   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      for (var = 0; var < pdata.nvars; var++)
      {
         for (box = 0; box < pdata.nboxes; box++)
         {
            GetVariableBox(pdata.ilowers[box], pdata.iuppers[box], 
                           pdata.vartypes[var], ilower, iupper);
            HYPRE_SStructVectorSetBoxValues(b, part, ilower, iupper,
                                            var, values);
         }
      }
   }
   HYPRE_SStructVectorAssemble(b);

   HYPRE_SStructVectorCreate(MPI_COMM_WORLD, grid, &x);
   HYPRE_SStructVectorSetObjectType(x, HYPRE_PARCSR);

   HYPRE_SStructVectorInitialize(x);
   for (j = 0; j < data.max_boxsize; j++)
   {
      values[j] = 0.0;
   }
   for (part = 0; part < data.nparts; part++)
   {
      pdata = data.pdata[part];
      for (var = 0; var < pdata.nvars; var++)
      {
         for (box = 0; box < pdata.nboxes; box++)
         {
            GetVariableBox(pdata.ilowers[box], pdata.iuppers[box], 
                           pdata.vartypes[var], ilower, iupper);
            HYPRE_SStructVectorSetBoxValues(x, part, ilower, iupper,
                                            var, values);
         }
      }
   }
   HYPRE_SStructVectorAssemble(x);

   HYPRE_SStructVectorGetObject(x, (void **) &parx);
   HYPRE_SStructVectorGetObject(b, (void **) &parb);
   HYPRE_SStructMaxwellZeroVector(parx, bdryRanks[0], bdryRanksCnt[0]);
   HYPRE_SStructMaxwellZeroVector(parb, bdryRanks[0], bdryRanksCnt[0]);

   hypre_TFree(bdryRanks[0]);
   hypre_TFree(bdryRanks);
   hypre_TFree(bdryRanksCnt);

   hypre_EndTiming(time_index);
   hypre_PrintTiming("SStruct Interface", MPI_COMM_WORLD);
   hypre_FinalizeTiming(time_index);
   hypre_ClearTiming();

   /*-----------------------------------------------------------
    * Print out the system and initial guess
    *-----------------------------------------------------------*/

   if (print_system)
   {
      HYPRE_SStructMatrixPrint("sstruct.out.A",  A, 0);
      HYPRE_SStructVectorPrint("sstruct.out.b",  b, 0);
      HYPRE_SStructVectorPrint("sstruct.out.x0", x, 0);
   }

   /*-----------------------------------------------------------
    * Debugging code
    *-----------------------------------------------------------*/

   hypre_TFree(values);

   if (solver_id == 1)
   {
      time_index = hypre_InitializeTiming("Maxwell Setup");
      hypre_BeginTiming(time_index);
                                                                                                                              
      HYPRE_SStructMaxwellCreate(MPI_COMM_WORLD, &solver);
      HYPRE_SStructMaxwellSetMaxIter(solver, 20);
      HYPRE_SStructMaxwellSetTol(solver, 1.0e-8);
      HYPRE_SStructMaxwellSetRelChange(solver, 0);
      HYPRE_SStructMaxwellSetNumPreRelax(solver, 1);
      HYPRE_SStructMaxwellSetNumPostRelax(solver, 1);
      HYPRE_SStructMaxwellSetRfactors(solver, data.rfactor);
      HYPRE_SStructMaxwellSetGrad(solver, T);
      /*HYPRE_SStructMaxwellSetConstantCoef(solver, 1);*/
      HYPRE_SStructMaxwellSetPrintLevel(solver, 1);
      HYPRE_SStructMaxwellSetLogging(solver, 1);
      HYPRE_SStructMaxwellSetup(solver, A, b, x);
                                                                                                                              
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Setup phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();
                                                                                                                              
      time_index = hypre_InitializeTiming("Maxwell Solve");
      hypre_BeginTiming(time_index);
                                                                                                                              
      HYPRE_SStructMaxwellSolve(solver, A, b, x);
                                                                                                                              
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Solve phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();
                                                                                                                              
      HYPRE_SStructMaxwellGetNumIterations(solver, &num_iterations);
      HYPRE_SStructMaxwellGetFinalRelativeResidualNorm(
                                           solver, &final_res_norm);
      HYPRE_SStructMaxwellDestroy(solver);
   }

   HYPRE_SStructVectorGather(x);

   /*-----------------------------------------------------------
    * Print the solution and other info
    *-----------------------------------------------------------*/

   if (print_system)
   {
      HYPRE_SStructVectorPrint("sstruct.out.x", x, 0);
   }

   if (myid == 0)
   {
      printf("\n");
      printf("Iterations = %d\n", num_iterations);
      printf("Final Relative Residual Norm = %e\n", final_res_norm);
      printf("\n");
   }

   /*-----------------------------------------------------------
    * Finalize things
    *-----------------------------------------------------------*/

   HYPRE_SStructGridDestroy(grid);
   for (s = 0; s < data.nstencils; s++)
   {
      HYPRE_SStructStencilDestroy(stencils[s]);
   }
   hypre_TFree(stencils);
   HYPRE_SStructGraphDestroy(graph);
   HYPRE_SStructMatrixDestroy(A);
   HYPRE_ParCSRMatrixDestroy(T);
   HYPRE_SStructVectorDestroy(b);
   HYPRE_SStructVectorDestroy(x);


   DestroyData(data);

   hypre_TFree(parts);
   hypre_TFree(refine);
   hypre_TFree(distribute);
   hypre_TFree(block);

   hypre_FinalizeMemoryDebug();

   /* Finalize MPI */
   MPI_Finalize();

   return (0);
}
