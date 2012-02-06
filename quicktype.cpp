//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

// quick type system
#include "quicktype.h"
#include <string.h>
#include "jlmap.h"

#include "l3obj.h"
#include "autotag.h"
#include "terp.h"
#include "l3pratt.h"

/////////////////////// end of includes

t_typ   t_qty   ="t_qty"; // struct quicktype, to describe other types at runtime.
t_typ   t_qxp   ="t_qxp"; // struct qexp, the structure of the tree.
t_typ   t_tdo   ="t_tdo"; // top-down operator precedence parsing output.


// from l3ts.proto
t_typ   t_qts   ="t_qts"; // l3ts::qts

t_typ   t_nv3   ="t_nv3"; // l3ts::nameval
t_typ   t_ob3   ="t_ob3"; // l3ts::object
t_typ   t_tm3   ="t_tm3"; // l3ts::timept

t_typ   t_gl3   ="t_gl3"; // l3ts::geoloc
t_typ   t_tp3   ="t_tp3"; // l3ts::timeplace
t_typ   t_da3   ="t_da3"; // l3ts::datapt

t_typ   t_ts3   ="t_ts3"; // l3ts::timeseries
t_typ   t_be3   ="t_be3"; // l3ts::BegEnd
t_typ   t_si3   ="t_si3"; // l3ts::SampleSpec

t_typ   t_bs3   ="t_bs3"; // l3ts::BeginEndSet
t_typ   t_wh3   ="t_wh3"; // l3ts::When
t_typ   t_id3   ="t_id3"; // l3ts::TSID

t_typ   t_ds3   ="t_ds3"; // l3ts::TSDesc
t_typ   t_he3   ="t_he3"; // l3ts::pb_msg_header
t_typ   t_qt3   ="t_qt3"; // l3ts::qtreepb
t_typ   t_qtr   ="t_qtr"; // qtree


t_typ   t_cmd   ="t_cmd"; // a command / function invocation.
t_typ   t_pcd   ="t_pcd"; // a parse_cmd struct

t_typ   u_pdata ="parse_data_t"; 
t_typ   u_sexpt ="sexp_t";
t_typ   u_pcont_t="pcont_t";

t_typ   t_ded ="t_ded"; // dead object.
t_typ   t_ddr ="t_ddr"; // dd* refs
t_typ   t_ust ="t_ust"; // a type set, or compound type chain;  ustaq<char> that points to t_typ

t_typ   t_ato ="t_ato"; // s-expression atom

t_typ   t_dou ="t_dou"; // doubles
t_typ   t_str ="t_str"; // strings
t_typ   t_obj ="t_obj"; // objects
t_typ   t_llr ="t_llr"; // llref
t_typ   t_ull ="t_ull"; // ustaq<T>::ll
t_typ   t_lnk ="t_lnk"; // lnk
t_typ   t_lin ="t_lin"; // link

t_typ   t_ddt ="t_ddt"; // double linked list

t_typ   t_fun ="t_fun"; // functions
t_typ   t_env ="t_env"; // envs are really just hash tables
t_typ   t_lst ="t_lst"; // lists
t_typ   t_dsq ="t_dsq"; // dstaq lists
t_typ   t_dsd ="t_dsd"; // dstaq.h template dstaq class, no wrapper.

t_typ   t_mqq ="t_mqq"; // message q / exception q

t_typ   t_cnd ="t_cnd"; // condition
t_typ   t_lit ="t_lit"; // literal keyword/function name (like a string, not double quoted).

t_typ   t_par ="t_par"; // formal parameter list
t_typ   t_arg ="t_arg"; // argument to a function
t_typ   t_ret ="t_ret"; // return value from a function

t_typ   t_vvc ="t_vvc"; // pointer to resolved values vector, a vector of l3obj*

t_typ   t_vec ="vec"; // the 'vec' keyword in a source file
t_typ   t_sxp ="t_sxp"; // s-expression type, an l3obj-wrapped sexpobj_struct 
t_typ   t_cal ="t_cal"; // call parameters for actual function invocations

// quick type system II, may or may not be implemented as need be. But these
//  type identifiers are reserved.

t_typ   t_any ="t_any"; // any type, will be infered; not specified (allows any type)
t_typ   t_tyy ="t_tyy"; // a type, one of these char[] pointers.
t_typ   t_nil ="t_nil"; // nil, like void in C, an empty list, nothing here, nothing returned, nothing in.

t_typ   t_set ="t_set"; // set
t_typ   t_map ="t_map"; // map
t_typ   t_mcr ="t_mcr"; // macro

t_typ   t_quo ="t_quo"; // quoted expression
t_typ   t_dfr ="t_dfr"; // data frame
t_typ   t_mat ="t_mat"; // matrix

t_typ   t_dag ="t_dag"; // directed acyclic graph
t_typ   t_blb ="t_blb"; // blob, binary large object
t_typ   t_vop ="t_vop"; // void* in C, a generic pointer, one word in size

t_typ   t_nls ="t_nls"; // named lists, like dynamic structs- just like R
t_typ   t_sct ="t_sct"; // a C struct
t_typ   t_enu ="t_enu"; // a C enum

t_typ   t_pid ="t_pid"; // a process handle
t_typ   t_tid ="t_tid"; // a thread handle
t_typ   t_sym ="t_sym"; // a symbol 
t_typ   t_syv ="t_syv"; // a symbol vector; see symvec.h

t_typ   t_usr ="t_usr"; // username
t_typ   t_ppd ="t_ppd"; // parent process id

t_typ   t_src ="t_src"; // source code, compilable, textual form
t_typ   t_cod ="t_cod"; // compiled, executable code.
t_typ   t_lab ="t_lab"; // a pre-computed goto label; a machine address to jump to.

t_typ   t_int ="t_int"; // 32bit integer; a C short
t_typ   t_lng ="t_lng"; // 64bit integer; a C long
t_typ   t_byt ="t_byt"; // an 8-bit byte stream (binary large or small object).

t_typ   t_chr ="t_chr"; // C character
t_typ   t_utc ="t_utc"; // a single utf8 unicode character
t_typ   t_ut8 ="t_ut8"; // utf8 string
t_typ   t_q3q ="t_q3q"; // triple quoted string
t_typ   t_sqo ="t_sqo"; // a single quote character

// function sub-types
t_typ   t_thk ="t_thk"; // a thunk: a function that takes no arguments and returns no arugments, run purely for side effects.
t_typ   t_uni ="t_uni"; // a universal function: void* univ(void*, void*);
t_typ   t_cst ="t_cst"; // a casting function:  takes an object of one type and turns it into another type.  <t_typ> caster(<t_typ> in_obj)

t_typ   t_ano ="t_ano"; // an annotation of existing functions, types, code, with extensible, user-defined properties, that are machine parsable.
t_typ   t_tpl ="t_tpl"; // a C++ style template
t_typ   t_tag ="t_tag"; // a memory ownership tag.
t_typ   t_cap ="t_cap"; // a cap-tag pair, this being the captain.

t_typ   t_knd ="t_knd"; // a kind (type of type)
t_typ   t_seq ="t_seq"; // a sequence (or or more, not necessarily a list or vector because may be lazily supplied)

t_typ   t_doc ="t_doc"; // documentation (text description)
t_typ   t_key ="t_key"; // a key in a map (abstract type)
t_typ   t_val ="t_val"; // a value in a map (abstract type)

// quick type system III (brainstorm, probably dreamin' )

t_typ   t_box ="t_box"; // a mailbox 
t_typ   t_msg ="t_msg"; // a message from the mailbox
t_typ   t_ccc ="t_ccc"; // complex number with 64-bit double real and 64-bit double imaginary parts.

t_typ   t_flt ="t_flt"; // a 32-bit floating point number
t_typ   t_sql ="t_sql"; // a sql query
t_typ   t_ver ="t_ver"; // a version identifier (abstract)

t_typ   t_uid ="t_uid"; // a UUID identifer
t_typ   t_laz ="t_laz"; // a lazy type (abstract)
t_typ   t_frc ="t_frc"; // a forced/immediate type (abstract, opposite of lazy)

t_typ   t_imm ="t_imm"; // an immutable type
t_typ   t_con ="t_con"; // a constant type
t_typ   t_vol ="t_vol"; // a volatile type

t_typ   t_ref ="t_ref"; // pass-by-reference
t_typ   t_vlu ="t_vlu"; // pass-by-value

t_typ   t_stc ="t_stc"; // static alloation/keyword
t_typ   t_aut ="t_aut"; // auto (stack based) allocation

t_typ   t_ifc ="t_ifc"; // an interface
t_typ   t_sup ="t_sup"; // a super class (base class)
t_typ   t_der ="t_der"; // a derived class (child class)

t_typ   t_ptr ="t_ptr"; // a pointer
t_typ   t_adr ="t_adr"; // an address
t_typ   t_com ="t_com"; // a src code comment

