//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef QUICKTYPE_H
#define QUICKTYPE_H

#include "qexp.h"
#include "ostate.h"
#include <iostream>


// quick type system
typedef const char* t_typ; // refer to one of the below

struct Tag;
struct _l3obj;



typedef int (*ptr2method) (_l3obj* obj, long arity, sexp_t* exp, _l3obj* env, _l3obj** retval,  Tag* owner, _l3obj* curfo, t_typ etyp, Tag* retown, FILE* ifp);

class qtree;
typedef qtree* (*ptr2new_qtree) (long id, const qqchar& v);


#define MIXIN_TYPSER() \
     t_typ      _type; \
     long       _ser;  \


typedef struct _tyse {
     MIXIN_TYPSER()
} tyse;

typedef unsigned int uint;
struct Tag;

 // MIXINS: for C compatibility, we use mixins instead of inheritance. Avoids vtable slicing issues too.

 // TOHEADER: the Tag-Object header...should be first in both Tag and l3obj, so that _type and _reserved
 //   are in the same offset relative to the start of the object for both.
 //
 // _malloc_size: jmalloc records size here...needed for realloc.
 // _owner      : jmalloc records owner here.
 //


#define MIXIN_MINHEADER() \
     MIXIN_TYPSER() \
     uint       _reserved; \
     uint       _malloc_size; \
     Tag*       _owner; \


struct minheader {
    MIXIN_MINHEADER();
};

std::ostream& operator<<(std::ostream& os, const tyse& printme);

template<typename V>
struct judySmap;

template<typename VK, typename V>
struct jlmap;

// table driven (runtime-enhanceable) type system.
struct quicktype {
    MIXIN_MINHEADER() // the _type here will be t_qty for quicktype

    t_typ       _ty; // the described type.
    ptr2method  _print;
    ptr2method  _ctor;
    ptr2method  _cpctor;
    ptr2method  _dtor;

    ptr2method  _save;
    ptr2method  _load;
    ptr2new_qtree  _qtreenew;
};


class quicktype_sys {

    judySmap<quicktype* >* _typemap;

 public:

    void load_builtin_types();

    quicktype_sys();
    ~quicktype_sys();

    t_typ which_type(const char* t, quicktype** pqt);
    t_typ which_type(const qqchar& t, quicktype** pqt);

    quicktype*  make_new_type(t_typ  addme);

    void register_type(t_typ  addme, quicktype* qt);
    void unregister_type(t_typ delme);

};

extern t_typ   t_qty; //   ="t_qty"; // struct quicktype

extern t_typ   t_qxp; //   ="t_qxp"; // struct qexp

extern t_typ   t_tdo; //   ="t_tdo"; // top-down operator precedence parsing output.





// from l3ts.proto
extern t_typ   t_qts; //   ="t_qts"; // l3ts::qts

extern t_typ   t_nv3; //   ="t_nv3"; // l3ts::nameval
extern t_typ   t_ob3; //   ="t_ob3"; // l3ts::object
extern t_typ   t_tm3; //   ="t_tm3"; // l3ts::timept

extern t_typ   t_gl3; //   ="t_gl3"; // l3ts::geoloc
extern t_typ   t_tp3; //   ="t_tp3"; // l3ts::timeplace
extern t_typ   t_da3; //   ="t_da3"; // l3ts::datapt

extern t_typ   t_ts3; //   ="t_ts3"; // l3ts::timeseries
extern t_typ   t_be3; //   ="t_be3"; // l3ts::BegEnd
extern t_typ   t_si3; //   ="t_si3"; // l3ts::SampleSpec

extern t_typ   t_bs3; //   ="t_bs3"; // l3ts::BeginEndSet
extern t_typ   t_wh3; //   ="t_wh3"; // l3ts::When
extern t_typ   t_id3; //   ="t_id3"; // l3ts::TSID

extern t_typ   t_ds3; //   ="t_ds3"; // l3ts::TSDesc
extern t_typ   t_he3; //   ="t_he3"; // l3ts::pb_msg_header
extern t_typ   t_qt3; //   ="t_qt3"; // l3ts::qtreepb
extern t_typ   t_qtr; //   ="t_qtr"; // qtree


