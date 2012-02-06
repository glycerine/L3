//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
 #include "jtdd.h"
 #include "jlmap.h"
 #define _ISOC99_SOURCE 1 // for NAN
 #include <math.h>
 #include <valgrind/valgrind.h>
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



 #include "objects.h"
 #include "l3link.h"

 #include <openssl/sha.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>


// (c-set-style "whitesmith")


 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"

#include "serialfac.h"
#include "terp.h" // for eval(), extern gnan, etc.
#include "judydup.h"
#include "tostring.h"
#include "symvec.h"
#include "llref.h"
#include "l3dstaq.h"
#include "dynamicscope.h"
#include "l3ts.pb.h"
#include "l3mq.h"
#include "l3dd.h"
#include "l3string.h"
#include "l3matrix.h"
#include "l3pratt.h"

/////////////////////// end of includes


//////////////// macros and typedefs
//////////////// globals and statics


char* global_ico = (char*)"!>";
char* global_iso = (char*)"@>";
char* global_oco = (char*)"@<";
char* global_oso = (char*)"!<";

char* notavail_global_jlmap_not_available = (char*)"notavail_global_jlmap_not_available";

EnvStack     global_env_stack; // global/default env for var lookup / namespace
//MethodStack  global_method_stack; // global/default env for method lookup

stopset* global_stopset = 0;

//minifs mini; // little convenient filesystem wrapper, extern is at the end of autotag.h

//const int  JERROR = -4;  // an error code, now in user_xcep.h

char cwd[PATH_MAX+1];

int enDV = 0;

// used by stringify(sexp)
l3path stringify_sexp_helper;


/////////// declarations

// from objects.h
//void dump_objstruct(objstruct* a, const char* indent);
void  print_obj(l3obj* obj,const char* indent, stopset* stoppers);


#if 0 // old
char* stringify(sexp_t* s) {
  stringify_sexp_helper.init();
  long n = print_sexp((l3obj*)stringify_sexp_helper.buf,PATH_MAX,s, 0,0,defptag_get(),  0,0,defptag_get() );
  if (n > 0) {
    stringify_sexp_helper.p += n;
  } else {
    stringify_sexp_helper.init("error printing sexp with print_sexp(): too long perhaps?");
  }

  return &(stringify_sexp_helper.buf[0]);
}

void dump(sexp_t* s ) {
  char buf[BUFSIZ];
  bzero(buf,BUFSIZ);
  print_sexp((l3obj*)buf,BUFSIZ,s,  0,0,glob,  0,0,glob );
  printf("%s\n",buf);
}
#endif  // old


char* pwd() {
  bzero(cwd,PATH_MAX+1);
  char* r = getcwd(cwd,PATH_MAX);
  if (!r) {
    fprintf(stderr,"error in pwd(); error is: '%s'\n",strerror(errno));    
    assert(0);
  }
  return cwd;
}



//
// same as SHA1 but return 48 byte hex ascii hex string encoding of the digest, with the last 8 bytes all '\0'
//
const  int sha1_buf_padded = 48;

unsigned char* HEX_SHA1(const unsigned char *d, unsigned long n, unsigned char* md) {

  static unsigned char buf[sha1_buf_padded];
  static unsigned char b[sha1_buf_padded];
  static char hex[] = "0123456789ABCDEF";

  bzero(buf,sha1_buf_padded);
  bzero(b, sha1_buf_padded);

  SHA1(d,n, buf);

  int j = 0;
  for (int i = 0; i < 20; i++) {
    j = i*2;
    b[j]   = (char)(hex[(int)( (unsigned char)buf[i] & (unsigned char)0x0F )]);
    b[j+1] = (char)(hex[(int)( ((unsigned char)buf[i] & (unsigned char)0xF0 ) >> 4)]);
  }

  if (md) {
    memcpy(md,b,48);
    return md;
  }

  return &b[0];
}

l3obj sample_for_sizing;

// we define the area we want to hash starting beyond the sha1 itself, so that it doesn't confuse
// later hashing.
long JUST_PAST_SHA1_OFFSET = (unsigned long)(&(sample_for_sizing._varname[0])) - (unsigned long)(&sample_for_sizing);

void  set_sha1(l3obj* o) {


#if 0  // temporarily turn off this feature; we lack tests and it was causing some valgrind issues; not using it at the moment.
  bzero(o->_sha1,sizeof(o->_sha1));

  HEX_SHA1(  (unsigned char*)o + JUST_PAST_SHA1_OFFSET,
     o->_malloc_size - JUST_PAST_SHA1_OFFSET,
     (unsigned char*)o->_sha1);
#endif

}


l3path  shaobj(l3obj* o) {
  l3path ret;

  HEX_SHA1(  (unsigned char*)o + JUST_PAST_SHA1_OFFSET,
     o->_malloc_size - JUST_PAST_SHA1_OFFSET,
     (unsigned char*)ret.buf);

  ret.set_p_from_strlen();
  return ret;
}


void  get_sha1(l3obj* o, l3path& sha) {
  sha.init(0);

  HEX_SHA1(  (unsigned char*)o + JUST_PAST_SHA1_OFFSET, 
     o->_malloc_size - JUST_PAST_SHA1_OFFSET,
     (unsigned char*)sha.buf);

  sha.set_p_from_strlen();
}

void print_sha1(l3obj* o) {

  l3path sha;
  get_sha1(o,sha);

  sha.outln();
}


double parse_double(char* d, bool& is_double) {

  char* endptr = d;
  double dval = strtod(d,&endptr);
  if (endptr != d) {
    is_double = true;
  } else {
    is_double = false;
  }

  return dval;
}


bool is_atom(sexp_t* s) {
    return(s->_ty == t_ato || s->_ty == t_dou);
}

bool is_list(sexp_t* s) {
    return !is_atom(s);
}

#if 0 // obsolete
// in our simple system, atoms can be strings or doubles
t_typ get_atom_type(sexp_t* s, double& dval){
  assert(s->ty == SEXP_VALUE);

  char* endptr = s->val;
  dval = strtod(s->val,&endptr);
  if (endptr != s->val) {
    // is a double...
    return t_dou;
  }

  return s->_ty;

  // see sexp.h
  /*
    elt_t is either { SEXP_VALUE which is an atom, or SEXP_LIST which is the head of a list}

typedef enum { 
  SEXP_BASIC, //Basic, unquoted value (string)
  SEXP_SQUOTE, // Single quote (tick-mark) value - contains a string representing a non-parsed portion of the s-expression.
  SEXP_DQUOTE,   // Double-quoted string.  Similar to a basic value, but potentially containing white-space.
  SEXP_BINARY   //Binary data.  This is used when the specialized parser is active and supports inlining of binary blobs of data inside an expression.
} atom_t
  */

} // end get_type
#endif

#if 0 // obsolete, just pass in the t_typ 
const char* owntag_en2str(owntag_en ot) {

  switch(ot) {
  case in_c_own: return  "!>"; break;
  case in_s_own: return  "@>"; break;
  case out_c_own: return "@<"; break;
  case out_s_own: return "!<"; break;    
  default:
      return "bad_owntag";
  }

  return 0;
}
#endif


/////////// get/set_reserved_bit :
uint get_reserved_bit(uint i, uint v) {
  return (uint)(((v & (1 << (i))) >> (i)));
}

uint set_reserved_bit(uint i, uint v, uint store) {
  return ((uint)(store << (i) | ((~(1 << (i))) & v)));
}


/////////// get/set_owntag : 4 states for each tag, 16 2-bit tags per 32-bits

uint get_owntag(uint i, uint v) {
  return (uint)(((v & ((uint)3 << (i << 1))) >> (i << 1)));
}

uint set_owntag(uint i, uint v, uint store) {
  return ((uint)(store << (i << 1) | ((~((uint)3 << (i << 1))) & v)));
}

void global_function_entry() {
  //  printf("global_function_entry()\n");
}

void global_function_finally() {
  //  printf("global_function_finally()\n");
}


void FuncThrowsJERROR() {
  printf("in FuncThrowsJERROR: about to throw a JERROR!\n");
  l3throw(JERROR);
  printf("in FuncThrowsJERROR: *AFTER* we threw a JERROR! *********** should never see this!\n");
}

double JasonsTestFunction()
// test function containing an XTRY block
{
  //NEW_AUTO_TAG(stacktag,glob);

  double volatile res;
  printf("in JasonsTestFunction(): at the top, before the try block.\n");
  res = 5.0/4.0;
  l3obj* ptr = 0;

   XTRY
      case XCODE:

  /* problem... this could be lost, yes */
          ptr = (l3obj*)class_l3base_create(0, 0, 0);
          //GCADD(p); // would be done automatically by new

          FuncThrowsJERROR();

         break;
     case JERROR:

       printf("in JasonsTestFunction(): we see a JERROR!\n");

       XHandled();
       //printf("in JasonsTestFunction(): and NOT handling the JERROR, to see it propagate...\n");
         break;
      default:
         printf("unknown exception!\n");
         break;
      case XFINALLY:
    printf("in JasonsTestFunction(): we are doing our XFINALLY BLOCK!!! - user code here\n");



    printf("in JasonsTestFunction(): msg 2/3 in XFINALLY BLOCK -- before class_Tag_destroy(stacktag)\n");


      // autoplaced...
    //class_Tag_destroy(&stacktag,0,0);


    //        FINALLY_POP_GCSTACK();

    printf("in JasonsTestFunction():  msg 3/3 in XFINALLY BLOCK -- after  class_Tag_destroy(stacktag)\n");

         break;
   XENDX

     printf("in JasonsTestFunction(): This follows the XENDX try-catch block...!!! with res = %f\n",res);

   return res;
}


typedef void (*genericfunction)();

l3obj* tricky(int a1, long a2, l3obj a3, l3obj* a4, l3obj** a5, l3obj*** a6) {

  return 0;
}

#if 0
void my_univ_function(merlin::call& m) {
  
  ffi_cif cif;
  ffi_type *args[1];
  void *values[1];
  const char *s;
  int rc;

  /* Initialize the argument info vectors */
  args[0] = &ffi_type_pointer;
  values[0] = &s;
  
  /* Initialize the cif */
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
           &ffi_type_uint, args) == FFI_OK)
    {
      s = "Hello World!";
      ffi_call(&cif, (genericfunction)puts, &rc, values);
      /* rc now holds the result of the call to puts */
      
      /* values holds a pointer to the function's arg, so to
     call puts() again all we need to do is change the
     value of s */
      s = "This is cool!";
      ffi_call(&cif, (genericfunction)puts, &rc, values);
    }
  
  return;

  /*
  ffi_cif CIF;
  RTYPE r;
  ffi_type* ARGTYPES = new ffi_type[m.narg];

  ffi_prep_cif(&CIF, FFI_DEFAULT_ABI, m.narg, &r, );
  
  // libffi supports closures.


  ffi_status s = ffi_call(invo);

  if (s == FFI_OK) {}
  if (s == FFI_BAD_TYPEDEF) {}
  if (s == FFI_BAD_API) {}

  //  RVALUE
  //  AVALUES
  */

}


//typdef void (*FFI_CLOSURE_FUNC_PTR) (ffi_cif *CIF, void *RET, void **ARGS, void *USER_DATA);

void my_closure_func(ffi_cif *CIF, void *RET, void **ARGS, void *USER_DATA) {
  printf("my_closure_func called!!\n");

  printf("USER_DATA points to: %p\n",USER_DATA);
  printf("RET points to %p\n",RET);
  printf("CIF points to %p\n",CIF);
  printf("ARGS points to %p\n", ARGS);

  uint narg = CIF->nargs;
  ffi_type** arg_types = CIF->arg_types;
  //ffi_type* pRtype = CIF->rtype;
  //uint bytes = CIF->bytes;
  //uint flags = CIF->flags;
    
  for (uint i = 0; i < narg; ++i) {
    printf("argument %d is of type: ",i);
    switch (arg_types[i]->type) {

      break; case  FFI_TYPE_VOID:      /*  0 */ printf("FFI_TYPE_VOID");
      break; case  FFI_TYPE_INT:       /*  1 */ printf("FFI_TYPE_INT");
      break; case  FFI_TYPE_FLOAT:     /*  2 */ printf("FFI_TYPE_FLOAT");
      break; case  FFI_TYPE_DOUBLE:    /*  3 */ printf("FFI_TYPE_DOUBLE");
      break; case  FFI_TYPE_LONGDOUBLE: /* 4 */ printf("FFI_TYPE_LONGDOUBLE");
      break; case  FFI_TYPE_UINT8:     /*  5 */ printf("FFI_TYPE_UINT8");
      break; case  FFI_TYPE_SINT8:     /*  6 */ printf("FFI_TYPE_SINT8");
      break; case  FFI_TYPE_UINT16:    /*  7 */ printf("FFI_TYPE_UINT16");
      break; case  FFI_TYPE_SINT16:    /*  8 */ printf("FFI_TYPE_SINT16");
      break; case  FFI_TYPE_UINT32:    /*  9 */ printf("FFI_TYPE_UINT32");
      break; case  FFI_TYPE_SINT32:    /*  10 */ printf("FFI_TYPE_SINT32");
      break; case  FFI_TYPE_UINT64:    /*  11 */ printf("FFI_TYPE_UINT64");
      break; case  FFI_TYPE_SINT64:    /*  12 */ printf("FFI_TYPE_SINT64");
      break; case  FFI_TYPE_STRUCT:    /*  13 */ printf("FFI_TYPE_STRUCT");
      break; case  FFI_TYPE_POINTER:   /*  14 */ printf("FFI_TYPE_POINTER");
      break; default:
      assert(0); // unrecognized type...
    }

    printf("\n");
  } // end i over narg
    printf("\n");

}


// do this analysis once, at load time. Cache the results for each function.
// so we only have to do it once.

// ret[0] = true iff returned object is an _l3obj*
// ret[1] = true iff 1st argument to the given function call is an _l3obj*
// ret[2] = true iff 2nd argument to the given function call is an _l3obj*
// ...
// so the length of returned vector ret = arity of function + 1.
//
// only now we compress ret into a 64bit long integer, instead of a vector of bool.
//
ulong locate_l3obj_ptr_in_call(void* a) {

  system("nm  test_l3obj   | grep l3obj  |fld -e 1 | egrep \"^T.*\" |fld -e 1 > test_l3obj.nm" );
  system("nm -C test_l3obj | grep _l3obj |fld -e 1 | egrep \"^T.*\" |fld -e 1 > test_l3obj.nmC");
  
  

  //  const std::type_info&  ti = typeid(tricky);
  const std::type_info&  ti = typeid(l3obj*);

  int status = 0;
  char* realname = abi::__cxa_demangle(ti.name(), 0, 0, &status);
  printf("the mangled type returned by typeid() is: '%s'   with __cxa_demangle() showing: realname: '%s'   status:%d\n",
     ti.name(), realname, status );
  if (status != -1) { free(realname); }


  return 0;
}


int main_test_l3obj(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  system("nm  test_l3obj   | grep l3obj  |fld -e 1 | egrep \"^T.*\" |fld -e 1 > test_l3obj.nm" );
  system("nm -C test_l3obj | grep _l3obj |fld -e 1 | egrep \"^T.*\" |fld -e 1 > test_l3obj.nmC");


  // yes, closures in libffi are available.  

  ffi_cif cif;
  ffi_type *args[1];
  void *values[1];
  const char *s;
  //int rc;

  /* Initialize the argument info vectors */
  args[0] = &ffi_type_pointer;
  values[0] = &s;
  
  /* Initialize the cif */
  assert(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
              &ffi_type_uint, args) == FFI_OK);
  s = "Hello Closure World!";

  // ffi_call(&cif, (genericfunction)puts, &rc, values);  

  printf("FFI_CLOSURES: %d\n",FFI_CLOSURES); // -> 1.
  printf("sizeof(ffi_closure): %ld\n",sizeof(ffi_closure)); // 48

  size_t closure_sz = sizeof(l3obj) + sizeof(ffi_closure);

  void* code = 0; // the executable address within the closure. typically 4K (a page) later than pclos;
                  // code will contain a pointer to the closure, which should be cast to the approp
                  // pointer-to-function and invoked to run the closure that results from
                  // ffi_prep_closure_loc;

  ffi_closure* pclos = (ffi_closure*)ffi_closure_alloc( closure_sz, &code);

  // libffi will give you back a function pointer in code, that should
  //   be cast to the appropriate pointer-to-function type. When invoked,
  //   then FUN will be called with the following arguments.
  //
  //  void (*FUN) (ffi_cif *CIF,   // The `ffi_cif' passed to `ffi_prep_closure_loc'.
  //
  //               void *RET,      // A pointer to the memory used for the function's return value.
  //                               //  FUN must fill this, unless the function is declared as
  //                               //  returning `void'.
  //
  //               void** ARGS,    //  A vector of pointers to memory holding the arguments to the function.
  //
  //               void *USER_DATA // The same USER_DATA that was passed to `ffi_prep_closure_loc'.
  //              );

  char buf[] = "some characters on the stack";
  void* USER_DATA = &buf; // arbitrary data, passed uninterpreted, to the closure function.
  ffi_status prepok = ffi_prep_closure_loc(pclos, &cif, my_closure_func, USER_DATA, code);
  assert(prepok == FFI_OK);

  typedef  int (*puts_func_type)(const char *s);
  puts_func_type myputs = (puts_func_type)code;
  //int mprc = myputs(s);

  myputs("With a different input...");
  myputs("Does this work?");


  // later, to free:
  ffi_closure_free(pclos);

  printf("Entering main()\n");

  //NEW_AUTO_TAG(stacktack,glob);

  l3path varname("test_in_main_test_l3obj");
  l3obj* p5 = make_new_class(0,glob,"main_test_l3obj",varname());
  if (!p5) assert(0);
  //  universal_object_dispatcher(p5,0,0);

  //pb: merlin::call c;
  //my_univ_function(c);
  
  // __cxa_demangle doc:
/*
https://idlebox.net/2008/0901-stacktrace-demangled/cxa_demangle.htt

From the libstdc++ documentation (abi Namespace Reference):
char* abi::__cxa_demangle(const char* mangled_name,
                          char* output_buffer, size_t* length,
                          int* status)

Parameters:

    mangled_name
        A NUL-terminated character string containing the name to be demangled.
    output_buffer
        A region of memory, allocated with malloc, of *length bytes, into which the demangled name is stored. If output_buffer is not long enough, it is expanded using realloc. output_buffer may instead be NULL; in that case, the demangled name is placed in a region of memory allocated with malloc.
    length
        If length is non-NULL, the length of the buffer containing the demangled name is placed in *length.
    status
        *status is set to one of the following values:

            * 0: The demangling operation succeeded.
            * -1: A memory allocation failure occurred.
            * -2: mangled_name is not a valid name under the C++ ABI mangling rules.
            * -3: One of the arguments is invalid.

Returns:
    A pointer to the start of the NUL-terminated demangled name, or NULL if the demangling fails. The caller is responsible for deallocating this memory using free. 
*/


//  const std::type_info&  ti = typeid(tricky);
  const std::type_info&  ti = typeid(l3obj*);
  /*
the mangled type returned by typeid() is: 'FP6_l3objilS_S0_PS0_PS1_E'   with __cxa_demangle() showing: realname: '_l3obj* ()(int, long, _l3obj, _l3obj*, _l3obj**, _l3obj***)'   status:0
  */

  // typeid
  //  const std::type_info&  ti = typeid(my_univ_function); // typeid.name(): 'FvRN6merlin4callEE' 
  //                                                    // with __cxa_demangle() showing: realname: 'void ()(merlin::call&)'
  // 

  //  const std::type_info&  ti = typeid(l3obj*);   // ex: 'P6_l3obj'   realname: '_l3obj*'   status:0

  // you can == and != two typeid() to check for equivalence.
  int status = 0;
  char* realname = abi::__cxa_demangle(ti.name(), 0, 0, &status);
  printf("the mangled type returned by typeid() is: '%s'   with __cxa_demangle() showing: realname: '%s'   status:%d\n",ti.name(), realname, status );
  if (status != -1) { free(realname); }



  google::protobuf::ShutdownProtobufLibrary();
  return 0;

} // end main


#endif


// previsously in l3obj.h   now here:


uint l3_get_ownt(l3obj* myl3obj) {
  assert(myl3obj);
  return get_owntag(0, myl3obj->_reserved);
}

// return the old value, set it to a new value.
uint  l3_set_ownt(l3obj* myl3obj, uint newvalue) {
  assert(myl3obj);
  uint ret = get_owntag(0, myl3obj->_reserved);
  myl3obj->_reserved = set_owntag(0, myl3obj->_reserved, newvalue);
  return ret;
}


// return the old value of the tag, reading it before the clear.
uint l3_clear_ownt(l3obj* myl3obj) {
  assert(myl3obj);
  uint ret = get_owntag(0, myl3obj->_reserved);
  myl3obj->_reserved = set_owntag(0, myl3obj->_reserved, 0);
  return ret;
}



// since dtor's represent our types, write that first.

void* class_l3base_create(void* parent, void* databytes, void* methodbytes);

L3METHOD(class_l3base_destroy)

  l3obj* ptr = (l3obj*)obj;
  long objsize = sizeof(ptr2method) + (char*)(ptr->_pde) - (char*)(&(ptr->_pde));
  long num_methods = 0;
  long datasize =0;
  long methodbytes = 0;

  if (objsize <= (long)(2*sizeof(void*))) {

    datasize = ((char*)(ptr->_pde) - (char*)(&(ptr->_pde))) - sizeof(void*);
    num_methods =0;
    
  } else {

    datasize = ((char*)(ptr->_pde) - (char*)(ptr->_pdb));
    methodbytes = ((char*)(ptr->_pdb) - ((char*)&(ptr->_pdb))) - sizeof(void*);
    num_methods = (long)methodbytes / sizeof(void*);
  }

  printf("class_l3base_destruct() firing : we see objsize of %ld and datasize of %ld, with nummethods=%ld  at addr p=%p\n",
     objsize,datasize,num_methods,obj);

  jfree(ptr); 

L3END(class_l3base_destroy)


// l3base: empty object.
//           the allocator/ctor

void* class_l3base_create(void* parent, void* databytes, void* methodbytes) {

  assert((long)databytes % sizeof(void*) == 0); /* must be aligned */
  assert((long)methodbytes % sizeof(void*) == 0); /* must be aligned */

  long data_plus_method_bytes = (long)databytes + (long)methodbytes;
  long size = 0;
  l3obj* ptr = 0;
  long methodbytes_check = 0;
  long datasize_check = 0;
  long num_methods = 0;

  if (data_plus_method_bytes > 0) {
    size = (sizeof(ptr2method)*3) + data_plus_method_bytes;
  } else {
    size = (sizeof(ptr2method)*2);
  }

  ptr = (l3obj*)malloc(size);

  l3path msg(0,"777777 %p malloc: of size %ld\n",ptr,size);
  printf("%s\n",msg());
  MLOG_ADD(msg());

  ptr->_dtor = &class_l3base_destroy;

  /* we are the 2nd pointer down, so subtract off one sizeof(void*) 
     to account for that */
  ptr->_pde = (void*)((char*)(&(ptr->_pde)) + size - sizeof(void*));

  //In general , if databytes + methodbytes > 0, then _pdb will exist too.
  if (data_plus_method_bytes > 0) {

    // If  _pdb exists, then databytes   = _pde - _pdb; 
    // which means _pdb = _pde - databytes
    ptr->_pdb = (void*)((char*)ptr->_pde - (long)databytes);

    /* more invariants:
    If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb).
    First  method is at &_bdbeg + 1word.
    Second method is at &_pdb + 2words (but here there is no 2nd method).
   */
    datasize_check = ((char*)(ptr->_pde) - (char*)(ptr->_pdb));

    methodbytes_check = ((char*)(ptr->_pdb) - ((char*)&(ptr->_pdb))) - sizeof(void*);
    assert(methodbytes_check == (long)methodbytes);
    num_methods = (long)methodbytes / sizeof(void*);

  } else {
     // no data, no methods, keep it a tight 2 words.
     assert(((char*)(ptr->_pde) - (char*)(&(ptr->_pde))) - sizeof(void*) == 0);
     assert(sizeof(ptr2method) + (char*)(ptr->_pde) - (char*)(&(ptr->_pde)) == size);  
  }

  printf("class_l3base_create() : we see objbytes of %ld, and datasize %ld bytes, %ld methods, for our addr: %p\n",
     size, datasize_check,num_methods,ptr);

  return (void*)ptr;
}




// l3base: empty object.
//           the allocator/ctor

void* new_l3base_create(void* parent, void* databegin, void* methodbytes) {

  long databytes = 0;
  assert((long)databytes % sizeof(void*) == 0); /* must be aligned */
  assert((long)methodbytes % sizeof(void*) == 0); /* must be aligned */

  long data_plus_method_bytes = (long)databytes + (long)methodbytes;
  long size = 0;
  l3obj* ptr = 0;
  long methodbytes_check = 0;
  long datasize_check = 0;
  long num_methods = 0;

  if (data_plus_method_bytes > 0) {
    size = (sizeof(ptr2method)*3) + data_plus_method_bytes;
  } else {
    size = (sizeof(ptr2method)*2);
  }

  ptr = (l3obj*)malloc(size);
  printf("777777 %p malloc: of size %ld\n",ptr,size);
  ptr->_dtor = &class_l3base_destroy;

  /* we are the 2nd pointer down, so subtract off one sizeof(void*) 
     to account for that */
  ptr->_pde = (void*)((char*)(&(ptr->_pde)) + size - sizeof(void*));

  //In general , if databytes + methodbytes > 0, then _pdb will exist too.
  if (data_plus_method_bytes > 0) {

    // If  _pdb exists, then databytes   = _pde - _pdb; 
    // which means _pdb = _pde - databytes
    ptr->_pdb = (void*)((char*)ptr->_pde - (long)databytes);

    /* more invariants:
    If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb).
    First  method is at &_bdbeg + 1word.
    Second method is at &_pdb + 2words (but here there is no 2nd method).
   */
    datasize_check = ((char*)(ptr->_pde) - (char*)(ptr->_pdb));

    methodbytes_check = ((char*)(ptr->_pdb) - ((char*)&(ptr->_pdb))) - sizeof(void*);
    assert(methodbytes_check == (long)methodbytes);
    num_methods = (long)methodbytes / sizeof(void*);

  } else {
     // no data, no methods, keep it a tight 2 words.
     assert(((char*)(ptr->_pde) - (char*)(&(ptr->_pde))) - sizeof(void*) == 0);
     assert(sizeof(ptr2method) + (char*)(ptr->_pde) - (char*)(&(ptr->_pde)) == size);  
  }

  printf("class_l3base_create() : we see objbytes of %ld, and datasize %ld bytes, %ld methods, for our addr: %p\n",
     size, datasize_check,num_methods,ptr);

  return (void*)ptr;
}




void gdump() {
  serialfactory->dump();


  // and do dfs 

  long unseen = glob->dfs_been_here();
  l3obj* ret = 0;
  printf("\n");
  glob->dfs_print(0,unseen,0,0, &ret, glob,0,0,glob,0);

  gUglyDetails++;
  printf("\nmain_env:\n");
  print(main_env,"",0);
  gUglyDetails--;

}

void tags() {
    //  sgt.get_base_tag()->dump("");
}

L3METHOD(lst)
   glob->dump("");
L3END(lst)


#if 0 // still used???

void* class_Tag_destroy(void* obj, void* expandptr, l3obj** retval) {
  static char _type[48] = "class_Tag";
  if (obj == 0) return _type; /* RTTI mechanism */

  Tag* me = (Tag*)obj;
  me->tag_destruct((l3obj*)me,L3NONSTANDARD_OBJONLY);
  return obj;
}
#endif

// static


void Tag::shallow_obj_copy_to(Tag* receiving_tag) {

   long i = 0;
   l3obj* src = 0;
   l3obj* dup = 0;

   /* old:
   osit it = _ownedset.begin();
   osit end = _ownedset.end();


   for(; it != end; ++it, ++i) {
     src = *it;
     dup = 0;
     deep_copy_obj(src,-1,0,0,&dup,receiving_tag,0,0,receiving_tag);

   }

*/

   Word_t * PValue;
   Word_t Index = 0;
   i = 0;

   src = 0;
   dup = 0;
   
   JLF(PValue, _owned, Index);
   while (PValue != NULL)
       {
           src = ((l3obj*)Index);
           dup = 0;
           // why not? because with captag, this quickly goes infinite loop.
           //printf("new way... not doing: deep_copy_obj(src(%p),-1,0,0,&dup(%p),receiving_tag,0,0,receiving_tag(%p))\n",src,&dup,receiving_tag);



           JLN(PValue, _owned, Index);
           ++i;
       }

}