t_typ   t_rtm ="t_rtm"; // a run time macro

t_typ   t_exp ="t_exp"; // an expression (abstract type)
t_typ   t_stm ="t_stm"; // a statement (abstract type)
t_typ   t_for ="t_for"; // a for loop / foreach statement (abstract type)

t_typ   t_ifs ="t_ifs"; // an if-then-else statement
t_typ   t_whi ="t_whi"; // a while loop
t_typ   t_exc ="t_exc"; // an exception

t_typ   t_thw ="t_thw"; // a throw statement
t_typ   t_cat ="t_cat"; // a catch statement
t_typ   t_fin ="t_fin"; // a finally statement

t_typ   t_iso ="t_iso"; // in_server_owns type 
t_typ   t_ico ="t_ico"; // in_client_owns type
t_typ   t_oso ="t_oso"; // out_server_owns type
t_typ   t_oco ="t_oco"; // out_client_owns type

t_typ   t_reg ="t_reg"; // a register type
t_typ   t_asi ="t_asi"; // an assignement statement
t_typ   t_boo ="t_boo"; // boolean typ
t_typ   t_boe ="t_boe"; // boolean expression

t_typ   t_pbm ="t_pbm"; // a protocol buffer message
t_typ   t_pbi ="t_pbi"; // a protocol buffer interface
t_typ   t_hdr ="t_hdr"; // a header file

t_typ   t_mdl ="t_mdl"; // a module of classes and functions and types
t_typ   t_flh ="t_flh"; // a file handle
t_typ   t_srm ="t_srm"; // a C++ stream (abstract)

t_typ   t_osm ="t_osm"; // a C++ output stream
t_typ   t_ism ="t_ism"; // a C++ input stream
t_typ   t_imp ="t_imp"; // an import statement, D-style import of module.

t_typ   t_inc ="t_inc"; // an include statement, for C style header include.
t_typ   t_oss ="t_oss"; // C++ ostringstream
t_typ   t_iss ="t_iss"; // C++ istringstream

t_typ   t_cpp ="t_cpp"; // inline C++ code
t_typ   t_lsp ="t_lsp"; // inline lisp code
t_typ   t_lsc ="t_lsc"; // inline lisp code, starting with (:   ...ending with )

t_typ   t_ftr ="t_ftr"; // inline fortran code

t_typ   t_jav ="t_jav"; // inline java code
t_typ   t_ocm ="t_ocm"; // inline ocaml code
t_typ   t_prl ="t_prl"; // inline perl code

t_typ   t_skt ="t_skt"; // unix socket
t_typ   t_pth ="t_pth"; // filesystem path
t_typ   t_dir ="t_dir"; // filesystem directory

t_typ   t_pip ="t_pip"; // unix pipe
t_typ   t_lck ="t_lck"; // lock (mutex)
t_typ   t_cvr ="t_cvr"; // condition variable

t_typ   t_htm ="t_htm"; // html text
t_typ   t_arf ="t_arf"; // aref hyperlink anchor
t_typ   t_url ="t_url"; // url

t_typ   t_git ="t_git"; // git repository
t_typ   t_scm ="t_scm"; // inline scheme code.

t_typ   t_brk ="t_brk"; // breakpoint
t_typ   t_lnn ="t_lnn"; // line number
t_typ   t_srf ="t_srf"; // source file (pathname)x

t_typ   t_shm ="t_shm"; // shared memory segment
t_typ   t_gph ="t_gph"; // graph

t_typ   t_wnd ="t_wnd"; // windowing system window
t_typ   t_dia ="t_dia"; // dialog box

t_typ   t_png ="t_png"; // PNG image
t_typ   t_jpg ="t_jpg"; // JPEG image

t_typ   t_arr ="t_arr"; // multi-dimenstional array
t_typ   t_lib ="t_lib"; // static library handle
t_typ   t_dll ="t_dll"; // dynamic linked library (.so) file handle

t_typ   t_siz ="t_siz"; // size_t C type (number of bytes)
t_typ   t_lvm ="t_lvm"; // an LLVM type
t_typ   t_lsh ="t_lsh"; // a Lush2 type

t_typ   t_new ="t_new"; // new C++ statement
t_typ   t_del ="t_del"; // delete C++ statement

t_typ   t_mlc ="t_mlc"; // malloc C statement
t_typ   t_fre ="t_fre"; // free C statement

t_typ   t_rea ="t_rea"; // real number type
t_typ   t_bgi ="t_bgi"; // big integer type (unlimited precision)
t_typ   t_mp3 ="t_mp3"; // mp3 file handle

t_typ   t_mpg ="t_mpg"; // mpeg file handle

t_typ   t_dat ="t_dat"; // date / timestamp
t_typ   t_tim ="t_tim"; // timestamp

t_typ   t_trv ="t_trv"; // tree view
t_typ   t_lbx ="t_lbx"; // list box
t_typ   t_mus ="t_mus"; // mouse position

t_typ   t_quu ="t_quu"; // queue type/fifo
t_typ   t_pop ="t_pop"; // pop  operation
t_typ   t_psh ="t_psh"; // push operation

t_typ   t_asm ="t_asm"; // inline assembly
t_typ   t_clo ="t_clo"; // closure
t_typ   t_tls ="t_tls"; // thread local storage

t_typ   t_tyl ="t_tyl"; // type list (list of t_typ attributes)
t_typ   t_wea ="t_wea"; // weak reference
t_typ   t_stg ="t_stg"; // strong reference

t_typ   t_dcl ="t_dcl"; // declaration
t_typ   t_dfn ="t_dfn"; // definition
t_typ   t_uns ="t_uns"; // unsigned (natural number)

t_typ   t_jdy ="t_jdy"; // Judy array
t_typ   t_jdl ="t_jdl"; // JudyL array
t_typ   t_jds ="t_jds"; // JudyS array

t_typ   t_ast ="t_ast"; // abstract syntax tree type.
t_typ   t_sig ="t_sig"; // unix signal

t_typ   t_opn ="t_opn"; // open paren
t_typ   t_cpn ="t_cpn"; // close paren

t_typ   t_obr ="t_obr"; // open brace {
t_typ   t_cbr ="t_cbr"; // close brace }

t_typ   t_osq ="t_osq"; // open square bracket [
t_typ   t_csq ="t_csq"; // close square brakcet ]

t_typ   t_idx ="t_idx"; // Lush2 index 
t_typ   t_ind ="t_ind"; // index operatation in [] square brackets

t_typ   t_let ="t_let"; // let statement
t_typ   t_lda ="t_lda"; // lambda statement

t_typ   t_fst ="t_fst"; // first (car)
t_typ   t_rst ="t_rst"; // rest (cdr)

t_typ   t_ctr ="t_ctr"; // constructor
t_typ   t_dtr ="t_dtr"; // destructor
t_typ   t_sco ="t_sco"; // scope statement (like D)

t_typ   t_nms ="t_nms"; // namespace
t_typ   t_fbn ="t_fbn"; // forbidden
t_typ   t_dpc ="t_dpc"; // deprecated

t_typ   t_dst ="t_dst"; // probability distribution

t_typ   t_den ="t_den"; // probablity density
t_typ   t_hst ="t_hst"; // histogram

t_typ   t_ncr ="t_ncr"; // increment statement
t_typ   t_dcr ="t_dcr"; // decrement statement
t_typ   t_evl ="t_evl"; // take value of, eval statement.

t_typ   t_cts ="t_cts"; // continuation
t_typ   t_hea ="t_hea"; // heap allocated memory
t_typ   t_stk ="t_stk"; // stack allocated memory

t_typ   t_bit ="t_bit"; // bitwise operation
// redundant with m_bar:  t_typ   t_bor ="t_bor"; // bitwise or
t_typ   t_xor ="t_xor"; // bitwise xor

t_typ   t_and ="t_and"; // bitwise and
t_typ   t_ndn ="t_ndn"; // bitwise nand
t_typ   t_bno ="t_bno"; // bitwise not

t_typ   t_lor ="t_lor"; // logical or
t_typ   t_lan ="t_lan"; // logical and
t_typ   t_not ="t_not"; // logical not

t_typ   t_tru ="t_tru"; // boolean true
t_typ   t_fal ="t_fal"; // boolena false

t_typ   t_nan ="t_nan"; // quiet nan
t_typ   t_nav ="t_nav"; // not available (missing data)

t_typ   t_inf ="t_inf"; // +infinity
t_typ   t_nnf ="t_nnf"; // -infinity

t_typ   t_zer ="t_zer"; // zero type (zeroed out memory allocation to start with).
t_typ   t_ini ="t_ini"; // initializer

t_typ   t_fix ="t_fix"; // fixed size data structure
t_typ   t_vrb ="t_vrb"; // variable sized data structure

t_typ   t_mix ="t_mix"; // mixin D style.
t_typ   t_dmd ="t_dmd"; // D code in line.
t_typ   t_dim ="t_dim"; // dimensions (or array)

