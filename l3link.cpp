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

#include "dstaq.h"
#include "l3dstaq.h"
#include "l3link.h"

/////////////////////// end of includes


// link : l3obj wrapper for lnk



linkobj::linkobj(l3obj* tar, Tag* owner, const char* key) {

    _lnk = new_lnk(tar,owner,key);
}

linkobj::linkobj(const linkobj& src) {
    CopyFrom(src);
}

linkobj& linkobj::operator=(const linkobj& src) {
    if (&src != this) {
        CopyFrom(src);
    }
    return *this;
}

void linkobj::CopyFrom(const linkobj& src) {
    _lnk = new_lnk(src._lnk->target(),
                   src._lnk->_owner,
                   src._lnk->name());
}

linkobj::~linkobj() {
    if (_lnk) {
        _lnk->_owner->lnk_remove(_lnk);
        del_lnk(_lnk);
        _lnk = 0;
    }
}



L3METHOD(link_dtor)
{
    assert(obj->_type == t_lin);

    linkobj* lin = (linkobj*)(obj->_pdb);
   
    lin->~linkobj();
}
L3END(link_dtor)


L3METHOD(link_cpctor)
{

    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    linkobj* link_src = (linkobj*)(src->_pdb);
    linkobj* lin    = (linkobj*)(nobj->_pdb);

    // assignment operator invocation.
    *lin = *link_src;

    // *retval was already set for us, by the base copy ctor.
}
L3END(link_cpctor)

// api
l3obj* make_new_link(l3obj* tar, const char* key, Tag* retown, l3obj* parent_env) {

   // make the linkobj

   l3path clsname("link");
   l3path objname("link");
   l3obj* nobj = make_new_class(sizeof(linkobj), retown,clsname(),objname());

   nobj->_type = t_lin;
   nobj->_parent_env  = parent_env;

   linkobj* lin = (linkobj*)(nobj->_pdb);
   lin = new(lin) linkobj(tar, retown, key); // placement new, so we can compose objects

   nobj->_dtor = &link_dtor;
   nobj->_cpctor = &link_cpctor;
   
   return nobj;
}

//
// (link "my_name" my_obj)
//     --> establishes this object with a name "my_name" and a target my_obj
//
L3KARG(link,2)
{
   l3obj* keyobj = 0;
   ptrvec_get(vv,0,&keyobj);
   l3path key;
   if (keyobj->_type != t_str) {
       printf("error in (link name target) link creation: name of link was not a string, but rather type '%s', in expression '%s'.\n",
              keyobj->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }
   string_get(keyobj, 0,&key);

   l3obj* tar = 0;
   ptrvec_get(vv,1,&tar);

   if (tar->_owner == vv->_mytag || tar->_owner->_parent == vv->_mytag) {
       // about to loose the temporarly, give ownership to...same as link.
       // this has to happen before we make the link, since the ownership
       // transfer will null the lnk pointer.
       tar->_owner->generic_release_to(tar, retown);
   } 

   l3obj* nobj = make_new_link(tar, key(), retown, env);
   *retval = nobj;

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(link)



// change the target
// 
//  (relink  change_this_linkobj  newtarget)
//
L3KARG(relink,2)
{
   l3obj* lo = 0;
   ptrvec_get(vv,0,&lo);
   if (lo->_type != t_lin) {
       printf("error in (relink  alink  newtarget) relinking: alink was not a link, but rather type '%s', in expression '%s'.\n",
              lo->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }


   l3obj* tar = 0;
   ptrvec_get(vv,1,&tar);    

   linkobj* mylink = (linkobj*)(lo->_pdb);

   l3path name(mylink->_lnk->name());


   mylink->_lnk->_owner->lnk_remove(mylink->_lnk);
   del_lnk(mylink->_lnk);

   mylink->_lnk = new_lnk(tar,
                            retown,
                            name());

   *retval = lo;
   
   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(relink)


//
// (chase alink) -> returns the pointed to l3obj*
//
L3KARG(chase,1)
{
   l3obj* lo = 0;
   ptrvec_get(vv,0,&lo);
   if (lo->_type != t_lin) {
       printf("error in (chase  alink): alink was not a link, but rather type '%s', in expression '%s'.\n",
              lo->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }

   linkobj* mylink = (linkobj*)(lo->_pdb);

   *retval = mylink->_lnk->chase();

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(chase)



//
// linkname: just return the link label (symbol name) as a string.
//
L3KARG(linkname,1)
{
   l3obj* lo = 0;
   ptrvec_get(vv,0,&lo);
   if (lo->_type != t_lin) {
       printf("error in (linkname  alink): alink was not a link, but rather type '%s', in expression '%s'.\n",
              lo->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }

   linkobj* mylink = (linkobj*)(lo->_pdb);

   *retval = make_new_string_obj(mylink->_lnk->name(), retown, "linkname_output");

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(linkname)



//
// (rename  link  newname) : rename the link
//
L3KARG(rename,2)
{
   l3obj* lo = 0;
   ptrvec_get(vv,0, &lo);

   if (lo->_type != t_lin) {
       printf("error in (linkname  alink): alink was not a link, but rather type '%s', in expression '%s'.\n",
              lo->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* newname = 0;
   ptrvec_get(vv,1, &newname);
   if (newname->_type != t_lit && newname->_type != t_str) {
       printf("error during rename of link: new name was not a string but rather '%s', in expression '%s'.\n",
              newname->_type, sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }
   l3path newkey;
   string_get(newname, 0,&newkey);

   if (newkey.len() == 0) {
       printf("error during rename of link: new name was the empty string in expression '%s'.\n", sexps());
       generic_delete(vv, L3STDARGS_OBJONLY); 
       l3throw(XABORT_TO_TOPLEVEL);
   }

   linkobj* mylink = (linkobj*)(lo->_pdb);

   mylink->_lnk->update_key(newkey());

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY); 
   }
}
L3END(rename)

