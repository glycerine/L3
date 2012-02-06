//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef TERP_H
#define TERP_H

#include "l3obj.h"
#include "autotag.h"

#ifndef  _DMALLOC_OFF
#include "sermon.h"
#endif


//globals 

extern quicktype_sys*  qtypesys;

extern l3obj*  main_env;
extern int     gVerboseFunction;
extern int     gUglyDetails;
extern BOOL    g_have_dash_e;
extern BOOL    g_have_dash_repl;
extern l3path  g_dash_e_cmd; // if g_dash_e_cmd.len() > 0 then we have a -e command.

extern l3obj*  exception_stack;

//easier double quotes that are cast to (char*) to avoid compiler complaints
#define DQ(x) ((char*)(x))

//typedef enum chomcommentstate { code, dquote, slashcode, slashdquote } ccs;

typedef enum lexstate { code, dquote, slashcode, slashdquote, comment, atom, squote, slashsquote, triple_quote, slash_triple_quote,  } lexst_t;
typedef lexst_t  ccs;

#include "bool.h"

int terp_main(int argc, char **argv);


//////////////////////////////////////////
//  debug-only serialnumber l3obj* LIVE and llref* LLR macros
//////////////////////////////////////////

#ifdef _JLIVECHECK
#define LIVE(myptr) { assert(myptr); serialfactory->check_alive_ptr_or_assert(myptr); }

#define LIVEO(myobj) { assert(myobj); serialfactory->check_alive_ptr_or_assert(myobj); serialfactory->check_alive_sn(myobj->_ser); }

#define LIVET(mytag) { assert(mytag); serialfactory->check_alive_ptr_or_assert(mytag); serialfactory->check_alive_sn(mytag->sn()); }
#define LIVEREF(myllref) { assert(myllref); check_one_llref(myllref); }

#define LLRADD(addme) llr_global_debug_list_add(addme)
#define LLRDEL(delme) llr_global_debug_list_del(delme)
#define LLRCHECKALL() llr_global_debug_list_check_dangling()

#else

#define LIVE(myptr)
#define LIVEO(myobj)
#define LIVET(mytag)
#define LIVEREF(myllref) 

#define LLRADD(addme) -1
#define LLRDEL(delme)
#define LLRCHECKALL()

#endif

//////////////////////////////////////////
//  end debug-only macros
//////////////////////////////////////////



// expected_type allows type checking when desired.
//   returns int for exception handling.
// t_typ can be null, if we don't know what type to expect.
//

// should we have a default autotag and a default ptag?

//
// return value (*retval) protocol: *retval can be null, if you want a
//  new retval, use the retown tag.

struct Tag;

// #define L3METHOD(funcname)    int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp) { LIVET(owner); LIVET(retown);   volatile int l3rc = 0;

#define L3METHOD(funcname)    int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp) { volatile int l3rc = 0;

#define L3METHOD_NOTAGCHECK(funcname)    int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp) { volatile int l3rc = 0;


#define L3M(funcname)    int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp) 

#define L3FORWDECL(funcname)  int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp);

#define L3PUREVIRT(funcname)  virtual int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp) = 0;

#define L3CALL_OBJ(o) o,arity,exp,env,retval,owner,curfo,etyp,retown,ifp

#define L3STDARGS  obj,arity,exp,env,retval,owner,curfo,etyp,retown,ifp

#define L3STD_OBJ  arity,exp,env,retval,owner,curfo,etyp,retown,ifp

#define L3STD_PRINTCALL   (*(long*)stoppers),(sexp_t*)indent,  env,retval,owner,  curfo,etyp,retown,ifp

#define L3STDARGS_ENV  env,retval,owner,curfo,etyp,retown,ifp
//#define L3STDARGS_OBJONLY    -1,0,  0,0,0,              0,0,0
#define L3STDARGS_OBJONLY      -1,0,  0,0,owner,          0,0,retown,ifp
// #define L3NONSTANDARD_OBJONLY  -1,0,  0,0,defptag_get(),  0,0,defptag_get()

