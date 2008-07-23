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





/******************************************************************************
 *
 * Preconditioned conjugate gradient (Omin) functions
 *
 *****************************************************************************/

/* This was based on the pcg.c formerly in struct_ls, with
   changes (GetPrecond and stop_crit) for compatibility with the pcg.c
   in parcsr_ls and elsewhere.  Incompatibilities with the
   parcsr_ls version:
   - logging is different; no attempt has been made to be the same
   - treatment of b=0 in Ax=b is different: this returns x=0; the parcsr
   version iterates with a special stopping criterion
*/

#include "krylov.h"
#include "_hypre_utilities.h"

/*--------------------------------------------------------------------------
 * hypre_PCGFunctionsCreate
 *--------------------------------------------------------------------------*/

hypre_PCGFunctions *
hypre_PCGFunctionsCreate(
   char * (*CAlloc)        ( size_t count, size_t elt_size ),
   int    (*Free)          ( char *ptr ),
   int    (*CommInfo)      ( void  *A, int   *my_id, int   *num_procs ),
   void * (*CreateVector)  ( void *vector ),
   int    (*DestroyVector) ( void *vector ),
   void * (*MatvecCreate)  ( void *A, void *x ),
   int    (*Matvec)        ( void *matvec_data, double alpha, void *A,
                             void *x, double beta, void *y ),
   int    (*MatvecDestroy) ( void *matvec_data ),
   double (*InnerProd)     ( void *x, void *y ),
   int    (*CopyVector)    ( void *x, void *y ),
   int    (*ClearVector)   ( void *x ),
   int    (*ScaleVector)   ( double alpha, void *x ),
   int    (*Axpy)          ( double alpha, void *x, void *y ),
   int    (*PrecondSetup)  ( void *vdata, void *A, void *b, void *x ),
   int    (*Precond)       ( void *vdata, void *A, void *b, void *x )
   )
{
   hypre_PCGFunctions * pcg_functions;
   pcg_functions = (hypre_PCGFunctions *)
      CAlloc( 1, sizeof(hypre_PCGFunctions) );

   pcg_functions->CAlloc = CAlloc;
   pcg_functions->Free = Free;
   pcg_functions->CommInfo = CommInfo;
   pcg_functions->CreateVector = CreateVector;
   pcg_functions->DestroyVector = DestroyVector;
   pcg_functions->MatvecCreate = MatvecCreate;
   pcg_functions->Matvec = Matvec;
   pcg_functions->MatvecDestroy = MatvecDestroy;
   pcg_functions->InnerProd = InnerProd;
   pcg_functions->CopyVector = CopyVector;
   pcg_functions->ClearVector = ClearVector;
   pcg_functions->ScaleVector = ScaleVector;
   pcg_functions->Axpy = Axpy;
/* default preconditioner must be set here but can be changed later... */
   pcg_functions->precond_setup = PrecondSetup;
   pcg_functions->precond       = Precond;

   return pcg_functions;
}

/*--------------------------------------------------------------------------
 * hypre_PCGCreate
 *--------------------------------------------------------------------------*/

void *
hypre_PCGCreate( hypre_PCGFunctions *pcg_functions )
{
   hypre_PCGData *pcg_data;

   pcg_data = hypre_CTAllocF(hypre_PCGData, 1, pcg_functions);

   pcg_data -> functions = pcg_functions;

   /* set defaults */
   (pcg_data -> tol)          = 1.0e-06;
   (pcg_data -> atolf)        = 0.0;
   (pcg_data -> cf_tol)       = 0.0;
   (pcg_data -> a_tol)        = 0.0;
   (pcg_data -> max_iter)     = 1000;
   (pcg_data -> two_norm)     = 0;
   (pcg_data -> rel_change)   = 0;
   (pcg_data -> recompute_residual) = 0;
   (pcg_data -> stop_crit)    = 0;
   (pcg_data -> converged)    = 0;
   (pcg_data -> owns_matvec_data ) = 1;
   (pcg_data -> matvec_data)  = NULL;
   (pcg_data -> precond_data) = NULL;
   (pcg_data -> print_level)  = 0;
   (pcg_data -> logging)      = 0;
   (pcg_data -> norms)        = NULL;
   (pcg_data -> rel_norms)    = NULL;
   (pcg_data -> p)            = NULL;
   (pcg_data -> s)            = NULL;
   (pcg_data -> r)            = NULL;

   return (void *) pcg_data;
}

