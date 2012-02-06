//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3_DYNAMIC_SCOPE_H
#define L3_DYNAMIC_SCOPE_H

#include "autotag.h"
#include "l3obj.h"
#include "terp.h"


//////////////////////////////////////////
//
// RESOLVE_REF : macro for easy switching of resolve style
//
//////////////////////////////////////////

#define RESOLVE_REF(id,startenv,deref,innermostref,penvstack,curcmd,noresolve_do) \
    resolve_static_then_dynamic(id,startenv,deref,innermostref,penvstack,curcmd,noresolve_do)


//    resolve_dynamic_then_static(id,startenv,deref,innermostref,penvstack,curcmd,noresolve_do)


/////////////////////////////////////////
//
// dynamic scoping routines: dynamicscope_* ; they use the global_env_stack, created at runtime.
//
/////////////////////////////////////////

// what do to if we can't resolve...
// typedef enum { UNFOUND_RETURN_ZERO=0, UNFOUND_THROW_TOP=1 } noresolve_action;


l3obj* resolve_up_dynamic_env_stack(const qqchar& id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                    const qqchar& curcmd, noresolve_action noresolve_do);


//
// this should probably become the default variable resolution routine: first check static env, then check dynamic env.
//
l3obj* resolve_static_then_dynamic(const qqchar& id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                   const qqchar& curcmd, noresolve_action noresolve_do);

//
// no, actually, *this* should probably become default: dynamic env first (so it can override)
//
l3obj* resolve_dynamic_then_static(const qqchar& id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                   const qqchar& curcmd, noresolve_action noresolve_do);


// 
// new arg set for resolve core, that allows us to unify all the resolve calls
// 
l3obj* resolve_core(char* nextonpath, l3obj* curenv, deref_syms deref, llref** innermostref,  objlist_t* penvstack, 
                    char* curcmd, noresolve_action noresolve_do);


l3obj* resolve_dotted_id(const qqchar& id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
                         const qqchar& curcmd, noresolve_action noresolve_do);


#endif // L3_DYNAMIC_SCOPE_H

