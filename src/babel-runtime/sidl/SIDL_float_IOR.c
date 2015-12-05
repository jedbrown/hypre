/*
 * File:        SIDL_float_IOR.c
 * Copyright:   (c) 2001 The Regents of the University of California
 * Release:     $Name: V1-9-0b $
 * Revision:    @(#) $Revision: 1.4 $
 * Date:        $Date: 2003/04/07 21:44:31 $
 * Description: float array implementation
 * AUTOMATICALLY GENERATED BY createarrays.py
 *
 */

#include "SIDL_float_IOR.h"
#include <stdlib.h>
#include <string.h>

#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

static void swap_i32(int32_t *i1, int32_t *i2)
{
  int32_t tmp = *i1;
  *i1 = *i2;
  *i2 = tmp;
}

#define COPY_VALUE(x) (x)
#define DESTROY_VALUE(x)
#define INIT_VALUES(ptr,size)

static float const SIDL_float__array_zero = 0;

/**
 * Destroy the given borrowed array. Trying to destroy a NULL array is
 * a noop.
 */
static void
SIDL_float__array_bdestroy(struct SIDL_float__array* array)
{
  if (array) {
    memset(array, 0, sizeof(struct SIDL_float__array) +
           3 * array->d_dimen * sizeof(int32_t));
    free(array);
  }
}

/**
 * Destroy the given array. Trying to destroy a NULL array is a noop.
 */
static void
SIDL_float__array_destroy(struct SIDL_float__array* array)
{
  if (array) {
#ifdef DESTROY_VALUES_TOO
    float *ptr = array->d_firstElement;
    int32_t i;
    int32_t size = 1;
    for(i = 0; i < array->d_dimen;++i){
      size *= (1 + array->d_upper[i] - array->d_lower[i]);
    }
    while (size--) {
      DESTROY_VALUE(*ptr);
      *(ptr++) = NULL;
    }
#endif
    free(array->d_firstElement);
    SIDL_float__array_bdestroy(array);
  }
}

/**
 * Destroy the given sliced array. Trying to destroy a NULL array is
 * a noop.
 */
static void
SIDL_float__array_sdestroy(struct SIDL_float__array* array)
{
  if (array) {
    static const int arraySize = sizeof(struct SIDL_float__array)
      + (sizeof(int32_t) - (sizeof(struct SIDL_float__array)
                        % sizeof(int32_t))) % sizeof(int32_t);
    struct SIDL_float__array **orig;
    orig = (struct SIDL_float__array **)((char *)array + arraySize +
      3 * sizeof(int32_t)*array->d_dimen);
    if (*orig) {
      SIDL_float__array_deleteRef(*orig);
      *orig = NULL;
    }
    SIDL_float__array_bdestroy(array);
  }
}

/**
 * Smart copy routine for self-sufficient and sliced arrays.  This just
 * increments the reference count and returns the same array.
 */
static struct SIDL_float__array*
SIDL_float__array_smartCp(struct SIDL_float__array* array)
{
  SIDL_float__array_addRef(array);
  return array;
}

/**
 * Smart copy routine for self-sufficient and sliced arrays.  This just
 * increments the reference count and returns the same array.
 */
static struct SIDL_float__array*
SIDL_float__array_borrowSmartCp(struct SIDL_float__array* array)
{
  struct SIDL_float__array* copy = NULL;
  if (array) {
    if (SIDL_float__array_isColumnOrder(array)) {
      copy = SIDL_float__array_createCol(array->d_dimen,
                                                array->d_lower,
                                                array->d_upper);
    }
    else {
      copy = SIDL_float__array_createRow(array->d_dimen,
                                                array->d_lower,
                                                array->d_upper);
    }
    SIDL_float__array_copy(array, copy);
  }
  return copy;
}

/**
 * Virtual function table for normal, self-sufficient arrays.
 */

static const struct SIDL_float__vtable normal_float_vtable = {
  SIDL_float__array_destroy,
  SIDL_float__array_smartCp
};
/**
 * Virtual function table for borrowed arrays.
 */

static const struct SIDL_float__vtable borrowed_float_vtable = {
  SIDL_float__array_bdestroy,
  SIDL_float__array_borrowSmartCp
};
/**
 * Virtual function table for sliced arrays with self-sufficient original
 * arrays.
 */

static const struct SIDL_float__vtable sliced_float_vtable = {
  SIDL_float__array_sdestroy,
  SIDL_float__array_smartCp
};
/**
 * Virtual function table for sliced arrays with borrowed original
 * arrays.
 */

