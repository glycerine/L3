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
 #include "l3obj.h"
 #include "autotag.h"
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
 #include "serialfac.h"
 #include "l3path.h"
 #include "objects.h"

 using std::string;

#ifndef _MACOSX
 #define HAVE_DECL_BASENAME 1
 #include <demangle.h>
#endif


 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"


// easy to find places for breakpoints
void hook_del(long sn, void* ptr);
void hook_new(long sn, void* ptr);


/////////////////////// end of includes

 // globals
long         serialnum_f::last_serialnum = 0;

hashmap_key2ptr       serialnum_f::key2ptr;
hashmap_ptr2key       serialnum_f::ptr2key;

hashmap_ptr2serial    serialnum_f::ptr2serial;
hashmap_serial2ptr    serialnum_f::serial2ptr;
long                  serialnum_f::_stop_on_allocation_sn = 0;
long                  serialnum_f::_stopline = 0;

hashmap_ptr2uuid   serialnum_f::ptr2uuid;
hashmap_uuid2ptr   serialnum_f::uuid2ptr;

serialnum_f* serialfactory = 0;

void serialnum_f::link_ptr_to_uuid(void* ptr, const std::string& uuid_string) {

    uuidstruct u;
    ptr2uuid_it it = ptr2uuid.find(ptr);
    if (it != ptr2uuid.end()) {

        // what if it is already linked... should we relink it? For now just complain:

        u = it->second;
        char buf_already[40];
        uuid_unparse(u.uuid, buf_already);

        printf("bad: in link_ser_to_uuid(%p, '%s'); this address is already linked to another uuid: '%s'\n",
               ptr, uuid_string.c_str(), buf_already);
        assert(0);
        exit(1);

    } else {
        // no clash found

        uuid_parse(uuid_string.c_str(), u.uuid);
        ptr2uuid.insert(hashmap_ptr2uuid::value_type(ptr,u));
        uuid2ptr.insert(hashmap_uuid2ptr::value_type(uuid_string,ptr));

    }
    
}

void serialnum_f::ptr_to_uuid(void* ptr, uuidstruct* uuid_out) {
    // generate these lazily, so as not to take up time during serial number allocation for malloc.
    
    uuidstruct u;
    
    ptr2uuid_it it = ptr2uuid.find(ptr);
    if (it == ptr2uuid.end()) {

        uuid_generate_time(u.uuid);
        char str_uuid[40];
        uuid_unparse(u.uuid, str_uuid);
        ptr2uuid.insert(hashmap_ptr2uuid::value_type(ptr,u));
        uuid2ptr.insert(hashmap_uuid2ptr::value_type(str_uuid,ptr));

        *uuid_out = u;
        return;
    }
    *uuid_out = it->second;
}

void* serialnum_f::uuid_to_ptr(const std::string& uuid) {

    uuid2ptr_it it = uuid2ptr.find(uuid);
    if (it == uuid2ptr.end()) {
        // plenty of these when we load older stored structures (?)
        printf("warning in serialnum_f::uuid2ser : uuid '%s' requested but not known!\n", uuid.c_str());
        //        assert(0);
        return 0;
    }

    return it->second;
}

// returns true if was present
bool serialnum_f::remove_uuid(const std::string& uuid) {

    uuid2ptr_it it = uuid2ptr.find(uuid);
    if (it == uuid2ptr.end()) {
        return false;
    }
    void* ptr = it->second;
    uuid2ptr.erase(it);

    ptr2uuid.erase(ptr);
    return true;
}


void* serialnum_f::ser2ptr(long sn) {
    
    serial2ptr_it it = serial2ptr.find(sn);
    if (it == serial2ptr.end()) {
        printf("internal error: serialnum_f::ser2ptr(%ld) did not have a matching pointer to return.\n",sn);
        assert(0);
        exit(1);
        return 0;
    }
    return it->second;
}


void serialnum_f::halt_on_sn(long stopon_sn) { 
  _stop_on_allocation_sn = stopon_sn; 
}


long serialnum_f::serialnum_obj(void* obj, Tag* owntag, const char* key /* key == varname */ ) {
    void* val = ((void*)(obj));
    return serialnum_priv(val, owntag, key);
}

long serialnum_f::serialnum_tag(Tag* tag, Tag* owntag, const char* key /* key == varname */ ) {
    void* val = tag;
    return serialnum_priv(val, owntag, key);
}