extern t_typ   t_cmd; //   ="t_cmd"; // a command / function invocation with implicit ($ )
extern t_typ   t_pcd; //   ="t_pcd"; // a parse_cmd struct

extern t_typ   u_pdata; // ="parse_data_t"; 
extern t_typ   u_sexpt; // ="sexp_t";
extern t_typ   u_pcont_t; // ="pcont_t";

extern t_typ   t_ded; // ="t_ded"; // dead object.
extern t_typ   t_ddr; // ="t_ddr"; // dd* refs
extern t_typ   t_ust; // ="t_ust"; // a type set, or compound type chain;  ustaq<char> that points to t_typ

extern t_typ   t_ato; // ="t_ato"; // s-expression atom

extern t_typ   t_dou; // ="t_dou"; // doubles
extern t_typ   t_str; // ="t_str"; // strings
extern t_typ   t_obj; // ="t_obj"; // objects
extern t_typ   t_llr; // ="t_llr"; // llref
extern t_typ   t_ull; // ="t_ull"; // ustaq<T>::ll
extern t_typ   t_lnk; // ="t_lnk"; // lnk
extern t_typ   t_lin; // ="t_lin"; // link

extern t_typ   t_ddt; // ="t_ddt"; // double linked list

extern t_typ   t_fun; // ="t_fun"; // functions
extern t_typ   t_env; // ="t_env"; // envs are really just hash tables
extern t_typ   t_lst; // ="t_lst"; // lists
extern t_typ   t_dsq; // ="t_dsq"; // dstaq lists, wrapped in l3object.
extern t_typ   t_dsd; // ="t_dsd"; // dstaq.h template dstaq class, no wrapper.

extern t_typ   t_mqq; // ="t_mqq"; // message q / exception q

extern t_typ   t_cnd; // ="t_cnd"; // condition
extern t_typ   t_lit; // ="t_lit"; // literal keyword/function name

extern t_typ   t_par; // ="t_par"; // formal parameter list
extern t_typ   t_arg; // ="t_arg"; // argument to a function
extern t_typ   t_ret; // ="t_ret"; // return value from a function

extern t_typ   t_vvc; // ="t_vvc"; // pointer to resolved values vector, a vector of l3obj*

extern t_typ   t_vec; // ="vec";   // vector type
extern t_typ   t_sxp; // ="t_sxp"; // s-expression type, an l3obj-wrapped sexpobj_struct 
extern t_typ   t_cal; // ="t_cal"; // call parameters for actual function invocations


// quick type system II, may or may not be implemented as need be. But these
//  type identifiers are reserved.

extern t_typ   t_any; // ="t_any"; // infered type, can be anything; i.e. not specified. Like auto in D.
extern t_typ   t_tyy; // ="t_tyy"; // a type, one of these char[] pointers.
extern t_typ   t_nil; // ="t_nil"; // nil, like void in C, an empty list, nothing here, nothing returned, nothing in.

extern t_typ   t_set; // ="t_set"; // set
extern t_typ   t_map; // ="t_map"; // map
extern t_typ   t_mcr; // ="t_mcr"; // macro

extern t_typ   t_quo; // ="t_quo"; // quoted expression
extern t_typ   t_dfr; // ="t_dfr"; // data frame
extern t_typ   t_mat; // ="t_mat"; // matrix

extern t_typ   t_dag; // ="t_dag"; // directed acyclic graph
extern t_typ   t_blb; // ="t_blb"; // blob, binary large object
extern t_typ   t_vop; // ="t_vop"; // void* in C, a generic pointer, one word in size

extern t_typ   t_nls; // ="t_nls"; // named lists, like dynamic structs- just like R
extern t_typ   t_sct; // ="t_sct"; // a C struct
extern t_typ   t_enu; // ="t_enu"; // a C enum

extern t_typ   t_pid; // ="t_pid"; // a process handle
extern t_typ   t_tid; // ="t_tid"; // a thread handle
extern t_typ   t_sym; // ="t_sym"; // a symbol
extern t_typ   t_syv; // ="t_syv"; // a symbol vector; see symvec.h