static const struct SIDL_float__vtable bsliced_float_vtable = {
  SIDL_float__array_sdestroy,
  SIDL_float__array_borrowSmartCp
};
/**
 * Allocate memory for the array meta-data and initialize the reference
 * count to one.
 */
static struct SIDL_float__array*
newArray(int32_t dimen, const int32_t lower[], const int32_t upper[],
         struct SIDL_float__array *orig)
{
  static const int arraySize = sizeof(struct SIDL_float__array)
    + (sizeof(int32_t) - (sizeof(struct SIDL_float__array)
                      % sizeof(int32_t))) % sizeof(int32_t);
  struct SIDL_float__array *result = (struct SIDL_float__array *)
    malloc(arraySize + 3 * sizeof(int32_t) * dimen +
           (orig ? sizeof(struct SIDL_float__array *) : 0));
  result->d_dimen = dimen;
  result->d_refcount = 1;
  result->d_lower = (int32_t *)((char *)result + arraySize);
  result->d_upper = result->d_lower + dimen;
  result->d_stride = result->d_upper + dimen;
  if (orig) {
    struct SIDL_float__array **ref;
    ref = (struct SIDL_float__array **)
      ((char *)result + arraySize + 3 * sizeof(int32_t)*dimen);
    *ref = orig;
    SIDL_float__array_addRef(orig);
    result->d_vtable = &sliced_float_vtable;
  }
  else {
    result->d_vtable = &normal_float_vtable;
  }
  memcpy(result->d_lower, lower, sizeof(int32_t)*dimen);
  memcpy(result->d_upper, upper, sizeof(int32_t)*dimen);
  return result;
}

/**
 * Create a dense float array of the given dimension with specified
 * index bounds in column-major order.  This array owns and manages its data.
 * This function does not initialize the contents of the array.
 */
struct SIDL_float__array *
SIDL_float__array_createCol(int32_t       dimen,
                            const int32_t lower[],
                            const int32_t upper[])
{
  int32_t size=1, i;
  struct SIDL_float__array *result = newArray(dimen, lower, upper, NULL);
  for(i = 0; i < dimen; ++i) {
    result->d_stride[i] = size;
    size *= (1 + upper[i] - lower[i]);
  }
  size *= sizeof(float);
  result->d_firstElement = (float *)malloc(size);
  INIT_VALUES(result->d_firstElement, size);
  return result;
}

/**
 * Create a dense float array of the given dimension with specified
 * index bounds in row-major order.  This array owns and manages its data.
 * This function does not initialize the contents of the array.
 */
struct SIDL_float__array *
SIDL_float__array_createRow(int32_t       dimen,
                            const int32_t lower[],
                            const int32_t upper[])
{
  int32_t size=1, i;
  struct SIDL_float__array *result = newArray(dimen, lower, upper, NULL);
  for(i = dimen-1; i >= 0; --i) {
    result->d_stride[i] = size;
    size *= (1 + upper[i] - lower[i]);
  }
  size *= sizeof(float);
  result->d_firstElement = (float *)malloc(size);
  INIT_VALUES(result->d_firstElement, size);
  return result;
}

/**
 * Create a dense one-dimensional vector of floats with a lower
 * index of 0 and an upper index of len-1. This array owns and manages
 * its data.
 * This function does not initialize the contents of the array.
 */
struct SIDL_float__array *
SIDL_float__array_create1d(int32_t len)
{
  static const int32_t zero = 0;
  --len;
  return SIDL_float__array_createCol(1, &zero, &len);
}

/**
 * Create a dense two-dimensional array of floats with a lower
 * indices of (0,0) and an upper indices of (m-1,n-1). The array is
 * stored in column-major order, and it owns and manages its data.
 * This function does not initialize the contents of the array.
 */
struct SIDL_float__array *
SIDL_float__array_create2dCol(int32_t m, int32_t n)
{
  static const int32_t zero[2] = {0, 0};
  int32_t upper[2];
  upper[0] = m-1;
  upper[1] = n-1;
  return SIDL_float__array_createCol(2, zero, upper);
}

/**
 * Create a dense two-dimensional array of floats with a lower
 * indices of (0,0) and an upper indices of (m-1,n-1). The array is
 * stored in row-major order, and it owns and manages its data.
 * This function does not initialize the contents of the array.
 */
struct SIDL_float__array *
SIDL_float__array_create2dRow(int32_t m, int32_t n)
{
  static const int32_t zero[2] = {0, 0};
  int32_t upper[2];
  upper[0] = m-1;
  upper[1] = n-1;
  return SIDL_float__array_createRow(2, zero, upper);
}

/**
 * Return the stride for a splice argument.
 */
static int32_t
getStride(const int32_t *srcStride, const int32_t i)
{
  return srcStride ? srcStride[i] : 1;
}
/**
 * Check the arguments of a splice call to make sure they're valid.
 */
