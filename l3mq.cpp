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

#include "mq.h"
#include "l3mq.h"

/////////////////////// end of includes


l3obj* mqueueobj::ith(long i, ddqueue::ll** phit) {
    lnk* tmp = _symlinks.ith(i,phit);
    if (tmp && tmp->target()) {
         return tmp->chase();
     }
     return gna;
}

#if 0
// current no way to delete by or value / key, only by position/order

l3obj* mqueueobj::remove_key(char* key) {


}

l3obj* mqueueobj::remove_val(l3obj* val) {
    if (_symlinks.del_val(val)) {
        return val;
    }
    return gna;
}
#endif


void mqueueobj::push_back(t_typ key, l3obj* val) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_back(link);
}


void mqueueobj::push_front(t_typ key, l3obj* val) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_front(link);
}

l3obj*  mqueueobj::del_ith(long i) {

   ddqueue::ll* hit = 0;
   l3obj* ohit = ith(i,&hit);

   assert(hit);

   lnk* deleteme = _symlinks.del(hit);
   assert(deleteme->chase() == ohit);

   _symlinks._mytag->lnk_remove(deleteme);
   del_lnk(deleteme);
   
   // might be gone now, so cannot do this:   return ohit;
   return gtrue;
}


l3obj* mqueueobj::pop_back() {
    lnk* link = _symlinks.pop_back();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* mqueueobj::pop_front() {
    lnk* link = _symlinks.pop_front();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* mqueueobj::back() {
    lnk* link = _symlinks.back_val();
    return link->chase();
}

l3obj* mqueueobj::front() {
    lnk* link = _symlinks.front_val();
    return link->chase();
}



// dtor
mqueueobj::~mqueueobj() {

}

// copy ctor
mqueueobj::mqueueobj(const mqueueobj& src) 
    : _symlinks(src._symlinks)
{
    // dstaq copy constructor for _symlinks works.
}

mqueueobj& mqueueobj::operator=(const mqueueobj& src) {
    if (&src == this) return *this;

    if (src.size()) {
        _symlinks.push_back_all_from(src._symlinks);
    }

    return *this;
}


// ctor
mqueueobj::mqueueobj(Tag* tag) 
: _symlinks(tag) 
{  

}


L3METHOD(make_new_mq)
{
   l3obj* nobj = 0;
   l3path clsname("mq");
   l3path objname("mq");
   long extra = sizeof(mqueueobj);
   make_new_captag((l3obj*)&objname, extra,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown,ifp);

   nobj->_type = t_mqq;
   nobj->_parent_env  = env;

   mqueueobj* mq = (mqueueobj*)(nobj->_pdb);
   mq = new(mq) mqueueobj(nobj->_mytag); // placement new, so we can compose objects

   nobj->_dtor = &mq_dtor;
   nobj->_cpctor = &mq_cpctor;
  
   *retval = nobj;
}
L3END(make_new_mq)


// l3obj wrapper for dstaq

// equivalent of make_new_mq
L3METHOD(mq)
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
   make_new_mq(L3STDARGS);

   l3obj* nobj = *retval;

   //
   // and pushback the contents requested by the initialization
   //
   l3obj* ele = 0;
   //sexp_t* nex = exp->list->next->next;
   for (long i = 0; i < N; ++i) {
        ptrvec_get(vv,i,&ele);

        // gotta do this *before* the mq_pushback, or else the
        //  llref / target cleanup will delete from the mq. not what we want.
        //
        // if vv is the owner, then transfer ownership to the dstaq,
        // so that command line entered values are retained.
        //

        if (ele->_owner == vv->_mytag || ele->_owner->_parent == vv->_mytag) {
            ele->_owner->generic_release_to(ele, nobj->_mytag);
        }

        mq_pushback_api(nobj,ele,0);
    }

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(mq)


L3METHOD(mq_dtor)
{
    assert(obj->_type == t_mqq);

    mqueueobj* mq = (mqueueobj*)(obj->_pdb);
   
    mq->~mqueueobj();
}
L3END(mq_dtor)


//
// vv = ptrvec to push onto the back of mqobj
//
void mq_pushback_api(l3obj* mqobj, l3obj* val, l3obj* keyobj) {
    LIVEO(mqobj);
    LIVEO(val);
    
    t_typ ty = 0;
    l3path key;
    if (keyobj) {
        string_get(keyobj,0,&key);
        
        // verify that it is a type
        ty = qtypesys->which_type(key(), 0);
        if (ty) {
            // okay

        } else {
            printf("error: mq_pushback key '%s' was not a recognized type.",key());
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    assert(mqobj->_type == t_mqq);

    mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

    mq->push_back(ty,val);

}

void mq_pushfront_api(l3obj* mqobj, l3obj* val, l3obj* keyobj) {
    LIVEO(mqobj);
    LIVEO(val);

    l3path key;
    if (keyobj) {
        string_get(keyobj,0,&key);
    }

    assert(mqobj->_type == t_mqq);

    mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

    mq->push_front(key(), val);

}



L3METHOD(mq_cpctor)
{

    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    Tag* mq_tag = nobj->_mytag;

    // assert we are a captag pair
    assert( mq_tag );
    assert( pred_is_captag_obj(nobj) );
    
    mqueueobj* mq_src = (mqueueobj*)(src->_pdb);
    mqueueobj* mq    = (mqueueobj*)(nobj->_pdb);

    // use the underlying dstaq copy constructor
    if (mq_src->size()) {

        // assignment operator invocation.
        mq = mq_src;
    }

    // *retval was already set for us, by the base copy ctor.
}
L3END(mq_cpctor)

// for both  mq_del_ith() and mq_ith()
long mq_helper_verify_integer_index(l3obj* mqobj, l3obj* vv, l3path& sexps, Tag* owner, Tag* retown) {
    FILE* ifp = 0;

   if (0==mqobj || mqobj->_type != t_mqq) {
       printf("error: mq_ith called on non t_mqq object, in expression '%s'.\n",
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* index = 0;

   ptrvec_get(vv,1,&index);
   if (0==index || index->_type != t_dou || double_size(index) < 1) {
       printf("error: mq_ith called with bad index value, non-numeric, in expression '%s'.\n",
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   double dindex = double_get(index,0);

   long   as_integer = (long)dindex;
   bool   is_integer = (dindex == (double)as_integer);

   if (!is_integer) {
       printf("error: mq_ith called with bad index value, non-integer index '%.6f', in expression '%s'.\n",
              dindex,
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   return as_integer;
}

L3KARG(mq_del_ith,2)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);

   long as_integer = mq_helper_verify_integer_index(mqobj, vv, sexps,owner,retown);

   mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

   // don't mod-wrap deletion indices, to catch errors.
   long sz = mq->size();
   long abs_index = labs(as_integer);
   if (abs_index >= sz) {
       printf("error in mq_del_ith: index %ld was out of range; size of mq is %ld.\n", as_integer, sz);
       l3throw(XABORT_TO_TOPLEVEL);
       generic_delete(vv, L3STDARGS_OBJONLY);   
   }

   *retval = mq->del_ith(as_integer);

   generic_delete(vv, L3STDARGS_OBJONLY);   
   
}
L3END(mq_del_ith)


L3KARG(mq_ith,2)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);

   long as_integer = mq_helper_verify_integer_index(mqobj, vv, sexps,owner,retown);

   mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

   *retval = mq->ith(as_integer,0);

   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(mq_ith)


L3METHOD(mq_size)
   return mq_len(L3STDARGS);
L3END(mq_size)

L3KARG(mq_len,1)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);

   if (0==mqobj || mqobj->_type != t_mqq) {
       printf("error: mq_len called on non t_mqq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

    long   sz = mq->size();
    l3obj* len = make_new_double_obj((double)sz, retown,"len");

    generic_delete(vv, L3STDARGS_OBJONLY);
    *retval = len;

}
L3END(mq_len)

L3METHOD(mq_find_name)
{
    *retval = gna;
}
L3END(mq_find_name)


L3METHOD(mq_find_val)
{
    *retval = gna;
}
L3END(mq_find_val)



L3METHOD(mq_erase_name)
{
    *retval = gtrue;
}
L3END(mq_erase_name)


L3METHOD(mq_erase_val)
{
    *retval = gtrue;
}
L3END(mq_erase_val)






// 
// add to end of the stack, with optional name.
//
// (pushback mqobj val "name")
//
L3METHOD(mq_pushback)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushback: " << exp->val() << " requires two or three arguments: ";
        printf("(pushdback mqobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
           sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* mq = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&mq);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( mq->_type != t_mqq) {
       printf("error: pushback called on non-mq object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   if(name) {
       if (name->_type != t_str && name->_type != t_lit) {
           printf("error: mq_pushback called with name that is neither string nor literal, in expression '%s'\n",sexps());
           generic_delete(vv, L3STDARGS_OBJONLY);
           l3throw(XABORT_TO_TOPLEVEL);
       }
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag || val->_owner->_parent == vv->_mytag) {
       val->_owner->generic_release_to(val, mq->_mytag);
   }

   mq_pushback_api(mq, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(mq_pushback)


L3KARG(mq_back,1)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);

   if (0==mqobj || mqobj->_type != t_mqq) {
       printf("error: mq_back called on non t_mqq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

    *retval = mq->back();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(mq_back)




L3KARG(mq_popback,1)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);
   mqueueobj* mq = 0;
   //l3obj* re = 0;

  XTRY
    case XCODE:
           if (0==mqobj || mqobj->_type != t_mqq) {
               printf("error: mq_popback called on non t_mqq object, in expression '%s'.\n",
                      sexps()
                      );
               l3throw(XABORT_TO_TOPLEVEL);
           }

           mq    = (mqueueobj*)(mqobj->_pdb);
           assert(mq);
           if (mq->size() == 0) {
               printf("error: mq_popback called on size zero dstaq; in expression '%s'.\n", sexps());
               l3throw(XABORT_TO_TOPLEVEL);
           }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.
           //           re = mq->back();
           //           *retval = make_new_link(re, "mq_popback_value", retown, env);
           // the above was problematic. make the user do back() if they want to save back element.

           mq->pop_back();

           *retval = gtrue;
       break;
    case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(mq_popback)




// 
// add to end of the stack, with optional name.
//
// (pushfront mqobj val "name")
//
L3METHOD(mq_pushfront)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushfront: "<< exp->val() << " requires two or three arguments: ";
        printf("(mq_pushfront mqobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
               sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* mq = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&mq);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( mq->_type != t_mqq) {
       printf("error: pushfront called on non-mq object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag || val->_owner->_parent == vv->_mytag) {
       val->_owner->generic_release_to(val, mq->_mytag);
   }

   mq_pushfront_api(mq, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(mq_pushfront)


L3KARG(mq_front,1)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);

   if (0==mqobj || mqobj->_type != t_mqq) {
       printf("error: mq_front called on non t_mqq object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    mqueueobj* mq    = (mqueueobj*)(mqobj->_pdb);

    *retval = mq->front();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(mq_front)




L3KARG(mq_popfront,1)
{
   l3obj* mqobj = 0;
   ptrvec_get(vv,0,&mqobj);
   mqueueobj* mq    = 0;   
   //l3obj* re = 0;

 XTRY
   case XCODE:
       if (0==mqobj || mqobj->_type != t_mqq) {
           printf("error: mq_popfront called on non t_mqq object, in expression '%s'.\n",
                  sexps()
                  );
           l3throw(XABORT_TO_TOPLEVEL);
       }

       mq    = (mqueueobj*)(mqobj->_pdb);
       assert(mq);
       if (mq->size() == 0) {
           printf("error: mq_popfront called on size zero dstaq; in expression '%s'.\n", sexps());
           l3throw(XABORT_TO_TOPLEVEL);
       }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.

       // no, problematic. make the user grab the element with a front() by itself it
       // they want it.
       //           re = mq->front();
       //           *retval = make_new_link(re, "mq_popfront_value", retown, env);

           mq->pop_front();

           *retval = gtrue;

       break;
 case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(mq_popfront)



long mq_len_api(l3obj* mqobj) {
    assert(mqobj);
    assert(mqobj->_type == t_mqq);
    
    mqueueobj* mq = (mqueueobj*)(mqobj->_pdb);

    return mq->size();
}



l3obj* mq_ith_api(l3obj* mqobj, long i, l3path* key) {
    assert(mqobj);
    assert(mqobj->_type == t_mqq);

    mqueueobj* mq = (mqueueobj*)(mqobj->_pdb);

    ddqueue::ll* hit = 0;
    l3obj* ret = mq->ith(i,&hit);

    if (hit && key) {
        key->reinit(hit->_ptr->name());
    }

    return ret;
}




void mq_print(l3obj* l3mq, const char* indent, stopset* stoppers) {
     assert(l3mq);
  
     l3path indent_more(indent);
     indent_more.pushf("%s","     ");

    mqueueobj* mq = (mqueueobj*)(l3mq->_pdb);


    long N = mq->size();
     printf("%s%p : (mq of size %ld): \n", indent, l3mq, N);
  
     mq->dump(indent_more(),stoppers);

}