t_typ   t_jmp ="t_jmp"; // jump statement

t_typ   t_alg ="t_alg"; // algebraic/math/floating point operation
t_typ   t_add ="t_add"; // addition
t_typ   t_sub ="t_sub"; // subtraction

t_typ   t_mul ="t_mul"; // multiplication
t_typ   t_div ="t_div"; // division

t_typ   t_mod ="t_mod"; // modulo
t_typ   t_aka ="t_aka"; // alias for
t_typ   t_tyd ="t_tyd"; // typedef

t_typ   t_rmj ="t_rmj"; // row-major matrix ordering (C/Lush2 style)
t_typ   t_cmj ="t_cmj"; // column-major matrix ordering (fortran/R syle)

t_typ   t_cro ="t_cro"; // coroutine

t_typ   t_ifd ="t_ifd"; // ifdef
t_typ   t_end ="t_end"; // endif
t_typ   t_dfp ="t_dfp"; // pound define

t_typ   t_pdf ="t_pdf"; // probability density function
t_typ   t_cdf ="t_cdf"; // cumulative density function

t_typ   t_swi ="t_swi"; // switch C statement
t_typ   t_cas ="t_cas"; // case C statement
t_typ   t_dft ="t_dft"; // default C statement

t_typ   t_cls ="t_cls"; // C++ class
t_typ   t_vrt ="t_vrt"; // virtual method
t_typ   t_pur ="t_pur"; // pure method

t_typ   t_pub ="t_pub"; // public declaration
t_typ   t_prv ="t_prv"; // private declaration
t_typ   t_pro ="t_pro"; // protected section declaration

t_typ   t_frd ="t_frd"; // friend declaration

t_typ   t_lhs ="t_lhs"; // left-hand-side
t_typ   t_rhs ="t_rhs"; // right-hand-side

t_typ   t_jid ="t_jid"; // job id
t_typ   t_usd ="t_usd"; // user defined type

t_typ   t_eof ="t_eof"; // end of file: used as the last token in a token stream.

// math types

t_typ   m_eql ="eq"; // eq equals
t_typ   m_eqe ="m_eqe"; // ==
t_typ   m_neq ="m_neq"; // != not equal
// assignment is t_asi above       // =

t_typ   m_bng ="m_bng"; // !
t_typ   m_mlt ="m_mlt"; // *
t_typ   m_crt ="m_crt"; // ^

t_typ   m_dvn ="m_dvn"; //  / division
t_typ   m_mod ="m_mod"; //  % modulo

t_typ   m_pls ="m_pls"; // +
t_typ   m_mns ="m_mns"; // -

t_typ   m_tld ="m_tld"; // ~ tilde

t_typ   m_gth ="m_gth"; // >
t_typ   m_lth ="m_lth"; // <

t_typ   m_gte ="m_gte"; // >=
t_typ   m_lte ="m_lte"; // <=

t_typ   m_peq ="m_peq"; // +=
t_typ   m_meq ="m_meq"; // -=
t_typ   m_dve ="m_dve"; // /=
t_typ   m_mte ="m_mte"; // *=
t_typ   m_cre ="m_cre"; // ^=

t_typ   m_ppl ="m_ppl"; // ++
t_typ   m_mmn ="m_mmn"; // --

t_typ   m_umi ="m_umi"; // - unary minus

t_typ   m_bar ="m_bar"; // |
t_typ   m_ats ="m_ats"; // @
t_typ   m_hsh ="m_hsh"; // #
t_typ   m_dlr ="m_dlr"; // $

t_typ   m_pct ="m_pct"; // %
t_typ   m_cln ="m_cln"; // :
t_typ   m_que ="m_que"; // ?
// keep identifiers together, don't break them up: t_typ   m_dot ="m_dot"; // .

t_typ   m_lan ="m_lan"; // &&  (logical and)
t_typ   m_lor ="m_lor"; // ||  (logical or)




// protobuf operators/tokens

t_typ t_pb_message    = "message";
t_typ t_pb_import     = "import";
t_typ t_pb_package    = "package";
t_typ t_pb_service    = "service";
t_typ t_pb_rpc        = "rpc";
t_typ t_pb_extensions = "extensions";
t_typ t_pb_extend     = "extend";
t_typ t_pb_option     = "option";
t_typ t_pb_required   = "required";
t_typ t_pb_repeated   = "repeated";
t_typ t_pb_optional   = "optional";
t_typ t_pb_enum       = "enum";

// pb types

//t_typ t_pby_double   = "double";
//t_typ t_pby_float    = "float";
t_typ t_pby_int32    = "int32";
t_typ t_pby_int64    = "int64";
t_typ t_pby_uint32   = "uint32";
t_typ t_pby_uint64   = "uint64";
t_typ t_pby_sint32   = "sint32";
t_typ t_pby_sint64   = "sint64";
t_typ t_pby_fixed32  = "fixed32";
t_typ t_pby_fixed64  = "fixed64";
t_typ t_pby_sfixed32 = "sfixed32";
t_typ t_pby_sfixed64 = "sfixed64";
//t_typ t_pby_bool     = "bool";
t_typ t_pby_string   = "string";
t_typ t_pby_bytes    = "bytes";


t_typ t_semicolon  = "t_semicolon";
t_typ t_newline    = "t_newline";
t_typ t_cppcomment = "t_cppcomment"; // the two character "//" sequence.
t_typ t_comma      = "t_comma";


//
// C/C99 reserved words
//
t_typ t_c_auto = "auto";
t_typ t_c_double = "double";
t_typ t_c_int = "int";
t_typ t_c_struct = "struct";
t_typ t_c_break = "break";
t_typ t_c_else = "else";
t_typ t_c_long = "long";
t_typ t_c_switch = "switch";
t_typ t_c_case = "case";
// t_typ t_c_enum = "enum"; // already in protobuf words above
t_typ t_c_register = "register";
t_typ t_c_typedef = "typedef";
t_typ t_c_char = "char";
t_typ t_c_extern = "extern";
t_typ t_c_return = "return";
t_typ t_c_union = "union";
t_typ t_c_const = "const";
t_typ t_c_float = "float";
t_typ t_c_short = "short";
t_typ t_c_unsigned = "unsigned";
t_typ t_c_continue = "continue";
t_typ t_c_for = "for";
t_typ t_c_signed = "signed";
t_typ t_c_void = "void";
t_typ t_c_default = "default";
t_typ t_c_goto = "goto";
t_typ t_c_sizeof = "sizeof";
t_typ t_c_volatile = "volatile";
t_typ t_c_do = "do";
t_typ t_c_if = "if";
t_typ t_c_static = "static";
t_typ t_c_while = "while";
t_typ t_c__Bool = "_Bool";
t_typ t_c_inline = "inline";
t_typ t_c__Complex = "_Complex";
t_typ t_c_restrict = "restrict";
t_typ t_c__Imaginary = "_Imaginary";


//
// C++ reserved words
//

// let these be atoms, for now.
t_typ t_cc_try = "try";
t_typ t_cc_catch = "catch";
t_typ t_cc_throw = "throw";


t_typ t_cc_asm = "asm";
t_typ t_cc_dynamic_cast = "dynamic_cast";
t_typ t_cc_namespace = "namespace";
t_typ t_cc_reinterpret_cast = "reinterpret_cast";
t_typ t_cc_bool = "bool";
t_typ t_cc_explicit = "explicit";
t_typ t_cc_new = "new";
t_typ t_cc_static_cast = "static_cast";
t_typ t_cc_typeid = "typeid";
t_typ t_cc_false = "false";
t_typ t_cc_operator = "operator";
t_typ t_cc_template = "template";
t_typ t_cc_typename = "typename";
t_typ t_cc_class = "class";
t_typ t_cc_friend = "friend";
t_typ t_cc_private = "private";
t_typ t_cc_this = "this";
t_typ t_cc_using = "using";
t_typ t_cc_const_cast = "const_cast";

t_typ t_cc_public = "public";
t_typ t_cc_virtual = "virtual";
t_typ t_cc_delete = "delete";
t_typ t_cc_mutable = "mutable";
t_typ t_cc_protected = "protected";
t_typ t_cc_true = "true";
t_typ t_cc_wchar_t = "wchar_t";
t_typ t_cc_and = "and";
t_typ t_cc_bitand = "bitand";
t_typ t_cc_compl = "compl";
t_typ t_cc_not_eq = "not_eq";
t_typ t_cc_or_eq = "or_eq";
t_typ t_cc_xor_eq = "xor_eq";
t_typ t_cc_and_eq = "and_eq";
t_typ t_cc_bitor = "bitor";
t_typ t_cc_not = "not";
t_typ t_cc_or = "or";
t_typ t_cc_xor = "xor";


// gotta figure out how to automatically maintain this list.