long serialnum_f::serialnum_sxp(void*  sxp, Tag* owntag) {
    void* val = sxp;
    l3path key(0,"sexp_t_%p",sxp);
    return serialnum_priv(val, owntag, key());
}


long serialnum_f::serialnum_priv(void* val, Tag* owntag, const char* key) {

    // skip leaks in here--they don't show up in valgrind but they do in sermon, but only on linux; sermon is off.
#ifndef  _DMALLOC_OFF
  BOOL sermon_prior_state =  gsermon->off();
#endif

  long sn = 0;
  last_serialnum++;
  sn = last_serialnum;

  hook_new(sn,val);
  if (_stop_on_allocation_sn) {
    if (sn == _stop_on_allocation_sn) {
      printf("At last_serialnum == %ld, set breakpoint on next line to debug.\n",sn);
      if (!_stopline) { _stopline = __LINE__; } printf("at serialnum_f::serialnum() in %s. Set breakpoint here.\n",AT);
    }
  }

  // sanity check that it's not already there...
  ptr2serial_it psn = ptr2serial.find(val);
  if (psn != ptr2serial.end()) {
    long snback = (psn->second);
    printf("ptr2serial map confusion ERROR: double entry of ptr detected in serialnum_f::serialnum(val:%p,key:'%s'): previously stored sn was: %ld.\n",val,key,snback);
    assert(0);
    exit(1);
  }

  serial2ptr_it snp = serial2ptr.find(sn);
  if (snp != serial2ptr.end()) {
    void* ptrback = (snp->second);
    printf("ptr2serial map confusion ERROR: double entry of sn detected in serialnum_f::serialnum(val:%p,key:'%s'): previously stored ptr was: %p.\n",val,key,ptrback);
    assert(0);
    exit(1);
  }

  // very bad, memory leaking idea:
  //  ptr2serial[val]=sn;
  //  serial2ptr[sn]=val;

  ptr2serial.insert(hashmap_ptr2serial::value_type(val,sn));
  serial2ptr.insert(hashmap_serial2ptr::value_type(sn,val));

  // Sanity checks: check for and reject on, pre-existing keys / ptrs
  key2ptr_it kp = key2ptr.find(key);
  if (kp != key2ptr.end()) {
    l3obj* objback = (l3obj*)(kp->second);
    print(objback,"----diagnostic in serialnum_f::reg()... print of *kp: ",0);
    printf("key2ptr map confusion ERROR: double entry of key detected in serialnum_f::reg(key:'%s',%p): previously stored ptr was: %p.\n",key,val, objback);
    printf("   with previous serialnum: %ld\n",ptr2serial[objback]);
    assert(0);
    exit(1);
  }

  ptr2key_it pk = ptr2key.find(val);
  if (pk != ptr2key.end()) {
    char* keyback = (char*)(pk->second.c_str());

    // and try to get a sn too
    

    printf("ptr2key map confusion ERROR: double entry of val detected in serialnum_f::reg(ptr:%p,key:'%s'):  previously stored key was: '%s'.\n",val,key,keyback);
    assert(0);
    exit(1);
  }

  long slen = strlen(key);
  if (0 == slen) {
    printf("serialnum_f::reg(key: '%s',objval: %p): key had length 0!!!!!!!!!!!!!!!!!!!! BAD! \n",key,val);
    assert(0);
    exit(1);
  }
  if (0 == val) {
    printf("serialnum_f::reg(key: '%s',objval: %p): key had length 0!!!!!!!!!!!!!!!!!!!! BAD! \n",key,val);
    assert(0);
    exit(1);
  }


  key2ptr.insert(hashmap_key2ptr::value_type(key,val));
  ptr2key.insert(hashmap_ptr2key::value_type(val,key));

  // very bad, memory leaking idea:
  //  key2ptr[key]=val;
  //  ptr2key[val]=key;

#ifndef  _DMALLOC_OFF
  gsermon->switch_to(sermon_prior_state);
#endif

  // finally, give back the serial number
  return sn; // for easy setting of breakpoint here.

}

