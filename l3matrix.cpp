//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
 #include "jtdd.h"
 #include "jlmap.h"
 #define _ISOC99_SOURCE 1 // for NAN
 #include <math.h>
 #include <valgrind/valgrind.h>
 #include <stdio.h>
 #include <limits.h>
 #include <climits>
 #include <time.h>
 #include <zmq.hpp>
 #include "autotag.h"
 #include "l3obj.h"
 #include "xcep.h"
 #include "merlin.pb.h"
 #include "qexp.h"
// #include <ffi.h>
 #include <cxxabi.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <string.h>
 #include <string>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include "rmkdir.h"
 #include <libgen.h>
 #include <assert.h>
 using std::string;
// #include "minifs.h"
//#include "slisp_util.h"

 #include "objects.h"
 // #include "pretty_print_sexp.h"
 #include "l3link.h"

 #include <openssl/sha.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>


// (c-set-style "whitesmith")


 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"

#include "serialfac.h"
#include "terp.h" // for eval(), extern gnan, etc.
#include "judydup.h"
#include "tostring.h"
#include "symvec.h"
#include "llref.h"
#include "l3dstaq.h"
#include "dynamicscope.h"
#include "l3ts.pb.h"
#include "l3mq.h"
#include "l3dd.h"
#include "l3string.h"
#include "l3matrix.h"


/////////////////////// end of includes




// return the value of JudyL[i] as a double
double double_get(l3obj* obj, long i) {
  LIVEO(obj);
  PWord_t   PValue = 0;
  double* pd = 0;
  JLG(PValue,obj->_judyL,i);
  if (!PValue) return NAN;
  pd = (double*) (PValue);
  double d = *pd;
  return d;
}

// set the value of JudyL[i] as a double
l3obj* double_set(l3obj* obj, long i, double d) {
    LIVEO(obj);
    double*   pd = 0;
    PWord_t   PValue = 0;
    JLI(PValue, obj->_judyL, i);

    pd = (double*)(PValue);
    *pd = d;
    return obj;
}


// get the number of doubles stored
long double_size(l3obj* obj) {
    //tmpoff    LIVEO(obj);
    Word_t    array_size;
    JLC(array_size, obj->_judyL, 0, -1);

    return (long)array_size;
}

// returns false if empty vector, else true
bool double_first(l3obj* obj, double* val, long* index) {
    assert(obj->_type == t_dou);
    LIVEO(obj);
    assert(index);
    assert(val);

    Word_t * PValue;                    // pointer to array element value
    Word_t Index = 0;
    
    JLF(PValue, obj->_judyL, Index);
    if (PValue) {
        *index = Index;
        *val = *(double*)(PValue);
        return true;
    }

    return false;
}

// return false if no more after the supplied *index value
//
bool double_next(l3obj* obj, double* val, long* index) {
    assert(obj->_type == t_dou);
    LIVEO(obj);
    
    Word_t * PValue = 0;
    Word_t Index = *index;
    
    JLN(PValue, obj->_judyL, Index);
    if (PValue) {
        *val = *((double*)(PValue));
        *index = Index;
        return true;
    }
    
    return false;
}

// simple linear search
bool double_search(l3obj* obj, double needle) {
       LIVEO(obj);
       if (isnan(needle)) return false;
   
       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
       double* dp = 0;

       JLF(PValue, obj->_judyL, Index);
       while (PValue != NULL)
       {
           dp = ((double*)(PValue));
           double d = *dp;
           if (feqs(d,needle,1e-6)) return true;
           JLN(PValue, obj->_judyL, Index);
       }

  return false;
}

void double_print(l3obj* obj, const char* indent) {
  LIVEO(obj);
  long N = double_size(obj);
  l3path details;

  if (gUglyDetails > 0) {
      details.pushf("(%p ser# %ld %s)", obj, obj->_ser, obj->_type);
  }

  printf("%s%s (double vector of size %ld): [ ", indent, details(), N);
  if (0==N) {
      printf("]\n");
  } else {
      double dval = 0;
      for (long i = 0; i < N; ++i) {
          dval = double_get(obj,i);
          printf("   %.6f", dval);
      }
      printf(" ]\n");
  }
}



