//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

#include "l3obj.h"
#include "autotag.h"
#include "objects.h"
#include "terp.h"
#include "judydup.h"
#include "tostring.h"
#include "symvec.h"
#include "llref.h"
#include "addalias.h"
#include "dynamicscope.h"
#include "l3link.h"

#include "l3sexpobj.h"

/////////////////////// end of includes


// revive code previously in l3obj.cpp, to wrap each qtree* into
//  an editable l3obj* (so we can add annotation as needed).

// obj has the t_tdo tdopout_struct object to take the qtree/sexpression from.
// 
L3KARG(sexp,1)
{

    l3obj* tdo = 0;
    ptrvec_get(vv,0,&tdo);

    if (!tdo || tdo->_type != t_tdo) {
        printf("error: argument to sexp was not a parse output from a call to tdop, of type t_tdo, in expression '%s'.\n",
               sexps());        
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    assert(tdo->_type == t_tdo);
    tdopout_struct* tdop1 = (tdopout_struct*)(tdo->_pdb);


    l3path basenm("sexp");
    l3obj* nobj = 0;

    make_new_captag((l3obj*)&basenm ,0,  exp,env, &nobj, owner, 0,t_obj,retown,ifp);

    LIVEO(nobj);
    LIVET(nobj->_mytag);
    if (tdop1->_parsed) {
        nobj->_sexp = copy_sexp(tdop1->_parsed, nobj->_mytag);
        assert_sexp_all_owned_by_tag(nobj->_sexp, nobj->_mytag);

        // and set the _myobj link
        nobj->_sexp->_myobj = nobj;
    }

#if 0   // old, separate sexpobj objects returned, instead of generic t_obj that can have annotation added easily that we do now instead of:

    // sexp_obj now need to be captags so they can encapsulate all
    // the sexp_t and atoms that make up their held s-expressions, 
    // and thus move them all en-masse when ownership is transferred.

    // make a captag with an embedded sexpobj_struct at the end of the object.
    //  the tag will hold all the sexp stuff that we copy in.
    
    long extra = sizeof(sexpobj_struct);
    make_new_captag( (l3obj*) &basenm, extra, exp, env, &nobj, owner,  0,t_sxp,retown,ifp);

    Tag*   captag_tag = nobj->_mytag;
    
    assert(nobj->_type == t_sxp); // already set, right? if not we need to set it.

    // ===========================================
    // now set contents, in nobj, with captag_tag owning everything to keep.
    // ===========================================

    sexpobj_struct* sxs = (sexpobj_struct*)nobj->_pdb;
    sxs = new(sxs) sexpobj_struct(); // placement new, so that the vectors can get cleaned-up sanely.


    if (0==tdop1->_parsed) {
        sxs->_psx = 0;
    } else {

        sxs->_psx = copy_sexp(tdop1->_parsed, captag_tag); // deep copy; see sexp_ops.h
        assert_sexp_all_owned_by_tag(sxs->_psx, captag_tag);
    }
    
    // methods, dtor, cpctor
    nobj->_dtor   = &sexpobj_dtor;
    nobj->_cpctor = &sexpobj_cpctor;

    
    // diagnostics:
    DV(
       printf("diagnostics in make_new_sexp_obj, sxs->_psx is:\n");
       sxs->_psx->printspan(0,"");
       );


#endif

    *retval = nobj;
    if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }

}
L3END(sexp)


L3METHOD(sexpobj_cpctor)
{
    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    Tag* sxs_tag = nobj->_mytag;

    // assert we are a captag pair
    assert( sxs_tag );
    assert( pred_is_captag_obj(nobj) );
    
    sexpobj_struct* sxs_src = (sexpobj_struct*)(src->_pdb);
    sexpobj_struct* sxs    = (sexpobj_struct*)(nobj->_pdb);

    sxs->_psx = 0;

    if (sxs_src->_psx) {
        sxs->_psx = copy_sexp( sxs_src->_psx, sxs_tag );
    } else {
        sxs->_psx = 0;
    }

}  
L3END(sexpobj_cpctor)



long sexpobj_num_children(l3obj* obj) {
    assert(obj->_type == t_sxp);
    LIVEO(obj);

    sexpobj_struct* sxs = (sexpobj_struct*)obj->_pdb;

    if (!sxs->_psx) return 0;

    return sxs->_psx->nchild();
}



void sexpobj_text(l3obj* obj, l3path* out) {
    assert(obj->_type == t_sxp);
    assert(out);
    LIVEO(obj);

    sexpobj_struct* sxs = (sexpobj_struct*)obj->_pdb;

    if (sxs->_psx) {

        l3path print_psx(sxs->_psx);
        out->pushf("%s",print_psx());
    }
}



void sexpobj_get_sexp(l3obj* obj, qtree** out) {
    assert(obj->_type == t_sxp);
    LIVEO(obj);

    sexpobj_struct* sxs = (sexpobj_struct*)obj->_pdb;

    *out = sxs->_psx;
}

void sexpobj_set_sexp(l3obj* obj, qtree* insexp) {
    assert(obj->_type == t_sxp);
    LIVEO(obj);
    assert(insexp);

    sexpobj_struct* sxs = (sexpobj_struct*)obj->_pdb;

    if (sxs->_psx) {
        // check and reject self assignment
        if (insexp == sxs->_psx) return; // done

        // in case we are grabbing a sub-tree of our own _psx, i.e. overlap, copy the new insexp, before deleting the old.
        qtree* tmpnew = 0;
        
        // deep copy that we own.
        tmpnew = copy_sexp(insexp,obj->_mytag); 

        // delete the old stuff
        destroy_sexp(sxs->_psx);

        // put in the new
        sxs->_psx = tmpnew;
    }    
    else {
        sxs->_psx = copy_sexp(insexp,obj->_mytag); // deep copy
    }

}

L3METHOD(sexpobj_dtor)
{
    assert(obj->_type == t_sxp);

    sexpobj_struct* sxs = (sexpobj_struct*)obj->_pdb;
    assert(sxs);
    
    Tag* tg = obj->_owner;
    assert(tg);

    if (sxs->_psx) {
        assert_sexp_all_owned_by_tag(sxs->_psx, tg);
        destroy_sexp(sxs->_psx);
        sxs->_psx = 0;
    }

    sxs->~sexpobj_struct();
    sxs=0;

}
L3END(sexpobj_dtor)