struct qtype_table {
    t_typ  _ty;
    ptr2method  _print;
    ptr2method  _ctor;
    ptr2method  _cpctor;
    ptr2method  _dtor;

    ptr2method  _save;
    ptr2method  _load;

    ptr2new_qtree  _qtreenew;
    ptr2method     _evalfun;  // the function to dispatch to... passing the qtree* as exp.
    ptr2method  _placeholder1;

};

//const char* valid_t_typ[] = {
qtype_table  valid_t_typ[] = {

    {t_qty, 0,0,0,0,  0,0,0,0,0 },

    {t_qxp, 0,0,0,0,  0,0,0,0,0 },
    {t_tdo, 0,0,0,0,  &tdo_save,&tdo_load,0,0,0 },

    // l3ts.proto
    {t_qts, &qts_print,  &qts, &qts_cpctor, &qts_dtor, &qts_save, &qts_load, 0,0,0 },

    {t_nv3, &nv3_print,  &nv3, &nv3_cpctor, &nv3_dtor, &nv3_save, &nv3_load, 0,0,0 },
    {t_ob3, &ob3_print,  &ob3, &ob3_cpctor, &ob3_dtor, &ob3_save, &ob3_load, 0,0,0 },
    {t_tm3, &tm3_print,  &tm3, &tm3_cpctor, &tm3_dtor, &tm3_save, &tm3_load, 0,0,0 },

    {t_gl3, &gl3_print,  &gl3, &gl3_cpctor, &gl3_dtor, &gl3_save, &gl3_load, 0,0,0 },
    {t_tp3, &tp3_print,  &tp3, &tp3_cpctor, &tp3_dtor, &tp3_save, &tp3_load, 0,0,0 },
    {t_da3, &da3_print,  &da3, &da3_cpctor, &da3_dtor, &da3_save, &da3_load, 0,0,0 },

    {t_ts3, &ts3_print,  &ts3, &ts3_cpctor, &ts3_dtor, &ts3_save, &ts3_load, 0,0,0 },
    {t_be3, &be3_print,  &be3, &be3_cpctor, &be3_dtor, &be3_save, &be3_load, 0,0,0 },
    {t_si3, &si3_print,  &si3, &si3_cpctor, &si3_dtor, &si3_save, &si3_load, 0,0,0 },

    {t_bs3, &bs3_print,  &bs3, &bs3_cpctor, &bs3_dtor, &bs3_save, &bs3_load, 0,0,0 },
    {t_wh3, &wh3_print,  &wh3, &wh3_cpctor, &wh3_dtor, &wh3_save, &wh3_load, 0,0,0 },
    {t_id3, &id3_print,  &id3, &id3_cpctor, &id3_dtor, &id3_save, &id3_load, 0,0,0 },

    {t_ds3, &ds3_print,  &ds3, &ds3_cpctor, &ds3_dtor, &ds3_save, &ds3_load, 0,0,0 },
    {t_he3, &he3_print,  &he3, &he3_cpctor, &he3_dtor, &he3_save, &he3_load, 0,0,0 },
    {t_qt3, &qt3_print,  &qt3, &qt3_cpctor, &qt3_dtor, &qt3_save, &qt3_load, 0,0,0 },


    // user defined types
    {u_pdata,   0,0,0,0,  0,0,0,0,0 },
    {u_sexpt,   0,0,0,0,  0,0,0,0,0 },
    {u_pcont_t, 0,0,0,0,  0,0,0,0,0 },

    // command /function invocation on command line.
    {t_cmd,     0,0,0,0,  0,0,&new_t_cmd,0,0 },

    // a parse_cmd struct
    {t_pcd,     0,0,0,0,  0,0,0,0,0 },

    // system builtin / nameval subtypes
    {t_dou, 0,0,0,0,  &dou_save, &dou_load, &new_t_dou,0,0 },
    {t_str, 0,0,0,0,  &str_save, &str_load, &new_t_ato,0,0 }, // treat as atom.
    {t_obj, 0,0,0,0,  &obj_save, &ob3_load, 0,0,0 },
    {t_lng, 0,0,0,0,  &lng_save, &lng_load, 0,0,0 },
    {t_byt, 0,0,0,0,  &byt_save, &byt_load, 0,0,0 },
    {t_vvc, 0,0,0,0,  &vvc_save, &vvc_load, 0,0,0 },

    // system pre-defined types
    {t_ded, 0,0,0,0,  0,0,0,0,0 },
    {t_ddr, 0,0,0,0,  0,0,0,0,0 },
    {t_ust, 0,0,0,0,  0,0,0,0,0 },
    {t_ato, 0,0,0,0,  0,0,&new_t_ato,0,0 },
    {t_boe, 0,0,0,0,  0,0,0,0,0 },
    {t_cnd, 0,0,0,0,  0,0,0,0,0 },
    {t_lit, 0,0,0,0,  0,0,0,0,0 },
    {t_llr, 0,0,0,0,  0,0,0,0,0 },
    {t_ull, 0,0,0,0,  0,0,0,0,0 },
    {t_lnk, 0,0,0,0,  0,0,0,0,0 },
    {t_lin, 0,0,0,0,  0,0,0,0,0 },
    {t_ddt, 0,0,0,0,  0,0,0,0,0 },
    {t_fun, 0,0,0,0,  0,0,0,0,0 },
    {t_env, 0,0,0,0,  0,0,0,0,0 },
    {t_lst, 0,0,0,0,  0,0,0,0,0 },
    {t_dsq, 0,0,0,0,  0,0,0,0,0 },
    {t_mqq, 0,0,0,0,  0,0,0,0,0 },
    {t_dsd, 0,0,0,0,  0,0,0,0,0 },
    {t_par, 0,0,0,0,  0,0,0,0,0 },
    {t_arg, 0,0,0,0,  0,0,0,0,0 },
    {t_ret, 0,0,0,0,  0,0,0,0,0 },
    {t_vec, 0,0,0,0,  0,0,&new_t_vec,0,0 },
    {t_sxp, 0,0,0,0,  0,0,0,0,0 },
    {t_cal, 0,0,0,0,  0,0,0,0,0 },
    {t_any, 0,0,0,0,  0,0,&new_t_any,0,0 },
    {t_tyy, 0,0,0,0,  0,0,0,0,0 },
    {t_nil, 0,0,0,0,  0,0,0,0,0 },
    {t_set, 0,0,0,0,  0,0,0,0,0 },
    {t_map, 0,0,0,0,  0,0,0,0,0 },
    {t_mcr, 0,0,0,0,  0,0,0,0,0 },
    {t_quo, 0,0,0,0,  0,0,0,0,0 },
    {t_dfr, 0,0,0,0,  0,0,0,0,0 },
    {t_mat, 0,0,0,0,  0,0,0,0,0 },
    {t_dag, 0,0,0,0,  0,0,0,0,0 },
    {t_blb, 0,0,0,0,  0,0,0,0,0 },
    {t_vop, 0,0,0,0,  0,0,0,0,0 },
    {t_nls, 0,0,0,0,  0,0,0,0,0 },
    {t_lab, 0,0,0,0,  0,0,0,0,0 },
    {t_pid, 0,0,0,0,  0,0,0,0,0 },
    {t_tid, 0,0,0,0,  0,0,0,0,0 },
    {t_sym, 0,0,0,0,  0,0,0,0,0 },
    {t_syv, 0,0,0,0,  0,0,0,0,0 },
    {t_usr, 0,0,0,0,  0,0,0,0,0 },
    {t_ppd, 0,0,0,0,  0,0,0,0,0 },
    {t_src, 0,0,0,0,  0,0,0,0,0 },
    {t_cod, 0,0,0,0,  0,0,0,0,0 },
    {t_int, 0,0,0,0,  0,0,0,0,0 },
    {t_chr, 0,0,0,0,  0,0,0,0,0 },
    {t_utc, 0,0,0,0,  0,0,0,0,0 },
    {t_ut8, 0,0,0,0,  0,0,0,0,0 },
    {t_q3q, 0,0,0,0,  0,0,0,0,0 },
    {t_sqo, 0,0,0,0,  0,0,0,0,0 },
    {t_thk, 0,0,0,0,  0,0,0,0,0 },
    {t_uni, 0,0,0,0,  0,0,0,0,0 },
    {t_cst, 0,0,0,0,  0,0,0,0,0 },
    {t_ano, 0,0,0,0,  0,0,0,0,0 },
    {t_tpl, 0,0,0,0,  0,0,0,0,0 },
    {t_tag, 0,0,0,0,  0,0,0,0,0 },
    {t_cap, 0,0,0,0,  0,0,0,0,0 },
    {t_knd, 0,0,0,0,  0,0,0,0,0 },
    {t_seq, 0,0,0,0,  0,0,0,0,0 },
    {t_ref, 0,0,0,0,  0,0,&new_t_ref,0,0 },
    {t_vlu, 0,0,0,0,  0,0,0,0,0 },
    {t_doc, 0,0,0,0,  0,0,0,0,0 },
    {t_key, 0,0,0,0,  0,0,0,0,0 },
    {t_val, 0,0,0,0,  0,0,0,0,0 },
    {t_box, 0,0,0,0,  0,0,0,0,0 },
    {t_msg, 0,0,0,0,  0,0,0,0,0 },
    {t_ccc, 0,0,0,0,  0,0,0,0,0 },
    {t_flt, 0,0,0,0,  0,0,0,0,0 },
    {t_sql, 0,0,0,0,  0,0,0,0,0 },
    {t_ver, 0,0,0,0,  0,0,0,0,0 },
    {t_uid, 0,0,0,0,  0,0,0,0,0 },
    {t_laz, 0,0,0,0,  0,0,0,0,0 },
    {t_frc, 0,0,0,0,  0,0,0,0,0 },
    {t_imm, 0,0,0,0,  0,0,0,0,0 },
    {t_con, 0,0,0,0,  0,0,0,0,0 },
    {t_vol, 0,0,0,0,  0,0,0,0,0 },
    {t_stc, 0,0,0,0,  0,0,0,0,0 },
    {t_aut, 0,0,0,0,  0,0,0,0,0 },
    {t_ifc, 0,0,0,0,  0,0,0,0,0 },
    {t_sup, 0,0,0,0,  0,0,0,0,0 },
    {t_der, 0,0,0,0,  0,0,0,0,0 },
    {t_ptr, 0,0,0,0,  0,0,0,0,0 },
    {t_adr, 0,0,0,0,  0,0,0,0,0 },
    {t_com, 0,0,0,0,  0,0,0,0,0 },
    {t_rtm, 0,0,0,0,  0,0,0,0,0 },
    {t_exp, 0,0,0,0,  0,0,0,0,0 },
    {t_stm, 0,0,0,0,  0,0,0,0,0 },
    {t_for, 0,0,0,0,  0,0,0,0,0 },
    {t_ifs, 0,0,0,0,  0,0,0,0,0 },
    {t_whi, 0,0,0,0,  0,0,0,0,0 },
    {t_exc, 0,0,0,0,  0,0,0,0,0 },
    {t_thw, 0,0,0,0,  0,0,0,0,0 },
    {t_cat, 0,0,0,0,  0,0,0,0,0 },
    {t_fin, 0,0,0,0,  0,0,0,0,0 },
    {t_iso, 0,0,0,0,  0,0,&new_t_iso, 0,0 },
    {t_ico, 0,0,0,0,  0,0,&new_t_ico, 0,0 },
    {t_oso, 0,0,0,0,  0,0,&new_t_oso, 0,0 },
    {t_oco, 0,0,0,0,  0,0,&new_t_oco, 0,0 },
    {t_reg, 0,0,0,0,  0,0,0,0,0 },
    {t_asi, 0,0,0,0,  0,0,0,0,0 },
    {t_boo, 0,0,0,0,  0,0,0,0,0 },
    {t_pbm, 0,0,0,0,  0,0,0,0,0 },
    {t_pbi, 0,0,0,0,  0,0,0,0,0 },
    {t_hdr, 0,0,0,0,  0,0,0,0,0 },
    {t_mdl, 0,0,0,0,  0,0,0,0,0 },
    {t_flh, 0,0,0,0,  0,0,0,0,0 },
    {t_srm, 0,0,0,0,  0,0,0,0,0 },
    {t_osm, 0,0,0,0,  0,0,0,0,0 },
    {t_ism, 0,0,0,0,  0,0,0,0,0 },
    {t_imp, 0,0,0,0,  0,0,0,0,0 },
    {t_inc, 0,0,0,0,  0,0,0,0,0 },
    {t_oss, 0,0,0,0,  0,0,0,0,0 },
    {t_iss, 0,0,0,0,  0,0,0,0,0 },
    {t_cpp, 0,0,0,0,  0,0,0,0,0 },

    {t_lsp, 0,0,0,0,  0,0,&new_t_lsp,0,0 }, // start lisp code in ($
    {t_lsc, 0,0,0,0,  0,0,&new_t_lsc,0,0 }, // start lisp code in (:

    {t_ftr, 0,0,0,0,  0,0,0,0,0 },
    {t_jav, 0,0,0,0,  0,0,0,0,0 },
    {t_ocm, 0,0,0,0,  0,0,0,0,0 },
    {t_prl, 0,0,0,0,  0,0,0,0,0 },
    {t_skt, 0,0,0,0,  0,0,0,0,0 },
    {t_pth, 0,0,0,0,  0,0,0,0,0 },
    {t_dir, 0,0,0,0,  0,0,0,0,0 },
    {t_pip, 0,0,0,0,  0,0,0,0,0 },
    {t_lck, 0,0,0,0,  0,0,0,0,0 },
    {t_cvr, 0,0,0,0,  0,0,0,0,0 },
    {t_htm, 0,0,0,0,  0,0,0,0,0 },
    {t_arf, 0,0,0,0,  0,0,0,0,0 },
    {t_url, 0,0,0,0,  0,0,0,0,0 },
    {t_git, 0,0,0,0,  0,0,0,0,0 },
    {t_scm, 0,0,0,0,  0,0,0,0,0 },
    {t_brk, 0,0,0,0,  0,0,0,0,0 },
    {t_lnn, 0,0,0,0,  0,0,0,0,0 },
    {t_srf, 0,0,0,0,  0,0,0,0,0 },
    {t_shm, 0,0,0,0,  0,0,0,0,0 },
    {t_gph, 0,0,0,0,  0,0,0,0,0 },
    {t_wnd, 0,0,0,0,  0,0,0,0,0 },
    {t_dia, 0,0,0,0,  0,0,0,0,0 },
    {t_png, 0,0,0,0,  0,0,0,0,0 },
    {t_jpg, 0,0,0,0,  0,0,0,0,0 },
    {t_arr, 0,0,0,0,  0,0,0,0,0 },
    {t_lib, 0,0,0,0,  0,0,0,0,0 },
    {t_dll, 0,0,0,0,  0,0,0,0,0 },
    {t_siz, 0,0,0,0,  0,0,0,0,0 },
    {t_lvm, 0,0,0,0,  0,0,0,0,0 },
    {t_lsh, 0,0,0,0,  0,0,0,0,0 },
    {t_new, 0,0,0,0,  0,0,0,0,0 },
    {t_del, 0,0,0,0,  0,0,0,0,0 },
    {t_mlc, 0,0,0,0,  0,0,0,0,0 },
    {t_fre, 0,0,0,0,  0,0,0,0,0 },
    {t_rea, 0,0,0,0,  0,0,0,0,0 },
    {t_bgi, 0,0,0,0,  0,0,0,0,0 },
    {t_mp3, 0,0,0,0,  0,0,0,0,0 },
    {t_mpg, 0,0,0,0,  0,0,0,0,0 },
    {t_dat, 0,0,0,0,  0,0,0,0,0 },
    {t_tim, 0,0,0,0,  0,0,0,0,0 },
    {t_trv, 0,0,0,0,  0,0,0,0,0 },
    {t_lbx, 0,0,0,0,  0,0,0,0,0 },
    {t_mus, 0,0,0,0,  0,0,0,0,0 },
    {t_quu, 0,0,0,0,  0,0,0,0,0 },
    {t_pop, 0,0,0,0,  0,0,0,0,0 },
    {t_psh, 0,0,0,0,  0,0,0,0,0 },
    {t_asm, 0,0,0,0,  0,0,0,0,0 },
    {t_clo, 0,0,0,0,  0,0,0,0,0 },
    {t_tls, 0,0,0,0,  0,0,0,0,0 },
    {t_tyl, 0,0,0,0,  0,0,0,0,0 },
    {t_wea, 0,0,0,0,  0,0,0,0,0 },
    {t_stg, 0,0,0,0,  0,0,0,0,0 },
    {t_dcl, 0,0,0,0,  0,0,&new_t_dcl,0,0 },
    {t_dfn, 0,0,0,0,  0,0,0,0,0 },
    {t_uns, 0,0,0,0,  0,0,0,0,0 },
    {t_jdy, 0,0,0,0,  0,0,0,0,0 },
    {t_jdl, 0,0,0,0,  0,0,0,0,0 },
    {t_jds, 0,0,0,0,  0,0,0,0,0 },
    {t_ast, 0,0,0,0,  0,0,0,0,0 },
    {t_sig, 0,0,0,0,  0,0,0,0,0 },
    {t_opn, 0,0,0,0,  0,0,&new_t_opn,0,0 },
    {t_cpn, 0,0,0,0,  0,0,0,0,0 },
    {t_obr, 0,0,0,0,  0,0,0,0,0 },
    {t_cbr, 0,0,0,0,  0,0,0,0,0 },
    {t_osq, 0,0,0,0,  0,0,0,0,0 },
    {t_csq, 0,0,0,0,  0,0,0,0,0 },
    {t_idx, 0,0,0,0,  0,0,0,0,0 },
    {t_ind, 0,0,0,0,  0,0,0,0,0 },
    {t_let, 0,0,0,0,  0,0,0,0,0 },
    {t_lda, 0,0,0,0,  0,0,0,0,0 },
    {t_fst, 0,0,0,0,  0,0,0,0,0 },
    {t_rst, 0,0,0,0,  0,0,0,0,0 },
    {t_ctr, 0,0,0,0,  0,0,0,0,0 },
    {t_dtr, 0,0,0,0,  0,0,0,0,0 },
    {t_sco, 0,0,0,0,  0,0,0,0,0 },
    {t_nms, 0,0,0,0,  0,0,0,0,0 },
    {t_fbn, 0,0,0,0,  0,0,0,0,0 },
    {t_dpc, 0,0,0,0,  0,0,0,0,0 },
    {t_dst, 0,0,0,0,  0,0,0,0,0 },
    {t_den, 0,0,0,0,  0,0,0,0,0 },
    {t_hst, 0,0,0,0,  0,0,0,0,0 },

    { t_dcr, 0,0,0,0,  0,0,0,0,0 },
    {t_evl, 0,0,0,0,  0,0,0,0,0 },
    {t_cts, 0,0,0,0,  0,0,0,0,0 },
    {t_hea, 0,0,0,0,  0,0,0,0,0 },
    {t_stk, 0,0,0,0,  0,0,0,0,0 },
    {t_bit, 0,0,0,0,  0,0,0,0,0 },
    //redundnat with m_bar:    {t_bor, 0,0,0,0,  0,0,0,0,0 },
    {t_xor, 0,0,0,0,  0,0,0,0,0 },
    {t_and, 0,0,0,0,  0,0,0,0,0 },
    {t_ndn, 0,0,0,0,  0,0,0,0,0 },
    {t_bno, 0,0,0,0,  0,0,0,0,0 },
    {t_lor, 0,0,0,0,  0,0,0,0,0 },
    {t_lan, 0,0,0,0,  0,0,0,0,0 },
    {t_not, 0,0,0,0,  0,0,0,0,0 },
    {t_tru, 0,0,0,0,  0,0,0,0,0 },
    {t_fal, 0,0,0,0,  0,0,0,0,0 },
    {t_nan, 0,0,0,0,  0,0,0,0,0 },
    {t_nav, 0,0,0,0,  0,0,0,0,0 },
    {t_inf, 0,0,0,0,  0,0,0,0,0 },
    {t_nnf, 0,0,0,0,  0,0,0,0,0 },
    {t_zer, 0,0,0,0,  0,0,0,0,0 },
    {t_ini, 0,0,0,0,  0,0,0,0,0 },
    {t_fix, 0,0,0,0,  0,0,0,0,0 },
    {t_vrb, 0,0,0,0,  0,0,0,0,0 },
    {t_mix, 0,0,0,0,  0,0,0,0,0 },
    {t_dmd, 0,0,0,0,  0,0,0,0,0 },
    {t_dim, 0,0,0,0,  0,0,0,0,0 },
    {t_jmp, 0,0,0,0,  0,0,0,0,0 },
    {t_alg, 0,0,0,0,  0,0,0,0,0 },
    {t_add, 0,0,0,0,  0,0,0,0,0 },
    {t_sub, 0,0,0,0,  0,0,0,0,0 },
    {t_mul, 0,0,0,0,  0,0,0,0,0 },
    {t_div, 0,0,0,0,  0,0,0,0,0 },
    {t_mod, 0,0,0,0,  0,0,0,0,0 },
    {t_aka, 0,0,0,0,  0,0,0,0,0 },
    {t_tyd, 0,0,0,0,  0,0,0,0,0 },
    {t_rmj, 0,0,0,0,  0,0,0,0,0 },
    {t_cmj, 0,0,0,0,  0,0,0,0,0 },
    {t_cro, 0,0,0,0,  0,0,0,0,0 },
    {t_ifd, 0,0,0,0,  0,0,0,0,0 },
    {t_end, 0,0,0,0,  0,0,0,0,0 },
    {t_dfp, 0,0,0,0,  0,0,0,0,0 },
    {t_pdf, 0,0,0,0,  0,0,0,0,0 },
    {t_cdf, 0,0,0,0,  0,0,0,0,0 },
    {t_swi, 0,0,0,0,  0,0,0,0,0 },
    {t_cas, 0,0,0,0,  0,0,0,0,0 },
    {t_dft, 0,0,0,0,  0,0,0,0,0 },
    {t_cls, 0,0,0,0,  0,0,0,0,0 },
    {t_vrt, 0,0,0,0,  0,0,0,0,0 },
    {t_pur, 0,0,0,0,  0,0,0,0,0 },
    {t_pub, 0,0,0,0,  0,0,0,0,0 },
    {t_prv, 0,0,0,0,  0,0,0,0,0 },
    {t_pro, 0,0,0,0,  0,0,0,0,0 },
    {t_frd, 0,0,0,0,  0,0,0,0,0 },
    {t_lhs, 0,0,0,0,  0,0,0,0,0 },
    {t_rhs, 0,0,0,0,  0,0,0,0,0 },
    {t_jid, 0,0,0,0,  0,0,0,0,0 },
    {t_usd, 0,0,0,0,  0,0,0,0,0 },
    {t_eof, 0,0,0,0,  0,0,&new_t_eof,0,0 }, // end of file: used as the last token in a token stream.


// math types

    {m_eql, 0,0,0,0,  0,0,&new_m_eql,0,0 }, // ="eq"; // eq equals, preferred instead of ==
    {m_eqe, 0,0,0,0,  0,0,&new_m_eqe,0,0 }, // ="m_eqe"; // ==
    {m_neq, 0,0,0,0,  0,0,&new_m_neq,0,0 }, // ="m_neq"; // != not equal
    {m_bng, 0,0,0,0,  0,0,&new_m_bng,0,0 }, // ="m_bng"; // !
    {m_mlt, 0,0,0,0,  0,0,&new_m_mlt,0,0 }, // ="m_mlt"; // *
    {m_crt, 0,0,0,0,  0,0,&new_m_crt,0,0 }, // ="m_crt"; // ^
    {m_dvn, 0,0,0,0,  0,0,&new_m_dvn,0,0 }, // ="m_dvn"; //  / division
    {m_mod, 0,0,0,0,  0,0,&new_m_mod,0,0 }, // ="m_mod"; //  % modulo
    {m_pls, 0,0,0,0,  0,0,&new_m_pls,0,0 }, // ="m_pls"; // +
    {m_mns, 0,0,0,0,  0,0,&new_m_mns,0,0 }, // ="m_mns"; // -

    {m_tld, 0,0,0,0,  0,0,&new_m_tld,0,0 }, // ="m_tld"; // ~ tilde

    {m_gth, 0,0,0,0,  0,0,&new_m_gth,0,0 }, // ="m_gth"; // >
    {m_lth, 0,0,0,0,  0,0,&new_m_lth,0,0 }, // ="m_lth"; // <
    {m_gte, 0,0,0,0,  0,0,&new_m_gte,0,0 }, // ="m_gte"; // >=
    {m_lte, 0,0,0,0,  0,0,&new_m_lte,0,0 }, // ="m_lte"; // <=
    {m_peq, 0,0,0,0,  0,0,&new_m_peq,0,0 }, // ="m_peq"; // +=
    {m_meq, 0,0,0,0,  0,0,&new_m_meq,0,0 }, // ="m_meq"; // -=
    {m_dve, 0,0,0,0,  0,0,&new_m_dve,0,0 }, // ="m_dve"; // /=
    {m_mte, 0,0,0,0,  0,0,&new_m_mte,0,0 }, // ="m_mte"; // *=
    {m_cre, 0,0,0,0,  0,0,&new_m_cre,0,0 }, // ="m_cre"; // ^=

    {m_ppl, 0,0,0,0,  0,0,&new_m_ppl,0,0 }, // ="m_cre"; // ++
    {m_mmn, 0,0,0,0,  0,0,&new_m_mmn,0,0 }, // ="m_cre"; // --

    {m_umi, 0,0,0,0,  0,0,&new_m_umi,0,0 }, // ="m_umi"; // - unary minus
    {m_bar, 0,0,0,0,  0,0,&new_m_bar,0,0 }, // ="m_bar"; // |
    {m_ats, 0,0,0,0,  0,0,&new_m_ats,0,0 }, // ="m_ats"; // @
    {m_hsh, 0,0,0,0,  0,0,&new_m_hsh,0,0 }, // ="m_hsh"; // #
    {m_dlr, 0,0,0,0,  0,0,&new_m_dlr,0,0 }, // ="m_dlr"; // $
    {m_pct, 0,0,0,0,  0,0,&new_m_pct,0,0 }, // ="m_pct"; // %
    {m_cln, 0,0,0,0,  0,0,&new_m_cln,0,0 }, // ="m_cln"; // :
    {m_que, 0,0,0,0,  0,0,&new_m_que,0,0 }, // ="m_que"; // ?
    {m_lan, 0,0,0,0,  0,0,&new_m_lan,0,0 }, // ="m_lan"; // &&
    {m_lor, 0,0,0,0,  0,0,&new_m_lor,0,0 }, // ="m_lor"; // ||

    // protobuf types/tokens/operators

    {t_pb_message,    0,0,0,0,  0,0,&new_t_pb_message,0,0 }, // = "message";
    {t_pb_import,     0,0,0,0,  0,0,&new_t_pb_import,0,0 }, // = "import";
    {t_pb_package,    0,0,0,0,  0,0,&new_t_pb_package,0,0 }, // = "package";
    {t_pb_service,    0,0,0,0,  0,0,&new_t_pb_service,0,0 }, // = "service";
    {t_pb_rpc,        0,0,0,0,  0,0,&new_t_pb_rpc,0,0 }, // = "rpc";
    {t_pb_extensions, 0,0,0,0,  0,0,&new_t_pb_extensions,0,0 }, // = "extensions";
    {t_pb_extend,     0,0,0,0,  0,0,&new_t_pb_extend,0,0 }, // = "extend";
    {t_pb_option,     0,0,0,0,  0,0,&new_t_pb_option,0,0 }, // = "option";
    {t_pb_required,   0,0,0,0,  0,0,&new_t_pb_required,0,0 }, // = "required";
    {t_pb_repeated,   0,0,0,0,  0,0,&new_t_pb_repeated,0,0 }, // = "repeated";
    {t_pb_optional,   0,0,0,0,  0,0,&new_t_pb_optional,0,0 }, // = "optional";
    {t_pb_enum,       0,0,0,0,  0,0,&new_t_pb_enum,0,0 }, // = "enum";

    // pb types

    {t_pby_int32,     0,0,0,0,   0,0,&new_t_pby_int32 }, //    = "int32";
    {t_pby_int64,     0,0,0,0,   0,0,&new_t_pby_int64 }, //    = "int64";
    {t_pby_uint32,    0,0,0,0,   0,0,&new_t_pby_uint32 }, //   = "uint32";
    {t_pby_uint64,    0,0,0,0,   0,0,&new_t_pby_uint64 }, //   = "uint64";
    {t_pby_sint32,    0,0,0,0,   0,0,&new_t_pby_sint32 }, //   = "sint32";
    {t_pby_sint64,    0,0,0,0,   0,0,&new_t_pby_sint64 }, //   = "sint64";
    {t_pby_fixed32,   0,0,0,0,   0,0,&new_t_pby_fixed32 }, //  = "fixed32";
    {t_pby_fixed64,   0,0,0,0,   0,0,&new_t_pby_fixed64 }, //  = "fixed64";
    {t_pby_sfixed32,  0,0,0,0,   0,0,&new_t_pby_sfixed32 }, // = "sfixed32";
    {t_pby_sfixed64,  0,0,0,0,   0,0,&new_t_pby_sfixed64 }, // = "sfixed64";
    {t_pby_string,    0,0,0,0,   0,0,&new_t_pby_string }, //   = "string";
    {t_pby_bytes,     0,0,0,0,   0,0,&new_t_pby_bytes }, //    = "bytes";


    {t_semicolon,     0,0,0,0,  0,0,&new_t_semicolon,0,0 }, // = "t_semicolon";
    {t_newline,       0,0,0,0,  0,0,&new_t_newline,0,0 }, // = "t_newline";
    {t_cppcomment,    0,0,0,0,  0,0,&new_t_cppcomment,0,0 }, // = "t_cppcomment";
    {t_comma,         0,0,0,0,  0,0,&new_t_comma,0,0 }, // = "t_comma";

    // C types
    {t_c_auto,        0,0,0,0,   0,0,&new_t_c_auto,0,0 },  // = "auto";
    {t_c_double,      0,0,0,0,   0,0,&new_t_c_double,0,0 },  // = "double";
    {t_c_int,         0,0,0,0,   0,0,&new_t_c_int,0,0 },  // = "int";
    {t_c_struct,      0,0,0,0,   0,0,&new_t_c_struct,0,0 },  // = "struct";
    {t_c_break,       0,0,0,0,   0,0,&new_t_c_break,0,0 },  // = "break";
    {t_c_else,        0,0,0,0,   0,0,&new_t_c_else,0,0 },  // = "else";
    {t_c_long,        0,0,0,0,   0,0,&new_t_c_long,0,0 },  // = "long";
    {t_c_switch,      0,0,0,0,   0,0,&new_t_c_switch,0,0 },  // = "switch";
    {t_c_case,        0,0,0,0,   0,0,&new_t_c_case,0,0 },  // = "case";
    //    {t_c_enum,        0,0,0,0,   0,0,&new_t_c_enum,0,0 },  // = "enum"; // enum already in protobuf words
    {t_c_register,    0,0,0,0,   0,0,&new_t_c_register,0,0 },  // = "register";
    {t_c_typedef,     0,0,0,0,   0,0,&new_t_c_typedef,0,0 },  // = "typedef";
    {t_c_char,        0,0,0,0,   0,0,&new_t_c_char,0,0 },  // = "char";
    {t_c_extern,      0,0,0,0,   0,0,&new_t_c_extern,0,0 },  // = "extern";
    {t_c_return,      0,0,0,0,   0,0,&new_t_c_return,0,0 },  // = "return";
    {t_c_union,       0,0,0,0,   0,0,&new_t_c_union,0,0 },  // = "union";
    {t_c_const,       0,0,0,0,   0,0,&new_t_c_const,0,0 },  // = "const";
    {t_c_float,       0,0,0,0,   0,0,&new_t_c_float,0,0 },  // = "float";
    {t_c_short,       0,0,0,0,   0,0,&new_t_c_short,0,0 },  // = "short";
    {t_c_unsigned,    0,0,0,0,   0,0,&new_t_c_unsigned,0,0 },  // = "unsigned";
    {t_c_continue,    0,0,0,0,   0,0,&new_t_c_continue,0,0 },  // = "continue";
    {t_c_for,         0,0,0,0,   0,0,&new_t_c_for,0,0 },  // = "for";
    {t_c_signed,      0,0,0,0,   0,0,&new_t_c_signed,0,0 },  // = "signed";
    {t_c_void,        0,0,0,0,   0,0,&new_t_c_void,0,0 },  // = "void";
    {t_c_default,     0,0,0,0,   0,0,&new_t_c_default,0,0 },  // = "default";
    {t_c_goto,        0,0,0,0,   0,0,&new_t_c_goto,0,0 },  // = "goto";
    {t_c_sizeof,      0,0,0,0,   0,0,&new_t_c_sizeof,0,0 },  // = "sizeof";
    {t_c_volatile,    0,0,0,0,   0,0,&new_t_c_volatile,0,0 },  // = "volatile";
    {t_c_do,          0,0,0,0,   0,0,&new_t_c_do,0,0 },  // = "do";
    {t_c_if,          0,0,0,0,   0,0,&new_t_c_if,0,0 },  // = "if";
    {t_c_static,      0,0,0,0,   0,0,&new_t_c_static,0,0 },  // = "static";
    {t_c_while,       0,0,0,0,   0,0,&new_t_c_while,0,0 },  // = "while";
    {t_c__Bool,       0,0,0,0,   0,0,&new_t_c__Bool,0,0 },  // = "_Bool";
    {t_c_inline,      0,0,0,0,   0,0,&new_t_c_inline,0,0 },  // = "inline";
    {t_c__Complex,    0,0,0,0,   0,0,&new_t_c__Complex,0,0 },  // = "_Complex";
    {t_c_restrict,    0,0,0,0,   0,0,&new_t_c_restrict,0,0 },  // = "restrict";
    {t_c__Imaginary,  0,0,0,0,   0,0,&new_t_c__Imaginary,0,0 },  // = "_Imaginary";

    // C++ types
    {t_cc_asm,        0,0,0,0,   0,0,&new_t_cc_asm,0,0 },  // = "asm";
    {t_cc_dynamic_cast,
                      0,0,0,0,   0,0,&new_t_cc_dynamic_cast,0,0 },  // = "dynamic_cast";
    {t_cc_namespace,  0,0,0,0,   0,0,&new_t_cc_namespace,0,0 },  // = "namespace";
    {t_cc_reinterpret_cast,    
                      0,0,0,0,   0,0,&new_t_cc_reinterpret_cast,0,0 },  // = "reinterpret_cast";

#if 0  // let these be atoms, for now.
    {t_cc_try,        0,0,0,0,   0,0,&new_t_cc_try,0,0 },  // = "try";
    {t_cc_catch,      0,0,0,0,   0,0,&new_t_cc_catch,0,0 },  // = "catch";
    {t_cc_throw,      0,0,0,0,   0,0,&new_t_cc_throw,0,0 },  // = "throw";
#else
    {t_cc_try,        0,0,0,0,   0,0,&new_t_ato,0,0 },  // = "try";
    {t_cc_catch,      0,0,0,0,   0,0,&new_t_ato,0,0 },  // = "catch";
    {t_cc_throw,      0,0,0,0,   0,0,&new_t_ato,0,0 },  // = "throw";
#endif

    {t_cc_bool,       0,0,0,0,   0,0,&new_t_cc_bool,0,0 },  // = "bool";
    {t_cc_explicit,   0,0,0,0,   0,0,&new_t_cc_explicit,0,0 },  // = "explicit";
    {t_cc_new,        0,0,0,0,   0,0,&new_t_cc_new,0,0 },  // = "new";
    {t_cc_static_cast,0,0,0,0,   0,0,&new_t_cc_static_cast,0,0 },  // = "static_cast";
    {t_cc_typeid,     0,0,0,0,   0,0,&new_t_cc_typeid,0,0 },  // = "typeid";
    {t_cc_false,      0,0,0,0,   0,0,&new_t_cc_false,0,0 },  // = "false";
    {t_cc_operator,   0,0,0,0,   0,0,&new_t_cc_operator,0,0 },  // = "operator";
    {t_cc_template,   0,0,0,0,   0,0,&new_t_cc_template,0,0 },  // = "template";
    {t_cc_typename,   0,0,0,0,   0,0,&new_t_cc_typename,0,0 },  // = "typename";
    {t_cc_class,      0,0,0,0,   0,0,&new_t_cc_class,0,0 },  // = "class";
    {t_cc_friend,     0,0,0,0,   0,0,&new_t_cc_friend,0,0 },  // = "friend";
    {t_cc_private,    0,0,0,0,   0,0,&new_t_cc_private,0,0 },  // = "private";
    {t_cc_this,       0,0,0,0,   0,0,&new_t_cc_this,0,0 },  // = "this";
    {t_cc_using,      0,0,0,0,   0,0,&new_t_cc_using,0,0 },  // = "using";
    {t_cc_const_cast, 0,0,0,0,   0,0,&new_t_cc_const_cast,0,0 },  // = "const_cast";

    {t_cc_public,     0,0,0,0,   0,0,&new_t_cc_public,0,0 },  // = "public";
    {t_cc_virtual,    0,0,0,0,   0,0,&new_t_cc_virtual,0,0 },  // = "virtual";
    {t_cc_delete,     0,0,0,0,   0,0,&new_t_cc_delete,0,0 },  // = "delete";
    {t_cc_mutable,    0,0,0,0,   0,0,&new_t_cc_mutable,0,0 },  // = "mutable";
    {t_cc_protected,  0,0,0,0,   0,0,&new_t_cc_protected,0,0 },  // = "protected";
    {t_cc_true,       0,0,0,0,   0,0,&new_t_cc_true,0,0 },  // = "true";
    {t_cc_wchar_t,    0,0,0,0,   0,0,&new_t_cc_wchar_t,0,0 },  // = "wchar_t";
    {t_cc_and,        0,0,0,0,   0,0,&new_t_cc_and,0,0 },  // = "and";
    {t_cc_bitand,     0,0,0,0,   0,0,&new_t_cc_bitand,0,0 },  // = "bitand";
    {t_cc_compl,      0,0,0,0,   0,0,&new_t_cc_compl,0,0 },  // = "compl";
    {t_cc_not_eq,     0,0,0,0,   0,0,&new_t_cc_not_eq,0,0 },  // = "not_eq";
    {t_cc_or_eq,      0,0,0,0,   0,0,&new_t_cc_or_eq,0,0 },  // = "or_eq";
    {t_cc_xor_eq,     0,0,0,0,   0,0,&new_t_cc_xor_eq,0,0 },  // = "xor_eq";
    {t_cc_and_eq,     0,0,0,0,   0,0,&new_t_cc_and_eq,0,0 },  // = "and_eq";
    {t_cc_bitor,      0,0,0,0,   0,0,&new_t_cc_bitor,0,0 },  // = "bitor";
    {t_cc_not,        0,0,0,0,   0,0,&new_t_cc_not,0,0 },  // = "not";
    {t_cc_or,         0,0,0,0,   0,0,&new_t_cc_or,0,0 },  // = "or";
    {t_cc_xor,        0,0,0,0,   0,0,&new_t_cc_xor,0,0 },  // = "xor";


    {0, 0,0,0,0, 0,0,0 } // use 0 _ty entry to signal the last entry in the type table.
};

