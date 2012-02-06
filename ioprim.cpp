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
 // #include "minifs.h"
 //#include "slisp_util.h"

 #include "objects.h"
 #include "terp.h"

using std::vector;
using std::string;

#include "ioprim.h"
#include "qexp.h"
#include "l3pratt.h"

/////////////////////// end of includes

//
// ioprim.cpp: I/O primitives
//
 
/*  wrap these <stdio.h> C library functions:

       FILE *fopen(const char *path, const char *mode);
       int fclose(FILE *fp);  // also happens on rm of fhl object
       int fflush(FILE *stream);

       ssize_t getline(char **lineptr, size_t *n, FILE *stream);
       ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);

       int fgetc(FILE *stream);
       int ungetc(int c, FILE *stream);

*/ 

L3METHOD(filehandle_dtor)

   RAISE_OBJ_TYPE(obj,t_flh);
   FILE* fh = 0;
   ptrvec_get(obj,0,(l3obj**)&fh);
   if (fh) fclose(fh);
   obj_dtor(L3STDARGS);

L3END(filehandle_dtor)

L3METHOD(fopen)
{
   arity = 2; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());

   l3obj* path = 0;
   l3path spath;
   ptrvec_get(vv,0,&path);
   string_get(path,0,&spath);

   l3obj* mode = 0;
   l3path smode;
   ptrvec_get(vv,1,&mode);
   string_get(mode,0,&smode);
   FILE* fh = 0;
   
   XTRY
       case XCODE:
           fh = fopen(spath(), smode());
           if (NULL == fh) {

#define BARF_ERRNO(opname,MSG)   \
               std::cout << "error on " << opname << ": " << MSG << strerror(errno)  << "\n"; \
               l3throw(XABORT_TO_TOPLEVEL);

           BARF_ERRNO(opname," returned NULL. ");

           }
           obj = (l3obj*)fh;
           make_new_fileh(L3STDARGS);
       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX
}
L3END(fopen)

L3METHOD(make_new_fileh)
   make_new_obj("fileh","fileh",retown,0,retval);
   l3obj* nobj = *retval;

   set_singleton(nobj); // don't duplicate the FILE*
   nobj->_type = t_flh;
   ptrvec_set(*retval,0,obj);
   nobj->_dtor  = &filehandle_dtor;
   nobj->_cpctor = &filehandle_cpctor;

L3END(make_new_fileh)


L3METHOD(filehandle_cpctor)

#if 0 // this does shallow copy, we probably want deep copy for file handles!
         LIVEO(obj);
         l3obj* src  = obj;     // template
         l3obj* nobj = *retval; // write to this object
         assert(src);
         assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()

   nobj->_type = t_flh;

   l3obj* existing_fh = 0;
   ptrvec_get(src,0, &existing_fh);
   ptrvec_set(nobj,0,existing_fh);

#endif

L3END(filehandle_cpctor)


L3METHOD(fclose)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());

   l3obj* fhandle = 0;
   FILE* fh = 0;

   XTRY
       case XCODE:

#define FHANDLE_OR_BARF() \
           ptrvec_get(vv,0,&fhandle); \
           if (fhandle->_type != t_flh) { \
               std::cout << "error: " << opname; \
               printf(" requires a file handle object as first arg; found %s instead of t_flh.\n", fhandle->_type); \
               l3throw(XABORT_TO_TOPLEVEL); \
           }

           FHANDLE_OR_BARF();

#define CHECK_NULL_FH() \
           ptrvec_get(fhandle,0,(l3obj**)&fh); \
           if (!fh) { \
               std::cout << "error on " << opname << ": filehandle object was NULL.\n"; \
               l3throw(XABORT_TO_TOPLEVEL); \
           }

           CHECK_NULL_FH();

           // set to zero upon close, so we don't try to close it again upon deletion.
           ptrvec_set(fhandle,0,0);

           if (fclose(fh)) {
               BARF_ERRNO(opname,"");
           }

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(fclose)

L3METHOD(fflush)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());
   l3obj* fhandle = 0;
   FILE* fh = 0;

   XTRY
       case XCODE:
           fhandle = 0;
           FHANDLE_OR_BARF();

           fh = 0;

           CHECK_NULL_FH();

           if (fflush(fh)) {
               BARF_ERRNO(opname,"");
           }
           break;
           case XFINALLY:
               generic_delete(vv, L3STDARGS_OBJONLY);   
           break;
   XENDX

L3END(fflush)

L3METHOD(fgets)

    arity = 1; // number of args, not including the operator pos.
    k_arg_op(L3STDARGS);
    l3obj* vv = *retval;
    *retval = 0;
   qqchar opname(exp->first_child()->val());

    l3obj* fhandle = 0;
    FILE* fh = 0;
    l3path line;

   XTRY
       case XCODE:
           FHANDLE_OR_BARF();
           CHECK_NULL_FH();

           if (NULL == fgets(line(),line.bufsz(),fh)) {
               BARF_ERRNO(opname,"");
           }
           
           *retval = make_new_string_obj(line(), retown, "string_from_fgets");
           
       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(fgets)


