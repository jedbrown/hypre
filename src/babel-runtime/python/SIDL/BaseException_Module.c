/*
 * File:          BaseException_Module.c
 * Symbol:        SIDL.BaseException-v0.8.2
 * Symbol Type:   class
 * Babel Version: 0.8.4
 * Release:       $Name: V1-9-0b $
 * Revision:      @(#) $Id: BaseException_Module.c,v 1.4 2003/04/07 21:44:28 painter Exp $
 * Description:   implement a C extension type for a SIDL extendable
 * 
 * Copyright (c) 2000-2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by the Components Team <components@llnl.gov>
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
 * 
 * WARNING: Automatically generated; changes will be lost
 * 
 * babel-version = 0.8.4
 */

/*
 * THIS CODE IS AUTOMATICALLY GENERATED BY THE BABEL
 * COMPILER. DO NOT EDIT THIS!
 * 
 * This file contains the implementation of a Python C
 * extension type (i.e. a Python type implemented in C).
 * This extension type provides Python interface to the
 * SIDL type SIDL.BaseException.
 */


/**
 * Symbol "SIDL.BaseException" (version 0.8.2)
 * 
 * Every exception inherits from <code>BaseException</code>.  This class
 * provides basic functionality to get and set error messages and stack
 * traces.
 */
#define SIDL_BaseException_INTERNAL 1
#include "SIDL_BaseException_Module.h"
#ifndef included_SIDL_BaseException_IOR_h
#include "SIDL_BaseException_IOR.h"
#endif
#include "SIDLObjA.h"
#include "SIDLPyArrays.h"
#include "Numeric/arrayobject.h"
#ifndef included_SIDL_Loader_h
#include "SIDL_Loader.h"
#endif
#ifndef included_SIDL_header_h
#include "SIDL_header.h"
#endif
#ifndef included_SIDL_interface_IOR_h
#include "SIDL_interface_IOR.h"
#endif
#include "SIDL_BaseInterface_Module.h"
#include "SIDL_BaseClass_Module.h"
#include "SIDL_ClassInfo_Module.h"
#include <stdlib.h>
#include <string.h>

static const struct SIDL_BaseException__external *_implEPV = NULL;