long serialnum_f::del(void* val, l3path* name_freed) {
  // erase from all 4, now 6, maps

  ptr2key_it    i = ptr2key.find(val);
  ptr2serial_it j = ptr2serial.find(val);
  std::string key;
  long serial = 0;

  if (i != ptr2key.end()) {
    key = i->second;
  } 
  else assert(0);
  // jea todo reactivate  else assert(0); // TODO: turn this back on and figure out why its firing.

  name_freed->init(key.c_str());

  if (j != ptr2serial.end()) {
    serial = j->second;
  } 
  else assert(0);

  // allow easy breakpointing on a specific serial number delete.
  hook_del(serial,val);

  // these two seem to work:
  serial2ptr.erase(serial);
  key2ptr.erase(key);

  // and erase from ptr2uuid and uuid2ptr
  ptr2uuid_it s2uuidit = ptr2uuid.find(val);
  if (s2uuidit != ptr2uuid.end()) {

      uuidstruct u = s2uuidit->second;
      char str_uuid[40];
      uuid_unparse(u.uuid, str_uuid);

      uuid2ptr_it u2s_it = uuid2ptr.find(str_uuid);
      if (u2s_it != uuid2ptr.end()) {
          uuid2ptr.erase(u2s_it);
      }

      ptr2uuid.erase(s2uuidit);
  }


  long ptr2serial_size_before = ptr2serial.size();

  DV(printf("size of ptr2serial was %ld before erase(val:%p)\n",ptr2serial_size_before,val));
  ptr2serial_it n = ptr2serial.find(val);
  if (n != ptr2serial.end()) {
    DV(printf("found val %p in ptr2serial, key: %p   value: %ld\n",val,n->first,n->second));
  } else {
    DV(printf("NOT FOUND: val %p in ptr2serial not found.\n",val));
    assert(0);
  }
    
  ptr2serial.erase(val);
  //  ptr2serial.erase(j);
  long ptr2serial_size_after = ptr2serial.size();
  DV(printf("size of ptr2serial was %ld after erase(val:%p)\n",ptr2serial_size_after,val));
  // jea todo reactivate  assert(ptr2serial_size_before - ptr2serial_size_after == 1);

  ptr2key_it m = ptr2key.find(val);
  if (m != ptr2key.end()) {
    DV(printf("found val %p in ptr2key, key: %p   value: %s\n",val,m->first,m->second.c_str()));
  } else {
    DV(printf("NOT FOUND: val %p in ptr2key not found.\n",val));
    assert(0);
  }

  long ptr2key_size_before = ptr2key.size();
  DV(printf("size of ptr2key was %ld before erase(val:%p)\n",ptr2key_size_before,val));

  ptr2key.erase(val);
  //  ptr2key.erase(i);
  long ptr2key_size_after = ptr2key.size();
  DV(printf("size of ptr2key was %ld after erase(val:%p)\n",ptr2key_size_after,val));
  // jea todo reactivate  assert(ptr2key_size_before - ptr2key_size_after == 1);

  //   check(); // definitely won't be balanced here yet!
  return serial;
}


void serialnum_f::show_named_mem() {
  key2ptr_it it = key2ptr.begin();
  key2ptr_it en = key2ptr.end();

  for (; it != en; ++it) {
      ptr2serial_it p2s = ptr2serial.find(it->second);

      if (p2s == ptr2serial.end()) {
          printf("%p  - [    ****    no ptr2serial entry]  -> %s\n",it->second, it->first.c_str());
      } else {
          long sn = p2s->second;
          printf("%p  - serial %ld  -> %s\n", it->second, sn , it->first.c_str());
      }
  }
}

void serialnum_f::check() {
  uint n = serial2ptr.size();
  uint m = ptr2serial.size();
  uint p = ptr2key.size();
  uint q = key2ptr.size();
  assert(n == m);
  assert(n == p);
  assert(n == q);
}

uint serialnum_f::size() {
  uint m = ptr2serial.size();
  //  check();
  return m;
}

// heap_has_any_user_obj
//
// returns 1 if all objects on heap are marked sysbuiltin, 0 otherwise
//
// if given, we do not count exclue_me, so as to allow the current
//   call to 'allbuiltin' to not alarm/fire leak detector.
//
int  serialnum_f::heap_all_builtin(qtree* exclude_me) {

  ptr2serial_it h  = ptr2serial.begin();
  ptr2serial_it he = ptr2serial.end();

  long n = 0;
  long heapsize = ptr2serial.size();
  l3obj* ptr = 0;
  Tag*   tag = 0;
  long  sn  = 0;

  for (; h != he; ++h,++n) {

      ptr = (l3obj*)(h->first);
      tag = (Tag*)(h->first);
      sn  = h->second;

      if (!is_sysbuiltin(ptr)) {

          if (ptr == (l3obj*)exclude_me) continue;

          printf("allbuiltin: %p ser# %ld was on heap but not sysbuiltin.\n",ptr,sn);
          return 0;
      }

      assert(n < heapsize);
  }

  return 1;
}