static int
goodSliceArgs(struct SIDL_float__array* src,
               int32_t       dimen,
               const int32_t *srcStart,
               const int32_t *srcStride,
               const int32_t *newStart,
               const int32_t *numElem)
{
  if (src && numElem && (dimen > 0) && (dimen <= src->d_dimen)) {
    int32_t i, numZeros;
    const int32_t *srcFirst = (srcStart ? srcStart : src->d_lower);
    for(i = 0, numZeros=0; i < src->d_dimen; ++i) {
      if ((srcFirst[i] < src->d_lower[i]) ||
          (srcFirst[i] > src->d_upper[i]) ||
          (numElem[i] &&
           (((srcFirst[i] + (numElem[i]-1)*getStride(srcStride,i)) >
              src->d_upper[i]) ||
            ((srcFirst[i] + (numElem[i]-1)*getStride(srcStride,i)) <
              src->d_lower[i]))))
        return 0;
      if (!numElem[i]) ++numZeros;
    }
    return (dimen + numZeros) == src->d_dimen;
  }
  return 0;
}

/**
 * Create a sub-array of another array. This resulting array shares
 * data with the original array. The new array can be of the same
 * dimension or potentially less assuming the original array has
 * dimension greater than 1.  If you are removing dimension, indicate the
 * dimensions to remove by setting numElem[i] to zero for any dimension i
 * that should go away in the new array.  The meaning of each argument
 * is covered below.
 * 
 * src       the array to be created will be a subset of this array. If
 *           this argument is NULL, NULL will be returned. The array
 *           returned borrows data from src, so modifying src or the
 *           returned array will modify both arrays.
 * 
 * dimen     this argument must be greater than zero and less than or
 *           equal to the dimension of src. An illegal value will cause
 *           a NULL return value.
 *           
 * numElem   this specifies how many elements from src should be taken in
 *           each dimension. A zero entry indicates that the dimension
 *           should not appear in the new array.  This argument should be
 *           an array with an entry for each dimension of src.  Passing
 *           NULL here will cause NULL to be returned.  If
 *           srcStart[i] + numElem[i]*srcStride[i] is greater than
 *           upper[i] for src or if srcStart[i] + numElem[i]*srcStride[i]
 *           is less than lower[i] for src, NULL will be returned.
 * 
 * srcStart  this array holds the coordinates of the first element of the
 *           new array. If this argument is NULL, the first element of
 *           src will be the first element of the new array. If non-NULL,
 *           this argument should be an array with an entry for each
 *           dimension of src.  If srcStart[i] is less than lower[i] for
 *           the array src, NULL will be returned.
 * 
 * srcStride this array lets you specify the stride between elements in
 *           each dimension of src. This stride is relative to the
 *           coordinate system of the src array. If this argument is
 *           NULL, the stride is taken to be one in each dimension.
 *           If non-NULL, this argument should be an array with an entry
 *           for each dimension of src.
 * 
 * newLower  this argument is like lower in a create method. It sets the
 *           coordinates for the first element in the new array.  If this
 *           argument is NULL, the values indicated by srcStart will be
 *           used. If non-NULL, this should be an array with dimen
 *           elements.
 */
struct SIDL_float__array *
SIDL_float__array_slice(struct SIDL_float__array *src,
                        int32_t       dimen,
                        const int32_t numElem[],
                        const int32_t *srcStart,
                        const int32_t *srcStride,
                        const int32_t *newStart)
{
  struct SIDL_float__array *result = NULL;
  if (goodSliceArgs(src, dimen, srcStart, srcStride, newStart, numElem)) {
    const int32_t *srcFirst = (srcStart ? srcStart : src->d_lower);
    const int32_t *newFirst = (newStart ? newStart : srcFirst);
    int32_t *newLast = malloc(sizeof(int32_t)*dimen);
    int32_t i, j;
    if (!newLast) return NULL;
    for(i = 0, j = 0; i < src->d_dimen; ++i) {
      if (numElem[i]) {
        newLast[j] = newFirst[j] + numElem[i] - 1;
        ++j;
      }
    }
    result = newArray(dimen, newFirst, newLast, src);
    free(newLast);
    if (result) {
      result->d_firstElement = src->d_firstElement;
      for(i = 0, j = 0; i < src->d_dimen; ++i) {
        result->d_firstElement +=
          ((srcFirst[i] - src->d_lower[i])*src->d_stride[i]);
        if (numElem[i]) {
          result->d_stride[j] = src->d_stride[i]*getStride(srcStride, i);
          ++j;
        }
      }
      if (src->d_vtable->d_smartcopy ==
          SIDL_float__array_smartCp) {
        result->d_vtable = &sliced_float_vtable;
      }
      else {
        result->d_vtable = &bsliced_float_vtable;
      }
    }
  }
  return result;
}