extern t_typ   t_src; // ="t_src"; // source code, compilable.
extern t_typ   t_cod; // ="t_cod"; // compiled, executable code.
extern t_typ   t_lab; // ="t_lab"; // a pre-computed goto label; a machine address to jump to.

extern t_typ   t_int; // ="t_int"; // 32bit integer; a C short
extern t_typ   t_lng; // ="t_lng"; // 64bit integer; a C long
extern t_typ   t_byt; // ="t_byt"; // an 8-bit byte stream (binary large or small object).

extern t_typ   t_chr; // ="t_chr"; // C character
extern t_typ   t_utc; // ="t_utc"; // a single utf8 unicode character
extern t_typ   t_ut8; // ="t_ut8"; // utf8 string

extern t_typ   t_q3q; // ="t_q3q"; // triple quoted string
extern t_typ   t_sqo; // ="t_sqo"; // a single quote character

extern t_typ   t_thk; // ="t_thk"; // a thunk: a function that takes no arguments and returns no arugments, run purely for side effects.
extern t_typ   t_uni; // ="t_uni"; // a universal function: void* univ(void*, void*);
extern t_typ   t_cst; // ="t_cst"; // a casting function:  takes an object of one type and turns it into another type.  <t_typ> caster(<t_typ> in_obj)

extern t_typ   t_ano; // ="t_ano"; // an annotation of existing functions, types, code, with extensible, user-defined properties, that are machine parsable.
extern t_typ   t_tpl; // ="t_tpl"; // a C++ style template
extern t_typ   t_tag; // ="t_tag"; // a memory ownership tag.
extern t_typ   t_cap; // ="t_cap"; // a cap-tag pair, this being the captain.

extern t_typ   t_knd; // ="t_knd"; // a kind (type of type)
extern t_typ   t_seq; // ="t_seq"; // a sequence (or or more, not necessarily a list or vector because may be lazily supplied)

extern t_typ   t_doc; // ="t_doc"; // documentation (text description)
extern t_typ   t_key; // ="t_key"; // a key in a map (abstract type)
extern t_typ   t_val; // ="t_val"; // a value in a map (abstract type)

extern t_typ   t_box; // ="t_box"; // a mailbox 
extern t_typ   t_msg; // ="t_msg"; // a message from the mailbox
extern t_typ   t_ccc; // ="t_ccc"; // complex number with 64-bit double real and 64-bit double imaginary parts.

extern t_typ   t_flt; // ="t_flt"; // a 32-bit floating point number
extern t_typ   t_sql; // ="t_sql"; // a sql query
extern t_typ   t_ver; // ="t_ver"; // a version identifier (abstract)

extern t_typ   t_uid; // ="t_uid"; // a UUID identifer
extern t_typ   t_laz; // ="t_laz"; // a lazy type (abstract)
extern t_typ   t_frc; // ="t_frc"; // a forced/immediate type (abstract, opposite of lazy)

extern t_typ   t_imm; // ="t_imm"; // an immutable type
extern t_typ   t_con; // ="t_con"; // a constant type
extern t_typ   t_vol; // ="t_vol"; // a volatile type

extern t_typ   t_ref; // ="t_ref"; // pass-by-reference
extern t_typ   t_vlu; // ="t_vlu"; // pass-by-value

extern t_typ   t_stc; // ="t_stc"; // static alloation/keyword
extern t_typ   t_aut; // ="t_aut"; // auto (stack based) allocation


extern t_typ   t_ifc; // ="t_ifc"; // an interface
extern t_typ   t_sup; // ="t_sup"; // a super class (base class)
extern t_typ   t_der; // ="t_der"; // a derived class (child class)

extern t_typ   t_ptr; // ="t_ptr"; // a pointer
extern t_typ   t_adr; // ="t_adr"; // an address
extern t_typ   t_com; // ="t_com;  // a src code comment

extern t_typ   t_rtm; // ="t_rtm"; // a run time macro


extern t_typ   t_exp; // ="t_exp"; // an expression (abstract type)
extern t_typ   t_stm; // ="t_stm"; // a statement (abstract type)
extern t_typ   t_for; // ="t_for"; // a for loop / foreach statement (abstract type)