L3METHOD(fgetc)
{
   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());

   l3obj* fhandle = 0;
   FILE* fh = 0;
   int c = 0;
   l3path s;

   XTRY
       case XCODE:
 
           FHANDLE_OR_BARF();
           CHECK_NULL_FH();

           c = fgetc(fh);
   
           if (EOF == c) {
               BARF_ERRNO(opname,"");
           }

           s.reinit("%c",c);

           *retval = make_new_string_obj(s(), retown, "string_from_fgets");
       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX
}
L3END(fgetc)


// (fprint fh stringtoprint)
L3METHOD(fprint)
{
   arity = 2; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());
   l3obj* fhandle = 0;
   l3obj* printme = 0;
   l3path sprintme;
   FILE* fh = 0;

   XTRY
       case XCODE:

           FHANDLE_OR_BARF();

           ptrvec_get(vv,1,&printme);
           string_get(printme,0,&sprintme);   
           if (sprintme.len() == 0) {
             break;
           }
           
           CHECK_NULL_FH();
           
           fprintf(fh,"%s",sprintme());
           
       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

}
L3END(fprint)


// return 0 if recognized, else -1.
// like 	if (0 == test_and_eval_bool_expr(obj,arity,exp,env, retval, owner, curfo, etyp)) return 0;
L3METHOD(ioprim) 
{
    if (0== exp || 0==exp->nchild()) return -1;
    
    qqchar s(exp->first_child()->val());
    

    if (0==s.strcmp("fopen")) {  fopen(L3STDARGS); return 0; }
    if (0==s.strcmp("fclose")) {  fclose(L3STDARGS); return 0; }
    if (0==s.strcmp("fflush")) {  fflush(L3STDARGS); return 0; }
    
    if (0==s.strcmp("fprint")) {  fprint(L3STDARGS); return 0; }
    //if (0==s.strcmp("fscanf"))  {   fscanf(L3STDARGS); return 0; }
    
    if (0==s.strcmp("fprint")) {  fprint(L3STDARGS); return 0; }
    //if (0==s.strcmp("scanf"))  {   scanf(L3STDARGS); return 0; }
    
    if (0==s.strcmp("fgets")) {  fgets(L3STDARGS); return 0; }
    if (0==s.strcmp("fgetc")) {  fgetc(L3STDARGS); return 0; }
    if (0==s.strcmp("nexttoken")) {  nexttoken(L3STDARGS); return 0; }
    
    return -1;
}
L3END(ioprim)



L3METHOD(nexttoken)
{
   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   qqchar opname(exp->first_child()->val());

   l3obj* fhandle = 0;
   FILE* fh = 0;

   l3path token;
   errno = 0;

   // handle quoted strings as one token

    ccs state = code;

    const char bksl = '\\'; // backslash
    const char dquo = '\"'; // doublequote
    const char spc =  ' ';
    const char tab =  '\t';
    const char newl = '\n';
    const char semi = ';';
   int started = 0;
   int eof = 0;
   int c = 0;
   int done = 0;
   
   XTRY
       case XCODE:
           FHANDLE_OR_BARF();
           CHECK_NULL_FH();


#define ADDC started = 1; *(token.p)=c; ++(token.p); *(token.p)='\0'

           while(!done) {
             c = fgetc(fh);
             if (EOF == c) {
               eof = 1;
               if (0 != errno) {
                   BARF_ERRNO(opname,"");
               }
               done = 1;
               break;
             }

             switch(state) {
             case code:
               switch(c) {
               case bksl: state = slashcode; ADDC; break;
               case dquo: state = dquote; ADDC; break;
               case semi: done =1; if (started) { ungetc(c,fh); } else { ADDC; } break;
               case spc:  case tab: case newl:
                 if (started) {
                   ungetc(c,fh);
                   done = 1;
                   break;
                 } else {
                   // gotta get to our first character...
                   continue;
                 }
               default:
                 ADDC;
               }
               break;
             case dquote:
               switch(c) {
               case bksl: state = slashdquote; ADDC;  break;
               case dquo: state = code; ADDC; break;
               default:
                 ADDC;
               }
               break;
             case slashcode:
               state = code; ADDC;
               break;
             case slashdquote:
               state = dquote; ADDC;
               break;
             default:
                 break;
             }
             
           } // end while ! done

           if (started) {
             *retval = make_new_string_obj(token(), retown, "nexttoken");
           } else {
             *retval = gnil;
           }

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX
}
L3END(nexttoken)

