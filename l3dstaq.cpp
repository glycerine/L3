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

#include "dstaq.h"
#include "l3dstaq.h"

/////////////////////// end of includes


l3obj* dstaqobj::ith(long i, dstaq<lnk>::ll** phit) {
    lnk* tmp = _symlinks.ith(i,phit);
    if (tmp && tmp->target()) {
         return tmp->chase();
     }
     return gna;
}

#if 0
// current no way to delete by or value / key, only by position/order

l3obj* dstaqobj::remove_key(char* key) {


}

l3obj* dstaqobj::remove_val(l3obj* val) {
    if (_symlinks.del_val(val)) {
        return val;
    }
    return gna;
}
#endif


void dstaqobj::push_back(l3obj* val, char* key) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_back(link);
}


void dstaqobj::push_front(l3obj* val, char* key) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_front(link);
}

l3obj*  dstaqobj::del_ith(long i) {

   dstaq<lnk>::ll* hit = 0;
   l3obj* ohit = ith(i,&hit);

   assert(hit);

   BOOL lastcopy = FALSE;
   
   lnk* deleteme = _symlinks.del(hit,&lastcopy);
   assert(deleteme->chase() == ohit);

   _symlinks._mytag->lnk_remove(deleteme);
   del_lnk(deleteme);
   
   // might be gone now, so cannot do this:   return ohit;
   return gtrue;
}