/**
 * Create an array that uses data memory from another source. This initial
 * contents are determined by the data being borrowed.
 */
struct SIDL_float__array *
SIDL_float__array_borrow(float* firstElement,
                         int32_t        dimen,
                         const int32_t  lower[],
                         const int32_t  upper[],
                         const int32_t  stride[])
{
  struct SIDL_float__array *result = newArray(dimen, lower, upper, NULL);
  memcpy(result->d_stride, stride, sizeof(int32_t)*dimen);
  result->d_firstElement = firstElement;
  result->d_vtable = &borrowed_float_vtable;
  return result;
}

/**
 * If array is borrowed, allocate a new self-sufficient array and copy
 * the borrowed array into the new array; otherwise, increment the
 * reference count and return the array passed in. Use this whenever
 * you want to make a copy of a method argument because arrays passed
 * into methods aren't guaranteed to exist after the method call.
 */
struct SIDL_float__array *
SIDL_float__array_smartCopy(struct SIDL_float__array *array)
{
  return array ? ((array->d_vtable->d_smartcopy)(array)) : NULL;
}
/**
 * Increment the arrays internal reference count by one. To make a
 * persistent copy (i.e. that lives longer than the current method
 * call) use smartCopy.
 */
void
SIDL_float__array_addRef(struct SIDL_float__array* array)
{
  if (array) {
    ++(array->d_refcount);
  }
}

/**
 * Decrement the arrays internal reference count by one. If the reference
 * count goes to zero, destroy the array.
 * Return true iff the array is destroyed
 */
void
SIDL_float__array_deleteRef(struct SIDL_float__array* array)
{
  if (array && !(--(array->d_refcount))) {
    /* self destruct */
    (*(array->d_vtable->d_destroy))(array);
  }
}

/**
 * Retrieve element i1 of a one-dimensional array.
 */
float
SIDL_float__array_get1(const struct SIDL_float__array* array,
                       const int32_t i1)
{
  if (array && (1 == array->d_dimen) &&
      ((array->d_lower[0] <= i1) && (array->d_upper[0] >= i1))) {
    return COPY_VALUE(*(array->d_firstElement +
                        (i1 - array->d_lower[0])*array->d_stride[0]));
  }
  return SIDL_float__array_zero;
}

/**
 * Retrieve element (i1,i2) of a two-dimensional array.
 */
float
SIDL_float__array_get2(const struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2)
{
  if (array && (2 == array->d_dimen)){
    /* unserialize array bounds checking (i.e. avoid strict left to right */
    /* evaluation of && which serializes evaluation) */
    register int c1 = (array->d_lower[0] <= i1);
    register int c2 = (array->d_upper[0] >= i1);
    register int c3 = (array->d_lower[1] <= i2);
    register int c4 = (array->d_upper[1] >= i2);
    c1 = c1 && c2;
    c3 = c3 && c4;
    if (c1 && c3) {
      return COPY_VALUE(*(array->d_firstElement +
                          (i1 - array->d_lower[0])*array->d_stride[0] +
                          (i2 - array->d_lower[1])*array->d_stride[1]));
    }
  }
  return SIDL_float__array_zero;
}

/**
 * Retrieve element (i1,i2,i3) of a three-dimensional array.
 */
float
SIDL_float__array_get3(const struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2,
                       const int32_t i3)
{
  if (array && (3 == array->d_dimen)){
    /* unserialize array bounds checking (i.e. avoid strict left to right */
    /* evaluation of && which serializes evaluation) */
    register int c1 = (array->d_lower[0] <= i1);
    register int c2 = (array->d_lower[1] <= i2);
    register int c3 = (array->d_lower[2] <= i3);
    c1 = c1 && (array->d_upper[0] >= i1);
    c2 = c2 && (array->d_upper[1] >= i2);
    c3 = c3 && (array->d_upper[2] >= i3);
    if (c1 && c2 && c3) {
      return COPY_VALUE(*(array->d_firstElement +
                          (i1 - array->d_lower[0])*array->d_stride[0] +
                          (i2 - array->d_lower[1])*array->d_stride[1] +
                          (i3 - array->d_lower[2])*array->d_stride[2]));
    }
  }
  return SIDL_float__array_zero;
}

/**
 * Retrieve element (i1,i2,i3,i4) of a four-dimensional array.
 */
