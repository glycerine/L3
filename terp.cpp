//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#include "jtdd.h"
#include "dv.h"
#include "compiler.h"
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <math.h>
#include <openssl/sha.h>
#include <valgrind/valgrind.h>
#include "l3obj.h"
#include "autotag.h"

#include "qexp.h"

#include "objects.h"
#include "terp.h"

#include "judydup.h"
#include <dlfcn.h>
#include "ut.h"
#include "tostring.h"
#include <stdio.h>
#include <editline/readline.h>
#include <histedit.h>
#include "symvec.h"
#include "dynamicscope.h"
#include "tyse.h"
#include "l3link.h"
//#include "umem.h"

#ifdef _USE_MTRACE
  #ifndef _MACOSX
    #include "mcheck.h"
  #endif
#endif // _USE_MTRACE 

// WTF doesn't including tyse.h just define this?
typedef  std::map<tyse*,long>    gtyseset;

#include "l3ts.pb.h"

#ifndef  _DMALLOC_OFF
#include "sermon.h"
#endif

#include "codepoints.h"
#include "memscan.h"
#include "l3ts_common.h"
#include "l3mq.h"
#include "l3pratt.h"

/////////////////////// end of includes

using std::vector;
using std::string;

// valgrind

#ifndef NVALGRIND
#  include <valgrind/memcheck.h>
#else
#  define VALGRIND_CREATE_MEMPOOL(...)
#  define VALGRIND_MEMPOOL_ALLOC(...)
#  define VALGRIND_MEMPOOL_FREE(...)
#  define VALGRIND_CREATE_BLOCK(...)
#  define VALGRIND_MAKE_MEM_NOACCESS(...)
#  define VALGRIND_MAKE_MEM_DEFINED(...)
#  define VALGRIND_MAKE_MEM_UNDEFINED(...)
#  define VALGRIND_CHECK_MEM_IS_DEFINED(...)
#  define VALGRIND_DISCARD(...)
#endif

void mtrace_on();

// add_history : in libedit. Add a line to history. See <editline/readline.h>
//
// this is using the readline compatibility interface...
// int add_history(const char *line);
//int		 rl_initialize(void);
//void		 clear_history(void);
//int		 rl_complete(int, int); // for tab completion, not cleanup.

// vs this is from el_* editline functions directly: histedit.h  see man editline(3)
//EditLine	*el_init(const char *, FILE *, FILE *, FILE *);
//void		 el_reset(EditLine *);
//void		 el_end(EditLine *); // how we need to cleanup.



// macros



// repl is it's own REPL function, just after main() now.
l3obj* repl(FILE* ifp, ustaq<sexp_t>* local_sexpstaq, l3obj* env, Tag* owner);


l3obj* repl_qtree(FILE* fp, ustaq<qtree>* local_qtreestaq);


//void fill_retval_with_copy_of(l3obj* src_dont_change, l3obj** retval);
L3FORWDECL(fill_retval_with_copy_of)

// globals

int    g_argc=0;
char** g_argv=0;
l3path g_dash_e_cmd; // if g_dash_e_cmd.len() > 0 then we have a -e command.
l3path g_cmdline;
BOOL   g_have_dash_e =FALSE;
BOOL   g_have_dash_repl   =FALSE;

int gVerboseFunction = 2;

Tag* glob = 0;

// details/pretty printing
int gUglyDetails = 0;

// for initializing the types
quicktype_sys*  qtypesys = 0;

// history of user issued commands
cmdhistory*   histlog = 0;

// mlog hast to be initialized by global_tyse_set
MLOG_ONLY( jmemlogger* mlog = 0; ) // defaults to the current directory, file will be memlog_<pid>.

// global_tyse_set has to be initialized before sexpstaq
gtyseset*     global_tyse_set = 0;

// ustaq<sexp_t>* sexpstaq = 0;

// the global serial number based memory monitor.
#ifndef  _DMALLOC_OFF
ser_mem_mon*    gsermon = 0;
#endif


l3obj* exception_stack = 0;

// optionally used by gsermon
codepoints* leaks = 0; // loads and makes easy to query file "./leakpoints"

// scan memory during sexp stuff to track when/where leak happened
memscanner* mscan = 0;


struct cmd_value_history {

  vec_obj             _lastvals;
  long                _maxsize;
  long                _nextnum;
  l3path              _nextstr;

  void reset() {
    _nextnum=0;
    _lastvals.clear();
    _nextstr.init("");
  }

  long  nextnum() { return _nextnum++; }
  char* nextstr() { _nextstr.reinit("%03ld", nextnum()); return _nextstr(); }
    
  cmd_value_history() : _nextnum(0) {}

  void delete_oldest() {

    // bad idea: then we also delete values that are in
    //  the environment that do have names!!
    /*
    if (!_lastvals.size()) return;
    l3obj* tmp = _lastvals[0];
    if (tmp) {
      tmp->_mytag->del(tmp);
      _lastvals[0]=0;
    }
    */
  }

  void push(l3obj* v) {    
    _lastvals.push_back(v);
  }
};

// global... so it can be cleared as well.
cmd_value_history*  repl_value_history_l3obj = 0;


// infix mode?
bool _global_infixmode = false;

qtreefactory* qtfac = 0;

PermaText*          permatext = 0;

// new version
struct global_init_in_order {

    MLOG_ONLY(   jmemlogger*      _mlog; )
    codepoints*         _leaks;
#ifndef  _DMALLOC_OFF
    ser_mem_mon*        _gsermon;
#endif

    gtyseset*           _global_tyse_set;

    quicktype_sys*      _qtypesys;

    serialnum_f*        _serialfactory;

    cmdhistory*         _histlog;
    stopset*            _global_stopset;
    cmd_value_history*  _repl_value_history_l3obj;
    memscanner*         _mscan;
    PermaText*          _permatext;

    global_init_in_order()
    {
        mtrace_on();

        permatext = _permatext = new PermaText;

        MLOG_ONLY( mlog            = _mlog            = new jmemlogger(0);)
        leaks           = _leaks          = new codepoints;

#ifndef  _DMALLOC_OFF
        gsermon         = _gsermon         = new ser_mem_mon;
#endif
        global_tyse_set = _global_tyse_set = new gtyseset;
        qtypesys        = _qtypesys        = new quicktype_sys;

        serialfactory   = _serialfactory   = new serialnum_f;

        histlog         = _histlog = new cmdhistory;
        global_stopset =  _global_stopset = new stopset;
        mscan          = _mscan = new memscanner;

        repl_value_history_l3obj  =  _repl_value_history_l3obj = new cmd_value_history;

        _global_infixmode = false;
        
        qtfac = 0; // just want it properly zeroed until we use it.
    }
    
#define DELGLOBAL(glo) delete _##glo; glo=0; _##glo=0;

    ~global_init_in_order()
    {
        _repl_value_history_l3obj->_lastvals.clear();
        DELGLOBAL(repl_value_history_l3obj);
        DELGLOBAL(mscan);

        _global_stopset->_stoppers.clear();
        DELGLOBAL(global_stopset);

        DELGLOBAL(histlog);

        DELGLOBAL(serialfactory);
        DELGLOBAL(qtypesys);
        DELGLOBAL(global_tyse_set);

        MLOG_ONLY(  
              gsermon->off(); 
              DELGLOBAL(gsermon); 
              )
        DELGLOBAL(leaks);

        DELGLOBAL(permatext);

        MLOG_ONLY(
                  l3path memlogpath(_mlog->get_mypath());
                  DELGLOBAL(mlog);
                  l3path memcheck_cmd(0,"./smcheck %s",memlogpath());
                  DV(printf("doing ... %s\n",memcheck_cmd());
                     system(memcheck_cmd()));
                  )

   } // end ~global_init_in_order
};


global_init_in_order* globi = 0;


void  delete_all_from_sexpstack(ustaq<sexp_t>* sxs) {
    assert(sxs);
    while (sxs->size()) {
        sexp_t* sx = sxs->pop_front();
        destroy_sexp(sx);
    }
    sxs->clear();
}

// signal that even sealed objects can be deleted.
long  global_terp_final_teardown_started = 0;

// hold the result of the last cmd, and get's gc at the top of the next cmd.
l3obj* toplevel_lastline_obj = 0;

l3obj* main_env = 0;

SHA_CTX shactx; // global


// path to memory log
l3path memlogpath;

unittest utest;
long  unittest_max = 0; // start testing if over 0, and go to test_max.
long  unittest_cur = 0; // reflects current test number
long  unittest_last = 0; // reflects last test run in a ut cmd

char linebuf[BUFSIZ];
char linebuf_after_colon_swap[BUFSIZ];


l3path cur_object;
l3path cur_method;
l3path tmpbuf;


l3path global_scope;
pid_t pid = getpid();

l3path home;

l3path history_path;

// the global nil and global true objects.
l3obj* gnil = 0;
l3obj* gtrue = 0;

l3obj* gnan = 0;
l3obj* gna = 0;


// global to remember current test we are on.
// the global current test name, ctest.
l3path ctst;
long   ctst_calls = 0; // number of times ctest called.

// statics

bool global_quit_flag = false; // set this when time to cleanup and quit.