//typedef char* t_typ;

// client should initialize/delete when done:


quicktype_sys::quicktype_sys() {

    _typemap = new judySmap<quicktype* >;
    load_builtin_types();
}

typedef jlmap_it<t_typ, quicktype* >  qt_it;

quicktype_sys::~quicktype_sys() {

    quicktype* qt = 0;
    
    l3path key;
    BOOL gotentry = _typemap->first(&key, &qt);

    while (gotentry) {
        if (qt) {
            tyse_free(qt);
        }
        key.clear();
        gotentry = _typemap->next(&key, &qt);
    }
    delete _typemap;
}


t_typ quicktype_sys::which_type(const char* t, quicktype** pqt) {

    quicktype* qt = _typemap->lookupkey(t);
    if (pqt) {
        *pqt = qt;
    }
    if (0==qt) return 0;
    return qt->_ty;
}


t_typ quicktype_sys::which_type(const qqchar& t, quicktype** pqt) {

    l3path r(t);

    quicktype* qt = _typemap->lookupkey(r());
    if (pqt) {
        *pqt = qt;
    }
    if (0==qt) return 0;
    return qt->_ty;
}


/*
struct qtype_table {
    t_typ       _ty;
    ptr2method  _print;
    ptr2method  _ctor;
    ptr2method  _cpctor;
    ptr2method  _dtor;
};
*/