float
SIDL_float__array_get4(const struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2,
                       const int32_t i3,
                       const int32_t i4)
{
  if (array) {
    switch(array->d_dimen){
    case 1: return SIDL_float__array_get1(array, i1);
    case 2: return SIDL_float__array_get2(array, i1, i2);
    case 3: return SIDL_float__array_get3(array, i1, i2, i3);
    case 4:
      {
        /* unserialize array bounds checking to avoid strict left to right */
        /* evaluation of && which serializes evaluation */
        register int c1 = (array->d_lower[0] <= i1);
        register int c2 = (array->d_lower[1] <= i2);
        register int c3 = (array->d_lower[2] <= i3);
        register int c4 = (array->d_lower[3] <= i4);
        c1 = c1 && (array->d_upper[0] >= i1);
        c2 = c2 && (array->d_upper[1] >= i2);
        c3 = c3 && (array->d_upper[2] >= i3);
        c4 = c4 && (array->d_upper[3] >= i4);
        c1 = c1 && c2;
        c3 = c3 && c4;
        if (c1 && c3) {
          return COPY_VALUE(*(array->d_firstElement +
                              (((i1 - array->d_lower[0])*array->d_stride[0] +
                                (i2 - array->d_lower[1])*array->d_stride[1]) +
                               ((i3 - array->d_lower[2])*array->d_stride[2] +
                                (i4 - array->d_lower[3])*array->d_stride[3]))));
        }
      }
    }
  }
  return SIDL_float__array_zero;
}

/**
 * Retrieve element indices of an n-dimensional array. Indices is assumed
 * to have the right number of elements for the dimension of array.
 */
float
SIDL_float__array_get(const struct SIDL_float__array* array,
                      const int32_t indices[])
{
  if (array) {
    float *result = array->d_firstElement;
    int32_t i = 0;
    while (i < array->d_dimen) {
      if ((indices[i] < array->d_lower[i]) ||
          (indices[i] > array->d_upper[i]))
        return SIDL_float__array_zero;
      result += ((indices[i] - array->d_lower[i])*array->d_stride[i]);
      ++i;
    }
    return COPY_VALUE(*result);
  }
  return SIDL_float__array_zero;
}

/**
 * Set element i1 of a one-dimensional array to value.
 */
void
SIDL_float__array_set1(struct SIDL_float__array* array,
                       const int32_t i1,
                       const float value)
{
  if (array && (1 == array->d_dimen) &&
      ((array->d_lower[0] <= i1) && (array->d_upper[0] >= i1))) {
    DESTROY_VALUE(*(array->d_firstElement +
      (i1 - array->d_lower[0])*array->d_stride[0]));
    *(array->d_firstElement +
      (i1 - array->d_lower[0])*array->d_stride[0]) = COPY_VALUE(value);
  }
}

/**
 * Set element (i1,i2) of a two-dimensional array to value.
 */
void
SIDL_float__array_set2(struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2,
                       const float value)
{
  if (array && (2 == array->d_dimen)){
    /* unserialize array bounds checking (i.e. avoid strict left to right */
    /* evaluation of && which serializes evaluation) */
    register int c1 = (array->d_lower[0] <= i1);
    register int c2 = (array->d_upper[0] >= i1);
    register int c3 = (array->d_lower[1] <= i2);
    register int c4 = (array->d_upper[1] >= i2);
    c1 = c1 && c2;
    c3 = c3 && c4;
    if (c1 && c3) {
      DESTROY_VALUE(*(array->d_firstElement +
        (i1 - array->d_lower[0])*array->d_stride[0] +
        (i2 - array->d_lower[1])*array->d_stride[1]));
      *(array->d_firstElement +
        (i1 - array->d_lower[0])*array->d_stride[0] +
        (i2 - array->d_lower[1])*array->d_stride[1]) = COPY_VALUE(value);
    }
  }
}

/**
 * Set element (i1,i2,i3) of a three-dimensional array to value.
 */
void
SIDL_float__array_set3(struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2,
                       const int32_t i3,
                       const float value)
{
  if (array && (3 == array->d_dimen)){
    /* unserialize array bounds checking (i.e. avoid strict left to right */
    /* evaluation of && which serializes evaluation) */
    register int c1 = (array->d_lower[0] <= i1);
    register int c2 = (array->d_lower[1] <= i2);
    register int c3 = (array->d_lower[2] <= i3);
    c1 = c1 && (array->d_upper[0] >= i1);
    c2 = c2 && (array->d_upper[1] >= i2);
    c3 = c3 && (array->d_upper[2] >= i3);
    if (c1 && c2 && c3) {
      DESTROY_VALUE(*(array->d_firstElement +
        (i1 - array->d_lower[0])*array->d_stride[0] +
        (i2 - array->d_lower[1])*array->d_stride[1] +
        (i3 - array->d_lower[2])*array->d_stride[2]));
      *(array->d_firstElement +
        (i1 - array->d_lower[0])*array->d_stride[0] +
        (i2 - array->d_lower[1])*array->d_stride[1] +
        (i3 - array->d_lower[2])*array->d_stride[2]) = COPY_VALUE(value);
    }
  }
}