#define L3DISPATCH(name)    } else if (v.strcmp(#name)==0) { return name(L3STDARGS); 

#define ODEL(delme) generic_delete((l3obj*)delme,L3STDARGS_OBJONLY);

// general conventions for the generic method use:
//
//   a) what to do (incoming command or action): obj, exp, arity
//          (arity frequently defaulted to -1, exp to 0)
//
//   b) with references to:  env, curfo (current function object)
//
//   c) returning: *retval
//
//   d) new allocations owned by: owner
//
//   e) type we are expected to generate: etyp  (frequently defaulted to 0)
// 

#define L3KARG(methodname, num_args_required) \
L3METHOD(methodname) \
   l3obj* vv  = 0; \
   k_arg_op(0,num_args_required,exp,env,&vv,owner,curfo,etyp,retown,ifp); \
   l3path sexps(exp);


#define L3KMIN(methodname, min_num_args_required) \
L3METHOD(methodname) \
   arity = num_children(exp); \
   l3path sexps(exp); \
   if (arity < min_num_args_required) { \
       if (min_num_args_required == 1 && arity == 1) { \
           std::cout << "error in arity: " << exp->val() << " requires at least one argument; '"<< sexps() << "' had no arguments.\n"; \
       } else { \
           std::cout << "error in arity: " << exp->val() << " requires at least " << min_num_args_required  \
                     << " arguments; '"<< sexps() << "' had " << arity << " arguments.\n";                 \
       } \
       XRaise(XABORT_TO_TOPLEVEL); \
   } \
   l3obj* vv  = 0; \
   k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);




//#define L3KMINMAX(unittest,kmin,kmax)
//L3METHOD(methodname)                                      
//   l3obj* minmax = parse_eval("(v " #kmin " " #kmax ")");
//   l3obj* vv = 0;                                                  
//   k_arg_op(minmax,num_args_required,exp,env,&vv,owner,curfo,etyp);   
//   l3path sexps(exp);


// create a local object
#define QUICKO(oname) l3obj* oname = 0; make_new_obj(l3path(0,"quicko_%s",(STD_STRING_WHERE).c_str())(),"quicko_" #oname, owner, 0, &oname)


#define L3EVALSEXP(sexp) eval(0,-1,sexp,env,retval,owner,curfo,etyp,retown,ifp)

#define L3EVALINTO(sexp,rv) eval(0,-1,sexp,env,rv,owner,curfo,etyp,retown,ifp)

#define L3EVALIN(sexp,mye) eval(0,-1,sexp,mye,retval,owner,curfo,etyp,retown,ifp)

#define L3END(funcname) LLRCHECKALL(); return (int)l3rc; }

#define L3STUB(funcname) L3METHOD(funcname) L3END(funcname)


L3FORWDECL(arithmetic_with_closure)
L3FORWDECL(arithmetic_with_tmp_tag)
L3FORWDECL(callobject_dtor)
L3FORWDECL(class_l3base_destroy)
L3FORWDECL(do_progn)
L3FORWDECL(double_dtor)
L3FORWDECL(eval)
L3FORWDECL(eval_cd)
L3FORWDECL(eval_dot)
L3FORWDECL(eval_dotdot)
L3FORWDECL(fill_retval_with_copy_of)
L3FORWDECL(function_decl_deallocate_all_done_with_defn)
L3FORWDECL(function_decl_per_call_dtor)
L3FORWDECL(hash_dtor)
L3FORWDECL(make_closure)
//L3FORWDECL(write_into_double_vec_from_exp)

// make_new_callobject  now replaced with  make_new_captag() and fill_in_preallocated_new_callobject().
L3FORWDECL(make_new_callobject) // was commented out, but keep this for now for comparison while transitioning in the new
L3FORWDECL(old_make_new_callobject)
L3FORWDECL(make_new_captag)
L3FORWDECL(delete_captag)
L3FORWDECL(fill_in_preallocated_new_callobject)