void serialnum_f::dump() {

  key2ptr_it h  = key2ptr.begin();
  key2ptr_it he = key2ptr.end();

  
  int n = 0;
  n = 0;
  printf("\n\n==== heap:  size %ld\n",key2ptr.size());
  for (; h != he; ++h,++n) {

    // also lookup serial for this ptr, but don't insert if not there by using the [] based query (Yuck!)
    hashmap_ptr2serial::iterator psit = ptr2serial.find(h->second);
    if (psit != ptr2serial.end()) {
      printf("%02d    %p  - serial %ld  -> %s\n",n,h->second, psit->second, h->first.c_str());
    } else {
      printf("%02d    %p  - [    ****    no ptr2serial entry]  -> %s\n",n,h->second, h->first.c_str());
    }
  }
  
  /*
  ptr2key_it j  = ptr2key.begin();
  ptr2key_it je = ptr2key.end();
  
  n = 0;
  printf("II. ptr2key:  %ld\n",ptr2key.size());
  for (; j != je; ++j,++n) {

    // also lookup serial for this ptr, but don't insert if not there by using the [] based query (Yuck!)
    hashmap_ptr2serial::iterator psit = ptr2serial.find(j->first);
    if (psit != ptr2serial.end()) {
      printf("%02d    %p  - serial %ld  -> %s\n",n,j->first, psit->second, j->second.c_str());
    } else {
      printf("%02d    %p  - [    ***    no ptr2serial entry] -> %s\n",n,j->first, j->second.c_str());
    }
  }
  





  serial2ptr_it i  = serial2ptr.begin();
  serial2ptr_it ie = serial2ptr.end();
  
  n = 0;
  printf("III. serial2ptr:  %ld\n",serial2ptr.size());
  for (; i != ie; ++i,++n) {

    // also lookup key for this ptr, but don't insert if not there by using the [] based query (Yuck!)
    hashmap_ptr2key::iterator pkit = ptr2key.find(i->second);
    if (pkit != ptr2key.end()) {
      printf("%02d    %p  - serial %ld  -> %s\n",n,                 i->second, i->first, pkit->second.c_str());
    } else {
      printf("%02d    %p  - serial %ld  -> [    ***    no ptr2key entry]\n",n, i->second, i->first);
    }
  }
  
  ptr2serial_it k  = ptr2serial.begin();
  ptr2serial_it ke = ptr2serial.end();

  n = 0;
  printf("IV. ptr2serial:  %ld\n",ptr2serial.size());
  for (; k != ke; ++k, ++n) {

    // also lookup key for this ptr, but don't insert if not there by using the [] based query (Yuck!)
    hashmap_ptr2key::iterator pkit = ptr2key.find(k->first);
    if (pkit != ptr2key.end()) {
      printf("%02d    %p  - serial %ld  -> %s\n",n,                 k->first, k->second, pkit->second.c_str());
    } else {
      printf("%02d    %p  - serial %ld  -> [    ***    no ptr2key entry]\n",n, k->first, k->second);
    }
  }
  */

  printf("  serialnum_f::last_serialnum = %ld\n",serialnum_f::last_serialnum  );

  //    check();
}

long serialnum_f::get_last_issued_sn() {
  return last_serialnum;
}

long serialnum_f::get_halt_on_sn() {
  return _stop_on_allocation_sn;
}

long serialnum_f::get_stopline() {
  return _stopline;
}

void serialnum_f::check_alive_ptr_or_assert(void* v) {

  ptr2serial_it n = ptr2serial.find(v);
  if (n == ptr2serial.end()) {
    printf("LIVE check failed: val %p in serailnum_f::ptr2serial not found.\n",v);
    assert(0);
    exit(1);
  }
}

void serialnum_f::check_alive_sn(long sn) {
  serial2ptr_it n  = serial2ptr.find(sn);
  if (n == serial2ptr.end()) {
    printf("LIVE check failed: sn %ld in serailnum_f::serial2ptr not found.\n",sn);
    assert(0);
    exit(1);
  }
}


bool  serialnum_f::is_alive_sn(long sn) {
  serial2ptr_it n  = serial2ptr.find(sn);
  if (n == serial2ptr.end()) {
      return false;
  }
  return true;
}