/**
 * Set element (i1,i2,i3,i4) of a four-dimensional array to value.
 */
void
SIDL_float__array_set4(struct SIDL_float__array* array,
                       const int32_t i1,
                       const int32_t i2,
                       const int32_t i3,
                       const int32_t i4,
                       const float value)
{
  if (array) {
    switch (array->d_dimen) {
    case 1: SIDL_float__array_set1(array, i1, value); break;
    case 2: SIDL_float__array_set2(array, i1, i2, value); break;
    case 3: SIDL_float__array_set3(array, i1, i2, i3, value); break;
    case 4:
      {
        /* unserialize array bounds checking to. avoid strict left to right */
        /* evaluation of && which serializes evaluation */
        register int c1 = (array->d_lower[0] <= i1);
        register int c2 = (array->d_lower[1] <= i2);
        register int c3 = (array->d_lower[2] <= i3);
        register int c4 = (array->d_lower[3] <= i4);
        c1 = c1 && (array->d_upper[0] >= i1);
        c2 = c2 && (array->d_upper[1] >= i2);
        c3 = c3 && (array->d_upper[2] >= i3);
        c4 = c4 && (array->d_upper[3] >= i4);
        c1 = c1 && c2;
        c3 = c3 && c4;
        if (c1 && c3) {
          DESTROY_VALUE(*(array->d_firstElement +
            (((i1 - array->d_lower[0])*array->d_stride[0] +
              (i2 - array->d_lower[1])*array->d_stride[1]) +
             ((i3 - array->d_lower[2])*array->d_stride[2] +
              (i4 - array->d_lower[3])*array->d_stride[3]))));
          *(array->d_firstElement +
            (((i1 - array->d_lower[0])*array->d_stride[0] +
              (i2 - array->d_lower[1])*array->d_stride[1]) +
             ((i3 - array->d_lower[2])*array->d_stride[2] +
             (i4 - array->d_lower[3])*array->d_stride[3]))) =
             COPY_VALUE(value);
        }
        break;
      }
    }
  }
}

/**
 * Set element indices of an n-dimensional array to value. indices is assumed
 * to have the right number of elements for the dimension of array.
 */
void
SIDL_float__array_set(struct SIDL_float__array* array,
                      const int32_t indices[],
                      const float value)
{
  if (array) {
    float *result = array->d_firstElement;
    int32_t i = 0;
    while (i < array->d_dimen) {
      if ((indices[i] < array->d_lower[i]) ||
          (indices[i] > array->d_upper[i])) return;
      result += ((indices[i] - array->d_lower[i])*array->d_stride[i]);
      ++i;
    }
    DESTROY_VALUE(*result);
    *result = COPY_VALUE(value);
  }
}

/**
 * Return the dimension of array. If the array pointer is NULL,
 * zero is returned.
 */
int32_t
SIDL_float__array_dimen(const struct SIDL_float__array* array)
{
  return (array) ? (array->d_dimen) : 0;
}

/**
 * Return the lower index bound on dimension ind. If ind is not a valid
 * dimension, zero is returned. The valid range for ind is 0 to dimen-1.
 */
int32_t
SIDL_float__array_lower(const struct SIDL_float__array* array,
                        const int32_t ind)
{
  return (array && (ind >= 0) && (ind < array->d_dimen)) ?
    array->d_lower[ind] : 0;
}

/**
 * Return the upper index bound on dimension ind. If ind is not a valid
 * dimension, negative one is returned. The valid range for ind is 0 to
 * dimen-1.
 */
int32_t
SIDL_float__array_upper(const struct SIDL_float__array* array,
                        const int32_t ind)
{
  return (array && (ind >= 0) && (ind < array->d_dimen)) ?
    array->d_upper[ind] : -1;
}

/**
 * Return the stride of dimension ind. If ind is not a valid
 * dimension, zero is returned. The valid range for ind is 0 to
 * dimen-1.
 */
int32_t
SIDL_float__array_stride(const struct SIDL_float__array* array,
                         const int32_t ind)
{
  return (array && (ind >= 0) && (ind < array->d_dimen)) ?
    array->d_stride[ind] : -1;
}

/**
 * Return a true value iff the array is a contiguous column-major ordered
 * array.  A NULL array argument causes 0 to be returned.
 */