void quicktype_sys::load_builtin_types() {

    int numtyp = sizeof(valid_t_typ)/sizeof(qtype_table);
    
    quicktype*  qt = 0;
    for (int i = 0; i < numtyp; ++i) {
        
        if (valid_t_typ[i]._ty == 0) {
            if(i != numtyp-1) {
                // we hit the end of the table early!
                printf("internal error: end of type table before expected: is it no longer of type qtype_table/is the sizeof(qtype_table) off above?\n");
                assert(0);
                exit(1);
            } else {
                // normal end of table.
                return;
            }
        }

        qt = make_new_type(valid_t_typ[i]._ty);

        set_sysbuiltin(qt);
        qt->_print  = valid_t_typ[i]._print;
        qt->_ctor   = valid_t_typ[i]._ctor;
        qt->_cpctor = valid_t_typ[i]._cpctor;
        qt->_dtor   = valid_t_typ[i]._dtor;

        qt->_load   = valid_t_typ[i]._load;
        qt->_save   = valid_t_typ[i]._save;
        qt->_qtreenew = valid_t_typ[i]._qtreenew;

        _typemap->insertkey(valid_t_typ[i]._ty,qt);
        
        DV(printf("_typemap  added: %s  -->  %ld\n",
                  valid_t_typ[i]._ty,qt->_ser));
    }

}