void Tag::copy_ownlist_to_ptrvec(l3obj** pvv) {


   l3obj* vv = *pvv;
   ptrvec_clear(vv);

   /* old
   osit it = _ownedset.begin();
   osit end = _ownedset.end();

   long i = 0;
   for(; it != end; ++it, ++i) {
      ptrvec_set(vv,i,*it);
   }
   */

   // new way: with JudyL instead of set

   Word_t * PValue;
   Word_t Index = 0;
   long i = 0;
   l3obj* o = 0;

   JLF(PValue, _owned, Index);
   while (PValue != NULL)
       {
           o = ((l3obj*)Index);
           ptrvec_set(vv,i,o);
           JLN(PValue, _owned, Index);
           ++i;
       }

}

  // Tag::Tag constructor
  // 
  // the ctor, or 3 arg constructor for Tag (there is no Tag::Tag() so that 
  // *I always know my parent, my name, and where we were declared*). The body lives in l3obj.cpp: circa 2358

// actual Tag::Tag constructor

 // Right now the constructor depends on C++ ctor member initialization, so can't be malloced while
 //  we are using set, map, etc.

Tag::Tag(const char* where_declared, Tag* parent, const char* myname, l3obj* mycap) 
//  : _parse_free_stack(this) 
{
    init(where_declared, parent, myname, mycap);
}


void  Tag::init(const char* where_declared, Tag* parent, const char* myname, l3obj* mycap) {

    Tag* ths = this;
    assert(ths);

    // psexep
    ths->pd_cache      = 0;
    ths->sexp_t_cache = 0;

    ths->_owned = 0;  // JudyL
    ths->_destruct_started = 0;
    ths->_been_here = 0;
    ths->_parent = parent;
    ths->_where_declared = where_declared; 
    ths->_captain = mycap;
    ths->_myname.reinit("%s_uniq%p",myname,ths);

    // in univheader
    ths->_type = t_tag;
    ths->_reserved = 0;
    
    // all tags have their sysbuiltin bit set.
    set_sysbuiltin(ths);

    // tell our parent about us:
    if (ths->_parent) {
        ths->_parent->subtag_push(ths);
    } else {
        // this had better be the global ptag!
        // yes, but about to be assigned, so cant:        assert(ths==glob);
    }

    // register
     Tag* pt = ths;
     ths->_sn_tag = serialfactory->serialnum_tag(pt, ths->_parent, ths->_myname());
     l3path msg(0,"777777 %p new: Tag '%s' @serialnum:%ld\n",pt, ths->_myname(), ths->_sn_tag);
     DV(printf("%s\n",msg()));
     MLOG_ADD(msg());

     // track tags using tyse* system as well.
     ths->_ser = tyse_tracker_add((tyse*) ths);

     // _been_here initial value: get it from the global base tag.
     // Note that we can't just use 0, since we might be on the opposite cycle where 1 indicates unseen.
     // But if this is ser #1 global tag, we do just go ahead and start with 0 (above in : dot list).
     Tag* base = glob;
     if (base) {
        ths->_been_here = base->_been_here;
     }

     // now done automatically by C++ ctor...

#if 0
     // stacks
     ths->genstack.init();
     ths->tbf_stack.init();
     ths->lnk_stack.init();
     ths->sexpstack.init();
     ths->subtag_stack.init();
     ths->del_itag_stack.init();
#endif

}


 // used to be in ctor, but now lazily allocate dir when file actually uses the tag.
void Tag::AllocateMyDir() {


    size_t w = 0;
    string mn(_myname());
    while(1) {
      w = mn.find(":",0);
      if (w == std::string::npos) break;
      mn.replace(w,1,"_");
    }

    // find last space and remove up to there.
    std::string::iterator lastkill = mn.end();
    std::string::iterator beg = mn.begin();
    while (lastkill >= beg && *lastkill != ' ') {
      --lastkill;
    }
    if (lastkill > beg) {
      ++lastkill;
      mn.erase(beg,lastkill);
    }
    
    // make dir
    if (recursive_mkdir(mn.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)) {
      fprintf(stderr,"error in Tag::Tag(): could not make directory _myname:'%s', error is: '%s'\n",mn.c_str(),strerror(errno));
      assert(0);
    }
}

// owner has the tag to trace in a depth-first-search.
// arity has the current "unseen" value, whether 0 or 1.
// a full dfs will flip one to the other. 0 -> 1, then 1 -> 0.
L3METHOD(Tag::dfs_print)

    Tag* visitme = (Tag*)owner;

    // tried to comment this out... got inf loop and ran off the end of the stack. hmm.
    // You would *think* that since Tags are in a Tree without cycles, that 
    // the unseen/seen check would not be necessary.

    long unseen = arity;
    long seen   = 1 - unseen;

    if (visitme->dfs_been_here() == seen) {
        DVV(printf("     dfs_print... ALREADY SEEN Tag(%p ser# %ld) %s...so exiting early at top of dfs_print()\n",visitme, visitme->_sn_tag, visitme->myname()));
       return 0;
    }
    visitme->dfs_been_here_set(seen);

    // emulate...subtag_destroy_all(L3STDARGS);
    
    Tag* childtag = 0;
    for(subtag_stack.it.restart(); !subtag_stack.it.at_end(); ++subtag_stack.it) {
        childtag = *subtag_stack.it;
        childtag->dfs_print(obj,arity,exp,env,retval,childtag,curfo,etyp,retown,ifp);
    }

#if 0
    itsit be = visitme->subtag_stack.begin();
    itsit en = visitme->subtag_stack.end();
    for ( ; be != en; ++be) {
        Tag* childtag = *be;
        (*be)->dfs_print(obj,arity,exp,env,retval,childtag,curfo,etyp,retown,ifp);
    }
#endif

    // emulate... del_all(L3STDARGS);

      l3obj* ptr = 0;
      Word_t * PValue;
      Word_t Index = 0;
      long i = 0;

      JLF(PValue, visitme->_owned, Index);
      while (PValue != NULL)
       {
           ptr = ((l3obj*)(Index));

           Tag* mytag = ptr->_mytag;
           if (mytag) {
               mytag->dfs_print(obj,arity,exp,env,retval,mytag,curfo,etyp,retown,ifp);
           }

           JLN(PValue, visitme->_owned, Index);
           ++i;
       }

    // done visiting all my children, now do my own thing...
printf("     dfs_print... in Tag(%p ser# %ld, _been_here:%ld) %s\n",visitme, visitme->_sn_tag, _been_here, visitme->myname());

L3END(Tag::dfs_print)

//
// owner = visitme
// obj   = indent, as l3obj*
//
// if etyp == 0 then don't print invisible objects
//
L3METHOD(Tag::print_owned_objs_with_canpath)

        Tag* visitme = owner;
        assert(obj);
        l3path* indent = (l3path*)obj;

    assert(visitme == this);
    l3obj* curobj = 0;
    l3obj* prevobj = 0;
    llref* llr = 0;
    l3path canpath;

    while(1) {
       llr = 0;
       prevobj = curobj;
       curobj =  enum_owned_from(prevobj, &llr);
       if (!curobj) break;
       // print curobj, using llr

       if (etyp == 0) {
           if (is_sysbuiltin(curobj)) continue; // skip printing.
           if (is_invisible(curobj))  continue; // skip printing.
       }

       canpath.clear(); // don't carry over from previous printing!
       canon_env_into_objpath((l3obj*)(&canpath),-1,0,  curobj,retval,owner,  curfo,etyp,retown,ifp);
       
       if (indent) { printf("%s",(*indent)()); }
       canpath.outln();


    } // end while(1)


L3END(print_owned_objs_with_canpath)


// do bfs on objects and their tags to get a canonically labelled ls
// exp will have indent char*
//
// if etyp == 0, we don't print invisible or sysbuiltin objects
//
L3METHOD(Tag::bfs_print_obj)

    Tag* visitme = (Tag*)owner;
    l3path indent((char*)exp);
    l3path indent_more((char*)exp);
    indent_more.pushf("     ");

    long unseen = arity;
    long seen   = 1 - unseen;

    if (visitme->dfs_been_here() == seen) {
       DVV(printf("     dfs_print... ALREADY SEEN Tag(%p ser# %ld) %s...so exiting early at top of dfs_print()\n",visitme, visitme->_sn_tag, visitme->myname()));
       return 0;
    }
    visitme->dfs_been_here_set(seen);

    // do my own thing...print myself as top level... print all canonical paths
    // of objects that I own.

    // enumerate all the objects I own...
    print_owned_objs_with_canpath((l3obj*)(&indent), // info here in obj
                              -1,0, env,retval,
                              visitme, // info here in owner
                              curfo,etyp,retown,ifp);

    // hmm.. need to determine if the reference is the canonical path
    // or not. If we rm a canonical path, then rm the actual object too.


    // get at subtags

    Tag* childtag = 0;
    for(subtag_stack.it.restart(); !subtag_stack.it.at_end(); ++subtag_stack.it) {
        childtag = *subtag_stack.it;
        childtag->bfs_print_obj(obj,arity,(sexp_t*)indent_more(),env,retval,childtag,curfo,etyp,retown,ifp);
    }

#if 0
    itsit be = visitme->subtag_stack.begin();
    itsit en = visitme->subtag_stack.end();
    for ( ; be != en; ++be) {
        Tag* childtag = *be;
        (*be)->bfs_print_obj(obj,arity,(sexp_t*)indent_more(),env,retval,childtag,curfo,etyp,retown,ifp);
    }
#endif

      // emulate... del_all(L3STDARGS);

      l3obj* ptr = 0;
      Word_t * PValue;
      Word_t Index = 0;
      long i = 0;

      JLF(PValue, visitme->_owned, Index);
      while (PValue != NULL)
       {
           ptr = ((l3obj*)(Index));

           Tag* mytag = ptr->_mytag;
           if (mytag) {
               mytag->bfs_print_obj(obj,arity,(sexp_t*)indent(),env,retval,mytag,curfo,etyp,retown,ifp);
           }

           JLN(PValue, visitme->_owned, Index);
           ++i;
       }

    // done visiting all my children, now do my own thing...
    DV(printf("     bfs_print_obj... in Tag(%p ser# %ld) %s\n",visitme, visitme->_sn_tag, visitme->myname()));

L3END(Tag::bfs_print_obj)


// owner has the tag to delete after a depth-first-search recursive delete on children Tags.
// arity has the current "unseen" value, whether 0 or 1.
// a full dfs will flip one to the other. 0 -> 1, then 1 -> 0.
L3METHOD(Tag::dfs_destruct)

    Tag* visitme = (Tag*)owner;
    assert(this == visitme);

    // emulate...subtag_destroy_all(L3STDARGS);

    // since dfs_destruct will take things off the list, we gotta grab the next
    // before we delete this one.
    
    long N = visitme->subtag_stack.size();
    ustaq<Tag>* sub = &(visitme->subtag_stack);
    Tag* childtag = 0;        

    if (N) {
        while((childtag = sub->front_val())) {

            // if we get pushed on 2x... hmm... problem.
            if (this == childtag) {
                assert(0);
                sub->pop_front();

            } else {
                LIVET(childtag);
                childtag->dfs_destruct(obj,arity,exp,env,retval,childtag,curfo,etyp,retown,ifp);
                delete childtag;
            }
        }
    }

   // done visiting all my children, now do my own thing...
   DV(printf("     dfs_destruct... in Tag(%p ser# %ld) %s\n",visitme, visitme->_sn_tag, visitme->myname()));

   // kick off the actuall destruct now that we've visited all children first in our depth first search sequence.
   l3obj* fakeret = 0;
   //   visitme->tag_destruct(obj,arity,exp,env,retval,visitme,curfo,etyp,retown,ifp);
   visitme->tag_destruct(obj,arity,exp,env,&fakeret,visitme,curfo,etyp,retown,ifp);

L3END(Tag::dfs_destruct)


// owner has the tag to copy during a depth-first-search recursive copy on children Tags.
// retown will own the new copy.
// where does the new copy get returned? return the captain of the tag in retval.
// where is the captain of this new tag -- lets prefer to generate a captag original pair
L3METHOD(Tag::dfs_copy)

    Tag* copyme = (Tag*)owner;
    l3obj* src = copyme->captain();
    assert(src);

    assert(this == copyme);

// we shouldn't need to mess with dfs been here, since we just proceed through
//   our owned list, and then recurse on subtags. shallow copy the objects on our
//   owned list. since the tags are in a tree, there is no inf loop.

// in fact, use arity instead to count recursive copy depth, so we know if we should
//  give our parent object an automatic alias for ourselves.

    // generate the new tag as a captag
    DV(printf("     dfs_copy... in Tag(%p ser# %ld) %s\n",copyme, copyme->_sn_tag, copyme->myname()));

    l3path objnm(0,"dfs_copy_of_%s",src->_varname);
    l3path classnm;

    if (src->_type == t_obj) {
        label* on = 0;
        label* cn = 0;
        obj_get_obj_class(src, &on, &cn);
        objnm.reinit("dfs_copy_of_%s",*on);
        classnm.reinit("%s",*cn);
    }
    l3obj* new_cap = 0;
    make_new_captag((l3obj*)&objnm ,src->_malloc_size - sizeof(l3obj),exp,env,(l3obj**) &new_cap, retown, (l3obj*)&classnm, src->_type, retown,ifp);
    assert(new_cap);
    assert(new_cap->_mytag);
    Tag*   new_tag = (Tag*)(new_cap->_mytag);
    assert(new_tag);
    assert(new_tag->captain() == new_cap);

    *retval = new_cap;

      // iterate over objects owned
      l3obj* ptr = 0;
      Word_t * PValue;
      Word_t Index = 0;
      long i = 0;

      JLF(PValue, copyme->_owned, Index);
      l3obj* new_obj_copied = 0;
      llref* llr = 0;
      llref* llr_priority = 0;
      char* name = 0;
      l3obj* labelme = 0;
      while (PValue != NULL) {

           ptr = ((l3obj*)(Index)); // key
           assert(ptr);
           LIVEO(ptr); assert(ptr->_type);

           llr = *(llref**)PValue;  // value
           LIVEREF(llr); assert(llr);

           // get the name from the src, using the priority ref
           llr_priority = priority_ref(llr);
           name = llr_priority->_key;
     
           // copy captain-setting
           if (copyme->captain() == ptr) {
               deep_copy_obj(ptr, arity+1, exp, env, &new_cap, new_tag, ptr, ptr->_type, new_tag,ifp);
               labelme = new_cap;

               // where do we put the new alias?  not sure. try in the new_cap... captain seems a good choice...hmm.
               // conjecture: subtag->captain should be known in subtag->parent->captain.  try that:
               if (arity > 0) {
                   add_alias_eno(new_tag->parent()->captain(), name, labelme);
               }

           } else {
               new_obj_copied = 0; // indicate that new allocation is needed
               deep_copy_obj(ptr, arity+1, exp, env, &new_obj_copied, new_tag, ptr, ptr->_type, new_tag,ifp);
               assert(new_obj_copied);
               assert(new_obj_copied->_owner == new_tag);
               labelme = new_obj_copied;

               // where do we put the new alias?  not sure. try in the new_cap... captain seems a good choice...hmm.
               add_alias_eno(new_cap , name, labelme);
           }

           JLN(PValue, copyme->_owned, Index);
           ++i;
       }

    
    // iterate over subtags...
    long N = copyme->subtag_stack.size();
    l3obj* new_child_cap = 0;
    if (N) {

        Tag* childtag_src = 0;
        for(subtag_stack.it.restart(); !subtag_stack.it.at_end(); ++subtag_stack.it) {

            childtag_src = *subtag_stack.it;
            assert(childtag_src);
            new_child_cap = 0;
            
            // special case exclude main_env (cycle at the top of the env)
            if (childtag_src && childtag_src->captain() != main_env) {
                childtag_src->dfs_copy(obj, arity+1, exp, env, &new_child_cap, childtag_src, curfo, etyp, new_tag,ifp);
                assert(new_child_cap);
                assert(new_child_cap->_mytag->parent() == new_tag);

            } else {
                // when are we getting this?
                printf("break here!\n");
                childtag_src->dfs_copy(obj, arity+1, exp, env, &new_child_cap, childtag_src, curfo, etyp, new_tag,ifp);
            }
        }

#if 0

        itsit cur = copyme->subtag_stack.begin();
        itsit nex = cur;

        for (i =0; i < N; i++) {
        
            nex = cur;
            ++nex;

            Tag* childtag_src = (Tag*)(*cur);
            new_child_cap = 0;
            
            // special case exclude main_env (cycle at the top of the env)
            if (childtag_src && childtag_src->captain() != main_env) {

                childtag_src->dfs_copy(obj, arity+1, exp, env, &new_child_cap, childtag_src, curfo, etyp, new_tag);
                assert(new_child_cap);
                assert(new_child_cap->_mytag->parent() == new_tag);
            } else {
                // when are we getting this?
                printf("break here!\n");
                childtag_src->dfs_copy(obj, arity+1, exp, env, &new_child_cap, childtag_src, curfo, etyp, new_tag);
            }

            cur = nex;
        }

#endif

    } // end if (N)


L3END(Tag::dfs_copy)


L3METHOD(Tag::shallow_tag_copy)
L3END(Tag::shallow_tag_copy)



//
// a captag obj is defined by: obj->_mytag->captain() == obj; where the argument tag is obj->_mytag just mentioned in obj->_mytag->captain()
//  Q: does a captag have to possess the property that:      obj->_owner == obj->_mytag? Answer: NO.
int  pred_is_captag(Tag* tag, l3obj* cp) {
   assert(cp);

   if (0==tag) return 0; // can't be a captag if there is no tag.

   if (   cp->_mytag == tag 
          // we cannot assume that _mytag and _owner are the same for any given captag. i.e. cannot do:
          //       && cp->_owner == cp->_mytag  // can't say this, b/c frequently false for real captags.
       && tag->captain() == cp) 
       { 
         return 1;
       }

   assert(cp->_mytag != tag); // did we change somebodies logic above?

   return 0;
}


// common shorthand
BOOL pred_is_captag_obj(l3obj* o) {
    return pred_is_captag(o->_mytag, o);
}


void check_tag_and_break_captag_selfcycle(Tag* tag) {
   // protocol for avoiding loops in cleanup 
   //  do Depth First Search, then zero the captain->mytag before doing any actual cleanup

   l3obj* cp = tag->captain();
   assert(cp); // all tags should have captains.
   
   // we inifinite loop without this zero-ing of captain->_mytag first.
   // pred_is_captag got a little too stringent for us.
   //   if (pred_is_captag(tag,cp)) {
   if (cp->_mytag == tag) {
          cp->_mytag = 0; // let the owner of _captain->_mytag clean up that tag.
  }
}


void check_obj_and_break_captag_selfcycle(l3obj* obj) {
  
   Tag* tag = obj->_mytag;
   if (!tag) return;

   check_tag_and_break_captag_selfcycle(tag);
}


L3METHOD(Tag::tag_destruct)

   check_tag_and_break_captag_selfcycle(this);

    // avoid duplicate frees, get us out of our parent's _owned set
      Tag* par = parent();
      if (par && par != this) {
        // have to ask the owning tag to delete, so it knows to not do so later as well upon cleanup.
        par->subtag_remove(this);
      }


    // Checking if forwarded?
    // That makes no sense. Because only objects have forwarded tags.
    // tags themselves simply own their objects, and delete them upon destruct.

    // do this first, so we can jfree() the subtag pointer too, since
    // that will probably get tbf_jfree_push()-ed as well.
    subtag_destroy_all(L3STDARGS);
    assert(subtag_stack.size()==0);

   // old:   sexpstack_destroy_all(); // put this before the tdop goes in del_all, so our diagnostics have a chance.
    del_all(L3STDARGS);

    sexpstack_destroy_all(); // put this *after* del_all so that function_defn dtor can remove their defn_qtree before we do.

    gen_free_all();
    tbf_jfree_all(L3STDARGS);
    lnk_jfree_all(L3STDARGS);

    llr_free_all(YES_DO_HASH_DELETE);
    newdel_destroy_all();
    tyse_free_all();
    atom_free_all();

    // don't free outselves because we may be stack allocated!
    
    // If this tag is owned by an object such as a closure that
    //   was long ago finished with actual execution, then there's no
    //   no way this tag can be atop the stack now.
    //  so comment these out:
    //    defptag_pop_iftop(this);

    //    if (sgt.erase_if_on_global_tag_stack(this)) {
    //      printf("ARG! Problem: destruct() called on tag that was not on the top of the global sgt stack, but *was* on the stack! %p\n",this);
    //      assert(0);
    //    }
    //    mini.rm_from_alltags(this); // clean up our filesystem directory.
    // how to cleanup the symlink from the call stack area?

    // tell serialfactory that we are dead.
    Tag* pt = this;
    l3path name_freed;
    long sn_del = serialfactory->del(pt,&name_freed);

    l3path msg(0,"888888 %p delete '%s' @serialnum:%ld\n",pt,name_freed(),sn_del);
    DV(printf("%s\n",msg()));
    MLOG_ADD(msg());

     // stacks
     genstack.destruct();
     tbf_stack.destruct();
     tyse_stack.destruct();
//     _parse_free_stack.destruct();
     lnk_stack.destruct();
     sexpstack.destruct();
     atomstack.destruct();
     subtag_stack.destruct();
     del_itag_stack.destruct();
     llrstack.destruct();

L3END(Tag::tag_destruct)



void insert_into_hashtable_private_to_add_alias_typed(l3obj* o, char* key, llref* addme)
{
    // don't bother with sealed references.
    if (is_sealed(o) && !global_terp_final_teardown_started) {
        printf("error in insert_into_hastable: object %p is sealed: aborting on request to insert key '%s'.\n",o,key);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    o->_judyS->insertkey(key,addme);
}

#if 0
// true => inserted successfully
// false => not inserted b/c was duplicate; previous value is stored in prev_value.
bool insert_into_hashtable_unless_already_present(l3obj* o, char* key, unsigned long value, Word_t& prev_value)
{
    // don't bother with sealed references.
    if (is_sealed(o) && !global_terp_final_teardown_started) {
        printf("error in insert_into_hastable_unless_already_present: object %p is sealed: aborting on request to insert key '%s'.\n",o,key);
        l3throw(XABORT_TO_TOPLEVEL);
    }

  //  Pvoid_t   PJArray = o->_judy; // (PWord_t)NULL;  // Judy array.
  PWord_t   PValue = 0;                   // Judy array element.
  uint8_t      Index[BUFSIZ];            // string to sort.
  strncpy((char*)Index,key,BUFSIZ);
   
  JSLI(PValue, o->_judyS, Index);   // store string into array
  if (*PValue) {
    prev_value = *PValue;
    return false;
  } else {
    (*PValue) = value;
    return true;
  }
  return false;
}
#endif


// true => inserted successfully
// false => not inserted b/c was duplicate; previous value is stored in prev_value.
bool insert_into_hashtable_unless_already_present_typed(l3obj* o, char* key, llref* value, llref** prev_value)
{
    // don't bother with sealed references.
    if (is_sealed(o) && !global_terp_final_teardown_started) {
        printf("error in insert_into_hastable_unless_already_present: object %p is sealed: aborting on request to insert key '%s'.\n",o,key);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    if (prev_value) {
        *prev_value = o->_judyS->lookupkey(key);

        if (*prev_value) {
            return false;
            //deletekey(key);
        }
    }

    o->_judyS->insertkey(key,value);
  return true;
}




void dump_double(l3obj* ele, char* indent, int count, uint8_t Index[BUFSIZ], char* postscript, stopset* stoppers) {
    assert(ele);
    char* trailer = (char*)"";
    double dval =0;
    l3path indent_more(indent);
    indent_more.pushf("%s","    ");
    l3path details;
    l3path ici;
    if (gUglyDetails > 0) {
        details.pushf("(%p ser# %ld %s)", ele, ele->_ser, ele->_type);
        ici.pushf("%s%02d:%s",indent,count,Index);
    } else {
        ici.pushf("%s%s",indent,Index);
    }

    if (postscript != 0) {
        trailer = postscript;
    }
    long sz = double_size(ele);
    if (sz <= 1) {
        if (sz) { 
            dval = double_get(ele,0); 
            printf("%s ->  %.6f   %s %s\n", ici(), dval, details(), trailer);
        } else {
            printf("%s ->  (empty double obj) %s %s\n", ici(), details(), trailer);
        }
    } else {
        printf("%s -> double vector of size %ld %s %s\n", ici(), sz, details(),trailer);
        print(ele,indent_more(),stoppers);
    }
}

 
// returns num elements found
size_t dump_hash(l3obj* o, char* indent, stopset* stoppers) 
{
    FILE* ifp = 0;
    size_t      sz =0;

    PWord_t   PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, o->_judyS->_judyS, Index);       // get first string
    char buf[BUFSIZ];
    int count = 0;

    l3path details;
    l3path ici; // indent count Index

    l3path indent_more(indent);
    indent_more.pushf("        ");

    bool isstop = false;
    t_typ ty = 0;
    while (PValue != NULL)
    {
      details.clear();
      ici.clear();

      bzero(buf,BUFSIZ);
      llref* llre = (llref*)(*PValue);
      LIVEREF(llre);

      l3obj* ele  = llre->_obj;

      if (stoppers) {
          isstop = obj_in_stopset(stoppers,ele);
      }

      // allow gVerboseFunction == 3 to show all builtin vars.
      if (is_sysbuiltin(llre)) { //  && gVerboseFunction < 3) {
          goto NEXT_JUDYS_STRING;
      }

      if (ele) {
          ty = ele->_type;
          if (gUglyDetails > 0) {
              details.pushf("(%p ser# %ld %s)", ele, ele->_ser, ele->_type);
              ici.pushf("%s%02d:%s",indent, count, Index);
          } else {
              ici.pushf("%s%s",indent, Index);
          }
      }

      if (ele && !isstop) {
          LIVEO(ele);

          // handle user-defined, table driven extensible types
          quicktype* qt = 0;
          t_typ known_type = qtypesys->which_type(ty, &qt);
          if (!known_type) {
              printf("error in print: unknown type '%s' in object %p ser# %ld.\n", ty, ele, ele->_ser);
              l3throw(XABORT_TO_TOPLEVEL);
          }
          
          if (qt && qt->_print) {
              
              // user defined type system... invoke the defined print method.
              printf("%s -> %s:\n", ici(), details());
              qt->_print(ele, (*(long*)stoppers),(sexp_t*)indent_more(),  0,0,0,  0,0,0,ifp);

          }
          else
          if (ty == t_dou) {
              dump_double(ele, indent, count, Index, (char*)"",stoppers);
          }
          else 
              if (ty == t_str) {

              l3path eles;
              long str_sz = string_size(ele);
              if (str_sz < 2) {
                  string_get(ele,0,&eles);
                  printf("%s -> %s \"%s\"\n", ici(), details(), eles());
              } else {
                  printf("%s -> string vec %s of size %ld:\n", 
                         ici(), details(), str_sz);
                  string_print(ele, indent_more());
              }
              
          }
      else
      if (ty == t_lit) {
          l3path eles;
          literal_get(ele,&eles);
          printf("%s -> literal '%s'\n", ici(), eles());
      }
      else
        if (ty == t_fun ) {
            printf("%s -> %s ", ici(), details());
            print(ele,indent,stoppers);
        }
        else
          if (ty == t_obj || ty == t_sxp) {
              printf("%s -> %s ", ici(), details());
              print(ele,indent,stoppers);      
          }
          else
            if (ty == t_vvc) {
                printf("%s -> %s ", ici(), details());
                print(ele,indent_more(),stoppers);
            }
          else
        if (ty == t_tru) {
            printf("%s -> T\n", ici());
        }
        else
          if (ty == t_fal || ty == t_nil) {
              printf("%s -> F\n", ici());
          }
          else
            if (ty == t_flh) {
                printf("%s -> %s FILEHANDLE\n", ici(), details());
          }
          else
            if (ty == t_syv) {
                printf("%s -> %s SYMVEC\n", ici(), details());
              print(ele,indent_more(),stoppers);
          }
          else
            if (ty == t_map) {
                printf("%s -> %s MAP\n", ici(), details());
                print(ele,indent_more(),stoppers);
          }
          else
            if (ty == t_nav) {
                printf("%s -> na\n", ici() );
          }
          else
            if (ty == t_nan) {
                printf("%s -> nan\n", ici() );
          }
          else
            if (ty == t_dsq) {
                printf("%s -> ", ici() );
                dq_print(ele, indent, 0);
          }
            else
            if (ty == t_mqq) {
                printf("%s -> ", ici() );
                mq_print(ele, indent, 0);
          }
            else
            if (ty == t_ddt) {
                printf("%s -> ", ici() );
                dd_print(ele, indent, 0);
          }
          else
            if (ty == t_lin) {
                linkobj* lin    = (linkobj*)(ele->_pdb);
                l3obj* link_target = lin->_lnk->chase();
                l3path lin_details;
                if (gUglyDetails > 0) {
                    lin_details.pushf("(%p ser# %ld %s)", link_target, link_target->_ser, link_target->_type);
                }

                printf("%s -=-> %s ", ici(), lin_details());
                lnk_print(lin->_lnk, indent_more(),stoppers);
          }
          else
            if (ty == t_tdo) {
                printf("%s -> tdop_parser %s ", ici(), details());
                print(ele,indent_more(),stoppers);
          }

          else {
            printf("internal error: likely use of already freed object: unregognized type in obj %p.\n",ele);
            assert(0);
            printf("%s -> (addr %p) of type %s\n", ici(), (void*)*PValue, ty);
            //    assert(0);
          }
      } // end if (ele && !isstop)
      else if (0==ele) {
         printf("%s -> (null)\n", ici());
      } else {
          assert(ele);
          assert(ty);
          if (ty==t_dou) {
              dump_double(ele, indent, count, Index,(char*)" [stopset shallow]",stoppers);
          } else {
              printf("%s -> %s %s   [stopset shallow]\n",ici(), details(), ele->_varname );
          }
      }

    NEXT_JUDYS_STRING:

      JSLN(PValue, o->_judyS->_judyS, Index);   // get next string
      //      if (!isstop) {
        ++sz;
        ++count;
        //      }
    }

    return sz;
}


void clear_hashtable(l3obj* o) {
    // can't delete sealed obj
    if (is_sealed(o) && !global_terp_final_teardown_started) {
        printf("warning in clear_hastable: object %p is sealed: ignoring request to clear_hastable(%p).\n",o,o);
        return;
    }

    //   long    Bytes = 0;                    // size of JudySL array.
    //   JSLFA(Bytes, o->_judyS);

    o->_judyS->clear();
}


void* lookup_hashtable(l3obj* o, char* key)
{
#if 0
    PWord_t   PValue  = 0;                    // Judy array element.
    LIVEO(o);
    JSLG(PValue, o->_judyS, (uint8_t*)key);       // get
    if (PValue)  return (void*)(*PValue);
    return 0;
#endif

    if (!o || 0 == o->_judyS) return 0;
    LIVEO(o);
    return o->_judyS->lookupkey(key);
}


void delete_key_from_hashtable(l3obj* obj, char* key, llref* llr_expect)
{
    LIVEO(obj);

    //    if (key && *key == '\0') {
    //        assert(0);
    //    } else {
        DV(printf("delete_key_from_hashtable: '%s' in obj %p ser# %ld   with llr_expect: %p _llsn:%ld\n", 
                  key, obj, obj->_ser, llr_expect, llr_expect->_llsn));
        //    }

    // don't bother with sealed references.
    if (is_sealed(obj) && !global_terp_final_teardown_started) {
        printf("warning in delete_key_from_hastable: object %p is sealed: ignoring request to delete key '%s'.\n",obj,key);
        return;
    }

  llref* foundref = (llref*)lookup_hashtable(obj,key);
  if (!foundref) {
      printf("warning in delete_key_from_hashtable: key '%s' not present in hashtable: (%p ser# %ld) %s \n",key,obj,obj->_ser,obj->_varname);
      assert(0);
      return; // done already, since key not present.
  }
  assert(foundref == llr_expect);

#if 0
  int Rc=0;
  JSLD(Rc,obj->_judyS, (uint8_t*)key);       // delete string
  assert(Rc != JERR); 
  assert(Rc == 1); // should have been found.
#endif
  
  obj->_judyS->deletekey(key);
}

// return 0 or 1, depending on if some string in
//  the hash table o maps to the target.
int hashtable_has_pointer_to(l3obj* o, l3obj* target) {
    assert(o);
    assert(target);
    LIVEO(o);

    PWord_t   PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, o->_judyS->_judyS, Index);       // get first string
    //char buf[BUFSIZ];
 
    while(PValue) {
      llref* ele = (llref*)(*PValue);
      if (!ele) break;
      if (target == ele->_obj) return 1;
      JSLN(PValue, o->_judyS->_judyS, Index);   // get next string
    }

    return 0;
}