extern t_typ   t_ifs; // ="t_ifs"; // an if-then-else statement
extern t_typ   t_whi; // ="t_whi"; // a while loop
extern t_typ   t_exc; // ="t_exc"; // an exception

extern t_typ   t_thw; // ="t_thw"; // a throw statement
extern t_typ   t_cat; // ="t_cat"; // a catch statement
extern t_typ   t_fin; // ="t_fin"; // a finally statement

extern t_typ   t_iso; // ="t_iso"; // in_server_owns type @>
extern t_typ   t_ico; // ="t_ico"; // in_client_owns type !>
extern t_typ   t_oso; // ="t_oso"; // out_server_owns type !<
extern t_typ   t_oco; // ="t_oco"; // out_client_owns type @<

extern t_typ   t_reg; // ="t_reg"; // a register type
extern t_typ   t_asi; // ="t_asi"; // an assignement statement
extern t_typ   t_boo; // ="t_boo"; // boolean type
extern t_typ   t_boe; // ="t_boe"; // boolean expression

extern t_typ   t_pbm; // ="t_pbm"; // a protocol buffer message
extern t_typ   t_pbi; // ="t_pbi"; // a protocol buffer interface
extern t_typ   t_hdr; // ="t_hdr"; // a header file

extern t_typ   t_mdl; // ="t_mdl"; // a module of classes and functions and types
extern t_typ   t_flh; // ="t_flh"; // a file handle
extern t_typ   t_srm; // ="t_srm"; // a C++ stream (abstract)

extern t_typ   t_osm; // ="t_osm"; // a C++ output stream
extern t_typ   t_ism; // ="t_ism"; // a C++ input stream
extern t_typ   t_imp; // ="t_imp"; // an import statement, D-style import of module.

extern t_typ   t_inc; // ="t_inc"; // an include statement, for C style header include.
extern t_typ   t_oss; // ="t_oss"; // C++ ostringstream
extern t_typ   t_iss; // ="t_iss"; // C++ istringstream

extern t_typ   t_cpp; // ="t_cpp"; // inline C++ code
extern t_typ   t_lsp; // ="t_lsp"; // inline lisp code, starting with ($   ...ending with )
extern t_typ   t_lsc; // ="t_lsc"; // inline lisp code, starting with (:   ...ending with )
extern t_typ   t_ftr; // ="t_ftr"; // inline fortran code

extern t_typ   t_jav; // ="t_jav"; // inline java code
extern t_typ   t_ocm; // ="t_ocm"; // inline ocaml code
extern t_typ   t_prl; // ="t_prl"; // inline perl code

extern t_typ   t_skt; // ="t_skt"; // unix socket
extern t_typ   t_pth; // ="t_pth"; // filesystem path
extern t_typ   t_dir; // ="t_dir"; // filesystem directory

extern t_typ   t_pip; // ="t_pip"; // unix pipe
extern t_typ   t_lck; // ="t_lck"; // lock (mutex)
extern t_typ   t_cvr; // ="t_cvr"; // condition variable

extern t_typ   t_htm; // ="t_htm"; // html text
extern t_typ   t_arf; // ="t_arf"; // aref hyperlink anchor
extern t_typ   t_url; // ="t_url"; // url

extern t_typ   t_git; // ="t_git"; // git repository
extern t_typ   t_scm; // ="t_scm"; // inline scheme code.

extern t_typ   t_brk; // ="t_brk"; // breakpoint
extern t_typ   t_lnn; // ="t_lnn"; // line number
extern t_typ   t_srf; // ="t_srf"; // source file (pathname)x

extern t_typ   t_shm; // ="t_shm"; // shared memory segment
extern t_typ   t_gph; // ="t_gph"; // graph

extern t_typ   t_wnd; // ="t_wnd"; // windowing system window
extern t_typ   t_dia; // ="t_dia"; // dialog box

extern t_typ   t_png; // ="t_png"; // PNG image
extern t_typ   t_jpg; // ="t_jpg"; // JPEG image

extern t_typ   t_arr; // ="t_arr"; // multi-dimenstional array
extern t_typ   t_lib; // ="t_lib"; // static library handle
extern t_typ   t_dll; // ="t_dll"; // dynamic linked library (.so) file handle

