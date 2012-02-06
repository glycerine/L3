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

//#include "slisp_util.h"

 #include "objects.h"
 #include "symvec.h"
 #include "l3dstaq.h"

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
#include "l3pratt.h"
#include "l3sexpobj.h"

/////////////////////// end of includes


// to string methods
void to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers);

void bool_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  t_typ ty = obj->_type;

  if (obj == gnil || ty == t_fal) {
    s->pushf("F%s",aft ? aft : "");
    return;
  } else if (obj == gtrue || ty == t_tru) {
    s->pushf("T%s",aft ? aft : "");
    return;
  }
  assert(0);
}

void double_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  assert(obj->_type == t_dou);

  long N = double_size(obj);
  double dval = 0;
  for (long i = 0; i < N; ++i) {
    dval = double_get(obj,i);
    s->pushf("%.6f%s",dval,aft ? aft : "");
  }
}

void string_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  assert(obj->_type == t_str);

  l3path stmp;
  string_get(obj,0,&stmp);

  //  s->pushf("%s%s", stmp(), aft);
  s->pushf("%s", stmp());
}

void literal_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  assert(obj->_type == t_lit);

  l3path stmp;
  literal_get(obj,&stmp);
  s->pushf("%s%s", stmp(), aft);
}

void ptrvec_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  assert(obj->_type == t_vvc);

#if 0   // I think the one at the top, in to_string(), suffices. This is stopping us too early.
  DV(p(stoppers));
  if ((long)stoppers == -1) {
      // disable stopset functionality with the -1 value.
  } else if (stoppers == 0) { 
    stopset_clear(&global_stopset);
    stoppers = &global_stopset;
    stopset_push(stoppers,obj);
  } else {
    if (obj_in_stopset(stoppers, obj)) return;
    stopset_push(stoppers,obj); // only print it once.
  }
#endif

  long N = ptrvec_size(obj);
  l3obj* ptr = 0;
  for (long i = 0; i < N; ++i) {
    ptr = 0;
    ptrvec_get(obj,i,&ptr);

    // temporarily disable stoppers here:
    //    to_string(ptr,s,aft,stoppers);
    // because they were interfering with referencing the curcur obj in a foreach loop
    to_string(ptr,s," ",(stopset*)-1);
  }
}

void fun_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);

  
  if (!obj->_type || obj->_type != t_fun) {
    printf("expected function object in fun_to_string.\n");
    assert(0);
  }
  
  defun* df = (defun*)obj->_pdb;
  assert(df);

  l3bigstr d;
  df->body->stringify(&d,0);
  s->pushf("%s",d());


#if 0 // old

  // anonymous functions now : s->pushf("(de %s ",df->funcname());
  s->pushf("(de ");
  s->pushf("(prop ");

  int nret = df->ret_argtyp.size();			  

  l3path argtyp_str;

  df->ret_name.it.set_staq(&df->ret_name);
  for (int i = 0; i < nret; ++i, ++(df->ret_name.it)) {
      argtyp_str.clear();
      df->ret_argtyp[i]->dump_to_l3path(&argtyp_str);
      
      s->pushf("(return %s %s %s) ", 
               *(df->ret_name.it), owntagtype2abbrev(df->ret_owntag[i]), argtyp_str());
  }

  int narg = df->arg_argtyp.size();
  l3path argtypstr;
  df->arg_name.it.set_staq(&df->arg_name);
  for (int i = 0; i < narg; ++i, ++(df->arg_name.it)) {
      argtypstr.clear();
      df->arg_argtyp[i]->dump_to_l3path(&argtypstr);
      s->pushf("(arg %s %s %s) ", *(df->arg_name.it), owntagtype2abbrev(df->arg_owntag[i]), argtypstr());
  }

  s->pushf(") ");

  // create pretty print version too...
  l3path pretty((*s)());
  l3path baseind("");
  pretty_print_sexp(s->p, PATH_MAX- (s->len()),df->body,&pretty,baseind(),defptag_get());
  pretty.pushf(")\n");
  s->set_p_from_strlen();
  s->pushf(")");

  // both all-on-one-line (s) and pretty print (pretty) are ready to go.
  // choose one.
  if (gVerboseFunction==2) {
      s->init(pretty()); // pretty chosen, obliterate others...
  }
#endif

}

void hash_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);

  s->pushf("{");

    PWord_t   PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, obj->_judyS->_judyS, Index);       // get first string
    char buf[BUFSIZ];

    while (PValue != NULL)
    {
      bzero(buf,BUFSIZ);

      llref* lre = (llref*)(*PValue);
      l3obj* ele = lre->_obj;

      s->pushf("%s: ",Index);
      if (ele) {
        to_string(ele, s, aft,stoppers);
      } else {
        s->pushf("0x0%s",aft ? aft : "");
      }
      JSLN(PValue, obj->_judyS->_judyS, Index);   // get next string
    }
    s->pushf("}%s",aft ? aft : "");
}