//
//  sanity_check_judys_of_obj
// if obj has a judyS, then check it for char[] -> 0x0 (should not find 0x0 in here).
//
// cannot do any allocation during a call to sane() or well mess up the iteration
//  above in the caller. therefore this is only an internal method now.
void sane(l3obj* cur) 
{
    LIVEO(cur);
    assert(cur->_type);
    if (cur->_type != t_obj) return;
    if (cur->_judyS == 0) return;

    size_t       sz = 0;
    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, cur->_judyS->_judyS, Index);       // get first string
    
    while (PValue != NULL)
        {
            llref* llr = *((llref**)(PValue));
            
            if (!llr) {
                printf("error in sanity_check: found null llref inside _judyS on %ld-th member object:  %p ser# %ld  under name '%s'\n",
                       sz, cur, cur->_ser, Index);
                assert(0);
            } else {
                LIVEREF(llr);
            }
            
            JSLN(PValue, cur->_judyS->_judyS, Index);   // get next string
            ++sz;
        }

    DV(printf("object %p ser# %ld passed santiy check: no null llref in _judyS table.\n",cur,cur->_ser));

}

L3METHOD(allsane)
{
    //#ifdef _EVAL_LIVEO

    long N = serialfactory->ptr2serial.size();
    printf("allsane has %ld objects to check.\n", N);

    ptr2serial_it be = serialfactory->ptr2serial.begin();
    ptr2serial_it en = serialfactory->ptr2serial.end();
    l3obj* cur = 0;
    long  Nobj = 0;
    long  i = 0;
    for( ; be != en; ++be, ++i) {
        cur = (l3obj*)(be->first);
        if (cur && cur->_type == t_obj) {
            ++Nobj;
            sane(cur);
            DV(printf("%ld passed.\n",i));
        }
    }

    printf("allsane checked %ld t_obj objects.\n", Nobj);


    *retval = gtrue;

    //#else
    //    *retval = gnil;
    //#endif
}
L3END(allsane)



// scan all known objects for justdeleted l3obj*, in everyone's _judyS : l3obj* -> llref* -> _env
// dont call this in production code, since it is O(n^2).
//
void serialnum_f::loose_ends_check(l3obj* justdeleted, long sn_justdeleted) {

    // is this function with the JSLF calls causing corruption?
    return;

    ptr2serial_it be = ptr2serial.begin();
    ptr2serial_it en = ptr2serial.end();

    size_t       sz = 0;
    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    //t_typ recognized_type = 0;

    l3obj* cur = 0;
    for( ; be != en; ++be) {
        cur = (l3obj*)(be->first);

        if (cur->_judyS->_judyS
            && cur->_type 
            && cur->_type == t_obj
            ) {

            DV(printf("type that is actually checked in loose_ends_check:    %s\n", cur->_type));

            Index[0] = '\0';                    // start with smallest string.
            JSLF(PValue, cur->_judyS->_judyS, Index);       // get first string
            
            while (PValue != NULL)
                {
                    llref* llr = *((llref**)(PValue));

                    if (0 != llr) {

                        if (llr->_env == justdeleted
                            || llr->_obj == justdeleted) {
                            
                            printf("Huston, we have a problem. In loose_ends_check for just deleted: (%p ser# %ld), there is a loose end in object: %p ser# %ld '%s'\n",
                                   justdeleted,
                                   sn_justdeleted,
                                   cur,
                                   cur->_ser,
                                   cur->_varname
                                   );
                            assert(0);
                            exit(1);
                        }
                    } else {
                        printf("warning: loose_ends_check() is skipping null llref inside _judyS on object %p ser# %ld   under key '%s'\n",cur, cur->_ser, Index);
                        assert(0);
                    }

                    JSLN(PValue, cur->_judyS->_judyS, Index);   // get next string
                    ++sz;
                }
            
        }
    }

}


// call heap_all_builtin
L3METHOD(allbuiltin) 
  if (serialfactory->heap_all_builtin(exp)) {
      *retval = gtrue;
  } else {
      *retval = gnil;
  }
L3END(allbuiltin)

  // allow easy breakpointing on a specific serial number delete.
void hook_del(long sn, void* ptr) {
    DV(printf("serialfac.cpp::hook_del(%ld)\n", sn));
}

void hook_new(long sn, void* ptr) {
    DV(printf("serialfac.cpp::hook_new(%ld)\n", sn));
}