extern t_typ   t_siz; // ="t_siz"; // size_t C type (number of bytes)
extern t_typ   t_lvm; // ="t_lvm"; // an LLVM type
extern t_typ   t_lsh; // ="t_lsh"; // a Lush2 type

extern t_typ   t_new; // ="t_new"; // new C++ statement
extern t_typ   t_del; // ="t_del"; // delete C++ statement

extern t_typ   t_mlc; // ="t_mlc"; // malloc C statement
extern t_typ   t_fre; // ="t_fre"; // free C statement

extern t_typ   t_rea; // ="t_rea"; // real number type
extern t_typ   t_bgi; // ="t_bgi"; // big integer type (unlimited precision)
extern t_typ   t_mp3; // ="t_mp3"; // mp3 file handle

extern t_typ   t_mpg; // ="t_mpg"; // mpeg file handle


extern t_typ   t_dat; // ="t_dat"; // date / timestamp
extern t_typ   t_tim; // ="t_tim"; // timestamp

extern t_typ   t_trv; // ="t_trv"; // tree view
extern t_typ   t_lbx; // ="t_lbx"; // list box
extern t_typ   t_mus; // ="t_mus"; // mouse position

extern t_typ   t_quu; // ="t_quu"; // queue type/fifo
extern t_typ   t_pop; // ="t_pop"; // pop  operation
extern t_typ   t_psh; // ="t_psh"; // push operation

extern t_typ   t_asm; // ="t_asm"; // inline assembly
extern t_typ   t_clo; // ="t_clo"; // closure
extern t_typ   t_tls; // ="t_tls"; // thread local storage

extern t_typ   t_tyl; // ="t_tyl"; // type list (list of t_typ attributes)
extern t_typ   t_wea; // ="t_wea"; // weak reference
extern t_typ   t_stg; // ="t_stg"; // strong reference

extern t_typ   t_dcl; // ="t_dcl"; // declaration
extern t_typ   t_dfn; // ="t_dfn"; // definition
extern t_typ   t_uns; // ="t_uns"; // unsigned (natural number)

extern t_typ   t_jdy; // ="t_jdy"; // Judy array
extern t_typ   t_jdl; // ="t_jdl"; // JudyL array
extern t_typ   t_jds; // ="t_jds"; // JudyS array

extern t_typ   t_ast; // ="t_ast"; // abstract syntax tree type.
extern t_typ   t_sig; // ="t_sig"; // unix signal

extern t_typ   t_opn; // ="t_opn"; // open paren
extern t_typ   t_cpn; // ="t_cpn"; // close paren

extern t_typ   t_obr; // ="t_obr"; // open brace {
extern t_typ   t_cbr; // ="t_cbr"; // close brace }

extern t_typ   t_osq; // ="t_osq"; // open square bracket [
extern t_typ   t_csq; // ="t_csq"; // close square brakcet ]

extern t_typ   t_idx; // ="t_idx"; // Lush2 index 
extern t_typ   t_ind; // ="t_ind"; // index operatation in [] square brackets

extern t_typ   t_let; // ="t_let"; // let statement
extern t_typ   t_lda; // ="t_lda"; // lambda statement

extern t_typ   t_fst; // ="t_fst"; // first (car)
extern t_typ   t_rst; // ="t_rst"; // rest (cdr)

extern t_typ   t_ctr; // ="t_ctr"; // constructor
extern t_typ   t_dtr; // ="t_dtr"; // destructor
extern t_typ   t_sco; // ="t_sco"; // scope statement (like D)

extern t_typ   t_nms; // ="t_nms"; // namespace
extern t_typ   t_fbn; // ="t_fbn"; // forbidden
extern t_typ   t_dpc; // ="t_dpc"; // deprecated

extern t_typ   t_dst; // ="t_dst"; // probability distribution
extern t_typ   t_den; // ="t_den"; // probablity density
extern t_typ   t_hst; // ="t_hst"; // histogram

extern t_typ   t_ncr; // ="t_ncr"; // increment statement
extern t_typ   t_dcr; // ="t_dcr"; // decrement statement
extern t_typ   t_evl; // ="t_evl"; // take value of, eval statement.