SIDL_bool
SIDL_float__array_isColumnOrder(const struct SIDL_float__array* array)
{
  if (!array) return FALSE;
  else {
    register int32_t i;
    register int32_t size;
    register const int32_t dimen = array->d_dimen;
    for(i = 0, size = 1; i < dimen ; ++i) {
      if (array->d_stride[i] != size) return FALSE;
      size *= (1 + array->d_upper[i] - array->d_lower[i]);
    }
    return TRUE;
  }
}

/**
 * Return a true value iff the array is a contiguous row-major ordered
 * array.  A NULL array argument causes 0 to be returned.
 */
SIDL_bool
SIDL_float__array_isRowOrder(const struct SIDL_float__array* array)
{
  if (!array) return FALSE;
  else {
    register int32_t i = array->d_dimen - 1;
    register int32_t size;
    for(size = 1; i >= 0 ; --i) {
      if (array->d_stride[i] != size) return FALSE;
      size *= (1 + array->d_upper[i] - array->d_lower[i]);
    }
    return TRUE;
  }
}

/**
 * Copy the contents of one array (src) to a second array (dest). For
 * the copy to take place, both arrays must exist and be of the same
 * dimension. This method will not modify dest's size, index bounds,
 * or stride; only the array element values of dest may be changed by
 * this function. No part of src is ever changed by copy.
 * 
 * On exit, dest[i][j][k]... = src[i][j][k]... for all indices i,j,k...
 * that are in both arrays. If dest and src have no indices in common,
 * nothing is copied. For example, if src is a 1-d array with elements
 * 0-5 and dest is a 1-d array with elements 2-3, this function will
 * make the following assignments:
 *   dest[2] = src[2],
 *   dest[3] = src[3].
 * The function copied the elements that both arrays have in common.
 * If dest had elements 4-10, this function will make the following
 * assignments:
 *   dest[4] = src[4],
 *   dest[5] = src[5].
 * 
 */
void
SIDL_float__array_copy(const struct SIDL_float__array* src,
                             struct SIDL_float__array* dest)
{
  if (src && dest && (src != dest) && (src->d_dimen == dest->d_dimen) &&
      src->d_dimen) {
    const int32_t dimen = src->d_dimen;
    int32_t * restrict overlap = (int32_t *)malloc(sizeof(int32_t)*dimen*4);
    if (overlap) {
      register float const * restrict srcFirst =
        src->d_firstElement;
      register float * restrict destFirst =
        dest->d_firstElement;
      register int32_t i;
      int32_t * restrict current = overlap + dimen;
      int32_t * restrict src_stride = current + dimen;
      int32_t * restrict dst_stride = src_stride + dimen;
      int32_t lower, upper, fastMover = dimen - 1, moverRank = 0;
      for(i = 0; i < dimen; ++i){
        lower = MAX(src->d_lower[i],dest->d_lower[i]);
        upper = MIN(src->d_upper[i],dest->d_upper[i]);
        overlap[i] = 1 + upper - lower;
        if (overlap[i] <= 0) goto cleanup;
        srcFirst += (src->d_stride[i]*(lower - src->d_lower[i]));
        destFirst += (dest->d_stride[i]*(lower - dest->d_lower[i]));
        current[i] = 0;
        src_stride[i] = src->d_stride[i];
        dst_stride[i] = dest->d_stride[i];
        if (((src_stride[i] == 1) || (src_stride[i] == -1) ||
             (dst_stride[i] == 1) || (dst_stride[i] == -1)) &&
            (overlap[i] >= moverRank)) {
          moverRank = overlap[i];
          fastMover = i;
        }
      }
      if (fastMover != dimen - 1) { /* move stride 1 to inner loop */
        /* this *might* improve cache performance */
        swap_i32(&overlap[fastMover], &overlap[dimen-1]);
        swap_i32(&src_stride[fastMover], &src_stride[dimen-1]);
        swap_i32(&dst_stride[fastMover], &dst_stride[dimen-1]);
      }
      switch(dimen) {
      case 1: /* handle a common special case */
        {
          const int32_t bound = overlap[0];
          const int32_t dstride = dst_stride[0];
          const int32_t sstride = src_stride[0];
          for(i = 0; i < bound; ++i) {
            DESTROY_VALUE(*destFirst);
            *destFirst = COPY_VALUE(*srcFirst);
            destFirst += dstride;
            srcFirst += sstride;
          }
        }
        break;
      case 2: /* handle a common special case (2-d arrays) */
        {
          const int32_t bound0 = overlap[0];
          const int32_t bound1 = overlap[1];
          const int32_t dstride1 = dst_stride[1];
          const int32_t sstride1 = src_stride[1];
          const int32_t dstride0 = dst_stride[0] - bound1*dstride1;
          const int32_t sstride0 = src_stride[0] - bound1*sstride1;
          int32_t j;
          for(i = 0; i < bound0; ++i) {
            for(j = 0; j < bound1; ++j) {
              DESTROY_VALUE(*destFirst);
              *destFirst = COPY_VALUE(*srcFirst);
              destFirst += dstride1;
              srcFirst += sstride1;
            }
            destFirst += dstride0;
            srcFirst += sstride0;
          }
        }
        break;
      case 3: /* handle a common special case (3-d arrays) */
        {
          const int32_t bound0 = overlap[0];
          const int32_t bound1 = overlap[1];
          const int32_t bound2 = overlap[2];
          const int32_t dstride2 = dst_stride[2];
          const int32_t sstride2 = src_stride[2];
          const int32_t dstride1 = dst_stride[1] - bound2*dstride2;
          const int32_t sstride1 = src_stride[1] - bound2*sstride2;
          const int32_t dstride0 = dst_stride[0] - bound1*dstride1;
          const int32_t sstride0 = src_stride[0] - bound1*sstride1;
          
          int32_t j, k;
          for(i = 0; i < bound0; ++i) {
            for(j = 0; j < bound1; ++j) {
              for(k = 0; k < bound2; ++k) {
                DESTROY_VALUE(*destFirst);
                *destFirst = COPY_VALUE(*srcFirst);
                destFirst += dstride2;
                srcFirst += sstride2;
              }
              destFirst += dstride1;
              srcFirst += sstride1;
            }
            destFirst += dstride0;
            srcFirst += sstride0;
          }
        }
        break;
      default: /* handle the general case */
        do {
          DESTROY_VALUE(*destFirst);
          *destFirst = COPY_VALUE(*srcFirst);
          for(i = dimen - 1; i >= 0; --i) {
            ++(current[i]);
            if (current[i] >= overlap[i]) {
              current[i] = 0;
              destFirst -= ((overlap[i]-1) * dst_stride[i]);
              srcFirst -= ((overlap[i]-1) * src_stride[i]);
            }
            else {
              destFirst += dst_stride[i];
              srcFirst += src_stride[i];
              break; /* exit for loop */
            }
          }
        } while (i >= 0);
        break;
      }
cleanup:
      free(overlap);
    }
  }
}