void double_copy(l3obj* obj, Tag* owner, l3obj** retval) {
  assert(obj);
  assert(obj->_type == t_dou);
  LIVEO(obj);

  long N = double_size(obj);
  DV(printf("copying %p : (double vector of size %ld).\n", obj, N));
  
  l3path newnm(0,"copy_of_%s",obj->_varname);
  double d0 = NAN;

  if (N) {
    double_get(obj,0);
  }
  l3obj* copy = make_new_double_obj(d0, owner, newnm());

  if (N > 1) {
    assert(0); // not yet implemented, copying of judyL array
  }

  *retval = copy;
}



bool double_is_sparse(l3obj* obj) {
    assert(obj->_type == t_dou);
    return is_sparse_array(obj);
}

void double_set_sparse(l3obj* obj) {
    assert(obj->_type == t_dou);
    set_sparse_array(obj);
}

void double_set_dense(l3obj* obj) {
    assert(obj->_type == t_dou);
    set_notsparse_array(obj);
}


L3METHOD(double_dtor)
{
    //void* double_dtor(void* thisptr, void* expandptr, l3obj** retval) {
    int rc = 0;
    
    if (obj && obj->_judyL) {
        JLFA(rc, obj->_judyL);
    }
    
    matrixobj* mo = (matrixobj*)obj->_pdb;
    mo->~matrixobj();
}
L3END(double_dtor)


// if isnan(d) then return a size 0 vector.
l3obj* make_new_double_obj(double d, Tag* owner, const char* varname) {

    l3obj* obj = make_new_class(sizeof(matrixobj), owner,"double",varname);
    obj->_type = t_dou;
    obj->_dtor = &double_dtor;
    obj->_cpctor = &double_cpctor;

    matrixobj* mo = (matrixobj*)obj->_pdb;
    mo = new(mo) matrixobj(); // placement new

    if (!isnan(d)) {
        double_set(obj, 0, d);
        mo->_dim._dim[0]=1;
        mo->_dim._dim[0]=1;
    } else {
        mo->_dim._dim[0]=0;
        mo->_dim._dim[1]=0;
    }

  return obj;
}


L3METHOD(double_cpctor)
{
         l3obj* src  = obj;     // template
         l3obj* nobj = *retval; // write to this object
         assert(src);
         assert(nobj);

         // _judyS
         // can't just do this, the llref will dangle!!!: 
         // nobj->_judyS = src->_judyS;


         // _judyL
         copy_judyL(src->_judyL, &(nobj->_judyL));
}
L3END(double_cpctor)


// return linearloc after bounds checking
//
long  matrix_idx(l3obj* obj, Ix i, const char* funcname) {
    assert(obj);
    assert(obj->_type == t_dou);

    matrixobj* mo = (matrixobj*)obj->_pdb;

    long curdim = 0;
    long curidx = 0;
    long curoff = 0;

    long linearloc = 0;

    for (int j = 0; j < Ix::dimmax; ++j) {
        curidx = i._dim[j];

        // only bounds check the non-zero requests.
        if (curidx) {
            curdim = mo->_dim._dim[j];
            if (curidx >= curdim) {
                printf("error in %s: index %ld was out of bounds, a maximum of %ld is allowed by dimension %d.\n",
                       funcname, curidx, curdim, j);
                l3throw(XABORT_TO_TOPLEVEL);        
            }

            curoff = mo->_offset._dim[j];
            linearloc += curidx * curoff;
        }
    }

    return linearloc;
}

double  matrix_idx_get(l3obj* obj, Ix i) {
    long linearloc = matrix_idx(obj,i, "matrix_idx_get");
    return double_get(obj,linearloc);
}

void matrix_idx_set(l3obj* obj, Ix i, double d) {
    long linearloc = matrix_idx(obj,i, "matrix_idx_set");
    double_set(obj,linearloc,d);
}