size_t hash_show_keys(l3obj* o, const char* indent) 
{
    LIVEO(o);
    size_t       sz = 0;
    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, o->_judyS->_judyS, Index);       // get first string

    l3path indent_more(indent);
    indent_more.pushf("%s","    ");

    while (PValue != NULL)
    {
      llref* ele = *((llref**)(PValue));
      printf("%s%02ld:%s -> llref(%p) -> _obj(%p)\n", indent, sz, Index,ele, ele->_obj);
      JSLN(PValue, o->_judyS->_judyS, Index);   // get next string
      ++sz;
    }

    return sz;
}



l3obj* print(l3obj* obj, const char* indent, stopset* stoppers) {
  if (!obj) return 0;
  LIVEO(obj);
  FILE* ifp = 0;

  if (stoppers == 0) { 
    stopset_clear(global_stopset);
    stoppers = global_stopset;
    stopset_push(stoppers,obj);
  } else {
      if (obj_in_stopset(stoppers, obj)) {
      printf("%s[stoppset preventing going deeper on obj: %p]\n",indent,obj);
      return 0;
    }
      stopset_push(stoppers,obj); // only print it once.
  }

  t_typ ty = obj->_type;
  l3path ind3(indent);
  ind3.pushf("%s      ",indent);

  l3path ind4(ind3);
  ind4.pushf("      ");

  // handle user defined types
  quicktype* qt = 0;
  t_typ known_type = qtypesys->which_type(ty, &qt);
  if (!known_type) {
      printf("error in print: unknown type '%s' in object %p ser# %ld.\n", ty, obj, obj->_ser);
      l3throw(XABORT_TO_TOPLEVEL);
  }

  l3path addr;
  l3path details;
  if (gUglyDetails > 0) {
      addr.pushf("%p", obj);
      details.pushf("(%p ser# %ld %s)", obj, obj->_ser, obj->_type);
  }


  if (obj == gnil || ty == t_fal) {
      printf("%s%s F\n", indent, addr());
    return obj;
  } else if (obj == gtrue || ty == t_tru) {
    printf("%s%s T\n", indent, addr());
    return obj;
  } else if (obj == gna) {
    printf("%s%s na\n", indent, addr());
    return obj;
  } else if (obj == gnan) {
    printf("%s%s nan\n", indent, addr());
    return obj;
  }

   if (ty == t_env) {
       dump_hash(obj,(char*)indent,stoppers);
   }
   else
   if (ty == t_dou) {
     double_print(obj,indent);
   }
   else
     if (ty == t_vvc) {
       ptrvec_print(obj,indent,stoppers);
     }
   else if (ty == t_fun) {

       //too busy ls output?
       print_defun_body(obj,indent,stoppers);

       //print_defun_short(obj,indent,stoppers);

     DVV( { printf("\n\n And full defun details:\n");
             print_defun_full(obj,ind3(),stoppers); });
   }
   else if (ty == t_cal) {
     print_cal(obj,indent,stoppers);
   }
   else if (ty == t_obj) {
     print_obj(obj,indent,stoppers);
   }
   else if (ty == t_syv) {
     symvec_print(obj,indent,stoppers);
   }
   else if (ty == t_map) {
     l3map_print(obj,indent,stoppers);
   }
   else if (ty == t_dsq) {
     dq_print(obj,indent,stoppers);
   }
   else if (ty == t_ddt) {
     dd_print(obj,indent,stoppers);
   }
   else if (ty == t_sxp) {
     sexpobj_print(obj,indent,stoppers);
   }
   else if (ty == t_tdo) {
     tdo_print(obj,indent,stoppers);
   }
   else
     if (ty == t_str) {
       l3path s;
       long str_sz = string_size(obj);
       if (str_sz < 2) {
           string_get(obj,0,&s);
           printf("%s%s \"%s\"\n", indent, details(), s());
       } else {
           printf("%s%s string vector of size %ld:\n", 
                  indent, details(), str_sz);
           string_print(obj, ind3());
       }
     }
     else
     if (ty == t_lit) {
       l3path s;
       literal_get(obj,&s);
       printf("%s%s literal '%s'\n", indent, details(), s());
     }
     else
     if (ty == t_lin) {
         printf("%s%s --> ", indent, details());
         linkobj* lin    = (linkobj*)(obj->_pdb);                
         lnk_print(lin->_lnk, indent,stoppers);
     }
     else 
         if (qt && qt->_print) {
         
             // user defined type system... invoke the defined print method.
             printf("%s%s:\n", indent,details());
             qt->_print(obj, (*(long*)(&stoppers)),(sexp_t*)ind4(),  0,0,0,  0,0,0,ifp);
                          
         } else {
             printf("%s%s: object of type %s'\n", indent, details(), ty);
         }

   if (obj->parent() != gnil) {

       printf("%s  _parent -> ",indent); 
       lnk_print(obj->_parent,"",stoppers);
   }
   if (obj->child() != gnil)  { 

       printf("%s  _child  -> ", indent);
       lnk_print(obj->_child,"",stoppers);
   }
   if (obj->sib() != gnil)    { 

       printf("%s  _sib    -> ", indent);
       lnk_print(obj->_sib,"",stoppers);
   }

   return obj;
}


// convenience method for syncing logs from gdb
void s() {
    MLOG_ONLY(    mlog->jmsync(); ) // in s(), only ever called from debugger.
    histlog->sync(); // in s(), only ever called from debugger.
}


// convenience method for printing an object from gdb
l3obj* p(l3obj* obj) {
    s();
    l3obj* tmp = print(obj,"",0);
    if (obj->_judyS) {
        obj->_judyS->dump("_judyS:    ");
    }
    return tmp;
}

Tag* p(Tag* t) {
    s();
  Tag* ptr = ((Tag*)t);
  ptr->dump("   ");
  return ptr;
}

llref* p(llref* r) {
    s();
    print_llref(r,(char*)"   ");
    return r;
}

sexp_t* p(sexp_t* s) {
    ::s();
    l3path expstring(s);
    expstring.outln();
    return s;
}

objlist_t* p(objlist_t* penvpath) {
    s();
    print_objlist(penvpath);
    return penvpath;
}

//////////// defun stuff


/*
struct defun {
  sexp_t* funcname;
  sexp_t* propset;
  sexp_t* body;

  int nprops; // ret_argype.size() + arg_argtyp.size()
  int narg;
  int nret;
  vector<string>    arg_name;
  vector<owntag_en> arg_owntag;
  vector<t_typ>     arg_argtyp;

  vector<t_typ>     ret_argtyp;

  long    srclen;
  char    src[8];
};
*/

//
// enDefunDumpOptions should correspond to gVerboseFunction levels exactly...
//
typedef enum {PRINT_DEFUN_NOBODY=0, PRINT_DEFUN_ONELINE=1, PRETTY_PRINT_DEFUN=2, PRETTYPRINT_ALL=3} enDefunDumpOptions;

void defun_dump(defun& d, const char* indent, stopset* stoppers, enDefunDumpOptions print_options) {
  char propstr[PATH_MAX+1];
  char* p = &propstr[0];
  l3path baseind(0,"%s",indent);
  l3path indmore(0,"%s      ",indent);

  // gone to anonymous functions:
  p += sprintf(p,"%s(de ",baseind());
  p += sprintf(p,"(prop ");

  int nret = d.ret_argtyp.size(); 
  char* abbrev = 0;

  d.ret_name.it.set_staq(&d.ret_name);
  for (int i = 0; i < nret; ++i, ++d.ret_name.it) {
      abbrev = owntagtype2abbrev(d.ret_owntag[i]);
      p += sprintf(p,"(return %s %s",
                   *(d.ret_name.it), abbrev ? abbrev : d.ret_owntag[i]);
      for(d.ret_argtyp[i]->it.set_staq(d.ret_argtyp[i]); !d.ret_argtyp[i]->it.at_end(); ++(d.ret_argtyp[i]->it)) {
          p += sprintf(p," %s",*(d.ret_argtyp[i]->it));
      }
      p += sprintf(p,") ");
  }

  int narg = d.arg_argtyp.size();

  d.arg_name.it.set_staq(&d.arg_name);
  for (int i = 0; i < narg; ++i, ++d.arg_name.it) {
      abbrev = owntagtype2abbrev(d.arg_owntag[i]);

      p += sprintf(p,"(arg %s %s", 
                   *(d.arg_name.it), abbrev ? abbrev : d.arg_owntag[i]);
      
      for(d.arg_argtyp[i]->it.set_staq(d.arg_argtyp[i]); !d.arg_argtyp[i]->it.at_end(); ++(d.arg_argtyp[i]->it)) {
          p += sprintf(p," %s",*(d.arg_argtyp[i]->it));
      }
      p += sprintf(p,") ");
  }

  p += sprintf(p,") ");

  l3path pretty(propstr);

  if (print_options == PRINT_DEFUN_NOBODY) {
      // show nothing more

  } else if (print_options == PRINT_DEFUN_ONELINE) {
      // printf("%s %s)\n",indent, propstr);
      printf("%s)\n", propstr);

  }
  else if (print_options == PRETTY_PRINT_DEFUN || print_options == PRETTYPRINT_ALL) {
      printf("\n");

      assert(d.body);
      if (d.propset) {
            d.propset->printspan(0,indmore());
      }
      d.body->printspan(0,indmore());
      printf("\n");
  }

}

t_typ str2owntag(const qqchar& s) {
    if (0==s.strcmp("!>") || 0==s.strcmp(t_ico)) return t_ico; // in_c_own;
    if (0==s.strcmp("@>") || 0==s.strcmp(t_iso)) return t_iso; // in_s_own;
    if (0==s.strcmp("@<") || 0==s.strcmp(t_oco)) return t_oco; // out_c_own;
    if (0==s.strcmp("!<") || 0==s.strcmp(t_oso)) return t_oso; // out_s_own;

    return 0;
}


char* owntagtype2abbrev(t_typ s) {

    if (0==strcmp(s,"!>") || 0==strcmp(s,t_ico)) return global_ico; // in_c_own;
    if (0==strcmp(s,"@>") || 0==strcmp(s,t_iso)) return global_iso; // in_s_own;
    if (0==strcmp(s,"@<") || 0==strcmp(s,t_oco)) return global_oco; // out_c_own;
    if (0==strcmp(s,"!<") || 0==strcmp(s,t_oso)) return global_oso; // out_s_own;

    return 0;
}


// require (arg    name1 tag1 type1 {t_typ1}+) 
//..           .... (arg name2 tag2 {t_typ2}+  )
// and require (return nameN tagN {t_typN}+) on the  return specifications.
//
bool handle_next_prop(defun& d, sexp_t* prop, l3obj* env, bool& is_ret) {

  is_ret = false;
  assert(is_list(prop));

  l3path propstring(prop);
  qqchar v(prop->headval());

  if (0 !=v.strcmp("arg") && 0 != v.strcmp("return")) {
      std::cout << "error in handle_next_prop: unrecognized propset component: '" << v << "'. Currently we recognize 'arg' and 'return'.\n";
      l3throw(XABORT_TO_TOPLEVEL);
      assert(0);
      return false;
  }

  if (0==v.strcmp("arg")) {
    is_ret = false;
  } else {
    is_ret = true;
  }

  long n = num_children(prop);
  if (n < 3) {
      l3path badprop(prop);
      printf("error: incorrect argument count in propset. An argument spec (arg ...) needs at least three arguments. Ex: \n"
             "(prop  (return res @< t_dou)  (arg varname1 !> t_dou)  (arg varname2 !> t_str))   ...bad propset was: '%s'\n",
             badprop());
    l3throw(XABORT_TO_TOPLEVEL);
    //    return false;
  }

  qqchar name(ith_child(prop,0)->val());
  qqchar tt(ith_child(prop,1)->val());
  t_typ transfertype = str2owntag(tt);

  if (transfertype == 0) {
      std::cout << "error in handle_next_prop: bad owntag spec '" << prop->val() << "'. Currently we recognize '!>', '!<', '@>', and '@<' as the tag ownership specification for an arg or return declaration.\n";
      l3throw(XABORT_TO_TOPLEVEL);
  }

  // read in multiple type properties now
  ustaq<char>* typchain = new ustaq<char>;
  typchain->_type = t_ust;
  tyse_tracker_add((tyse*)typchain);

  for (int j = 2; j < n; ++j ) {
      qqchar type1(ith_child(prop,j)->val());

      t_typ ty1 = qtypesys->which_type(type1,0);
      if (!ty1) {
          std::cout << "bad type specifier '"<< type1 << "' in function property '" << propstring() << "'.\n";
          l3throw(XABORT_TO_TOPLEVEL);
      }
      typchain->push_back(ty1);
  }

  if (0==v.strcmp("arg")) {
      
      d.arg_argtyp.push_back(typchain); // takes a whole ustaq<char> 
      d.arg_owntag.push_back(transfertype);
      d.arg_name.push_back(name.str_dup());
#if 0
      name.dup();
      d.arg_name.push_back(name.b);
      name.release();
#endif

  } else if (0==v.strcmp("return")) {

    if (transfertype == t_oso || transfertype == t_oco) {
      // okay, no other check.
    } else {
        printf("error: bad return tag spec in '%s': only out_s_own (!<) and out_c_own (@<) are allowed on return specs. Ex: (de incr1 (prop (return ret @< t_dou) (arg x !> t_dou)) (setq ret (+ x 1)))\n",propstring());
      l3throw(XABORT_TO_TOPLEVEL);
    }

    d.ret_argtyp.push_back(typchain);

#if 0
    name.dup();
    d.ret_name.push_back(name.b);
    name.release();
#endif

    d.ret_name.push_back(name.str_dup());
    d.ret_owntag.push_back(transfertype);

  } else {
      std::cout << "error: unrecognized prop type '" << v << "' in "; printf("property '%s'\n",propstring());
      l3throw(XABORT_TO_TOPLEVEL);
  }
  
  return true;
}


// return true if okay, false if bad declaration
bool parse_defun_propset(defun& d, l3obj* env) {
    LIVEO(env);
  // use a set to detect dupliate paramater names 
  // since the return and formal arg names must all be distinct.
  std::set<std::string> uniqparm;

  sexp_t* propset = d.propset;
  sexp_t* nex = propset->first_child();
  qqchar  nv(propset->val());

#if 0 // old stuff
  if(0 != nv.strcmp("prop")) {
      
    // if we don't see prop, then assume traditional listp style with all types as doubles
    // and all input arguments as tag !>   and a single return value of type double with tag @<.

    printf("error: function definition propset list must start with (prop ...)\n");
    return false;

  }
#endif

  int nret = 0;
  int narg = 0;

  long nc = propset->nchild();
  for (int i = 0; i < nc; ++i) {
      nex = propset->ith_child(i);

      if (nex->_ty == t_comma) continue;

      bool is_ret; 
      if (!handle_next_prop(d,nex,env,is_ret)) {
          printf("error: could not handle_next_prop\n");
          assert(0);
      }
      if (is_ret) { nret++; } else { narg++; }
  }
  d.nprops = nc;
  d.narg = narg;
  d.nret = nret;

  DV(defun_dump(d,"debug parse_defun_propset: ",0,PRETTY_PRINT_DEFUN));

  return true;
}


//// apply == universal_object_dispatcher() 



//// eval object


L3STUB(system_eval_dtor)

L3METHOD(system_eval_trybody)

  LIVEO(obj);
  assert(obj);
  // get out the body sexp, it's time to evaluate it!
  if (!obj->_type || obj->_type != t_fun) {
    printf("expected function object in system_eval_trybody.\n");
    assert(0);
  }
  
  defun* df = (defun*)obj->_pdb;
  assert(df);
  l3path dfbodyshow(df->body);

DV(printf("df->body has expression:\n"));
DV(dfbodyshow.outln());

  // hmm... try this, but be ready to comment it out again.
  //obj->_parent_env = env;

  // or lexical scoping of the non-call-site variables
  // not what we want: inability to get at local variables first:  assert(obj->_parent_env == df->env);

  // this needs to be the call object (i.e. closure_env) so that temp
  //  arguments get allocated in that env, which can disappear when 
  //  the call is done. Hmm... this comment is not consistent with what we are seeing, because we are seeing the passing of the t_fun.
 Tag* owner_to_use = owner;
  if (env) {
    if (env->_mytag) {
       owner_to_use = env->_mytag;  // owner...this is the tag of the captag 
    }
  }

  eval(obj,arity,df->body,
       env,
       retval,
       owner_to_use,
       curfo,
       etyp,
       retown,ifp);

L3END(system_eval_trybody)


L3STUB(system_eval_ctor)


L3METHOD(function_decl_per_call_dtor)
  LIVEO(obj);
  Tag* pt = (Tag*)(obj->_mytag);

  defun* df = (defun*)obj->_pdb;
  assert(df);
  // here we want to deallocate those objects the user
  //  has specified in the function body
  //  hmm... not sure what that would be yet.

  // we were seeing a memory leak in that the Tag of the function_decl, which may own
  // dtor methods for safe keeping, was leaking.
  //  so copy the dtor code from the  callobject_dtor
  if (pt && !is_forwarded_tag(obj)) {
    DV(printf("in function_decl_per_call_dtor(): l3obj* %p had non-forwarded tag: so we do full Tag::destruct() now on tag %p.\n",obj,pt));
    pt->tag_destruct((l3obj*)pt,arity,exp,L3STDARGS_ENV);

    DV(printf("this had better be empty, because we are about to delete Tag pt == %p",pt));
    DV(pt->dump("about to delete pt:    "));
    delete pt;
    obj->_mytag=0;

  } else {
    DV(printf("in function_decl_per_call_dtor(): l3obj* %p has forwarded tag: skipping Tag::destruct() on tag %p.\n",obj,pt));
  }

L3END(function_decl_per_call_dtor)



L3METHOD(function_decl_deallocate_all_done_with_defn)
{
    LIVEO(obj);
    
    defun* df = (defun*)obj->_pdb;
    assert(df);

    // unfortunately, Tag above actually owns them, so we should clean
    // our own garbage. BUT, that tag may have already deleted them, or
    //  be somewhere totally different. So we need to figure out how
    //  to get notified if we do get deleted.
    // these might be redundant...or maybe not, depending on copy status.
    // wrong level of granuality. Tag will take care of them.
#if 1
    if (df->defn_qtree)    {
        recursively_destroy_sexp(df->defn_qtree, df->defn_qtree->_owner);
    }
#endif
    
    // arg_name and ret_name allocated with strdup
    for (df->arg_name.it.set_staq(&df->arg_name); !df->arg_name.it.at_end(); ++df->arg_name.it) {
        ::free(*(df->arg_name.it));
    }
    df->arg_name.clear();
    df->arg_owntag.clear();

    vec_ustaq_typ_it be = df->arg_argtyp.begin();
    vec_ustaq_typ_it en = df->arg_argtyp.end();
    ustaq<char>* u = 0;
    for ( ; be != en; ++be) {
        u= (*be);
        u->clear();
        delete (*be);
    }

    //    df->arg_argtyp.cleanstyle_set(cln_tyse_free);
    //    df->arg_argtyp.cleanup_contained_and_clear();


    
    for (df->ret_name.it.set_staq(&df->ret_name); !df->ret_name.it.at_end(); ++df->ret_name.it) {
        ::free(*(df->ret_name.it));
    }
    df->ret_name.clear();
    df->ret_owntag.clear();

    vec_ustaq_typ_it be2 = df->ret_argtyp.begin();
    vec_ustaq_typ_it en2 = df->ret_argtyp.end();
    for ( ; be2 != en2; ++be2) {
        u= (*be2);
        u->clear();
        delete (*be2);
    }
    //    df->ret_argtyp.cleanstyle_set(cln_tyse_free);
    //    df->ret_argtyp.cleanup_contained_and_clear();

    // was placement newed up, inside the allocation with the whole object: 
    // so we do not do  delete df; instead we call the destructor explicitly,
    // and let the free deallocate the superset memory block.
    df->~defun(); // this is the correct counter-part to the placement new. An explicit destructor call.
    df = 0;
    
}
L3END(function_decl_deallocate_all_done_with_defn)

// obj *no longer* has the env_to_insert_in, so we can set _parent_env correctly for local lookup; this
// functionality was factored out into a separate function: set_static_scope_parent_env_on_function_defn()
//
// now obj has a  l3path*  that gives us good debugging internal labels 
//
// takes ownership of exp, so copy it first if you need it to outlast the function defn (like when?)
//

L3METHOD(make_new_function_defn)

  DV(std::cout << "diagnostics in make_new_function_defn, exp: " << exp->span() << "\n");

  arity = exp->nchild();
  if (arity != 3) {
      printf("error in make_new_function_defn: expected arity of 3, got: %ld\n",arity);
      l3throw(XABORT_TO_TOPLEVEL);
  }


  assert(obj);
  l3path& name_to_insert = *((l3path*)obj);

