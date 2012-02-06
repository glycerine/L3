 //
 // Copyright (C) 2011 Jason E. Aten. All rights reserved.
 //

#include "l3xcep.h"
#include "l3pratt.h"
#include "l3dstaq.h"

// user defined exceptions. and system support for break/continue in loops.



L3METHOD(l3x_throw)
{
    DV(printf("in l3x_throw stub.\n");
       exp->print("exp in l3x_throw:  "));

    l3obj* pXVal = 0;

    arity = exp->nchild();

    XTRY
       case XCODE:
           if (arity>=0) {

               // save exception values with glob owning...if not the exception dstaq
               LIVET(exception_stack->_mytag);
               do_progn(0,arity,exp,  env, &pXVal,owner,curfo,etyp,    exception_stack->_mytag,ifp);
               dq_pushback_api(exception_stack, pXVal,0);
           }
           break;

       case XFINALLY:
           l3throw(XUSER_EXCEPTION);
           break;

   XENDX

}
L3END(l3x_throw)



L3METHOD(l3x_try)
{
    DV(printf("in l3x_try stub.\n");
       exp->print("exp in l3x_try:  "));

   qtree* catch_node =0;
   long   catch_i = -1;
   qtree* finally_node = 0;
   long   finally_i = -1;
   long   i = 0;

   // scan for catch and finally
   arity = exp->nchild();
   qtree* ch = 0;
   for (long i =0; i < arity; ++i) {
       ch= exp->ith_child(i);
       if (ch->_ty == t_obr && ch->_headnode) {
           if (0 == ch->headval().strcmp("finally")) {
               if (finally_i != -1) {
                   printf("error in try block: only one finally block allowed. This expression had more than one finally declaration:\n");
                   exp->printspan(0,"      ");
                   l3throw(XABORT_TO_TOPLEVEL);
               }
               finally_i = i;
               finally_node = ch;
               //               l3x_finally(obj,ch->nchild(),ch,  L3STDARGS_ENV);

           } else if (0 == ch->headval().strcmp("catch")) {
               if (catch_i != -1) {
                   printf("error in try block: only one catch block allowed. This expression had more than one catch declaration:\n");
                   exp->printspan(0,"      ");
                   l3throw(XABORT_TO_TOPLEVEL);
               }
               catch_i = i;
               catch_node = ch;
               //               l3x_catch(obj,ch->nchild(),ch,  L3STDARGS_ENV);
           }
       }
   }

 XTRY 
    case XCODE:
       ch =0;
       for (i =0; i < arity; ++i) {
           if (i == catch_i || i == finally_i) continue;
           ch= exp->ith_child(i);
           eval(obj, -1,ch, L3STDARGS_ENV);
       }
   break;
 case XUSER_EXCEPTION:
     if (catch_node) {
         arity = catch_node->nchild();
         for (i =0; i < arity; ++i) {
             ch= catch_node->ith_child(i);
             eval(obj, -1,ch, L3STDARGS_ENV);
         }
     }
    break;
 case XFINALLY:

     if (0==XData.IsHandled) {

         if (catch_node) {
             arity = catch_node->nchild();
             for (i =0; i < arity; ++i) {
                 ch= catch_node->ith_child(i);
                 eval(obj, -1,ch, L3STDARGS_ENV);
             }
         }
     }

     if (finally_node) {
         arity = finally_node->nchild();

         for (i =0; i < arity; ++i) {
             ch= finally_node->ith_child(i);
             eval(obj, -1,ch, L3STDARGS_ENV);
         }
         // seems necessary to get ($finally) call to function inside the above eval:         
         XData.State = XFinally;


     }
    break;
 XENDX

}
L3END(l3x_try)




L3METHOD(curly_brace_handler)
{

  DV(std::cout << "diagnostics in curly_brace_handler, exp: " << exp->_span << "\n");

  if (!exp || 0==exp->_val.b || exp->_ty != t_obr) {
      printf("error in curly_brace_handler: expected type t_obr and '{' as _val.\n");
      l3throw(XABORT_TO_TOPLEVEL);
  }


  assert(obj);
  l3path& name_to_insert = *((l3path*)obj);

  // should be same as name_to_insert:  sexp_t* fn     = exp->ith_child(0);
  if (exp->headnode() == 0) {
      printf("error in curly_brace_handler: expression did not have a headnode.\n");
      exp->p();
      l3throw(XABORT_TO_TOPLEVEL);      
  }

  sexp_t* decl = 0;
  sexp_t* body = exp;

    size_t txtlen = 0;   // strlen(linebuf)+1;

    size_t firstsz = txtlen + sizeof(defun);
    size_t alignbytes = firstsz % 8;
    size_t total_extra = firstsz + (8 - alignbytes);

    if (sizeof(l3obj)+total_extra > INT_MAX) {
        long overtot = sizeof(l3obj) + total_extra;
        printf("error in curly_brace_handler: object too big: requested size %ld exceeds max allowed object size %ld (INT_MAX).\n",overtot, (long)INT_MAX);
        return 0;
    }

    l3path objname(0,"%s_function_defn",name_to_insert());
    l3path clsname("function_defn");

    l3obj* p = 0;
    make_new_captag((l3obj*)&objname,total_extra,exp,  env,(l3obj**)&p,owner,  (l3obj*)&clsname,t_fun,retown,ifp);
    assert(p);
    assert(p->_mytag);
    Tag*   new_tag = (Tag*)(p->_mytag);

    // make a copy of the parse... owned by the defn.

    // transfer exp, decl and body, so we have them.
    transfer_subtree_to(exp, new_tag);


    defun* df = (defun*)p->_pdb;

    df = new(df) defun(); // placement new, so that the vectors can get cleaned-up sanely.
    df->defn_qtree = exp;
    df->propset = decl;
    df->body =  body;
    df->myob = p;
    df->env = env;

    p->_sib = 0;


  /* system_eval methods */
  p->_dtor    = &function_decl_deallocate_all_done_with_defn;

  p->_trybody = &system_eval_trybody;
  p->_ctor    = &system_eval_ctor;
  p->_cpctor  = &function_defn_cpctor;

  p->_type = t_fun;

  LIVEO(p);

  DV(printf("here in curly_brace_handler is the object, pre set_sha1:\n");
     print(p,"",0));

  set_sha1(p);
  
  *retval = p;

}
L3END(curly_brace_handler)




L3METHOD(l3x_catch)
{
    DV(printf("in l3x_catch stub.\n");
       exp->print("exp in l3x_catch:  "));

  l3obj* fun = 0;
  l3path nm("catch");
  curly_brace_handler((l3obj*)&nm,-1, exp, env,  &fun, owner, 0, t_fun, retown,ifp);

  fun->_parent_env = env;
  add_alias_eno(env, "catch", fun);

}
L3END(l3x_catch)



L3METHOD(l3x_finally)
{
    DV(printf("in l3x_finally stub.\n");
       exp->print("exp in l3x_finally:  "));

  l3obj* fun = 0;
  l3path nm("finally");
  curly_brace_handler((l3obj*)&nm,-1, exp, env,  &fun, owner, 0, t_fun, retown,ifp);

  fun->_parent_env = env;

  add_alias_eno(env, "finally", fun);

}
L3END(l3x_finally)



L3METHOD(l3x_handled)
{
    XHandled();
}
L3END(l3x_handled)

