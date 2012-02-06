//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#include "jtdd.h"
#include "dynamicscope.h"
#include "symvec.h"
#include "l3obj.h"

/////////////////////// end of includes

// dynamicscope.cpp :  dynamic scoping lookups
//
// we can re-use  resolve_core()     which takes 'curenv' (first param), in which to look for 'nextonpath'.
// we can re-use  resolve_dotted_id  which takes startingenv (2nd param) to check for id (1st param).
//
// we only need a dynamic version  resolve_dotted_up_the_parent_env_chain().
//

// Defn: statically resolve symbol names    => chase env->_parent_env pointers
// Defn: Dynamically resolve symbols names  => chase up the global_env_stack
//

// here is the static resolving function, it lives in l3obj.cpp:3956
// l3obj* resolve_dotted_up_the_parent_env_chain(qqchar id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack);


// here is the dynamic version:

l3obj* resolve_up_dynamic_env_stack(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                    const qqchar&  curcmd, noresolve_action noresolve_do) {

   l3obj* cur = startingenv;
   void* found = 0;

   if (startingenv) {

       found = resolve_dotted_id(id, cur, deref,innermostref,penvstack, curcmd, noresolve_do);
       
       if (found) {
           return (l3obj*)found;
       }
   }

   DV(printf("trying up the global_env_stack...\n"));


#if 0

   long Nenv = global_env_stack.size();
   long ienv = 0;

   EnvStack_it  be(&global_env_stack._stk);
   for( ; !be.at_end(); ++be ) {

         cur = *be;
         
         // now we allow zeroes. just skip them. old:    assert(cur); // envs on stack should all be good
         if (cur) {  

             DV(std::cout << "trying to resolve '"<< id << "' in the cur env:\n");
             DV(print(cur,"   ",0));

             found = resolve_dotted_id(id, cur, deref,innermostref,penvstack, curcmd, noresolve_do);

             if (found) {
                 return (l3obj*)found;
             }
         }

         // tried up one, note that on penvstack...
         if (penvstack) { penvstack->push_back(0); } // 0..0..obj => ../../obj

         // avoid runaways...
         ++ienv;
         assert(ienv <= Nenv);

   } // end for loop

#else

             DV(std::cout << "trying to resolve '"<< id << "' in the cur env:\n");
             DV(print(cur,"   ",0));

             found = resolve_dotted_id(id, cur, deref,innermostref,penvstack, curcmd, noresolve_do);

             if (found) {
                 return (l3obj*)found;
             }

#endif

   DV(std::cout << "error: could not resolve reference to element '" << id << "'.\n");

   return 0;
 }


// and here is a version which tries static first, then dynamic.

l3obj* resolve_static_then_dynamic(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                   const qqchar&  curcmd, noresolve_action noresolve_do) {
    
    LIVEO(startingenv)
    // Throwing doesn't work because we also may want to leave literals, like "+", alone.
    // noresolve_action noresolve_do = UNFOUND_THROW_TOP;
    assert(noresolve_do == UNFOUND_RETURN_ZERO);

    l3obj* found = resolve_dotted_up_the_parent_env_chain(id,startingenv, deref,innermostref, penvstack, curcmd, noresolve_do);
    
    if(!found) {
        found = resolve_up_dynamic_env_stack(id,startingenv, deref,innermostref, penvstack, curcmd, noresolve_do);

        if (!found) {
            if (noresolve_do == UNFOUND_THROW_TOP) {
                std::cout << "error: variable name resolution failed resolve_static_then_dynamic(): could not resolve '"<< id << "'.\n";
                l3throw(XABORT_TO_TOPLEVEL);
            }
            return 0;
        }
    }

    return found;
}




l3obj* resolve_dynamic_then_static(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                   const qqchar&  curcmd, noresolve_action noresolve_do) {


    // Throwing doesn't work because we also may want to leave literals, like "+", alone.
    // noresolve_action noresolve_do = UNFOUND_THROW_TOP;
    assert(noresolve_do == UNFOUND_RETURN_ZERO);

    l3obj* found = resolve_up_dynamic_env_stack(id,startingenv, deref,innermostref, penvstack, curcmd, noresolve_do);
    
    if(!found) {
        found = resolve_dotted_up_the_parent_env_chain(id,startingenv, deref,innermostref, penvstack, curcmd, noresolve_do);

        if (!found) {
            if (noresolve_do == UNFOUND_THROW_TOP) {
                std::cout << "error: variable name resolution failed resolve_static_then_dynamic(): could not resolve '"<<id<<"'.\n";
                l3throw(XABORT_TO_TOPLEVEL);
            }
            return 0;
        }
    }

    return found;
}