/*--------------------------------------------------------------------------
 * hypre_PCGDestroy
 *--------------------------------------------------------------------------*/

int
hypre_PCGDestroy( void *pcg_vdata )
{
   hypre_PCGData *pcg_data = pcg_vdata;
   hypre_PCGFunctions *pcg_functions = pcg_data->functions;

   if (pcg_data)
   {
      if ( (pcg_data -> norms) != NULL )
      {
         hypre_TFreeF( pcg_data -> norms, pcg_functions );
         pcg_data -> norms = NULL;
      } 
      if ( (pcg_data -> rel_norms) != NULL )
      {
         hypre_TFreeF( pcg_data -> rel_norms, pcg_functions );
         pcg_data -> rel_norms = NULL;
      }
      if ( pcg_data -> matvec_data != NULL && pcg_data->owns_matvec_data )
      {
         (*(pcg_functions->MatvecDestroy))(pcg_data -> matvec_data);
         pcg_data -> matvec_data = NULL;
      }
      if ( pcg_data -> p != NULL )
      {
         (*(pcg_functions->DestroyVector))(pcg_data -> p);
         pcg_data -> p = NULL;
      }
      if ( pcg_data -> s != NULL )
      {
         (*(pcg_functions->DestroyVector))(pcg_data -> s);
         pcg_data -> s = NULL;
      }
      if ( pcg_data -> r != NULL )
      {
         (*(pcg_functions->DestroyVector))(pcg_data -> r);
         pcg_data -> r = NULL;
      }
      hypre_TFreeF( pcg_data, pcg_functions );
      hypre_TFreeF( pcg_functions, pcg_functions );
   }

   return(hypre_error_flag);
}

/*--------------------------------------------------------------------------
 * hypre_PCGGetResidual
 *--------------------------------------------------------------------------*/