L3FORWDECL(make_new_function_defn)

// convert from t_tdo to t_sxp, which wraps the toplevel parsed s-expression.
L3FORWDECL(sexp)
L3FORWDECL(sexpobj_dtor)
L3FORWDECL(sexpobj_cpctor)


L3FORWDECL(obj_ctor)
L3FORWDECL(obj_dtor)
L3FORWDECL(obj_trybody)
L3FORWDECL(run_closure)
L3FORWDECL(string_dtor)
L3FORWDECL(system_eval_trybody)
L3FORWDECL(system_ls_trybody)
L3FORWDECL(try_dispatch)
L3FORWDECL(universal_object_dispatcher)
L3FORWDECL(print_list_to_string)
L3FORWDECL(print_strings)
L3FORWDECL(eval_if_expr)
L3FORWDECL(test_and_eval_bool_expr)
L3FORWDECL(fresh_copy)
L3FORWDECL(fresh_copy_of_obj)

int is_true(l3obj* obj, long recursion_level);
L3FORWDECL(owns)
L3FORWDECL(eval_to_ptrvec)
L3FORWDECL(get_ownlist)
L3FORWDECL(member_a_of_b)
L3FORWDECL(create_vector)
L3FORWDECL(vv_to_doublev)
L3FORWDECL(vv_to_stringv)
L3FORWDECL(eval_len)

// arity specifies k, number of expected args (not including operator pos).
L3FORWDECL(k_arg_op)
L3FORWDECL(any_k_arg_op)

L3FORWDECL(exists)
L3FORWDECL(lexists)
L3FORWDECL(lookup)
L3FORWDECL(llookup)

L3FORWDECL(transfer)
L3FORWDECL(eval_parse)

//L3FORWDECL(parse)

L3FORWDECL(lexists)
L3FORWDECL(ioprim)
L3FORWDECL(strlen)

L3FORWDECL(fopen)
L3FORWDECL(fprint)
L3FORWDECL(fclose)
L3FORWDECL(fflush)
L3FORWDECL(fgets)
L3FORWDECL(fgetc)
L3FORWDECL(eq_string_string)
L3FORWDECL(nexttoken)
L3FORWDECL(make_new_fileh)

L3FORWDECL(generic_ctor)
L3FORWDECL(generic_trybody)
L3FORWDECL(generic_dtor)

L3FORWDECL(left)
L3FORWDECL(right)
L3FORWDECL(parent)
L3FORWDECL(sib)
L3FORWDECL(ischild)

L3FORWDECL(set_obj_left)
L3FORWDECL(set_obj_right) /* does child->sib */
L3FORWDECL(set_obj_parent)
L3FORWDECL(set_obj_sib)
L3FORWDECL(lastchild) /* find last child, addchild will use this to find sib; returns F if no children */
L3FORWDECL(addchild)

L3FORWDECL(setq)
L3FORWDECL(setq_rhs_first)

L3FORWDECL(numchild)
L3FORWDECL(firstchild)
L3FORWDECL(ith_child)

// boolean 
L3FORWDECL(and_)
L3FORWDECL(or_)
L3FORWDECL(not_)
L3FORWDECL(xor_)
L3FORWDECL(setdiff)
L3FORWDECL(intersect)
L3FORWDECL(all)
L3FORWDECL(any)
L3FORWDECL(union_)
L3FORWDECL(as_bool)


//loops
L3FORWDECL(foreach)
L3FORWDECL(aref)
L3FORWDECL(while1)
L3FORWDECL(break_)

// lists
L3FORWDECL(first)
L3FORWDECL(rest)
L3FORWDECL(cons)

// exceptions
L3FORWDECL(throw_)
L3FORWDECL(catch_)
L3FORWDECL(finally_)

