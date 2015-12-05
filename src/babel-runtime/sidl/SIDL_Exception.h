/*
 * File:        SIDL_Exception.h
 * Copyright:   (c) 2001 The Regents of the University of California
 * Release:     $Name: V1-9-0b $
 * Revision:    @(#) $Revision: 1.4 $
 * Date:        $Date: 2003/04/07 21:44:31 $
 * Description: convenience C macros for managing SIDL exceptions
 *
 * These macros help to manage SIDL exceptions in C.  The caller is
 * respondible for the following:
 *
 * 1) consistently checking for exceptions after each function call that
 *    may throw an exception
 * 2) checking for return arguments using either SIDL_CHECK or SIDL_CATCH
 * 3) clearing handled exceptions with SIDL_CLEAR
 * 4) if using SIDL_CHECK, creating an EXIT label with the associated
 *    clean-up code
 * Copyright (c) 2000-2001, The Regents of the University of Calfornia.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by the Components Team <components@llnl.gov>
 * UCRL-CODE-2002-054
 * All rights reserved.
 * 
 * This file is part of Babel. For more information, see
 * http://www.llnl.gov/CASC/components/. Please read the COPYRIGHT file
 * for Our Notice and the LICENSE file for the GNU Lesser General Public
 * License.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License (as published by
 * the Free Software Foundation) version 2.1 dated February 1999.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
 * conditions of the GNU Lesser General Public License for more details.
 * 
 * You should have recieved a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef included_SIDL_Exception_h
#define included_SIDL_Exception_h

#ifndef included_SIDL_BaseException_h
#include "SIDL_BaseException.h"
#endif

#ifndef NULL
#define NULL 0
#endif

/*
 * Define __FUNC__ to be "unknown" so that we do not force users to define
 * __FUNC__ before their functions.
 */

#ifndef __FUNC__
#define __FUNC__ "unknown"
#endif

/**
 * SIDL helper macro that throws an exception.  This macro will create an
 * exception class of the specified type, assign it to the exception variable
 * name, set the message and traceback information, and then jump to the
 * user-defined EXIT block.  If the exception variable is not NULL, then
 * no new exception is thrown.
 *
 * EXAMPLE:
 * void myfunction(..., SIDL_BaseException *_ex)
 * {
 *   ...
 *   SIDL_THROW(*_ex, MyPackage_MyException_Class, "oops");
 *   ...
 *   return;
 *
 *   EXIT:;
 *     / * clean up and return with exception set in _ex * /
 *     return;
 * }
 *
 * WARNINGS:
 * Do not use this within an EXIT block!
 */
#define SIDL_THROW(EX_VAR,EX_CLS,MSG) {                                    \
  if (EX_VAR == NULL) {                                                    \
    EX_VAR = (SIDL_BaseException) EX_CLS##__create();                      \
    if (EX_VAR != NULL) {                                                  \
      SIDL_BaseException_setNote(EX_VAR, MSG);                          \
      SIDL_BaseException_add(EX_VAR, __FILE__, __LINE__, __FUNC__); \
    }                                                                      \
  }                                                                        \
  goto EXIT;                                                               \
} 

/**
 * SIDL helper macro that checks the status of an exception.  If the exception
 * is not set, then this macro does nothing.  If the exception is set, then
 * a stack trace line is added to the exception and control jumps to the user
 * defined EXIT block for exception processing.
 *
 * Suggested usage: This macro should be placed at the end of the line of
 * each function call.  By doing so, the line entered into the stack trace
 * is more accurate and the code more readable.
 *
 * EXAMPLE:
 * void myfunction(..., SIDL_BaseException *_ex)
 * {
 *   ...
 *   foo(..., _ex); SIDL_CHECK(*_ex);
 *   ...
 *   EXIT:;
 *     / * clean up and return with exception set in _ex * /
 * }
 *
 * WARNINGS:  
 * Do not use this within an EXIT block!
 */
#define SIDL_CHECK(EX_VAR) {                                             \
  if (EX_VAR != NULL) {                                                  \
    SIDL_BaseException_add(EX_VAR, __FILE__, __LINE__, __FUNC__); \
    goto EXIT;                                                           \
  }                                                                      \
} 

/**
 * SIDL helper macro that clears the exception state.  Nothing is done if
 * if the exception was not set.  If the exception was set, then it deallocates
 * the exception class and sets the variable to NULL.
 *
 * EXAMPLE:
 * void myfunction(..., SIDL_BaseException *_ex)
 * {
 *   ...
 *   foo(..., _ex); SIDL_CHECK(*_ex);
 *   ...
 *   EXIT:;
 *     / * erase the exception and handle the error somehow * /
 *     SIDL_CLEAR(*_ex); /
 * }
 */
#define SIDL_CLEAR(EX_VAR) {                    \
  if (EX_VAR != NULL) {                         \
    SIDL_BaseException_deleteRef(EX_VAR);       \
    EX_VAR = NULL;                              \
  }                                             \
}

/**
 * SIDL helper macro that checks whether the exception has been set and is
 * of the specified type.  This macro should be used similar to Java catch
 * statements to catch the exception and process it.  This macro simply tests
 * whether the exception exists and whether it matches the specified type; it
 * does not clear or process the exception.
 *
 * EXAMPLE:
 * void myfunction(..., SIDL_BaseException *_ex)
 * {
 *   ...
 *   foo(..., _ex);
 *   if (SIDL_CATCH(*_ex, "MyPackage.MyException")) {
 *     / * process exception and then clear it * /
 *     SIDL_CLEAR(*_ex);
 *   } else if (SIDL_CATCH(*_ex, "YourPackage.YourException") {
 *     / * process exception and then clear it * /
 *     SIDL_CLEAR(*_ex);
 *   }
 *   / * jump to exit block if we cannot handle exception * /
 *   SIDL_CHECK(*_ex);
 *   ...
 *   EXIT:;
 *     ...
 * }
 */
#define SIDL_CATCH(EX_VAR,SIDL_NAME) \
  ((EX_VAR != NULL) && SIDL_BaseException_isType(EX_VAR, SIDL_NAME))

#endif