int hypre_PCGGetResidual( void *pcg_vdata, void **residual )
{
   /* returns a pointer to the residual vector */

   hypre_PCGData  *pcg_data     = pcg_vdata;
   *residual = pcg_data->r;
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetup
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetup( void *pcg_vdata,
                void *A,
                void *b,
                void *x         )
{
   hypre_PCGData *pcg_data = pcg_vdata;
   hypre_PCGFunctions *pcg_functions = pcg_data->functions;
   int            max_iter         = (pcg_data -> max_iter);
   int          (*precond_setup)() = (pcg_functions -> precond_setup);
   void          *precond_data     = (pcg_data -> precond_data);


   (pcg_data -> A) = A;

   /*--------------------------------------------------
    * The arguments for CreateVector are important to
    * maintain consistency between the setup and
    * compute phases of matvec and the preconditioner.
    *--------------------------------------------------*/

   if ( pcg_data -> p != NULL )
      (*(pcg_functions->DestroyVector))(pcg_data -> p);
   (pcg_data -> p) = (*(pcg_functions->CreateVector))(x);

   if ( pcg_data -> s != NULL )
      (*(pcg_functions->DestroyVector))(pcg_data -> s);
   (pcg_data -> s) = (*(pcg_functions->CreateVector))(x);

   if ( pcg_data -> r != NULL )
      (*(pcg_functions->DestroyVector))(pcg_data -> r);
   (pcg_data -> r) = (*(pcg_functions->CreateVector))(b);

   if ( pcg_data -> matvec_data != NULL && pcg_data->owns_matvec_data )
      (*(pcg_functions->MatvecDestroy))(pcg_data -> matvec_data);
   (pcg_data -> matvec_data) = (*(pcg_functions->MatvecCreate))(A, x);

   precond_setup(precond_data, A, b, x);

   /*-----------------------------------------------------
    * Allocate space for log info
    *-----------------------------------------------------*/

   if ( (pcg_data->logging)>0  || (pcg_data->print_level)>0 ) 
   {
      if ( (pcg_data -> norms) != NULL )
         hypre_TFreeF( pcg_data -> norms, pcg_functions );
      (pcg_data -> norms)     = hypre_CTAllocF( double, max_iter + 1,
                                                pcg_functions);

      if ( (pcg_data -> rel_norms) != NULL )
         hypre_TFreeF( pcg_data -> rel_norms, pcg_functions );
      (pcg_data -> rel_norms) = hypre_CTAllocF( double, max_iter + 1,
                                                pcg_functions );
   }

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSolve
 *--------------------------------------------------------------------------
 *
 * We use the following convergence test as the default (see Ashby, Holst,
 * Manteuffel, and Saylor):
 *
 *       ||e||_A                           ||r||_C
 *       -------  <=  [kappa_A(C*A)]^(1/2) -------  < tol
 *       ||x||_A                           ||b||_C
 *
 * where we let (for the time being) kappa_A(CA) = 1.
 * We implement the test as:
 *
 *       gamma = <C*r,r>/<C*b,b>  <  (tol^2) = eps
 *
 *--------------------------------------------------------------------------*/

int
hypre_PCGSolve( void *pcg_vdata,
                void *A,
                void *b,
                void *x         )
{
   hypre_PCGData  *pcg_data     = pcg_vdata;
   hypre_PCGFunctions *pcg_functions = pcg_data->functions;

   double          r_tol        = (pcg_data -> tol);
   double          a_tol        = (pcg_data -> a_tol);
   double          atolf        = (pcg_data -> atolf);
   double          cf_tol       = (pcg_data -> cf_tol);
   int             max_iter     = (pcg_data -> max_iter);
   int             two_norm     = (pcg_data -> two_norm);
   int             rel_change   = (pcg_data -> rel_change);
   int             recompute_residual = (pcg_data -> recompute_residual);
   int             stop_crit    = (pcg_data -> stop_crit);
/*
   int             converged    = (pcg_data -> converged);
*/
   void           *p            = (pcg_data -> p);
   void           *s            = (pcg_data -> s);
   void           *r            = (pcg_data -> r);
   void           *matvec_data  = (pcg_data -> matvec_data);
   int           (*precond)()   = (pcg_functions -> precond);
   void           *precond_data = (pcg_data -> precond_data);
   int             print_level  = (pcg_data -> print_level);
   int             logging      = (pcg_data -> logging);
   double         *norms        = (pcg_data -> norms);
   double         *rel_norms    = (pcg_data -> rel_norms);
                
   double          alpha, beta;
   double          gamma, gamma_old;
   double          bi_prod, i_prod, eps;
   double          pi_prod, xi_prod;
   double          ieee_check = 0.;
                
   double          i_prod_0;
   double          cf_ave_0 = 0.0;
   double          cf_ave_1 = 0.0;
   double          weight;
   double          ratio;

   double          guard_zero_residual, sdotp;
   int             tentatively_converged = 0; 

   int             i = 0;
   int             my_id, num_procs;

   (pcg_data -> converged) = 0;

   (*(pcg_functions->CommInfo))(A,&my_id,&num_procs);

   /*-----------------------------------------------------------------------
    * With relative change convergence test on, it is possible to attempt
    * another iteration with a zero residual. This causes the parameter
    * alpha to go NaN. The guard_zero_residual parameter is to circumvent
    * this. Perhaps it should be set to something non-zero (but small).
    *-----------------------------------------------------------------------*/

   guard_zero_residual = 0.0;

   /*-----------------------------------------------------------------------
    * Start pcg solve
    *-----------------------------------------------------------------------*/

   /* compute eps */
   if (two_norm)
   {
      /* bi_prod = <b,b> */
      bi_prod = (*(pcg_functions->InnerProd))(b, b);
      if (print_level > 1 && my_id == 0) 
          printf("<b,b>: %e\n",bi_prod);
   }
   else
   {
      /* bi_prod = <C*b,b> */
      (*(pcg_functions->ClearVector))(p);
      precond(precond_data, A, b, p);
      bi_prod = (*(pcg_functions->InnerProd))(p, b);
      if (print_level > 1 && my_id == 0)
          printf("<C*b,b>: %e\n",bi_prod);
   };

   /* Since it is does not diminish performance, attempt to return an error flag
      and notify users when they supply bad input. */
   if (bi_prod != 0.) ieee_check = bi_prod/bi_prod; /* INF -> NaN conversion */
   if (ieee_check != ieee_check)
   {
      /* ...INFs or NaNs in input can make ieee_check a NaN.  This test
         for ieee_check self-equality works on all IEEE-compliant compilers/
         machines, c.f. page 8 of "Lecture Notes on the Status of IEEE 754"
         by W. Kahan, May 31, 1996.  Currently (July 2002) this paper may be
         found at http://HTTP.CS.Berkeley.EDU/~wkahan/ieee754status/IEEE754.PDF */
      if (print_level > 0 || logging > 0)
      {
        printf("\n\nERROR detected by Hypre ...  BEGIN\n");
        printf("ERROR -- hypre_PCGSolve: INFs and/or NaNs detected in input.\n");
        printf("User probably placed non-numerics in supplied b.\n");
        printf("Returning error flag += 101.  Program not terminated.\n");
        printf("ERROR detected by Hypre ...  END\n\n\n");
      }
      hypre_error(HYPRE_ERROR_GENERIC);
      return hypre_error_flag;
   }

   eps = r_tol*r_tol; /* note: this may be re-assigned below */
   if ( bi_prod > 0.0 ) {
      if ( stop_crit && !rel_change && atolf<=0 ) {  /* pure absolute tolerance */
         eps = eps / bi_prod;
         /* Note: this section is obsolete.  Aside from backwards comatability
            concerns, we could delete the stop_crit parameter and related code,
            using tol & atolf instead. */
      }
      else if ( atolf>0 )  /* mixed relative and absolute tolerance */
         bi_prod += atolf;
      else /* DEFAULT (stop_crit and atolf exist for backwards compatibilty
              and are not in the reference manual) */
      {
        /* convergence criteria:  <C*r,r>  <= max( a_tol^2, r_tol^2 * <C*b,b> )
            note: default for a_tol is 0.0, so relative residual criteria is used unless
            user specifies a_tol, or sets r_tol = 0.0, which means absolute
            tol only is checked  */
         eps = hypre_max(r_tol*r_tol, a_tol*a_tol/bi_prod);
         
      }
   }
   else    /* bi_prod==0.0: the rhs vector b is zero */
   {
      /* Set x equal to zero and return */
      (*(pcg_functions->CopyVector))(b, x);
      if (logging>0 || print_level>0)
      {
         norms[0]     = 0.0;
         rel_norms[i] = 0.0;
      }

      return hypre_error_flag;
      /* In this case, for the original parcsr pcg, the code would take special
         action to force iterations even though the exact value was known. */
   };

   /* r = b - Ax */
   (*(pcg_functions->CopyVector))(b, r);
   (*(pcg_functions->Matvec))(matvec_data, -1.0, A, x, 1.0, r);
 
   /* p = C*r */
   (*(pcg_functions->ClearVector))(p);
   precond(precond_data, A, r, p);

   /* gamma = <r,p> */
   gamma = (*(pcg_functions->InnerProd))(r,p);

   /* Since it is does not diminish performance, attempt to return an error flag
      and notify users when they supply bad input. */
   if (gamma != 0.) ieee_check = gamma/gamma; /* INF -> NaN conversion */
   if (ieee_check != ieee_check)
   {
      /* ...INFs or NaNs in input can make ieee_check a NaN.  This test
         for ieee_check self-equality works on all IEEE-compliant compilers/
         machines, c.f. page 8 of "Lecture Notes on the Status of IEEE 754"
         by W. Kahan, May 31, 1996.  Currently (July 2002) this paper may be
         found at http://HTTP.CS.Berkeley.EDU/~wkahan/ieee754status/IEEE754.PDF */
      if (print_level > 0 || logging > 0)
      {
        printf("\n\nERROR detected by Hypre ...  BEGIN\n");
        printf("ERROR -- hypre_PCGSolve: INFs and/or NaNs detected in input.\n");
        printf("User probably placed non-numerics in supplied A or x_0.\n");
        printf("Returning error flag += 101.  Program not terminated.\n");
        printf("ERROR detected by Hypre ...  END\n\n\n");
      }
      hypre_error(HYPRE_ERROR_GENERIC);
      return hypre_error_flag;
   }

   /* Set initial residual norm */
   if ( logging>0 || print_level > 0 || cf_tol > 0.0 )
   {
      if (two_norm)
         i_prod_0 = (*(pcg_functions->InnerProd))(r,r);
      else
         i_prod_0 = gamma;

      if ( logging>0 || print_level>0 ) norms[0] = sqrt(i_prod_0);
   }
   if ( print_level > 1 && my_id==0 )
   {
      printf("\n\n");
      if (two_norm)
      {
         if ( stop_crit && !rel_change && atolf==0 ) {  /* pure absolute tolerance */
            printf("Iters       ||r||_2     conv.rate\n");
            printf("-----    ------------   ---------\n");
         }
         else {
            printf("Iters       ||r||_2     conv.rate  ||r||_2/||b||_2\n");
            printf("-----    ------------   ---------  ------------ \n");
         }
      }
      else  /* !two_norm */
      {
         printf("Iters       ||r||_C     conv.rate  ||r||_C/||b||_C\n");
         printf("-----    ------------    ---------  ------------ \n");
      }
   }

   while ((i+1) <= max_iter)
   {
      /*--------------------------------------------------------------------
       * the core CG calculations...
       *--------------------------------------------------------------------*/
      i++;

      /* s = A*p */
      (*(pcg_functions->Matvec))(matvec_data, 1.0, A, p, 0.0, s);

      /* alpha = gamma / <s,p> */
      sdotp = (*(pcg_functions->InnerProd))(s, p);
      if ( sdotp==0.0 )
      {
         /* ++ierr;*/
         if (i==1) i_prod=i_prod_0;
         break;
      }
      alpha = gamma / sdotp;

      gamma_old = gamma;

      /* x = x + alpha*p */
      (*(pcg_functions->Axpy))(alpha, p, x);

      /* r = r - alpha*s */
      (*(pcg_functions->Axpy))(-alpha, s, r);
         
      /* s = C*r */
      (*(pcg_functions->ClearVector))(s);
      precond(precond_data, A, r, s);

      /* gamma = <r,s> */
      gamma = (*(pcg_functions->InnerProd))(r, s);

      /* set i_prod for convergence test */
      if (two_norm)
         i_prod = (*(pcg_functions->InnerProd))(r,r);
      else
         i_prod = gamma;

      /*--------------------------------------------------------------------
       * optional output
       *--------------------------------------------------------------------*/
#if 0
      if (two_norm)
         printf("Iter (%d): ||r||_2 = %e, ||r||_2/||b||_2 = %e\n",
                i, sqrt(i_prod), (bi_prod ? sqrt(i_prod/bi_prod) : 0));
      else
         printf("Iter (%d): ||r||_C = %e, ||r||_C/||b||_C = %e\n",
                i, sqrt(i_prod), (bi_prod ? sqrt(i_prod/bi_prod) : 0));
#endif
 
      /* print norm info */
      if ( logging>0 || print_level>0 )
      {
         norms[i]     = sqrt(i_prod);
         rel_norms[i] = bi_prod ? sqrt(i_prod/bi_prod) : 0;
      }
      if ( print_level > 1 && my_id==0 )
      {
         if (two_norm)
         {
            if ( stop_crit && !rel_change && atolf==0 ) {  /* pure absolute tolerance */
               printf("% 5d    %e    %f\n", i, norms[i],
                      norms[i]/norms[i-1] );
            }
            else 
            {
               printf("% 5d    %e    %f    %e\n", i, norms[i],
                      norms[i]/norms[i-1], rel_norms[i] );
            }
         }
         else 
         {
               printf("% 5d    %e    %f    %e\n", i, norms[i],
                      norms[i]/norms[i-1], rel_norms[i] );
         }
      }


      /*--------------------------------------------------------------------
       * check for convergence
       *--------------------------------------------------------------------*/
      if (i_prod / bi_prod < eps)  /* the basic convergence test */
            tentatively_converged = 1;
      if ( tentatively_converged && recompute_residual )
         /* At user request, don't trust the convergence test until we've recomputed
            the residual from scratch.  This is expensive in the usual case where an
            the norm is the energy norm.
            This calculation is coded on the assumption that r's accuracy is only a
            concern for problems where CG takes many iterations. */
      {
         /* r = b - Ax */
         (*(pcg_functions->CopyVector))(b, r);
         (*(pcg_functions->Matvec))(matvec_data, -1.0, A, x, 1.0, r);

         /* set i_prod for convergence test */
         if (two_norm)
            i_prod = (*(pcg_functions->InnerProd))(r,r);
         else
         {
            /* s = C*r */
            (*(pcg_functions->ClearVector))(s);
            precond(precond_data, A, r, s);
            /* iprod = gamma = <r,s> */
            i_prod = (*(pcg_functions->InnerProd))(r, s);
         }
         if (i_prod / bi_prod >= eps) tentatively_converged = 0;
      }
      if ( tentatively_converged && rel_change && (i_prod > guard_zero_residual ))
         /* At user request, don't treat this as converged unless x didn't change
            much in the last iteration. */
      {
	    pi_prod = (*(pcg_functions->InnerProd))(p,p); 
 	    xi_prod = (*(pcg_functions->InnerProd))(x,x);
            ratio = alpha*alpha*pi_prod/xi_prod;
            if (ratio >= eps) tentatively_converged = 0;
      }
      if ( tentatively_converged )
         /* we've passed all the convergence tests, it's for real */
      {
         (pcg_data -> converged) = 1;
         break;
      }

      if ( (gamma<1.0e-292) && ((-gamma)<1.0e-292) ) {
         /* ierr = 1;*/
         hypre_error(HYPRE_ERROR_CONV);
         
         break;
      }
      /* ... gamma should be >=0.  IEEE subnormal numbers are < 2**(-1022)=2.2e-308
         (and >= 2**(-1074)=4.9e-324).  So a gamma this small means we're getting
         dangerously close to subnormal or zero numbers (usually if gamma is small,
         so will be other variables).  Thus further calculations risk a crash.
         Such small gamma generally means no hope of progress anyway. */

      /*--------------------------------------------------------------------
       * Optional test to see if adequate progress is being made.
       * The average convergence factor is recorded and compared
       * against the tolerance 'cf_tol'. The weighting factor is  
       * intended to pay more attention to the test when an accurate
       * estimate for average convergence factor is available.  
       *--------------------------------------------------------------------*/

      if (cf_tol > 0.0)
      {
         cf_ave_0 = cf_ave_1;
         if ( i_prod_0<1.0e-292 ) {
            /* i_prod_0 is zero, or (almost) subnormal, yet i_prod wasn't small
               enough to pass the convergence test.  Therefore initial guess was good,
               and we're just calculating garbage - time to bail out before the
               next step, which will be a divide by zero (or close to it). */
            /* ierr = 1; */
            hypre_error(HYPRE_ERROR_CONV);
            
            break;
         }
	 cf_ave_1 = pow( i_prod / i_prod_0, 1.0/(2.0*i)); 

         weight   = fabs(cf_ave_1 - cf_ave_0);
         weight   = weight / hypre_max(cf_ave_1, cf_ave_0);
         weight   = 1.0 - weight;
#if 0
         printf("I = %d: cf_new = %e, cf_old = %e, weight = %e\n",
                i, cf_ave_1, cf_ave_0, weight );
#endif
         if (weight * cf_ave_1 > cf_tol) break;
      }

      /*--------------------------------------------------------------------
       * back to the core CG calculations
       *--------------------------------------------------------------------*/

      /* beta = gamma / gamma_old */
      beta = gamma / gamma_old;

      /* p = s + beta p */
      (*(pcg_functions->ScaleVector))(beta, p);   
      (*(pcg_functions->Axpy))(1.0, s, p);
   }

   /*--------------------------------------------------------------------
    * Finish up with some outputs.
    *--------------------------------------------------------------------*/

   if ( print_level > 1 && my_id==0 )
      printf("\n\n");

   (pcg_data -> num_iterations) = i;
   if (bi_prod > 0.0)
      (pcg_data -> rel_residual_norm) = sqrt(i_prod/bi_prod);
   else /* actually, we'll never get here... */
      (pcg_data -> rel_residual_norm) = 0.0;

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetTol, hypre_PCGGetTol
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetTol( void   *pcg_vdata,
                 double  tol       )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> tol) = tol;
 
   return hypre_error_flag;
}

int
hypre_PCGGetTol( void   *pcg_vdata,
                 double * tol       )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   *tol = (pcg_data -> tol);
 
   return hypre_error_flag;
}
/*--------------------------------------------------------------------------
 * hypre_PCGSetAbsoluteTol, hypre_PCGGetAbsoluteTol
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetAbsoluteTol( void   *pcg_vdata,
                 double  a_tol       )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> a_tol) = a_tol;
 
   return hypre_error_flag;
}

int
hypre_PCGGetAbsoluteTol( void   *pcg_vdata,
                 double * a_tol       )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   *a_tol = (pcg_data -> a_tol);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetAbsoluteTolFactor, hypre_PCGGetAbsoluteTolFactor
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetAbsoluteTolFactor( void   *pcg_vdata,
                               double  atolf   )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> atolf) = atolf;
 
   return hypre_error_flag;
}

int
hypre_PCGGetAbsoluteTolFactor( void   *pcg_vdata,
                               double  * atolf   )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   *atolf = (pcg_data -> atolf);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetConvergenceFactorTol, hypre_PCGGetConvergenceFactorTol
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetConvergenceFactorTol( void   *pcg_vdata,
                                  double  cf_tol   )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> cf_tol) = cf_tol;
 
   return hypre_error_flag;
}

int
hypre_PCGGetConvergenceFactorTol( void   *pcg_vdata,
                                  double * cf_tol   )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   *cf_tol = (pcg_data -> cf_tol);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetMaxIter, hypre_PCGGetMaxIter
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetMaxIter( void *pcg_vdata,
                     int   max_iter  )
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> max_iter) = max_iter;
 
   return hypre_error_flag;
}

int
hypre_PCGGetMaxIter( void *pcg_vdata,
                     int * max_iter  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *max_iter = (pcg_data -> max_iter);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetTwoNorm, hypre_PCGGetTwoNorm
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetTwoNorm( void *pcg_vdata,
                     int   two_norm  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   (pcg_data -> two_norm) = two_norm;
 
   return hypre_error_flag;
}

int
hypre_PCGGetTwoNorm( void *pcg_vdata,
                     int * two_norm  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *two_norm = (pcg_data -> two_norm);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetRelChange, hypre_PCGGetRelChange
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetRelChange( void *pcg_vdata,
                       int   rel_change  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   (pcg_data -> rel_change) = rel_change;
 
   return hypre_error_flag;
}

int
hypre_PCGGetRelChange( void *pcg_vdata,
                       int * rel_change  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *rel_change = (pcg_data -> rel_change);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetRecomputeResidual, hypre_PCGGetRecomputeResidual
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetRecomputeResidual( void *pcg_vdata,
                       int   recompute_residual  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   (pcg_data -> recompute_residual) = recompute_residual;
 
   return hypre_error_flag;
}

int
hypre_PCGGetRecomputeResidual( void *pcg_vdata,
                       int * recompute_residual  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *recompute_residual = (pcg_data -> recompute_residual);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetStopCrit, hypre_PCGGetStopCrit
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetStopCrit( void *pcg_vdata,
                       int   stop_crit  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   (pcg_data -> stop_crit) = stop_crit;
 
   return hypre_error_flag;
}

int
hypre_PCGGetStopCrit( void *pcg_vdata,
                       int * stop_crit  )
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *stop_crit = (pcg_data -> stop_crit);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGGetPrecond
 *--------------------------------------------------------------------------*/