//
//
//
L3METHOD(fresh_copy)
    LIVEO(env);
    if (arity != 1) {
        printf("error: cp needs one argument, the object to copy\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }

    qqchar v = exp->ith_child(0)->val();
    llref* innermostref = 0;
    l3obj* found = RESOLVE_REF(v,env,AUTO_DEREF_SYMBOLS, &innermostref,0,v,UNFOUND_RETURN_ZERO);

    if (!found) {
        std::cout << "error: cp could not find object '" << v << "' to copy.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }



    LIVEO(found);
    LIVET(found->_owner);
    Tag* found_mytag = found->_mytag;
    if (found_mytag) {
       LIVET(found_mytag);
    }
    // sealed objects are static, and not copied: only referred to by reference
    if (is_sealed(found)) {
        *retval = found;
        return 0;
    }

    if (found_mytag && pred_is_captag(found_mytag, found)) {
        // set arity to 0 to indicate starting depth for recursion detection
        found->_mytag->dfs_copy(obj, 0, exp, env, retval, found_mytag, curfo, etyp, retown, ifp);
    } else {
        deep_copy_obj(found,arity,0, L3STDARGS_ENV);
    }

L3END(fresh_copy)

//
// version of the above where we copy obj and ignore exp.
//
L3METHOD(fresh_copy_of_obj)

    l3obj* found = obj;
    LIVEO(found);
    LIVET(found->_owner);
    Tag* found_mytag = found->_mytag;
    if (found_mytag) {
       LIVET(found_mytag);
    }
    // sealed objects are static, and not copied: only referred to by reference
    if (is_sealed(found)) {
        *retval = found;
        return 0;
    }

    if (found_mytag && pred_is_captag(found_mytag, found)) {
        // set arity to 0 to indicate starting depth for recursion detection
        found->_mytag->dfs_copy(0, 0, 0, env, retval, found_mytag, curfo, etyp, retown,ifp);
    } else {
        deep_copy_obj(found,0,0, L3STDARGS_ENV);
    }

L3END(fresh_copy_of_obj)


// (owns   owned?   owner_to_test_if_it_owns_owned_directly)
//
//  or, if arity = -1, then for api call:
//     obj   = owned?
//     curfo = testowner
//
L3METHOD(owns)
{
    l3obj*  vv = 0;
    l3obj*  owned = 0;
    l3obj*  testowner = 0;

    if (arity == -1) {
        if (!obj && !curfo) {
            printf("error: owns call via api needs two arguments, (owns  obj=owned?  curfo=testowner)\n");
            l3throw(XABORT_TO_TOPLEVEL);
        }
        LIVEO(obj);
        LIVEO(curfo);
        owned     = obj;
        testowner = curfo;

    } else {

        if (arity != 2) {
            printf("error: owns needs two arguments, (owns  owned?  testowner)\n");
            l3throw(XABORT_TO_TOPLEVEL);
        }
        
        eval_to_ptrvec(L3STDARGS);
        vv = *retval;

        ptrvec_get(vv,0,&owned);
        ptrvec_get(vv,1,&testowner);
    }


    if (testowner->_type == t_lit) {
      l3path s;
      string_get(testowner,0,&s);
      printf("error: could not resolve symbol '%s' in owns.\n",s());
      l3throw(XABORT_TO_TOPLEVEL);
    }

        
    BOOL b_owned_captag = pred_is_captag_obj(owned);
    
    if (testowner->_mytag &&
           ( (owned->_owner == testowner->_mytag
              ||  (b_owned_captag && owned->_owner->parent() == testowner->_mytag)))) {
                        *retval = gtrue;
    } else {
        // owned may be deeply nested, so do a rigorous nca check before
        // concluding not owned.
        
        Tag* top_tag = testowner->_mytag ? testowner->_mytag : testowner->_owner;
        Tag* bot_tag = owned->_owner;
        assert(bot_tag);
        
        Tag* the_nca = compute_nca(top_tag, bot_tag,  env,owner);
        if (the_nca == top_tag) {
            *retval = gtrue;
            
        } else {
            *retval = gnil;
            
        }
    }

    // clean up!! don't return above early or this next line won't happen.
    // thats why we wrap in a eval_to_ptrvec: because any throws are
    // caught there and cleaned up at that level if need be, and if that
    // happens then we never even see the stuff after eval_to_ptrvec.
    if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
}
L3END(owns)


//
// return a ptrvec, whose tag owns all the values pointed to 
//  by the ptrvec.  Thus destroying that one object will cleanup
//  all the tmp values.
//
// The ptrvec contains the eval() values from exp.

L3METHOD(eval_to_ptrvec)

  arity = num_children(exp);
  if (arity == 0) return 0;
  
  assert(retval);
  l3path eval_to_ptrvec_tmptag_name(0,"eval_to_ptrvec_tmptag_name");

  volatile l3obj* vv = 0;
  volatile l3obj* tmpobj = 0;
  volatile Tag* tmptag = 0;


  // debugging aid: see what call we are in easily.
  l3path   exp_printed(exp);
  DV(printf("we are procesing: '%s'\n",exp_printed()));

  sexp_t* nex = 0;  // first_child(exp);
  volatile bool done_with_code = false;

  // default value in case of throw
  *retval = gnil;

  XTRY
     case XCODE:
         // for more lasting values: the vv->_mytag
         // for very temp values:    the tmptag

         make_new_ptrvec(0, -1, 0, env, (l3obj**)&vv, owner, curfo, t_vvc,retown,ifp);
         tmptag =  new Tag(STD_STRING_WHERE.c_str(), retown, eval_to_ptrvec_tmptag_name(),(l3obj*)vv); // allocate 2nd, tmptag, so we don't muddy up vv->_mytag

         for (long i = 0; i < arity; ++i) {
             nex = exp->ith_child(i);

             tmpobj = 0;

             // give the tmptag as owner, so all the intermediates can be deleted.

             eval(0, -1, nex, (l3obj*)vv, (l3obj**)&tmpobj, (Tag*)tmptag, curfo, etyp, (Tag*)(vv->_mytag),ifp); // I think we need to preserve curfo for env searching.
             ptrvec_set((l3obj*)vv, i, (l3obj*)tmpobj);
         }

         *retval = (l3obj*)vv;
         // this is false now that vv is allocated as a captag: assert(vv->_owner == retown);

         done_with_code = true;
     break;

     case XFINALLY:

           ((Tag*)tmptag)->tag_destruct((l3obj*)tmptag,arity,exp, L3STDARGS_ENV);
           delete tmptag;


         //cleanup:
         if (!done_with_code) {
             // dtor for ptrvec will now cleanup tag etc.
             if (vv) { 
                 generic_delete((l3obj*)vv, L3STDARGS_OBJONLY); 
             }
             *retval = gnil;
         }
     break;
  XENDX

L3END(eval_to_ptrvec)



// if a new allocation, use the retown tag
L3METHOD(prep_double_returntype)
   
   double* dval = (double*)&obj;
   if (*retval && (*retval)->_type == t_dou) {
     double_set(*retval,0,*dval);
     return 0;
   }

   l3path nm(0,"return_val_double_ontag_%p", retown);
   l3obj* new_double = make_new_double_obj(*dval, retown, nm());

   //  fill_retval_with_copy_of(new_double, retval);
   // why make a copy, when we just allocated a new double???
// fill_retval_with_copy_of((l3obj*)new_double, -1,0,env,retval,owner,curfo,etyp);

   *retval = new_double;

L3END(prep_double_returntype)


void quiet() {
  enDV=0;
  printf("quiet mode selected. No debugging statements will be shown.\n");
}

void loud() {
  enDV=1;
  printf("loud mode selected. Debugging statements will be shown.\n");
}

//
// 3 states for function display
// gVerboseFunction == 0 : don't show function definitions
// gVerboseFunction == 1 : show defn on one line
// gVerboseFunction == 2 : show defn pretty printed
//
void function_defn_quiet() {
    if (gVerboseFunction > 0) {
        gVerboseFunction--;
        printf("function definitions quieter: mode %d selected.\n",gVerboseFunction);
    } else {
        printf("already as quiet as can be on funtion definitions.\n");
    }
}

void function_defn_loud() {
    if (gVerboseFunction < 3) {
        gVerboseFunction++;
        printf("function definitions louder: mode %d selected.\n",gVerboseFunction);
    } else {
        printf("already as loud as can be on funtion definitions.\n");
    }

}

// 'pretty'
void print_display_pretty() {
    gUglyDetails--;
    if (gUglyDetails < 0) {
        gUglyDetails = 0;
    }
    if (gUglyDetails) {
        printf("ugly details level %d selected.\n",gUglyDetails);
    } else {
        printf("pretty printing chosen.\n");
    }
}

// 'ugly'
void print_display_ugly() {
    gUglyDetails++;
    printf("ugly details level %d selected.\n",gUglyDetails);
}


L3STUB(system_ls_dtor)

L3STUB(system_ls_ctor)

L3METHOD(system_ls_trybody)

  DV(printf("** BODY   system_ls_trybody  (obj= %p, env= %p)\n",obj,env));

  char buf[PATH_MAX+1];
  bzero(buf,PATH_MAX+1);
  sprintf(buf,"%s",cur_object());
  printf("\n======= (persistent default) ptag  : %s   ======\n\n",buf+global_scope.len()-1);

  sprintf(buf,"cd %s; find . -print | grep -v main",cur_object());

  system(buf);

  printf("\n\n======= (auto default) current method : %s    ======\n\n",cur_method());

  l3path cmd("ls -1 | grep -v ");
  cmd.pushf("%p",obj);
  system(cmd());

L3END(system_ls_trybody)




void simpler_ls_cmd_without_mallocing() {

  printf("\n======= (persistent default) ptag  : %s   ======\n\n",cur_object()+global_scope.len()-1);
  l3path cmd(0,"cd %s; find . -print | grep -v -e \"[.]/main\"",cur_object());
  system(cmd());

  printf("\n\n======= (auto default) current method : %s    ======\n\n",cur_method());
  cmd.reinit("ls -ultgGhF --time-style=+ | tail -n +2 | sed 's/.................\\(.*\\)/\\1/'");
  system(cmd());
}



////////// terp code

int call_repl_in_trycatch(FILE* fp, ustaq<sexp_t>** pp_sexpstaq, bool loop_until_quit, l3obj* env, Tag* owner);


#if 0

/**
 * look up an entry and purge it -- not done yet
 */
void purge(char *varname, l3obj *d, llref* llr) {
  /* find an entry and purge it */
    delete_key_from_hashtable(d,varname,llr);
}

/**
 * purge all entries in the dictionary and free the dictionary itself.
 */
void purge_all(l3obj *d) {
  assert(d);
  clear_hashtable(d);
}
#endif




#if 0 // move up above

struct cmd_value_history {

  std::vector<l3obj*> _lastvals;
  long                _maxsize;
  long                _nextnum;
  l3path              _nextstr;

  void reset() {
    _nextnum=0;
    _lastvals.clear();
    _nextstr.init("");
  }

  long  nextnum() { return _nextnum++; }
  char* nextstr() { _nextstr.reinit("%03ld", nextnum()); return _nextstr(); }
    
  cmd_value_history() : _nextnum(0) {}

  void delete_oldest() {

    // bad idea: then we also delete values that are in
    //  the environment that do have names!!
    /*
    if (!_lastvals.size()) return;
    l3obj* tmp = _lastvals[0];
    if (tmp) {
      tmp->_mytag->del(tmp);
      _lastvals[0]=0;
    }
    */
  }

  void push(l3obj* v) {    
    _lastvals.push_back(v);
  }
};

// global... so it can be cleared as well.
cmd_value_history*  repl_value_history_l3obj = 0;
#endif


// ============= try_exp_dispatch

L3METHOD(try_exp_dispatch)

   // obj has the function to run.

   assert(obj->_type == t_fun || obj->_type == t_clo || obj->_type == t_lda);

  //l3obj* try_exp_dispatch(l3obj* obj, sexp_t* exp, l3obj* env,l3obj** retval)

  volatile l3obj* args_captag = 0; // the t_cal callobject
  volatile l3obj* fill = 0;
  volatile Tag*  try_disp_tag = 0;
//  volatile Tag* orig_top = defptag_get();
//  volatile l3obj* orig_curfo = curfo; // make volatile version so it's sure to be there.

   // if sn is wiped out back to zero, then we know it's already cleaned up...
   volatile long   captag_sn = 0;

  // debugging aid: see what call we are in easily.
  l3path   exp_printed(exp);
  l3path basenm("captag_try_exp_disp");

  XTRY
     case XCODE:


         // remind me: why did we go to a captag, instead of the simpler tag as commented out above...? i.e. why this:
         // perhaps because each tag needed a captain, and we didn't have any object for the tag to point to that
         //  would for sure not create a cycle...?maybe?
         make_new_captag((l3obj*)&basenm ,sizeof(actualcall),exp,env,(l3obj**) &args_captag , owner,0,t_cap,retown,ifp);

         captag_sn = args_captag->_ser;
         try_disp_tag = (volatile Tag*)(args_captag->_owner);
         assert(args_captag->_owner == args_captag->_mytag);







         fill_in_preallocated_new_callobject((l3obj*)args_captag,arity,exp,env,(l3obj**) &fill, (Tag*)try_disp_tag, curfo,t_cal,(Tag*)try_disp_tag,ifp);

#if 0  // not sure we can replicate this now...

         // because we are passing env as curfo to universal_object_dispatcher, we also push curfo here to make 
         // sure it's available on the dynamic env stack.
         if (curfo) {
             global_env_stack.pop_front();
             global_env_stack.push_front(curfo);
             assert(global_tag_stack.size() == global_env_stack.size());     // try to find imbalance as soon as it happens...    
         }

         assert(global_tag_stack.size() == global_env_stack.size());     // try to find imbalance as soon as it happens...    
#endif

         // env's owner gets transfered any @> objects.
         // holds the args goes in as new env.
         // note that we plug in try_disp_tag as the retown here.
         //     universal_object_dispatcher(obj,arity,exp, (l3obj*)args_captag, retval, env->_owner, env,etyp, (Tag*)try_disp_tag);
         universal_object_dispatcher(obj,arity,exp, 
                                   (l3obj*)args_captag, /* env*/ 
                                   retval,       /* retval*/
                                   env->_owner,  /* owner */
                                   env,          /* curfo */
                                   etyp,         /* etyp */
                                     //env->_owner        /* retown */ // or retown?
                                   retown,ifp);

     break;
     case XFINALLY:


           // the only sane thing is to have universal_object_dispatch *not* do any cleanup; we will do that, since
           // we did the allocation in the first place.

     // sanity: if captag is already gone, don't delete again.
     if (args_captag->_type && captag_sn == args_captag->_ser) { 
         LIVEO(((l3obj*)args_captag));
         delete_captag((l3obj*)args_captag,L3STDARGS_OBJONLY); 
     } else {
       assert(0); // wierd that it is already gone?!?!?
     }
     break;
  XENDX


L3END(try_exp_dispatch)

  //
  // call dfs_destruct on obj->_owner
  //
L3METHOD(delete_captag)

    LIVEO(obj);
    // wrong:    Tag* captag_tag = obj->_owner;
    Tag* captag_tag = obj->_mytag;

    int need_del_in_owner = (obj->_mytag != obj->_owner);
    Tag* obj_owner = obj->_owner;

    //   invoke_optional_dtor(L3STDARGS); // think we need to pass env = obj here, so dtor knows its env.
    invoke_optional_dtor(obj,arity,exp,   env,retval,owner,   curfo,etyp,retown,ifp);

    long unseen = captag_tag->dfs_been_here();
    l3obj* fakeret = 0;
    captag_tag->dfs_destruct(obj,
			     unseen, // info here
			     exp,

                 env,
                 &fakeret, // retval: have to be sure there is someething here, or eval will exit too early.
			     captag_tag, // info here

			     curfo,etyp,retown,ifp);

    delete captag_tag;

    if (need_del_in_owner) {
        // gotta give a good owner and retown here, or else the LIVET entry checks will fail. Hence we give obj_owner.
        obj_owner->del_owned(obj,-1,0,         // this one okay: it does have to be a del_owned and not a generic_delete :-)
                             0,0,obj_owner,
                             0,0,obj_owner,ifp);
    }

L3END(delete_captag)

// typedef double (*bin_op_double)(double a, double b); // in objects.h

double bin_op_div(double a, double b) { return a / b; }
double bin_op_mul(double a, double b) { return a * b; }
double bin_op_add(double a, double b) { return a + b; }
double bin_op_sub(double a, double b) { return a - b; }


// ============= arithmetic_with_tmp_tag
//

L3METHOD(arithmetic_with_tmp_tag)
{
//void arithmetic_with_tmp_tag(long arity, l3obj* obj, sexp_t* exp, l3obj* env, l3obj** retval)

  volatile Tag*  tmptag = 0;
  volatile l3obj* back = 0;
//  volatile Tag*  orig_top = defptag_get();

  l3path arith_tmptag_name(0,"arith_tmptag_name");
  assert(arity == exp->nchild());
  sexp_t* nex  =  ith_child(exp,1);

  const qqchar v = first_child(exp)->val();

  bin_op_double op = 0;

  if (v.strcmp("/") == 0) { op = &bin_op_div; }
  else if (v.strcmp("*") == 0) { op = &bin_op_mul; }
  else if (v.strcmp("+") == 0) { op = &bin_op_add; }
  else if (v.strcmp("-") == 0) { op = &bin_op_sub; }
  else { 
    assert(0);  } // should have checked for +,-,* or / before dispatching to here.

  double res = 0;

  XTRY
     case XCODE:
         // have to give it its arguments... and it's default tag
         tmptag =  new Tag(STD_STRING_WHERE.c_str(), retown, arith_tmptag_name(),0);
//         defptag_push((Tag*)tmptag, curfo);

         // run along the values that are our operands, and accumulate our
         // arithmetic result.
         back = make_new_double_obj(NAN, (Tag*)tmptag, "tmp_double_for_arithmetic_eval");

         // first argument doesn't get op applied to it.
         eval(0, -1,nex, env, (l3obj**)&back, (Tag*)tmptag, curfo, t_dou, (Tag*)tmptag,ifp);
         res = double_get((l3obj*)back,0);

         // 2nd and every arg after first does:
         for (long i = 2; i < arity; ++i ) {
             nex = exp->ith_child(i);

             eval(0,-1,nex, env, (l3obj**)&back, (Tag*)tmptag, curfo, t_dou, (Tag*)tmptag,ifp);

             res = op(res,double_get((l3obj*)back,0));

         }

     double_set((l3obj*)back,0,res);
         //fill_retval_with_copy_of((l3obj*)back, retval);

     break;
     case XFINALLY:

       ((Tag*)tmptag)->tag_destruct((l3obj*)tmptag,arity,exp, L3STDARGS_ENV);
       delete tmptag;
      break;
  XENDX
}
L3END(arithmetic_with_tmp_tag)


#if 0 // old version, crashing b/c tmp tag has no captain. see below for new version
L3METHOD(assert_with_tmp_tag)

  volatile Tag*   tmptag = 0;
  volatile l3obj* tmpobj_for_assert_values = 0;
//  volatile Tag*  orig_top = defptag_get();

  l3path assert_tmptag_name(0,"assert_tmptag_name");
  sexp_t* nex  =  ith_child(exp,1);
  *retval = gtrue; // default value, because otherwise we crash! =)

  XTRY
     case XCODE:
         // have to give it its arguments... and it's default tag
         tmptag =  new Tag(STD_STRING_WHERE.c_str(), retown, assert_tmptag_name(),0);


         tmpobj_for_assert_values = 0; // make_new_double_obj(NAN, (Tag*)tmptag, "tmp_for_assert");
         // delme:         tmpres                   = make_new_double_obj(NAN, (Tag*)tmptag, "tmpres_for_assert");

         for (long i = 1; i < arity; ++i) {
             assert(nex);
             eval(0,-1,nex,env, (l3obj**)&tmpobj_for_assert_values, (Tag*)tmptag, curfo, etyp, retown,ifp);

             DV(
                printf("in assert_with_tmp_tag, the %ld-th value is:\n",i-1);
                print((l3obj*) tmpobj_for_assert_values,"    ",0);
                );

             if (! is_true((l3obj*)tmpobj_for_assert_values,0)) {
                 l3path sexptxt(nex);
                 l3path msg(0,"fatal error: assert failed on: '%s'",sexptxt());

                 HLOG_ADD_SYNC(msg());
                 MLOG_ADD_SYNC(msg()); // to  memlog_<pid>
                 
                 assert(0);
                 exit(1);
             }
             nex = nex->next;
         }
     break;
     case XFINALLY:

       pop_to_tag((Tag*)orig_top,false);
       ((Tag*)tmptag)->tag_destruct((l3obj*)tmptag,arity,exp, L3STDARGS_ENV);
       delete tmptag;
      break;
  XENDX

L3END(assert_with_tmp_tag)
#endif // 0


L3METHOD(fill_retval_with_copy_of)

    return deep_copy_obj(L3STDARGS);

L3END(fill_retval_with_copy_of)



// ============= make_and_run_closure
//
// obj (1st param) should hold a bin_op_double function pointer

L3METHOD(make_closure)

  make_new_obj("closure", "make_closure_output", retown, 0, retval);
  l3obj* nobj = *retval;
  objstruct* os = (objstruct*)nobj->_pdb;
  os->op = (bin_op_double)obj;

L3END(make_closure)

// rough correspondence:
//
//  clo = unix pipeline component
//
//  data = stdin (also env = env vars?)
//
//  *retval = stdout
//          exception mech = stderr
//

L3METHOD(run_closure)

  l3path expstring(exp);
  l3obj* clo = curfo;
  l3obj* data = obj;
//  volatile Tag*  orig_top = defptag_get();
  volatile l3obj* nextarg = 0;
  double dtmp = 0;

  assert(clo->_type == t_obj);
  objstruct* os = (objstruct*)clo->_pdb;

  bin_op_double op = os->op;

  volatile l3obj* back = 0;
  double    res = 0;
  long N = 0;

  XTRY
     case XCODE:
         // have to give it its arguments... and it's default tag
//     defptag_push(clo->_mytag, clo);

     if (data->_type != t_vvc) {
       printf("ptrvec type (t_vvc) expected as data to closure. Instead we had: '%s'.\n", data->_type);
       l3throw(XABORT_TO_TOPLEVEL);
     }
     //     back = make_new_double_obj(NAN, (Tag*)clo->_mytag, "closure_result");

     // assign ownership of the new retval to same owner as the owner of the original retval,
     // because owner has gotten set to a tmp tag for any tmp values that get generated. Only
     // retown has the true owner  of *retval, which should be preserved for any replacement of *retval.
     back = make_new_double_obj(NAN, retown, "closure_result");
     
     N = double_size(data);
     if (N <= 0) {
       
       printf("no data found to apply closure to.\n");
       l3throw(XABORT_TO_TOPLEVEL);     
       
     } else if (N < 2) {
       
       printf("currently closures need at least two data values to compute on.\n");
       l3throw(XABORT_TO_TOPLEVEL);         
       
     } else {
       
         if (data->_type != t_vvc) {
             l3path data_canpath;
             llref* data_ref = nextarg->_owner->find((l3obj*)nextarg);
             llref_get_canpath(data_ref, &data_canpath);

             printf("error in run_closure: data->_type for '%s' was not t_vvc. Unsupported at the moment.\n",data_canpath());
             l3throw(XABORT_TO_TOPLEVEL);
         }

         ptrvec_get(data, 0, (l3obj**)(&nextarg));

         // for diagnostics, figure out the canonical path of this referenced object...
         l3path canpath;
         llref* ref = nextarg->_owner->find((l3obj*)nextarg);
         llref_get_canpath(ref, &canpath);

         if (nextarg->_type == t_lit) {
             printf("error in run_closure: argument '%s' was t_lit (literal) in expression '%s'."
                    " Unbound variable detected. Aborting.\n",
                    canpath(),
                    expstring());
             l3throw(XABORT_TO_TOPLEVEL);
         }

         if (nextarg->_type != t_dou) {
             printf("error in run_closure: argument '%s' was not numeric. Unsupported at the moment.\n",canpath());
             l3throw(XABORT_TO_TOPLEVEL);
         }
         res = double_get((l3obj*)nextarg, 0);

         for (int i = 1; i < N; ++i) {
             ptrvec_get(data, i, (l3obj**)(&nextarg));

             if (0==nextarg) {
                 printf("error in run_closure: argument # %d was empty.\n",i);
                 l3throw(XABORT_TO_TOPLEVEL);
             }

             if (nextarg->_type != t_dou) {
                 printf("error in run_closure: argument # %d was not a double as expected.\n",i);
                 l3throw(XABORT_TO_TOPLEVEL);
             }
             dtmp = double_get((l3obj*)nextarg, 0);

             res = op(res, dtmp);
         }
       
         double_set((l3obj*)back,0,res);
       *retval = (l3obj*)back;
     }
     break;
     case XFINALLY:

      break;
  XENDX

L3END(run_closure)

#if 0
  // new run_closure that crashes on fact factorial test

// rough correspondence:
//
//  clo = unix pipeline component
//
//  data = stdin (also env = env vars?)
//
//  *retval = stdout
//          exception mech = stderr
//
L3METHOD_TMPCAPTAG(run_closure)

     l3obj* clo = curfo;
     l3obj* data = obj;

     volatile l3obj* nextarg = 0;
     double dtmp = 0;

     assert(clo->_type == t_obj);
     objstruct* os = (objstruct*)clo->_pdb;

     bin_op_double op = os->op;

     volatile l3obj* back = 0;
     double    res = 0;
     long N = 0;

     if (data->_type != t_vvc) {
       printf("ptrvec type (t_vvc) expected as t_vvc data to closure. Instead we had: '%s'.\n", data->_type);
       l3throw(XABORT_TO_TOPLEVEL);
     }

     N = double_size(data);
     if (N <= 0) {
       
       printf("no data found to apply closure to.\n");
       l3throw(XABORT_TO_TOPLEVEL);     
       
     } else if (N <= 2) {
       
       printf("currently closures need at least two data values to compute on.\n");
       l3throw(XABORT_TO_TOPLEVEL);                
     }

   L3TRY_TMPCAPTAG(run_closure, 0)

         // transfer back to retown at the end of the try block, if successful.
         back = make_new_double_obj(NAN, (Tag*)tmp_tag, "closure_result");
     
         ptrvec_get(data, 1, (l3obj**)(&nextarg));

         if (nextarg->_type != t_dou) {
             printf("error in run_closure: argument was not numeric. Unsupported at the moment.\n");
             l3throw(XABORT_TO_TOPLEVEL);         
         }
         res = double_get((l3obj*)nextarg, 0);

         for (int i = 2; i < N; ++i) {
             ptrvec_get(data, i, (l3obj**)(&nextarg));
             assert(nextarg->_type == t_dou);
             dtmp = double_get((l3obj*)nextarg, 0);

             res = op(res, dtmp);
         }
       
         double_set((l3obj*)back,0,res);
         *retval = (l3obj*)back;
         back->_owner->generic_release_to((l3obj*)back, retown,ifp);


L3END_CATCH_TMPCAPTAG(new_run_closure)
#endif



// ============= arithmetic_with_closure
//

L3METHOD(arithmetic_with_closure)
{
  volatile Tag*  tmptag = 0;
  volatile l3obj* vv = 0;
//  volatile Tag*  orig_top = defptag_get();

  l3path arith_tmptag_name(0,"arith_tmptag_name");
  qqchar v;
  if (exp->_headnode) {
      v = exp->headval();
  } else {
      v = exp->val();
  }

  bin_op_double op = 0;

  if (v.strcmp("/") == 0) { op = &bin_op_div; }
  else if (v.strcmp("*") == 0) { op = &bin_op_mul; }
  else if (v.strcmp("+") == 0) { op = &bin_op_add; }
  else if (v.strcmp("-") == 0) { op = &bin_op_sub; }
  else { 
    assert(0);  } // should have checked for +,-,* or / before dispatching to here.


  //  double res = 0;
  l3obj* clo = 0;

  XTRY
     case XCODE:
         // have to give it its arguments... and it's default tag
         tmptag =  new Tag(STD_STRING_WHERE.c_str(), owner, arith_tmptag_name(),0);



         make_closure((l3obj*)op, arity, exp, env, &clo, (Tag*)tmptag, curfo, 0, (Tag*)tmptag,ifp);
         ((Tag*)tmptag)->captain_set(clo);

         // run along the values that are our operands, and accumulate our
         // arithmetic result.

         eval_to_ptrvec(L3STDARGS);
         vv = *retval;

         // curfo gets the function to run
         // obj gets the data.
         run_closure((l3obj*)vv,arity,exp,env,retval,clo->_mytag,clo,0,retown,ifp);

     break;
     case XFINALLY:


       ((Tag*)tmptag)->tag_destruct((l3obj*)tmptag,arity,exp, L3STDARGS_ENV);
       delete tmptag;
       if (vv) { bye(vv, owner); }

      break;
  XENDX
}
L3END(arithmetic_with_closure)


#if 0 // redo with 3 macro tmp_tag
L3METHOD(print_list_to_string)

  arity = num_children(exp);
  volatile Tag*  tmptag = 0;
  volatile l3obj* tmpstring = 0;
  volatile l3obj* presstring = 0;
//  volatile Tag*  orig_top = defptag_get();

  l3path p_tmptag_name(0,"p_tmptag");

  l3path mystring;
  l3path part;
  l3path testget;
  sexp_t* nextstring = 0;
  l3path printed_nextstring;

  XTRY
     case XCODE:
         // have to give it its arguments... and it's default tag
         tmptag =  new Tag(STD_STRING_WHERE.c_str(), owner, p_tmptag_name(),0);


         presstring = make_new_string_obj((char*)"", retown, (char*)"p_string");

         if (arity > 1) {
            nextstring = ith_child(exp,1);

            // for easier diagnostics
            printed_nextstring.reinit(nextstring);
            DV(printf("printed_nextstring is: %s\n",printed_nextstring()));

            while(1) {
            part.init();
            tmpstring  = 0;
            if (0==nextstring) break;

            // evaluate to get a string
            eval(0,-1, nextstring, env, (l3obj**)&tmpstring, (Tag*)tmptag, curfo, t_str, (Tag*)tmptag);

            string_get((l3obj*)tmpstring, &part);
            mystring.pushf("%s", part());
            nextstring = nextstring->next;
            }
         }
         string_set((l3obj*)presstring,mystring());

         string_get((l3obj*)presstring,&testget);

         *retval = (l3obj*)presstring;

      break;
      case XFINALLY:
          pop_to_tag((Tag*)orig_top,false);
          ((Tag*)tmptag)->tag_destruct((l3obj*)tmptag,arity,exp, L3STDARGS_ENV);
          delete tmptag;
      break;
   XENDX

 L3END(print_list_to_string)
#endif

#if 0
// change to the new object specified by envpath
void  push_and_pop_global_tagenv_stacks(objlist_t* penvpath) {

       DV( {
               printf("======= starting : push_and_pop_global_tagenv_stacks called with envpath: \n");
               print_objlist(penvpath);
               printf("======= done with: push_and_pop_global_tagenv_stacks called\n");       
           });
       assert(penvpath->size() > 0);
       objlist_it en = penvpath->end();
       objlist_it be = penvpath->begin();

       l3obj* o = 0;
       for(; be != en; ++be) {
           o = *be;
           DV(print(o," p&p next object is: ",0));
           if (0==o) {
               global_env_stack.pop_front();
               global_tag_stack.pop_front();
               global_tag_stack_loc.pop_front();               

           } else {
               global_env_stack.push_front(o);
               assert(o->_mytag);
               global_tag_stack.push_front(o->_mytag);
               global_tag_stack_loc.push_front(STD_STRING_WHERE);
           }
       }
}
#endif



 // cd obj : make the named object the
 //  default place for method calls, method definitions.
 //  and env/var references

 L3METHOD(eval_cd)
 {

       //void eval_cd(sexp_t* exp, l3obj* env, l3obj** retval, t_typ etyp = 0) {
       assert(exp);
       if (1 != exp->nchild()) {
           printf("error: cd requires one argument of which object to start using\n");
           l3throw(XABORT_TO_TOPLEVEL);
       }
       
       qqchar where    = exp->ith_child(0)->val();
       
       if (0 == where.strcmp("..")) {
           l3throw(XCD_DOT_DOT);
           return 0;
       }
       
       DV(std::cout << "cd: attempting cd into '" << where << "'.\n");
       
       llref* innermostref = 0;
       objlist_t* envpath=0;
       l3obj* found = RESOLVE_REF(where,env,AUTO_DEREF_SYMBOLS, &innermostref, envpath,where,UNFOUND_RETURN_ZERO);
       if (!found) {
           std::cout << "error: unknown identifier '" << where << "'\n";
           l3throw(XABORT_TO_TOPLEVEL);
       }

       if (found->_mytag && is_captag(found)) {
              ustaq<sexp_t>*  local_sexpstaq = new ustaq<sexp_t>;
              XTRY
                  case XCODE:
                      repl(ifp, local_sexpstaq, (l3obj*)found, found->_mytag);
                      break;
                  case XCD_DOT_DOT:
                      printf("bumped up.\n");
                      XHandled();
                      break;

                  case XFINALLY:
                      delete_all_from_sexpstack(local_sexpstaq);
                      delete local_sexpstaq;
                      break;
              XENDX
              return 0;

       } else {
           std::cout << "error in cd: could not cd into object '"<< where  <<"': was not a captag/has no _mytag.\n";
           l3throw(XABORT_TO_TOPLEVEL);
       }

 }
 L3END(eval_cd)

 // dot or . means member select, as in C.
 //  find the named object, put it on the current env stack.
 // e.g. (. a ) means change the env to within the a object
 // a.   = (. a)

 L3METHOD(eval_dot)

   if (arity < 0) {
       arity = num_children(exp);
   }

   if (arity < 1) {
     printf("error: . requires (at least one argument (the obj/env to work witin.\n");
     l3throw(XABORT_TO_TOPLEVEL);
   }

   l3path my_sub_exp(exp);

   long  inex = 0;
   sexp_t* eln   = exp->ith_child(inex++);
   assert(eln);
   l3path newobjname(eln->val());
   newobjname.trim();

   if (newobjname.len() == 0) {
     printf("error: . requires one argument (the obj/env to work witin.\n");
     l3throw(XABORT_TO_TOPLEVEL);
   }

   llref* innermostref = 0;
   volatile l3obj* found = RESOLVE_REF(newobjname(),env,AUTO_DEREF_SYMBOLS,&innermostref, 0,newobjname(),UNFOUND_RETURN_ZERO);
   if (!found) {
     printf("error in dot request: could not find object '%s' to dot into (work within) in expression '%s'.\n", newobjname(), my_sub_exp());
     l3throw(XABORT_TO_TOPLEVEL);
   }

   if (found->_type != t_obj) {
       printf("error in dot request: unsatisfiable request to dot into '%s' of type '%s'; in expression '%s'.\n", 
              newobjname(), found->_type, my_sub_exp());
       l3throw(XABORT_TO_TOPLEVEL);
   }

   // just a state change, push on env stack and call to command prompt
   if (arity ==1) {
       // check for null tag first...
       if (0==((l3obj*)found)->_mytag) {
           printf("error in dot request: '%s' to dot into '%s' because it had _mytag set to zero.\n", my_sub_exp(),newobjname());
           l3throw(XABORT_TO_TOPLEVEL);
       }

       Tag* tag_to_use = found->_mytag;
       if (!tag_to_use) {
          tag_to_use = owner;
       }

         ustaq<sexp_t>* local_sexpstaq = new ustaq<sexp_t>;

         XTRY
          case XCODE:
             repl(ifp, local_sexpstaq, (l3obj*)found, tag_to_use);
             break;
          case XCD_DOT_DOT:
             printf(".. bumped up one level.\n");
             XHandled();
             break;

          case XFINALLY:
             delete_all_from_sexpstack(local_sexpstaq);
             delete local_sexpstaq;
             break;
         XENDX

       return 0;
   } // end if arity == 1.


   // vs more arguments: with ...  we execute actions with this object then pop back up.
   if (arity > 1) {

    XTRY
       case XCODE:

          eln   = exp->ith_child(inex++);
          eval(0,-1,eln, (l3obj*)found, retval, owner, curfo, etyp, retown,ifp);

          break;

       case XFINALLY:


          break;
    XENDX

   } // end if arity > 1


 L3END(eval_dot)


   L3METHOD(eval_dotdot)
   {
       l3throw(XCD_DOT_DOT);
   }
  L3END(eval_dotdot)



 ////////////////////////
 //  do_progn
 ////////////////////////

L3METHOD(do_progn)
{


     l3obj* progn_tmp = 0; 

     int len = exp->nchild();
     if (len < 1) {
         l3path msg(exp);
         printf("error in progn: must have at least one sub-expression!: '%s'\n",msg());
         l3throw(XABORT_TO_TOPLEVEL);
     }

     sexp_t* next_sib = 0;

 // snippet above was from here.

 // begin tmp_cap, tmp_tag boilerplate.

  volatile l3obj* tmp_cap = 0; // the t_cal callobject
  volatile Tag*  tmp_tag = 0;
//  volatile Tag* orig_top = defptag_get();

  // if sn is wiped out back to zero, then we know it's already cleaned up...
  volatile long   captag_sn = 0;
  l3path basenm("progn_tmp_captag");

  XTRY
     case XCODE:
         // a tmp tag needs a captain that surely will not make a cycle. So the tmp tag gets its own captain.
         make_new_captag((l3obj*)&basenm ,sizeof(actualcall),exp,env,(l3obj**) &tmp_cap , owner,0,t_cap,retown,ifp);

         captag_sn = tmp_cap->_ser;
         tmp_tag = (volatile Tag*)(tmp_cap->_owner);
         assert(tmp_cap->_owner == tmp_cap->_mytag);



         // end of boilerplate: actual code here

         for (int i = 0; i < len; i++) {
             next_sib = exp->ith_child(i);

             tmpbuf.reinit(next_sib);
             DV(printf("debug: next progn sub expression is next_sib: '%s' \n",tmpbuf()));

             if (i==len-1) {

                 // the eval was this for quite a while, but this uses env rather than tmp_cap... which seems... wrong?
                 //                 eval(0,-1,next_sib,L3STDARGS_ENV); // here use retown, since we're on the last expression.

                 eval(0,-1,next_sib,env,retval,owner,curfo,etyp,retown,ifp);
#if 0  // can we elim this... so that setq can set *retval ?
                 // need to be sure to transfer ownership of the last value to retown, or
                 // else if it's just a reference it could get deleted along with tmp_tag.
                 // e.g. consider (progn (:a=(new a a)) (:a2 = a)) ; 
                 if (*retval) {
                     if (!is_sysbuiltin(*retval)) {
                         (*retval)->_owner->generic_release_to((*retval),retown); // this is also a problem, if setq does set *retval, which it does not for now.
                     }
                 }
#endif
             } else {
                 // We were having problems with statement K in a progn overwriting the value of statement K-1,
                 // when we used retval for evertyhing. So instead,
                 // try setting passint a zeroed progn_tmp each time... yes this works.
                 // plus in owner here for retown, since we don't care about the intermediate results.
                 progn_tmp = 0;
                 eval(0,-1,next_sib,
                      env,
                      &progn_tmp,    // retval
                      (Tag*)tmp_tag, // owner
                      curfo,etyp,(Tag*)tmp_tag
                      ,ifp);
         }

         DV(print(progn_tmp, "progn_tmp:   ",0));

     }

     // end actual code, resume boilerplate.
     break;

     case XFINALLY:

           delete_captag((l3obj*)tmp_cap,L3STDARGS_OBJONLY);
           break;
   XENDX

}
L3END(do_progn)



   //////////////////////////
   //    eval function
   //////////////////////////

   // t_typ can be null, if we don't know what type to expect.

 L3METHOD(eval)
 {



     // signal of universal shutdown: if retval is 0, then we are cleaning up
     // and can't do any more allocation of memory, even (or especially) in a dtor
     // otherwise, there *should* always be a retval.
     // 
     // the problem was encountered that final teardown() on the global
     //  tag was invoking (so "some message"), which wants to allocate new
     //   string objects. Eck. We don't want anymore allocation when we are
     //   are tearing down. So dtors in particular, must be prepared to
     //   handle the case when retval == 0 and not do any allocation under
     //   these circumstances.
     //
     if (retval== 0) return 0;

     if(!exp) {
         printf("error in eval: no expression to evaluate.\n");
         l3throw(XABORT_TO_TOPLEVEL);
     }

     // rapidly ignore ; semicolon terminators
     if (exp->_ty == t_semicolon) return 0;

     qqchar v  = exp->val();
     qtree* hd = exp->headnode();

     if (exp->_ty == t_osq) {
         if (exp->_headnode) {
             return  open_square_bracket(L3STDARGS);
         } else {
             printf("not implemented yet: [] vector creation.\n");
             assert(0);
         }
     }
     else 
     if (hd) {
         v = hd->val();
     }

     // recompute this at the top of eval
     arity = exp->nchild();

     l3path current_eval_line(exp);   // current eval line
     l3path& cel = current_eval_line; // easier to type
     DV(printf("current_eval_line: %s\n",cel()));


     if (exp->_ty == t_asi) {
         return setq(L3STDARGS);
         //return setq_rhs_first(L3STDARGS);
     }
     else  if (exp->_ty == t_opn || exp->_ty == t_lsp) {
             if (hd) {
                 v = exp->headval();
                 goto PROCESS_COMMANDS;
             }
             
             // handle parenthesized expressions...
             if (arity) {
                 return eval(obj,-1, exp->first_child(), L3STDARGS_ENV);
             }
             // empty () I guess
             return 0;
     }  else  if (exp->_ty == t_ut8) {
         qqchar without_dquotes(v);
         without_dquotes.dquote_narrow();
         *retval = make_new_string_obj(without_dquotes, retown, "dquote");
         return 0;
     } else if (exp->_ty == t_q3q) {
         qqchar without_triple_quotes(v);
         without_triple_quotes.triple_quote_narrow();
         *retval = make_new_string_obj(without_triple_quotes, retown, "triple_quote");
         return 0;
     } else if (exp->_ty == t_sqo) {
         qqchar without_single_quotes(v);
         without_single_quotes.single_quote_narrow();
         *retval = make_new_string_obj(without_single_quotes, retown, "single_quote");
         return 0;
     }


     if (exp->_ty == t_ato || exp->_ty == t_dou || exp->_ty == t_str) {

         double dval;
         if (exp->_ty == t_ato || exp->_ty == t_dou) {
             // it could be a number/double

             char* endptr = v.e;
             dval = strtod(v.b,&endptr);
             double** pd = (double**)&dval;

             if (endptr != v.b) {

                 if (arity != 0) {
                     l3path vs(v);
                     printf("error in eval: unexpected child of number '%s' in parse tree of number; in expression '%s', parsed as:\n",vs(), cel());
                     exp->print_stringification(0);
                     l3throw(XABORT_TO_TOPLEVEL);         
                 }

                 prep_double_returntype((l3obj*)(*pd),arity,exp,L3STDARGS_ENV);
                 return 0;
             }

             // boolean primitives true and false.
             if (v.strcmp("false")==0 || v.strcmp("F")==0 || v.strcmp("#f")==0 || v.strcmp("nil")==0) {
                 *retval = gnil;
                 return 0;

             } else if (v.strcmp("true")==0 || v.strcmp("#t")==0 || v.strcmp("T")==0) {
                 *retval = gtrue;
                 return 0;
             }

             llref* innermostref = 0;
             objlist_t* envpath =0;

             l3obj* found = 0;


             if (!found && env) {
                 found = RESOLVE_REF(v,
                                                                env,
                                                                AUTO_DEREF_SYMBOLS,
                                                                &innermostref, 
                                                                envpath,
                                                                current_eval_line(),
                                                                UNFOUND_RETURN_ZERO);
             }

             // try in curfo, next.
             if (!found && curfo) {
                 found = RESOLVE_REF(v,
                                                                curfo,
                                                                AUTO_DEREF_SYMBOLS,
                                                                &innermostref, 
                                                                envpath,
                                                                current_eval_line(),
                                                                UNFOUND_RETURN_ZERO);
             }

             if (!found) {

                 // new style...
                 goto PROCESS_COMMANDS;

                 // old style...
                 *retval = make_new_literal(v,retown,qqchar("literal"));
                 return 0;



             } else {

                 // allow symbols (symvec) to evaluate to their contained values... 
                 l3obj* finalret = found;

                 if (found->_type == t_syv) {
                     long len = symvec_size(found);
                     DV(printf("we see found %p symvec ser # %ld, of length %ld\n",
                               found, found->_ser, len));
                 }

                 *retval = finalret;
                 return 0;

             }
             return 0;

         } // end  if (exp->aty == SEXP_BASIC)

         // we get here for strings. update: do we still?
         if (exp->_ty == t_ut8) {
             *retval = make_new_string_obj(v, retown, "dquote");
             return 0;
         }

         // do we ever get here?
         // assert(0); // check that we don't! =) we'll this line did: "01 : '243' : t_ato" without the double quotes.
         printf("error in eval: unrecognized input '%s'\n",cel());
         l3throw(XABORT_TO_TOPLEVEL);         

         return 0;
     }


 PROCESS_COMMANDS:


     DV(printf("we see arity %ld of this sexp:\n",arity));
     DV(exp->p());

     if (v.strcmp("quit") == 0 || v.strcmp("q") == 0 || v.strcmp("Q") == 0) {

         global_quit_flag = true;
         return 0;

     } else if (0==v.strcmp("++") || 0==v.strcmp("--")) {
         return plusplus_minusminus(L3STDARGS);
         
     } else if (0==v.strcmp("loud")) {
         loud();
         return 0;

     } else if (0==v.strcmp("UGLY")) {
         print_display_ugly();
         return 0;

     } else if (0==v.strcmp("PRETTY")) {
         print_display_pretty();
         return 0;

     } else if (0==v.strcmp("quiet")) {
         quiet();
         return 0;

     } else if (0==v.strcmp("floud")) {
         function_defn_loud();
         return 0;

     } else if (0==v.strcmp("fquiet")) {
         function_defn_quiet();
         return 0;

     } else if (0==v.strcmp("cd")) {
         eval_cd(0,arity,exp, L3STDARGS_ENV);
         return 0;

     }
     else if (0==v.strcmp(".")) {
         eval_dot(0,arity, exp, L3STDARGS_ENV);
         return 0;

     }
     else if (0==v.strcmp("..")) {
         eval_dotdot(0,arity, exp, L3STDARGS_ENV);
         return 0;

     }
     else if (v.strcmp("gdump") == 0) {
         gdump();
         return 0;

     } else if (v.strcmp("ls") == 0) {

         printf("=== current environment is:\n");
         print(env,"   ",0);

     } else if (0==v.strcmp("quiet")) {
         quiet();
         return 0;

     } else if (0==v.strcmp("loud")) {
         loud();
         return 0;

     } else if (0==v.strcmp("version")) {
         printf("version: $Id$-expansion\n");
         return 0;

     } else if (0==v.strcmp("del_all")) {
         printf("del_all unimplemented at the moment: not sure how to "
                "delete the current env when the completion routines "
                "bracketing this one will expect to pop that same "
                "env off the env stack...\n\nFor now please do "
                "manual (rm badboy) to delete object badbody.\n");
         l3throw(XABORT_TO_TOPLEVEL);

#if 0
         printf("deleting all variables in default ptag.\n");
         l3obj* cap = defptag_get()->captain();
         if (cap) {
             clear_hashtable(cap);
         }
         defptag_get()->del_all(L3STDARGS);
#endif

         return 0;

     } else if (0==v.strcmp("clear_history")) {
         printf("clearing command line history (does not actually delete "
                "objects ... use del_all for that.\n");
         repl_value_history_l3obj->reset();
         return 0;

     } else
         // unittest
         if (v.strcmp("ut") == 0) {
             unittest_max = 1; // start testing if over 0, and go to test_max.
             unittest_cur = 0; // reflects current test number

             int nc = num_children(exp);
             bool is_double = false;
             double dval = 0;

             if (nc > 3) {
                 printf("unittest specification error: More than two arguments given. "
                        "Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
                 return 0;  
             } else if (nc >= 1) {

                 unittest_cur = 0; // reflects current test number
                 if (nc == 1) {
                     unittest_max = utest.max_test();
                     printf("(ut) requested: running all unit tests  #%ld  -  #%ld.\n", 
                            unittest_cur, unittest_max);
                     return 0;
                 } else  {
                     is_double = false;
                     dval = parse_double(ith_child(exp,1)->val().b, is_double);
                     if (!is_double || (is_double && dval < 0)) {
                         printf("unittest specification error: last_test_num was not a natural number. "
                                "Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
                         return 0;
                     }       
                     unittest_max = floor(dval);
                 }

                 if (nc ==2) {
                     printf("(ut %ld) requested: running unit tests  #%ld  -  #%ld.\n",
                            unittest_max, unittest_cur, unittest_max);
                     return 0;
                 } else {
                     assert(nc == 3);
                     is_double = false;
                     dval = parse_double(ith_child(exp,2)->val().b, is_double);
                     if (!is_double || (is_double && dval < 0)) {
                         printf("unittest specification error: first_test_num was not a natural "
                                "number. Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
                         unittest_max = 0;
                         unittest_cur = 0;
                         return 0;
                     } else {
                         unittest_cur = unittest_max;
                         unittest_max = floor(dval);
                         printf("(ut %ld %ld) requested: running unit tests  #%ld  through  #%ld.\n",
                                unittest_cur, unittest_max, unittest_cur, unittest_max);
                         return 0;
                     }
                 }

             }
             return 0;
         } 

     // end "ut" unittest processing.

     // uu: shortcut to run (ut 1 1)
         else if (0==v.strcmp("uu")) {
             printf("running (ut 1 1)\n");

             unittest_cur = 1;
             unittest_max = 1;

             return 0;
         }
         else if (0==v.strcmp("src")) {

             if (arity != 1) { 
                 printf("error in src: specify filename or filepath to read in and execute source commands from.\n"); 
                 return 0; 
             }
             l3path path(home);
             path.chompslash();
             path.pop();

             qqchar fn = ith_child(exp,0)->val();
             l3path fname(fn);
             fname.dquote_strip();

             if (fname.buf[0]=='/') {
                 // use abs path provied
                 path.clear();
                 path.pushq(fname());
             } else {
                 path.pushf("/");
                 path.pushq(fname());
             }
             char* use = (char*)path();

             if (!file_exists(use)) {
                 l3path msg(0,"src command error: file '%s' not found.",use);
                 msg.outln();
                 return 0;
             }

             FILE* src = fopen(use,"r");
             if (!src) {
                 l3path msg(0,"could not fopen this '%s' for reading: '%s' reported.",use,strerror(errno));
                 msg.outln();
                 return 0;
             }

             call_repl_in_trycatch(src,0,false,env,owner);
             fclose(src);
             src=0;
             return 0;

             // end src
         }

#if 0 // no more colon dispatch

         else if (0==v.strcmp(":") || 0==v.strcmp(",")) {

             // use a different character for variable oppos dispatch!
             // (!n means use the (counting from zero, or 0-based) n-th list item as the operator position)
             //
             // so....
             // (!1 means use the (0-based counting) 1st arg (second arg if counting from one) as the dispatch operator)
             //
             // (: is short hand for (!1
             // (  is short hand for (!0  (i.e. the default dispatch for lisp can be thought of as (:0 op ...) -> (op ...)
             //
             // (!-1 means dispatch on the very last verb in the list
             // (!-2 means dispatch on the second to last verb
             // (!2 means dispatch on the (0-based 2nd, 1-based 3rd) arg in the list.
             //
             // the operator is removed from the list and placed in head position, and then we re-eval the form.

             // (: a b ...)  means turn into (b a ...) and re-evaluate; i.e. switch the first and second list elements, and eliminate ":";

             // ex: (: a = 5)          =>  (= a 5)        ; works!
             // ex: (: a += 1 2 3 4)   =>  (+= a 1 2 3 4) ; works!
             // ex: (: a < 50)         =>  (< a 50) ; that works
             // ex: (: b = (: a < 5))  =>  (= b (< a 5)) ; that works
                
             // ex: (sel a (: a < 5))       ; fairly readable, yes! equiv. to a[ a<5 ]; # in R.
             // ex: (: a = (sel a (: a < 5)))   ; works for me. equiv to  a = a[a<5]; # in R
    
             // (: a b ...)  means turn into (b a ...) and re-evaluate; i.e. switch the first and second list elements, and eliminate ":";

             // so: check for arity at least 1 else error
             if (arity <= 1) {
                 printf("error in '%s': colon infix operator needs at least one argument.\n",linebuf);
                 return 0;
             }

             // for arity 2, e.g. (: a) , eliminate the starting ":" and re-eval delete the 1st list item by itself; e.g. (: a) -> (a)
             if (arity == 2) {
                 sexp_t* nex = exp->list->next;
                 sexp_t* colon_elt = exp->list;
                 exp->list = nex; // skip colon
                 colon_elt->next = 0;
                 destroy_sexp(colon_elt);

                 print_sexp((l3obj*)linebuf_after_colon_swap,BUFSIZ,exp, 0,0,owner,  0,0,owner);
                 DV(printf("NB: After colon swap we have: '%s'\n",linebuf_after_colon_swap));
                 return L3EVALSEXP(exp);
             }

             // for arity 2 or more, eliminate the starting and ":", move the 3rd list element to be the first
             //    (: a b c...)  ->  (b a c...) ; where c... can be 0 or more elements, they stay untouched.
    

             sexp_t* el    = first_child(exp);
             sexp_t* eln   = ith_child(exp,1);
             sexp_t* elnn  = ith_child(exp,2);
             sexp_t* elnnn = ith_child(exp,3);

             eln->next = elnnn;
             elnn->next = eln;
             exp->list = elnn;

             // cleanup by deleting the ":" element, now that it is out of the list.
             el->next = 0;
             destroy_sexp(el); // conjecture: this is causing us to loose other elements too.

             // updates exp
             print_sexp((l3obj*)linebuf_after_colon_swap,BUFSIZ,exp, 0,0,owner,  0,0,owner);

             DV(printf("NB: After colon swap we have: '%s'\n",linebuf_after_colon_swap));

             return L3EVALSEXP(exp);

         } 
#endif // 0


         else if (v.strcmp("de") == 0 || v.strcmp("fn") == 0) {
             return defmethod(L3STDARGS);

             // ====================================
             // ====================================
             //   progn
             // ====================================
             // ====================================

         } else if (v.strcmp("progn")==0) {
             // evaluate all subitems in the list, return the value of the last one.

             do_progn(0,arity,exp, L3STDARGS_ENV);
             return 0;
             // end progn

         } else if (exp->_ty == t_obr && 0==v.strcmp("{")) {
             return do_progn(0,arity,exp, L3STDARGS_ENV);

         } else if (v.strcmp("+") == 0 
                    || v.strcmp("-") == 0
                    || v.strcmp("*") == 0
                    || v.strcmp("/") == 0
                    || v.strcmp("~") == 0
                    ) {

             if (arity == 1 ) {
                 if (v.strcmp("-")==0 || v.strcmp("~")==0) {
                     // needs to be before the binary op check...
                     if (-1 != double_unaryop(L3STDARGS)) {
                         return 0; // handled.
                     }
                 }
             }

             if (arity < 2) {
                 std::cout << "arity error in '" << linebuf  << "': saw " << arity-1 << " argument(s), but '"
                           << v << "' needs at least 2.\n";
                 return 0;
             }

             arithmetic_with_closure(0,arity, exp, L3STDARGS_ENV);
             return 0;

         } else if (v.strcmp("setq") == 0 || v.strcmp("=") == 0) {
             return setq(L3STDARGS);
             //return setq_rhs_first(L3STDARGS);

             //=================================
             //=================================
             // 
             //  object t_obj methods
             //
             //=================================
             //=================================


         } else if (v.strcmp("new")==0) {
    
             if (arity > 2) {
                 printf("arity error in new: too many arguments to new."
                        " (new classname objectname) ; new requires a "
                        "class name (or 'obj') and an objectname; (new obj myobj)"
                        " for a blank base object.\n");
                 l3throw(XABORT_TO_TOPLEVEL);
             }
             l3path classname(0,"o");
             l3path   objname(0,"o");
    
             if (arity >= 1) {
                 classname.reinit(ith_child(exp,0)->val());
             }    
             if (arity == 2) {
                 objname.reinit(ith_child(exp,1)->val());
             }

             make_new_captag((l3obj*)&objname ,0,exp,env,retval, owner, (l3obj*)&classname,t_obj,retown,ifp);
             return 0; // return values go back in retval
    
         } else if (v.strcmp("p")==0) {

             return print_list_to_string(0,arity,exp, L3STDARGS_ENV);

         } else if (v.strcmp("if")==0) {
             return eval_if_expr(L3STDARGS);

         } else if (v.strcmp("false")==0 || v.strcmp("F")==0 || v.strcmp("#f")==0 || v.strcmp("nil")==0) {
             *retval = gnil;
             return 0;

         } else if (v.strcmp("true")==0 || v.strcmp("#t")==0 || v.strcmp("T")==0) {
             *retval = gtrue;
             return 0;

         } else if (v.strcmp("lambda")==0) {

         } else if (v.strcmp("transfer")==0) {
             transfer(L3STDARGS);

         } else if (v.strcmp("len")==0) {
             eval_len(L3STDARGS);

         } else if (v.strcmp("srllen")==0) {
             strlen(L3STDARGS);

         } else if (v.strcmp("v")==0) {
             create_vector(L3STDARGS);

         } else if (v.strcmp("in")==0 || v.strcmp("member")==0) {
             // (in a b) returns T if object a is in the ptrvec b; (member needle haystack) is alias.
             member_a_of_b(L3STDARGS);

         } else if (v.strcmp("ownlist")==0) {
             get_ownlist(L3STDARGS);

         } else if (v.strcmp("assert")==0) {
             return assert_with_tmp_tag(L3STDARGS);

         } else if (v.strcmp("owns")==0) {
             //(owns  what owner); predicate, returns T or F
             return owns(L3STDARGS);

         } else if (v.strcmp("cp")==0) {
             return fresh_copy(L3STDARGS);

         } else if (v.strcmp("si")==0) {
             printf("si not implemented!\n");
             l3throw(XABORT_TO_TOPLEVEL);

         } else if (v.strcmp("se")==0) {      
             return print_strings((l3obj*)1,arity,exp,L3STDARGS_ENV);

         } else if (v.strcmp("rchk")==0) {      
             printf("checking global llref list for dangling refs...starting.\n");
             llr_global_debug_list_check_dangling();
             printf("checking global llref list for dangling refs...done.\n");
             return 0;

         } else if (is_protobuf_reserved_word(L3STDARGS)) {
             std::cout << "error: '" << v << "' is a reserved (protobuf) operation.\n";
             l3throw(XABORT_TO_TOPLEVEL);

         } else if (v.strncmp("://",3)==0 || v.strncmp("//",2)==0) {
             // a cpp comment
             return 0;

         } else if (v.strcmp("aref")==0) {
             return aref(L3STDARGS);

         } else if (v.strcmp("so")==0) {
             print_strings(0,arity,exp,L3STDARGS_ENV);
             printf("\n"); // so we can see the output.
             return 0;

             // top-down operator precedence parsing
             L3DISPATCH(tdop)
             L3DISPATCH(sexp)
                 // L3DISPATCH(addtxt) // add more text to a tdop parser.
             L3DISPATCH(pratt)  // start infix mode interp
             L3DISPATCH(prefix)  // switch back to prefix mode interp (the original mode for terp).

             // user defined exceptions
                 L3DISPATCH(l3x_throw)
                 L3DISPATCH(l3x_try)
                 L3DISPATCH(l3x_catch)
                 L3DISPATCH(l3x_finally)
                 L3DISPATCH(l3x_handled)

         } else if (v.strcmp("throw")==0) {
             return l3x_throw(L3STDARGS);

         } else if (v.strcmp("try")==0) {
             return l3x_try(L3STDARGS);

         } else if (v.strcmp("set_catch")==0) {
             return l3x_catch(L3STDARGS);

         } else if (v.strcmp("set_finally")==0) {
             return l3x_finally(L3STDARGS);

         } else if (v.strcmp("handled")==0) {
             return l3x_handled(L3STDARGS);


             // dq methods
             L3DISPATCH(dq)
                 
                 L3DISPATCH(dq_clear)
                 L3DISPATCH(dq_dtor)
                 L3DISPATCH(dq_cpctor)
                 L3DISPATCH(dq_del_ith)
                 
                 L3DISPATCH(dq_ith)
                 
                 L3DISPATCH(dq_len)
                 L3DISPATCH(dq_size)
                 L3DISPATCH(dq_find_name)
                 L3DISPATCH(dq_find_val)
                 
                 L3DISPATCH(dq_erase_name)
                 L3DISPATCH(dq_erase_val)
                 
                 L3DISPATCH(dq_front)
                 L3DISPATCH(dq_pushfront)
                 L3DISPATCH(dq_popfront)
                 
                 L3DISPATCH(dq_back)
                 L3DISPATCH(dq_pushback)
                 L3DISPATCH(dq_popback)
                 
             // mq methods
             L3DISPATCH(mq)
                 
                 L3DISPATCH(mq_dtor)
                 L3DISPATCH(mq_cpctor)
                 L3DISPATCH(mq_del_ith)
                 
                 L3DISPATCH(mq_ith)
                 
                 L3DISPATCH(mq_len)
                 L3DISPATCH(mq_size)
                 L3DISPATCH(mq_find_name)
                 L3DISPATCH(mq_find_val)
                 
                 L3DISPATCH(mq_erase_name)
                 L3DISPATCH(mq_erase_val)
                 
                 L3DISPATCH(mq_front)
                 L3DISPATCH(mq_pushfront)
                 L3DISPATCH(mq_popfront)
                 
                 L3DISPATCH(mq_back)
                 L3DISPATCH(mq_pushback)
                 L3DISPATCH(mq_popback)

             // dd methods
             L3DISPATCH(dd)
                 
                 L3DISPATCH(dd_dtor)
                 L3DISPATCH(dd_cpctor)
                 L3DISPATCH(dd_del_ith)
                 
                 L3DISPATCH(dd_ith)
                 
                 L3DISPATCH(dd_len)
                 L3DISPATCH(dd_size)
                 
                 L3DISPATCH(dd_front)
                 L3DISPATCH(dd_pushfront)
                 L3DISPATCH(dd_popfront)
                 
                 L3DISPATCH(dd_back)
                 L3DISPATCH(dd_pushback)
                 L3DISPATCH(dd_popback)


             L3DISPATCH(logical_binop)
             L3DISPATCH(eval_parse)
                 //             L3DISPATCH(parse)

             L3DISPATCH(defmethod)
             L3DISPATCH(rm)
             
             // set/test a state string so we easily know what test we are on.
             L3DISPATCH(ctest)
             L3DISPATCH(get_ctest)


             // test that memory is empty except for system builtin marked objects/tags
             L3DISPATCH(allbuiltin)

                 L3DISPATCH(prog1)
                 L3DISPATCH(binop)
                 L3DISPATCH(unaryop)
                 L3DISPATCH(seal)
                 L3DISPATCH(dtor_add)

                 // ls on tags
                 L3DISPATCH(lst)

                 // ls breadth first search
                 L3DISPATCH(lsb)

                 // show command history, like shell
                 L3DISPATCH(history)

                 // testing maps
                 L3DISPATCH(test_l3map)

                 // captain of the nearest common ancestor in the tag tree
                 L3DISPATCH(nca)

                 // symbols
                 L3DISPATCH(test_symvec)
                 L3DISPATCH(make_new_symvec)
                 L3DISPATCH(symvec_setf)
                 L3DISPATCH(aliases)
                 L3DISPATCH(canon)

                 // history management & automatic deletes from toplevel
                 L3DISPATCH(history_limit_get)
                 L3DISPATCH(history_limit_set)

                 L3DISPATCH(left)
                 L3DISPATCH(right)
                 L3DISPATCH(parent)
                 L3DISPATCH(sib)
                 L3DISPATCH(ischild)

                 L3DISPATCH(numchild)
                 L3DISPATCH(firstchild)
                 L3DISPATCH(ith_child)
                 L3DISPATCH(lastchild)

                 L3DISPATCH(set_obj_left)
                 L3DISPATCH(set_obj_right)
                 L3DISPATCH(set_obj_parent)
                 L3DISPATCH(set_obj_sib)
                 L3DISPATCH(addchild)

                 L3DISPATCH(run)

                 L3DISPATCH(to_string)
                 L3DISPATCH(strlen)

                 // boolean 
                 L3DISPATCH(and_)
                 L3DISPATCH(or_)
                 L3DISPATCH(not_)
                 L3DISPATCH(xor_)
                 L3DISPATCH(setdiff)
                 L3DISPATCH(intersect)
                 L3DISPATCH(all)
                 L3DISPATCH(any)
                 L3DISPATCH(union_)
                 L3DISPATCH(as_bool)

                 //loops
                 L3DISPATCH(foreach)
                 L3DISPATCH(while1)
                 L3DISPATCH(break_)
                 L3DISPATCH(c_for_loop)
                 L3DISPATCH(c_continue)
                 L3DISPATCH(c_break)

                 } else if (v.strcmp("for")==0)      { return c_for_loop(L3STDARGS);
                 } else if (v.strcmp("continue")==0) { return c_continue(L3STDARGS);
                 } else if (v.strcmp("break")==0)    { return c_break(L3STDARGS);
        
                 // lists
                 L3DISPATCH(first)
                 L3DISPATCH(rest)
                 L3DISPATCH(cons)

                 L3DISPATCH(sys)
                 L3DISPATCH(system)
                 L3DISPATCH(poptop)

                 // identifier management
                 L3DISPATCH(exists)
                 L3DISPATCH(lexists)
                 L3DISPATCH(lookup)
                 L3DISPATCH(llookup)

                 // symbols
                 L3DISPATCH(quote)
                 L3DISPATCH(symvec_aref)

                 // rm
                 L3DISPATCH(softrm)

                 // diagnostic help
                 L3DISPATCH(setstop)
                 L3DISPATCH(getstop)

                 // log that we expect zero user allocations in the log
                 L3DISPATCH(checkzero)
                 L3DISPATCH(allsane)

                 L3DISPATCH(link)
                 L3DISPATCH(relink)
                 L3DISPATCH(chase)
                 L3DISPATCH(linkname)
                 L3DISPATCH(rename)

                 L3DISPATCH(save)
                 L3DISPATCH(load)
                 L3DISPATCH(nv3)
                 L3DISPATCH(make)

                 } else if (v.strcmp("ptag_get")==0) {

         } else if (v.strcmp("ptag_set")==0) {

         } else if (v.strcmp("cd")==0) {
             printf("cd unimplemented...\n");

         } else if (v.strcmp("finally_add")==0) {

         } else if (v.strcmp("finally_del")==0) {

         } else if (v.strcmp("with")==0) {

         } else if (v.strcmp("end")==0) {

         } else if (v.strcmp("data_add")==0) {

             //obj->_judyL = tmp;

         } else if (v.strcmp("data_del")==0) {    

         } else if (v.strcmp("data_list")==0) {    
    
         } else if (v.strcmp("method_add")==0) {
    
         } else if (v.strcmp("method_list")==0) {
    
         } else if (v.strcmp("method_del")==0) {
    
         } else if (v.strcmp("prop_set")==0) {
    
         } else if (v.strcmp("prop_get")==0) {


    
             //=================================
             //  end of t_obj methods
             //=================================

         } else if (v.strcmp("rm") == 0 || v.strcmp("del") == 0) {
             return rm(L3STDARGS);

         } else if (v.strcmp("n") == 0) {
             // simple fast shortcut to run the next test: "n".  next test is last_test +1, so (ut N N) will reset where "n" starts from.
             return utest.run_next_test();

         } else {
             /// gotta check for a string (eq string1 string2) before we allow double eq
             if (0==v.strcmp("eq")) {
                 // try string eq eq
                 if (0==eq_string_string(L3STDARGS)) return 0;
             }

             // is it a boolean operator? They get their own function.
             // returns 0 if recognized; else -1
             if (0 == logical_binop(L3STDARGS)) return 0;
             if (0 == binop(L3STDARGS)) return 0;
             if (0 == unaryop(L3STDARGS)) return 0;

             // an I/O primitive?
             if (0 == ioprim(L3STDARGS)) return 0;


             //is it the name of a function we should invoke? invocation here:
             llref* innermostref = 0;
             objlist_t* envpath=0;

             // same order of evaluation for invocation as in lhs_setq: first env, then curfo.
             l3obj* obj = 0;
             if (!obj && env)   {
                   obj = RESOLVE_REF(v,env,AUTO_DEREF_SYMBOLS,&innermostref, envpath,current_eval_line(),UNFOUND_RETURN_ZERO);
             }
             if (!obj && curfo) {
                   obj = RESOLVE_REF(v,curfo,AUTO_DEREF_SYMBOLS,&innermostref, envpath,current_eval_line(),UNFOUND_RETURN_ZERO);
             }


             if (obj && (obj->_type == t_fun || obj->_type == t_clo || obj->_type == t_lda)) {
                 return try_exp_dispatch(L3STDARGS);
             }

             if (obj) {
                 if (arity == 1) {
                     // what happens if we don't print here, but just return? does it prevent the double printout at the command prompt?
                     DV(std::cout << "'" << v << "' resolves to:\n");
                     DV(print(obj,"   ",0));

                     *retval = obj;
                     return 0;
                 } else {
                     // see if we've got a function call next
                     //QUICKO(functioncheck);
                     //L3EVALINTO(exp->next,functioncheck);
                     L3EVALIN(ith_child(exp,1),obj);
                 }
             }

             // check to see if it's a linked in function already, i.e. from a library or jit-compiled function.
             //    llvm::Function *CalleeF = TheModule->getFunction(std::string(v));

             //    void* funfound = dlsym(RTLD_DEFAULT,v);
             //    printf("dlsym lookup on '%s' returned %p\n",v,funfound);
             /*
               if (CalleeF != 0) {
 

               assert(0); // not yet implemented.

               //      hmmm... this creates code that later does a call (a thunk), but doesn't actuall do the call.
               //      std::vector<llvm::Value*> ArgsV;
               //      for (unsigned i = 0, e = Args.size(); i != e; ++i) {
               //    ArgsV.push_back(Args[i]->Codegen());
               //    if (ArgsV.back() == 0) return 0;
               //      }
               //      Builder.CreateCall(CalleeF, ArgsV.begin(), ArgsV.end(), "calltmp");

               //      if (Function *LF = F->Codegen()) {

               // or this? no, this actually JITs the function LF.   void *FPtr = TheExecutionEngine->getPointerToFunction(LF);
      
               // Cast it to the right type (takes no arguments, returns a double) so we
               // can call it as a native function.
               //      double (*FP)() = (double (*)())(intptr_t)FPtr;

               return 0;
               }
             */

             // if we allow literals, then set etyp to t_lit...
             if (etyp && etyp == t_lit) {
                 *retval = make_new_literal(v,retown,qqchar("literal"));
                 return 0;
             }

             std::cout << "eval error: unknown identifier '" << v << "'\n";
             l3throw(XABORT_TO_TOPLEVEL);
         } 

 }
L3END(eval)



void teardown(FILE* fp, Tag* owner) {

  // the dtor takes care of this:  purge_all(env);
  fclose(fp);
  histlog->teardown();

  // don't do this or it will be a double free: env is owned by global_tag, which
  // will do the jfree() automatically.
  //  jfree(env);


//  bal_pop_to_tag(global_tag_stack.back_val(),false,true);
//  global_tag_stack_loc.clear();

  global_terp_final_teardown_started = 1;


  // should we pop to the top first? Shrug. It should work anyway right?

  DV(gdump());
  
  Tag* t = (Tag*)owner;
  assert(t);


  if (t) {
    long unseen = t->dfs_been_here();
    l3obj* ret = 0;
    t->dfs_destruct(0,unseen,0,main_env, &ret, t, 0, 0, t, fp); // destroy the global_root tag/env
    


    delete t;
  } else {
      // why no global tag?
      assert(0);
  }


  l3path memcheck(global_scope());
  memcheck.chompslash();
  memcheck.pop();
  l3path memcheck_cmd(0,"cd %s; ./memcheck %s",memcheck(),memlogpath());

#if 1  // throwing off our sermon checking, disable for now...

  // Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

#endif   

#if 0 // too many valgrind errors, shut off the compiler stuff for now.
    compiler_teardown();
#endif

  // closedown/flush the logs
    // no, let global_init_in_order do it, so we capture global deallocs in order:  mlog->destruct();
  histlog->teardown();

  // release all the cache sexp elements that sfsexp library keeps
  // around to speed things up. Gotta do this before deleting globi, which releases mlog.


  // this delete has to be after all other memory free-ing is done:
  delete globi;
  globi = 0;


  // try to get rid of additional valgrind reported irrelevant false pos leaks
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
}

// set home to where we started.

void terp_sa_sigaction(int signum, siginfo_t* si, void* vs) {
    // do we want to 
    printf(" [ctrl-c]\n");
    l3throw(XABORT_TO_TOPLEVEL); // to clean up all in-progress commands?
    //  printf("in terp_sa_sigaction, with signum: %d   si: %p   vs: %p\n",signum,si,vs);
}

void setup_and_init(Tag** ppt) {

    // make us internationalizable -- http://www.debian.org/doc/manuals/intro-i18n/ch-locale.en.html
    setlocale(LC_ALL, "");

    // why should it be this difficult to get teardown ordering of globals...
    
    globi = new global_init_in_order;

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sigaction sa;
    bzero(&sa,sizeof(struct sigaction));
    struct sigaction oldact;
    sa.sa_sigaction = &terp_sa_sigaction;
    sa.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(SIGINT , &sa, &oldact)) {
        perror("error: terp could not setup SIGINT signal handler. Aborting.");
        exit(1);
    }

 SHA1_Init(&shactx);

  l3path global_tag_name(0,"global_tag");
  Tag* pt = glob = *ppt =  new Tag(STD_STRING_WHERE.c_str(), 0, global_tag_name(),0);


  unittest_max = 0; // start testing if over 0, and go to test_max.
  unittest_cur = 0; // reflects current test number


  global_scope.reinit("%s/interp_%d",pwd(),pid);
  DV(printf("global_scope being set to:\n"));
  DV(global_scope.dump());

  history_path.reinit("%s/history_%d",pwd(),pid);

  histlog->init(history_path);
  //  histlog->keep_last(0);
  histlog->keep_last(1);
  //histlog->keep_last(2); // Use (history_limit_set N) to change, (history_limit_get) to query.
  histlog->monitor_this_tag(pt);

  home = global_scope;
  DV(printf("home being set to:\n"));
  DV(home.dump());

  cur_object = global_scope;

  cur_method.reinit("%s/main",home());

  memlogpath.init(global_scope());
  memlogpath.chompslash();
  memlogpath.pop();

  // do main_env before gnil, so that gnil can find its owner's captain
  //
  // main_env setup : captain of global_tag pt
  l3path mainnm(0,"main_env");
  make_new_obj("main_env","main_env",pt,0,&main_env);
  pt->captain_set(main_env);
  set_undeletable(main_env);
  set_sysbuiltin(main_env);

  // init gnil and gtrue
  gnil = make_new_class(0,glob,"gnil","gnil");
  gnil->_type = t_fal;
  set_undeletable(gnil);
  set_sysbuiltin(gnil);

  gtrue = make_new_class(0,glob,"gtrue","gtrue");
  gtrue->_type = t_tru;
  set_undeletable(gtrue);
  set_sysbuiltin(gtrue);

  gnan = make_new_double_obj(NAN,glob,"gnan");
  gnan->_type = t_nan;
  set_undeletable(gnan);
  set_sysbuiltin(gnan);

  gna = make_new_double_obj(NAN,glob,"gna");
  gna->_type = t_nav;
  set_undeletable(gna);
  set_sysbuiltin(gna);

#if 0   // too many valgrind errors, shut off compiler init for now
  // do compiler init
  compiler_init();
#endif

  // finish main_env setup...
  // and monitor it for deletable objects too.
  if (main_env->_mytag) { histlog->monitor_this_tag(main_env->_mytag); }


  // run test_owned!  debug code here:
  //  main_env->_mytag->test_owned();

//  global_env_stack.push_front(main_env);

  // make the exception stack, which is a regular dq object.
  make_new_dq(0,-1,0, main_env, &exception_stack,pt, 0,0,pt,0);
  set_undeletable(exception_stack);
  set_sysbuiltin(exception_stack);

  assign_global_symbols();

 
}

void set_builtin(llref* r) {
    assert(r);
    // llref now have _reserved so can be marked as sysbuiltin
    set_sysbuiltin(r);
}

void assign_global_symbols() {

    l3path tr("T");
    l3path fa("F");
    l3path ma("main_env");
    l3path nan("nan");
    l3path na("na");
    l3path xc("XQ");  // exception queue.

    set_builtin(add_alias_eno(main_env, tr(), gtrue));
    set_builtin(add_alias_eno(main_env, fa(), gnil));
    set_builtin(add_alias_eno(main_env, ma(), main_env));

    set_builtin(add_alias_eno(main_env, nan(), gnan));
    set_builtin(add_alias_eno(main_env, na(), gna));

    set_builtin(add_alias_eno(main_env, xc(), exception_stack));
}


#if 0
void test_judy_hashtables() {

  l3obj* o = make_new_class(0,glob,"test_judy_hashtables","test_judy_hashtables_o");

  size_t sz = dump_hash(o, DQ("test_judy_hashtables(): "),0);

  l3path key1(0,"firstkey");
  l3path key2(0,"key2");

  l3path val1(0,"value1 one");
  l3path val2(0,"two two value two");

  add_alias_eno(o, key1(), (l3obj*)val1());
  sz = dump_hash(o, DQ("test_judy_hashtables(): "),0);
  add_alias_eno(o, key2(), (l3obj*)val2());
  sz = dump_hash(o, DQ("test_judy_hashtables(): "),0);

  void* back1 = lookup_hashtable(o, key1());
  void* back2 = lookup_hashtable(o, key2());

  printf("judy['%s'] -> %s\n",key1(),(char*)back1);
  printf("judy['%s'] -> %s\n",key2(),(char*)back2);

  sz = dump_hash(o, DQ("test_judy_hashtables(): "),0);


  l3obj* odup = make_new_class(0,glob,"test_judy_hashtables","test_judy_SL_dup");
  copy_judySL(o->_judyS, &(odup->_judyS));

  dump_hash(odup, DQ("test_judy_hashtables() after DL dup: "),0);

  clear_hashtable(o);

  sz = dump_hash(o, DQ("test_judy_hashtables(): "),0);

}
#endif

void test_judy_dup() {

  l3obj* src = make_new_double_obj(10.0, glob, "test_judy_dup_double");

  l3obj* initnan = make_new_double_obj(NAN, glob, "test_judy_dup_double_INITNAN");

  for (int i = 0; i < 10; ++i) {
    double_set(src, i, i*5+3);
  }

  print(src," src:   ",0);

  void* dest = 0;  
  copy_judyL(src->_judyL, &dest);

  print(src," src again:   ",0);
  
  Word_t    array_size;
  JLC(array_size, dest, 0, -1);
  
  printf("saw %ld in dest\n",array_size);

  Word_t * PValue = 0;
  JLG(PValue, dest,5);
  if (!PValue) assert(0);

  initnan->_judyL = dest;

  print(initnan, "initnan after copy_judyL:   ",0);


  judySmap<int> tester;

  int one = 1;
  int two = 2;
  int three = 3;
  
  tester.insertkey("one",one);
  tester.insertkey("two",two);
  tester.insertkey("three",three);

  tester.dump("tester:   ");

  judySmap<int> t2(tester);

  printf("did copy ctor work?\n");
  t2.dump("t2:   ");

  judySmap<int> t3;
  t3 = t2;
  t3.dump("t3:     ");

  
}

std::string FindAndReplace( std::string tInput, const std::string& tFind, const std::string& tReplace, char* dqmap ) { 

  size_t uPos = 0; 
  size_t uFindLen = tFind.length(); 
  size_t uReplaceLen = tReplace.length();

  if( uFindLen == 0 ) return tInput;

  for( ;(uPos = tInput.find( tFind, uPos )) != std::string::npos; )
    {
      if (dqmap[uPos] == 'x') {
        ++uPos;
    continue; // in quoted string; don't alter it.
      }

      tInput.replace( uPos, uFindLen, tReplace );
      uPos += uReplaceLen;
    }

  return tInput;
}

// detect {! = + - * / =}
bool id_doublechar_eq_op(size_t pos, const std::string& s) {
  switch(s[pos]) {
    // these *should* all fall through, without a break;
  case '=':
  case '!':
  case '*':
  case '/':
  case '+':
  case '-':
  case '<':
  case '>':
    return true;
    break;
  default:
    return false;
  }

  return false;
}


//
// add dont_rightspace_if_digit_to_right  for ":2", ":-2",  and "-2" ; so they stay together.
//
std::string LoneMathOpAddSpaces(char* dqmap,  std::string tInput, const char* matchme, bool dont_rightspace_if_digit_to_right = false, bool dont_leftspace_if_digit = false) { 

  assert(strlen(matchme)==1); // we only matchme of length 1 at the moment.

  char* repl = (char*)malloc(strlen(matchme)+3);
  sprintf(repl," %s ",matchme);

  std::string tFind(matchme);
  std::string tReplace(repl);

  std::string tDQ(dqmap);

  size_t uPos = 0; 
  size_t uFindLen = tFind.length();
  size_t uReplaceLen = tReplace.length();

  for( ;(uPos = tInput.find( tFind, uPos )) != std::string::npos && uPos < tInput.length(); )
    {
      if (dqmap[uPos] == 'x') {
            uPos +=1;
        continue; // in double quote; don't mess with it.
      }

      // if at . and next char is . , don't sep
      if (tInput[uPos] == '.' && tInput[uPos+1] == '.') {
    uPos +=2;
    continue;
      }

      // if we're at our matchme char and the part of string is a number (-1,1,0,etc), parsable by strtod(), then
      // don't split up, if requsted. This allows "-2" to be parsed as a negative double. It also
      // allows ":4" and ":-1" to be one token.
      if (dont_rightspace_if_digit_to_right) {
    const char* startptr = tInput.c_str()+uPos+1;
    const char* endptr   = startptr;
    strtod(startptr,(char**)&endptr);
    if (endptr && (endptr != startptr)) {
      uPos += ((endptr - startptr) + 1);
      continue;
    }
      }

      // if we're at our matchme character (say it's a dot . ), then if there is a
      // digit to the left, we don't want to split up the number and it's decimal point.
      // hence this option. Which only applies if there is at least one digit to the left.
      if (dont_leftspace_if_digit && uPos>0) {

    const char* startptr = tInput.c_str()+uPos-1;
    const char* endptr   = startptr;

    strtod(startptr,(char**)&endptr);
    if (endptr && (endptr != startptr)) {
      // we saw a digit to the left. In fact, strtod parsed up until endptr
      uPos += (endptr - startptr);
    }
      }

      // don't replace if previous character was {! = + - * / > < }
      if (uPos > 0) {
    if (id_doublechar_eq_op(uPos-1, tInput)) {
      uPos++;
      continue;
    } else if (uPos < tInput.length()-1) {
      // if we're at '=' (new: or our matchme char) and next char is also a special char then don't split up.
      if (id_doublechar_eq_op(uPos+1,tInput)) {
        uPos += 2;
        continue;
      }
    }
      } else {
    // we're at the start
    assert(uPos ==0);
    if (id_doublechar_eq_op(1,tInput)) {
      // don't separate "==...", nor +=, nor =+, etc.
      uPos=2;
      continue;
    }
      }
      
      tInput.replace(uPos, uFindLen, tReplace);
      tDQ.replace(uPos, uFindLen, tReplace);
      uPos += uReplaceLen;
    }
  
  free(repl);
  strcpy(dqmap,tDQ.c_str());
  return tInput;
}



// (a) if we detect '=' then add a space on either side. And don't split up this second case by accident:
// (b) if we detect {==, !=, *=, /=, +=, -=} then add a space on either side too.
// returns true unless expansion overflows BUFSIZ

bool add_spaces_on_either_side_of_assignment_equals(char* lb, char* dqmap) {
  bool ret = true;
  std::string s = lb;
  size_t orig_len = strlen(lb);

  std::string tInput(lb);

  std::string tFind;
  std::string tReplace;

  tFind    = "==";
  tReplace = " == ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "!=";
  tReplace = " != ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "+=";
  tReplace = " += ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "-=";
  tReplace = " -= ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "/=";
  tReplace = " /= ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "*=";
  tReplace = " *= ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = ">=";
  tReplace = " >= ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  tFind    = "<=";
  tReplace = " <= ";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);

  // This one is to allow asignment to negative numbers happen.
  // It has to come after the two char strings above that end in "="
  tFind    = "=-";
  tReplace = "= -";
  tInput = FindAndReplace(tInput, tFind, tReplace, dqmap);


  tInput = LoneMathOpAddSpaces(dqmap, tInput,"=");
  tInput = LoneMathOpAddSpaces(dqmap, tInput,"+");
  tInput = LoneMathOpAddSpaces(dqmap, tInput,"*");
  tInput = LoneMathOpAddSpaces(dqmap, tInput,"/");
  tInput = LoneMathOpAddSpaces(dqmap, tInput,"-",true); // don't space on right if digit to right
  tInput = LoneMathOpAddSpaces(dqmap, tInput,":",true); // don't space on right if digit to right
  tInput = LoneMathOpAddSpaces(dqmap, tInput,",");

  //  tInput = LoneMathOpAddSpaces(dqmap, tInput,".",true,true); // don't space on right if digit to right or left

  
  if (tInput.length() >= BUFSIZ-1) {
    printf("after expansion of spaces around {=,==,!=,*=,/=,+=,-=} we have "
       "overflowed our linebuffer with the result: '%s'.\n\n"
       "Max input line size (pre-expansion) is %d. Max size of an expanded line is %d chars. Originally this line "
       "was (pre-expansion) %ld chars. Please try again with a smaller input line.\n",
       tInput.c_str(),
       BUFSIZ>>1,
       BUFSIZ-2,
       orig_len);

    assert(tInput.length() < BUFSIZ-1);
    exit(1); // can't skip a line in production, oy, so be sure and exit here.
    ret = false;
  }

  strncpy(lb, tInput.c_str(), BUFSIZ-2);
  lb[BUFSIZ-1]='\0'; // be sure it is null terminated.

  return ret;
}

// and if the first thing is *not* a parenthesis, then also 
//  wrap line with ( : line ), so it gets treated as infix.
//
void wrap_with_parens_if_colon_starts(char* lb) {

  // see if first_nonws value on lb is a colon
  char* first_nonws = &lb[0];
  if (!first_nonws) return;

  while(*first_nonws && (*first_nonws == ' ' || *first_nonws == '\t')) {
    ++first_nonws;
  }

  if (!first_nonws) return;

  if (!(*first_nonws)) return;

  //  if (*first_nonws != ':' && *first_nonws != ',' && *first_nonws != '(') return;

  // INVAR: either ':' or ',' or '('

  // addition for lines not starting with '('
  if (*first_nonws == '(' ) return;

  if (strlen(first_nonws)==0) return;

  // non parenthesis starting lines, add parens:
      
  size_t len = strlen(lb);

  // chomp
  while(len > 0 && (lb[len-1]=='\n' || lb[len-1]=='\t' || lb[len-1]==' ')) {
    lb[len-1]='\0';
    len--;
  }
  if (len == 0) return;


  if(len >= BUFSIZ - 4) {
    printf("error: line too long: '%s'; max allowed line is %d characters.\n",lb, BUFSIZ >> 1);
    exit(1);
  }

  assert(first_nonws >= lb+2); // should always be true because we put '  ' (2 spaces) at the start of lb


  *(first_nonws-2)  = '('; // leave the colon in place, so we know to switch (: A B) -> (B A) later.
  *(first_nonws-1)  = ':'; // leave the colon in place, so we know to switch (: A B) -> (B A) later.

  //  *(first_nonwhite)    = ' '; // leave the colon in place, so we know to switch (: A B) -> (B A) later
  lb[len]      = ')';
  lb[len+1]='\0'; // skip the newline now.
}



void add_space_if_parent_colon_starts(char* lb) {

  // see if paren_colon is first, "(:a" ->   "(: a", and "(  :a" -> "(:   a"
  char* paren_first = &lb[0];
  if (!paren_first) return;

  while(*paren_first && (*paren_first == ' ' || *paren_first == '\t')) {
    ++paren_first;
  }

  if (!paren_first) return;

  if (*paren_first != '(') return;

  // INVAR: we had a parenthesis as our first character, at paren_first

  // now see if we have a colon next.
  char* next_colon = paren_first+1;

  // skip spaces
  while(*next_colon && (*next_colon == ' ' || *next_colon == '\t')) {
    ++next_colon;
  }

  // INVAR: next_colon is at the next non-whitespace character after the first open paren
  if (*next_colon != ':') return;

  // we what we came for: condense "(  :" into "(:  ", while handling "(:" as well.
  *next_colon = ' ';
  *paren_first = ' ';
  
  lb[0]='(';
  lb[1]=':';



}

// chompchomment()  ; anything at or after the semicolon is truncated out of existence with an '\0'
//
// only thing difficult is: we cannot be
//  in the middle of a string.
//
//  obviously " starts and stops a string
//  but what about \"? A \" in the middle of a string--it should not terminate the string early.
//   and it should be ignored (an error really) but we ignore \" if not in the middle of a string,
//   because it might be in '\"' as in a reference to the single double quote char.
//
//
// relevant tokens
// 
//     backslash 
//     "  "
//     ;
//
// takes in the Finite state machine:
//
//     1.seen doublequote => in the middle of string so semicolon is ignored
//     2.not seen double quote => not in a string so ; denotes start of comment to end of line.
//     3.seen backslash => if next character is doublequote we are still in same mode as before
//
//  so we need 2 "seen backslash" states - one for "in double quotes", one for 'not in double quote'.
//
// typedef enum chomcommentstate { code, dquote, slashcode, slashdquote } ccs;

void term(char* p) { *p = '\0'; } // helper for chomp_semicolon_to_eol


inline bool is_cppcom(const char* p) {
    if (*p != '/') return false;
    if (*(p+1) != '/') return false;
    return true;
}


void cpp_comment_to_blank(char* s, char* stop) {

    // scan s up to but not including stop, replacing cpp style
    // comments with blanks, but not getting fooled by // inside
    // double quoted strings.
    enum cppcom_st { cpp_code, cpp_dquote, cpp_slashcode, cpp_slashdquote, cpp_comment };

    cppcom_st state = cpp_code;


    const char blank = ' ';

    char* prestop = stop-1;
    char* p = s;
    char  c = *p;

    while(1) {
        c = *p;
        if (c=='\0') return; // always done, no matter state. 

        switch(state) {
        case cpp_code:
            if (p < prestop && *p && *(p+1)) {
                if (is_cppcom(p)) {
                    state = cpp_comment;
                    *p     = blank;
                    *(p+1) = blank;
                    break;
                }
            }
            switch(c) {
            case '\\': state = cpp_slashcode; break;
            case '\"': 
                {
                    state = cpp_dquote;
                }
                break;
            }
            break;
        case cpp_dquote:
            switch(c) {
            case '\\': state = cpp_slashdquote; break;
            case '\"': state = cpp_code; break;
            }
            break;
        case cpp_slashcode:
            state = cpp_code; 
            break;
        case cpp_slashdquote:
            state = cpp_dquote; 
            break;
        case cpp_comment:
            if (c == '\n') {
                state = cpp_code;
                break;
            }
            *p = blank;
            break;
        default:
            // nothing, fine
            break;
        }
        ++p;
    }
}


void chompcomment(char* lb, l3path* dquotemap) {

    // make a version of lb where all the double quotes are just solid xxxxx
    // so we can avoid disrupting strings when we insert spaces.
    dquotemap->init(lb);

    ccs state = code;

    const char bksl = '\\'; // backslash
    const char semi = ';' ; // semicolon
    const char dquo = '\"'; // doublequote
    const char end = '\0'; // end of string

    char* p = lb;
    char  c = *p;
    char* qm = (*dquotemap)();

    while(1) {
        c = *p;
        if (c==end) return; // always done, no matter state. 

        switch(state) {
        case code:
            switch(c) {
            case bksl: state = slashcode; break;
            case semi: return term(p); break;
            case dquo: state = dquote; *qm = 'x'; break;
            }
            break;
        case dquote:
        *qm = 'x';
            switch(c) {
            case bksl: state = slashdquote; break;
            case semi: state = dquote; break;
            case dquo: state = code; break;
            }
            break;
        case slashcode:
            switch(c) {
            case bksl: state = code; break;
            case semi: state = code; break; // the sequence \; will *not* still start a comment- else it leaves us with \newline
            case dquo: state = code; break;
            }
            break;
        case slashdquote:
        *qm = 'x';
            switch(c) {
            case bksl: state = dquote; break;
            case semi: state = dquote; break; // the sequence "   \; " will not start a comment.
            case dquo: state = dquote; break;
            }
            break;
        default:
            // nothing, fine
            break;
        }
        ++p;
    ++qm;
    }
}


void test_chompcomment() {

    char t[10][80] = { "one",
                       "a basic ; commented line",
                       "a \";\" embedded semicolon in dquotes ; commented line",
                       "a \\; commented line"
    };
    l3path dquotemap;
    for (int i = 0; i < 10; i++) {
        printf("\n\nbefore: %s\n",t[i]);
        chompcomment(t[i], &dquotemap);
        printf("\n after: %s\n",t[i]);
    }
}

void test_cpp_comment_to_blank() {

    char t[10][80] = { "one",
                       "a basic // commented line",
                       "a \"// \" embedded semicolon in dquotes // commented line",
                       "a \\// non-commented line",
                       "a \\/// a commented line",
                       ""
    };
    l3path dquotemap;
    for (int i = 0; i < 5; i++) {
        printf("\n\nbefore: %s\n",t[i]);
        cpp_comment_to_blank(t[i],t[i+1]);
        printf("\n after: %s\n",t[i]);
    }
}


// remove trailing whitespace. Return new length of buf.
long chomp(char* lb) {
  long n = strlen(lb)-1; // index of last char
  if (n) {
    while(n>0 && (lb[n]=='\n' || lb[n]=='\t' || lb[n]==' ')) {
      lb[n]='\0';
      n--;
    }
  }

  return n+1;
}

// remove whitespace prefix
long triml(char* lb) {
    long len = strlen(lb);
    char* p = lb + len;
    
    long trimmed = 0;
    char* fnws = lb;
    while (fnws < p && *fnws) {
        if (*fnws == ' ' || *fnws == '\t' || *fnws == '\n') {
            ++fnws;
            --len;
            ++trimmed;
        } else break;
    }
    
    if (fnws > lb) {
        memmove(lb, fnws, len+1);
        p = lb + len;
        *p = '\0';
    }

    return trimmed;
}

long trim(char* lb) {
    long trimmed = 0;
    trimmed += chomp(lb);
    trimmed += triml(lb);
    return trimmed;
}



void test_l3path() {

  l3path a("my big init string!");
  l3path b("");
  l3path c(pwd());
  
  a.dump();
  b.dump();
  c.dump();


}


char* first_nonwhitespace(const char* s) {
  const char* p = s;
  const char* limit = s+BUFSIZ;
  while ((*p == ' ' || *p == '\t') && p < limit) ++p;

  return (char*)p;
}




void test_set_forwarding_bits() {

  l3obj obj;
  bzero(&obj,sizeof(l3obj));

  printf("with all zeroes, is_forwarded_tag(obj)) = %d\n",is_forwarded_tag(&obj));

 //
 // the default... so if _mytag is zero then the object has no default tag for new alloations.
 // say that _mytag is actually forwarded, which means don't tell the tag to delete all at when this object dies.
 //  used by smaller objects (doubles)
  //set_forwarded_tag(obj)    { (obj)->_reserved = set_reserved_bit(RESERVED_FORWARDED_MYTAG, (obj)->_reserved, 1); }
  set_forwarded_tag(&obj);
  printf("after setting, is_forwarded_tag(obj)) = %d\n",is_forwarded_tag(&obj));

 //
 // set the default: (always set for callobjects, closures, and objects that have their own internal heap allocated variables)
/// #define set_newborn_obj_flags(obj) { (obj)->_reserved = set_reserved_bit(RESERVED_FORWARDED_MYTAG, (obj)->_reserved, 0); }
  set_newborn_obj_default_flags(&obj); // for testing purposes in test_set_forwarding_bits()
  printf("after clearing, is_forwarded_tag(obj)) = %d\n",is_forwarded_tag(&obj));

  //uint set_reserved_bit(uint i, uint v, uint store) {
  uint v = 0; // starting value
  uint i = 0;
  uint store = 1;
  uint ans = ((uint)(store << (i) | ((~(1 << (i))) & v)));

  store = 0;
  ans = ((uint)(store << (i) | ((~(1 << (i))) & v)));

  printf("ans is %d\n",ans);
}


// convert a.b.c.d.e   ->  (. a b c d e )
//   space is significant!!  so a.b .c.d.e  ->  (. a b) . (. c d e)
   
bool starts_with_leftparen(const char* input) { 
  l3path s(input);
  s.trim();
  return s.buf[0]=='(';
}

// returns 0 if no dot before first whitespace.
//  else returns pointer to the first dot.   
char* has_dot_before_first_space(char* input ) { 
  char* ws = input;
  char* dot = 0;
  while ( (*ws)  && (*ws != ' ' || *ws != '\t' || *ws != '\n')) {
    if (*ws == '.') {
      dot = ws;
      break;
    }
    ++ws;
  }

  return dot;
}

char* next_space(char* input ) { 
   char* ws = input;
   char* space = 0;
   while ( (*ws)  && (*ws != ' ' || *ws != '\t' || *ws != '\n')) {
      if (*ws == '.') {
        space = ws;
        break;
      }
      ++ws;
   }
   return space;
}




void test_rightel() {

  printf("rightmost is: '%s'\n", rightmost_dot_elem("hi.there.guys"));
  printf("rightmost is: '%s'\n", rightmost_dot_elem("hi"));
  printf("rightmost is: '%s'\n", rightmost_dot_elem("hi.there.guys."));

}




int handle_cmdline_args(int argc, char** argv, ustaq<sexp_t>** plocal_sexpstaq) {

    // just stash them for later, and notice if we have -e
    g_argc = argc;
    g_argv = argv;

    g_cmdline.reinit("");
    for (int j = 0; j < argc; ++j) {
        g_cmdline.pushf("%s ",argv[j]);
    }
    g_cmdline.trimr();
    // check for arguments -e and -r
    long elen = 0;
    double dval = 0;
    bool   is_double = false;

    for (int i = 1; i < argc; ++i) {
        if (0==strcmp(argv[i],"-repl") || 0==strcmp(argv[i],"-r")) {
            // start repl even with -e, after doing -e commands.
            g_have_dash_repl = TRUE;

        } else if (
                   0==strcmp(argv[i],"-sm") 
                   || 0==strcmp(argv[i],"-sf")
                   || 0==strcmp(argv[i],"--stop")
                   ) {

            if (argc <= i+1) {
                fprintf(stderr,"error: no serial number given after command line %s (stop on serial num). aborting.\n",argv[i]);
                exit(1);
            } else {

                elen = strlen(argv[i+1]);
                dval = parse_double(argv[i+1], is_double);

                long   as_integer = (long)dval;
                bool   is_integer = (dval == (double)as_integer);

                if (elen <= 0) {
                    fprintf(stderr,"error: null %s (stop on serial num) command found. aborting.\n",argv[i]);
                    exit(1);
                } else if (!is_double || !is_integer || as_integer <= 0) {
                    fprintf(stderr,"error: %s (stop on serial num) parameter '%s' was not an integer > 0. aborting.\n",
                            argv[i],
                            argv[i+1]);
                    exit(1);
                }
                
                // reuse -sm logic above as --stop for serialfac serial numbers too...
                if (strcmp(argv[i],"--stop")==0) {
                    serialfactory->halt_on_sn(as_integer);
                } else {


#ifndef  _DMALLOC_OFF
                    // store it for later
                    if (0==strcmp(argv[i],"-sm")) {
                        gsermon->stop_set_malloc(as_integer);
                    } else if ( 0==strcmp(argv[i],"-sf")) {
                        gsermon->stop_set_free(as_integer);
                        
                    } else {
                        assert(0);
                    }
#endif
                }

            }

        } else if (0==strcmp(argv[i],"-e")) {

            if (argc > i+1) {
                elen = strlen(argv[i+1]);
                if (elen > L3PATH_MAX) {
                    fprintf(stderr,"error: command too long; -e command must be of length %d or less. We saw length %ld.\n", 
                            L3PATH_MAX, elen);
                    exit(1);
                } else if (elen <= 0) {
                    fprintf(stderr,"error: null -e command found. aborting.\n");
                    exit(1);
                }

                // store it for later
                g_dash_e_cmd.pushf("%s ",argv[i+1]);
                g_have_dash_e = TRUE;

                // don't break to allow multiple -e commands to be pushed on.
            } else {
                fprintf(stderr,"terp error: terp -e not followed by command in command line:\n");

                // re-construct command line for diagnostics
                g_cmdline.errln();
                exit(1);
            }
        } // end if -e
    }

    // check for -e "(some call)" arguments
    if (g_have_dash_e) {
        permatext->add(g_dash_e_cmd());

        long z= 0;
        FILE* ifp = 0;
        queue_up_some_sexpressions(permatext->frame(), &z, *plocal_sexpstaq, glob,0,ifp,0);

        // and mark these as builtins, since they are around all the time while other tests run.
        ustaq<sexp_t>& st = ** plocal_sexpstaq; // convenience reference.
        for (st.it.restart(); !st.it.at_end(); ++st.it) {
            recursively_set_sysbuiltin(*st.it);
        }

        histlog->add_editline_history(g_dash_e_cmd());
        histlog->add(g_dash_e_cmd());
    }

    return (*plocal_sexpstaq)->size();
}

#if 0  // replaced by generalization in call_repl_in_trycatch
// don't require a file handle.
int eval_queued_sexp(ustaq<sexp_t>* local_sexpstaq) {

    volatile FILE *devnull = 0;
    
    XTRY
      case XCODE:
         local_sexpstaq = new ustaq<sexp_t>;
         devnull = fopen("/dev/null","r"); // gives EOF when read.
         repl((FILE*)devnull,local_sexpstaq);
      case XQUITTIN_TIME:
      case XABORT_TO_TOPLEVEL:
      case XBREAK_NEAREST_LOOP:
         XHandled();
         break;
      case XFINALLY:
         fclose((FILE*)devnull);

          if (local_sexpstaq) {
              if (((ustaq<sexp_t>*)local_sexpstaq)->size()) {
                  delete_all_from_sexpstack( (ustaq<sexp_t>*)local_sexpstaq);
              }
          }

         break;
   XENDX

   return 0;
}
#endif

void mtrace_on() {

#ifdef _USE_MTRACE

#ifndef _MACOSX
    setenv("MALLOC_TRACE","/home/jaten/dj/strongref/mtrace.log",1);
    mtrace();
#endif

#endif // _USE_MTRACE

}


// the recommendation is to never call muntrace()
//void mtrace_off() {
//    muntrace();
//}

/****
 * main
 ****/

#ifndef _NOMAIN_

int main(int argc, char **argv) {



    return terp_main(argc, argv);
}

#endif

int terp_main(int argc, char **argv) {


    // where to get input from:
    //  fp = fopen("sexps.in","r+");
    FILE *fp = stdin;

    // allocate glob and main_env, and other startup objects (F, T, nan, NA, etc).
    setup_and_init(&glob);

    // handle_cmdline_args() needs the history setup from setup_and_init()
    ustaq<sexp_t>* local_sexpstaq = new ustaq<sexp_t>;
    handle_cmdline_args(argc, argv, &local_sexpstaq);

    l3obj* env = main_env;

#if 0 // temporarily turn off main.


    // main() function setup
    //    const char* maindefn = "(de main (prop (return rc !< t_dou) (arg argc !> t_dou) (arg argv !> t_vvc)) 0)";
    const char* maindefn = "fn(int !> argc, vec string !> argv, return !< double rc) { rc = 0 }";

  // this seems to leak... not sure why cleanup_sexp() doesn't get, but turn off tracking temporarily
  //  so it doesn't show up as a false positive. was exception throwing issue.
#ifndef  _DMALLOC_OFF
  gsermon->off();
#endif


  l3obj* main_tdop = 0;
  qqchar md(maindefn);
  tdop((l3obj*)&md,0,0,  env,&main_tdop,glob,  0,t_tdo,glob);
  qtree* main_sexp = tdop_get_parsed(main_tdop);

  main_sexp->p();

  l3obj* fun = 0;
  l3path nm("main");
  bool main_done = false;
   XTRY
      case XCODE:
          make_new_function_defn((l3obj*)&nm,-1, main_sexp, env,  &fun, glob, 0, t_fun, glob);
          main_done = true;
          break;
      case XFINALLY:
          if (!main_done) {
              printf("error during initialization of main function: caught exception, aborting.\n");
              exit(1);
          }
          break;
   XENDX


#ifndef  _DMALLOC_OFF
  gsermon->on();
#endif


  set_static_scope_parent_env_on_function_defn(fun,0,0,   env,0,0,   0,0,0);
  add_alias_eno(env, (char*)"main", fun);

  //  global_method_stack.push_front(fun);
  set_sysbuiltin(fun);

#endif   // 0 // temporarily turn off main.

  l3path zmsg(0,"000000 DONE WITH SYSTEM INIT.");
  MLOG_ADD(zmsg());
  
  // actually, we want the user to be able to modify main function. So we *dont mark it builtin*.
  //  set_builtin(llref_from_path("main",main_env,0));

  int ret = 0;

  // decide if its a normal startup (no args), or "-e cmd", or "-e cmd -r"
  if(g_have_dash_repl || !g_have_dash_e) {

      // -e cmds (if any) already queue up.

      // normal repl startup
      ret = call_repl_in_trycatch(fp,&local_sexpstaq,true,env, glob);
      
  } else {
      //
      // -e without -r :
      //
      //      just do the -e command, then quit.
      //

      ret = call_repl_in_trycatch(0,&local_sexpstaq,false,env, glob);
  }

  printf(" [done]\n"); // get a nice next shell prompt.

  // call_repl_in_trycatch should delete and zero this. but
  // on exception sometimes it isn't happening?!?!?
  if(local_sexpstaq) {
      if(local_sexpstaq->size()) {
          delete_all_from_sexpstack(local_sexpstaq);
      }
      delete local_sexpstaq;
      local_sexpstaq =0;
  }

  zmsg.reinit("000000 DONE WITH USERCODE AND DOING CLEANUP");
  MLOG_ADD(zmsg());
  

  DV(gdump()); // is there anything left on the global_tag_stack, that should
  // not be?  b/c this next statement is seg faulting.

  teardown(fp,glob);


  return 0;
}


//
// parse into a series of sexpression, pushed onto the stk. Incomplete/remaining is indicated by *z not advancing
//  the full length of start -> '\0'.
//
//  long *z = increment *z by num char consumed.
//
// char* start = start parsing from here.
//
// stk = push_back sexpressions as they are found, caller will then pop_front to get the
//           next to evaluate, and  should call destroy_sexp(x) on each x obtained from stk. retown owns them.
//
void queue_up_some_sexpressions(char* start, long* z, ustaq<sexp_t>* stk, Tag* retown, l3obj* env, FILE* ifp, l3path* prompt) {
    LIVET(retown);

    long start_size = stk->size();
    assert(0 == start_size);

    sexp_t*  actualexp = 0;  // actual vs the (struct ustaq<qtree>::ll *) that we iterate through in the stk.
    long nchar_consumed = 0;

    l3obj* my_tmp_tdop = 0;

    parse_cmd cmd;
    cmd._rstk = stk;
    cmd._fp   = ifp;
    cmd._prompt = prompt;
    cmd._filename = "(prompt input)";
    parse_sexp(start, strlen(start), retown, cmd, &nchar_consumed, env, &my_tmp_tdop,ifp);

    // transfer ownership to retown so they outlast the tdop if need be, and we'll delete them
    // when done with parsing, one by one.
    long newsize = stk->size();
    if (newsize > start_size) {
        ustaq<sexp_t>::ll* cur = stk->head();
        ustaq<sexp_t>::ll* last = stk->tail();

        while(1) {
            actualexp = cur->_ptr;
            transfer_subtree_to(actualexp, retown);

            if (cur == last) break;
            cur = cur->_next;
        }
    }

    if (my_tmp_tdop) {
        generic_delete((l3obj*)(my_tmp_tdop),  -1,0,  0,0,retown,  0,0, retown,ifp);
    }
    *z += nchar_consumed;
}

   
l3obj* repl_qtree(FILE* fp, ustaq<qtree>* local_qtreestaq) {

    return 0;
}


l3obj* repl(FILE* ifp, ustaq<sexp_t>* local_sexpstaq, l3obj* env, Tag* owner) {
  
  char *status = 0;
  sexp_t *sx = NULL;
  l3path testline;
  l3path lastline;
  l3path prompt;
  l3path utcomment;
  long strlen_rl = 0;
  int   feof_fp = 0;

  volatile bool  already_builtin = false;
  
  // since repl needs to be re-entrant, we can't use the global sexpstaq
  // instead have a local one, passed in from caller.

  int z = 0; // linebuf offset, to alloc accumulation of multiple lines.

  DV(printf("home length is : %ld\n", home.len()));

  while(1) {
    utcomment.clear();

    cur_method.init(pwd()+home.len()-1);

    if (enDV) {
      printf("\nhere is the current env:\n\n");
      print(env,"",0);
    }

    const char* cmd_num_str = repl_value_history_l3obj->nextstr();

    l3path env_nm(env->_varname);
    env_nm.rdel(16); // delete characters from the right, to get the env name without the pointer part.


  CONTINUE_PARTIAL:

    //    prompt.reinit("pid::%d   obj::%s   mthd::%s   %s%s> ",
    prompt.reinit("pid::%d   obj::%s   %s%s> ",
       pid, 
       env_nm(),
       // met_nm(),
       cmd_num_str,
       (z==0) ? "" : "+"
    );

    if (unittest_max > 0 && unittest_cur <= unittest_max) {
      // continue automated unittest sequence.
        utest.pick_test(unittest_cur, &testline);
        unittest_cur++;
        long utn = unittest_cur-1;

        testline.trim();
        utcomment.reinit("%s",testline());
        utcomment.trim();
        if (utcomment.len() == 0) continue; // don't bother with blank lines.
        utcomment.pushf(" // (ut %ld %ld)",utn, utn);

        permatext->add(testline());
        strcpy(linebuf+z,testline());
        testline.init("");
        printf("\n*** ut %ld testing this code: %s // (ut %ld %ld)\n",utn,linebuf,utn,utn);
        status = linebuf+z;
        chomp(linebuf);
        histlog->add_editline_history(utcomment()); // adds to editline history, not our file.

    } else { // not doing ut

        if (local_sexpstaq && local_sexpstaq->size() == 0) {

            if (ifp) {

                // ***   the main call to    get_more_input()    is here ***
                strlen_rl = get_more_input(ifp, &prompt, true /*show_prompt_if_not_stdin*/, true /*echo_line*/, false /*show_plus*/, feof_fp);
                strcpy(linebuf+z, permatext->frame());
                status = linebuf+z;

            } else {
                assert(ifp==0);
                assert(local_sexpstaq->size() == 0);
                return 0;
            }

        } else {
            // else local_sexpstaq->size() != 0 and so we have an expression to process.
            status = linebuf+z;      
        }

    } // end else not doing ut

    if (ifp && feof(ifp) != 0) {
        return 0;
        // don't raise quit, we might be processing a (src file) command and want to return to
        //  the toplevel afterwards.
        //      l3throw(XQUITTIN_TIME);
    }

    if (status == 0) {
        // happens when we got SIGINT during read.
        z = 0;
        printf("\n");
        continue;
    }
    
    /* if not EOF and status was NULL, something bad happened. */
    if (status != linebuf+z) {
        printf("Error encountered on fgets.\n");
        l3throw(XQUITTIN_TIME);
    }

    // write history ASAP! i.e. before we chomp comments or do any other mods
    //  but if we are from a unittest ut line, then note that in the history
    if (utcomment.len()) {
        histlog->add(utcomment());
    } else {
        histlog->add(linebuf+z);
    }

    if(strlen(linebuf) >= BUFSIZ - 4) {
        // only allow half the buffer as a line len...other half is for expansion (insertion of spaces).
        printf("error: line too long: '%s'; max allowed line is %d characters.\n",linebuf, BUFSIZ >> 1);
        l3throw(XQUITTIN_TIME);
    }


    char* bol  = permatext->frame();
    long  blen = permatext->len();
    if (0==blen && (local_sexpstaq == 0 || local_sexpstaq->size()==0)) continue;


    Tag* tag_to_use = env->_mytag;

       if (!tag_to_use) { 
          tag_to_use = glob; 
          LIVET(tag_to_use);
       } else {
          LIVET(tag_to_use);
       }


    if (local_sexpstaq->size() == 0) {
        long consumed = 0;
        queue_up_some_sexpressions(bol, &consumed, local_sexpstaq, tag_to_use, env, ifp, &prompt);

        if (consumed) {
            assert(consumed > 0);
            qqchar s(bol, bol + consumed);
            lastline.reinit(s);
            strcpy(&linebuf[0], &bol[consumed]);
        } else {
            // apparently nothing was queued... verify 
            assert(local_sexpstaq->size() == 0);
        }

    }

    z = strlen(linebuf);
            
    if (local_sexpstaq->size()) {
        if (sx) {
            destroy_sexp(sx);
        }
        sx = local_sexpstaq->pop_front();

    } else {
        sx = 0; // probably have to read more first.

        // probably incomplete s-exp, and we need more input.
        goto CONTINUE_PARTIAL;
    }
  




    // now that we have retown, servers (functions) that wish to 
    //  set a new retval should just do so, using retown to own the allocation.
    //
    l3obj* retval = 0;

    long arity = num_children(sx);

    // partial parses already reflected above.
    //    z =0;

    // main REPL loop eval here:

    LIVEO(env); // sanity check we are passing in a good env.

       XTRY
           case XCODE:
               // allow this node to direct expression without causing leak alarms.
               already_builtin = is_sysbuiltin(sx);
               if (!already_builtin) {
                   recursively_set_sysbuiltin(sx);
               }

               ///*** the main eval is here ***////
               eval(0,arity,sx,env, &retval, tag_to_use, 0,0,tag_to_use,ifp);
               break;
          
           case XFINALLY:
               // and turn off that leak exclusion now. Unless it wasn't need in the first place--for -e cmd lines args, for example.
               if (!already_builtin) {
                   recursively_set_notsysbuiltin(sx);
               }

               // cleanup, even if we throw.
               if (sx) {
                   if (tag_to_use->sexpstack_exists(sx)) {
                       // specify which tag to delete from.
                       recursively_destroy_sexp(sx, tag_to_use);
                   }
                   sx=0;
               }
           break;
       XENDX

          if (retval) {
              printf("\n = ");
              repl_value_history_l3obj->push(retval);
              print(retval,"",0);
          }
          printf("\n");
          histlog->last_cmd_had_value(&lastline, &retval, owner);
          
          if (z > 0) {
              goto CONTINUE_PARTIAL;
          }

          
          fflush(stderr);
          bzero(linebuf,BUFSIZ);
          
          if (global_quit_flag) {
              l3throw(XQUITTIN_TIME);
          }

  } // end while(1)

  return 0;
}


// allow the parser to call for additional input if the given expression
// is incomplete. new input is stored in PermaText, naturally.
//

long get_more_input(FILE* ifp, l3path* prompt, bool show_prompt_if_not_stdin, bool echo_line, bool show_plus, int& feof_fp) {

    char* rl = 0;
    long  getmo = 0;
    feof_fp = 0;
    l3path pr;
    if (prompt) {
        pr.reinit((*prompt)());
        if (show_plus) {
            pr.rdel(2);
            pr.pushf("+> ");
        }
    }


            if (ifp == stdin) {
                rl = histlog->readline(pr());
                if (0 == rl) {
                    if (ifp == stdin) {
                        global_quit_flag=true;
                        l3throw(XQUITTIN_TIME);
                    }
                    return 0;
                }
                if (rl && *rl) { 
                    histlog->add_editline_history(rl); // adds to editline history.
                } else {
                    // we could be at eof
                    if (feof(ifp)) {
                        if (ifp == stdin) {
                            global_quit_flag=true;
                            l3throw(XQUITTIN_TIME);
                        }
                        return 0;
                    }
                    // nothing back, or zero len back.
                    if (rl) { free(rl); }
                    permatext->add_newline();
                    return permatext->len();
                }
                permatext->add(rl);
                free(rl);
                return permatext->len();

            } else if (ifp) {
                // ifp != stdin
                // reading from file instead of stdin. No need to print prompt...but might make things clearer.

                // don't want to print prompt if we are just doing -e cmd and then done.
                //  so we need to force EOF if it's there (for -e, ifp = /dev/null )
                ungetc(fgetc(ifp),ifp);
                feof_fp = feof(ifp);
                if (feof_fp != 0) {
                    return 0;
                }

                if (show_prompt_if_not_stdin) {
                    printf("%s",pr());
                }
                getmo = permatext->add_line_from_fp(ifp);
                
                if (getmo) {
                    if (echo_line) {
                        printf("%s\n",permatext->frame());
                    }
                    histlog->add_editline_history(permatext->frame()); // adds to editline history, not any file.
                }
                return getmo;
            }

            return 0;

} // end get_more_input



//
// two ways to passing input: in text form from FILE* ifp,  or in already parse form in local_sexpstaq
//
//  *pp_sexpstaq : pass in an pointer to zero, or non-empty pointer to pre-existing staq with
//    loaded commands (e.g. from -e command line commands, or elsewhere). Upon exit, we
//    guarantee we will delete the staq and zero *p_sexpstaq; so the client need not cleanup / delete.
//
int call_repl_in_trycatch(FILE* ifp, ustaq<sexp_t>** pp_sexpstaq, bool loop_until_quit, l3obj* env, Tag* retown) {

  int volatile res = 0;

  DV(printf("call_repl_in_trycatch(): at the top, before the try block.\n"));

  volatile ustaq<sexp_t>** ppv_sxs = 0;
  volatile ustaq<sexp_t>*  vsxs = 0;



  do {

   XTRY
      case XCODE:

          DV(printf("call_repl_in_trycatch(): starting a fresh repl in XCODE.\n")); 

          ppv_sxs = (volatile ustaq<sexp_t>**)pp_sexpstaq;

          //
          // set vsxs
          //
          if (0==ppv_sxs) {
              // client passed us zero; no input sexps.
              vsxs = (volatile ustaq<sexp_t>*)(new ustaq<sexp_t>);
          } else {
              // client passed us input sexp_t's (or possibly an empty container) to consume and trash when we are done.
              vsxs = *ppv_sxs;
              if (0==vsxs) {
                  vsxs = (volatile ustaq<sexp_t>*)(new ustaq<sexp_t>);
              }
          }
          assert(vsxs);

          // is this correct? hmm... it fires on a pre-loaded -e staq of commands, so I don't think it can be right.
          // assert(((ustaq<sexp_t>*)vsxs)->size()==0);

      // a target for XPOPTOP handler to restart at...
      GO_REPL:

          //**********  START REPL HERE *********//
          repl(ifp,(ustaq<sexp_t>*)(vsxs), env, retown);

         break;
     case XABORT_TO_TOPLEVEL:

         DV(printf("call_repl_in_trycatch(): we see XABORT_TO_TOPLEVEL!\n"));
         XHandled();
         break;

      case XBREAK_NEAREST_LOOP:
          printf("error: break_ invoked without enclosing loop. Going to toplevel.\n");
          XHandled();
          break;

      case XCONTINUE_LOOP:
          printf("error: continue invoked without enclosing loop. Going to toplevel.\n");
          XHandled();
          break;

      case XQUITTIN_TIME:
          XHandled();
          loop_until_quit = false;
          break;

      case XUSER_EXCEPTION:
          XHandled();
          printf("error: got to top level with XUSER_EXCEPTION not handled.\n");
          break;

      case XCD_DOT_DOT:
          printf("at global top.\n");
          XHandled();
          goto GO_REPL;
          break;

      case XPOPTOP:
          printf("at global top.\n");
          XHandled();
          goto GO_REPL;
          break;

      default:
          printf("error: Aborting on unknown exception that reached toplevel: %d \n",XVALUE);
          assert(0);
          exit(1);
         break;

      case XFINALLY:
          DV(printf("call_repl_in_trycatch(): we are doing our XFINALLY BLOCK.\n"));

          assert(vsxs);
          if (((ustaq<sexp_t>*)(vsxs))->size()) {
              delete_all_from_sexpstack( (ustaq<sexp_t>*)(vsxs));
          }
          delete vsxs;
          vsxs = 0;
          // and check if we should zero the client's passed in value, so that reference is gone.
          if (ppv_sxs) {
              *ppv_sxs = 0;
          }


         break;
   XENDX

   } while(loop_until_quit); // end do 

  //   printf("call_repl_in_trycatch(): This follows the XENDX try-catch block...!!! with res = %d\n",res);

   return res;
}


   //
   // traverse the tree indicated by owner, pushing object pointers onto the back of the ptrvec
   //  in the obj slot. curfo is not included (the root of the search) if encountered.
   //
L3METHOD(dfs_enumerate_owned)
   assert(obj);
   assert(obj->_type == t_vvc);

   // new way: with JudyL instead of set

   Tag* target = owner;

   // depth-first visit child tags, passing obj to each to accumulate owned l3obj*
    Tag* childtag = 0;
    for(target->subtag_stack.it.restart(); !target->subtag_stack.it.at_end(); ++target->subtag_stack.it) {
        childtag = *target->subtag_stack.it;
        dfs_enumerate_owned(obj,arity,exp,env,retval,childtag,curfo,etyp,retown,ifp);
    }

   // then enumerate my own person ownees.
   Word_t * PValue;
   Word_t Index = 0;
   long i = 0;
   l3obj* o = 0;

   JLF(PValue, target->_owned, Index);
   while (PValue != NULL)
       {
           o = ((l3obj*)Index);
           if (o != curfo) {
               ptrvec_pushback(obj,o);
           }
           JLN(PValue, target->_owned, Index);
           ++i;
       }



L3END(dfs_enumerate_owned)


// (ownlist obj) returns a ptrvec of objects owned by obj
L3METHOD(get_ownlist)

    if (arity != 1) {
        printf("arity error in ownlist: only arity 1 supported. usage: (ownlist owning_object_whose_contents_are_desired).\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }

   qqchar v = ith_child(exp,0)->val();
   llref* innermostref = 0;
   l3obj* found = RESOLVE_REF(v, env, AUTO_DEREF_SYMBOLS, &innermostref, 0, v, UNFOUND_RETURN_ZERO);

   if (!found) {
       std::cout << "error in ownlist: subject '" << v << "' not found.\n";
       l3throw(XABORT_TO_TOPLEVEL);
   }

   if (!found->_mytag) {
       std::cout << "error in ownlist: subject '" << v << "' does not own anything; has no _mytag.\n";
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3obj* vv = 0;
   make_new_ptrvec(0,-1,0,env, &vv, retown, curfo, 0,retown,ifp);

   dfs_enumerate_owned(vv,-1,0,  env,retval,
                       found->_mytag,   // owner = this is the tag to enumerate owned pointers from itself and all subtags
                       found,           // curfo = exclude this root of the search if encountered in subtags (e.g. for captags)
                       0,retown,ifp);

   *retval = vv;
   DV(if (vv) {
           print(vv," get_ownlist returned: ",0);
       });

L3END(get_ownlist)



L3METHOD(member_a_of_b)

    if (arity != 2) {
        printf("arity error testing member_a_of_b: only arity 2 supported. usage: (member needle haystack).\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }

    l3obj*  argsvv = 0;

    l3obj*  needle   = 0;
    l3obj*  haystack = 0;

    volatile bool done_with_code = false;
    volatile bool found = false;
    l3obj* dv = 0;
    double dneedle = 0;

  XTRY
     case XCODE:

     eval_to_ptrvec(L3STDARGS);

     argsvv = *retval;

     ptrvec_get(argsvv,0, &needle);
     ptrvec_get(argsvv,1, &haystack);

     assert(needle);
     assert(haystack);

     // automatically fail if needle has size 0 -- put in to handle (in [] 3), but 
     //   also fails on objects, so that won't work.

     // automatically fail if haystack has size 0
     if (ptrvec_size(haystack)==0) {
         *retval = gnil;
         done_with_code = true;
         break;
     }

     if (needle->_type == t_obj) {
         // just use the address of needle directly
         found = ptrvec_search(haystack, needle);

             if (found) {
                 *retval = gtrue;
             } else {
                 *retval = gnil;
             }
             done_with_code = true;
             break;
     }

     if (needle->_type == t_dou) {
         if (haystack->_type == t_dou) {

             // they agree, no coercion necessary
             dneedle = double_get(needle,0);
             found = double_search(haystack,dneedle);

             if (found) {
                 *retval = gtrue;
             } else {
                 *retval = gnil;
             }
             done_with_code = true;
             break;
         }

         // needle is double, could haystack be coerced?
         if (haystack->_type == t_vvc) {

             // linear search, with value comparison on double objects.
             found = ptrvec_search_double(haystack, needle);

             if (found) {
                 *retval = gtrue;
             } else {
                 *retval = gnil;
             }
             done_with_code = true;
             break;
         }
      } // end if needle double


         // fallback: try to do a pointer comparison
         found = ptrvec_search(haystack, needle);

         if (found) {
            *retval = gtrue;
         } else {
            *retval = gnil;
         }
     
         done_with_code = true;

     break;
     case XFINALLY:
            if (argsvv) { generic_delete(argsvv,L3STDARGS_OBJONLY); }
            if (dv)     { generic_delete(dv,    L3STDARGS_OBJONLY); }

     break;
  XENDX

L3END(member_a_of_b)



L3METHOD(vv_to_doublev)

  l3obj* vv = obj;
  l3obj* res = 0;
  volatile bool done_with_code = false;
  l3obj* tmp = 0;
  double dtmp = 0;
  long N = ptrvec_size(vv);

  XTRY
     case XCODE:

       res = make_new_double_obj(NAN,retown,"vv_to_doublev_res");
       for (long i = 0; i < N; ++i) {
           ptrvec_get(vv,i,&tmp);
           if (tmp->_type != t_dou) { 
               printf("error in create vector: element %ld was not of type double, but rather %s.\n",i+1,tmp->_type);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           dtmp = double_get(tmp,0);
           double_set(res,i, dtmp);
       } 

       *retval = res;
       done_with_code = true;

     break;
     case XFINALLY:
           if (!done_with_code) { 
               if (res) { generic_delete(res,L3STDARGS_OBJONLY);  }
           } else {
               *retval = res;
           }
     break;
  XENDX


L3END(vv_to_doublev)


L3METHOD(vv_to_stringv)
{
    l3obj* vv = obj;
    l3obj* res = 0;
    volatile bool done_with_code = false;
    l3obj* tmp = 0;
    long N = ptrvec_size(vv);
    l3path stmp;
    qqchar empty;
    
    XTRY
     case XCODE:

       res = make_new_string_obj(empty,retown,"vv_to_stringv_res");
       for (long i = 0; i < N; ++i) {
           ptrvec_get(vv,i,&tmp);
           if (tmp->_type != t_str) { 
               printf("error in create vector: element %ld was not of type string, but rather %s.\n",i+1,tmp->_type);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           stmp.clear();
           string_get(tmp,0,&stmp);
           string_set(res,i, stmp());
       } 
       
       *retval = res;
       done_with_code = true;
       
     break;
     case XFINALLY:
           if (!done_with_code) { 
               if (res) { generic_delete(res,L3STDARGS_OBJONLY);  }
               *retval = gnil;
           }
     break;
  XENDX

}
L3END(vv_to_stringv)


L3METHOD(create_vector)

    arity = num_children(exp);
    l3obj* tmpvv = 0;
    bool done_with_code = false; 

    XTRY
       case XCODE:

       if (arity > 0) {

           eval_to_ptrvec(obj,arity,exp,env,retval,owner,curfo,0,retown,ifp);

           // if they are all the same type, then reduce to a single array of that type...
           l3obj* first_obj = 0;
           l3obj* next_obj = 0;
           ptrvec_get(*retval,0,&first_obj);
           t_typ first_type = first_obj->_type;
           bool all_same_type = true;
           for (long i = 1; i < arity; ++i ) {
               next_obj = 0;
               ptrvec_get(*retval,i,&next_obj);
               if (next_obj->_type != first_type) {
                   all_same_type = false;
               }
           }
           if (all_same_type) {
               if (first_type == t_dou) {

                   l3obj* old_ptr_vec = *retval;
                   vv_to_doublev(*retval,-1,0,env, retval,owner,curfo,0,retown,ifp);
                   if (old_ptr_vec != *retval) {
                       generic_delete(old_ptr_vec,L3STDARGS_OBJONLY);
                   }

               } else if (first_type == t_str) {
                   
                   l3obj* old_ptr_vec = *retval;
                   vv_to_stringv(*retval,-1,0,env, retval,owner,curfo,0,retown,ifp);
                   if (old_ptr_vec != *retval) {
                       generic_delete(old_ptr_vec,L3STDARGS_OBJONLY);
                   }
               }
           }

        } else {

           // arity == 0; empty vector
           *retval = make_new_class(0,retown,"double","create_vector_empty_vec");
           (*retval)->_type = t_dou;
        }
       done_with_code = true;

     break;
     case XFINALLY:
           if (!done_with_code) { 
               if (*retval) { 
                   generic_delete(*retval,L3STDARGS_OBJONLY);
                   *retval = gnil;
               }
           }
           if (tmpvv) { generic_delete(tmpvv,L3STDARGS_OBJONLY); }
     break;
  XENDX

    
L3END(create_vector)


// arity specifies k, number of expected args
// specify t_lit for etyp to allow the references to be currently non-existant paths
//
L3METHOD(k_arg_op)

   long k = arity;
   assert(k > 0);
   arity = num_children(exp);
   //arity = first_child_chain_len(exp);
   l3path sexps(exp);

   if (arity != k) {
     if (k==1) { 
         std::cout << "error in arity: " << exp->headval(); printf(" requires exactly one argument; '%s' had %ld args.\n",sexps(),arity);
     } else {
         std::cout << "error in arity: " << exp->headval();  printf(" requires exactly %ld arguments; '%s' had %ld args.\n",k,sexps(),arity);
     }
     l3throw(XABORT_TO_TOPLEVEL);
   }

    l3obj* vv = 0;
    eval_to_ptrvec(obj,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);

    long N = ptrvec_size(vv);
    assert(N == k);

    // screen for unresolved references
    l3obj* ele = 0;


   // skip t_lit checks if caller indicated they are willing/expecting some literals
   if (etyp != t_lit) {

       for (long i = 0; i < N; ++i) {
           
           ptrvec_get(vv,i,&ele);
           if (ele->_type == t_lit) {
               
               l3path litstring;
               literal_get(ele,&litstring);
               printf("error: could not resolve literal '%s' in expression '%s'.\n",litstring(),sexps());
               generic_delete(vv, L3STDARGS_OBJONLY);
               l3throw(XABORT_TO_TOPLEVEL);
           }
       }
   }

   *retval = vv;

L3END(k_arg_op)


L3METHOD(any_k_arg_op)

   arity = num_children(exp);
   l3path sexps(exp);

   if (arity < 1) {
       std::cout << "error in arity: " << exp->val() << " requires at least one argument;"; printf(" '%s' had %ld args.\n", sexps(),arity); 
       l3throw(XABORT_TO_TOPLEVEL);
   }

    l3obj* vv = 0;
    eval_to_ptrvec(obj,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);

    long N = ptrvec_size(vv);

    // screen for unresolved references
    l3obj* ele = 0;

    for (long i = 0; i < N; ++i) {

        ptrvec_get(vv,i,&ele);
        if (!ele) {
                printf("error in any_k_arg_op(): could not resolve %ld-th argument in in expression '%s'.\n",i,sexps());
                generic_delete(vv, L3STDARGS_OBJONLY);
                l3throw(XABORT_TO_TOPLEVEL);
        }
        if (ele->_type == t_lit) {
            l3path litstring;
            literal_get(ele,&litstring);

            // check if it is a type expression...
            quicktype* qt = 0;
            t_typ ty = qtypesys->which_type(litstring(), &qt);
            if (ty) {
                // okay, allow type literals.
                
            } else {

                printf("error: could not resolve literal '%s' in expression '%s'.\n",litstring(),sexps());
                generic_delete(vv, L3STDARGS_OBJONLY);
                l3throw(XABORT_TO_TOPLEVEL);
            }
        }
    }

    *retval = vv;

L3END(any_k_arg_op)


   // called by "len"
L3METHOD(eval_len)

   k_arg_op(obj,1,exp,L3STDARGS_ENV);

   l3path sexps(exp);

   l3obj* vv  = *retval;

   l3obj* ele0 = 0;
   ptrvec_get(vv,0,&ele0);

   /*
   if (ele0->_type == t_lit) {
     l3path litstring;
     literal_get(ele0,&litstring);
     printf("error in len: literal '%s' in expression '%s' had no actual value; could not resolve in current env.\n",litstring(),sexps());
     generic_delete(vv, L3STDARGS_OBJONLY);
     l3throw(XABORT_TO_TOPLEVEL);
   }
 */

   long   sz = ptrvec_size(ele0);

   l3obj* len = make_new_double_obj((double)sz, retown,"len");
   generic_delete(vv, L3STDARGS_OBJONLY);
   *retval = len;
L3END(eval_len)


L3METHOD(strlen)

   print_list_to_string(0,arity,exp,L3STDARGS_ENV);
   l3obj* vv = *retval;
   l3path s;
   string_get(vv,0,&s);
   long slen = strlen(s());

   generic_delete(vv, L3STDARGS_OBJONLY);

   *retval = make_new_double_obj((double)slen, retown, "strlen");

L3END(strlen)

   /*

     "Resource theorem":

     To maintain the tag tree during transfer:

     In the call: (transfer target receiver) :
     
     if target owns receiver (where owns includes recursively down the ), then forbid transfer.
     
     but a faster way to check the same thing:

     if receiver is a tag-descendant of target, then forbid transfer.
     i.e. if we find the nca of bottom=new_owner and top=transfer_me and the result is transfer_me, then stop.
     i.e. call:     if (owns obj=new_owner curfo=transfer_me) then reject.
    */

   // TODO: add owns() check to avoid creating cycles;
   //  i.e  owns() will call compute_nca() // (owns   ownee   owner_to_test_if_it_owns_ownee_directly)

// old: (transfer  obj  newowner "newname") ; newowner must be an object.
//  new, I don't think "newname" is supported below: (transfer  obj  newowner) ; newowner must be an object.
L3METHOD(transfer)

   l3obj* vv  = 0; 
   // specify t_lit for etyp to allow the references to be currently non-existant paths
   k_arg_op(0,2,exp,env,&vv,owner,curfo,t_lit,retown,ifp);
   l3path sexps(exp);


   l3path s_trme(ith_child(exp,0));
   l3path s_nown(ith_child(exp,1));

   l3obj* transfer_me  = 0;
   l3obj* new_owner    = 0;
   l3obj* makes_cycle = 0;
   Tag* newown_tag = 0;
   Tag* tm_own = 0;
   Tag* tm_own_par = 0;
   llref* tmref = 0;
   l3path bname;

  XTRY
     case XCODE:

        ptrvec_get(vv,0,&transfer_me);
        ptrvec_get(vv,1,&new_owner);

       // if sealed, don't do it.
       if (is_sealed(transfer_me) && !global_terp_final_teardown_started) {
           printf("error in transfer: object '%s' is sealed and cannot be moved.\n",s_trme());
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
           l3throw(XABORT_TO_TOPLEVEL);
       }
     
       tm_own = transfer_me->_owner;
       tm_own_par = tm_own->parent();
       assert(tm_own_par);

       // default, but to be corrected below if need be.
       bname.reinit(s_trme.dotbasename());

       // figure out newown_tag, allowing t_lit

       if (new_owner->_type == t_lit) {
           // allow one level undefined. but previous level back better be okay!
           l3path str;
           string_get(new_owner, 0, &str);

           bname.reinit(str.dotbasename());
           l3path up_one(str);
           up_one.pop_dot();

           l3obj* found = 0;
           llref* innermostref = 0;
           objlist_t* envpath=0;
           found = RESOLVE_REF(up_one(),env,AUTO_DEREF_SYMBOLS, &innermostref, envpath,up_one(),UNFOUND_RETURN_ZERO);       

           if (!found) {
               printf("error in transfer: problem with destination '%s': could not find enclosing environment '%s'.\n",
                      s_nown(), up_one());
               if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
               l3throw(XABORT_TO_TOPLEVEL);
           }
           new_owner = found;
           newown_tag = new_owner->_mytag;
       } else {
           // hmm... as long as pre-existing is an object, take it as the new_owner

           //       l3path up_one(s_nown);
           //       up_one.pop_dot();

           if (new_owner->_type != t_obj) {
               printf("error in transfer: problem with fully resolved destination env in expression '%s': '%s' was not an object.\n",
                      sexps(), s_nown.dotbasename());
               if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
               l3throw(XABORT_TO_TOPLEVEL);
           }

           newown_tag = new_owner->_mytag;
       }


#if 0
   // I think we can skip this check now... but not sure.
   if (new_owner->_type != t_obj 
       || newown_tag == 0 
       || is_forwarded_tag(new_owner) 
       ) {
       printf("error: transfer unsuccessful; new owner '%s' was not an object with its own tagowner.\n",s_nown());
           generic_delete(vv, L3STDARGS_OBJONLY);
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
           *retval = gnil;
           return 0;
   }
#endif

   //l3obj* newown_tag_cap = newown_tag->captain();

   // check for transfer to same place...
   if (newown_tag == tm_own) {
       printf("error in transfer: destination same as source, in expression '%s'.\n", sexps());
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       l3throw(XABORT_TO_TOPLEVEL);
   }


   // forbid transfer to child, which would create a cycle, and sadly result in disappearing both objects.
   // i.e. if (owns obj=transferme curfo=new_owner) then reject.
   owns(new_owner, -1,0,  env,&makes_cycle,owner,  transfer_me,0,owner,ifp);
   if (makes_cycle == gtrue) {
       printf("error in transfer: cycle producing transfer rejected: destination is a descendant of target-to-move, in expression '%s'.\n", sexps());
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
       l3throw(XABORT_TO_TOPLEVEL);
   }

       // upon transfer, wipe out previous refs (conjecture: try this policy, it might be good b/c it may reduce cognitive load on callers/callees)
       tmref = transfer_me->_owner->find(transfer_me);
   
       // but do leave the owner with its own reference...
       llref_del_ring(tmref, DELETE_ALL_EXCEPT_PRIORITY_ONE, YES_DO_HASH_DELETE);

       if (!is_forwarded_tag(transfer_me) && pred_is_captag(tm_own, transfer_me)) {

           // for captags, move the tag; that should be all we need to do.
           tm_own_par->subtag_remove(tm_own);
           newown_tag->subtag_push(tm_own);
           tm_own->set_parent(newown_tag);

       } else {
           // this won't work for cap-tags, where we want to move the captag tag without messing with the cap<->tag relationship.
           transfer_me->_owner->generic_release_to(transfer_me, newown_tag);
       }

         add_alias_eno(new_owner, bname(), transfer_me);

         // this is critical to be able to resolve references in the new env.. but doesnt seem to work., because it's not being called on (de f.h ...)
         transfer_me->_parent_env = new_owner;

         if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv=0; }
         *retval = gtrue;

     break;
     case XFINALLY:
             if (vv) {  generic_delete((l3obj*)vv, L3STDARGS_OBJONLY);  }
     break;
  XENDX


L3END(transfer)

#if 0
   //L3KMINMAX(unittest,2,3)

    // unittest
    if (v.strcmp("ut") == 0) {
      unittest_max = 1; // start testing if over 0, and go to test_max.
      unittest_cur = 0; // reflects current test number
      
      int nc = num_children(exp);
      bool is_double = false;
      double dval = 0;

      if (nc > 3) {
    printf("unittest specification error: More than two arguments given. Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
    return 0;   
      } else if (nc <= 1) {
    printf("unittest specification error: No last_test_num (an natural number) given. Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
    return 0;
      }
      else  if (nc >= 2) {
           is_double = false;
       dval = parse_double(ith_child(exp,1)->val, is_double);
       if (!is_double || (is_double && dval < 0)) {
         printf("unittest specification error: last_test_num was not a natural number. Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
         return 0;
       }
       
       unittest_max = floor(dval);
       unittest_cur = 0; // reflects current test number
       
       if (nc ==2) {
                printf("(ut %ld) requested: running unit tests  #%ld  -  #%ld.\n",unittest_max, unittest_cur, unittest_max);
        return 0;
       } else {
           assert(nc == 3);
           is_double = false;
           dval = parse_double(ith_child(exp,2)->val, is_double);
           if (!is_double || (is_double && dval < 0)) {
           printf("unittest specification error: first_test_num was not a natural number. Use: (ut  {required_last_test_num}  {optional_start_test_num} ).\n");
           unittest_max = 0;
           unittest_cur = 0;
           return 0;
           } else {
           unittest_cur = unittest_max;
           unittest_max = floor(dval);
           printf("(ut %ld %ld) requested: running unit tests  #%ld  through  #%ld.\n",unittest_cur, unittest_max, unittest_cur, unittest_max);
           return 0;
           }
       }

      }
      return 0;
    } 

L3END(unittest)
#endif


// eval_parse(): parse and evaluate a string, newly re-written to use the new tdop parser.

// uses:
void write_string_to_temp_file(FILE** src, l3path* writeme);

void write_string_to_temp_file(FILE** src, l3path* writeme) {

    *src = tmpfile();
    if (0== *src) {
        perror("write_string_to_temp_file failed to get tmpfile():");
        return;
    }
    fprintf(*src, "%s", (*writeme)());
    rewind(*src);
}


L3KARG(eval_parse,1)
{
    // to parse from string.... put the string in a temp FILE*, so we can get from that.
    // 1. get tmpnam name
    // 2. open as mmap-ed file
    // 3. write string to it

    l3path s;
    l3obj* cur = 0;
    ptrvec_get(vv,0,&cur);
    if (cur->_type != t_str && cur->_type != t_lit) {
        printf("error in eval_parse: requires a string; argument was instead of type %s, in expression '%s'.\n", 
               cur->_type, sexps());
        if (vv) { generic_delete((l3obj*)vv, L3STDARGS_OBJONLY); }
        l3throw(XABORT_TO_TOPLEVEL);
    }
    string_get(cur,0,&s);
    s.pushf("\n"); //  hack, so we don't hit eof right away... because hitting eof happens before we process the line otherwise.
    if (vv) { generic_delete((l3obj*)vv, L3STDARGS_OBJONLY); }
    vv = 0;

    DV(printf("progress report, in eval_parse we have string to parse: '%s'\n", sexps()));
    
    FILE* src = 0;
    write_string_to_temp_file(&src, &s);

                                   
    // 4. parse the string from that

    if (!src) {
        printf("error in eval_parse: could not put string to temp file for parsing.\n");
        if (vv) { generic_delete((l3obj*)vv, L3STDARGS_OBJONLY); }
        l3throw(XABORT_TO_TOPLEVEL);        
    }

    call_repl_in_trycatch(src,0,false,env,owner);

    fclose(src);

  if (vv) { generic_delete((l3obj*)vv, L3STDARGS_OBJONLY); }

}
L3END(eval_parse)




   // implementation of exists, lexists, lookup, and llookup.
   //
   // exists: 
   // allow user to check for a string listing in an environment.
   // obj = string to query for. Returns T or F.
   //
   // lookup: 
   // allow user to check for a string listing in an environment.
   // obj = string to query for. Returns the object or F.
   //
   // local versions of the above: don't check up the env stack.
   //
L3KMIN(exists,1)

   int local_only = 0;
   int return_ref = 0;
   long flags = *((long*)(&obj));

   switch(flags) {
   case 0: local_only = 0; return_ref = 0; break; // exists
   case 1: local_only = 1; return_ref = 0; break; // lexists
   case 2: local_only = 0; return_ref = 1; break; // lookup
   case 3: local_only = 1; return_ref = 1; break; // llookup
   default:
       assert(0);
   }

   long N = ptrvec_size(vv);
   l3obj* cur = 0;
   l3path s;
   l3obj* found = 0;
   llref* innermostref = 0;
   for (long i =0; i < N; ++i ) {
       ptrvec_get(vv,i,&cur);
       if (cur->_type != t_str && cur->_type != t_lit) {
           std::cout << "error: " << exp->val();
           printf("requires a string; argument %ld was instead of type %s.\n", i+1, cur->_type);
           l3throw(XABORT_TO_TOPLEVEL);
       }
       string_get(cur,0,&s);
       found = 0;
       if (local_only) {
           found = (l3obj*)lookup_hashtable(env,s());
       } else {
           found = RESOLVE_REF(s(), env, AUTO_DEREF_SYMBOLS,&innermostref,0,s(),UNFOUND_RETURN_ZERO);
       }

       if (found) {
           if (return_ref) {
               ptrvec_set(vv,i,found);
           } else {
               ptrvec_set(vv,i,gtrue);
           }
       } else {
           ptrvec_set(vv,i,gnil);
       }
   }
   *retval = vv;

L3END(exists)


L3METHOD(lexists)
   long req = 1;
   obj = *(l3obj**)(&req);
   exists(L3STDARGS);
L3END(lexists)


L3METHOD(lookup)
   long req = 2;
   obj = *(l3obj**)(&req);
   exists(L3STDARGS);
L3END(lookup)

L3METHOD(llookup)
   long req = 3;
   obj = *(l3obj**)(&req);
   exists(L3STDARGS);
L3END(llookup)



L3METHOD(system)
   to_string(L3STDARGS);
   l3obj* s = *retval;

   l3path cmd;
   string_get(s,0,&cmd);

   obj = (l3obj*)popen(cmd(), "r");
   make_new_fileh(L3STDARGS);

L3END(system)


L3METHOD(sys)
   to_string(L3STDARGS);
   l3obj* s = *retval;

   l3path cmd;
   string_get(s,0,&cmd);

   FILE* fh = popen(cmd(), "r");

   l3obj* stringvec = 0;
   make_new_ptrvec(0, -1, 0, env, (l3obj**)&stringvec, owner, curfo, t_vvc, retown,ifp);
   assert(stringvec->_mytag);

   l3path tmp;
   qqchar qc;
   char* rc = 0;
   long i = 0;
   while (1) {
       rc = fgets(tmp(), tmp.bufsz(), fh);
       tmp.set_p_from_strlen();
       tmp.chomp();
       if (0 == rc) break;
       printf("%s",tmp());
       qc = tmp.as_qq();
       ptrvec_set(stringvec,i,make_new_string_obj(qc, stringvec->_mytag,"system_out"));
       tmp.buf[0]='\0';
       ++i;
   }
   fclose(fh);
   *retval = stringvec;

L3END(sys)

// return to toplevel from wherever we were...
L3METHOD(poptop)
         l3throw(XPOPTOP);
L3END(poptop)

// make_new_captag
//
// obj has *l3path with object name of the newly made captag
//
// curfo has the *l3path with the class name (if 0 we'll construct a default)
//
// retown has the parent tag that will own the the tag
// *retval will get the new object
// (*retval)->_mytag has the new tag; as does (*retval)->_owner, and these are equal.
//
// arity has number of extra bytes for the object, beyond sizeof(l3obj)
//
// if etyp is t_obj then we initialize as an object too. In this case arity is replaced by sizeof(objstruct) for object allocation.
//
L3METHOD(make_new_captag)
   LIVET(retown);
   assert(arity >= 0);
   if (etyp == t_obj) {
       arity = sizeof(objstruct);
   }

   l3path* pbasenm = (l3path*)obj;
   l3path tagname(*pbasenm);
   tagname.pushf("_captagTag");
   Tag* tag = new Tag(STD_STRING_WHERE.c_str(), retown, tagname(),0);


   l3path objnm(*pbasenm);
   //   objnm.pushf("_obj");

   l3path clsnm(*pbasenm);

   if (curfo) {
       l3path* pclsnm = (l3path*)curfo;
       clsnm.init( (*pclsnm)());
   } else {
       clsnm.pushf("_cls");
   }


   // make new_obj generates a new tag automatically...not really what we want...since
   // we want want to use tag from above. So we call make_new_class directly instead of make_new_obj.


   l3obj* newobj = make_new_class(arity, tag, clsnm(), objnm()); // arity has extra bytes size in it; beyond sizeof(l3obj).
   newobj->_type = etyp; // cannot leave this 0x0 or the object will look dead.

   if (etyp == t_obj) {
       newobj->_type = t_obj;
       do_obj_init(newobj, objnm(), clsnm());
   }




   tag->captain_set(newobj);
   newobj->_mytag = tag;
   set_notforwarded_tag(newobj);
   set_captag(newobj);
   assert(newobj->_owner == tag);

   // copy parent for now. but I do not think we need this now.
   // tag->dfs_been_here_set(tag->parent()->dfs_been_here());

   *retval = newobj;

L3END(make_new_captag)

L3METHOD(history)
   histlog->show_history();
L3END(history)


L3METHOD(lsb) 
     printf("=== current environment, in breadth-first order, with canonical names, is:\n");
       // breadth-first-search

#if 0  // not yet reimplemented for recursion
     Tag* glob = (Tag*)sgt.get_base_tag();
     long unseen =  glob->dfs_been_here();
     glob->bfs_print_obj(0,unseen,0,0, retval, glob,0,0,glob);
#endif

L3END(lsb)



L3METHOD_TMPCAPTAG(prog1)


     l3obj* progn_tmp = 0; 
     tmpbuf.reinit(exp);
     DV(printf("debug: prog1 statement is: '%s' \n",tmpbuf()));

     int len = exp->nchild();
     if (len < 1) {
         printf("error in prog1: must have at least one sub-expression!: '%s'\n",tmpbuf());
         l3throw(XABORT_TO_TOPLEVEL);
     }

     sexp_t* next_sib = 0;


     L3TRY_TMPCAPTAG(prog1, 0)

         for (int i = 0; i < len; i++) {
             next_sib = exp->ith_child(i);
             tmpbuf.reinit(next_sib);
             DV(printf("debug: next progn sub expression is next_sib: '%s' \n",tmpbuf()));

             if (i==1) {
                 eval(0,-1,next_sib,L3STDARGS_ENV); // here use retown, since we're on the first expression.
             } else {
                 progn_tmp = 0;
                 eval(0,-1,next_sib,env,&progn_tmp,(Tag*)tmp_tag,curfo,etyp,(Tag*)tmp_tag,ifp);
         }

         DV(print(progn_tmp, "progn_tmp:   ",0));
     }

L3END_CATCH_TMPCAPTAG(prog1)



// new version of assert_with_tmp_tag uses the 3 macro tmp_tag approach demonstrated above.

L3METHOD_TMPCAPTAG(assert_with_tmp_tag)

  volatile l3obj* tmpobj_for_assert_values = 0;

  sexp_t* nex  =  0;
  *retval = gtrue; // default value, because otherwise we crash! =)

     L3TRY_TMPCAPTAG(assert_with_tmp_tag, 0)

         for (long i = 0; i < arity; ++i) {
             nex = exp->ith_child(i);
             assert(nex);
             eval(0,-1,nex,env, (l3obj**)&tmpobj_for_assert_values, (Tag*)tmp_tag, curfo, etyp, (Tag*)tmp_tag,ifp);

             DV(
                printf("in assert_with_tmp_tag, the %ld-th value is:\n",i-1);
                print((l3obj*) tmpobj_for_assert_values,"    ",0);
                );

             if (! is_true((l3obj*)tmpobj_for_assert_values,0)) {
                 l3path sexptxt(nex);
                 l3path msg(0,"fatal error: assert failed on: '%s'",sexptxt());

                 HLOG_ADD_SYNC(msg());
                 MLOG_ADD_SYNC(msg()); // to  memlog_<pid>
                 
                 assert(0);
                 exit(1);
             }
         }


L3END_CATCH_TMPCAPTAG(assert_with_tmp_tag)


L3METHOD_TMPCAPTAG(print_list_to_string)

  arity = num_children(exp);
  volatile l3obj* tmpstring = 0;
  volatile l3obj* presstring = 0;

  l3path mystring;
  l3path part;
  l3path testget;
  sexp_t* nextstring = 0;
  l3path printed_nextstring;
  qqchar empty;
  long i = 0;

     L3TRY_TMPCAPTAG(print_list_to_string, 0)

         presstring = make_new_string_obj(empty, retown, (char*)"p_string");

         for (i = 0; i < arity; ++i) {
             nextstring = exp->ith_child(i);

             // for easier diagnostics
             printed_nextstring.reinit(nextstring);
             DV(printf("printed_nextstring is: %s\n",printed_nextstring()));
             
             part.init();
             tmpstring  = 0;
             
             // evaluate to get a string
             eval(0,-1, nextstring, env, (l3obj**)&tmpstring, (Tag*)tmp_tag, curfo, t_str, (Tag*)tmp_tag,ifp);
             
             to_string((l3obj*)tmpstring, &part, 0,0);
             mystring.pushf("%s", part());
         }

         string_set((l3obj*)presstring,0,mystring());

         string_get((l3obj*)presstring,0,&testget);

         *retval = (l3obj*)presstring;

L3END_CATCH_TMPCAPTAG(print_list_to_string)

   // permanantly mark as immovable and undeletable
L3KARG(seal,1)
   l3path s_sealme(ith_child(exp,1));
   l3obj* seal_me  = 0;
   ptrvec_get(vv,0,&seal_me);
   set_sealed(seal_me);
   *retval = seal_me;

   generic_delete(vv, L3STDARGS_OBJONLY);
L3END(seal)



L3METHOD(setq)
{
    l3path current_eval_line(exp);   // current eval line

    arity = exp->nchild();
    if (arity != 2) {
        printf("error wrong arity in expression '%s': saw %ld argument(s), "
               "expected 2. setq requires two arguments; e.g. (setq a 10); "
               "where a is the variable name, and 10 is the value assigned "
               "to a.\n",current_eval_line(),
               arity);
        l3throw(XABORT_TO_TOPLEVEL);
        //      return 0;
    }

    Tag*     retown_for_rhs_eval = retown;
    l3obj*   val_for_rhs = 0;    // something for retval_for_rhs_eval to point to if retval isn't right.
    l3obj**  retval_for_rhs_eval = &val_for_rhs;
    char*    basenm=0;

    l3obj* lhs_obj =0;
    llref* lhs_iref = 0;
    Tag*   lhs_owner = 0;
    l3obj* lhs_env = 0;
    l3obj* target = 0;

    // finished shift to using lhs_setq here:

             l3obj* env_to_insert_in = 0;
             l3path name_to_insert;

             assign_det ad;
             ad.init();
             obj = (l3obj*)(&ad);

             //
             // obj   passes in  assign_det* to fill in.
             //
             lhs_setq(L3STDARGS);

             // unpack the results filled into ad

             // name_to_insert lives in ad, no need to unpack.
             env_to_insert_in = ad.env_to_insert_in;
         
             lhs_iref   = ad.lhs_iref;
             lhs_env    = ad.lhs_env;
             lhs_owner  = ad.lhs_owner;
             lhs_obj    = ad.lhs_obj;
             target     = ad.target;
         
             // can't do this, or else it'll get deleted!!!! : retown              = ad.retown_for_rhs_eval;

             retown_for_rhs_eval = ad.retown_for_rhs_eval;
             
             // sanity check the results from lhs_setq
             LIVEO(env_to_insert_in);
             LIVET(retown_for_rhs_eval);
             if (lhs_iref) { LIVEREF(lhs_iref); }

             basenm = ad.name_to_insert.dotbasename();

             // end of lhs_setq use.

    // like defmethod, setq has got to:
    //
    //   (a) make the new object with the appropriate retown
    //   (b) set the _parent_env static scope for function objects
    //   (c) add an alias for it, giving it a name.
    //
    //make_new_function_defn(L3STDARGS);
    //set_static_scope_parent_env_on_function_defn(*retval,0,0,   env_to_insert_in,0,0,   0,0,0);
    //add_alias_eno(env_to_insert_in, basenm, *retval);



    // II. generate RHS (right-hand-side) with an eval
    eval(0,-1,exp->ith_child(1), env,retval_for_rhs_eval,owner,curfo,etyp,retown_for_rhs_eval,ifp);


    if (*retval_for_rhs_eval == 0) {
        DV(printf("possible internal error in setq assignment: *retval_for_rhs_eval == 0 detected. Plugging in nil.\n"));
        //        l3throw(XABORT_TO_TOPLEVEL);
        *retval_for_rhs_eval = gnil;
    }

        // Late addition: assigning *retval:
        // 
        // can we do this now...? for a while we thought not, since it broke a test involving progn. so we changed the do_progn code 
        // so that it no longer transfers ownership. tests still pass, but leaving these comments in case of
        // future difficulties.
        *retval = *retval_for_rhs_eval;


    // convert self assignment -> noop (check and avoid self-assignment)
    l3obj* rhs_obj = *retval_for_rhs_eval;
    LIVEO(rhs_obj);
    if (rhs_obj == lhs_obj) {
        DV(printf("setq warning: assingment converted to no-op; self-assignment for '%s' detected and skipped.\n",basenm));
        return 0;
    }


    // rewriting a ref with a new value? ... move the ref to a new RHS specified ring.
    if (target) {
        // yes, we are re-writing/overwriting a pre-existing lhs. 


        //   the owner of the lhs may
        //   need to take ownership of the rhs, or else we could be assigning a tmp that will
        //   disappear back to the caller.
        Tag* rhs_owner = rhs_obj->_owner;
        assert(rhs_owner);
        assert(lhs_owner); // should have been set above when lhs_iref was set to non-zero.



        llref* rhs_ref = rhs_obj->_owner->find(rhs_obj); // ?or  llref* rhs_ref = priority_ref(rhs_obj); // becuase we don't want the priority 1 ref...
        assert(rhs_ref);


        // if this is a @< ref, then we want to actually do something more:
        //  we'll want to change *all* the references in the lhs_iref ring to become members of the rhs_ref ring.
        //  And we'll want to delete the old, overwritten value.

        if (lhs_iref->has_prop(t_oco)) {

            move_all_visible_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: lhs '%s' is an out_c_own @< reference: discarding overwritten value (%p ser# %ld).\n",
                      basenm,lhs_obj,lhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);
            
            return 0;

        } else if (lhs_iref->has_prop(t_ref) || lhs_iref->has_prop(t_oso)) {

            assert(rhs_obj->_owner == lhs_owner);

            // is a pass by reference, so modify the underlying value...
            // right now we just replace (so type independent), but compiler would
            // want to optimize this to re-writing just the memory location of concern,
            // rather than deallocating the old object and putting the new object
            // in place of all references.
            llref_update_obj_of_ring_in_place(lhs_iref, rhs_ref);
            

            // actualcall.arg_val can have cached l3obj* ... update these.
            scan_env_stack_for_calls_with_cached_obj(0, lhs_obj, rhs_obj);

            DV(printf("setq: pass-by-reference argument updated. '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);
            return 0;


        } else if (lhs_iref->has_prop(t_iso)) {
            // dupliate default code, modify next: TODO FINISH IMPLEMENTATION of @> !!!

            // if this is also a local object, then we need to update that reference too...
            if (lhs_env->_parent_env) {
                llref* ref_in_cap = query_for_env(lhs_iref, lhs_env->_parent_env);
                if (ref_in_cap) {
                    move_llref_to_new_ring(ref_in_cap, rhs_ref);
                }
            }

            // marked @>
            move_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: existing llref '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);
            scan_env_stack_for_calls_with_cached_obj(lhs_env, lhs_obj, rhs_obj);

            // necessary any more?            add_alias_eno(env_to_insert_in, basenm, *retval_for_rhs_eval);
            return 0;
     

        } else {
            // just lone single assignment, not marked @<
            move_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: existing llref '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));


            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);


            // this seems to already be taken care of... but not always.
            llref* confirm = (llref*)lookup_hashtable(lhs_env,basenm);
            if(confirm) {
                assert(confirm->_obj == *retval_for_rhs_eval);
            } else {
                add_alias_eno(lhs_env, basenm, *retval_for_rhs_eval);
            }
            
            return 0;  // without setting *retval, apparently important because otherwise progn deletes it...???

        } // end lone single assignment

        // is this already taken care of here too? highly doubtful.
        assert(!lookup_hashtable(lhs_env,basenm));
        add_alias_eno(lhs_env, basenm, *retval_for_rhs_eval);

        return 0;

    } // end if (lhs_iref) / re-writing/overwriting a pre-existing lhs




    size_t sz_before = 0;
    DV(sz_before = hash_show_keys(env_to_insert_in, "before:  "); );

    add_alias_eno(env_to_insert_in, basenm, *retval_for_rhs_eval);

    size_t sz_after = 0;
    DV(sz_after = hash_show_keys(env_to_insert_in, "after:  "); );
    // replacements do happen, so can't: assert(sz_after == 1 + sz_before);

    DV(printf("sz_before: %ld     sz_after: %ld\n", sz_before, sz_after); );

    // ask if _parent_env is zero, and if so...should we adopt it?
    if (0 == rhs_obj->_parent_env) {
        rhs_obj->_parent_env = env_to_insert_in;
    }

    // do we ever what to say *retval = *retval_for_rhs_eval;
    *retval = *retval_for_rhs_eval;
}
L3END(setq) 


////////////////////////////////
//
// setq_rhs_first : in order to evaluate properly these expressions:
//   a=2 
//   a=a=3
//
// We need to evaluate the RHS first, before evaluating the LHS, of RHS=LHS assignment.
// Because if we grab the old a=2 reference first for the LHS, and
//   then it gets replaced during the a=3, then we have a stale reference when
//   it comes to assigning finally to the left-most a in 'a=a=3'.
//
// On the other hand, why did we choose to evaluate the LHS first originally in setq?
//

L3METHOD(setq_rhs_first)
{
    l3path current_eval_line(exp);   // current eval line

    arity = exp->nchild();
    if (arity != 2) {
        printf("error wrong arity in expression '%s': saw %ld argument(s), "
               "expected 2. setq requires two arguments; e.g. (setq a 10); "
               "where a is the variable name, and 10 is the value assigned "
               "to a.\n",current_eval_line(),
               arity);
        l3throw(XABORT_TO_TOPLEVEL);
        //      return 0;
    }

    Tag*     retown_for_rhs_eval = retown;
    l3obj*   val_for_rhs = 0;    // something for retval_for_rhs_eval to point to if retval isn't right.
    l3obj**  retval_for_rhs_eval = &val_for_rhs;
    char*    basenm=0;

    l3obj* lhs_obj =0;
    llref* lhs_iref = 0;
    Tag*   lhs_owner = 0;
    l3obj* lhs_env = 0;
    l3obj* target = 0;


       

    // II. generate RHS (right-hand-side) with an eval
    eval(0,-1,exp->ith_child(1), env,retval_for_rhs_eval,owner,curfo,etyp,retown_for_rhs_eval,ifp);


    if (*retval_for_rhs_eval == 0) {
        //DV(
        printf("possible internal error in setq assignment: *retval_for_rhs_eval == 0 detected."); //  Plugging in nil.\n");
        // );
        l3throw(XABORT_TO_TOPLEVEL);
        *retval_for_rhs_eval = gnil;
    }

        // Late addition: assigning *retval:
        // 
        // can we do this now...? for a while we thought not, since it broke a test involving progn. so we changed the do_progn code 
        // so that it no longer transfers ownership. tests still pass, but leaving these comments in case of
        // future difficulties.
        *retval = *retval_for_rhs_eval;


    // convert self assignment -> noop (check and avoid self-assignment)
    l3obj* rhs_obj = *retval_for_rhs_eval;


    // now do I. LHS...


    // finished shift to using lhs_setq here:

             l3obj* env_to_insert_in = 0;
             l3path name_to_insert;

             assign_det ad;
             ad.init();
             obj = (l3obj*)(&ad);

             //
             // obj   passes in  assign_det* to fill in.
             //
             lhs_setq(L3STDARGS);

             // unpack the results filled into ad

             // name_to_insert lives in ad, no need to unpack.
             env_to_insert_in = ad.env_to_insert_in;
         
             lhs_iref   = ad.lhs_iref;
             lhs_env    = ad.lhs_env;
             lhs_owner  = ad.lhs_owner;
             lhs_obj    = ad.lhs_obj;
             target     = ad.target;
         
             // can't do this, or else it'll get deleted!!!! : retown              = ad.retown_for_rhs_eval;

             retown_for_rhs_eval = ad.retown_for_rhs_eval;
             
             // sanity check the results from lhs_setq
             LIVEO(env_to_insert_in);
             LIVET(retown_for_rhs_eval);
             if (lhs_iref) { LIVEREF(lhs_iref); }

             basenm = ad.name_to_insert.dotbasename();

             // end of lhs_setq use.

    // like defmethod, setq has got to:
    //
    //   (a) make the new object with the appropriate retown
    //   (b) set the _parent_env static scope for function objects
    //   (c) add an alias for it, giving it a name.
    //
    //make_new_function_defn(L3STDARGS);
    //set_static_scope_parent_env_on_function_defn(*retval,0,0,   env_to_insert_in,0,0,   0,0,0);
    //add_alias_eno(env_to_insert_in, basenm, *retval);




    // back to stuff that was just after rhs_obj = *retval_for_rhs_eval;  before

    LIVEO(rhs_obj);
    if (rhs_obj == lhs_obj) {
        //DV(
        printf("setq warning: assingment converted to no-op; self-assignment for '%s' detected and skipped.\n",basenm);
        //);

        *retval = *retval_for_rhs_eval;
        return 0;
    }


    // rewriting a ref with a new value? ... move the ref to a new RHS specified ring.
    if (target) {
        // yes, we are re-writing/overwriting a pre-existing lhs. 


        //   the owner of the lhs may
        //   need to take ownership of the rhs, or else we could be assigning a tmp that will
        //   disappear back to the caller.
        Tag* rhs_owner = rhs_obj->_owner;
        assert(rhs_owner);
        assert(lhs_owner); // should have been set above when lhs_iref was set to non-zero.



        llref* rhs_ref = rhs_obj->_owner->find(rhs_obj); // ?or  llref* rhs_ref = priority_ref(rhs_obj); // becuase we don't want the priority 1 ref...
        assert(rhs_ref);


        // if this is a @< ref, then we want to actually do something more:
        //  we'll want to change *all* the references in the lhs_iref ring to become members of the rhs_ref ring.
        //  And we'll want to delete the old, overwritten value.

        if (lhs_iref->has_prop(t_oco)) {

            move_all_visible_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: lhs '%s' is an out_c_own @< reference: discarding overwritten value (%p ser# %ld).\n",
                      basenm,lhs_obj,lhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);
            
            *retval = *retval_for_rhs_eval;
            return 0;

        } else if (lhs_iref->has_prop(t_ref) || lhs_iref->has_prop(t_oso)) {

            assert(rhs_obj->_owner == lhs_owner);

            // is a pass by reference, so modify the underlying value...
            // right now we just replace (so type independent), but compiler would
            // want to optimize this to re-writing just the memory location of concern,
            // rather than deallocating the old object and putting the new object
            // in place of all references.
            llref_update_obj_of_ring_in_place(lhs_iref, rhs_ref);
            

            // actualcall.arg_val can have cached l3obj* ... update these.
            scan_env_stack_for_calls_with_cached_obj(0, lhs_obj, rhs_obj);

            DV(printf("setq: pass-by-reference argument updated. '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);

            *retval = *retval_for_rhs_eval;
            return 0;


        } else if (lhs_iref->has_prop(t_iso)) {
            // dupliate default code, modify next: TODO FINISH IMPLEMENTATION of @> !!!

            // if this is also a local object, then we need to update that reference too...
            if (lhs_env->_parent_env) {
                llref* ref_in_cap = query_for_env(lhs_iref, lhs_env->_parent_env);
                if (ref_in_cap) {
                    move_llref_to_new_ring(ref_in_cap, rhs_ref);
                }
            }

            // marked @>
            move_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: existing llref '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));

            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);
            scan_env_stack_for_calls_with_cached_obj(lhs_env, lhs_obj, rhs_obj);

            // necessary any more?            add_alias_eno(env_to_insert_in, basenm, *retval_for_rhs_eval);

            *retval = *retval_for_rhs_eval;
            return 0;
     

        } else {
            // just lone single assignment, not marked @<
            move_llref_to_new_ring(lhs_iref, rhs_ref);

            DV(printf("setq: existing llref '%s' reference has joined a new llref ring pointing to (%p ser# %ld).\n",
                      basenm,rhs_obj,rhs_obj->_ser));


            lhs_obj->_owner->reference_deleted(lhs_obj,L3STD_OBJ);


            // this seems to already be taken care of... but not always.
            llref* confirm = (llref*)lookup_hashtable(lhs_env,basenm);
            if(confirm) {
                assert(confirm->_obj == *retval_for_rhs_eval);
            } else {
                add_alias_eno(lhs_env, basenm, *retval_for_rhs_eval);
            }

            *retval = *retval_for_rhs_eval;
            return 0;

        } // end lone single assignment

        // is this already taken care of here too? highly doubtful.
        assert(!lookup_hashtable(lhs_env,basenm));
        add_alias_eno(lhs_env, basenm, *retval_for_rhs_eval);

        *retval = *retval_for_rhs_eval;
        return 0;

    } // end if (lhs_iref) / re-writing/overwriting a pre-existing lhs




    size_t sz_before = 0;
    DV(sz_before = hash_show_keys(env_to_insert_in, "before:  "); );

    add_alias_eno(env_to_insert_in, basenm, *retval_for_rhs_eval);

    size_t sz_after = 0;
    DV(sz_after = hash_show_keys(env_to_insert_in, "after:  "); );
    // replacements do happen, so can't: assert(sz_after == 1 + sz_before);

    DV(printf("sz_before: %ld     sz_after: %ld\n", sz_before, sz_after); );

    // ask if _parent_env is zero, and if so...should we adopt it?
    if (0 == rhs_obj->_parent_env) {
        rhs_obj->_parent_env = env_to_insert_in;
    }

    // do we ever what to say *retval = *retval_for_rhs_eval;
    *retval = *retval_for_rhs_eval;
}
L3END(setq_rhs_first)





void   print_objlist(objlist_t* envpath) {
    
    //    if (envpath->size() > 0) return;
    
    objlist_it en = envpath->end();
    objlist_it last = en;
    --last;
    objlist_it be = envpath->begin();

    l3obj* o = 0;
    for(; be != en; ++be) {
        o = *be;
        if (o) {
            printf("%s",o->_varname);
        } else {
            printf("(null)");
        }
        if (be != last) {
            printf("/");
        }
    }
    printf("\n");

}


   //
   // just keep a record of which test is currently running, so if it fails, we
   //  can locate it easily. Returns the string that was set.
   //
L3KMIN(ctest,1)

   l3obj* str_testname  = 0;
   ptrvec_get(vv,0,&str_testname);
   if (str_testname->_type != t_str) {
       printf("error: ctest expected the first argument to be a string (%s). We got type '%s' instead.\n", 
              t_str,
              str_testname->_type);
       generic_delete(vv, L3STDARGS_OBJONLY);
       l3throw(XABORT_TO_TOPLEVEL);
   }

   // looks good, store it and return it.
   ctst.clear();
   string_get(str_testname, 0, &ctst); // does pushf, hence the clear() is needed before hand.
   str_testname->_owner->generic_release_to(str_testname, retown);
   *retval = str_testname;   
   ++ctst_calls; // track number of times ctest called, for ease of pinpointing issue.

   generic_delete(vv, L3STDARGS_OBJONLY);
L3END(ctest)


L3METHOD(get_ctest)
   printf("get_ctest(): ctest has been called %ld times.\n", ctst_calls);
   *retval = make_new_string_obj(ctst.as_qq(), retown, "get_ctest_output");
L3END(get_ctest)


// defmethod, aka de: function definition

             /*
               (de dowork        ; defun 
               (prop
               (return int)
               (return l3obj*) ; implicitly out.c.own or @<
               (arg arg1 int       )  ; defaults to pass by value. (server gets a copy; C semantics)
               (arg arg2  l3obj*  !>) ; in.c.own
               (arg name3 l3obj*  !<) ; out.s.own
               (arg name5 l3obj*  @>) ; in.s.own
               (arg name6 l3obj*  @<) ; out.c.own
               )
               ... body here...
               )

               ex2:

               (de add1 
               (prop (return t_dou) (arg x t_dou !>)) 
               (progn
               (: a = 4) 
               (+ x a)
               )
               )

               (progn (one)(two)...(last))  -> runs all of them in an order, and returns the value of the last one.

               can it be a user defined function? I don't think so.

             */

L3METHOD(defmethod)
{
    l3path expstring(exp);

             if (arity != 3) {
                 printf("error: wrong arity error in '%s': saw %ld argument(s), expected 3 "
                        "arguments following the de/fn keyword. Example: ($de funcname ($prop ...) body)"
                        "  for definition of function requires a function name, a prop set,"
                        " and a function body; e.g. "
                        "($de add1 ($prop ($return x @< t_dou) ($arg x !> t_dou)) ($+ x 1))\n",
                        expstring(),arity);
                 l3throw(XABORT_TO_TOPLEVEL);
             }

             l3obj* env_to_insert_in = 0;
             l3path name_to_insert;

             assign_det ad;
             ad.init();
             obj = (l3obj*)(&ad);

             //
             // obj   passes in  &ad
             //
             lhs_setq(L3STDARGS);

             // setup for make_new_function_defn call, using the ad filled in results:             
             env_to_insert_in     = ad.env_to_insert_in;
             retown               = ad.retown_for_rhs_eval; // *((Tag**)retval);
             char* basenm         = ad.name_to_insert();

             // cleanup: get rid of Tag* that is not an l3obj* in  *retval; make_new_function_defn will fill in next.
             *retval = 0;
             
             // sanity check the results from lhs_setq
             LIVEO(env_to_insert_in);
             LIVET(retown);

             //
             // obj has l3path* name_to_insert for good debugging internal labels.
             //

             // these are the three calls that have to happen for a defmethod: 
             // ; we've got to 
             //
             //   (a) make the new object with the appropriate retown
             //   (b) set the _parent_env static scope for function objects
             //   (c) add an alias for it, giving it a name.
             //
             make_new_function_defn(L3STDARGS);
             set_static_scope_parent_env_on_function_defn(*retval,0,0,   env_to_insert_in,0,0,   0,0,0,ifp);

             add_alias_eno(env_to_insert_in, basenm, *retval);


}
L3END(defmethod)



//
// need to break setq into two halves that are usable, lhs_setq and rhs_setq
//  here is the lhs alone... while we figure out how to integrate de for defining functions.
//

// lhs_setq : ends with just setting retval to be the Tag* to assign to
//
// *retval = 0, now see ((assign_det*)obj)->retown_for_rhs_eval  for the Tag* to use for rhs eval.
//
// obj   passes in  assign_det*
// 
//
//
L3METHOD(lhs_setq)
{
    assert(obj);
    assign_det* ad = (assign_det*)(obj);

    l3path current_eval_line(exp);   // current eval line

    // first resolve a retown from the left hand side, then pass that into eval.
    //  that way we'll own it if nobody else does; but we won't try to take
    //   take ownership of gnil or gtrue.
    // Again the principle that applies: figure out ownership desired for new values as early as possible, 
    //  and provide that in retown : to allocate a *retval value on if need be. Old values are already owned,
    //  and we can just point at them. (Hopefully--we may still want to have a weakref system for updating the
    //  places that are using the symbol when it goes bye-bye.

    // this stratgy seems to be working and seems theoretically sound: do this for "de" as well.

    // LHS (left-hand-side)

    l3obj* env_to_insert_in = 0; // aka lhs_env
    l3path& name_to_insert = (ad->name_to_insert);

    l3obj* lhs_obj =0;
    llref* lhs_iref = 0;
    Tag*   lhs_owner = 0;
    l3obj* lhs_env = 0;

    assert(env);
    LIVEO(env);

    // I. Analyze the LHS reference to Figure out who will own the rhs obj, which results in setting retown_for_rhs_eval appropriately.
    //     Do this as early as possible! Because you really don't want to have to go and mess with ownership after the eval is done.
    //

    // A) look in formal parameter env.
    // B) look in curfo, the current function's object, aka 'this' in C++.


    objlist_t envstack;
    l3obj* target = 0;

    qqchar dottedname = exp->ith_child(0)->val();
    if (0==dottedname.strcmp("[")) {
        printf("assignment into [ subscripted array...not implemented yet.\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }


    // A) in env, since env contains the formals of the call.
    //
    if (!target && env) {

              // reset result vars, in case prev filled...
              name_to_insert.clear();
              envstack.clear();
              lhs_iref = 0;
        
              target = validate_path_and_prep_insert(
                                                     dottedname, // exp->ith_child(0)->val(),    // char* dottedname,     1  
                                                     env,                     // l3obj* startingenv,   3  
                                                     AUTO_DEREF_SYMBOLS,      //  deref_syms deref,     6  
                                                     &lhs_iref,                //  llref** innermostref, 7  
                                                     &envstack,               //  objlist_t* penvstack, -> replaces 4 env_to_insert_in, plus does more.
                                                     current_eval_line(),     //  char* curcmd_for_diag, 2  current_eval_line()
                                                     UNFOUND_RETURN_ZERO,     // noresolve_action noresolve_do, -

                                                     &name_to_insert         // l3path* name_to_insert  5  
                                                     );
    }

    // B) in curfo, if available.
    //
    if (!target && curfo) {
             target = validate_path_and_prep_insert(
                                                     dottedname,  // exp->ith_child(0)->val(),    // char* dottedname,     1  
                                                     curfo,                     // l3obj* startingenv,   3  
                                                     AUTO_DEREF_SYMBOLS,      //  deref_syms deref,     6  
                                                     &lhs_iref,                //  llref** innermostref, 7  
                                                     &envstack,               //  objlist_t* penvstack, -> replaces 4 env_to_insert_in, plus does more.
                                                     current_eval_line(),     //  char* curcmd_for_diag, 2  current_eval_line()
                                                     UNFOUND_RETURN_ZERO,     // noresolve_action noresolve_do, -

                                                     &name_to_insert         // l3path* name_to_insert  5
                                                     );
    }

    //    char* basenm = name_to_insert.dotbasename();

    // we may have a partially resolved path (and so be creating a new insert of an alias within an object).
    // or, we may be replacing an existing ref with a new one.
    // how do we tell the difference?  Does lhs_iref refer to the env_to_insert_in?

    llref* lhs_1iref = 0; // the (invisible priority 1) owned reference

    Tag*     retown_for_rhs_eval = retown;
    l3obj**  retval_for_rhs_eval = retval;
    l3obj*   val_for_rhs = 0;    // something for retval_for_rhs_eval to point to if retval isn't right.
    BOOL     pass_by_value = TRUE;

    if (target) {

        // REPLACING EXISTING VARIABLE

        assert(lhs_iref);
        lhs_1iref = get_ref_at_fixed_priority(lhs_iref, 1);
        assert(lhs_1iref);

        // this reference already exists, replace it, or adjust llref->_obj to reflect new rhs.
        lhs_obj = lhs_iref->_obj;
        lhs_owner = lhs_obj->_owner;

        // I think we want these to be the default unless otherwise indicated.
        retown_for_rhs_eval = lhs_owner;
        retval_for_rhs_eval = &val_for_rhs;

        // but do handle captags special
        if (pred_is_captag(lhs_owner, lhs_obj)) {
            lhs_owner = lhs_owner->parent();
            retown_for_rhs_eval = lhs_owner;
        } 

        // set lhs_env and env_to_insert_in
        lhs_env = lhs_iref->_env;
        env_to_insert_in = lhs_iref->_env;


        if (lhs_iref->has_prop(t_ref)) {
            pass_by_value = FALSE;
        }

        // detect if we are referring to the calls local references to arguments.

        // pass-by-reference vs pass-by-value
        // 
        // if the reference is pass-by-reference, then let the above defaults work, else 
        // for pass-by-value, we need to fix it up for local storage in the t_cal 
        // object's storage (_mytag (1st pref) or _owner (2nd pref)).
        //

#define PBV_PBR() \
        if (lhs_iref->_env->_type == t_cal) { \
            if (pass_by_value) { \
                if (lhs_iref->_env->_mytag) { \
                    retown_for_rhs_eval = lhs_iref->_env->_mytag; \
                } else { \
                    retown_for_rhs_eval = lhs_iref->_env->_owner; \
                } \
            } \
        } 


        // is this an argument to or return value from a function?
        // note that while we have t_arg and t_ret, they aren't yet employed.
        owntag_en transfer_type = bad_owntag; // aka 0
        if (lhs_iref->has_prop(t_ico) ) { assert(transfer_type==0); transfer_type = in_c_own; }
        if (lhs_iref->has_prop(t_iso) ) { assert(transfer_type==0); transfer_type = in_s_own; }
        if (lhs_iref->has_prop(t_oco) ) { assert(transfer_type==0); transfer_type = out_c_own; }
        if (lhs_iref->has_prop(t_oso) ) { assert(transfer_type==0); transfer_type = out_s_own; }

        if (transfer_type) {
            switch(transfer_type) {
            case in_c_own:
                {
                    PBV_PBR(); // Yes. In-client-own or !> is the only one of the four that
                    // is actually pass-by-value semanitcs. There is no ownership transfer requested.
                    // If it is t_ref then yes we want to update the reference even though we
                    // won't be changing ownership. If it isn't t_ref, then... we assume pass-by-value.
                }
                break;
            case in_s_own:
                {

                    printf("got an @> arg!\n");
                    // now we don't use the exact same owner as who we are replacing, 
                    // because we want to transfer

                }
                break;
            case out_c_own:
                {
                    // factory methods, @<
                    // pass by reference really seems implied here!
                    
                    printf("got an @< arg!\n");

                }
                break;
            case out_s_own:
                {
                    // pass by reference really seems implied here as well. so comment out.
                    printf("got an !< arg!\n");
                }
                break;
            default:
                assert(0);
            }
            
        } // end if transfer_type

    }
    else {

        // ORIGINATING NEW VARIABLE
        
        assert(0==target);
        if (envstack.size() == 0) {
            env_to_insert_in = env;
        } else {
            env_to_insert_in = envstack.back();
        }
   
        // since envstack.back() could be 0, have to keep checking...

        if (env_to_insert_in == 0) {
            if (lhs_iref) {
                env_to_insert_in = lhs_iref->_obj;
            } else {
                env_to_insert_in = env;
            }
        }

        if (0==env_to_insert_in->_mytag) {
            printf("error: object %p (ser# %ld) %s  had no _mytag (_mytag==0); so cannot insert into this object.\n", 
                   env_to_insert_in, env_to_insert_in->_ser,  env_to_insert_in->_varname); 
            l3throw(XABORT_TO_TOPLEVEL);
        }

        retown_for_rhs_eval = env_to_insert_in->_mytag;
        retval_for_rhs_eval = &val_for_rhs;

    }


    // lhs_setq : ends with just setting retval to be the tag to assign to
    *retval = 0;

    // fill in assign_det struct
    // already done by reference:    ad->name_to_insert      = name_to_insert();
    ad->env_to_insert_in    = env_to_insert_in;
    ad->lhs_iref            = lhs_iref;
    ad->lhs_env             = lhs_env;
    ad->lhs_owner           = lhs_owner;
    ad->lhs_obj             = lhs_obj;
    ad->target              = target;
    ad->retown_for_rhs_eval = retown_for_rhs_eval;

}
L3END(lhs_setq)


// rm (aka del) set up to handle multiple objs now
L3KMIN(rm, 1)

    if (arity == 1) {
        arity = 1;
        hard_delete(L3STDARGS);

        // cleanup the L3KMIN generated args:
        generic_delete(vv, L3STDARGS_OBJONLY);
        return 0;
    }

   long N = ptrvec_size(vv);
   l3obj* cur = 0;
   sexp_t*  nextname = 0;
   for (long i =0; i < N; ++i ) {
       ptrvec_get(vv,i,&cur);
       nextname = exp->ith_child(i);
       // have to pass retval, since
       // hard_delete returns the env that we deleted from in *retval, (for delete and replace actions).
       //
       hard_delete(cur,-1, nextname, env,retval,owner,   0,0,retown,ifp);
   }
   *retval = gtrue;
   generic_delete(vv, L3STDARGS_OBJONLY);

L3END(rm)

// ease of debugging
void p(codepoints* cp) {
    cp->dump();
}

L3METHOD(checkzero)
{
    MLOG_ONLY(l3path zmsg(0,"555555 ZERO CHECK POINT: WE EXPECT ZERO USERLAND ALLOCATIONS AT THIS POINT.");)
    MLOG_ADD(zmsg());

    if (serialfactory->heap_all_builtin(exp)) {
        *retval = gtrue;
    } else {
        *retval = gnil;
    }

    assert(*retval == gtrue);
}
L3END(checkzero)



L3METHOD(is_protobuf_reserved_word)
{
    if (!exp) return 0;
    if (!(exp->nchild())) return 0;
    if (!(exp->first_child()->val().sz())) return 0;

    qqchar tgt(exp->val());

    if (0==tgt.strcmp("message")) return 1;
    if (0==tgt.strcmp("import")) return 1;
    if (0==tgt.strcmp("package")) return 1;
    if (0==tgt.strcmp("service")) return 1;
    if (0==tgt.strcmp("rpc")) return 1;
    if (0==tgt.strcmp("extensions")) return 1;
    if (0==tgt.strcmp("extend")) return 1;
    if (0==tgt.strcmp("option")) return 1;
    if (0==tgt.strcmp("required")) return 1;
    if (0==tgt.strcmp("repeated")) return 1;
    if (0==tgt.strcmp("optional")) return 1;
    if (0==tgt.strcmp("enum")) return 1;

    return 0;
}
L3END(is_protobuf_reserved_word)


//
// don't call it 'infix' because the 'inf' at the beginning looks like infinity (parses as a double).
//
L3METHOD(pratt)
{
   _global_infixmode = true;
   printf("pratt parsing mode.\n");
   *retval = gtrue;
}
L3END(pratt)


L3METHOD(prefix)
{
   _global_infixmode = false;
   printf("prefix parsing mode.\n");
   *retval = gtrue;
}
L3END(prefix)


#if 0
template<int str_max>
l3str<str_max>::l3str(const sexp_t* sexp) {
    reinit(sexp);
}

template<int str_max>
void l3str<str_max>::reinit(const sexp_t* sexp) {

    qqchar tspan = sexp->span();
    long n = tspan.copyto(&buf[0], bufsz()-2);

    p = &buf[0] + n;
    *p = 0;
}

#endif