extern t_typ   t_cts; // ="t_cts"; // continuation
extern t_typ   t_hea; // ="t_hea"; // heap allocated memory
extern t_typ   t_stk; // ="t_stk"; // stack allocated memory

extern t_typ   t_bit; // ="t_bit"; // bitwise operation
// redundant with m_bar: extern t_typ   t_bor; // ="t_bor"; // bitwise or
extern t_typ   t_xor; // ="t_xor"; // bitwise xor

extern t_typ   t_and; // ="t_and"; // bitwise and
extern t_typ   t_ndn; // ="t_ndn"; // bitwise nand
extern t_typ   t_bno; // ="t_bno"; // bitwise not

extern t_typ   t_lor; // ="t_lor"; // logical or
extern t_typ   t_lan; // ="t_lan"; // logical and
extern t_typ   t_not; // ="t_not"; // logical not

extern t_typ   t_tru; // ="t_tru"; // boolean true
extern t_typ   t_fal; // ="t_fal"; // boolena false

extern t_typ   t_nan; // ="t_nan"; // quiet nan
extern t_typ   t_nav; // ="t_nav"; // not available (missing data)

extern t_typ   t_inf; // ="t_inf"; // +infinity
extern t_typ   t_nnf; // ="t_nnf"; // -infinity

extern t_typ   t_zer; // ="t_zer"; // zero type (zeroed out memory allocation to start with).
extern t_typ   t_ini; // ="t_ini"; // initializer

extern t_typ   t_fix; // ="t_fix"; // fixed size data structure
extern t_typ   t_vrb; // ="t_vrb"; // variable sized data structure

extern t_typ   t_mix; // ="t_mix"; // mixin D style.
extern t_typ   t_dmd; // ="t_dmd"; // D code in line.
extern t_typ   t_dim; // ="t_dim"; // dimensions (or array)

extern t_typ   t_jmp; // ="t_jmp"; // jump statement

extern t_typ   t_alg; // ="t_alg"; // algebraic/math/floating point operation
extern t_typ   t_add; // ="t_add"; // addition
extern t_typ   t_sub; // ="t_sub"; // subtraction

extern t_typ   t_mul; // ="t_mul"; // multiplication
extern t_typ   t_div; // ="t_div"; // division

extern t_typ   t_mod; // ="t_mod"; // modulo
extern t_typ   t_aka; // ="t_aka"; // alias for
extern t_typ   t_tyd; // ="t_tyd"; // typedef

extern t_typ   t_rmj; // ="t_rmj"; // row-major matrix ordering (C/Lush2 style)
extern t_typ   t_cmj; // ="t_cmj"; // column-major matrix ordering (fortran/R syle)

extern t_typ   t_cro; // ="t_cro"; // coroutine

extern t_typ   t_ifd; // ="t_ifd"; // ifdef
extern t_typ   t_end; // ="t_end"; // endif
extern t_typ   t_dfp; // ="t_dfp"; // pound define

extern t_typ   t_pdf; // ="t_pdf"; // probability density function
extern t_typ   t_cdf; // ="t_cdf"; // cumulative density function

extern t_typ   t_swi; // ="t_swi"; // switch C statement
extern t_typ   t_cas; // ="t_cas"; // case C statement
extern t_typ   t_dft; // ="t_dft"; // default C statement

extern t_typ   t_cls; // ="t_cls"; // C++ class
extern t_typ   t_vrt; // ="t_vrt"; // virtual method
extern t_typ   t_pur; // ="t_pur"; // pure method

extern t_typ   t_pub; // ="t_pub"; // public declaration
extern t_typ   t_prv; // ="t_prv"; // private declaration
extern t_typ   t_pro; // ="t_pro"; // protected section declaration

extern t_typ   t_frd; // ="t_frd"; // friend declaration

extern t_typ   t_lhs; // ="t_lhs"; // left-hand-side
extern t_typ   t_rhs; // ="t_rhs"; // right-hand-side

extern t_typ   t_jid; // ="t_jid"; // job id
extern t_typ   t_usd; // ="t_usd"; // user defined type

extern t_typ   t_eof; // ="t_eof"; // end of file: used as the last token in a token stream.


// math types

extern t_typ   m_eql; // ="eq"; // eq equals
extern t_typ   m_eqe; // ="m_eqe"; // ==