static PyObject *
pStub_BaseException_addRef(PyObject *_self, PyObject *_args,                  \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    static char *_kwlist[] = {
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "", _kwlist);
    if (_okay) {
      (*(_self_ior->d_epv->f_addRef))(_self_ior);
      _return_value = Py_None;
      Py_INCREF(_return_value);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_deleteRef(PyObject *_self, PyObject *_args,               \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    static char *_kwlist[] = {
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "", _kwlist);
    if (_okay) {
      (*(_self_ior->d_epv->f_deleteRef))(_self_ior);
      _return_value = Py_None;
      Py_INCREF(_return_value);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_isSame(PyObject *_self, PyObject *_args,                  \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    struct SIDL_BaseInterface__object* iobj = NULL;
    static char *_kwlist[] = {
      "iobj",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "O&", _kwlist,
      (void *)SIDL_BaseInterface__convert, &iobj);
    if (_okay) {
      SIDL_bool _return = (SIDL_bool) 0;
      int _proxy__return;
      _return = (*(_self_ior->d_epv->f_isSame))(_self_ior, iobj);
      _proxy__return = _return;
      _return_value = Py_BuildValue(
        "i",
        _proxy__return);
      SIDL_BaseInterface_deref(iobj);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_queryInt(PyObject *_self, PyObject *_args,                \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    char* name = NULL;
    static char *_kwlist[] = {
      "name",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "z", _kwlist,
      &name);
    if (_okay) {
      struct SIDL_BaseInterface__object* _return = NULL;
      _return = (*(_self_ior->d_epv->f_queryInt))(_self_ior, name);
      _return_value = Py_BuildValue(
        "O&",
        (void *)SIDL_BaseInterface__wrap, _return);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_isType(PyObject *_self, PyObject *_args,                  \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    char* name = NULL;
    static char *_kwlist[] = {
      "name",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "z", _kwlist,
      &name);
    if (_okay) {
      SIDL_bool _return = (SIDL_bool) 0;
      int _proxy__return;
      _return = (*(_self_ior->d_epv->f_isType))(_self_ior, name);
      _proxy__return = _return;
      _return_value = Py_BuildValue(
        "i",
        _proxy__return);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_getClassInfo(PyObject *_self, PyObject *_args,            \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    static char *_kwlist[] = {
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "", _kwlist);
    if (_okay) {
      struct SIDL_ClassInfo__object* _return = NULL;
      _return = (*(_self_ior->d_epv->f_getClassInfo))(_self_ior);
      _return_value = Py_BuildValue(
        "O&",
        (void *)SIDL_ClassInfo__wrap, _return);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_getNote(PyObject *_self, PyObject *_args,                 \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    static char *_kwlist[] = {
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "", _kwlist);
    if (_okay) {
      char* _return = NULL;
      _return = (*(_self_ior->d_epv->f_getNote))(_self_ior);
      _return_value = Py_BuildValue(
        "z",
        _return);
      free((void *)_return);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_setNote(PyObject *_self, PyObject *_args,                 \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    char* message = NULL;
    static char *_kwlist[] = {
      "message",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "z", _kwlist,
      &message);
    if (_okay) {
      (*(_self_ior->d_epv->f_setNote))(_self_ior, message);
      _return_value = Py_None;
      Py_INCREF(_return_value);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_getTrace(PyObject *_self, PyObject *_args,                \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    static char *_kwlist[] = {
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "", _kwlist);
    if (_okay) {
      char* _return = NULL;
      _return = (*(_self_ior->d_epv->f_getTrace))(_self_ior);
      _return_value = Py_BuildValue(
        "z",
        _return);
      free((void *)_return);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_addLine(PyObject *_self, PyObject *_args,                 \
  PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    char* traceline = NULL;
    static char *_kwlist[] = {
      "traceline",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "z", _kwlist,
      &traceline);
    if (_okay) {
      (*(_self_ior->d_epv->f_addLine))(_self_ior, traceline);
      _return_value = Py_None;
      Py_INCREF(_return_value);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
pStub_BaseException_add(PyObject *_self, PyObject *_args, PyObject *_kwdict) {
  PyObject *_return_value = NULL;
  struct SIDL_BaseException__object *_self_ior =
    ((struct SIDL_BaseException__object *)
     SIDL_Cast(_self, "SIDL.BaseException"));
  if (_self_ior) {
    char* filename = NULL;
    int32_t lineno = (int32_t) 0;
    char* methodname = NULL;
    long _proxy_lineno;
    static char *_kwlist[] = {
      "filename",
      "lineno",
      "methodname",
      NULL
    };
    const int _okay = PyArg_ParseTupleAndKeywords(
      _args, _kwdict, 
      "zlz", _kwlist,
      &filename,
      &_proxy_lineno,
      &methodname);
    if (_okay) {
      lineno = _proxy_lineno;
      (*(_self_ior->d_epv->f_add))(_self_ior, filename, lineno, methodname);
      _return_value = Py_None;
      Py_INCREF(_return_value);
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError, 
      "self pointer is not a SIDL.BaseException");
  }
  return _return_value;
}

static PyObject *
_createCast(PyObject *self, PyObject *args) {
  struct SIDL_BaseException__object *optarg = NULL;
  int _okay = PyArg_ParseTuple(args, "|O&",                                   \
    (void *)SIDL_BaseException__convert, &optarg);
  if (_okay) {
    if (optarg) {
      (*(optarg->d_epv->f_addRef))(optarg);
    }
    else {
      optarg = (*(_implEPV->createObject))();
    }
    return SIDL_BaseException__wrap(optarg);
  }
  return NULL;
}

static PyMethodDef _BaseExceptionModuleMethods[] = {
  { "BaseException", _createCast, METH_VARARGS, "Constructor and/or cast method for SIDL.BaseException"
   },
  { NULL, NULL }
};

static PyMethodDef _BaseExceptionObjectMethods[] = {
  { "add", (PyCFunction)pStub_BaseException_add,
  (METH_VARARGS | METH_KEYWORDS),
"\
add(in string filename,\n\
    in int lineno,\n\
    in string methodname)\n\
RETURNS\n\
    None\n\
\n\
\
Formats and adds an entry to the stack trace based on the \n\
file name, line number, and method name."
   },
  { "addLine", (PyCFunction)pStub_BaseException_addLine,
  (METH_VARARGS | METH_KEYWORDS),
"\
addLine(in string traceline)\n\
RETURNS\n\
    None\n\
\n\
\
Adds a stringified entry/line to the stack trace."
   },
  { "addRef", (PyCFunction)pStub_BaseException_addRef,
  (METH_VARARGS | METH_KEYWORDS),
"\
addRef()\n\
RETURNS\n\
    None\n\
\n\
\
<p>\n\
Add one to the intrinsic reference count in the underlying object.\n\
Object in <code>SIDL</code> have an intrinsic reference count.\n\
Objects continue to exist as long as the reference count is\n\
positive. Clients should call this method whenever they\n\
create another ongoing reference to an object or interface.\n\
</p>\n\
<p>\n\
This does not have a return value because there is no language\n\
independent type that can refer to an interface or a\n\
class.\n\
</p>"
   },
  { "deleteRef", (PyCFunction)pStub_BaseException_deleteRef,
  (METH_VARARGS | METH_KEYWORDS),
"\
deleteRef()\n\
RETURNS\n\
    None\n\
\n\
\
Decrease by one the intrinsic reference count in the underlying\n\
object, and delete the object if the reference is non-positive.\n\
Objects in <code>SIDL</code> have an intrinsic reference count.\n\
Clients should call this method whenever they remove a\n\
reference to an object or interface."
   },
  { "getClassInfo", (PyCFunction)pStub_BaseException_getClassInfo,
  (METH_VARARGS | METH_KEYWORDS),
"\
getClassInfo()\n\
RETURNS\n\
   (SIDL.ClassInfo _return)\n\
\n\
\
Return the meta-data about the class implementing this interface."
   },
  { "getNote", (PyCFunction)pStub_BaseException_getNote,
  (METH_VARARGS | METH_KEYWORDS),
"\
getNote()\n\
RETURNS\n\
   (string _return)\n\
\n\
\
Return the message associated with the exception."
   },
  { "getTrace", (PyCFunction)pStub_BaseException_getTrace,
  (METH_VARARGS | METH_KEYWORDS),
"\
getTrace()\n\
RETURNS\n\
   (string _return)\n\
\n\
\
Returns formatted string containing the concatenation of all \n\
tracelines."
   },
  { "isSame", (PyCFunction)pStub_BaseException_isSame,
  (METH_VARARGS | METH_KEYWORDS),
"\
isSame(in SIDL.BaseInterface iobj)\n\
RETURNS\n\
   (bool _return)\n\
\n\
\
Return true if and only if <code>obj</code> refers to the same\n\
object as this object."
   },
  { "isType", (PyCFunction)pStub_BaseException_isType,
  (METH_VARARGS | METH_KEYWORDS),
"\
isType(in string name)\n\
RETURNS\n\
   (bool _return)\n\
\n\
\
Return whether this object is an instance of the specified type.\n\
The string name must be the <code>SIDL</code> type name.  This\n\
routine will return <code>true</code> if and only if a cast to\n\
the string type name would succeed."
   },
  { "queryInt", (PyCFunction)pStub_BaseException_queryInt,
  (METH_VARARGS | METH_KEYWORDS),
"\
queryInt(in string name)\n\
RETURNS\n\
   (SIDL.BaseInterface _return)\n\
\n\
\
Check whether the object can support the specified interface or\n\
class.  If the <code>SIDL</code> type name in <code>name</code>\n\
is supported, then a reference to that object is returned with the\n\
reference count incremented.  The callee will be responsible for\n\
calling <code>deleteRef</code> on the returned object.  If\n\
the specified type is not supported, then a null reference is\n\
returned."
   },
  { "setNote", (PyCFunction)pStub_BaseException_setNote,
  (METH_VARARGS | METH_KEYWORDS),
"\
setNote(in string message)\n\
RETURNS\n\
    None\n\
\n\
\
Set the message associated with the exception."
   },
  { NULL, NULL }
};

SIDL_BaseException__wrap_RETURN
SIDL_BaseException__wrap SIDL_BaseException__wrap_PROTO {
  return SIDL_Object_Create((struct SIDL_BaseClass__object *) sidlobj,        \
    _BaseExceptionObjectMethods, SIDL_PyStealRef);
}

SIDL_BaseException__weakRef_RETURN
SIDL_BaseException__weakRef SIDL_BaseException__weakRef_PROTO {
  return SIDL_Object_Create((struct SIDL_BaseClass__object *) sidlobj,        \
    _BaseExceptionObjectMethods, SIDL_PyWeakRef);
}

SIDL_BaseException_deref_RETURN
SIDL_BaseException_deref SIDL_BaseException_deref_PROTO {
  if (sidlobj) {
    (*(sidlobj->d_epv->f_deleteRef))(sidlobj);
  }
}

SIDL_BaseException__newRef_RETURN
SIDL_BaseException__newRef SIDL_BaseException__newRef_PROTO {
  return SIDL_Object_Create((struct SIDL_BaseClass__object *) sidlobj,        \
    _BaseExceptionObjectMethods, SIDL_PyNewRef);
}

SIDL_BaseException__addRef_RETURN
SIDL_BaseException__addRef SIDL_BaseException__addRef_PROTO {
  if (sidlobj) {
    (*(sidlobj->d_epv->f_addRef))(sidlobj);
  }
}

SIDL_BaseException__convert_RETURN
SIDL_BaseException__convert SIDL_BaseException__convert_PROTO {
  *sidlobj = SIDL_Cast(obj, "SIDL.BaseException");
  if (*sidlobj) {
    (*((*sidlobj)->d_epv->f_addRef))(*sidlobj);
  }
  else if (obj != Py_None) {
    PyErr_SetString(PyExc_TypeError, 
      "argument is not a(n) SIDL.BaseException");
    return 0;
  }
  return 1;
}

static int
_convertPython(void *sidlarray, const int *ind, PyObject *pyobj)
{
  struct SIDL_BaseException__object *sidlobj;
  if (SIDL_BaseException__convert(pyobj, &sidlobj)) {
    SIDL_interface__array_set((struct SIDL_interface__array *)sidlarray,
    ind, (struct SIDL_BaseInterface__object *)sidlobj);
    return FALSE;
  }
  return TRUE;
}

SIDL_BaseException__convert_python_array_RETURN
SIDL_BaseException__convert_python_array                                      \
  SIDL_BaseException__convert_python_array_PROTO {
  int result = 0;
  PyObject *pya = PyArray_FromObject(obj, PyArray_OBJECT, 0, 0);
  *sidlarray = NULL;
  if (pya) {
    if (PyArray_OBJECT == ((PyArrayObject *)pya)->descr->type_num) {
      int dimen, *lower, *upper, *stride;
      if (SIDL_array__extract_python_info
        (pya, &dimen, &lower, &upper, &stride))
      {
          *sidlarray = (struct                                                \
            SIDL_BaseException__array*)SIDL_interface__array_createRow
          (dimen, lower, upper);
        free(stride);
        free(upper);
        free(lower);
        result = SIDL_array__convert_python
          (pya, dimen, *sidlarray, _convertPython);
        if (*sidlarray && !result) {
          SIDL_interface__array_deleteRef(
            (struct  SIDL_interface__array *)*sidlarray);
          *sidlarray = NULL;
        }
      }
    }
    Py_DECREF(pya);
  }
  return result;
}

static int
_convertSIDL(void *sidlarray, const int *ind, PyObject **dest)
{
  struct SIDL_BaseException__object *sidlobj = (struct                        \
    SIDL_BaseException__object*)
  SIDL_interface__array_get((struct SIDL_interface__array *)
    sidlarray, ind);
  *dest = SIDL_BaseException__wrap(sidlobj);
  return (*dest == NULL);
}

SIDL_BaseException__convert_sidl_array_RETURN
  SIDL_BaseException__convert_sidl_array                                      \
    SIDL_BaseException__convert_sidl_array_PROTO {PyObject *pya = NULL;
  if (sidlarray) {
    const int dimen = SIDL_interface__array_dimen(
      (struct SIDL_interface__array *)sidlarray);
    int i;
    int *lower = (int *)malloc(sizeof(int) * dimen);
    int *upper = (int *)malloc(sizeof(int) * dimen);
    int *numelem = (int *)malloc(sizeof(int) * dimen);
    for(i = 0; i < dimen; ++i) {
      lower[i] = SIDL_interface__array_lower(
        (struct SIDL_interface__array *)sidlarray, i);
      upper[i] = SIDL_interface__array_upper(
        (struct SIDL_interface__array *)sidlarray, i);
      numelem[i] = 1 + upper[i] - lower[i];
    }
    pya = PyArray_FromDims(dimen, numelem, PyArray_OBJECT);
    if (pya) {
      if (!SIDL_array__convert_sidl(pya, dimen, lower, upper,
        numelem, sidlarray, _convertSIDL))
      {
        Py_DECREF(pya);
        pya = NULL;
      }
    }
    free(numelem);
    free(upper);
    free(lower);
  }
  return pya;
}

SIDL_BaseException__destroy_sidl_array_RETURN
SIDL_BaseException__destroy_sidl_array                                        \
  SIDL_BaseException__destroy_sidl_array_PROTO {
  if (sidlarray) {
    SIDL_interface__array_deleteRef(
      (struct SIDL_interface__array *)sidlarray);
  }
}
void
initBaseException(void) {
  PyObject *module, *dict, *c_api;
  static void *ExternalAPI[SIDL_BaseException__API_NUM];
  module = Py_InitModule3("BaseException", _BaseExceptionModuleMethods, "\
\
Every exception inherits from <code>BaseException</code>.  This class\n\
provides basic functionality to get and set error messages and stack\n\
traces."
  );
  _implEPV = SIDL_BaseException__externals();
  if (!_implEPV) {
    Py_FatalError("Cannot load implementation for SIDL class                  \
      SIDL.BaseException");
  }
  dict = PyModule_GetDict(module);
  ExternalAPI[SIDL_BaseException__wrap_NUM] = (void*)SIDL_BaseException__wrap;
  ExternalAPI[SIDL_BaseException__convert_NUM] =                              \
    (void*)SIDL_BaseException__convert;
  ExternalAPI[SIDL_BaseException__convert_python_array_NUM] =                 \
    (void*)SIDL_BaseException__convert_python_array;
  ExternalAPI[SIDL_BaseException__convert_sidl_array_NUM] =                   \
    (void*)SIDL_BaseException__convert_sidl_array;
  ExternalAPI[SIDL_BaseException__destroy_sidl_array_NUM] =                   \
    (void*)SIDL_BaseException__destroy_sidl_array;
  ExternalAPI[SIDL_BaseException__weakRef_NUM] =                              \
    (void*)SIDL_BaseException__weakRef;
  ExternalAPI[SIDL_BaseException_deref_NUM] = (void*)SIDL_BaseException_deref;
  ExternalAPI[SIDL_BaseException__newRef_NUM] =                               \
    (void*)SIDL_BaseException__newRef;
  ExternalAPI[SIDL_BaseException__addRef_NUM] =                               \
    (void*)SIDL_BaseException__addRef;
  import_SIDLObjA();
  if (PyErr_Occurred()) {
    Py_FatalError("Error importing SIDLObjA module.");
  }
  SIDL_BaseException__type =                                                  \
    PyErr_NewException("SIDL.BaseException.Exception", SIDL_PyType(), NULL);
  PyDict_SetItemString(dict, "Exception", SIDL_BaseException__type);
  ExternalAPI[SIDL_BaseException__type_NUM] = SIDL_BaseException__type;
  c_api = PyCObject_FromVoidPtr((void *)ExternalAPI, NULL);
  PyDict_SetItemString(dict, "_C_API", c_api);
  Py_XDECREF(c_api);
  if (PyErr_Occurred()) {
    Py_FatalError("Cannot initialize Python module SIDL.BaseException.");
  }
  import_SIDLPyArrays();
  if (PyErr_Occurred()) {
    Py_FatalError("Error importing SIDLPyArrays module.");
  }
  import_array();
  if (PyErr_Occurred()) {
    Py_FatalError("Error importing Numeric Python module.");
  }
  SIDL_BaseInterface__import();
  SIDL_BaseClass__import();
  SIDL_ClassInfo__import();
}
