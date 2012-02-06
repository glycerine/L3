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
#include "l3dd.h"

/////////////////////// end of includes


l3obj* ddqueueobj::ith(long i, ddqueue::ll** phit) {
    lnk* tmp = _symlinks.ith(i,phit);
    if (tmp && tmp->target()) {
         return tmp->chase();
     }
     return gna;
}

#if 0
// current no way to delete by or value / key, only by position/order

l3obj* ddqueueobj::remove_key(char* key) {


}

l3obj* ddqueueobj::remove_val(l3obj* val) {
    if (_symlinks.del_val(val)) {
        return val;
    }
    return gna;
}
#endif


void ddqueueobj::push_back(t_typ key, l3obj* val) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_back(link);
}


void ddqueueobj::push_front(t_typ key, l3obj* val) {
    lnk* link = new_lnk(val, _symlinks._mytag, key);
    _symlinks.push_front(link);
}

l3obj*  ddqueueobj::del_ith(long i) {

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


l3obj* ddqueueobj::pop_back() {
    lnk* link = _symlinks.pop_back();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* ddqueueobj::pop_front() {
    lnk* link = _symlinks.pop_front();
    
    l3obj* ret = link->chase();

    assert(_symlinks._mytag->lnk_exists(link));

    _symlinks._mytag->lnk_remove(link);
    del_lnk(link);

    return ret;
}

l3obj* ddqueueobj::back() {
    lnk* link = _symlinks.back_val();
    return link->chase();
}

l3obj* ddqueueobj::front() {
    lnk* link = _symlinks.front_val();
    return link->chase();
}



// dtor
ddqueueobj::~ddqueueobj() {

}

// copy ctor
ddqueueobj::ddqueueobj(const ddqueueobj& src) 
    : _symlinks(src._symlinks)
{
    // dstaq copy constructor for _symlinks works.
}

ddqueueobj& ddqueueobj::operator=(const ddqueueobj& src) {
    if (&src == this) return *this;

    if (src.size()) {
        _symlinks.push_back_all_from(src._symlinks);
    }

    return *this;
}


// ctor
ddqueueobj::ddqueueobj(Tag* tag) 
: _symlinks(tag) 
{  

}


L3METHOD(make_new_dd)
{
   l3obj* nobj = 0;
   l3path clsname("dd");
   l3path objname("dd");
   long extra = sizeof(ddqueueobj);
   make_new_captag((l3obj*)&objname, extra,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown,ifp);

   nobj->_type = t_ddt;
   nobj->_parent_env  = env;

   ddqueueobj* dd = (ddqueueobj*)(nobj->_pdb);
   dd = new(dd) ddqueueobj(nobj->_mytag); // placement new, so we can compose objects

   nobj->_dtor = &dd_dtor;
   nobj->_cpctor = &dd_cpctor;
  
   *retval = nobj;
}
L3END(make_new_dd)


// l3obj wrapper for dstaq

// equivalent of make_new_dd
L3METHOD(dd)
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
   make_new_dd(L3STDARGS);

   l3obj* nobj = *retval;

   //
   // and pushback the contents requested by the initialization
   //
   l3obj* ele = 0;
   //sexp_t* nex = exp->list->next->next;
   for (long i = 0; i < N; ++i) {
        ptrvec_get(vv,i,&ele);

        // gotta do this *before* the dd_pushback, or else the
        //  llref / target cleanup will delete from the dd. not what we want.
        //
        // if vv is the owner, then transfer ownership to the dstaq,
        // so that command line entered values are retained.
        //

        if (ele->_owner == vv->_mytag || ele->_owner->_parent == vv->_mytag) {
            ele->_owner->generic_release_to(ele, nobj->_mytag);
        }

        dd_pushback_api(nobj,ele,0);
    }

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(dd)


L3METHOD(dd_dtor)
{
    assert(obj->_type == t_ddt);

    ddqueueobj* dd = (ddqueueobj*)(obj->_pdb);
   
    dd->~ddqueueobj();
}
L3END(dd_dtor)


//
// vv = ptrvec to push onto the back of ddobj
//
void dd_pushback_api(l3obj* ddobj, l3obj* val, l3obj* keyobj) {
    LIVEO(ddobj);
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
            printf("error: dd_pushback key '%s' was not a recognized type.",key());
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    assert(ddobj->_type == t_ddt);

    ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

    dd->push_back(ty,val);

}

void dd_pushfront_api(l3obj* ddobj, l3obj* val, l3obj* keyobj) {
    LIVEO(ddobj);
    LIVEO(val);

    l3path key;
    if (keyobj) {
        string_get(keyobj,0,&key);
    }

    assert(ddobj->_type == t_ddt);

    ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

    dd->push_front(key(), val);

}



L3METHOD(dd_cpctor)
{

    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    Tag* dd_tag = nobj->_mytag;

    // assert we are a captag pair
    assert( dd_tag );
    assert( pred_is_captag_obj(nobj) );
    
    ddqueueobj* dd_src = (ddqueueobj*)(src->_pdb);
    ddqueueobj* dd    = (ddqueueobj*)(nobj->_pdb);

    // use the underlying dstaq copy constructor
    if (dd_src->size()) {

        // assignment operator invocation.
        dd = dd_src;
    }

    // *retval was already set for us, by the base copy ctor.
}
L3END(dd_cpctor)

// for both  dd_del_ith() and dd_ith()
long dd_helper_verify_integer_index(l3obj* ddobj, l3obj* vv, l3path& sexps, Tag* owner, Tag* retown) {
    FILE* ifp = 0;
 
   if (0==ddobj || ddobj->_type != t_ddt) {
       printf("error: dd_ith called on non t_ddt object, in expression '%s'.\n",
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* index = 0;

   ptrvec_get(vv,1,&index);
   if (0==index || index->_type != t_dou || double_size(index) < 1) {
       printf("error: dd_ith called with bad index value, non-numeric, in expression '%s'.\n",
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   double dindex = double_get(index,0);

   long   as_integer = (long)dindex;
   bool   is_integer = (dindex == (double)as_integer);

   if (!is_integer) {
       printf("error: dd_ith called with bad index value, non-integer index '%.6f', in expression '%s'.\n",
              dindex,
              sexps());
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   return as_integer;
}


L3KARG(dd_del_ith,2)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);

   long as_integer = dd_helper_verify_integer_index(ddobj, vv, sexps,owner,retown);

   ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

   // don't mod-wrap deletion indices, to catch errors.
   long sz = dd->size();
   long abs_index = labs(as_integer);
   if (abs_index >= sz) {
       printf("error in dd_del_ith: index %ld was out of range; size of dd is %ld.\n", as_integer, sz);
       l3throw(XABORT_TO_TOPLEVEL);
       generic_delete(vv, L3STDARGS_OBJONLY);   
   }

   *retval = dd->del_ith(as_integer);

   generic_delete(vv, L3STDARGS_OBJONLY);   
   
}
L3END(dd_del_ith)


L3KARG(dd_ith,2)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);

   long as_integer = dd_helper_verify_integer_index(ddobj, vv, sexps,owner,retown);

   ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

   *retval = dd->ith(as_integer,0);

   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dd_ith)


L3METHOD(dd_size)
   return dd_len(L3STDARGS);
L3END(dd_size)

L3KARG(dd_len,1)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);

   if (0==ddobj || ddobj->_type != t_ddt) {
       printf("error: dd_len called on non t_ddt object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

    long   sz = dd->size();
    l3obj* len = make_new_double_obj((double)sz, retown,"len");

    generic_delete(vv, L3STDARGS_OBJONLY);
    *retval = len;

}
L3END(dd_len)





// 
// add to end of the stack, with optional name.
//
// (pushback ddobj val "name")
//
L3METHOD(dd_pushback)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushback: "<< exp->val() << " requires two or three arguments: ";
        printf("(pushdback ddobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
               sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* dd = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&dd);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( dd->_type != t_ddt) {
       printf("error: pushback called on non-dd object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   if(name) {
       if (name->_type != t_str && name->_type != t_lit) {
           printf("error: dd_pushback called with name that is neither string nor literal, in expression '%s'\n",sexps());
           generic_delete(vv, L3STDARGS_OBJONLY);
           l3throw(XABORT_TO_TOPLEVEL);
       }
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag || val->_owner->_parent == vv->_mytag) {
       val->_owner->generic_release_to(val, dd->_mytag);
   }
   

   dd_pushback_api(dd, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dd_pushback)


L3KARG(dd_back,1)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);

   if (0==ddobj || ddobj->_type != t_ddt) {
       printf("error: dd_back called on non t_ddt object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

    *retval = dd->back();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(dd_back)




L3KARG(dd_popback,1)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);
   ddqueueobj* dd = 0;
   //l3obj* re = 0;

  XTRY
    case XCODE:
           if (0==ddobj || ddobj->_type != t_ddt) {
               printf("error: dd_popback called on non t_ddt object, in expression '%s'.\n",
                      sexps()
                      );
               l3throw(XABORT_TO_TOPLEVEL);
           }

           dd    = (ddqueueobj*)(ddobj->_pdb);
           assert(dd);
           if (dd->size() == 0) {
               printf("error: dd_popback called on size zero dstaq; in expression '%s'.\n", sexps());
               l3throw(XABORT_TO_TOPLEVEL);
           }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.
           //           re = dd->back();
           //           *retval = make_new_link(re, "dd_popback_value", retown, env);
           // the above was problematic. make the user do back() if they want to save back element.

           dd->pop_back();

           *retval = gtrue;
       break;
    case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(dd_popback)




// 
// add to end of the stack, with optional name.
//
// (pushfront ddobj val "name")
//
L3METHOD(dd_pushfront)
{
    long min_num_args_required = 2;
    long max_num_args_allowed = 3;
    arity = num_children(exp);

    l3path sexps(exp);
    if (arity < min_num_args_required || arity > max_num_args_allowed) {
        std::cout << "error in pushfront: " << exp->val() << " requires two or three arguments: ";
        printf("(dd_pushfront ddobj val \"name\") where \"name\" is optional; '%s' had %ld arguments.\n",
               sexps(), arity);
        XRaise(XABORT_TO_TOPLEVEL);
    }

    l3obj* vv  = 0;
    if (arity>0) {
        // screens for unresolved references, gets values up front. skips operator position.
        any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    }
    

    l3obj* dd = 0;
    l3obj* val = 0;
    l3obj* name = 0;
    ptrvec_get(vv,0,&dd);
    ptrvec_get(vv,1,&val);
    if (ptrvec_size(vv) == 3) {
        ptrvec_get(vv,2,&name);
    }

   if( dd->_type != t_ddt) {
       printf("error: pushfront called on non-dd object.\n");
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   //
   // if vv is the owner, then transfer ownership to the dstaq,
   // so that command line entered values are retained.
   //
   
   if (val->_owner == vv->_mytag || val->_owner->_parent == vv->_mytag) {
       val->_owner->generic_release_to(val, dd->_mytag);
   }
   

   dd_pushfront_api(dd, val, name);

   *retval = val;
   generic_delete(vv, L3STDARGS_OBJONLY);

}
L3END(dd_pushfront)


L3KARG(dd_front,1)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);

   if (0==ddobj || ddobj->_type != t_ddt) {
       printf("error: dd_front called on non t_ddt object, in expression '%s'.\n",
              sexps()
              );
       l3throw(XABORT_TO_TOPLEVEL);
   }

    ddqueueobj* dd    = (ddqueueobj*)(ddobj->_pdb);

    *retval = dd->front();

    generic_delete(vv, L3STDARGS_OBJONLY);
}
L3END(dd_front)




L3KARG(dd_popfront,1)
{
   l3obj* ddobj = 0;
   ptrvec_get(vv,0,&ddobj);
   ddqueueobj* dd    = 0;   
   //l3obj* re = 0;

 XTRY
   case XCODE:
       if (0==ddobj || ddobj->_type != t_ddt) {
           printf("error: dd_popfront called on non t_ddt object, in expression '%s'.\n",
                  sexps()
                  );
           l3throw(XABORT_TO_TOPLEVEL);
       }

       dd    = (ddqueueobj*)(ddobj->_pdb);
       assert(dd);
       if (dd->size() == 0) {
           printf("error: dd_popfront called on size zero dstaq; in expression '%s'.\n", sexps());
           l3throw(XABORT_TO_TOPLEVEL);
       }

           // problem: if we are about to assign this and we are loosing an link, we might
           //          end up deleting before we are done with it. Transfer ownership first
           //          to the retown.

           // one soln: make a new link and return that.

       // no, problematic. make the user grab the element with a front() by itself it
       // they want it.
       //           re = dd->front();
       //           *retval = make_new_link(re, "dd_popfront_value", retown, env);

           dd->pop_front();

           *retval = gtrue;

       break;
 case XFINALLY:
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       break;
  XENDX

}
L3END(dd_popfront)



long dd_len_api(l3obj* ddobj) {
    assert(ddobj);
    assert(ddobj->_type == t_ddt);
    
    ddqueueobj* dd = (ddqueueobj*)(ddobj->_pdb);

    return dd->size();
}



l3obj* dd_ith_api(l3obj* ddobj, long i, l3path* key) {
    assert(ddobj);
    assert(ddobj->_type == t_ddt);

    ddqueueobj* dd = (ddqueueobj*)(ddobj->_pdb);

    ddqueue::ll* hit = 0;
    l3obj* ret = dd->ith(i,&hit);

    if (hit && key) {
        key->reinit(hit->_ptr->name());
    }

    return ret;
}




void dd_print(l3obj* l3dd, const char* indent, stopset* stoppers) {
     assert(l3dd);
  
     l3path indent_more(indent);
     indent_more.pushf("%s","     ");

    ddqueueobj* dd = (ddqueueobj*)(l3dd->_pdb);

    long N = dd->size();
     printf("%s%p : (dd of size %ld): \n", indent, l3dd, N);
  
     dd->dump(indent_more(),stoppers);

}