quicktype*  quicktype_sys::make_new_type(t_typ  addme) {

    // check for conflicts with existing types
    t_typ  already_exists = which_type(addme,0);

    if (already_exists) {
        printf("error in quicktype_sys::make_new_type: type '%s' already exists.\n", already_exists);
        assert(0);
        return 0;
    }

    // malloc and assign type and tyse tracker serial number.
    quicktype* qt = (quicktype*)::tyse_malloc(t_qty,sizeof(quicktype),1);
    assert(qt->_type == t_qty);
    assert(qt->_ser);

    qt->_ty = addme;

    return qt;
}


void quicktype_sys::register_type(t_typ  addme, quicktype* qt) {

    _typemap->insertkey(addme, qt);
}

void quicktype_sys::unregister_type(t_typ delme) {
    
    _typemap->deletekey(delme);
}



std::ostream& operator<<(std::ostream& os, const tyse& printme) {

    os << " _type: '" << printme._type  << "'    _ser: " << printme._ser;
#if 0
    if (0==strcmp(printme._type, "sexp_t") && ((sexp_t*)&printme)->val ) {
        os << "     sexp_t->val = '" << ((sexp_t*) &printme)->val << "'";
    }
#endif
    os << std::endl;
    return os;

}

std::ostream& operator<<(std::ostream& os, const tyse*& printme) {

    if (printme) {
        os << " _type: '" << printme->_type  << "'    _ser: " << printme->_ser << std::endl;
    } else {
        os << "(null tyse*)" << std::endl;
    }
    return os;

}