l3obj* dstaqobj::pop_back() {
    lnk* link = _symlinks.pop_back();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* dstaqobj::pop_front() {
    lnk* link = _symlinks.pop_front();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* dstaqobj::back() {
    lnk* link = _symlinks.back_val();
    return link->chase();
}

l3obj* dstaqobj::front() {
    lnk* link = _symlinks.front_val();
    return link->chase();
}

void  dstaqobj::clear() {
    while(size()) {
        pop_front();
    }
    _symlinks.clear();
}



// dtor
dstaqobj::~dstaqobj() {

}

// copy ctor
dstaqobj::dstaqobj(const dstaqobj& src) 
    : _symlinks(src._symlinks)
{
    // dstaq copy constructor for _symlinks works.
}

dstaqobj& dstaqobj::operator=(const dstaqobj& src) {
    if (&src == this) return *this;

    if (src.size()) {
        _symlinks.push_back_all_from(src._symlinks);
    }

    return *this;
}


// ctor
dstaqobj::dstaqobj(Tag* tag) 
: _symlinks(tag) 
{  

}


// erase everything
L3KARG(dq_clear,1)
{

    l3obj* dqobj = 0;
    ptrvec_get(vv,0,&dqobj);

   if (0==dqobj || dqobj->_type != t_dsq) {
       printf("error: dq_clear called on non t_dsq object, in expression '%s'.\n",
              sexps());
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       l3throw(XABORT_TO_TOPLEVEL);
   }


    assert(dqobj->_type == t_dsq);

    dstaqobj* dq = (dstaqobj*)(dqobj->_pdb);

    dq->clear();

    if (vv) {  generic_delete(vv, L3STDARGS_OBJONLY);  }
}
L3END(dq_clear)


L3METHOD(make_new_dq)
{
   l3obj* nobj = 0;
   l3path clsname("dq");
   l3path objname("dq");
   long extra = sizeof(dstaqobj);
   make_new_captag((l3obj*)&objname, extra,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown,ifp);

   nobj->_type = t_dsq;
   nobj->_parent_env  = env;

   dstaqobj* dq = (dstaqobj*)(nobj->_pdb);
   dq = new(dq) dstaqobj(nobj->_mytag); // placement new, so we can compose objects

   nobj->_dtor = &dq_dtor;
   nobj->_cpctor = &dq_cpctor;
  
   *retval = nobj;
}
L3END(make_new_dq)


// l3obj wrapper for dstaq

// equivalent of make_new_dq
L3METHOD(dq)
{
   arity = num_children(exp);
   l3path sexps(exp);
   l3obj* vv  = 0;
   if (arity>0) {
       // screens for unresolved references, gets values up front.
       any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
   }

   long N = 0;
   if (vv) {
       N = ptrvec_size(vv);
   }


   // make the dstaq
   make_new_dq(L3STDARGS);

   l3obj* nobj = *retval;

   //
   // and pushback the contents requested by the initialization
   //
   l3obj* ele = 0;
   //sexp_t* nex = exp->list->next->next;
   for (long i = 0; i < N; ++i) {
        ptrvec_get(vv,i,&ele);

        // gotta do this *before* the dq_pushback, or else the
        //  llref / target cleanup will delete from the dq. not what we want.
        //
        // if vv is the owner, then transfer ownership to the dstaq,
        // so that command line entered values are retained.
        //

        if (ele->_owner == vv->_mytag) {
            ele->_owner->generic_release_to(ele, nobj->_mytag);
        } else {
            // and check for captags too...
            if (ele->_owner->_parent == vv->_mytag) {
                ele->_owner->generic_release_to(ele, nobj->_mytag);
            }
        }
        

        dq_pushback_api(nobj,ele,0);
    }

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(dq)


L3METHOD(dq_dtor)
{
    assert(obj->_type == t_dsq);

    dstaqobj* dq = (dstaqobj*)(obj->_pdb);
   
    dq->~dstaqobj();
}
L3END(dq_dtor)


//
// vv = ptrvec to push onto the back of dqobj
//
void dq_pushback_api(l3obj* dqobj, l3obj* val, l3obj* optional_name) {
    LIVEO(dqobj);
    LIVEO(val);

    l3path key;
    if (optional_name) {
        string_get(optional_name,0,&key);
    }

    assert(dqobj->_type == t_dsq);

    dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

    dq->push_back(val,key());

}

void dq_pushfront_api(l3obj* dqobj, l3obj* val, l3obj* optional_name) {
    LIVEO(dqobj);
    LIVEO(val);

    l3path key;
    if (optional_name) {
        string_get(optional_name,0,&key);
    }

    assert(dqobj->_type == t_dsq);

    dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

    dq->push_front(val,key());

}



L3METHOD(dq_cpctor)
{

    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    Tag* dq_tag = nobj->_mytag;

    // assert we are a captag pair
    assert( dq_tag );
    assert( pred_is_captag_obj(nobj) );
    
    dstaqobj* dq_src = (dstaqobj*)(src->_pdb);
    dstaqobj* dq    = (dstaqobj*)(nobj->_pdb);

    // use the underlying dstaq copy constructor
    if (dq_src->size()) {

        // assignment operator invocation.
        dq = dq_src;
    }

    // *retval was already set for us, by the base copy ctor.
}
L3END(dq_cpctor)

// for both  dq_del_ith() and dq_ith()
long dq_helper_verify_integer_index(l3obj* dqobj, l3obj* vv, l3path& sexps, Tag* owner, Tag* retown) {
    FILE* ifp =0;

   if (0==dqobj || dqobj->_type != t_dsq) {
       printf("error: dq_ith called on non t_dsq object, in expression '%s'.\n",
              sexps());
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* index = 0;

   ptrvec_get(vv,1,&index);
   if (0==index || index->_type != t_dou || double_size(index) < 1) {
       printf("error: dq_ith called with bad index value, non-numeric, in expression '%s'.\n",
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   double dindex = double_get(index,0);

   long   as_integer = (long)dindex;
   bool   is_integer = (dindex == (double)as_integer);

   if (!is_integer) {
       printf("error: dq_ith called with bad index value, non-integer index '%.6f', in expression '%s'.\n",
              dindex,
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   return as_integer;
}

L3KARG(dq_del_ith,2)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);

   long as_integer = dq_helper_verify_integer_index(dqobj, vv, sexps,owner,retown);

   dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

   // don't mod-wrap deletion indices, to catch errors.
   long sz = dq->size();
   long abs_index = labs(as_integer);
   if (abs_index >= sz) {
       printf("error in dq_del_ith: index %ld was out of range; size of dq is %ld.\n", as_integer, sz);
       l3throw(XABORT_TO_TOPLEVEL);
       generic_delete(vv, L3STDARGS_OBJONLY);   
   }

   *retval = dq->del_ith(as_integer);

   generic_delete(vv, L3STDARGS_OBJONLY);   
   
}
L3END(dq_del_ith)


L3KARG(dq_ith,2)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);

   long as_integer = dq_helper_verify_integer_index(dqobj, vv, sexps,owner,retown);

   dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

   *retval = dq->ith(as_integer,0);

   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dq_ith)


L3METHOD(dq_size)
   return dq_len(L3STDARGS);
L3END(dq_size)

L3KARG(dq_len,1)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);

   if (0==dqobj || dqobj->_type != t_dsq) {
       printf("error: dq_len called on non t_dsq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

    long   sz = dq->size();
    l3obj* len = make_new_double_obj((double)sz, retown,"len");

    generic_delete(vv, L3STDARGS_OBJONLY);
    *retval = len;

}
L3END(dq_len)


// stubs not yet implemented...

L3METHOD(dq_find_name)
{
    assert(0);
    *retval = gna;
}
L3END(dq_find_name)


L3METHOD(dq_find_val)
{
    assert(0);
    *retval = gna;
}
L3END(dq_find_val)



L3METHOD(dq_erase_name)
{
    assert(0);
    *retval = gtrue;
}
L3END(dq_erase_name)


L3METHOD(dq_erase_val)
{
    assert(0);
    *retval = gtrue;
}
L3END(dq_erase_val)




// 
// add to end of the stack, with optional name.
//
// (pushback dqobj val "name")
//
L3METHOD(dq_pushback)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushback: "<< exp->val() << " requires two or three arguments: ";
        printf("(pushdback dqobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
               sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* dq = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&dq);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( dq->_type != t_dsq) {
       printf("error: pushback called on non-dq object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   if(name) {
       if (name->_type != t_str && name->_type != t_lit) {
           printf("error: dq_pushback called with name that is neither string nor literal, in expression '%s'\n",sexps());
           generic_delete(vv, L3STDARGS_OBJONLY);
           l3throw(XABORT_TO_TOPLEVEL);
       }
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag) {
       val->_owner->generic_release_to(val, dq->_mytag);
   } else {
       // and check for captags too...
       if (val->_owner->_parent == vv->_mytag) {
           val->_owner->generic_release_to(val, dq->_mytag);
       }
   }
   

   dq_pushback_api(dq, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dq_pushback)


L3KARG(dq_back,1)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);

   if (0==dqobj || dqobj->_type != t_dsq) {
       printf("error: dq_back called on non t_dsq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

    *retval = dq->back();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(dq_back)




L3KARG(dq_popback,1)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);
   dstaqobj* dq = 0;
   //l3obj* re = 0;

  XTRY
    case XCODE:
           if (0==dqobj || dqobj->_type != t_dsq) {
               printf("error: dq_popback called on non t_dsq object, in expression '%s'.\n",
                      sexps()
                      );
               l3throw(XABORT_TO_TOPLEVEL);
           }

           dq    = (dstaqobj*)(dqobj->_pdb);
           assert(dq);
           if (dq->size() == 0) {
               printf("error: dq_popback called on size zero dstaq; in expression '%s'.\n", sexps());
               l3throw(XABORT_TO_TOPLEVEL);
           }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.
           //           re = dq->back();
           //           *retval = make_new_link(re, "dq_popback_value", retown, env);
           // the above was problematic. make the user do back() if they want to save back element.

           dq->pop_back();

           *retval = gtrue;
       break;
    case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(dq_popback)




// 
// add to end of the stack, with optional name.
//
// (pushfront dqobj val "name")
//
L3METHOD(dq_pushfront)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushfront: " << exp->val() << " requires two or three arguments: ";
        printf( "(dq_pushfront dqobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
                sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* dq = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&dq);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( dq->_type != t_dsq) {
       printf("error: pushfront called on non-dq object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag) {
       val->_owner->generic_release_to(val, dq->_mytag);
   } else {
       // and check for captags too...
       if (val->_owner->_parent == vv->_mytag) {
           val->_owner->generic_release_to(val, dq->_mytag);
       }
   }
   

   dq_pushfront_api(dq, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dq_pushfront)


L3KARG(dq_front,1)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);

   if (0==dqobj || dqobj->_type != t_dsq) {
       printf("error: dq_front called on non t_dsq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    dstaqobj* dq    = (dstaqobj*)(dqobj->_pdb);

    *retval = dq->front();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(dq_front)




L3KARG(dq_popfront,1)
{
   l3obj* dqobj = 0;
   ptrvec_get(vv,0,&dqobj);
   dstaqobj* dq    = 0;   
   //l3obj* re = 0;

 XTRY
   case XCODE:
       if (0==dqobj || dqobj->_type != t_dsq) {
           printf("error: dq_popfront called on non t_dsq object, in expression '%s'.\n",
                  sexps()
                  );
           l3throw(XABORT_TO_TOPLEVEL);
       }

       dq    = (dstaqobj*)(dqobj->_pdb);
       assert(dq);
       if (dq->size() == 0) {
           printf("error: dq_popfront called on size zero dstaq; in expression '%s'.\n", sexps());
           l3throw(XABORT_TO_TOPLEVEL);
       }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.

       // no, problematic. make the user grab the element with a front() by itself it
       // they want it.
       //           re = dq->front();
       //           *retval = make_new_link(re, "dq_popfront_value", retown, env);

           dq->pop_front();

           *retval = gtrue;

       break;
 case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(dq_popfront)



long dq_len_api(l3obj* dqobj) {
    assert(dqobj);
    assert(dqobj->_type == t_dsq);
    
    dstaqobj* dq = (dstaqobj*)(dqobj->_pdb);

    return dq->size();
}



l3obj* dq_ith_api(l3obj* dqobj, long i, l3path* key) {
    assert(dqobj);
    assert(dqobj->_type == t_dsq);

    dstaqobj* dq = (dstaqobj*)(dqobj->_pdb);

    dstaq<lnk>::ll* hit = 0;
    l3obj* ret = dq->ith(i,&hit);

    if (hit && key) {
        key->reinit(hit->_ptr->name());
    }

    return ret;
}




void dq_print(l3obj* l3dq, const char* indent, stopset* stoppers) {
     assert(l3dq);
  
     l3path indent_more(indent);
     indent_more.pushf("%s","     ");

    dstaqobj* dq = (dstaqobj*)(l3dq->_pdb);
    l3path details;
    if (gUglyDetails>0) {
        details.pushf("%s%p : ",l3dq);
    }

    long N = dq->size();
    printf("%s%s (dq of size %ld): \n", indent, details(), N);
  
     dq->dump(indent_more(),stoppers);

}