extern t_typ   m_neq; // ="m_neq"; // != not equal
// assignment is t_asi above       // =

extern t_typ   m_bng; // ="m_bng"; // !
extern t_typ   m_mlt; // ="m_mlt"; // *
extern t_typ   m_crt; // ="m_crt"; // ^

extern t_typ   m_dvn; // ="m_dvn"; //  / division
extern t_typ   m_mod; // ="m_mod"; //  % modulo

extern t_typ   m_pls; // ="m_pls"; // +
extern t_typ   m_mns; // ="m_mns"; // -

extern t_typ   m_tld; // ="m_tld"; // ~ tilde

extern t_typ   m_gth; // ="m_gth"; // >
extern t_typ   m_lth; // ="m_lth"; // <

extern t_typ   m_gte; // ="m_gte"; // >=
extern t_typ   m_lte; // ="m_lte"; // <=

extern t_typ   m_peq; // ="m_peq"; // +=
extern t_typ   m_meq; // ="m_meq"; // -=
extern t_typ   m_dve; // ="m_dve"; // /=
extern t_typ   m_mte; // ="m_mte"; // *=
extern t_typ   m_cre; // ="m_cre"; // ^=

extern t_typ   m_ppl; //  ="m_ppl"; // ++
extern t_typ   m_mmn; //  ="m_mmn"; // --

extern t_typ   m_umi; // ="m_umi"; // - unary minus

extern t_typ   m_bar; // ="m_bar"; // |
extern t_typ   m_ats; // ="m_ats"; // @
extern t_typ   m_hsh; // ="m_hsh"; // #
extern t_typ   m_dlr; // ="m_dlr"; // $

extern t_typ   m_pct; // ="m_pct"; // %
extern t_typ   m_cln; // ="m_cln"; // :
extern t_typ   m_que; // ="m_que"; // ?
// keep identifiers together, don't break them up: // extern t_typ   m_dot; // ="m_dot"; // .

extern t_typ   m_lan; // ="m_lan"; // &&  (logical and)
extern t_typ   m_lor; // ="m_lor"; // ||  (logical or)



// protobuf operators/tokens

extern t_typ t_pb_message; //    = "message";
extern t_typ t_pb_import; //     = "import";
extern t_typ t_pb_package; //    = "package";
extern t_typ t_pb_service; //    = "service";
extern t_typ t_pb_rpc; //        = "rpc";
extern t_typ t_pb_extensions; // = "extensions";
extern t_typ t_pb_extend; //     = "extend";
extern t_typ t_pb_option; //     = "option";
extern t_typ t_pb_required; //   = "required";
extern t_typ t_pb_repeated; //   = "repeated";
extern t_typ t_pb_optional; //   = "optional";
extern t_typ t_pb_enum; //       = "enum";

// protobuf types

//extern t_typ t_pby_double;   //   = "double";
//extern t_typ t_pby_float;    //    = "float";
extern t_typ t_pby_int32;    //    = "int32";
extern t_typ t_pby_int64;    //    = "int64";
extern t_typ t_pby_uint32;   //   = "uint32";
extern t_typ t_pby_uint64;   //   = "uint64";
extern t_typ t_pby_sint32;   //   = "sint32";
extern t_typ t_pby_sint64;   //   = "sint64";
extern t_typ t_pby_fixed32;  //  = "fixed32";
extern t_typ t_pby_fixed64;  //  = "fixed64";
extern t_typ t_pby_sfixed32; // = "sfixed32";
extern t_typ t_pby_sfixed64; // = "sfixed64";
//extern t_typ t_pby_bool;     //     = "bool";
extern t_typ t_pby_string;   //   = "string";
extern t_typ t_pby_bytes;    //    = "bytes";

//
// type designators in l3
extern t_typ t_vec; 


extern t_typ t_semicolon;  //  = "t_semicolon";
extern t_typ t_newline;    //  = "t_newline";
extern t_typ t_cppcomment; //  = "t_cppcomment";  // the two character "//" sequence.
extern t_typ t_comma;      //  = "t_comma";