// should be same as name_to_insert:  sexp_t* fn     = exp->ith_child(0);
  if (exp->headnode() == 0) {
      printf("error in make_new_function_defn: expression did not have a headnode.\n");
      exp->printspan(0,"     ");
      l3throw(XABORT_TO_TOPLEVEL);      
  }

  // validate that the name is not already bound to something else...
  // to avoid problems like:
  //   seq=2
  //   seq=($de seq ($prop) 23); // crashes because seq was already bound to 2 and the LHS ref is made-stale by the RHS.
  //
   qqchar v = exp->ith_child(0)->val();
   llref* innermostref = 0;
   l3obj* found = RESOLVE_REF(v, env, AUTO_DEREF_SYMBOLS, &innermostref, 0, v, UNFOUND_RETURN_ZERO);
   if (found) {
       l3path pre_existing_name(v);
       printf("error in make_new_function_defn: a binding for the proposed function name '%s' already exists. Please rm it before defining it as a function. Expression was:\n", pre_existing_name());
      exp->printspan(0,"     ");
      l3throw(XABORT_TO_TOPLEVEL);
   }


  sexp_t* decl = exp->ith_child(1);
  sexp_t* body = exp->ith_child(2);

  size_t txtlen = strlen(linebuf)+1;

    size_t firstsz = txtlen + sizeof(defun);
    size_t alignbytes = firstsz % 8;
    size_t total_extra = firstsz + (8 - alignbytes);

    if (sizeof(l3obj)+total_extra > INT_MAX) {
        long overtot = sizeof(l3obj) + total_extra;
        printf("object too big: requested size %ld exceeds max allowed object size %ld (INT_MAX).\n",overtot, (long)INT_MAX);
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

#if 0  // try this a little further down, since it seems to be leaking captagTag's
    if (!parse_defun_propset(*df, env)) {
        // abort; get rid of object.
        generic_delete(p,L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }
#endif

  /* system_eval methods */
  p->_dtor    = &function_decl_deallocate_all_done_with_defn;

  p->_trybody = &system_eval_trybody;
  p->_ctor    = &system_eval_ctor;
  p->_cpctor  = &function_defn_cpctor;

  p->_type = t_fun;

  LIVEO(p);

    if (!parse_defun_propset(*df, env)) {
        // abort; get rid of object.
        generic_delete(p,L3STDARGS_OBJONLY);
        l3throw(XABORT_TO_TOPLEVEL);
    }


  DV(printf("here in make_new_function_defn is the object, pre set_sha1:\n");
     print(p,"",0));

  set_sha1(p);
  
  *retval = p;

L3END(make_new_function_defn)

//
// obj  : input: obj has the function whose parent_env is to be set
// env  : input: env has the env to set parent_env to.
//
//   all other params: ignored.
//
L3METHOD(set_static_scope_parent_env_on_function_defn)
{
#if 0
    if (obj->_type != t_fun && obj->_type != t_clo && obj->_type != t_lda) {
        printf("error: set_static_scope_parent_env_on_function_defn called on non function object of type '%s'.\n",
               obj->_type);
        l3throw(XABORT_TO_TOPLEVEL);
    }
#endif
    assert(0==obj->_parent_env);

    obj->_parent_env = env;
}
L3END(set_static_scope_parent_env_on_function_defn)


L3METHOD(function_defn_cpctor)
{
         LIVEO(obj);
         l3obj* src  = obj;     // template
         l3obj* nobj = *retval; // write to this object
         assert(src);
         assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
              
                defun* dfsrc = (defun*)(src->_pdb);
                defun* df    = (defun*)(nobj->_pdb);

                df->defn_qtree = copy_sexp(dfsrc->defn_qtree,retown);
                df->propset = df->defn_qtree->ith_child(1);
                df->body    = df->defn_qtree->ith_child(2);

                l3path check_body(df->body);
                DV(printf("check_body: %s\n", check_body()));

                df->myob =   nobj;
                df->env  =   dfsrc->env;

  nobj->_parent_env = 0; // can't assume that src->_parent_env won't get deleted later. (this does happen).
  nobj->_sib = 0; 

  nobj->_ctor    = src->_ctor;
  nobj->_trybody = src->_trybody;
  nobj->_dtor    = src->_dtor;
  nobj->_cpctor  = src->_cpctor;

  //  nobj->_type = src->_type; // already set

  set_sha1(nobj);

} 
L3END(function_defn_cpctor)



void  print_defun(l3obj* obj,const char* indent, stopset* stoppers) {
  if (!obj) return;
  assert(obj->_type == t_fun);

  if (!enDV)  {
    printf("%s%p :  function definition.\n",indent,obj);
    return;
  }

  printf("%s%p :  defun, details:\n",indent,obj);
  defun* df = (defun*)obj->_pdb;

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  defun_dump(*df, indent_more(),0,PRETTY_PRINT_DEFUN);
  printf("%s     bound variables in this defun (declaration) env (local hashtable):\n", indent_more());
  dump_hash(obj, indent_more(),0);  
}


void  print_defun_full(l3obj* obj,const char* indent, stopset* stoppers) {

  if (!obj) return;
  assert(obj->_type == t_fun);
  printf("%s%p :  defun, details:\n",indent,obj);
  defun* df = (defun*)obj->_pdb;

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  defun_dump(*df, indent_more(),0,PRETTY_PRINT_DEFUN);
  printf("%s     bound variables in this defun (declaration) env (local hashtable):\n", indent_more());
  dump_hash(obj, indent_more(),0);  


  // gone to anonymous functions: printf("symbol_name funcname: %s\n",df->funcname());

  l3path propset(df->propset->span());
  printf("sexp_t* propset: %s\n", propset());

  l3path body(df->body->span());

  printf("sexp_t* body: %s\n",body());
  printf("\n");
  printf("l3obj*  myob:   %p\n",  df->myob);
  printf("l3obj*  env:    %p\n",  df->env);
  printf("int     nprops: %d\n",    df->nprops);
  printf("\n");

  printf("int narg: %d\n", df->narg);
  printf("int nret: %d\n", df->nret);
  printf("\n");

  //  std::vector<std::string>  arg_name;
  printf("std::vector<std::string>  arg_name:   %ld\n", df->arg_name.size());

  //  std::vector<owntag_en>    arg_owntag;
  printf("std::vector<owntag_en>    arg_owntag: %ld\n", df->arg_owntag.size());

  //  std::vector<t_typ>        arg_argtyp;
  printf("std::vector< ustaq<char> >        arg_argtyp: %ld\n", df->arg_argtyp.size());
  printf("\n");

  //std::vector<std::string>  ret_name;
  printf("std::vector<std::string>  ret_name:   %ld\n", df->ret_name.size());

  //std::vector<owntag_en>    ret_owntag);
  printf("std::vector<owntag_en>    ret_owntag: %ld\n", df->ret_owntag.size());

  //std::vector<t_typ>
  printf("std::vector<t_typ>        ret_argtyp: %ld\n", df->ret_argtyp.size());
  printf("\n");

  printf("\n");

}



long invocation_arity_including_funcname(sexp_t* sexp) {
    long n = sexp->nchild();
    if (sexp->_headnode) { ++n; }
    return n;
}

/*
  long narg;
  std::vector<char*>     arg_txt;
  std::vector<t_typ>     arg_typ;
  std::vector<l3obj*>    arg_val;
  std::vector<llref*>    arg_val_ref;
  l3path                 orig_call;
  sexp_t*                orig_call_sxp;
  char*                  srcfile_malloced;
  long                   line;
};
*/

void dump_actualcall(actualcall* a, const char* indent, stopset* stoppers) {
  assert(a);

  printf("%snarg: %ld\n",indent, a->narg);
  
  for (uint i = 0; i < a->arg_key.size(); ++i) {
    printf("%sarg_key[%d] = '%s'\n",indent, i, a->arg_key[i]);
  }

  for (uint i = 0; i < a->arg_txt.size(); ++i) {
    printf("%sarg_txt[%d] = '%s'\n",indent, i, a->arg_txt[i]);
  }

  for (uint i = 0; i < a->arg_typ.size(); ++i) {
      printf("%sarg_typ[%d] = '%s'\n",indent, i, a->arg_typ[i]);
  }

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  l3obj* aa = 0;
  llref* ll = 0;
  assert((long)a->arg_val.size() == a->arg_val_ref.size());

  a->arg_val_ref.it.restart(); 
  for (uint i = 0; i < a->arg_val.size(); ++i, ++(a->arg_val_ref.it)) {
      assert(!(a->arg_val_ref.it.at_end()));

      printf("%sarg_val[%d] = \n",indent, i);
      aa = a->arg_val[i];
      print(aa,indent_more(),stoppers);
      
      ll = *(a->arg_val_ref.it);
      if (ll) {
          printf("\n%sarg_val_ref[%d] = \n",indent, i);
          print_llref(ll,(char*)indent);
      }

  }

  DV(
  printf("%sorig_call_malloced = '%s'\n",indent, a->orig_call());
  l3path orig(a->orig_call_sxp);
  printf("%sorig_call_sxp = '%s'\n",indent, orig());
  printf("%ssrcfile_malloced = '%s'\n",indent, a->srcfile_malloced);
  printf("%sline = %ld\n",indent, a->line);
     );
}


void  print_cal(l3obj* obj,const char* indent, stopset* stoppers) {
  if (!obj) return;
  assert(obj->_type == t_cal);
  LIVEO(obj);
  actualcall* pcall = (actualcall*)obj->_pdb;
  dump_actualcall(pcall, indent,stoppers);
  printf("%shere is the local hash table for the call object:\n",indent);

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");
  dump_hash(obj,indent_more(),stoppers);
  printf("%scall object is named: '%s'\n",indent, obj->_varname);
}




L3METHOD(callobject_dtor)
{
    LIVEO(obj);
    actualcall* pcall = (actualcall*)obj->_pdb;
    assert(pcall);

    // arg_key
    pcall->arg_key.clear();

    // arg_txt
    if (pcall->arg_txt.size()) {
        vec_cpchar_it be, en;
        be = pcall->arg_txt.begin();
        en = pcall->arg_txt.end();
        for (; be != en; ++be) {
            ::free(*be);
        }
        pcall->arg_txt.clear();
    }
    pcall->arg_typ.clear();
    pcall->arg_val.clear();
    pcall->arg_val_ref.clear();

    // 
    if (pcall->orig_call_sxp) {
        // causes crashes..
        //        destroy_sexp(pcall->orig_call_sxp);
    }
    pcall->arg_val_ref.clear();
    
    if (pcall->srcfile_malloced) {
        ::free(pcall->srcfile_malloced);
        pcall->srcfile_malloced = 0;
    }
    
    pcall->ret_val = 0;

    pcall->~actualcall(); // opposite of placement new is explicit destructor invocation.
    pcall =0;
}
L3END(callobject_dtor)  


L3STUB(callobject_trybody)
L3STUB(callobject_ctor)

// fill_in_preallocated_new_callobject() : 
//
// call objects also get their own special Tag: this is
//  to hold the temporaries created during a function execution that
//  should be cleaned up after the call is done: basically all stack
//  auto alloated variables delt with during the function call are
//  owned by the call object's own Tag, which then disposes of them
//  when the call is done.
//
//
//  calls can also be sources of new objects; if a call's decl has out.s.own, out.c.own, 
//   or a return value (which has to be either out.s.own or out.c.own) then
//    for each of the out.s.own(s), the callobj needs to have a new objtect pointer
//    available to hold them.
//

// new protocol, call          make_new_captag((l3obj*)&basenm ,arity,exp,env,(l3obj**) &args , owner, curfo,t_cal,owner);
// first, and then pass that args in as the obj.

// TODO: verify that this is an exact replacement for the old version of make_new_callobject... b/c it didn't seem
//  to be working right / as well.

// call as: make_new_callobject(0,-1,exp, env, (l3obj**) &args, (Tag*)try_disp_tag, curfo, t_cal,(Tag*)try_disp_tag);
L3METHOD(make_new_callobject)

  l3obj* args = 0; // the t_cal callobject
  l3path basenm("make_new_callobject_result");

  make_new_captag((l3obj*)&basenm ,arity,exp,env,(l3obj**) &args, owner, 0,t_cal, owner,ifp);

  fill_in_preallocated_new_callobject(L3STDARGS);

L3END(make_new_callobject)


L3METHOD(fill_in_preallocated_new_callobject)
{  

  qqchar operatorname(exp->headval());
  llref* innermostref = 0;

  l3obj* fun = RESOLVE_REF(operatorname,env,AUTO_DEREF_SYMBOLS, &innermostref,0,0,UNFOUND_RETURN_ZERO);

  if (!fun && curfo) {
         fun = RESOLVE_REF(operatorname,curfo,AUTO_DEREF_SYMBOLS, &innermostref,0,0,UNFOUND_RETURN_ZERO);
  }

  if (fun == 0) {
      std::cout << "error in attempted call: could not find function named '"<< operatorname << "'"; 
      printf(" in the current environment %p.\n",env);
      l3throw(XABORT_TO_TOPLEVEL);
  }
  
  assert(fun->_type == t_fun);
  defun* decl = (defun*)fun->_pdb;
  
  long farity = invocation_arity_including_funcname(exp) -1;
  
  if (farity != decl->narg) {
      std::cout << "error in attempted call: arity mismatch. Function '"<< operatorname << "'"; 
      printf("requires %d argument(s), and %ld were supplied.\n",decl->narg,farity);
      l3throw(XABORT_TO_TOPLEVEL);
  }
  
  assert(farity >=0);

  l3path funcall_name(operatorname);
  funcall_name.pushf("_call");

  l3obj* nobj = obj; // converted from the above for fill_in_preallocated_new_callobject.

  // TODO: 
  // get the function to call, called the operator, allowing it to be the result of another
  // function call in this position... but implement this later!
  //  l3obj* operator = eval(first_child(exp),env);
  //  if (operator->_type != t_str) {
  //    
  //  }
  //
  // simpler first cut: take the named string as the function
  
  
  nobj->_type  = t_cal;
  
  actualcall* pcall = (actualcall*)nobj->_pdb;

  pcall = new(pcall) actualcall(); // placement new. for vectors.

  // constructor does this now:  pcall->arg_val_ref.init();

  pcall->narg = farity;

  // _judyL : the data of the "call" is the auto stack

  // these are the variables that would, under compilation, end
  // up on the call stack (auto allocated).
  //
  // notice that the parent is here the lexical environment where
  //  the call was declared. This needs to change to become the
  //  dynamic env when the call is invoked. So that the owner
  //  ship of values can be transfered... actually just set
  //  _mytag appropriatedly.

  // this is *our* tag, it lives as long as we do; so we don't
  //  assign it to our owner





  pcall->orig_call.reinit(exp);
  pcall->srcfile_malloced=0;
  // line
  pcall->line = 0;

  // orig_call_sxp
  // new:
  pcall->orig_call_sxp = exp; 
  

  /* callobject specific methods */
  nobj->_dtor    = &callobject_dtor;
  nobj->_trybody = &callobject_trybody;
  nobj->_ctor    = &callobject_ctor;

  // evaluate the actual arguments in env

  sexp_t* nex = 0;

  for (long i = 0; i < farity; ++i ) {

      nex = exp->ith_child(i);

      l3path cur_sexp(nex);

    char* owned = strdup(cur_sexp());   
    pcall->arg_txt.push_back(owned);
    // and free that malloc with the tag...!
    assert(nobj->_mytag);
    // the arg_txt.push_back(owned) above will take care of cleanup: this therefore is duplicate and generates a double free error:
    // nobj->_mytag->gen_push(owned);

    l3path nmarg(0,"actual_arg_%d_to_call_",i);
    nmarg.pushq(operatorname);
    l3obj* arg = 0;

    //    eval(0,-1,nex,env,&arg,owner,curfo,arg->_type, retown,ifp);
    eval(0,-1,nex,env,&arg,owner,curfo,0, nobj->_mytag,ifp);
    assert(arg);

    if (arg->_type == t_lit) {
        std::cout << "error in invocation of '"<< operatorname << "': "; 
        printf("expression '%s' could not be resolved.\n", cur_sexp());
        *retval = 0;

        l3throw(XABORT_TO_TOPLEVEL);
    }


    // if we have a reference in nex, remember it by name...
    bool pushed_arg_val_ref = false;
    if (num_children(nex)==0) {
        if (cur_sexp.len()) {
            if (cur_sexp()[0] != '(') {
                // we could have a reference...
                llref* innermostref = 0;

                //                l3obj* found_nex_ref = resolve_dotted_id(cur_sexp(), env, AUTO_DEREF_SYMBOLS, &innermostref,0,0,UNFOUND_RETURN_ZERO);
                l3obj* found_nex_ref = resolve_static_then_dynamic(cur_sexp(), env, AUTO_DEREF_SYMBOLS, &innermostref,0,0,UNFOUND_RETURN_ZERO);

                if (found_nex_ref && innermostref) {
                    pcall->arg_val_ref.push_back(innermostref);
                    pushed_arg_val_ref = true;
                }
            }
        }
    }
    if (!pushed_arg_val_ref) {
        pcall->arg_val_ref.push_back((llref*)0);
    }


    pcall->arg_val.push_back(arg);
    if (!qtypesys->which_type((char*)arg->_type,0)) {
      assert(0); // bad type?
    }
    pcall->arg_typ.push_back(arg->_type);
    //    pcall->arg_key // <- not filled in until invocation (lazily). This is
    //                         actually implemented.

  } // end for i over farity

  // the call naturally contains a pointer to the dynamic scope.

  // set our _parent_env env to be env, the dynamic env of the call
  // for (auto / stack based value lookup). For lexical lookup,
  // the call declaration decl object might want to (should?) have _parent_env set
  // to the lexical environment.

  // no: better: make in fun->_parent_env b/c that reflects the local ownership env.
   // no, can be zero: LIVEO(fun->_parent_env);
  nobj->_parent_env = fun->_parent_env;

  // the defun object naturally contains a pointer in it's _parent_env to its static scope,
  //  available when it was defined. The defun object hopefully cached everything it
  // needed at the point of definition, but may not have if there are free variables.

  DV(print(nobj,"debug: make_new_callobject(): ",0));

  *retval = nobj;
}
L3END(fill_in_preallocated_new_callobject)



MLOG_ONLY( extern jmemlogger* mlog; )

//
// protocol: write details to disk. When reg() gets called with a string name
//  for the memory, then we'll delete the text file and replace it with a
//  symlink. That way we can tell if any of them never get converted, which
//  ones were left dangling without a name.
//

void* jmalloc(size_t size,
          Tag* owntag, 
          merlin::jmemrec_TagOrObj_en type, 
          const char* classname,
          const char* where_srcfile_line,
              l3path* vn
              ,long notl3obj, // defaults should be 0, for allocating l3obj*; for other structures, set to 1
              t_typ ty
              ) {

  assert(owntag);
  assert(vn);
  l3path& varname = *vn;

  DV(  if (notl3obj) {
      printf("note: jmalloc is allocating a non- l3obj now...\n");
      printf("\n");
      });

  //  sgt.global_dump();
  if (!notl3obj) { assert(size >= sizeof(l3obj)); }

  // minimal requirements are now are in uhead, so do this to let the compiler confirm
  l3obj* ret = (l3obj*)malloc(size);
  //uh* ret = (uh*)malloc(size);
  bzero(ret,size);
  ret->_type = ty;

  varname.pushf("_uniq%p",ret);

  ret->_owner = owntag; 

  long sn = serialfactory->serialnum_obj(ret, owntag, varname());

  // NDEBUG
  if (!notl3obj)  { 
      strncpy( ((l3obj*)ret)->_varname, varname(), sizeof(((l3obj*)ret)->_varname));
  }

  ret->_ser = sn; 

  /* retain our size in bytes used for malloc, for umem realloc/free */
  ret->_malloc_size = size;

  DVV(serialfactory->dump());

  if (!notl3obj)  { 
      owntag->add((l3obj*)ret,varname(),sn);
  } else {
      if (ret->_type == t_lnk) {
          assert(size == sizeof(lnk));
          owntag->lnk_push((lnk*)ret);
      } else {
          assert(0); // what other cases?
      }
  }

  //  name2owner(ret,vn());


  l3path msg(0,"777777 %p jmalloc: of size %ld '%s'  @serialnum:%ld\n",ret,size,varname(),sn);
  DV(printf("%s\n",msg()));
  MLOG_ADD(msg());

  return ret;
}


void jfree(l3obj* ptr) {
  DV(printf("jfree called! on ptr = %p \n",ptr));
  long sn_for_breakpt = ptr->_ser;
  assert(sn_for_breakpt);

  //#ifdef _JLIVECHECK // how do you know you even have an object here??? have to put this conditionally under _JLIVECHECK

  LIVEO(ptr);

  BOOL builtin = is_sysbuiltin(ptr);
  BOOL undeletable = is_undeletable(ptr);

  if (builtin || undeletable) {
      if (!global_terp_final_teardown_started) {
          printf("internal error: trying to jfree(%p) when it is marked as bultin/undeletable.\n",ptr);
          assert(0);
      }
  }
  //#endif


#if 0 // calling obj_llref_cleanup() here is a formula for disaster. jfree cannot be recursively free-ing!! lnk can be to builtins etc!!!
  if (ptr->_type == t_lnk ) {
      // make sure we don't do any other mucking with memory beyond
      assert(ptr->_malloc_size == sizeof(lnk));
  } else {
      // free any llrefs still in here...
      if (ptr->_type == t_obj) {
          obj_llref_cleanup(ptr,L3NONSTANDARD_OBJONLY);
      } else if (ptr->_type == t_syv) {
          // _judyS points to symbol* instead of llref...
      }
  }
#endif

  //Size size  = ptr->_malloc_size;
  l3path name_freed;
  long ser = serialfactory->del(ptr,&name_freed);
  if (!builtin)  {  serialfactory->loose_ends_check(ptr,ser); }

  // we don't do this: it only happens the other way around: that delall calls jfree()!
  //Tag* owntag = (Tag*)ptr->_owner;
  // owntag->del(ptr);



  l3path msg(0,"888888 %p jfree. '%s' @serialnum:%ld",ptr, name_freed(), ser);
  DV(printf("%s\n",msg()));
  MLOG_ADD(msg());

#if 0  // what if jmalloc/jfree are used on non- t_obj ???

      // make sure these get zeroed. To make it easy to locate double frees.
      ptr->_owner = 0;
      ptr->_mytag = 0;
      bzero(ptr,ptr->_malloc_size); // sets _malloc_size = 0, as well as _type = 0;

#endif

  ::free(ptr);

}




// utility functions for sexp_t* handling
//


bool has_children(sexp_t* s) {
  assert(s);
  return s->nchild() != 0;
}

long num_children(sexp_t* s) {
  assert(s);
  return s->nchild();
}

long first_child_chain_len(sexp_t* s) {
  assert(s);

  sexp_t* p = s;
  long n = 0;
  
  while(1) {
      if (0 == p->_chld.size()) break;
      ++n;
      p = p->_chld[0];
  }

  return n;
}

// returns 0 if no such child
sexp_t* first_child(sexp_t* s) {
  assert(s);
  return s->first_child();
}


// returns 0 if no such child
sexp_t* ith_child(sexp_t* s, long i) {
  assert(s);
  assert(i >= 0);

  return s->ith_child(i);
}

// end sexp_t* utilities


// new object dipatcher: makes calls to an objects methods right away, to
// avoid issues with stack restoration on longjmp. Also declares the
// return value to be volatile, so it is preserved!


/////////// generic methods


L3METHOD(generic_dtor)

#if 0 //  definitely don't think the object's dtor should be deleting a tag!!!
    if (obj->_mytag) {
        if (!is_forwarded_tag(obj)) {

               obj->_mytag->tag_destruct(obj,L3STDARGS_OBJONLY);
               if (obj->_mytag) {
                 delete obj->_mytag;
               }

        }
    }
#endif
L3END(generic_dtor)


L3STUB(generic_trybody)
L3STUB(generic_ctor)

// i.e. shallow copy... might be ok for double and value vectors, but not
// for anything with references in it.
L3METHOD(generic_cpctor)

         l3obj* src  = obj;     // template
         l3obj* nobj = *retval; // write to this object
         assert(src);
         assert(nobj);


#if 0 
// these were overwriting important stuff!?!?!
         // _judyS
         copy_judySL(src->_judyS, &(nobj->_judyS));

         // _judyL
         copy_judyL(src->_judyL, &(nobj->_judyL));
#endif

L3END(generic_cpctor)

//////////// end generic methods



 l3obj* make_new_class(size_t num_addl_bytes, Tag* owner, const char* classname, const char* vn) {
  /* ensure additional bytes requested are aligned */
  assert(num_addl_bytes % sizeof(void*) == 0); 
  size_t sz = sizeof(l3obj);
  //  printf("raw: sizeof(l3obj) is %ld\n",sz);

  sz += num_addl_bytes;
  //  printf("with %ld additional bytes: sizeof(l3obj) is actually %ld\n",num_addl_bytes,sz);

  /* allocation */
  if (classname == 0) {
    classname = "generic_new_class";
  }

  l3path varname(vn);

  // NEWOBJ is a macro for jmalloc
  //                       jmalloc bzero's everything; then
  //                       jmalloc sets _owner   <-- don't overwrite!; then
  //                       jmalloc sets _malloc_size  <-- don't overwrite
  //
  l3obj* p = (l3obj*)NEW_OBJ(sz,owner,classname,&varname);

  set_newborn_obj_default_flags(p);

  // on Mac, new doesn't get tracked by dmalloc. Drat.
  // So instead, do a malloc with a placement new.
  //     p->_judyS = new judys_llref;

  p->_judyS = (judys_llref*)::malloc(sizeof(judys_llref));
  p->_judyS = new(p->_judyS) judys_llref(); // placement new

  /* generic methods */
  p->_dtor    = &generic_dtor;
  p->_trybody = &generic_trybody;
  p->_ctor    = &generic_ctor;
  p->_cpctor  = &generic_cpctor;

  /*_pde and _pdb default values*/
  p->_pdb = (char*)p + sizeof(l3obj);
  p->_pde = (char*)p + sz;

  p->_owner = owner;

  /* default value for _mytag forwarding is forwarded */
  set_forwarded_tag(p);

  // _mytag should last as long as this object! no shorter, no longer.
  p->_mytag = 0;

  return p;
}


L3METHOD(hash_dtor)

   obj->_judyS->clear();
   obj->_judyS = 0;

L3END(hash_dtor)

// unfinished...and un used at the moment.
L3METHOD(hash_cpctor)
   
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj);


    
    // _judyS
    //copy_judySL(src->_judyS, &(nobj->_judyS));
    *nobj->_judyS = *src->_judyS;

// from show keys...

    size_t       sz = 0;
    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, src->_judyS->_judyS, Index);       // get first string


    while (PValue != NULL)
    {
      llref* ele = *((llref**)(PValue));
      assert(ele);

      JSLN(PValue, src->_judyS->_judyS, Index);   // get next string
      ++sz;
    }

    return sz;


L3END(hash_cpctor)


l3obj* make_new_hash_obj(Tag* owner, const char* varname) {
  l3obj*  obj = make_new_class(0, owner,"hash",varname);
  obj->_type  = t_env;
  obj->_dtor = &hash_dtor;
  obj->_cpctor = &hash_cpctor;
  return obj;
}



//
// env = has call object (args) if is type _cal
//       Otherwise env just passed along as env.
//
// universal_object_dispatcher does obj = fun to call (function defintion)
//                                  env = callobject with arguments to call with, can be 0 if no args.
//                                  curfo = the calling env, in which the call takes place (and where to return any out @< variables)