// system commands
L3FORWDECL(sys)    // returns filehandle to popen(cmd)
L3FORWDECL(system) // print the output of the cmd
L3FORWDECL(poptop) // return to top level env

L3FORWDECL(to_string)

// history / auto-delete control
L3FORWDECL(history_limit_get)
L3FORWDECL(history_limit_set)

// view history
L3FORWDECL(history)




// quote generates symbols
L3FORWDECL(quote)

// display all aliases for the given object.
L3FORWDECL(aliases)
L3FORWDECL(valiases)
L3FORWDECL(canon)

// used by aliases:
L3FORWDECL(llref2ptrvec)



// macros


L3FORWDECL(run) // like apply. but with a better name. run the object's trybody method(s); or execute a lambda or function by name.


char* has_dot_before_first_space(char* input );

// the global nil; initialized in setup_and_init() in terp.cpp. Value in terp.cpp.
extern l3obj* gnil;
extern l3obj* gtrue;
extern l3obj* gna;
extern l3obj* gnan;


extern cmdhistory* histlog;

#ifndef  _DMALLOC_OFF
extern ser_mem_mon* gsermon;
#endif

L3FORWDECL(test_symvec)

// ls on tags
L3FORWDECL(lst)

L3FORWDECL(test_l3map)

// nearest common ancestor
L3FORWDECL(nca)

L3FORWDECL(lsb)

L3FORWDECL(hard_delete)
L3FORWDECL(softrm)
L3FORWDECL(setstop)
L3FORWDECL(getstop)
L3FORWDECL(prog1)

void assign_global_symbols();

extern unittest utest;
extern long  unittest_max;
extern long  unittest_cur;
extern long  unittest_last;

// signal that even sealed objects can be deleted.
extern long  global_terp_final_teardown_started;