//
// C/C99 reserved words
//
extern t_typ t_c_auto; // = "auto";
extern t_typ t_c_double; // = "double";
extern t_typ t_c_int; // = "int";
extern t_typ t_c_struct; // = "struct";
extern t_typ t_c_break; // = "break";
extern t_typ t_c_else; // = "else";
extern t_typ t_c_long; // = "long";
extern t_typ t_c_switch; // = "switch";
extern t_typ t_c_case; // = "case";
// extern t_typ t_c_enum; // = "enum"; // already in protobuf words above
extern t_typ t_c_register; // = "register";
extern t_typ t_c_typedef; // = "typedef";
extern t_typ t_c_char; // = "char";
extern t_typ t_c_extern; // = "extern";
extern t_typ t_c_return; // = "return";
extern t_typ t_c_union; // = "union";
extern t_typ t_c_const; // = "const";
extern t_typ t_c_float; // = "float";
extern t_typ t_c_short; // = "short";
extern t_typ t_c_unsigned; // = "unsigned";
extern t_typ t_c_continue; // = "continue";
extern t_typ t_c_for; // = "for";
extern t_typ t_c_signed; // = "signed";
extern t_typ t_c_void; // = "void";
extern t_typ t_c_default; // = "default";
extern t_typ t_c_goto; // = "goto";
extern t_typ t_c_sizeof; // = "sizeof";
extern t_typ t_c_volatile; // = "volatile";
extern t_typ t_c_do; // = "do";
extern t_typ t_c_if; // = "if";
extern t_typ t_c_static; // = "static";
extern t_typ t_c_while; // = "while";
extern t_typ t_c__Bool; // = "_Bool";
extern t_typ t_c_inline; // = "inline";
extern t_typ t_c__Complex; // = "_Complex";
extern t_typ t_c_restrict; // = "restrict";
extern t_typ t_c__Imaginary; // = "_Imaginary";


//
// C++ reserved words
//

// let these be atoms, for now.
extern t_typ t_cc_try; // = "try";
extern t_typ t_cc_catch; // = "catch";
extern t_typ t_cc_throw; // = "throw";


extern t_typ t_cc_asm; // = "asm";
extern t_typ t_cc_dynamic_cast; // = "dynamic_cast";
extern t_typ t_cc_namespace; // = "namespace";
extern t_typ t_cc_reinterpret_cast; // = "reinterpret_cast";

extern t_typ t_cc_bool; // = "bool";
extern t_typ t_cc_explicit; // = "explicit";
extern t_typ t_cc_new; // = "new";
extern t_typ t_cc_static_cast; // = "static_cast";
extern t_typ t_cc_typeid; // = "typeid";
extern t_typ t_cc_false; // = "false";
extern t_typ t_cc_operator; // = "operator";
extern t_typ t_cc_template; // = "template";
extern t_typ t_cc_typename; // = "typename";
extern t_typ t_cc_class; // = "class";
extern t_typ t_cc_friend; // = "friend";
extern t_typ t_cc_private; // = "private";
extern t_typ t_cc_this; // = "this";
extern t_typ t_cc_using; // = "using";
extern t_typ t_cc_const_cast; // = "const_cast";
extern t_typ t_cc_inline; // = "inline";
extern t_typ t_cc_public; // = "public";
extern t_typ t_cc_virtual; // = "virtual";
extern t_typ t_cc_delete; // = "delete";
extern t_typ t_cc_mutable; // = "mutable";
extern t_typ t_cc_protected; // = "protected";
extern t_typ t_cc_true; // = "true";
extern t_typ t_cc_wchar_t; // = "wchar_t";
extern t_typ t_cc_and; // = "and";
extern t_typ t_cc_bitand; // = "bitand";
extern t_typ t_cc_compl; // = "compl";
extern t_typ t_cc_not_eq; // = "not_eq";
extern t_typ t_cc_or_eq; // = "or_eq";
extern t_typ t_cc_xor_eq; // = "xor_eq";
extern t_typ t_cc_and_eq; // = "and_eq";
extern t_typ t_cc_bitor; // = "bitor";
extern t_typ t_cc_not; // = "not";
extern t_typ t_cc_or; // = "or";
extern t_typ t_cc_xor; // = "xor";



#endif /* QUICKTYPE_H */