L3METHOD(universal_object_dispatcher)
{


  LIVEO(obj);

  // we should already have an allocated space for the return value in retval
  // if it will be required.
  l3obj* fun = obj;
  l3obj* cal = 0;

  if (env) {
      if (env->_type == t_cal) {
          cal = env;
      }
  }

  Tag*  transfer_to_tag = 0;
  l3obj* transfer_to_obj = 0;
  l3path addme;
  l3path bname;
  l3path args_orig_refname;
  char context_name[BUFSIZ];
  bzero(context_name,BUFSIZ);
  //  volatile Tag* orig_top = glob;

  assert(fun->_type == t_fun);
  defun*      decl = (defun*)fun->_pdb;

  // the return value object, properly owned by callobj->_parent_env
  //l3obj* retobj = 0;



  // but cal might also be a formal function invocation.
  // which we now detect here.
  actualcall* pcall = 0;
  if (cal) {

  // this has to happen inside TRY/FINALLY...or else doesn't get cleaned up right
//      global_env_stack.push_front(cal);

    assert(cal->_type == t_cal);
    pcall = (actualcall*)cal->_pdb;

    // generate the mapping from formals -> actuals, in an env
    if(pcall->narg != decl->narg) {
        // went to anonymous functions:
        //        printf("Error: function '%s' expects %d arguments, this call '%s' instead has %ld\n",
        //               decl->funcname(),  
        printf("Error: function expects %d arguments, this call '%s' instead has %ld\n",
               decl->narg,
               pcall->orig_call(), pcall->narg);
        return 0;
    }

    // type check

    // TODO: upon typecheck: check that arg names and return names don't overlap

    int narg = pcall->narg;
    l3path t_typ_chain_as_string;

    decl->arg_name.it.set_staq(&decl->arg_name);
    for (int i = 0; i < narg; ++i, ++(decl->arg_name.it) ) {
        t_typ_chain_as_string.clear();

        ustaq<char>* ust = (decl->arg_argtyp[i]);
        if (!(ust->member(t_any))) {
            
            if (!(ust->member(pcall->arg_typ[i]))) {

                    ust->dump_to_l3path(&t_typ_chain_as_string);

                    printf("error: type mismatch during function invocation. "
                           "Function expects arg #%d to be type '%s', "
                           "but call '%s' had arg #%d with type '%s'. "
                           "Args are numbered starting at 1 here.\n",
                           i+1, t_typ_chain_as_string(),
                           pcall->orig_call(), i+1, pcall->arg_typ[i]);
                    l3throw(XABORT_TO_TOPLEVEL);
                    return 0;
            }
        }
        pcall->arg_key.push_back(*(decl->arg_name.it));
    }

    // set the pointers on the call->_judyS to point to the actuals (symlinks though, no ownership here)

    DV(printf("before assigning actual call arguments into formals slots of the function defn, here is cal:\n"));
    DV(print(fun,"debug cal: ",0));

    l3obj* next_actual_val = 0; // (pcall->arg_val[i]);

    for (int i = 0; i < narg; ++i) {
      next_actual_val = (pcall->arg_val[i]);
      DV(printf(               "debug: next formal is named %s:\n",(char*)pcall->arg_key[i]));
      DV(print(next_actual_val,"debug:                       \\--> actual->formal matchup: ",0));

      llref* added = add_alias_eno(cal, (char*)pcall->arg_key[i], next_actual_val);

      // add pass by reference to the newly created ref.
      // does it matter if this is !>   @>   !<   or  @<  ?
      // let's keep pass by refernce/vs value separate from ownership at the moment.

      if (decl->arg_argtyp[i]->member(t_ref)) { 
          added->add_prop(t_ref); 
      }

    }

    DV(printf("after assigning actual call arguments into formals slots of the cal, here is cal:\n"));
    // fires assert b/c arg_val_ref not done yet:    DV(print(cal,"debug cal: ",0));

    if (cal->_parent_env) {
      DV(printf("and the parent if any of cal:\n"));
      DV(if (cal->_parent_env) print(cal->_parent_env,"debug cal->_parent_env: ",0));
    }

    // print fun and parent
    DV(print(fun,"debug fun: ",0));
    DV(if (fun->_parent_env) print(fun->_parent_env,"debug fun->_parent_env: ",0));
    
    // ==================================================
    //
    //  TAG TAG TAG processing!
    //
    // here is where we arrange for storage tag transfers.
    // ==================================================
    
    next_actual_val = 0;
    llref* llr_for_props = 0;
    llref* callers_llr = 0;
    int i = 0;

    decl->arg_name.it.set_staq(&decl->arg_name);
    for(pcall->arg_val_ref.it.restart();  !pcall->arg_val_ref.it.at_end();  ++i, ++(pcall->arg_val_ref.it), ++(decl->arg_name.it) ) {
        
        assert(i < narg);
        callers_llr = *(pcall->arg_val_ref.it);
        next_actual_val = (pcall->arg_val[i]);

        t_typ_chain_as_string.clear();
        decl->arg_argtyp[i]->dump_to_l3path(&t_typ_chain_as_string);
      
        DV(printf("debug: next formal is %s %s %s :\n",
                  *(decl->arg_name.it),
                  owntagtype2abbrev(decl->arg_owntag[i]),
                  t_typ_chain_as_string()
                  )
           );
      
        DV(print(next_actual_val,"debug:                       \\--> actual->formal matchup: ",0));


        // gotta get the llref, since that is where all the tag/ownership and other variable properties live.
        llr_for_props = (llref*)lookup_hashtable(cal, (char*)pcall->arg_key[i]);
        LIVEREF(llr_for_props);
        assert(llr_for_props);
        assert(llr_for_props->_type == t_llr);
        assert(0==strcmp(llr_for_props->_key, (char*)pcall->arg_key[i]));
      
      t_typ ot = decl->arg_owntag[i];

      if (ot == t_ico) {

          llr_for_props->add_prop(t_ico);
          assert(llr_for_props->has_prop(t_ico));

      } else if (ot == t_iso) {

          llr_for_props->add_prop(t_iso);
          assert(llr_for_props->has_prop(t_iso));

          // the default tag for the server/function has gotta own this
          
          // old, from when fun where not cap-tag pairs.
          //          transfer_to_tag = fun->_owner;
          //          transfer_to_obj = fun->_owner->captain();

          // new, now that function definitions are cap-tag pairs.
          transfer_to_tag = fun->_owner->_parent;
          transfer_to_obj = fun->_owner->_parent->captain();

          // the release_to will delete the original reference, which we want
          // to preserve and continue to make available to the client, even though
          // ownership has changed... so how do keep that ref around when we transfer? 
          // we cannot keep callers_llr, since we don't know who owns it...???
          l3obj* callers_ref_env = callers_llr->_env;
          l3path callers_ref_key(callers_llr->_key);

          next_actual_val->_owner->generic_release_to(next_actual_val, transfer_to_tag);

          args_orig_refname.init(pcall->arg_txt[i]);
          
          // try this instead:  decl->arg_name[i] has our formal

          llref* already_preexisting = (llref*)lookup_hashtable(transfer_to_obj, *(decl->arg_name.it) ); 
          if (already_preexisting) {

              // delete the old ref first...
              l3obj* preobj = already_preexisting->_obj;
              llref_del(already_preexisting,YES_DO_HASH_DELETE);

              // and give tag a chance to delete the object...or not.
              preobj->_owner->reference_deleted(preobj,L3STD_OBJ);

          } 
          llref* added_iso = add_alias_eno(transfer_to_obj, *(decl->arg_name.it), next_actual_val);
          added_iso->add_prop(t_iso);

          // now restablish the env/key from callers_llr
          if ( !lookup_hashtable(callers_ref_env, callers_ref_key()) ) {
              add_alias_eno(callers_ref_env, callers_ref_key(), next_actual_val);
          }

      } else if (ot == t_oco) {

          // out_c_own parameters
          //
          // @< or out_c_own is simply call by reference; where the callee can (and will) change the
          //   value during the call, so that the caller sees (and continues to own) a new value. 
          //   Although actually the function may choose not to change the out_c_own variable as well,
          //    but this is typically the exceptional case, since why else pass it in!?!

          llr_for_props->add_prop(t_oco);
          assert(llr_for_props->has_prop(t_oco));
          
          // don't change ownership; the passed in actual arg indicates name and ownerhsip.
          // i.e. What we want: we don't change the owner... we don't change the name, we just change the value.
          // which means.... there isn't anything else to do, since we already did it above at:
          //          add_alias_eno(cal, (char*)pcall->arg_key[i], next_actual_val);
          
          
      } else if (ot == t_oso) {
          
          llr_for_props->add_prop(t_oso);
          assert(llr_for_props->has_prop(t_oso));
          
      } else {
          printf("internal error: bad transfer type.\n");
          assert(0);
      }
    } // end i over narg
    
    // handle the return values

    // pcall->retval now exists and will point to the return value, whose
    // ownership is the caller by default, to get stuff going (before we make in more complicated, get
    // all the bugs out at this level!

    // for now, keep things simple, just have one return value.
    // other return values can be named out parameters in the call args.
    if (decl->ret_argtyp.size() > 1) {
        printf("Only one return value supported at the moment.\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }     
  } //end if(cal)


  // now we are ready to tell fun to evaluate its body in the context of its _judyS -> _parent_env


  //  printf("Entering  universal_object_dispatcher(%p)\n",p);

   //     exception handling setup...
    

  // like define NEW_AUTO_TAG; but here we manage the stack auto tags
  // for the functions below us that we dispatch to.
  // since these cannot be declared volatile, they need to
  // live on the previous stack frame up, which won't get
  // corrupted when longjmp / exception happens.

  // Why Tag stacktag is commented out below, and
  //  a dynamically allocated l3obj* callobject is used instead.
  //
  // The call object has it's own tag. This is natural. The
  //  tag is accessed at (l3obj*)obj->_mytag. Later, _mytag
  //  could also point to something on the C stack, if we watned.
  //
  // This is due to a 
  // change of strategy: now we allocate an actual call object
  // The call object is the natural place for the
  // variables allocated during the call to live. This makes
  // for a clean separation between the function definition
  // object (during a call the function definition remains immutable),
  // and the dynamic call environment.
  //
  // It's not clear if this is a better choice than the
  // stacktag implementation below, but at the moment I think
  // it let's us track and free memory that was allocated during
  // a call in an easier way than using the "Tag stacktag" on
  // the C stack method. We may resume this later, once we have
  // the ownership semantics clearly worked out. (For performance...)

  /*
  Tag stacktag(STD_STRING_WHERE, glob,"univ");
  l3path stackname;
  stackname.pushf("%p_stack_call_tag",&stacktag);
  stacktag.set_myname(stackname());
   //   printf("Tag(%p) allocated.\n",&stacktag);       
   global_tag_stack.push_front(&stacktag);  
   //   global_tag_stack_loc.push_front(STD_STRING_WHERE); 
   char oname[PATH_MAX+1];
   sprintf(oname,"%s::%s", context_name , decl->funcname() );
   global_tag_stack_loc.push_front(oname);
  */


  // if this function lives in an object, that object needs to be on the env stack as
  // well, so that references to the objects variables get resolved right... actually no.
  // Because the static name resolution via _parent_env should serve in that case.
  //  The dynamic env stack is not the right place for the static object/function relationship.

  // curfo is like "this" is C++ : it should be on the dynamic env stack, so that locals contained
  //  in the current object can be referenced, say from destructors running on the obj.

#if 0
  if(curfo) {
      assert(curfo->_mytag); // if not, why not? at least have a forwarded tag!?!
      //
      // if necessary/the above assert fires and we can't fix the source, do:
      // if (!curfo->_mytag) { curfo->_mytag = curfo->_owner; assert(!is_forwarded_tag(curfo)); set_forwarded_tag(curfo); }
      //
      defptag_push(curfo->_mytag, curfo);
  }

  if (cal) {
      assert(curfo->_mytag);
      defptag_push(cal->_mytag, cal);
  }
#endif

   // keep legacy invocation code below.
   volatile l3obj* vfun = fun;
   volatile int    code_done = 0;

   XTRY
 case XCODE:
   //      if (cal) {       // moved to above
   //         global_env_stack.push_front(cal);
   //      }
   if (vfun->_ctor)    {  vfun->_ctor(    (l3obj*)vfun,-1,0,   L3STDARGS_ENV); } // env,retval,owner,curfo,etyp,retown
   if (vfun->_trybody) {  vfun->_trybody( (l3obj*)vfun,-1,0,   L3STDARGS_ENV); } // env,retval,owner,curfo,etyp,retown

    // ==================================================
    //
    //  RETURN RETURN RETURN TAG TAG TAG processing!
    //
    //  the names of the return parameters get injected into the function code namespace so
    //  that they can be assigned to directly within the body code.
    //  
    //  Assume that value is in *retval, and so the caller already does own that.
    //   Hence all we really need to do is to put a name binding into the body namespace of the function.
    //
    // ==================================================
    if (decl->nret) {

        if (decl->ret_owntag[0] == t_oco) {
            // should already be all taken care of by using retown if need be?

            if ((*retval)->_owner != retown) {
                printf("Transfering for @< return value, using release_to call -- is this right?!\n");
                (*retval)->_owner->generic_release_to(*retval, retown);
            }
        }
    } else {
        *retval = gnil; // overwrite any other not-real retval. since it'll probably be gone shortly.
    }
    // end return values
    // end return values
    code_done = 1;
   break;

   // we don't want to catch this here--let it go up to the toplevel.
   //
   // case XABORT_TO_TOPLEVEL:
   //   XHandled();
   //   break;
   

 case XFINALLY:

   // NB: I guess destructors 
   // have to be forbidden from throwing--because otherwise we might not 
   // get back here to do part II... or we need to put in a 4th phase
   // of the exception handling cycle that gets run after finally and
   // while cleaning up the stack.
   
   global_function_finally();
   break;
   
 default:
   break;
   
   XENDX

}
L3END(universal_object_dispatcher)


 // eliminate name2owner, make the name come in when the object is allocated!
 /*void name2owner(l3obj* o, const char* name) {
  o->_owner->reg(name,o);
  DV(printf("%p <-> %s\n",o,name));
}
 */


char* rightmost_dot_elem(const char* path) {
  
  const char* i = path;
  const char* start = path;
  while(1) {
    if ('\0'== *i) break;
    if ('.' == *i) {
      start = i+1;
    }
    i++;
  }

  return (char*)start;
}

// what do to if we can't resolve...  moved to dynamicscope.h
// typedef enum { UNFOUND_RETURN_ZERO=0, UNFOUND_THROW_TOP=1 } noresolve_action;

// resolve_core : the main routine for lookup of names.
//
// used by resolve routines below... check for symbol nextonpath in curenv.
//
// currently, does not touch penvstack
//
l3obj* resolve_core(char*       nextonpath,
                    l3obj*      curenv,
                    deref_syms  deref,
                    llref**     innermostref,
                    objlist_t*  penvstack,
                    char*       curcmd,
                    noresolve_action noresolve_do) {

    assert(curenv);
    assert(nextonpath);

    symbol* psym = 0;
    l3obj* found = 0;

    llref* llr = (llref*)lookup_hashtable(curenv, nextonpath); // can return 0.
        if (llr) {
            assert(llr->_type == t_llr);

            if (curenv->_type==t_syv) {
                psym = (symbol*)llr;
                found = (l3obj*)1;
            } else {
                found = llr->_obj;
                LIVEO(found);
                *innermostref = llr;
            }
        }

        if (!found) {
            if (noresolve_do == UNFOUND_THROW_TOP) {
                printf("error: variable name resolution failed in resolve_core(): could not resolve left-hand-side element '%s' in expression '%s'.\n",nextonpath,curcmd);
                l3throw(XABORT_TO_TOPLEVEL);
            }
            return 0;
        }

        // have to handle being inside symvec namespaces special, because they point to symbol*, not l3obj*
        if (curenv->_type==t_syv) {
            if (deref == AUTO_DEREF_SYMBOLS) {
                found = psym->_obj;
                } else {
                   found = 0;
                   if (noresolve_do == UNFOUND_THROW_TOP) {
                       DV(printf("warning: could not locally resolve reference to element '%s' in expression '%s'.\n",nextonpath,curcmd));
                       l3throw(XABORT_TO_TOPLEVEL);
                   }
                   return 0;
                }
        }

   return found;
}


 //
 // resolve_id() : and chase _parent_env pointers up the environment chain if need be.
 //
 // currently does not throw but returns 0 if not found (ignores noresolve_do at the moment, just due to legacy development path).
 //

 l3obj* resolve_dotted_id(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
                          const qqchar&  curcmd, noresolve_action noresolve_do) {
     //LIVEO(startingenv); // guess this needs to be a little flexible (for now?!?)
     // handle dotted identifiers now too.

     l3path nid(id);
     l3path cc(curcmd);

    std::vector<char*> dotlist;
    l3path modified_inplace(id); // holds the dotlist strings that we point to from the stringified sexp.

    gen_dotlist(&dotlist, &modified_inplace);

    l3obj*  curenv = startingenv;
    l3obj*  found = 0;
    long n = dotlist.size();
    std::vector<char*>::iterator pathit = dotlist.begin();

    if (n < 1) {
        DV(printf("warning: could not locally resolve reference to element '%s'.\n",nid()));
        //l3throw(XABORT_TO_TOPLEVEL);
        return 0;
    }

    // track those envs we've gone into - not used (yet).
    //  std::vector<l3obj*> envstack;

    if (n > 0) {
    long countdown = n;
    while(countdown >= 1) {

        found = resolve_core(*pathit, curenv, deref, innermostref, 0, nid(), UNFOUND_RETURN_ZERO);

        if (!found) {
            return 0;
        }
        curenv = found;

        if (penvstack) { penvstack->push_back(curenv); }
        ++pathit;
        --countdown;
    }
    }


   return (l3obj*)curenv;
 }


 l3obj* resolve_dotted_up_the_parent_env_chain(const qqchar& id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
    const qqchar& curcmd, noresolve_action noresolve_do) {

   LIVEO(startingenv)
   l3obj* cur = startingenv;
   void* found = 0;
   while(1) {
     if (!cur) return 0;

     found = resolve_dotted_id(id, cur, deref, innermostref, penvstack, curcmd, UNFOUND_RETURN_ZERO);

     if (found) {
         return (l3obj*)found;
     }

     DV(printf("trying in my parent env...\n"));
     if (0==cur->_parent_env) { return 0; }
     LIVEO(cur->_parent_env);

     cur = cur->_parent_env;

     // so we allow cur to be... 0 here? I guess so.
     if (penvstack) { penvstack->push_back(0); } // 0..0..obj => ../../obj
     if (cur) { DV(print(cur,"   ",0)); }

     if(cur && cur == cur->_parent_env) {
         DV(std::cout << "error: could not resolve reference to element '"<<id<<"'.\n");

         printf("WARNING (problem?) cur == cur->_parent_env for cur object: %p when seeking identifier: '",cur);
         std::cout << id << "'\n";
         print(cur,"cur: ",0);
         return 0;
     }
   } // end while(1)

   DV(std::cout << "error: could not resolve reference to element '"<<id<<"'.\n");

   return 0;
 }

// validate_path_and_prep_insert
//
// what env_to_insert_in do we return if the dottedname has no dots, so 
// that it is specifying "add this name to the current object env" ?
//

// conforms to this...plus name_to_insert.
//l3obj* resolve_static_then_dynamic(char* id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
//                                   char* curcmd, noresolve_action noresolve_do) {
//
// return pre-existing object specified by dottedname, or 0 if no-such named object exists.
//
l3obj* validate_path_and_prep_insert(const qqchar&  dottedname, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
                                     const qqchar&  curcmd,  noresolve_action noresolve_do, 
                                     l3path* name_to_insert
                                     ) {

    l3path cc(curcmd);

    l3obj* target = 0;
    std::vector<char*> dotlist;
    l3path modified_inplace(dottedname); // holds the dotlist strings that we point to from the stringified sexp.

    gen_dotlist(&dotlist, &modified_inplace);

    l3obj*  curenv = startingenv;
    long n = dotlist.size();
    std::vector<char*>::iterator pathit = dotlist.begin();

    if (n < 1) {
        std::cout << "assignment error: could not resolve left-hand-side element in expression '"<< curcmd << "'.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }

    if (n > 1) {
        long countdown = n-1;
        while(countdown >= 1) {
            
            // old style
            // curenv = resolve_core(curenv, *pathit, curcmd,deref,UNFOUND_THROW_TOP, innermostref);
            // new style call:
            curenv = resolve_core(*pathit,
                                  curenv,
                                  deref,
                                  innermostref,
                                  penvstack,
                                  cc(),
                                  noresolve_do //UNFOUND_THROW_TOP
                                  );
	    if (!curenv) return 0;
            if (penvstack) { penvstack->push_back(curenv); }
            ++pathit;
            --countdown;
        }
        // gotta set innermostref if there is an existing ref, but also
        // don't stress if we aren't overwriting an old reference. (i.e. do UNFOUND_RETURN_ZERO here)
        // old style: resolve_core(curenv, *pathit, curcmd,deref,UNFOUND_RETURN_ZERO, innermostref);
        // new style:
        target = resolve_core(*pathit, curenv, deref, innermostref, penvstack, cc(), UNFOUND_RETURN_ZERO);
        if (penvstack) { penvstack->push_back(target); }

    } else {
        assert(n == 1);

        // gotta set innermostref, but don't stress if we aren't overwriting an old reference. (don't do UNFOUND_THROW_TOP)

        target = resolve_static_then_dynamic(*pathit, curenv, deref, innermostref, penvstack, curcmd, UNFOUND_RETURN_ZERO);
        if (penvstack) { penvstack->push_back(target); }
    }

    // INVAR: *pathit is the last element requested.
    if (name_to_insert) {
        name_to_insert->init(*pathit);
    }
    LIVEO(curenv); // from when we used to return curenv. but still a valid check.

    if (target) { // 0 is also allowed in target, indicating "not found"
        LIVEO(target);
    }

    return target;
}



void dotlist_dump(std::vector<char*>* dotlist) {
  assert(dotlist);
  printf("dotlist_dump(%p):\n",dotlist);
  std::vector<char*>::iterator  be = dotlist->begin();
  std::vector<char*>::iterator  en = dotlist->end();
  long i = 0;
  for( ; be != en; ++be) {
    printf("dotlist[%01ld]:  '%s'\n",i,*be);
    ++i;
  }
}

void gen_dotlist(std::vector<char*>* dotlist, l3path* path) {
    assert(path);
    assert(dotlist);
    path->trim();
    
    char* endstr = &(path->buf[0]);
    char* start = endstr;
    while(1) {
      if (*endstr=='\0' || *endstr==' ' || *endstr=='\t' || *endstr=='\n') {
          *endstr = '\0';
          dotlist->push_back(start);

          DV(dotlist_dump(dotlist));
          return;
      }
      if (*endstr=='.') {
    *endstr = '\0';
    dotlist->push_back(start);
    start = endstr+1;
      }
      ++endstr;
    }

} // end gen_dotlist



 void throw_to_toplevel() {

 }



#if 0 // old 

Tag* glob {
  assert(global_tag_stack.size());
  return global_tag_stack.front_val();
}

// use the macro deftag_push() instead of calling this directly
void indirect_push_defptag_private_to_macro(Tag* newfront) {
  global_tag_stack.push_front(newfront);
  //??? should we? no...because already done by deftag_push(): global_tag_stack_loc.push_front(STD_STRING_WHERE);
}




void env_pop_iftop(l3obj* expected_front) {
  if (global_env_stack.front_val() == expected_front) {
    global_env_stack.pop_front();
  } else {
      assert(0);
  }
}


// returns number of env popped off the global_env_stack
//
long private_pop_to_env(l3obj* expected_front, bool removeit, bool assert_notfound) {
        l3obj* top = 0;
        long npop = 0;

        top = global_env_stack.front_val();
        if (0 == top) {
            if (!assert_notfound) return npop;
            printf("terp internal error: expected env %p to be on global_env_stack, but was never found.\n",expected_front);
            assert(0);
        }

        if (top == expected_front) {
            if (removeit) {
                global_env_stack.pop_front();
                ++npop;
            }
            return npop;
        }

        // scan the stack for expected_front, keeping stack intact in case expected_front is
        // not present, in which case assert(0).
        long howdeep_max = global_env_stack.size();
        long howdeep = 0;


        //        EnvStack_it be = global_env_stack.begin();
        //        EnvStack_it en = global_env_stack.end();
        //        for (; be != en; ++be, ++howdeep) {

        for (EnvStack_it be(&global_env_stack._stk); !be.at_end(); ++be, ++howdeep) {

            assert( howdeep < howdeep_max);

            if (expected_front == *be) {                
                // unwind the stack from be2 to be
                while (1) {
                    top = global_env_stack.front_val();

                    if (top != expected_front) {
                        global_env_stack.pop_front();
                        ++npop;
                    } else break;
                }
                DV(printf("pop_to_env popped %ld envs off the global_env_stack to restore to %p\n",howdeep, expected_front); );
                if (removeit) {
                    global_env_stack.pop_front();
                    ++npop;
                }
                return npop;
            } // end if expected_front == *be

        } // end for be

        if (!assert_notfound) return npop;
        printf("terp internal error: expected env %p to be on global_env_stack, but was never found.\n",expected_front);
        assert(0);

        return npop;
}



// now should be called only by: bal_pop_to_tag(), so both env and tag get popped by same amount.
//
// returns npop, the number of tags popped off the stack
//
long private_pop_to_tag(Tag* expected_front, bool removeit, bool assert_notfound) {
        Tag* top = 0;
        long npop = 0;

        top = global_tag_stack.front_val();
        if (0 == top) {
            if (!assert_notfound) return npop;
            printf("terp internal error: expected tag %p to be on global_tag_stack, but was never found.\n",expected_front);
            assert(0);
        }

        if (top == expected_front) {
            if (removeit) {
                global_tag_stack.pop_front();
                global_tag_stack_loc.pop_front();
                global_env_stack.pop_front();
                ++npop;
            }
            return npop;
        }

        // scan the stack for expected_front, keeping stack intact in case expected_front is
        // not present, in which case assert(0).

        long howdeep_max = global_tag_stack.size();
        long howdeep = 0;

        //        TagStack_it be = global_tag_stack.begin();
        //        TagStack_it en = global_tag_stack.end();
        //        for (; be != en; ++be, ++howdeep) {

        for (TagStack_it be(&global_tag_stack); !be.at_end(); ++be, ++howdeep) {

            assert( howdeep < howdeep_max);

            if (expected_front == *be) {                
                // unwind the stack from be2 to be
                while (1) {
                    top = global_tag_stack.front_val();

                    if (top != expected_front) {
                        global_tag_stack.pop_front();
                        global_tag_stack_loc.pop_front();
                        global_env_stack.pop_front();
                        ++npop;
                    } else break;

                }
                DV(printf("pop_to_tag popped %ld tags off the global_tag_stack to restore to %p\n",howdeep, expected_front); );
                if (removeit) {
                    global_tag_stack.pop_front();
                    global_tag_stack_loc.pop_front();
                    global_env_stack.pop_front();
                    ++npop;
                }
                return npop;
            } // end if expected_front == *be

        } // end for be

        if (!assert_notfound) return npop;
        printf("terp internal error: expected tag %p to be on global_tag_stack, but was never found.\n",expected_front);
        assert(0);

        return npop;
}



long bal_pop_to_tag(Tag* expected_front, bool removeit, bool assert_notfound) {
    assert(global_tag_stack.size() == global_env_stack.size());     // try to find imbalance as soon as it happens...    
    long npop = private_pop_to_tag(expected_front, removeit,assert_notfound);
    assert(global_tag_stack.size() == global_env_stack.size());     // try to find imbalance as soon as it happens...

    return npop;
}


#endif // 0 for global env/tag stack stuff, eliminated.


/**
 * insert a variable reference into the dictionary : use add_alias_eno if you don't have an llref yet...
 */

struct llref;


l3obj* insert_private_to_add_alias(char* varname, llref* val, l3obj* d) {
    LIVEO(d);
    LIVEREF(val);

  //  unsigned long* ulp = (unsigned long*)&val;
  //  insert_into_hashtable_private_to_add_alias(d, varname, *ulp);

  insert_into_hashtable_private_to_add_alias_typed(d, varname, val);
  return d;
}

int feqs(double a, double b, double tol) {
  double diff = b-a;
  if (isnan(diff)) return 0;
  double res = ::fabs(diff);
  if (res <= tol) return 1;
  return 0;
}

//
// use the int return value here: 1 for true, 0 for false.
//
// evaluates obj, everthing else ignored.
//

//typedef enum { TNONE=0, TFAL=1, TTRU=2, TNIL=3, TNAN=4, TNAV=5 } LOGICAL_TYPE;

LOGICAL_TYPE get_logical_type(l3obj* obj) {

    if (   obj->_type == t_fal ) return TFAL;
    if (   obj->_type == t_tru ) return TTRU;
    if (   obj->_type == t_nil ) return TNIL;
    if (   obj->_type == t_nan ) return TNAN;
    if (   obj->_type == t_nav ) return TNAV;

    return TNONE;
}

int is_true(l3obj* obj, long recursion_level) {

    if (recursion_level > 20) {
        printf("error: infinite loop detected in is_true, recursion level was %ld. Aborting computation.\n",recursion_level);
        l3throw(XABORT_TO_TOPLEVEL);
    }
   
   l3obj* nobj = obj;
   l3obj* subobj = 0;
   bool testres = true; // for unrecognized types.

   if (nobj->_type == t_nil || nobj == gnil || nobj->_type == t_fal) {
       testres = false;
   } else if (nobj->_type == t_dou) {

     double dval = double_get(nobj,0);
     if (!feqs(dval,0,1e-6)) {
       testres = true;
     } else {
       testres = false;
     }
   } else if (nobj->_type == t_str) {
     l3path s;
     string_get(nobj,0, &s);
     if (*(s())) {
       testres = true;
     } else {
       testres = false;
     }
   } else if (nobj->_type == t_fun) {
     testres = true;
   } else if (nobj->_type == t_vvc) {
       long N = ptrvec_size(nobj);
       if (0==N) {
           testres = false; // ptrvec of size zero --> F
       } else {
           for (long i =0; i < N; ++i) {
               ptrvec_get(nobj,i,&subobj);
               if (!is_true(subobj,recursion_level+1)) {
                   testres=false;
                   break;
               } 
           } // end for loop
           testres = true; // since all member sub-objects were true
       }
   }

if (testres) return 1; // true

return 0; // false

}



L3METHOD(eval_if_expr)
{
    l3path exp_as_text(exp);
    if (arity < 2 || arity > 3) {
        printf("error: bad if expression '%s'. Use: (if <test> <thenclause>) or (if <test> <thenclause> <elseclause>); so 3 or 4 arguments. We saw %ld arguments.\n",exp_as_text(),arity);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    sexp_t* test_exp = ith_child(exp,0);
    sexp_t* then_exp = ith_child(exp,1);
    sexp_t* else_exp = 0;
    if (arity == 3) {
        else_exp = ith_child(exp,2);
    }
    
    //eval(0,-1,test_exp,env, retval, owner, curfo, t_boe, retown);
    // put owner for retval here b/c we don't care about the result of the test expr, once we know if its true or false.
    eval(0,-1,test_exp,env, retval, owner, curfo, t_boe, owner,ifp);
    
    l3obj* nobj = *retval;
    if (0==nobj) {
        printf("error in eval_if_expr: test expression returned 0x0 object...hmm. We expected gnil or gtrue.\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    int  testres = is_true(nobj,0);
    
    // now we can discard nobj
    *retval =0;
    
    if(!is_undeletable(nobj)) {
        generic_delete(nobj, L3STDARGS_OBJONLY);
    }
    
    if (testres) {
        eval(0,-1,then_exp,L3STDARGS_ENV);
        
    } else {
        if (else_exp) {
            //eval(0,-1,else_exp,L3STDARGS_ENV);
            do_progn(0,-1,else_exp, L3STDARGS_ENV); 
       }
    }
}    
L3END(eval_if_expr)


L3METHOD(evals_to_true)
{
    l3path exp_as_text(exp);
    if (arity < 3 || arity > 4) {
        printf("error: bad evals to  expression '%s'. wrong number of arguments.\n",exp_as_text());
        l3throw(XABORT_TO_TOPLEVEL);
    }

}
L3END(evals_to_true)



bool bool_bin_op_ne(double a, double b) { return a != b; }
bool bool_bin_op_eq(double a, double b) { return a == b; }

bool bool_bin_op_gt(double a, double b) { return a >  b; }
bool bool_bin_op_ge(double a, double b) { return a >= b; }

bool bool_bin_op_lt(double a, double b) { return a <  b; }
bool bool_bin_op_le(double a, double b) { return a <= b; }

bool bool_una_op_isnan (double a) { return isnan(a); }
bool bool_una_op_iszero(double a) { return (a == 0); }
bool bool_una_op_notzero(double a) { return (a != 0); }
bool bool_una_op_not(double a)    { return feqs(a,0,1e-6); }

double  dou_una_op_negativesign(double a) { return -a; }

#if 0 // replaced by binop and unaryop

// returns 0 if handled, -1 if not handled.
// call: if (0 == test_and_eval_bool_expr(0,-1,else_exp,env, retval, owner, curfo, etyp)) return 0;
L3METHOD(test_and_eval_bool_expr)

  if (arity < 2) return -1;
  if (arity > 3) return -1;

  sexp_t* op  = first_child(exp);
  char*   val = op->val;
  assert(val);

 bool_bin_op  binary =0;
 bool_una_op  unary  =0;

// binary operators
if (0 == strcmp(val,"!=")) { binary = &bool_bin_op_ne; }
else if (0 == strcmp(val,"eq")) { binary = &bool_bin_op_eq; }
else if (0 == strcmp(val,">")) { binary = &bool_bin_op_gt; }
else if (0 == strcmp(val,">=")) { binary = &bool_bin_op_ge; }
else if (0 == strcmp(val,"<")) { binary = &bool_bin_op_lt; }
else if (0 == strcmp(val,"<=")) { binary = &bool_bin_op_le; }

// unary operators
 else if (0 == strcmp(val,"isnan")) { unary = &bool_una_op_isnan; }
else if (0 == strcmp(val,"iszero")) { unary = &bool_una_op_iszero; }
else if (0 == strcmp(val,"notzero")) { unary = &bool_una_op_notzero; }
else if (0 == strcmp(val,"not")) { unary = &bool_una_op_not; }

if (!binary && !unary ) return 1;

if (binary) {
  sexp_t* s1  = ith_child(exp,1);
  sexp_t* s2  = ith_child(exp,2);
  double d1 =0;
  double d2 =0;
  *retval = gnil; // default 
  bool boolres = false;

  l3obj* arg1 = 0; // make_new_double_obj(NAN,owner,"eval_bool_expr_arg1_value");
  l3obj* arg2 = 0; // make_new_double_obj(NAN,owner,"eval_bool_expr_arg2_value");
  
  // since we don't care about using the ultimate *retval (arg1, arg2), pass in owner for retown here.
   XTRY
      case XCODE:
         eval(obj,-1, s1, env, &arg1, owner, curfo, t_dou, owner);
         eval(obj,-1, s2, env, &arg2, owner, curfo, t_dou, owner);

         d1 = double_get(arg1,0);
         d2 = double_get(arg2,0);
         boolres = binary(d1,d2);

         if (boolres) {
             *retval = gtrue;
         } else {
             *retval = gnil;
         }

         break;
      case XFINALLY:
          if (arg1) { generic_delete(arg1, L3STDARGS_OBJONLY); } 
          if (arg2) { generic_delete(arg2, L3STDARGS_OBJONLY); } 

         break;
   XENDX  

  return 0; // handled.
 }

if (unary) {

  sexp_t* s1  = ith_child(exp,1);
  
  l3obj* arg1 = 0; // make_new_double_obj(NAN,owner,"eval_bool_expr_arg1_value");
  double d1 = 0;
  bool boolres = false;          

   XTRY
      case XCODE:  
          eval(obj,-1, s1, env, &arg1, owner, curfo, t_dou, owner);

          if (unary == &bool_una_op_not) {
              if (arg1->_type == t_tru) {
                  *retval = gnil;
                  break;
                  
              } else if (arg1->_type == t_fal) {
                  *retval = gtrue;
                  break;
                  
              }
          }
          
          d1 = double_get(arg1,0);
          boolres = unary(d1);
          
          if (boolres) {
              *retval = gtrue;
          } else {
              *retval = gnil;
          }

         break;
      case XFINALLY:
          if (arg1) { generic_delete(arg1, L3STDARGS_OBJONLY); }
         break;
   XENDX  

   return 0; // handled.
 } // end if unary

return -1;

L3END(test_and_eval_bool_expr)
#endif // 0


L3METHOD(print_strings)

   if (arity < 1) {
      return 0; // fine to print nothing.
   }

   FILE* fh = stdout;
   if (obj) {
      fh = stderr;
   }

    // detect final teardown (retval == 0), and just print the string literals in that case
    if (0 == retval) {
        printf("alert: print_to_string() called during final shutdown (i.e. retval == 0), so not printing.\n");
        return 0;
    }

   to_string(L3STDARGS);

   l3obj* nobj = *retval;

   l3path s;
   if (nobj && nobj->_type == t_str) {
       string_get(nobj,0,&s);
       fprintf(fh,"%s",s());
   }
   generic_delete(nobj, L3STDARGS_OBJONLY);
   *retval = 0;
    
L3END(print_strings)


// NOT DONE!
L3METHOD(make_new_file_handle_obj)
    {
        l3path exp_as_text(exp);
        if (arity < 1 || arity > 2) {
            printf("error: bad file path in make_new_file_handle_obj: '%s'.\n",exp_as_text());
            l3throw(XABORT_TO_TOPLEVEL);
        }
        
        sexp_t* filepath  = first_child(exp);
        qqchar   val(filepath->val());
        
        qqchar   flags;
        if (arity == 2) {
            flags = ith_child(exp,1)->val();
        }
        
        l3path varname(0,"fd_");
        varname.pushq(val);
        
        l3obj* p = make_new_class(0,retown,"fd",varname());
        p->_type = t_flh;
        p->_parent_env = env; // lexical scope -- but maybe not what we want at the command line?
        
        //FILE* fh = fopen(val,flags);
        
        *retval = p;
    }
L3END(make_new_file_handle_obj)



l3obj*  make_new_literal(const qqchar& s, Tag* owner, const qqchar& varname) {
    l3obj* obj = make_new_string_obj(s, owner,varname);
    obj->_type = t_lit;
    return literal_set(obj,s);
}


l3obj*  literal_set(l3obj* obj, const qqchar& key) {
    return string_set(obj,0,key);
}


void    literal_get(l3obj* obj, l3path* val) {
    string_get(obj,0,val);
}


void    literal_print(l3obj* obj) {
    l3path s;
    literal_get(obj,&s);
    s.outln();
}



void  print_defun_body(l3obj* obj,const char* indent, stopset* stoppers) {

  if (!obj) return;
  assert(obj->_type == t_fun);
  l3path indent_more(indent);

  indent_more.pushf("%s","    ");

  if (gUglyDetails>0) {
      printf("%sfn %p (ser# %ld) '%s'",
             indent_more(),
             obj,obj->_ser,obj->_varname);
  } else {
      printf("fn");
  }
  defun* df = (defun*)obj->_pdb;

  
  switch(gVerboseFunction) {
  case 0:
      printf(".\n");
      defun_dump(*df, "", stoppers, PRINT_DEFUN_NOBODY);
      break;
  case 1:
      printf(":\n");
      defun_dump(*df, indent, stoppers, PRINT_DEFUN_ONELINE);
      break;
  case 2:
  case 3:
      defun_dump(*df, indent_more(),stoppers, PRETTY_PRINT_DEFUN);
      break;
  default:
      printf("internal error: gVerboseFunction out of range.\n");
      assert(0);
  }

  //  printf("%s     bound variables in this defun (declaration) env (local hashtable):\n", indent_more());
  dump_hash(obj, indent_more(),stoppers);

}

// tight, less verbose, function defn output
void  print_defun_short(l3obj* obj,const char* indent, stopset* stoppers) {

    if (!obj) return;
    assert(obj->_type == t_fun);
    printf("fn %p (ser# %ld)\n",obj,obj->_ser);
    //defun* df = (defun*)obj->_pdb;
    
    l3path indent_more(indent);
    indent_more.pushf("%s","    ");
    
    //defun_dump(*df, "",stoppers);
    dump_hash(obj, indent_more(),stoppers);
}


// return 0 if handled, -1 if not.
L3METHOD(eq_string_string)

   int retcode = 0;
   arity = 2; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;

   l3obj* string1 = 0;
   l3path sstring1;

   l3obj* string2 = 0;
   l3path sstring2;

   XTRY
       case XCODE:

           ptrvec_get(vv,0,&string1);
           if (string1->_type != t_str) {
             retcode = -1;
             break;
           }
           string_get(string1,0,&sstring1);

           ptrvec_get(vv,1,&string2);
           if (string2->_type != t_str) {
             retcode = -1;
             break;
           }

           string_get(string2,0,&sstring2);

           if (0==strcmp(sstring1(),sstring2())) {
             *retval = gtrue;
           } else {
             *retval = gnil;
           }

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);
       break;
   XENDX

   return retcode; // indiate we are done
L3END(eq_string_string)


bool obj_in_stopset(stopset* st, void* obj) {
   st->_en = st->_stoppers.end();
   st->_fi = st->_stoppers.find(obj);

   if (st->_fi != st->_en) return true;

   return false;
 }

 void stopset_push(stopset* st, void* o) {
   st->_stoppers.insert(o);
 }

 void stopset_clear(stopset* st) {
   st->_stoppers.clear();
 }

 void p(stopset* st) {
     if (st ==0 || (long)st == -1) return;

     // copy the stopset so while printing it doesn't change.
     stopset  b = *st;

     stopset_set_it en = b._stoppers.end();
     stopset_set_it be = b._stoppers.begin();

     long i = 0;
     long N = b._stoppers.size();
     tyse* cur = 0;
     for(; be != en; ++be) {
         cur = (tyse*)(*be);
         printf("  stopset %p :  [%02ld  of  %02ld] = (%p  ser# %ld  %s)\n",st, i, N, cur, cur->_ser, cur->_type);
     }
 }

////

void Tag::test_owned() {


    l3obj* o3 = 0;
    make_new_obj("dummytest3", "dummytest3", glob, 0, &o3);

    l3obj* o4 = 0;
    make_new_obj("dummytest4", "dummytest4", glob, 0, &o4);

    l3obj* o5 = 0;
    make_new_obj("dummytest5", "dummytest5", glob, 0, &o5);

    add(o3, "mytest3",3);
    add(o4, "mytest4",4);
    add(o5, "mytest5",5);

    long sz = owned_size();
    printf("sz is %ld\n",sz);

    long gotit3 = owns(o3);
    printf("gotit3 = %ld\n",gotit3);

    long gotit4 = owns(o4);
    printf("gotit = %ld\n",gotit4);

    llref* myref = find(o5);
    printf("myref = %p\n",myref);


    llref* emp = find(o4);
    printf("find(o4) --> = %p\n",emp);

    dump_owned("");

    // manual loop, first set, then get.

    l3obj* o6 = 0;
    make_new_obj("dummytest6", "dummytest6", glob, 0, &o6);
    add(o6, "mytest6",6);
    dump_owned("");

    erase(o6);
    dump_owned("");

    erase(o5);
    dump_owned("");

    erase(o3);
    dump_owned("");

    // should just have o4
    add(o6, "mytest6",6);
    dump_owned("");

    // and o6
    
    // what happens on bad erase...nothing, just returns quietly.
    erase(o3);
    dump_owned("");
    printf("find on non-existant returns...%p\n",find(o3));
    dump_owned("");
}



 // find the nearest common ancestor tag of a and b, so we can do lifetime promotion when there
 //  is an alias created.
 // create a hash table of nodes encountered when running from a to root.
 // run from b to root, and check each node encountered for presence in the hash table.
 //    return the first one found.
 //
Tag* compute_nca(Tag* a, Tag* b,    l3obj* env, Tag* owner) {
    FILE* ifp = 0;
    assert(a);
    assert(b);
    if (a == b) return a;

    Tag* globalroot = glob;

    if (a == globalroot) return a;
    if (b == globalroot) return b;

    Tag* ancesta = a;
    Tag* ancestb = b;

    // really only need a Judy1 array...but just use a map, since we
    // have it ready to go.
    l3obj* a2root = make_new_l3map(a, "nca_a_to_root_path");

    // traverse from b to a, filling in the map
    long avoid_inf_loop = 100;
    while (ancesta != globalroot && avoid_inf_loop > 0) {
        ins_l3map(a2root,(void*)ancesta,(void*)1);
        ancesta = ancesta->parent();
        --avoid_inf_loop;
    }

    if(0 == avoid_inf_loop) {
      printf("internal error: detected infinite loop in compute_nca(%p,%p):\n",a,b);

      //      printf("gdump:\n");
      //      gdump();

      //      printf("lsb:\n");
      //      lsb(0,L3STDARGS_OBJONLY);

      printf("...extended... internal error: detected infinite loop in compute_nca(%p,%p):\n",a,b);
      //      l3map_print(a2root, "", 0);
      //      printf("a:\n");
      //      p(a);
      //      printf("b:\n");
      //      p(b);
      assert(0);
      exit(1);
    }

    DV(printf("here  is a2root map:\n"); l3map_print(a2root,"",0););

    void* dummy = 0;
    while (ancestb != globalroot) {
        if (ele_in_l3map(a2root,(void*)ancestb,&dummy)) {
            assert(dummy == (void*)1);
            generic_delete(a2root, -1,0,  0,0,owner,  0,0,owner,ifp);
            return ancestb;
        }
        ancestb = ancestb->parent();
    }

    generic_delete(a2root, -1,0,  0,0,owner,  0,0,owner,ifp);
    return globalroot;
}

L3METHOD(nca)
{
   k_arg_op(obj,2,exp,L3STDARGS_ENV);

   l3path sexps(exp);
   l3obj* vv  = *retval;
   *retval = 0;

   l3obj* objA = 0;
   l3obj* objB = 0;
   Tag* tagA = 0;
   Tag* tagB = 0;
   Tag* nca = 0;
   l3obj* cap_of_nca = 0;

   XTRY
       case XCODE:

   ptrvec_get(vv,0,&objA);
   ptrvec_get(vv,1,&objB);

   // find the nca
   tagA = objA->_owner;
   tagB = objB->_owner;

   /* // actually, ignore forwarding, since we care about who *can* own 
      if (is_forwarded_tag(objA) && objA->_mytag) { tagA = _mytag; }
      if (is_forwarded_tag(objB) && objB->_mytag) { tagB = _mytag; }
   */

   if (objA->_mytag) { tagA = objA->_mytag; }
   if (objB->_mytag) { tagB = objB->_mytag; }

   nca = compute_nca(tagA, tagB, env, owner);

   // return it's captain
   cap_of_nca = nca->captain();
   if (0==cap_of_nca) {
       printf("internal error in nca: there was no captain for the nca tag; nca tag was (%p ser# %ld %s)",nca,nca->sn(),nca->myname());
       l3throw(XABORT_TO_TOPLEVEL);
   }

   if (vv == cap_of_nca) {
     // both were temp variables; anybody can own them
     // but don't delete the owner in this case...

     // hhmmmm....not done here
     assert(0);
     exit(1);
   }

   *retval = cap_of_nca;


   break;
   case XFINALLY:
       generic_delete(vv, L3STDARGS_OBJONLY);   
   break;
   XENDX

}
L3END(nca)



// ============================================
// ============================================
//
// maps and functions on them
//
// ============================================
// ============================================

// map based on judyL arrays
l3obj* make_new_l3map(Tag* owner, const char* varname) {

  l3obj* obj = make_new_class(0, owner,"map",varname);
  obj->_type = t_map;
  return obj;
}

// returns 1 if in the map, setting *val
// otherwise returns 0
int ele_in_l3map(l3obj* map, void* key, void** val) {
    assert(map->_type == t_map);

    PWord_t   PValue = 0;
    Word_t* pkey = (Word_t*)&key;
    JLG(PValue,map->_judyL,(*pkey));
    
    if (!PValue) return 0;
    *val = *((void**) (PValue));
    return 1;
    
}

void ins_l3map(l3obj* map, void* key, void* val) {

    PWord_t   PValue = 0;
    Word_t* pkey = (Word_t*)&key;
    JLI(PValue, map->_judyL, (*pkey));
    *((void**)PValue) = val;
}

void del_l3map(l3obj* map, void* key) {
    Word_t* pkey = (Word_t*)&key;
    int Rc_int=0;
    JLD( Rc_int, map->_judyL, (*pkey));
    assert(Rc_int != JERR);
}

long l3map_size(l3obj* obj) {
    Word_t    array_size;
    JLC(array_size, obj->_judyL, 0, -1);
    return (long)array_size;
}

L3METHOD(test_l3map)

    l3obj* m = make_new_l3map(glob,"test_l3map");
    printf("should be empty:\n");
    l3map_print(m,"",0);

    ins_l3map(m,(void*)1,(void*)2);
    ins_l3map(m,(void*)3,(void*)4);
    ins_l3map(m,(void*)5,(void*)6);

    printf("should have stuff in it:\n");
    l3map_print(m,"",0);

    l3map_clear(m);

    printf("should be empty:\n");
    l3map_print(m,"",0);

    ins_l3map(m,(void*)7,(void*)8);
    ins_l3map(m,(void*)9,(void*)10);
    ins_l3map(m,(void*)11,(void*)12);

    printf("should have stuff:\n");
    l3map_print(m,"",0);

    l3map_clear(m);

    printf("should be empty:\n");
    l3map_print(m,"",0);

L3END(test_l3map)


void l3map_print(l3obj* l3map, const char* indent, stopset* stoppers) {
     assert(l3map);
  
     l3path indent_more(indent);
     indent_more.pushf("%s","     ");

     long N = l3map_size(l3map); 
     printf("%s%p : (l3map of size %ld): \n", indent, l3map, N);
  
     // a second way, to confirm:
       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
       void* pvindex = 0;

       JLF(PValue, l3map->_judyL, Index);
       l3path varname;
       Tag* tg = 0;

       while (PValue != NULL)
       {
           //           printf("%sl3map content [%02lu] =  '%p' ", indent, Index, (l3obj*)(*PValue));
           pvindex = *((void**)(&Index));
           //printf("%sl3map[%p ser# %ld '%s'] = %p\n", indent_more(), pvindex, ((l3obj*)pvindex)->_ser, ((l3obj*)pvindex)->_varname, *((void**)PValue));

           //printf("%sl3map[%p] = %p\n", indent_more(), pvindex, *((void**)PValue));

           tg=((Tag*)(pvindex));
           printf("%sl3map[%p _sn_tag# %ld '%s'] = %p\n", indent_more(), pvindex, tg->sn(), tg->myname(), *((void**)PValue));
           JLN(PValue, l3map->_judyL, Index);
       }

}

void l3map_clear(l3obj* l3map) {
  assert(l3map);

   long  Rc = 0;

   if (l3map->_judyL) {
       JLFA(Rc,  (l3map->_judyL));
   }

#if 0
   if (l3map->_judyS) {
       JSLFA(Rc,  (l3map->_judyS)); // this was JLFA
   }
#endif
   l3map->_judyS->clear();

   assert(l3map_size(l3map)==0);
}


// promote ownership to nca mechanism / stubb

void promote_ownership_what_from_to(l3obj* what, Tag* fromtag, Tag* totag) {

}

void bye(volatile l3obj* delme, Tag* owner) {
    assert(delme);
    generic_delete((l3obj*)delme,  -1,0,  0,0,owner,  0,0,owner,0);
}

// 
// obj = delete this if arity is -1
//
// else take target from sexp
//
// return the env that we deleted from in *retval, (for delete and replace actions).
//
L3METHOD(hard_delete)
{
    l3obj* nobj = 0;
    l3path canonical_path_str;
    llref* innermostref = 0;
    l3path sexps(exp);
    qqchar tgt;

    l3obj* found_ref = 0;

    //default to failing to delete
    *retval = gnil;

    if (-1 == arity && obj != 0) {
        // take target from obj
        nobj = obj;

        // and exp->val point to the reference to be deleted too... say for x=F pointer to builtin (un ref-counted) value.
        tgt = exp->val();
        
        // confirms link is good. then don't use nobj when just removing an llref
        objlist_t* envpath=0;
        found_ref  = RESOLVE_REF(tgt,env,AUTO_DEREF_SYMBOLS,&innermostref, envpath, tgt, UNFOUND_RETURN_ZERO);

        if (!found_ref) {
            // this is okay, since the expression might have been not a literal but another expression
            // that produced a string or literal. Or it might be a reference which has already been previously deleted.
            *retval = gnil; return 0;
        } else {
            rm_alias(env,tgt);
        }
        
    } else {
        // find target using sexp and arity

        // look up argument, remove it from the env, and free it.
        if (arity != 1) {
            printf("hard_delete takes exactly one string arg: the variable to remove from the enclosing env.\n");
            l3throw(XABORT_TO_TOPLEVEL);
        }
    
        // don't eval!    l3obj* target = eval(exp->list->next,env);
        // why not? because we want to be able to delete the llref and not the pointed to object..
        tgt = exp->first_child()->val();
        
        // confirms link is good. then don't use nobj when just removing an llref
        objlist_t* envpath = 0;
        nobj = RESOLVE_REF(tgt,env,AUTO_DEREF_SYMBOLS,&innermostref, envpath, tgt, UNFOUND_RETURN_ZERO);
        
        if (!nobj) {
            std::cout << "error: hard_delete could not find target '" << tgt << "'.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }

        // how do we get the llref? when this say a "w = F" reference, and we just want to delete w.
        // check for this situation:
        if (!is_sysbuiltin(innermostref) && is_sysbuiltin(nobj) ) {
            // we have request to delete a user-made reference to a builtin object.
            // just delete the reference and exit cleanly.

            rm_alias(env,tgt);
            *retval = gtrue;
            return 0;
        }

#if 0 // temporarliy disable this check, until/if we put the auto pushing of stack tracking in place.

        // if we calling with it, don't do it.
        l3obj*  env_within_target = genvs.env_is_on_global_env_stack(nobj);
        Tag*    tag_within_target = sgt.tag_is_on_global_tag_stack(nobj->_mytag);
        if (env_within_target || tag_within_target) {
            std::cout << "error in hard_delete: object '" << tgt << "' is on the current env stack; cannot deleted an object from inside itself.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }
#endif

        // even more stringent check... target->_owner must be below our owner...so that progn can contain a rm, but not any deeper.
        Tag* lca = compute_nca(nobj->_owner, owner->parent(),  env,owner);
        if (lca != owner->parent()) {
            std::cout << "error in hard_delete: object '"<< tgt << "' was not owned by a descendant: rm can only delete descendants.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }




        // if sealed, don't do it.
        if (is_sealed(nobj) && !global_terp_final_teardown_started) {
            std::cout << "error in hard_delete: object '"<< tgt << "' is sealed and cannot be deleted.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }

        // sysbuiltin objects: do not remove.
        if (is_sysbuiltin(nobj) && !global_terp_final_teardown_started) {
            std::cout << "error in hard_delete: object '"<< tgt << "' is a sysbuiltin object and cannot be deleted.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }
        
        // same for undeletable; these can be added to but not deleted.
        if (is_undeletable(nobj) && !global_terp_final_teardown_started) {
            std::cout << "error in hard_delete: object '"<< tgt << "' is marked undeletable.\n";
            l3throw(XABORT_TO_TOPLEVEL);
        }


    } // end target selection

    long tbd_sn = nobj->_ser;

    // sanity check not already deleted (say by a recursive earlier delete).

    if (0== tbd_sn || 0==nobj->_owner) {
        l3path msg(0,"error in hard_delete: object '");
        if (!obj) {
            msg.pushq(tgt);
            msg.pushf("' appears to be already gone, in expression '%s'.\n",sexps());
        } else {
            msg.pushf("%p ser# %ld' appears to be already gone.\n",nobj, tbd_sn);
        }
        printf("%s",msg());
        l3throw(XABORT_TO_TOPLEVEL);
    }

    LIVEO(nobj);
    BOOL builtin = is_sysbuiltin(nobj);
    if (builtin) {
        *retval = gtrue;
        return 0;
    }

    // hard delete of ownership. delete the object and all of it's references.
    assert(nobj);
    assert(nobj->_owner);
    l3obj* fakeret = 0; // have to give a retval...or eval wont work.
    
    if ((nobj->_mytag) && pred_is_captag(nobj->_mytag,nobj)) { 
        // i.e. nobj->_mytag->captain() == nobj; // definition of captag.
        delete_captag(nobj,-1,0,env, &fakeret,nobj->_mytag,0,0,nobj->_mytag,ifp);
    } else {
        if (!is_sysbuiltin(nobj)) {
           generic_delete(nobj, L3STDARGS_OBJONLY);
        }
    }
    // successful delete if we got here.
    *retval = gtrue;

    // accessing already freed memory, not good: but turn off inside the function.
    if (!builtin) { serialfactory->loose_ends_check(nobj,tbd_sn); }

}
L3END(hard_delete)



L3METHOD(softrm)

    // look up argument, remove it from the env, and free it.

    if (arity != 1) {
        printf("softrm takes exactly one string arg: the variable to remove from the enclosing env.\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    qqchar tgt(exp->first_child()->val());
    llref* innermostref = 0;

    l3path expstring(exp);
    l3obj* nobj = resolve_dotted_id(tgt,env,AUTO_DEREF_SYMBOLS,&innermostref,0,expstring(),UNFOUND_RETURN_ZERO);

    if (!nobj) {
        std::cout << "error: softrm could not find target '" << tgt << "'.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    rm_alias(env,tgt);

L3END(softrm)


L3KARG(setstop, 1)

   l3obj* sn_to_stopon  = 0;
   ptrvec_get(vv,0,&sn_to_stopon);

   if (sn_to_stopon->_type != t_dou) {
      printf("error: arg to setstop was not numeric. setstop takes exactly one positive integer numeric arg: the serialnum_f::last_serialnum number to stop on (set a breakpoint in serialfac.cpp to utilize).\n");
      l3throw(XABORT_TO_TOPLEVEL);     
   }
   double d = double_get(sn_to_stopon,0);

   if (d < 1 || round(d) != d) {
      printf("error: arg to setstop was not a positive integer. setstop takes exactly one integer numeric arg: the serialnum_f::last_serialnum number to stop on (set a breakpoint in serialfac.cpp to utilize).\n");
      l3throw(XABORT_TO_TOPLEVEL);           
   }

   long  sn = (long)d;
   serialfactory->halt_on_sn(sn);
   long last = serialfactory->get_last_issued_sn();
   long stopline = serialfactory->get_stopline();

   printf("setstop: halt serialnumber set to %ld.  Issue debugger break serialfac.cpp:%ld command to stop at that allocation. Current last allocation: %ld.\n",
       sn,
          stopline ? stopline : 83,
       last
       );
   if (last >= sn) {
     printf("setstop: warning: already at or past last_serialnum... this stop setting will have no effect.\n");
   }

   generic_delete(vv, L3STDARGS_OBJONLY);

L3END(setstop)

L3METHOD(getstop)
  long sn = serialfactory->get_halt_on_sn();
  long last = serialfactory->get_last_issued_sn();
  if (sn == 0) {
    printf("getstop: serialfactor.get_halt_on_sn() returned %ld. No stopping will occur (feature is off).\n",sn);
  } else {
    printf("getstop: serialfactor.get_halt_on_sn() returned %ld.   Last issued sn: %ld\n",sn,last);
    if (last  >= sn) {
      printf("getstop: warning: already past stop point!\n");
    }
  }


L3END(getstop)

enum LOGICALOP { LOG_NONE=0, EQ=1, NEQ=2};




L3METHOD(logical_binop)
{
    arity = num_children(exp);
    if (arity < 1) return -1; // not handled.

    l3obj* vv  = 0;
    any_k_arg_op(0,-1,exp,env,&vv,owner,curfo,etyp,retown,ifp);
    l3path sexps(exp);

   LOGICALOP logop = LOG_NONE;

    sexp_t* op  = exp->headnode();
    if (!op) {
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
       return -1;
    }
    qqchar   val(op->val());

    if (0 == val.strcmp("!=")) { logop = NEQ; }
    else if (0 == val.strcmp("eq")) { logop = EQ; }
    else if (0 == val.strcmp("neq")) { logop = NEQ; }
    else {
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        return -1; // not handled
    }

    l3obj* arg1 = 0;
    l3obj* arg2 = 0;
    ptrvec_get(vv,0,&arg1);
    ptrvec_get(vv,1,&arg2);

    LOGICAL_TYPE t1 = get_logical_type(arg1);
    LOGICAL_TYPE t2 = get_logical_type(arg2);

    if (TNONE == t1 || TNONE == t2) {
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        return -1; // not handled
    }

    // valid possible types: t_fal, t_tru, t_nil, t_nan, t_nav, corresponding to:
    //
    //  typedef enum { NONE=0, TFAL=1, TTRU=2, TNIL=3, TNAN=4, TNAV=5 } LOGICAL_TYPE;


    if (t1 == TNIL || t1 == TNAN || t1 == TNAV) {
        *retval = gna;
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        return 0;
    }
    if (t2 == TNIL || t2 == TNAN || t2 == TNAV) {
        *retval = gna;
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        return 0;
    }

    // t1 and t2 must be either TFAL or TRUE

    switch(logop) {

     case(EQ): {
         if (t1 == t2) {
             *retval = gtrue;
         } else {
             *retval = gnil;
         }
     }
         break;
     case(NEQ): {
         if (t1 == t2) {
             *retval = gnil;
         } else {
             *retval = gtrue;
         }
     }
         break;
      
  default:
      assert(0);
  }
  

  if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }

}
L3END(logicalop)


typedef enum { BNONE=0, BEQ=1, BNEQ=2, BGT=3, BLT=4, BGE=5, BLE=6 } BINOP;

l3obj*  compare_obj_and_logical(BINOP bop, l3obj* obj, LOGICAL_TYPE logi) {

    assert(get_logical_type(obj) == TNONE);
    assert(logi != TNONE);
    assert(bop  != BNONE);

    switch(bop) {
    case    BGT: 
    case    BLT: 
    case    BGE:
    case    BLE:
        return gnil; // don't apply to objs and logicals.
        break;
    case    BEQ:
        {
            switch(logi) {
            case TTRU: 
                return gtrue; // obj == T, yes.
            case TFAL:
            case TNIL: 
            case TNAN:
            case TNAV:
                return gnil; // obj == F, no
                break;
            case TNONE:
                assert(0);
            }
        }
        break;

    case    BNEQ:
        {
            switch(logi) {
            case TTRU: 
                return gnil; // obj != T, no.
            case TFAL:
            case TNIL: 
            case TNAN:
            case TNAV:
                return gtrue; // obj != F, yes.
                break;
            case TNONE:
                assert(0);
            }
        }
        break;
        
    case    BNONE:
    default:
        assert(0);
    }

    return gnil;
}

// binop: redo binary ops using tmp_tag/tmp_cap
// to replace old: L3METHOD(test_and_eval_bool_expr)
//
// returns 0 if handled, -1 if not handled.
//
L3METHOD_TMPCAPTAG(binop)

    if (arity < 2) return -1;
    if (arity > 2) return -1;
    
    sexp_t* op  = exp->headnode();
    bool_bin_op  binary =0;
    BINOP bop = BNONE;

    if (!op) {

       if      (exp->_ty == m_lth) { binary = &bool_bin_op_lt; bop=BLT; }
       else if (exp->_ty == m_gth) { binary = &bool_bin_op_gt; bop=BGT; }
       else if (exp->_ty == m_lte) { binary = &bool_bin_op_le; bop=BLE; }
       else if (exp->_ty == m_gte) { binary = &bool_bin_op_ge; bop=BGE; }
       else if (exp->_ty == m_neq) { binary = &bool_bin_op_ne; bop=BNEQ; }
       else if (exp->_ty == m_eql || exp->_ty == m_eqe) { binary = &bool_bin_op_eq; bop=BEQ; }
       else
       return -1;  // not handled.
    } else {
        qqchar   val(op->val());

        // binary operators
             if (0 == val.strcmp("!=") || 0 == val.strcmp("neq")) { binary = &bool_bin_op_ne; bop=BNEQ; }
        else if (0 == val.strcmp("eq")) { binary = &bool_bin_op_eq; bop=BEQ; }
        else if (0 == val.strcmp(">"))  { binary = &bool_bin_op_gt; bop=BGT; }
        else if (0 == val.strcmp(">=")) { binary = &bool_bin_op_ge; bop=BGE; }
        else if (0 == val.strcmp("<"))  { binary = &bool_bin_op_lt; bop=BLT; }
        else if (0 == val.strcmp("<=")) { binary = &bool_bin_op_le; bop=BLE; }
        else return -1; // not handled
    }    

    sexp_t* s1  = ith_child(exp,0);
    sexp_t* s2  = ith_child(exp,1);
    double d1 =0;
    double d2 =0;
    *retval = gnil; // default 
    bool boolres = false;
    
    LOGICAL_TYPE logical1 = TNONE;
    LOGICAL_TYPE logical2 = TNONE;
    
    l3obj* arg1 = 0; // make_new_double_obj(NAN,owner,"eval_bool_expr_arg1_value");
    l3obj* arg2 = 0; // make_new_double_obj(NAN,owner,"eval_bool_expr_arg2_value");
    
    L3TRY_TMPCAPTAG(binop, 0)
        
        eval(obj,-1, s1, env, &arg1, (Tag*)tmp_tag, curfo, t_dou, (Tag*)tmp_tag,ifp);
        eval(obj,-1, s2, env, &arg2, (Tag*)tmp_tag, curfo, t_dou, (Tag*)tmp_tag,ifp);
    
        // handle comparison between objects and F / T / nil / NA / NAN 
        //typedef enum { TNONE=0, TFAL=1, TTRU=2, TNIL=3, TNAN=4, TNAV=5 } LOGICAL_TYPE;

        logical1 = get_logical_type(arg1);
        logical2 = get_logical_type(arg2);
        
        // a t_obj is never t_nav or t_nan or t_nil or t_tru or t_fal, so we know this is false, unless we are comparing to false.
        if ( arg1->_type == t_obj && logical2 != TNONE ) {
            *retval = compare_obj_and_logical(bop, arg1, logical2);
            break;
        }

        if (arg2->_type == t_obj && logical1 != TNONE) {
            *retval = compare_obj_and_logical(bop, arg2, logical1);
            break;
        }
        
         if (arg1->_type == t_obj && arg2->_type == t_obj) {
             // ideally we would dispatch on a type-overloaded equality/binop method, but for now...
             // we can handle eq and != for objects by identity check.

             if (binary == bool_bin_op_ne) {
                 if (arg1 != arg2) { *retval = gtrue; } else { *retval = gnil; }
                 // l3rc = 0; // by default, so no need to set it now
                 break;

             } else if (binary == bool_bin_op_eq) {
                 if (arg1 == arg2) { *retval = gtrue; } else { *retval = gnil; }
                 // l3rc = 0; // by default, so no need to set it now
                 break;
                 
             } else {
                 l3rc = -1;
                 break; // let cleanup happen
             }
         }


         if (arg1->_type != t_dou || arg2->_type != t_dou) {
             l3rc = -1;
             break; // let cleanup happen
         }

         d1 = double_get(arg1,0);
         d2 = double_get(arg2,0);
         boolres = binary(d1,d2);

         if (boolres) {
             *retval = gtrue;
         } else {
             *retval = gnil;
         }

L3END_CATCH_TMPCAPTAG(binop)


// for boolean unary ops : not, isnan, iszero, notzero
L3METHOD_TMPCAPTAG(unaryop)

  if (arity != 1) return -1;

  qqchar val;
  if (exp->_headnode) {
     val = exp->headval();
  } else {
     val = exp->val();
  }

  bool_una_op  unary  =0;

  // unary operators

  if (0 == val.strcmp("not") || 0 == val.strcmp("!"))   { unary = &bool_una_op_not; }
  else if (0 == val.strcmp("isnan")) { unary = &bool_una_op_isnan; }
  else if (0 == val.strcmp("iszero")) { unary = &bool_una_op_iszero; }
  else if (0 == val.strcmp("notzero")) { unary = &bool_una_op_notzero; }
  else return -1; // not handled

  sexp_t* s1  = exp->first_child();
  double d1 =0;
  *retval = gnil; // default 
  bool boolres = false;

  l3obj* arg1 = 0;

  L3TRY_TMPCAPTAG(unaryop, 0)

          eval(obj,-1, s1, env, &arg1, (Tag*)tmp_tag, curfo, t_dou, (Tag*)tmp_tag,ifp);

          if (unary == &bool_una_op_not) {
              if (is_true(arg1,0)) {
                  *retval = gnil;
              } else {
                  *retval = gtrue;
              }
          }
          else {
              d1 = double_get(arg1,0);
              boolres = unary(d1);
          
              if (boolres) {
                  *retval = gtrue;
              } else {
                  *retval = gnil;
              }
          }

L3END_CATCH_TMPCAPTAG(unaryop)



// for double unary ops : -
L3METHOD_TMPCAPTAG(double_unaryop)

    assert(exp);
    if (arity != 1) return -1;
    
    qqchar val;
    if (exp->_headnode) {
        val = exp->headval();
    } else {
        val = exp->val();
    }
    
    dou_una_op  unary  =0;
    
    // unary operators
    
    if (0 == val.strcmp("-"))        { unary = &dou_una_op_negativesign; }
    else if (0 == val.strcmp("~"))   { unary = &dou_una_op_negativesign; }
    else return -1; // not handled
    
    sexp_t* s1  = exp->first_child();
    double d1 =0;
    *retval = gnil; // default 
    
    l3obj* arg1 = 0;
    l3obj* negated = 0;
    long   sz = 0;

    L3TRY_TMPCAPTAG(unaryop, 0)

        eval(obj,-1, s1, env, &arg1, (Tag*)tmp_tag, curfo, t_dou, (Tag*)tmp_tag,ifp);

        if (arg1->_type != t_dou) {
            l3path sexps(exp);
            printf("error: negation applied to non double value, in '%s'.\n",sexps());
            l3throw(XABORT_TO_TOPLEVEL);
        }

        sz = double_size(arg1);
        *retval = negated = make_new_double_obj(NAN,retown, "double_unaryop");

        for (long i = 0; i < sz; ++i) {
            d1 = double_get(arg1,i);
            double_set(negated, i, -d1); // here is the actual negation.
        }


L3END_CATCH_TMPCAPTAG(double_unaryop)


L3KARG(plusplus_minusminus,1)
{
    l3obj* changeme = 0;
    ptrvec_get(vv,0,&changeme);    
    l3path v(exp->_val);

    if (!changeme || changeme->_type != t_dou) {
        printf("error: argument to '%s' was not a double type, in expression '%s'.\n",
               v(), sexps());
        if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
        l3throw(XABORT_TO_TOPLEVEL);
    }

    long sz = double_size(changeme);
    double d1=0;
    for (long i = 0; i < sz; ++i) {
        d1 = double_get(changeme,i);
        if (exp->_ty == m_ppl) {

            double_set(changeme, i, d1+1); // ++

        } else if (exp->_ty == m_mmn) {

            double_set(changeme, i, d1-1); // --

        } else {
            printf("internal error in plusplus_minusminus: "
                   "'%s' did not have type m_mmn (--) or m_ppl (++), in expression '%s'\n",
                   v(), sexps());
            if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    *retval = changeme;
    if (vv) { generic_delete(vv,L3STDARGS_OBJONLY); }        
}
L3END(plusplus_minusminus)


// brute force, sweep everything, set it to given value and recurse unless already set to this value.
//
void dfs_reset_all_been_here_to_thisval(Tag* me, long val) {

    Tag* visitme = (Tag*)me;

    if (visitme->dfs_been_here() == val) {
       return;
    }
    visitme->dfs_been_here_set(val);

    long N = visitme->subtag_stack.size();
    if (N) {

        ustaq<Tag>::ustaq_it it(&(visitme->subtag_stack));
        Tag* childtag = 0;
        for( ; !it.at_end(); ++it) {

            childtag = *it;
            assert(childtag);
            dfs_reset_all_been_here_to_thisval(childtag,val);
        }

#if 0
        Tag::itsit cur = visitme->subtag_stack.begin();
        Tag::itsit nex = cur;

        for (long i =0; i < N; i++) {
        
            nex = cur;
            ++nex;
            Tag* childtag = (Tag*)(*cur);
            dfs_reset_all_been_here_to_thisval(childtag,val);
            cur = nex;
        }
#endif

    }

#if 0
      l3obj* ptr = 0;
      Word_t * PValue;
      Word_t Index = 0;
      long i = 0;

      JLF(PValue, visitme->_owned, Index);
      while (PValue != NULL)
       {
           ptr = ((l3obj*)(Index));

           Tag* mytag = ptr->_mytag;
           if (mytag) {
               dfs_reset_all_been_here_to_thisval(mytag,val);
           }

           JLN(PValue, visitme->_owned, Index);
           ++i;
       }

   visitme->dfs_been_here_set(0);

#endif


}

// pass in a tag to be zeroed. Since various
// parts of the tree may be in 0 or 1 state, we first shift
// everything to -1, then to zero.
void zero_dfs_been(Tag* tag) {
   dfs_reset_all_been_here_to_thisval(tag,-1);
   dfs_reset_all_been_here_to_thisval(tag,0);
}


void judySLdump(void* judySL, const char* indent) {

    size_t       sz = 0;
    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, judySL, Index);       // get first string

    while (PValue != NULL)
    {
      llref* ele = *((llref**)(PValue));
      printf("%s%02ld:%s -> %p\n", indent, sz, Index, ele);

      JSLN(PValue, judySL, Index);   // get next string
      ++sz;
    }
}


long judySL_size(void* judySL) {
    Word_t    sz = 0;
    JLC(sz, judySL, 0, -1);
    return sz;
}



// get the llref for a reference
llref* llref_from_path(const char* path, l3obj* startingenv, l3obj** pfound) {

  llref* targetref = 0;
  l3obj* target = RESOLVE_REF((char*)path,startingenv,AUTO_DEREF_SYMBOLS, &targetref,0,(char*)path,UNFOUND_RETURN_ZERO);

  // only return l3obj* if it was actually found, and if it was requested by non-zero pfound.
  if (pfound && target) {
      *pfound = target;
  }

  return targetref;
}


///////////// Tag:: methods from autotag.h


 Tag::~Tag() {
   // we don't actually want to utilize C++ destructors, because they
   // can't be relied upon using our finally blocks and try/catch (plus
   // they are slow and can sometimes fail. but this is here for 
   // debugging.

   DV(printf("in ~Tag() C++ destructor for %p\n",this));

   tyse_tracker_del((tyse*) this);
 }


  long   Tag::dfs_been_here() { return _been_here; }
  void   Tag::dfs_been_here_set(long newval) { _been_here = newval; }

  void   Tag::recursively_been_here_set(long newval) {
      _been_here = newval;

      long stsz = subtag_stack.size();
      if (stsz == 0 ) return;

      Tag* childtag = 0;
      for(subtag_stack.it.restart(); !subtag_stack.it.at_end(); ++subtag_stack.it) {
          
          childtag = *subtag_stack.it;
          assert(childtag);
          childtag->recursively_been_here_set(newval);
      }
  }

  long   Tag::destruct_started() { return _destruct_started; }
  void   Tag::destruct_started_set() { _destruct_started = 1; }
  Tag*  Tag::parent() { return _parent; }


  Tag* Tag::reg(const char* key, Obj* val) {
     assert(_captain);

       assert(val);
       if (find(val)) {
           DV(printf("Tag::reg(key:'%s',val:%p) : %p already present in _owned: good,"
                     " we insist that it be owned before we can register "
                     "a name for it with reg().\n",key,val,val));
       } else {
           DV(printf("%p not already present.\n",val));
           printf("%p not already present: declining key '%s': I can only name those objects I own!\n",val,key);
           return 0;
       }

       return this;
  }


  // Require passing parent in upon construction. It should not be
  // settable later; therefore we don't define set_parent();
void  Tag::set_parent(Tag* newparent) {
      _parent = newparent;
    }



void Tag::add(l3obj* o, const char* varname, long sn) { 
     assert(o); 

    if (sn==0)        {  sn = o->_ser; }
    if (varname == 0) {  varname = o->_varname; }

    DV(printf("Tag(%p:myname(%s))::add(): I received ownership of %p  - ser# %ld - varname:'%s'.\n", this, myname(), o, sn, varname));

    l3obj* cap = captain();
  
    // if we don't have a captain yet, assume we must
    // be bootstrapping or generating a captag pair; so set
    // the captain to be the object.
    if (cap == 0) {
        captain_set(o);
        cap = o;
    }

    o->_owner = this;

    // r gets stored in the l3obj* -> llref* _owned JudyL map, as the *PValue range part.
    llref* r = llref_new((char*)varname,o, cap,0,this,0);

    PWord_t PValue;
    Word_t* pw = (Word_t*)&o;
    JLI(PValue, _owned, *pw);
    if(PValue == PJERR) {
        printf("Tag::add(): malloc failed on JLI");
        assert(0);
        exit(1);
    }
    // allow duplicate adds:    assert(*PValue == 0);
    *PValue = *((long unsigned int*) &r);

  }

bool Tag::owns(l3obj* o) {
      return find(o) != 0;
  }


  // new JudyL way
void Tag::dump_owned(const char* pre) {

      l3obj* cap = 0;
      long   capsn = 0;
      char*  capnm = 0;
      l3path capstring(0,"[captain: null]");

      cap = captain();
      if (cap) {
          capsn = cap->_ser;
          capnm = cap->_varname;
          capstring.reinit("[captain: %p ser# %ld %s]",cap,capsn,capnm);
      }

      // not so fast. We got lots of other stacks to check, not just owned.
      //
      //      if (owned_size() == 0 && subtag_stack.size() == 0) {
      //          printf("%sTag '%s' (ser# %ld) %p is empty. %s\n", pre, myname(), _sn_tag,this,capstring());
      //         return;
      //      }

      l3path indent(pre);
      l3path indent_more(indent);
      indent_more.pushf("     ");
      l3path indent_more_more(indent);
      indent_more_more.pushf("     ");
      l3obj* o = 0;

      long ownsz = owned_size();
      if (ownsz == 0) {
          printf("%sTag '%s' (ser# %ld) %s owns nothing.\n",pre, myname(), _sn_tag, capstring());
      } else {
         // own something
          printf("%sTag '%s' (ser# %ld) %s owns: \n",pre, myname(), _sn_tag, capstring());

         // new way:
         Word_t * PValue;
         Word_t Index = 0;
         long i = 0;
         llref* llr = 0;

         JLF(PValue, _owned, Index);
         while (PValue != NULL)
             {
                 o = (l3obj*)Index;
                 llr = *(llref**)PValue;
                 LIVEREF(llr);

                 printf("%s: %p (ser# %ld) <-> %s  (llref %p)\n",indent_more(), o, o->_ser, o->_varname, llr);
                 // o->_varname is same as serialfactory->ptr2key[o].c_str());

                 // need this to avoid crashes and the 3rd check to avoid inf loops 
                 if (o->_mytag                      // avoid crashes
                     &&  !is_forwarded_tag(o)       // allow T/F to printx
                     && !(o->_mytag == o->_owner))  // avoid inf loops on captags, where _mytag == _owner
                     {
                         o->_mytag->dump(indent_more_more()); 
                     }

                 JLN(PValue, _owned, Index);
                 ++i;
             }

      } // end if own something

      // show the stacks. Err, well, show the genstack for sexp atom debugging.
      //  the others could be added similarly if need be:
      if (genstack.size()) {
          printf("%sgenstack:\n",indent_more());
          genstack.dump(indent_more_more());
      } else {
          printf("%sgenstack: empty.\n",indent_more());
      }

      if (lnk_stack.size()) {
          printf("%slnk_stack:\n",indent_more());
          lnk_stack.dump(indent_more_more());
      } else {
          printf("%slnk_stack: empty.\n",indent_more());          
      }


      if (sexpstack.size()) {
          printf("%ssexpstack:\n",indent_more());

          print_ustaq_of_sexp_as_outline(sexpstack, indent_more());

#if 0
          //          sexpstack.dump_tyse(indent_more_more());
          int iii = 0;
          for (sexpstack.it.restart(); !sexpstack.it.at_end(); ++sexpstack.it, ++iii) {
              std::cout << indent_more() << " ->sexp<- " << iii << " ser# "<< (*sexpstack.it)->_ser
                        << ":  " << (*sexpstack.it)->_ty << " '" << (*sexpstack.it)->span() << "'\n";
          }
#endif

      } else {
          printf("%ssexpstack: empty.\n",indent_more());
      }
#if 0 // to verbose, skip atom and tyse stacks with all the sexp / atoms.
      if (atomstack.size()) {
          printf("%satomstack:\n",indent_more());
          atomstack.dump_char(indent_more_more());
      } else {
          printf("%satomstack: empty.\n",indent_more());
      }

      if (tyse_stack.size()) {
          printf("%styse_stack:\n",indent_more());
          tyse_stack.dump_tyse(indent_more_more());
      } else {
          printf("%styse_stack: empty.\n",indent_more());
      }
#endif
      long stsz = subtag_stack.size();
      if (stsz) {
         print_subtags(indent(),indent_more());
      }
  }


void Tag::print_subtags(char* indent, char* indent_more) {

    // print subtags too...

    long stsz = subtag_stack.size();
    if (stsz == 0 ) {
      printf("%ssubtags: none.\n",indent);
    } else {
        printf("%ssubtags: \n",indent);

        Tag* childtag = 0;
        for(subtag_stack.it.restart(); !subtag_stack.it.at_end(); ++subtag_stack.it) {

            childtag = *subtag_stack.it;
            assert(childtag);
            childtag->dump(indent_more);
        }

#if 0
        Tag* myst = 0;
        itsit be = subtag_stack.begin();
        itsit en = subtag_stack.end();
        for ( ; be != en; ++be ) {
            myst = (Tag*)(*be);
            myst->dump(indent_more);
        }
#endif

    } // end else stsz > 0

    dump_tbf(indent);

} // end print_subtags


llref* Tag::find(l3obj* obj) {
      PWord_t PValue;
      l3obj* Index = obj; // 
      JLF(PValue,_owned,(*((Word_t*)&Index)));
      if (0==PValue) return 0; // not found
      if (Index != obj) return 0; // not found
      llref* llr =  *(llref**)PValue;
      LIVEREF(llr);
      return llr;
  }

void Tag::erase(l3obj* o) {

      llref* found = find(o);
      if (!found) {
          printf("error in erase: object %p (ser# %ld) '%s' was not found in Tag %p (ser# %ld)\n",o,o->_ser,o->_varname,this,this->_sn_tag);
          l3throw(XABORT_TO_TOPLEVEL);
      }

      // gotta delete the llref's before we delete the objects that they point to and/or reside in as references!!!
      assert(found->_priority==1); // one of our invarients, the priority==1 is the ref stored in the _owned map.
      assert(found->_refowner);


       DV(print_llref(found,DQ("     ")));
       llref_del_ring(found,DELETE_PRIORITY_ONE_TOO,YES_DO_HASH_DELETE);

      int rc=0;
      Word_t* po = (Word_t*)(&o);
      JLD(rc,_owned, *po);
      if (rc == JERR) {
          printf("Tag::erase(): malloc failed in JLD.\n");
          assert(0);
          exit(1);
      }

  }

long   Tag::owned_size() {
    Word_t    array_size;
    JLC(array_size, _owned, 0, -1);
    return (long)array_size;
  }

void Tag::name(l3obj* o, const char* objname) {
      assert(find(o) != 0);      
  }

// generalized to handle cap-tag pairs
void Tag::generic_release_to(l3obj* o, Tag* new_owner) {

    assert(o);
    assert(new_owner);
    if(this == new_owner) return; /* we already own it. noop. */

    // cannot move sysbuiltin objects.
    if (is_sysbuiltin(o)) {
        return;
    }

    llref* pri = find(o);
    assert(pri);

    if ((o->_mytag) && pred_is_captag(o->_mytag,o)) { 
        // for cap-tags, we want to change the parent of the tag to be new_owner,
        // and leave the rest alone.
        if (o->_mytag->_parent == new_owner) return; // no-op

        // update the tag tree 
        o->_mytag->_parent->subtag_remove(o->_mytag);
        new_owner->subtag_push(o->_mytag);
        o->_mytag->_parent = new_owner;


    } else {
        // put this inside the add() : o->_owner = new_owner;
        new_owner->add(o);
        erase(o);
    }
}

#if 0 // generic_release_to prefered.

void Tag::release_to(l3obj* o, Tag* new_owner) {
    assert(o);
    assert(new_owner);
    if(this == new_owner) return; /* we already own it. noop. */

    // cannot move sysbuiltin objects.
    if (is_sysbuiltin(o)) {
        return;
    }

    assert(find(o) != 0);

    // move the file asociated
    //    l3path oldpath(path());
    //    oldpath.pushf("/%p",o);
    //    l3path newpath(new_owner->path());
    //    newpath.pushf("/%p",o);
    //    if (file_exists(oldpath())) { rename(oldpath(), newpath()); }

    o->_owner = new_owner; // critical that this happens too!

    new_owner->add(o);

    erase(o);
  }
#endif


L3METHOD(invoke_optional_dtor)
       // call the dtor if present. Since they may need to refer to objects by reference, we do this before
       //  cleaning up any llref*.
       if (obj->_dtor) {
           if (!is_dtor_done(obj)) {
               set_dtor_done(obj);
               obj->_dtor(obj,-1,exp,L3STDARGS_ENV);
           }
       }
L3END(invoke_optional_dtor)

  // captain
l3obj* Tag::captain()           {   return _captain; }

  //  we want to avoid changing this, so try to never need it, but
  //   we need it during bootstrap of global env / main obj.
  //  (Since they are circular, in that the global_ptag owns
  //   the main_env, and the main_env is the captain of the
  //   global_ptag.)
void   Tag::captain_set(l3obj* cap) { 
      _captain = cap; 
  }

  // deletion protocol
  // 
  // We thought deletion of a single object obj via generic_delete
  //  could happen in a separate dtor/ then free all from tags. but two
  //  phases doesn't work; we just need to cleanup from bottom up, returning
  //  from a dfs.
  //
  //   What is a forwarded tag, is this the same as _mytag != _owner. Yes. think of it as _mytag==0, but
  //            some functions always expect to be able to use _mytag for default allocation, and so
  //            (double objects) set _mytag to point somewhere else and set the forwarded bit. Hence
  //            this communicates the information to those functions about where to do default allocations...
  //            which should really be referred to the _owner parameter of each method now. And so
  //            we can probably eliminate the forwarded bit. But in case we don't get to that, treat
  //            forwarded bit set objects as having _mytag == 0, for all intents and purposes as far
  //            as deleting objects. So we act just like a) _mytag == 0 when there is a forwarded tag,
  //            and assume we have no tag-owned sub-objects to deal with.

  // Commentary: the phase1/phase2 didn't work because many of
  //  our dtors (e.g. for hash tables) deallocate their memory in the dtor, and so we will
  //  loose references to subobjects etc. No instead, I think we *must* do dfs dtor calling, calling
  //  the dtor bottom most first, and zipping up to the top as we go.

   // only deletes if this tag actually owns the object.
   // obj = object to be deleted.
   // 
L3METHOD(Tag::del_owned)
       LIVEO(obj);
       assert(obj);
       llref* llr_inref = find(obj);
       DV(print_llref(llr_inref,(char*)"   llr_inref: "););
       if (!llr_inref) {
           // we don't own it.
           printf("warning in del_owned: we don't own object %p  ser# %ld.\n",obj,obj->_ser);
           return 0;
           //l3throw(XABORT_TO_TOPLEVEL);
       }
       assert(llr_inref); // implies we own it, because find checks our _owned array for membership, and returns 0 if not found.
       assert(llr_inref->_priority == 1);

       // skip sysbuiltin objects : important so we don't cleanup T/F/na/nan etc
       if (is_sysbuiltin(obj) && !global_terp_final_teardown_started) {
           printf("warning in del_owned: object (%p ser# %ld '%s') is sysbuiltin. Not deleting.\n",obj,obj->_ser,obj->_varname);
           return 0;
           //l3throw(XABORT_TO_TOPLEVEL);
       }
       // skip undeletable objects
       if (is_undeletable(obj) && !global_terp_final_teardown_started) {
           printf("warning in del_owned: object (%p ser# %ld '%s') is marked undeletable. Not deleting.\n",obj,obj->_ser,obj->_varname);
           return 0;
           //l3throw(XABORT_TO_TOPLEVEL);
       }

       // llr_inref : gets us to the circularly linked doubly-linked list that contains all references to obj.

       DV(printf("888888 %p free in Tag(%p)::del_owned calling jfree() on %p ser# %ld\n",obj,this,obj,obj->_ser));

#if 0
       // call the dtor if present. Since they may need to refer to objects by reference, we do this before
       //  cleaning up any llref*.
       if (obj->_dtor) {
           if (!is_dtor_done(obj)) {
               set_dtor_done(obj);
               obj->_dtor(obj,-1,exp,L3STDARGS_ENV);
           }
       }
#endif
         invoke_optional_dtor(L3STDARGS);

         // use class specific dtor instead!
         //       post_dtor_hardcoded_special_type_destruct_handling(obj);
       

       // there are two parts to cleaning up the llref*
       // Part  IN-GONE: delete the in-pointers that point to us.
       // Part OUT-GONE: delete the out-pointers that are in our _judyS array that point to other objeccts.

       // Part IN-GONE llref cleanup:

       // should this be llref_del_any_priority(llr_inref)? 
       // No, erase does find() and then takes care of the priority 1 ref cleanup.
       llref_del(llr_inref,YES_DO_HASH_DELETE); // okay on hash delete for inrefs, yes.
       llr_inref = 0;

       // Part OUT-GONE llref cleanup:
       if (obj->_judyS) {

           if ( obj->_type == t_syv) {


           } else {

               char indent[] = "   ";
               size_t       sz = 0;

               llref* llr = 0;
               l3path key;
               BOOL gotone = obj->_judyS->first(&key,&llr);
               
               while (gotone) {
                      LIVEREF(llr);
                      DV(printf("%s%02ld:%s -> %p\n", indent, sz, key(), llr));
                      assert(llr->_env == obj);
                      assert(llr->_priority != 1);

                       // slight optimization over just doing 'llref_del(llr);': because we're going to
                       // do JSLFA anyway at the end after this loop, we
                       // dont do the individual deletes from the _judyS array
                       // as we go and delete the llref*.
                      llref_del_any_priority_and_no_hash_del(llr);

                      key.clear();
                      gotone = obj->_judyS->next(&key,&llr);
                      ++sz;
                   }


#if 0           
               PWord_t      PValue = 0;                   // Judy array element.
               uint8_t      Index[BUFSIZ];            // string to sort.
               
               Index[0] = '\0';                    // start with smallest string.
               JSLF(PValue, obj->_judyS->_judyS, Index);       // get first string
               char indent[] = "   ";
               
               while (PValue != NULL)
                   {
                       llref* llr = *((llref**)(PValue));
                       LIVEREF(llr);
                       DV(printf("%s%02ld:%s -> %p\n", indent, sz, Index, llr));
                       assert(llr->_env == obj);
                       assert(llr->_priority != 1);

                       // slight optimization over just doing 'llref_del(llr);': because we're going to
                       // do JSLFA anyway at the end after this loop, we
                       // dont do the individual deletes from the _judyS array
                       // as we go and delete the llref*.
                       llref_del_any_priority_and_no_hash_del(llr);

                       JSLN(PValue, obj->_judyS->_judyS, Index);
                       ++sz;
                   }
#endif

               // instead of new/delete we now do  malloc+placement new / ~dtor+free, so that Mac can track the judys_llref.
               //delete obj->_judyS;

               obj->_judyS->~judys_llref();
               ::free(obj->_judyS);


               obj->_judyS=0;
               //int Rc = 0;
               //JSLFA(Rc,  (obj->_judyS));
               //obj->_judyS = 0;
           }
       }

       if (obj->_judyL) {

           if ( obj->_type == t_syv) {
               assert(0); // what here?
           } else {
               int Rc = 0;
               JLFA(Rc,  (obj->_judyL));
           }
           obj->_judyL = 0;
       }

       // obj can be already zeroed after the _dtor cleaned up...if so it will be zero.
       if (0 != obj->_type) {
           erase(obj);    
           jfree(obj);
       } else {
           printf("very bad: we got an object with _typ == 0.  in Tag::del_owned().  NOT GOOD!\n");
           printf(" ** ALERT ALERT ALERT ************* skipping cleanup of object %p   ser # %ld   '%s'  *********\n", 
                  obj, obj->_ser, obj->_varname);
           assert(0);
       }

L3END(Tag::del_owned)


 void Tag::delfree(l3obj* o) {
    LIVEO(o);
    assert(o);
    DV(printf("Tag(%p)::del on %p\n",this,o));
    assert(find(o));

    erase(o); 
    jfree(o);
  }

   // jfree all the set of owned pointers.
L3METHOD(Tag::del_all)

    DV(printf("in (%p _sn_tag:%ld)  Tag::del_all()\n",this, this->_sn_tag));

    l3obj* ptr = 0;
    l3obj* findme = 0;

   Word_t * PValue;
   Word_t Index = 0;
   long i = 0;

   JLF(PValue, _owned, Index);
   while (PValue != NULL)
       {
           ptr = ((l3obj*)(Index));
           DV(printf("new way: 888888 %p free in Tag(%p)::del_all calling jfree() on %p ser# %ld\n",ptr,this,ptr,ptr->_ser));

           // this replaces all the following commented out code.
           findme = ptr;
           LIVE(findme);

           del_owned(findme,-1,0,L3STDARGS_ENV); // or generic_delete? e.g.
           //not right now, I think we handle captags okay. // generic_delete(findme, L3STDARGS_OBJONLY);

           JLN(PValue, _owned, Index);
           ++i;
       }
    int bytes_freed = 0;
    JLFA(bytes_freed,_owned);

L3END(Tag::del_all);


void Tag::dump_tbf(const char* pre) {

    l3obj* p = 0;
    int i = 0;
    if (tbf_stack.size() > 0) {
        for( tbf_stack.it.restart(); !tbf_stack.it.at_end(); ++tbf_stack.it, ++i) {
            p = *tbf_stack.it;
            printf("%s[%02d] on tbf_stack = type %s (%p ser# %ld) _malloc_size= %d   [ for reference sizeof(uh)=%ld,   sizeof(l3obj)=%ld ]\n",
                   pre,
                   i,
                   p->_type,
                   p,
                   p->_ser,
                   p->_malloc_size,
                   sizeof(uh),
                   sizeof(l3obj)
                   );
        }
    }
}

void Tag::dump(const char* pre) {

      l3obj* cap = captain();

      long   capsn = 0;
      char*  capnm = 0;
      l3path capstring(0,"[captain: none]");

      if (cap) {
          capsn = cap->_ser;
          capnm = cap->_varname;
          capstring.reinit("[captain: %p ser# %ld %s]",cap,capsn,capnm);
      } else {
          printf("internal error in Tag::dump(): no captain found for tag (%p ser# %ld %s)-- all tags should have captains!\n",this,this->sn(),this->myname());
          assert(0); // all tags should have captains!
          exit(1);
      }

      //      long ownsz = owned_size();
      //      if (ownsz == 0 && subtag_stack.size() == 0) {
      //          printf("%sTag '%s' (ser# %ld) %p is empty. %s\n", pre, myname(), _sn_tag,this,capstring());
      //        return;
      //      }

      l3path indent(pre);
      l3path indent_more(indent);
      indent_more.pushf("     ");
      l3path indent_more_more(indent);
      indent_more_more.pushf("     ");

      dump_owned(pre);

      dump_tbf(indent_more());

  } // end dump()



  void  Tag::set_myname(const char* m) {
    _myname.reinit("%s_uniq%p",m,this);
  }

  const char* Tag::get_tag_srcwhere() {
    return _where_declared();
  }

  const char* Tag::myname() {
    return _myname();
  }

  long Tag::sn() {
      return _sn_tag;
  }

  const char* Tag::path() {
    if (mypath.len()) return mypath();
    
    Tag* cur = this;
    std::vector<Tag*> stack;
    long maxloop = 100;
    while(cur) {
      stack.push_back(cur);
      cur = cur->parent();
      --maxloop;
      if (!maxloop) {
          printf("internal error: infinite loop detected in tag tree!\n");
          assert(0);
          exit(1);
      }
    }
    
    long N = (long)stack.size();
    for (long i = 0; i < N; ++i) {
      cur = stack[N-i-1];
      mypath.push(cur->myname());
    }

    return mypath();
  }


  // gen_

  void  Tag::gen_push(void* p) {
      genstack.push_front(p);
  }
  
  void  Tag::gen_free_all() {

    ustaq<void>::ustaq_it it(&genstack);
    void* p = 0;
    for( ; !it.at_end(); ++it) {
        p = *it;
        assert(p);
        ::free(p);
        }    
    genstack.clear();
  }

  void*  Tag::gen_remove(void* p) {
      assert(genstack.size());
     return genstack.del_val(p);
  }

  void*  Tag::gen_exists(void* p) {

     return genstack.member(p);
  }

  //
  // atom_ : only tracked by sermon, not tyse nor l3obj->_ser 
  //         numbers because there is no embedded header for these guys.
  //

  void  Tag::atom_push(void* p) {
      atomstack.push_front(p);
  }
  
  void  Tag::atom_free_all() {

    ustaq<void>::ustaq_it it(&atomstack);
    void* p = 0;
    for( ; !it.at_end(); ++it) {
        p = *it;
        assert(p);
        ::free(p);
        }    
    atomstack.clear();
  }

  void*  Tag::atom_remove(void* p) {
      assert(atomstack.size());
      return atomstack.del_val(p);
  }

  void*  Tag::atom_exists(void* p) {
      return atomstack.member(p);
  }


  // tbf_

  void  Tag::tbf_push(l3obj* p) {
      tbf_stack.push_front(p);
  }

  // actually returns a l3obj*
  l3obj* Tag::tbf_pop() { 
      return tbf_stack.pop_front();

  }

  l3obj* Tag::tbf_top() {

      return tbf_stack.front_val();
  } 

  L3METHOD(Tag::tbf_jfree_all)

    ustaq<l3obj>::ustaq_it it(&tbf_stack);
    l3obj* ptr = 0;
    for( ; !it.at_end(); ++it) {
        ptr = (*it);

        if (ptr->_type == t_lnk) {
            assert(0); // lnk should go on their own lnk_stack
            jfree(ptr);
            // no _dtor in lnk
            continue; 
        }

        // call the dtor if present
        if (ptr->_dtor) {
            ptr->_dtor(ptr,-1,exp,L3STDARGS_ENV);
        }        
        jfree(ptr);
    }
    tbf_stack.clear();

 L3END(Tag::tbf_jfree_all)

 lnk* Tag::lnk_exists(lnk* p) {
     if (lnk_stack.member(p)) return p;
     return 0;
 }

lnk*  Tag::lnk_remove(lnk* p) {
    return lnk_stack.del_val(p);
}

void  Tag::lnk_push(lnk* p) {
    assert(p);
    assert(p->_type = t_lnk);
    lnk_stack.push_front(p);
}

void  Tag::lnk_jfree(lnk* p) {
    LIVEREF(p->llr());
    lnk_stack.del_val(p);
    jfree((l3obj*)p);
}



L3METHOD(Tag::lnk_jfree_all)
{   long i = 0;
    ustaq<lnk>::ustaq_it it(&lnk_stack);
    lnk* ptr = 0;
    for( ; !it.at_end(); ++it, ++i) {
        ptr = (*it);
        assert(ptr->_type == t_lnk);
        jfree((l3obj*)ptr);
    }
    lnk_stack.clear();
}
L3END(Tag::lnk_jfree_all)


  void  Tag::llr_push(llref* p) {
    llrstack.push_front(p);
    assert(p->_obj->_owner == p->_refowner);
    assert(this == p->_refowner);
  }

  void  Tag::llr_remove(llref* p) {
    llref* was_there = llrstack.del_val(p);
    assert(was_there);
  }

  BOOL  Tag::llr_is_member(llref* p) {
      return (llrstack.member(p) != 0);
  }

  llref* Tag::llr_pop() { 
    return llrstack.pop_front();
  }

  llref* Tag::llr_top() {
      return llrstack.front_val();
  }
  

void Tag::llr_free_all(en_do_hash_delete do_hash_delete) {
    long N = llrstack.size();
    if(0==N) return;
    long i = 0;

    llref* w = 0;
    long llsn = 0;
    long objser = 0;

    for( llrstack.it.restart(); !llrstack.it.at_end(); ++llrstack.it, ++i) {
        w = *llrstack.it;
        LIVEREF(w);

        llsn   = w->_llsn;
        objser = w->_obj->_ser;

        unchain(w);
        if (do_hash_delete==YES_DO_HASH_DELETE) { llref_hash_delete(w); }

        llr_free(w);
        DV(printf("llr_free_all(): done free-ing %ld out of %ld of llrstack; _llsn=%ld   _obj->_ser=%ld \n",i,N,llsn,objser));
    }
    llrstack.clear();
}

  void Tag::llr_show_stack() {
      long N = llrstack.size();
      long i = 0;

      llref* r = 0;
      for( llrstack.it.restart(); !llrstack.it.at_end(); ++llrstack.it, ++i) {
          r = *llrstack.it;
          assert(r);
          printf("llr_show_stack: %p element, %ld of %ld:\n",r,i,N);
          print_llref(r,DQ("     "));
          printf("\n");
      }
  }

//
// just delete the llref, don't to any unchain-ing etc; this should be
//  the final stop; the llr's last resting place.
//
// obj = has the llref* (cast to l3obj*) to be cast back to (llref*) and llr_free()-ed.
//
L3METHOD(Tag::llr_free_and_notify)
{
    llref* freeme = (llref*)obj;

    l3obj* target = freeme->_obj;
    llr_free(freeme);

    // notify owner, in case it wants to eagerly delete a referenceless object.
    if (target) {
        target->_owner->reference_deleted(target,L3STD_OBJ);
    }
}
L3END(Tag::llr_free_and_notify)




void   Tag::llr_free(llref* freeme) {
      assert(freeme); 

      Tag* owner = this;
      
      assert(this == freeme->_refowner);

      DVV(printf("llr_show_stack BEFORE llr_free action:\n");
          llr_show_stack(););

      // remove it from llrstack
      llref* found = llrstack.del_val(freeme);

      if (found) {
          assert(found == freeme);
          // Zero to help catch any dereferences that occur after free.
          LLRDEL(freeme); // log it first.   does: llr_global_debug_list_del(freeme)
          
          //
          // no: this makes a double delete:         unchain(freeme,do_hash_delete);
          //
          // can't zero before delete, because otherwise component ustaq doesn't know how to delete.
          // now it does make sense... member ustaq<char> _properties is alive still..
          // so we cannot zero out the memory; let delete take care of:       bzero(freeme,sizeof(llref));
          // 

          delete freeme; // now uses new/delete instead of malloc/free. new is used so that ustaq (contained object) init() is done.

      } else {
          printf("internal error: llr_free(%p llsn# %ld) detected attempt to free llref that was not on owner's llrstack.  owner=(%p ser# %ld) '%s'.\n",
                 freeme,freeme->_llsn,owner,owner->_sn_tag, owner->myname());
          assert(0);
      }

      DVV(printf("llr_show_stack AFTER llr_free action:\n");
          llr_show_stack(););
  }



  void  Tag::sexpstack_push(sexp_t* p) {
      assert(p);
      //      assert(p->_ser != 0);
      //      assert(p->_owner == this);

      sexpstack.push_front(p);
  }

  sexp_t*  Tag::sexpstack_remove(sexp_t* p) {
    return sexpstack.del_val(p);
  }

  sexp_t*  Tag::sexpstack_exists(sexp_t* p) {
      if (sexpstack.member(p)) return p;
      return 0;
  }

  sexp_t* Tag::sexpstack_pop() { 
      return sexpstack.pop_front();
  }

  sexp_t* Tag::sexpstack_top() {
      return sexpstack.front_val();
  } 

  void Tag::sexpstack_destroy_all() {
      ustaq<sexp_t>::ustaq_it it(&sexpstack);
      for( ; !it.at_end(); ++it) {
          destroy_sexp(*it);
      }
      sexpstack.clear();
  }

  // subtags: Tag* implementers owned by us,
  //  that will have destruct() called on them
  //  when we do the same.
  typedef  std::list<Tag*>  itag_stack;
  typedef itag_stack::iterator itsit;
  typedef itag_stack::reverse_iterator ritsit;

  itag_stack subtag_stack;

  void  Tag::subtag_push(Tag* p) {
    assert(p != this);
    subtag_stack.push_front(p);
  }
  Tag* Tag::subtag_pop() { 
    return subtag_stack.pop_front();
  }

  Tag*   Tag::subtag_remove(Tag* psubtag) {
    return subtag_stack.del_val(psubtag);

#if 0
    // returns 0 if empty.

    long N = subtag_stack.size();
    if (0==N) return 0;

    itsit be = subtag_stack.begin();
    itsit en = subtag_stack.end();

    // assume the last is probably our target, so go through the list from last to first.
    itsit cur = subtag_stack.end();
    --cur;

    // TODO: for longer lists, a faster lookup method?
    // but probably our lists are very small/short.
    //    for (long i = 0 ; be != en; ++be, ++i ) {
    for (long i = 0; i < N; ++i) {
        
        if (psubtag == *cur) {
            subtag_stack.erase(cur);
            return psubtag;
        }
        --cur;
        if (i == N-1) {
            assert(cur == be);
        }
    }
#endif

    return 0;
  }


  Tag* Tag::subtag_top() {
    return subtag_stack.front_val();
  } 


  L3METHOD(Tag::subtag_destroy_all)
  {
    // no! we cannot use an iterator when deleting!!!. bad:   ustaq<Tag>::ustaq_it it(&subtag_stack);

    Tag* childtag = 0;
    while(1) {
        childtag = subtag_stack.front_val();
        if (childtag) {
            childtag->dfs_destruct(obj,arity,exp,env,retval,
                                   childtag,
                                   curfo,etyp,retown,ifp);
            delete childtag;
        } else break;
    }
    assert(subtag_stack.size()==0);
    subtag_stack.clear();

#if 0
    long N = subtag_stack.size();
    itsit cur = subtag_stack.begin();
    itsit nex = cur;

    if (N) {
        for (long i =0; i < N; i++) {        
            nex = cur;
            ++nex;
            // now we can delete cur which will then become invalid.
            Tag* childtag = (Tag*)*cur;
            childtag->dfs_destruct(obj,arity,exp,env,retval,
                                   childtag,
                                   curfo,etyp,retown,ifp);
            cur = nex;
        }
    }
    subtag_stack.clear();
#endif
  }
  L3END(Tag::subtag_destroy_all)


  //// newdel: to be deleted with c++ delete
  // currently Tags cannot be malloced due to use of C++ objects
  //  that have required constructors that must be run, so
  // we have to new and delete them.


  void  Tag::newdel_push(Tag* p) {
    del_itag_stack.push_front(p);
  }
  Tag* Tag::newdel_pop() { 
      return del_itag_stack.pop_front();

#if 0
    // returns 0 if empty.
    its_del_it be = del_itag_stack.begin();
    Tag* ret = *be;
    del_itag_stack.pop_front();
    return ret;
#endif
  }
  Tag* Tag::newdel_top() {
      return del_itag_stack.front_val();

#if 0
    // returns 0 if empty.
    if (del_itag_stack.size()==0) return 0;
    its_del_it be = del_itag_stack.begin();
    return *be;
#endif

  } 
  void Tag::newdel_destroy_all() {
    FILE* ifp = 0;
    ustaq<Tag>::ustaq_it it(&del_itag_stack);
    Tag* childtag = 0;
    l3obj* fakeret = 0;
    for( ; !it.at_end(); ++it) {
        childtag = *it;

        // jea: should we switch from delete to... tag_destruct? or dfs_de
        //delete(childtag);
        long unseen = childtag->dfs_been_here();
        childtag->dfs_destruct(0,unseen,0, 0,&fakeret,childtag, 0,0,childtag,ifp);

        l3path msg(0,"444444 %p delete\n",childtag);
        printf("%s\n",msg());
        MLOG_ADD(msg());
    }
    del_itag_stack.clear();

#if 0
    its_del_it be = del_itag_stack.begin();
    its_del_it en = del_itag_stack.end();
    for ( ; be != en; ++be) {
      delete (*be);

      l3path msg(0,"444444 %p delete\n",*be);
      printf("%s\n",msg());
      MLOG_ADD(msg());

    }
    del_itag_stack.clear();
#endif

  }

  // pass start = 0 to start at the front.
  l3obj* Tag::enum_owned_from(l3obj* start, llref** llrback) {

         *llrback = 0; // default ret value
         Word_t * PValue;
         Word_t Index = *((Word_t*)(&start));
         llref* llr = 0;

         if (start == 0) {
             JLF(PValue, _owned, Index);
         } else {
             JLN(PValue, _owned, Index);
         }
         
         if (PValue == NULL) { return 0; }

         l3obj* obj = (l3obj*)Index;

         if (0 != obj) {
             llr = *(llref**)PValue;
             *llrback = priority_ref(llr);
         }

         return obj;

  } // end enum_owned_from()


// generic_delete(): detect and handle delete_captag for captags, else just del_owned.
//
// generic_delete(to_be_del, L3STDARGS_OBJONLY);
//
L3METHOD(generic_delete)

    l3obj* fakeret = 0;
    if ((obj->_mytag) && pred_is_captag(obj->_mytag,obj)) { 
        delete_captag(obj,-1,0,env, &fakeret,obj->_mytag,0,0,obj->_mytag,ifp);
    } else {
        if (!is_sysbuiltin(obj)) {
            obj->_owner->del_owned(obj,arity,exp,L3STDARGS_ENV);
        }
    }

L3END(generic_delete)



void   Tag::release_llref_to(llref* transfer_me, Tag* newowner) {

      llref* found = llrstack.del_val(transfer_me);
      assert(found); // we must actually be the owner before we can release it.
      assert(found == transfer_me);

      transfer_me->_refowner = newowner;
      newowner->llr_push(transfer_me);
}


 // update arg_val when it changes
void update_arg_val(actualcall* pcall, l3obj* delme, l3obj* replacement) {
    
    int narg = pcall->narg;
    for (int i = 0; i < narg; ++i) {

        if (pcall->arg_val[i] == delme) {
            pcall->arg_val[i] = replacement;
            return;
        }
    }

    // no: we cannot assert here.
    // we have to allow search and not finding! e.g. by scan_env_stack_for_calls_with_cached_obj()
    // assert(0); 
}


// if given, scan starting env, then up the global_env_stack,
// trying to replace delme with replacement in actualcall->arg_val
void scan_env_stack_for_calls_with_cached_obj(l3obj* startingenv,  l3obj* delme, l3obj* replacement) {

    assert(delme);
    assert(replacement);

    actualcall* pcall = 0;
    if (startingenv) {
        if (startingenv->_type == t_cal) {
            pcall = (actualcall*)(startingenv)->_pdb;
            update_arg_val(pcall, delme, replacement);
        }
    }

    //    EnvStack_it be = global_env_stack.begin();
    //    EnvStack_it en = global_env_stack.end();
    //    while(be != en) {

    for ( EnvStack_it be(&global_env_stack._stk); !be.at_end(); ++be ) {

        if (!(*be)) continue;

        if ((*be)->_type == t_cal) {
            pcall = (actualcall*)(*be)->_pdb;
            update_arg_val(pcall, delme, replacement);
        }
    //        ++be;
    }
    
}

//
// called by rm_alias: returns 0 if deleted the object, else returns a 1 if there is an llref that still refers to the object.
//
// obj = changedref_obj : to be possibly deleted.
//
//
L3METHOD(Tag::reference_deleted) 
{
    l3obj* changedref_obj = obj;

    llref* myref = find(changedref_obj);
    if (!myref) {
#if 0
        // we don't own it, problem.
        printf("internal error: reference_deleted(%p #%ld) called on object this tag(%p #%ld) does not own\n",
               changedref_obj,
               changedref_obj->_ser,
               this,
               this->sn());
        assert(0);
        exit(1);
#endif
        return 0; // allow this to enable lhs eval before rhs.
    }

    if (global_terp_final_teardown_started) {
        // skip the sealed, sysbuiltin, undeletable checks...

    } else {
        if (is_sealed(changedref_obj)) return 1;
        if (is_sysbuiltin(changedref_obj)) return 1;
        if (is_undeletable(changedref_obj)) return 1;
    }

    if (llref_pred_refcount_one(myref)) {
        //generic_delete(changedref_obj,-1,0, 0,0,this, 0,0,this);

        // why would you need env? here is how to get it, but shouldn't the dtor *know* 
        //  on what object it is being called...???
        generic_delete(changedref_obj,arity,exp, env,retval,this, curfo,etyp,this,ifp);
        return 0;
    }

    long numref = llref_size(myref);
    DV(
       if (numref == 2) {
           printf("Tag::reference deleted: we seen numref == 2\n");
       }
       );

    return 1;
}
L3END(Tag::reference_deleted)

#if 0
void  top_down_set_been_here(long newval) {
    glob->recursively_been_here_set(newval);
}
#endif

void lnk::zeroref() {
    _link_llr = 0;
    //    _target = 0;
}


l3obj*  lnk::chase() {
    if (0==this) return gnil; // null pointer
    if (_link_llr) {
        l3obj* target = _link_llr->_obj;
        LIVEO(target);
        return target;
    } else {
        return gnil;
    }
}

void    lnk::linkto(l3obj* tar) {
    LIVEO(tar);
    //    _target     = tar;
    //    _target_ser = tar->_ser;    

    if (_link_llr) {
        if (_link_llr->_obj == tar) return;
        llref_del(_link_llr,YES_DO_HASH_DELETE);
    } 

    _link_llr = llref_new(_key(), tar, 0, tar->_owner->find(tar), tar->_owner,this);
}


l3obj* l3obj::parent() { return _parent->chase(); }
l3obj* l3obj::child()  { return _child->chase(); }
l3obj* l3obj::sib()    { return _sib->chase(); }

void l3obj::parent(_l3obj* p) {
    if (!_parent) {
        _parent = new_lnk(p, _mytag ? _mytag : _owner, "parent");
    }
    _parent->linkto(p); 
}
void l3obj::child(_l3obj* p)  { 
    if (!_child) {
        _child = new_lnk(p, _mytag ? _mytag : _owner,"child");
    }
    _child->linkto(p); 
}
void l3obj::sib(_l3obj* p)    {
    if (!_sib) {
        _sib = new_lnk(p, _mytag ? _mytag : _owner, "sib");
    }
    _sib->linkto(p); 
}


lnk* new_lnk(l3obj* tar, Tag* owner, const char* key) {
     l3path vn(0,"lnk_to_%p",tar);

     DV(printf("making new_lnk, sizeof(lnk) is %ld\n", sizeof(lnk)));
     lnk* nlink =  (lnk*)jmalloc(sizeof(lnk), //size_t size,
                                 owner,       // Tag* owntag, 
                                 merlin::jmemrec_TagOrObj_en_JMEM_LNK,    // merlin::jmemrec_TagOrObj_en type, 
                                 "lnk",       // const char* classname,
                                 0,           // const char* where_srcfile_line,
                                 &vn,         // l3path* vn
                                 1,            // ,long notl3obj // defaults should be 0, for allocating l3obj*; for other structures, set to 1
                                 t_lnk
                                 );
     assert(nlink);
     nlink->_type = t_lnk;
     //nlink->_target = tar;
     //nlink->_target_ser = tar->_ser;
     nlink->_owner  = owner;

     assert(owner->lnk_exists(nlink));

     // symbol like attribs

     // tar->_owner has to own the llref itself; so its on the same chain as the
     // other references, and can be zeroed if the object disappears.
     nlink->_link_llr = llref_new((char*)key, tar, 0, tar->_owner->find(tar), tar->_owner,nlink);

     if (key) {
         nlink->_key.reinit(key);
     } else {
         nlink->_key.clear();
     }

     return nlink;
}

void del_lnk(lnk* link) {
    FILE* ifp = 0;
    assert(link);

    // have to tell the owner of the _link_llr to delete it before we totally disappear.
    llref* llr = link->llr();
    if (llr) { 
        LIVEREF(llr);

        l3obj* tar = llr->_obj;
        LIVEO(tar);

        unchain(llr);
        llr->_refowner->llr_free(llr);


#if 1  // 1 because we want objects/tmps only maintained by links to disappear when the last link goes.

    // don't do this incase there is an assignment in progress right now
    // where the object will get picked up.
    Tag* tarowner = tar->_owner;
    LIVET(tarowner);

    tarowner->reference_deleted(tar, -1,0,  tar,0, tarowner,  tar,0,tarowner,ifp);
#endif

    }
    
    jfree((l3obj*)link);
}

void lnk_print(lnk* link, const char* indent, stopset* stoppers) {
    assert(link);
    l3path  m;
    m.pushf("%s%s -lnk-> ", 
           indent,
            link->_key());
    m.out();

    if (link->_link_llr && link->_link_llr->_obj) {
        if (stoppers) { stopset_push(stoppers,link); }
        print(link->_link_llr->_obj,indent,stoppers);
    } else {
        printf("0x0\n");
    }
}

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


// l3path.h header cannot know about sexp_t yet, and in release build the above doesn't work? hence this helper
//
void l3str_reinit_helper(const sexp_t* sexp, char*& p, char** pbuf, int bufsz) {

    qqchar tspan = sexp->span();
    long n = tspan.copyto(*pbuf, bufsz-2);

    p = (*pbuf) + n;
    *p = 0;
}



tyse*   Tag::tmalloc(t_typ ty, size_t size, int zerome) {

    tyse* ptr = (tyse*)tyse_malloc(ty, size, zerome);
    tyse_push(ptr);

    return ptr;
}

//void    Tag::tfree(tyse*  ptr) {
void    Tag::tfree(void*  ptr) {
    tyse_remove((tyse*)ptr);
    tyse_free(ptr);
}



L3METHOD(Tag::tyse_tfree_all)

    ustaq<tyse>::ustaq_it it(&tyse_stack);
    tyse* ptr = 0;

    for( ; !it.at_end(); ++it) {
        ptr = (*it);
        tfree(ptr);
    }
    tyse_stack.clear();

L3END(Tag::tyse_tfree_all)


void  Tag::tyse_push(tyse* p) {
      tyse_stack.push_front(p);
}

  
void  Tag::tyse_free_all() {

    ustaq<tyse>::ustaq_it it(&tyse_stack);
    tyse* p = 0;
    for( ; !it.at_end(); ++it) {
        p = *it;
        assert(p);
        tfree(p);
        }
    tyse_stack.clear();
}


tyse*  Tag::tyse_remove(tyse* p) {

     return tyse_stack.del_val(p);
}

tyse*  Tag::tyse_exists(tyse* p) {

    if(tyse_stack.member(p)) return p;
    return 0;
}


char*  Tag::strdup_atom(char* src) {
    char* s = strdup(src);
    atom_push(s);
    return s;
}

char*  Tag::strdup_qq(const qqchar& src) {
    long N = src.sz();
    char* s = (char*)malloc(N+1);
    src.copyto(s,N);
    s[N]=0;
    atom_push(s);
    return s;
}

ustaq<lnk>* Tag::print_lnk_stack(char* indent) {

    lnk_stack.dump(indent,false);

    return &lnk_stack;
}

actualcall::actualcall() {
    //    printf("86868686  actualcall running on %p\n",this);

}
actualcall::~actualcall() {
    //    printf("97979797 ~actualcall running on %p\n",this);

}

l3obj*     lnk::target()     { return _link_llr ? _link_llr->_obj : 0; } 


void lnk::update_key(char* newkey) {
    _key.reinit(newkey);
}



// old:
//typedef  ddlist<l3obj> EnvStack;


// new:

void EnvStack::push_back(l3obj* o) {
    _stk.push_back(o);
}

void EnvStack::push_front(l3obj* o) {
    _stk.push_front(o);

    dump();
}
long EnvStack::size() {
    return _stk.size();
}


void EnvStack::dump() {
    
    printf("dump_global_env_stack():  ");
    if (_stk.size()==0) {
        printf(" empty.\n");
        return;
    }
    printf("\n");
    long i  =0;
    l3obj* o = 0;
    for (_stk.it.restart(); !_stk.it.at_end(); ++_stk.it, ++i) {
        o = *(_stk.it);
        if (o) {
            printf("   [%02ld]  ser# %ld  %p\n", i, o->_ser, o);
        } else {
            printf("   [%02ld]  0x0\n", i);
        }
    }
}