#define L3METHOD_TMPCAPTAG(FUNC_WITH_TMP_CAPTAG)	\
 \
 L3METHOD(FUNC_WITH_TMP_CAPTAG) \
  \
  volatile l3obj* tmp_cap = 0; \
  volatile Tag*  tmp_tag = 0; \
  \
  volatile long   captag_sn = 0; \
  l3path basenm(#FUNC_WITH_TMP_CAPTAG "_tmp_captag"); \

  
// have to do set_sysbuiltin on tmp_cap, or else we cannot do (assert (allbuiltin)) in testing.
#define L3TRY_TMPCAPTAG(FUNC_WITH_TMP_CAPTAG, EXTRABYTES_IN_TMP_CAP)	\
  XTRY \
     case XCODE: \
         make_new_captag((l3obj*)&basenm, EXTRABYTES_IN_TMP_CAP, exp,env,(l3obj**) &tmp_cap , owner,0,t_cap,retown,ifp); \
         set_sysbuiltin(tmp_cap); \
 \
         captag_sn = tmp_cap->_ser; \
         tmp_tag = (volatile Tag*)(tmp_cap->_owner); \
         assert(tmp_cap->_owner == tmp_cap->_mytag); \
 \




#define L3END_CATCH_TMPCAPTAG(FUNC_WITH_TMP_CAPTAG)	\
 \
     break; \
 \
     case XFINALLY: \
           if (tmp_cap->_type && tmp_cap->_ser == captag_sn) { \
               set_notsysbuiltin(tmp_cap); \
               delete_captag((l3obj*)tmp_cap,L3STDARGS_OBJONLY); } \
           break; \
   XENDX \
 \
 L3END(FUNC_WITH_TMP_CAPTAG)
 

/*
// example use of function fx with 0 extra bytes required in the tmp_cap l3obj*.
L3METHOD_TMPCAPTAG(fx)
L3TRY_TMPCAPTAG(fx, 0)
L3END_CATCH_TMPCAPTAG(fx)
*/

L3FORWDECL(binop)
L3FORWDECL(unaryop)

// zero_dfs_been
// pass in a tag to be zeroed. Since various
// parts of the tree may be in 0 or 1 state, we first shift
// everything to -1, then to zero.
void zero_dfs_been(Tag* tag);

L3FORWDECL(function_defn_cpctor)
L3FORWDECL(double_cpctor)
L3FORWDECL(assert_with_tmp_tag)
L3FORWDECL(seal)
L3FORWDECL(allbuiltin)

L3FORWDECL(ctest)
L3FORWDECL(get_ctest)


#define RAISE_OBJ_TYPE(objtocheck,expectedtype) { assert(objtocheck); assert(objtocheck->_type); if (objtocheck->_type != expectedtype) { printf("RAISE_OBJ_TYPE check for expected type %s failed on object (type: %s, %p ser# %ld '%s') at %s.\n", expectedtype, objtocheck->_type, objtocheck, objtocheck->_ser, objtocheck->_varname, STD_STRING_WHERE.c_str()); XRaise(XABORT_TO_TOPLEVEL); } }

void  push_and_pop_global_tagenv_stacks(objlist_t* envpath);
void   print_objlist(objlist_t* objpath);

L3FORWDECL(generic_delete)


//
// input: obj has the function whose parent_env is to be set
// env  : has the env to set parent_env to.
//
L3FORWDECL(set_static_scope_parent_env_on_function_defn)
L3FORWDECL(defmethod)
L3FORWDECL(lhs_setq)

// rm handles multiples, vs hard_delete handles just one object.
L3FORWDECL(rm)


L3FORWDECL(invoke_optional_dtor)
L3FORWDECL(dfs_enumerate_owned)

// dstaq
L3FORWDECL(make_new_dstaq)

// logical args
L3FORWDECL(logical_binop)

int handle_cmdline_args(int argc, char** argv);
void queue_up_some_sexpressions(char* start, long* z, ustaq<sexp_t>* stk, Tag* retown, l3obj* env, FILE* fp, l3path* prompt);

// dstaq
L3FORWDECL(dq_clear)
L3FORWDECL(make_new_dq)
L3FORWDECL(dq)
L3FORWDECL(dq_dtor)
L3FORWDECL(dq_cpctor)
L3FORWDECL(dq_del_ith)

L3FORWDECL(dq_ith)

L3FORWDECL(dq_len)
L3FORWDECL(dq_size)
L3FORWDECL(dq_find_name)
L3FORWDECL(dq_find_val)

L3FORWDECL(dq_erase_name)
L3FORWDECL(dq_erase_val)

L3FORWDECL(dq_front)
L3FORWDECL(dq_pushfront)
L3FORWDECL(dq_popfront)

L3FORWDECL(dq_back)
L3FORWDECL(dq_pushback)
L3FORWDECL(dq_popback)


// message queue
L3FORWDECL(make_new_mq)
L3FORWDECL(mq)
L3FORWDECL(mq_dtor)
L3FORWDECL(mq_cpctor)
L3FORWDECL(mq_del_ith)

L3FORWDECL(mq_ith)

L3FORWDECL(mq_len)
L3FORWDECL(mq_size)
L3FORWDECL(mq_find_name)
L3FORWDECL(mq_find_val)

L3FORWDECL(mq_erase_name)
L3FORWDECL(mq_erase_val)

L3FORWDECL(mq_front)
L3FORWDECL(mq_pushfront)
L3FORWDECL(mq_popfront)

L3FORWDECL(mq_back)
L3FORWDECL(mq_pushback)
L3FORWDECL(mq_popback)


// doubly linked list: dd / t_ddt
L3FORWDECL(make_new_dd)
L3FORWDECL(dd)
L3FORWDECL(dd_dtor)
L3FORWDECL(dd_cpctor)
L3FORWDECL(dd_del_ith)

L3FORWDECL(dd_ith)

L3FORWDECL(dd_len)
L3FORWDECL(dd_size)

L3FORWDECL(dd_front)
L3FORWDECL(dd_pushfront)
L3FORWDECL(dd_popfront)

L3FORWDECL(dd_back)
L3FORWDECL(dd_pushback)
L3FORWDECL(dd_popback)



// checkzero puts a point in the memlog
//  where user allocations should be flat:
//  the same as (assert (allbuiltin))-- but
//  the log note allows us to optimize the
//  offline log processing.
L3FORWDECL(checkzero)
L3FORWDECL(allsane)

L3FORWDECL(link)
L3FORWDECL(relink)
L3FORWDECL(chase)
L3FORWDECL(linkname)
L3FORWDECL(rename)


// in l3ts_server.cpp

// now nv3
//L3FORWDECL(nv)
//L3FORWDECL(nv_print)
//L3FORWDECL(nv_cpctor)
//L3FORWDECL(nv_dtor)
//L3FORWDECL(nv_load)
//L3FORWDECL(nv_save)

L3FORWDECL(save)
L3FORWDECL(load)

L3FORWDECL(dou_save)
L3FORWDECL(dou_load)
L3FORWDECL(open_nv)
L3FORWDECL(discern_msg_type)


L3FORWDECL(dou_save)
L3FORWDECL(dou_load)

#define L3TYPE_FORW(tname) \
L3FORWDECL(tname) \
L3FORWDECL(tname##_load) \
L3FORWDECL(tname##_save) \
L3FORWDECL(tname##_print) \
L3FORWDECL(tname##_cpctor) \
L3FORWDECL(tname##_dtor) 


L3TYPE_FORW(qts)
L3TYPE_FORW(nv3)
L3TYPE_FORW(ob3)
L3TYPE_FORW(tm3)

L3TYPE_FORW(gl3)
L3TYPE_FORW(tp3)
L3TYPE_FORW(da3)

L3TYPE_FORW(ts3)
L3TYPE_FORW(be3)
L3TYPE_FORW(si3)

L3TYPE_FORW(bs3)
L3TYPE_FORW(wh3)
L3TYPE_FORW(id3)

L3TYPE_FORW(he3)
L3TYPE_FORW(ds3)
L3TYPE_FORW(qt3)

// nameval subtypes
L3TYPE_FORW(dou)
L3TYPE_FORW(lng)
L3TYPE_FORW(str)
L3TYPE_FORW(obj)
L3TYPE_FORW(byt)
L3TYPE_FORW(vvc)

// in lex_twopointer.cpp
L3FORWDECL(make)

// check for and prevent reserved (protobuf) words
L3FORWDECL(is_protobuf_reserved_word)

void cpp_comment_to_blank(char* s, char* stop);

// top down operator precedence parsing (Pratt parsing)
L3FORWDECL(tdop)
//L3FORWDECL(addtxt) // add more text to a tdop parser.
L3FORWDECL(pratt) // start infix mode interpretation.
L3FORWDECL(prefix) // switch back to old school.

L3FORWDECL(double_unaryop)

// allow the parser code to just request more input if it hits eof.
//
long get_more_input(FILE* fp, l3path* prompt, bool show_prompt_if_not_stdin, bool echo_line, bool show_plus, int& feof_fp);

L3FORWDECL(open_square_bracket)

// user defined exception hanlding
L3FORWDECL(l3x_throw)
L3FORWDECL(l3x_try)
L3FORWDECL(l3x_catch)
L3FORWDECL(l3x_finally)
L3FORWDECL(l3x_handled)

extern Tag* glob;

L3FORWDECL(system_eval_trybody)
L3FORWDECL(system_eval_ctor)
L3FORWDECL(function_defn_cpctor)

L3FORWDECL(c_for_loop)
L3FORWDECL(c_continue)
L3FORWDECL(c_break)

L3FORWDECL(plusplus_minusminus)

#endif // TERP_H
