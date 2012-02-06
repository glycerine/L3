//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
 #define _ISOC99_SOURCE 1 // for NAN
 #include <math.h>
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



 #include "objects.h"
 #include "symvec.h"
 #include <openssl/sha.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>

// (c-set-style "whitesmith")

#ifndef _MACOSX

 #define HAVE_DECL_BASENAME 1
 #include <demangle.h>

#endif

 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"

#include "serialfac.h"
#include "terp.h" // for eval()
#include "judydup.h"
#include "tostring.h"
#include "loops.h"
#include "l3pratt.h"
#include "qexp.h"

/////////////////////// end of includes

// (foreach coll i c (so "the " i "-th item is: " c ", a.k.a. :" (aref coll i))) ; auto-bind i and c to a new env
L3METHOD(foreach)
   arity = num_children(exp);
   if (arity != 4) {
      printf("error in (foreach collection index curobj body): foreach did not have arity 4\n");
      l3throw(XABORT_TO_TOPLEVEL);
   }
   // can't do k_arg_op(), because we don't want to evaluable the body yet
   //   k_arg_op(L3STDARGS);

   l3path index_name(ith_child(exp,1)->val());
   if (num_children(ith_child(exp,1)) > 0 || index_name.grep("(") >= 0) {
      printf("error in (foreach collection index curobj body): index variable must be a simple variable name to be created.\n");
      l3throw(XABORT_TO_TOPLEVEL);
   }

   l3path curobj_name(ith_child(exp,2)->val());
   if (num_children(ith_child(exp,2)) > 0 || index_name.grep("(") >= 0) {
      printf("error in (foreach collection index curobj body): curobj variable must be a simple variable name to be created.\n");
      l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* index = make_new_double_obj(0,owner,index_name());;
   l3obj* curobj = 0;

   // make a symbol whose object pointer gets updated each time through the loop
   make_new_symvec(0,-1,0,env, &curobj,owner,0,0,owner,ifp);
   symbol* psym = symvec_set(curobj, 0, curobj_name(), 0); // initially have to have symbol pointing to 0, since we don't konw where else yet.

   // get the array to run across.
   l3obj* arr = 0;
   sexp_t* arr_sexp = ith_child(exp,1);
   eval(0,-1,arr_sexp,env, &arr, owner,curfo,etyp,owner,ifp);

   // the body of the for loop is what we evaluate each time through.
   sexp_t* body_sexp = ith_child(exp,3);
   l3path  body_sexp_as_string(body_sexp);
DV(printf("body: %s\n",body_sexp_as_string()));

   long   lenavail = 0;

   // for iterating over double vectors, create a tmp double object of size 1
   // whose 0th element will hold the ith element of the original vector.
   l3obj*  tmpcurobj = make_new_double_obj(NAN,owner,"foreach_tmpcurobj");

   // check that we know how to loop over the type of arr
   if (arr->_type == t_vvc) {
       lenavail = ptrvec_size(arr);
   } else if (arr->_type == t_dou) {
       lenavail = double_size(arr);
   } else if (arr->_type == t_syv) {
       lenavail = symvec_size(arr);
   } else {
       printf("error in foreach: we don't currently implement looping over %s objects: only double or pointer arrays at the moment.\n",arr->_type);
       l3throw(XABORT_TO_TOPLEVEL);
   }

     // how many elements: put it in variable $
     l3obj* lenobj = make_new_double_obj((double)lenavail,owner,"foreach_lenobj");;

     // always create the index, curobj, and $ (length of array) afresh, like C++ loop semantics.

     // execute the body inside an env that has index and curobj bound appropriately
     l3obj* new_foreach_env = make_new_class(0,owner,"foreach_env","foreach_env");
     new_foreach_env->_type = t_env;

     l3path foreach_env_tag_name("foreach_env_tag");
     new_foreach_env->_mytag = new Tag(STD_STRING_WHERE.c_str(), owner, foreach_env_tag_name(), new_foreach_env);
     new_foreach_env->_parent_env = env;

     //     insert(&index_name, index, new_foreach_env);
     add_alias_eno(new_foreach_env, index_name(), index);
     //     insert(&curobj_name, curobj, new_foreach_env);
     add_alias_eno(new_foreach_env, curobj_name(), curobj);

     l3path dollar("$");
     //     insert(&dollar,lenobj, new_foreach_env);
     add_alias_eno(new_foreach_env, dollar(), lenobj);

     // do the loop, updating the index and curobj each pass through, then
     // evaluating the body...

     // we can use the _judyL array JLF (first) and JLN (next) functions...
     Word_t * PValue;                    // pointer to array element value
     Word_t Index = 0;
     long i = 0;
     l3obj* curcur = 0;
     l3obj* curretval = 0;
     XTRY
      case XCODE:

       // using JLF and JLN lets us handle sparse arrays naturally 
       JLF(PValue, arr->_judyL, Index);
       while (PValue != NULL)
       {
           assert(i < lenavail);

             // get index
             double_set(index,0,(double)Index);

             // get curcur
             if (arr->_type == t_vvc) {
               curcur = (l3obj*)(*PValue);

             } else if (arr->_type == t_dou) {
                 double dcur = *((double*)(PValue));
                 double_set(tmpcurobj,0,dcur);
                 curcur = tmpcurobj;

             } else if (arr->_type == t_syv) {
               curcur = (l3obj*)(*PValue);

             } else assert(0); // should have eliminated all other cases above.

             // the critical piece to making curobj work is updating the symbol's _obj pointer:
             psym->_obj = curcur; // updates what curobj actually points to.

             eval(0,-1,body_sexp, new_foreach_env, &curretval, new_foreach_env->_mytag,0,0,new_foreach_env->_mytag,ifp);

             DV(printf("in foreach pass %ld < %ld: our *retval is:\n",i,lenavail);
                print(curretval,"foreach debug:  ",0);
             );

           JLN(PValue, arr->_judyL, Index);
           i++;
       } // end while PValue != NULL

         break;

      case XFINALLY:
         if (new_foreach_env) {
             generic_delete(new_foreach_env,-1,0,L3STDARGS_ENV); 
         }
         break;
     XENDX
  


L3END(foreach)


L3METHOD(c_for_loop)
{
   LIVEO(exp);
   DV(printf("c_for_loop called.\n");
      exp->print("c_for_loop called with exp:  "));

   l3path sexps(exp);
   arity = num_children(exp);

   if (arity == 0) {
      printf("error: for loop with no test found, aborting. Expression was: '%s'\n",sexps());
      l3throw(XABORT_TO_TOPLEVEL);      
   }


   if (exp->_body == 0 || exp->_body->nchild() == 0) {
      printf("warning: empty for loop body...doing nothing in this loop: '%s'.\n",sexps());
      return 0;
   }


   // the body of the for loop is what we evaluate each time through.
   qtree* body_sexp = exp->_body;
   l3path  body_sexp_as_string(body_sexp);
   DV(printf("body: %s\n",body_sexp_as_string()));


   qtree* initializer_exp = 0;
   qtree* test_done_exp   = 0;
   qtree* advance_exp     = 0;

   if (arity == 1) {
      // just the test
      test_done_exp = exp->ith_child(0);

   } else if (arity == 2) {
      // test and advance
      test_done_exp = exp->ith_child(0);
      advance_exp   = exp->ith_child(1);

   } else if (arity == 3) {
      initializer_exp = exp->ith_child(0);
      test_done_exp   = exp->ith_child(1);
      advance_exp     = exp->ith_child(2);

   }

     // do the loop, updating the index and curobj each pass through, then
     // evaluating the body...

     l3obj* init_res = 0;
     l3obj* test_res = 0;
     volatile bool done = false;

     XTRY
      case XCODE:
          if (initializer_exp) {
             eval(obj,initializer_exp->nchild(),initializer_exp,   env,&init_res,owner,curfo,  etyp,owner,ifp);
          }
          break;
     XENDX


     while(!done) {

     XTRY
       case XCODE:
             eval(obj,test_done_exp->nchild(),test_done_exp,   env,&test_res,owner,curfo,  etyp,owner,ifp);
             if (!test_res) {
                 done = true;
                 break;
             }
             if (!is_true(test_res,0)) {
                 done = true;
                 break;
             }

            // body each time through the "for" loop.
            do_progn(obj,body_sexp->nchild(),body_sexp,  L3STDARGS_ENV);

            // no break

      case XCONTINUE_LOOP:
          DV(printf("state in c_for_loop is XCONTINUE_FOR_LOOP...  doing advance\n"));
          XHandled();

            // advance
            if (advance_exp) {
                  eval(obj,advance_exp->nchild(),advance_exp,   env,&test_res,owner,curfo,  etyp,owner,ifp);
            }
            // XData.State = XCode;
          break;

      case XBREAK_NEAREST_LOOP:
           XHandled();
           done = true;
           break;

      case XFINALLY:
          DV(printf("state in c_for_loop is XFINALLY\n"));
          // not here...because this is hit at the bottom of every loop. So Dont do: done = true;
         break;
     XENDX

   } // end while(!done)  


}
L3END(c_for_loop)


L3METHOD(c_continue)
{
   printf("c_continue called... throwing XCONTINUE_LOOP.\n");
   DV(exp->print("c_continue called with exp:  "));

   l3throw(XCONTINUE_LOOP);

}
L3END(c_continue)


L3METHOD(c_break)
{
   printf("c_break called... throwing XBREAK_NEAREST_LOOP.\n");
   DV(exp->print("c_break called with exp:  "));

   l3throw(XBREAK_NEAREST_LOOP);
}
L3END(c_break)



L3METHOD(while1)
   XTRY
      case XCODE:
         while(1) {
             eval(0,-1,exp->first_child(),L3STDARGS_ENV);
             DV(printf("in while1: our *retval is:\n");
                print(*retval,"debug in while1:  ",0);
                );
         }
         break;

      case XBREAK_NEAREST_LOOP:
         XHandled();
         break;

      case XFINALLY:

         break;
   XENDX
L3END(while1)

L3METHOD(break_)
   l3throw(XBREAK_NEAREST_LOOP);
L3END(break_)


L3METHOD(throw_)

L3END(throw_)



L3METHOD(catch_)

L3END(catch_)


L3METHOD(finally)

L3END(finally)


// add obj to the back of retval
L3METHOD(pushback)
   if (!obj) return 0;

   if (*retval == 0) {
      *retval = obj;
      return 0;
   }

   if ((*retval)->_type != obj->_type) {
      printf("error in pushback: type mismatch between %s trying to pushback onto %s.\n",obj->_type, (*retval)->_type );
      l3throw(XABORT_TO_TOPLEVEL);
   }

double dval = 0;

 if (obj->_type == t_dou) {
     long sz = double_size(*retval);
     dval = double_get(obj,0);
     double_set(*retval,sz,dval);
     return 0;
 }

      printf("error: pushback not implemented for type %s.\n",obj->_type);
      l3throw(XABORT_TO_TOPLEVEL);

L3END(pushback)



// loop methods