void obj_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  if (!obj) return;

  if ((long)stoppers == -1) {
      // disable stopset functionality with the -1 value.
  } else if (stoppers == 0) { 
    stopset_clear(global_stopset);
    stoppers = global_stopset;
    stopset_push(stoppers,(tyse*)obj);
  } else {
      if (obj_in_stopset(stoppers, (tyse*)obj)) return;
      stopset_push(stoppers,(tyse*)obj); // only print it once.
  }

  assert(obj->_type == t_obj);

  objstruct* os = (objstruct*)obj->_pdb;
  s->pushf("(obj classname:'%s' objname:'%s' ", os->classname, os->objname);

  for (uint i = 0; i < os->mem_dtor.size(); ++i) {
    printf("mem_dtor[%02d]='%s' ", i, os->mem_dtor[i]->_varname);
  }
  hash_to_string(obj,s,aft,stoppers);
  s->pushf(")%s",aft ? aft : "");
}


void symvec_to_string(l3obj* symvec, l3path* s, const char* aft, stopset* stoppers) {

  assert(symvec);
  assert(symvec->_type == t_syv);

  // no stoppers, since they seem to mess things up.

  long N = symvec_size(symvec);
  l3obj* ptr = 0;
  l3path sym_name;
  symbol* sym = 0;

  s->pushf("[symvec (size %ld) ", N);

  for (long i = 0; i < N; ++i) {
    sym = symvec_get_by_number(symvec, i, &sym_name, &ptr);
    
    s->pushf("symbol{ %s } -> (%s %p ser# %ld): '",sym_name(),ptr->_type, ptr, ptr->_ser);
    to_string(ptr,s,aft,0);
    if (i < (N-1)) { s->pushf("', "); } else { s->pushf("'"); }
  }
  s->pushf("]");

}

void dq_to_string(l3obj* dqobj, l3path* s, const char* aft, stopset* stoppers) {

  assert(dqobj);
  assert(dqobj->_type == t_dsq);

  long N = dq_len_api(dqobj);
  l3obj* ptr = 0;
  l3path key;

  s->pushf("[dq (size %ld) ", N);

  for (long i = 0; i < N; ++i) {
      ptr = dq_ith_api(dqobj, i, &key);
      if (ptr) {
          s->pushf("lnk{ %s } -> (%s %p ser# %ld): '",key(), ptr->_type, ptr, ptr->_ser);
          to_string(ptr,s,aft,stoppers);
          if (i < (N-1)) { s->pushf("', "); } else { s->pushf("'"); }

      } else {
          s->pushf("lnk{ %s } -> (0x0)",key());
      }
  }
  s->pushf("]");


}


void tdo_to_string(l3obj* tdoobj, l3path* s, const char* aft, stopset* stoppers) {
  assert(tdoobj);
  assert(tdoobj->_type == t_tdo);

  tdopout_struct* tdop1 = (tdopout_struct*)(tdoobj->_pdb);
  if (tdop1->_parsed == 0) return; // nothing
  
  s->pushf("%s", tdop1->_output_stringified_compressed.c_str());
}


void sexp_to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);
  assert(obj->_type == t_sxp);  

  l3path stmp;
  sexpobj_text(obj, &stmp);

  s->pushf("%s", stmp());
}



void to_string(l3obj* obj, l3path* s, const char* aft, stopset* stoppers) {
  assert(obj);

  DV(p(stoppers));
  if ((long)stoppers == -1) {
      // disable stopset functionality with the -1 value.
  } else if (stoppers == 0) { 
    stopset_clear(global_stopset);
    stoppers = global_stopset;
    stopset_push(stoppers,(tyse*)obj);
  } else {
      if (obj_in_stopset(stoppers, (tyse*)obj)) return;
      stopset_push(stoppers,(tyse*)obj); // only print it once.
  }

#define pcase(x) if (obj->_type == x) 
  pcase(t_vvc) { ptrvec_to_string(obj,s,aft,stoppers); }
  pcase(t_syv) { symvec_to_string(obj,s,aft,stoppers); }
  pcase(t_dou) { double_to_string(obj,s,aft,stoppers); }
  pcase(t_str) { string_to_string(obj,s,aft,stoppers); }
  pcase(t_lit) { literal_to_string(obj,s,aft,stoppers);}
  pcase(t_fun) { fun_to_string(obj,s,aft,stoppers);    }
  pcase(t_env) { hash_to_string(obj,s,aft,stoppers);   }
  pcase(t_obj) { obj_to_string(obj,s,aft,stoppers);    }
  pcase(t_tru) { bool_to_string(obj,s,aft,stoppers);   }
  pcase(t_fal) { bool_to_string(obj,s,aft,stoppers);   }
  pcase(t_sxp) { sexp_to_string(obj,s,aft,stoppers);   }
  pcase(t_dsq) { dq_to_string(obj,s,aft,stoppers);     }
  pcase(t_tdo) { tdo_to_string(obj,s,aft,stoppers);    }
}