int
hypre_PCGGetPrecond( void         *pcg_vdata,
                     HYPRE_Solver *precond_data_ptr )
{
   hypre_PCGData *pcg_data = pcg_vdata;


   *precond_data_ptr = (HYPRE_Solver)(pcg_data -> precond_data);

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetPrecond
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetPrecond( void  *pcg_vdata,
                     int  (*precond)(),
                     int  (*precond_setup)(),
                     void  *precond_data )
{
   hypre_PCGData *pcg_data = pcg_vdata;
   hypre_PCGFunctions *pcg_functions = pcg_data->functions;

 
   (pcg_functions -> precond)       = precond;
   (pcg_functions -> precond_setup) = precond_setup;
   (pcg_data -> precond_data)  = precond_data;
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetPrintLevel, hypre_PCGGetPrintLevel
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetPrintLevel( void *pcg_vdata,
                        int   level)
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   (pcg_data -> print_level) = level;
 
   return hypre_error_flag;
}

int
hypre_PCGGetPrintLevel( void *pcg_vdata,
                        int * level)
{
   hypre_PCGData *pcg_data = pcg_vdata;

 
   *level = (pcg_data -> print_level);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGSetLogging, hypre_PCGGetLogging
 *--------------------------------------------------------------------------*/

int
hypre_PCGSetLogging( void *pcg_vdata,
                      int   level)
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   (pcg_data -> logging) = level;
 
   return hypre_error_flag;
}

int
hypre_PCGGetLogging( void *pcg_vdata,
                      int * level)
{
   hypre_PCGData *pcg_data = pcg_vdata;
 
   *level = (pcg_data -> logging);
 
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGGetNumIterations
 *--------------------------------------------------------------------------*/

int
hypre_PCGGetNumIterations( void *pcg_vdata,
                           int  *num_iterations )
{
   hypre_PCGData *pcg_data = pcg_vdata;

   *num_iterations = (pcg_data -> num_iterations);

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGGetConverged
 *--------------------------------------------------------------------------*/

int
hypre_PCGGetConverged( void *pcg_vdata,
                       int  *converged)
{
   hypre_PCGData *pcg_data = pcg_vdata;

   *converged = (pcg_data -> converged);

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGPrintLogging
 *--------------------------------------------------------------------------*/

int
hypre_PCGPrintLogging( void *pcg_vdata,
                       int   myid)
{
   hypre_PCGData *pcg_data = pcg_vdata;

   int            num_iterations  = (pcg_data -> num_iterations);
   int            print_level     = (pcg_data -> print_level);
   double        *norms           = (pcg_data -> norms);
   double        *rel_norms       = (pcg_data -> rel_norms);

   int            i;

   if (myid == 0)
   {
      if (print_level > 0)
      {
         for (i = 0; i < num_iterations; i++)
         {
            printf("Residual norm[%d] = %e   ", i, norms[i]);
            printf("Relative residual norm[%d] = %e\n", i, rel_norms[i]);
         }
      }
   }
  
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PCGGetFinalRelativeResidualNorm
 *--------------------------------------------------------------------------*/

int
hypre_PCGGetFinalRelativeResidualNorm( void   *pcg_vdata,
                                       double *relative_residual_norm )
{
   hypre_PCGData *pcg_data = pcg_vdata;

   double         rel_residual_norm = (pcg_data -> rel_residual_norm);

  *relative_residual_norm = rel_residual_norm;
   
   return hypre_error_flag;
}