/**
 * If necessary, convert a general matrix into a matrix with the required
 * properties. This checks the dimension and ordering of the matrix.
 * If both these match, it simply returns a new reference to the existing
 * matrix. If the dimension of the incoming array doesn't match, it
 * returns NULL. If the ordering of the incoming array doesn't match the
 * specification, a new array is created with the desired ordering and
 * the content of the incoming array is copied to the new array.
 * 
 * The ordering parameter should be one of the constants defined in
 * enum SIDL_array_ordering (e.g. SIDL_general_order,
 * SIDL_column_major_order, or SIDL_row_major_order). If you
 * specify SIDL_general_order, this routine will only check the
 * dimension because any matrix is SIDL_general_order.
 * 
 * The caller assumes ownership of the returned reference unless it's NULL.
 * 
 */
struct SIDL_float__array*
SIDL_float__array_ensure(struct SIDL_float__array* src,
                         int32_t                dimen,
                         int                    ordering)
{
  struct SIDL_float__array* result = NULL;
  if (src && (src->d_dimen == dimen)) {
    switch(ordering) {
    case SIDL_column_major_order:
      if (SIDL_float__array_isColumnOrder(src)) {
        SIDL_float__array_addRef(src);
        result = src;
      }
      else {
        result = SIDL_float__array_createCol
          (dimen, src->d_lower, src->d_upper);
        SIDL_float__array_copy(src, result);
      }
      break;
    case SIDL_row_major_order:
      if (SIDL_float__array_isRowOrder(src)) {
        SIDL_float__array_addRef(src);
        result = src;
      }
      else {
        result = SIDL_float__array_createRow
          (dimen, src->d_lower, src->d_upper);
        SIDL_float__array_copy(src, result);
      }
      break;
    default:
      SIDL_float__array_addRef(src);
      result = src;
      break;
    }
  }
  return result;
}

/**
 * Return a pointer to the first element of the data space of the array.
 * Using the lower and stride information, you can access elements of the
 * array without using a function call. NULL is returned iff array is NULL.
 * 
 * Direct access should only be used when efficiency requires it.
 */
float *
SIDL_float__array_first(const struct SIDL_float__array* array)
{
  return array ? array->d_firstElement : NULL;
}

