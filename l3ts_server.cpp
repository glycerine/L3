//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//


// old, from sdk/demo:
//
//  Hello Protocol Buffers server
//  Binds REP socket to tcp://*:4444
//  Expects "Hello" from client, replies with "World"
//
//  Changes for 2.1:
//  - added socket close before terminating
//
#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>		// for varargs.
#include <string.h>		// for str*().
#include <errno.h>
#include <ctype.h>		// for isspace(), etc.
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <uuid/uuid.h>

#include <zmq.h>
#include "l3ts.pb.h"
#include "l3path.h"
#include "l3ts_common.h"
#include "rmkdir.h"
#include "l3obj.h"
#include "autotag.h"
#include "terp.h"
#include "objects.h"
#include "l3string.h"
#include "l3pratt.h"

using std::string;
using std::cout;
using std::endl;
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/coded_stream.h>

using namespace google::protobuf::io;

using namespace google::protobuf;

using namespace l3ts;

t_typ nameval_to_qtype(l3ts::nameval*  nv);

L3FORWDECL(nv3)
L3FORWDECL(nv3_dtor)
L3FORWDECL(nv3_cpctor)
L3FORWDECL(open_nv3)


#if 0   // moved to l3ts_common.cpp
void my_free (void *data, void *hint)
{
  free (data);
}

l3ts::nameval*  decode_obj_env_for_nameval(l3obj* obj, l3obj* env) {

    l3ts::nameval* nv = 0;
    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__nameval());
        
        nv = any->mutable__nameval();
    }

    return nv;
}


// determine the size of the header by creating a header and serializing it
//
size_t get_header_len() {
  string sizing_header_string;
  l3ts::pb_msg_header sizing_header;
  sizing_header.set_npayload_bytes(99);
  assert(sizing_header.SerializeToString(&sizing_header_string));
  size_t header_len = sizing_header_string.size();
  return header_len;
}
#endif


static size_t hlen = get_header_len();

void load_test_nameval(l3ts::nameval& nv1) {  

//, l3ts::object  objABC

    l3path fname(0,"test_data.3ts");

    nv1.set_nvtyp(l3ts::nameval::STR);
    //    std::cout << "nv1: " << nv1.DebugString() << std::endl;

    //    nv1.set_strkey("heya strkey in protobuf!");
    //    std::cout << "nv1: " << nv1.DebugString() << std::endl;

    //    nv1.add_idbyte("heta I'm an idbyte!");
    //    nv1.add_idbyte("and another add_idbyte called a second time");
    //    std::cout << "nv1: " << nv1.DebugString() << std::endl;
   

    for (double d = 0; d < 6; ++d) {
        nv1.add_dou(d);
    }

    nv1.add_inames("row0");
    nv1.add_inames("row1");
    nv1.add_inames("row2");

    nv1.add_jnames("col0");
    nv1.add_jnames("col1");

    //    std::cout << "nv1: " << nv1.DebugString() << std::endl;

    //    std::cout << "nv1 jnames has size: " << nv1.jnames_size() << std::endl;

}


class InhFromTimept : public l3ts::timept {

    void defn() { printf("Hi I'm defn in InhFromTimept\n");}

};


void load_test_timeseries_data() {

    l3path fname(0,"test_data.3ts");

    l3ts::nameval nv1;

    nv1.set_nvtyp(l3ts::nameval::STR);
    std::cout << "nv1: " << nv1.DebugString() << std::endl;

    //    nv1.set_strkey("heya strkey in protobuf!");
    std::cout << "nv1: " << nv1.DebugString() << std::endl;

    //    nv1.add_idbyte("heta I'm an idbyte!");
    //    nv1.add_idbyte("and another add_idbyte called a second time");
    //    std::cout << "nv1: " << nv1.DebugString() << std::endl;
   

    l3ts::nameval nv2;
    nv2.set_nvtyp(l3ts::nameval::DOU);

    nv2.add_dim(3);
    nv2.add_dim(2);
    
    for (double d = 0; d < 6; ++d) {
        nv2.add_dou(d);
    }

    nv2.add_inames("row0");
    nv2.add_inames("row1");
    nv2.add_inames("row2");

    nv2.add_jnames("col0");
    nv2.add_jnames("col1");

    std::cout << "nv2: " << nv2.DebugString() << std::endl;

    std::cout << "nv2 jnames has size: " << nv2.jnames_size() << std::endl;

    l3ts::nameval nv3;
    nv3.set_nvtyp(l3ts::nameval::DOU);





    l3ts::object  objABC;

    l3ts::nameval* nv4 = objABC.add_nv();
    *nv4 = nv1;

    l3ts::nameval* nv5 = objABC.add_nv();
    l3ts::nameval* nv6 = objABC.add_nv();

    *nv5 = nv2;
    *nv6 = nv3;


    std::cout << "objABC: " << objABC.DebugString() << std::endl;


    //l3ts::timept tp1;
    InhFromTimept tp1;

    tp1.set_sec(123);
    tp1.set_nano(9);
    // now we have encloseing serial_baseday
    //tp1.set_serialday(1969*365);

    std::cout << "tp1: " << tp1.DebugString() << std::endl;


    l3ts::timept tp2;
    
    l3ts::geoloc gl;

    gl.set_geoname("Hi I'm a geoname.");
    gl.add_geoid("54321");
    std::cout << "gl: " << gl.DebugString() << std::endl;


    ::l3ts::tmplace my_time_place;
    ::l3ts::timept* ptp8 = my_time_place.mutable_tm();

    ::l3ts::geoloc* pgeoloc8 = my_time_place.mutable_place();

    *ptp8 = tp1;
    *pgeoloc8 = gl;

    std::cout << "my_time_place: " << my_time_place.DebugString() << std::endl;


    l3ts::datapt  pt;

    l3ts::tmplace*  ptmpl0   = pt.add_tmpl();
    l3ts::object*   pobj0    = pt.add_objs();

    *ptmpl0 = my_time_place;
    *pobj0  = objABC;

    std::cout << "pt: " << pt.DebugString() << std::endl;


    l3ts::timeseries ts;
    ts.set_serial_baseday(100);
    l3ts::datapt*  pdp = ts.add_tspoints();

    *pdp = pt;

    std::cout << "ts: " << ts.DebugString() << std::endl;

    bool cpp = false;

    // write to disk:

    if (cpp) {
        std::fstream output(fname(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!ts.SerializeToOstream(&output)) {
            std::cerr << "Failed to write address book." << std::endl;
            assert(0);
        }
        // important to do this so we can read it back in:
        output.close();
    }
    else {

        int fd = open(fname(), O_WRONLY);

        if (!ts.SerializeToFileDescriptor(fd)) {
            fprintf(stderr, "Failed to serialize protocol buffer message to file '%s'.\n",fname());
            assert(0);
        }
        close(fd);

    }


        // read from disk:

    // confirm we got a non-zero file

    file_ty filety = FILE_DOES_NOT_EXIST;
    long    filesz = 0;
    bool    fexists = file_exists(fname(), &filety, &filesz);

    if (!fexists || filety != REGFILE || filesz <= 0) {
        printf("error: attempt to read bad file path '%s'.\n", fname());
        l3throw(XABORT_TO_TOPLEVEL);
    }

    // reinflate the data
    l3ts::timeseries reinflated;

    if (cpp) {
        
        std::ifstream rein(fname(), std::ios::in | std::ios::binary);
        if (!rein) {
            std::cerr << "file not found: " << fname()  << std::endl;
            assert(0);
        } else
            if (!reinflated.ParseFromIstream(&rein)) {
                std::cerr << "Failed reinflate test data out.data.3ts." << std::endl;
                assert(0);
            }
        rein.close();
        
        std::cout << "reinflated: " << reinflated.DebugString() << std::endl;
        

    } else {

        // non-c++ / for sockets do:        

        int fd = open(fname(), O_RDONLY);         

        if (!reinflated.ParseFromFileDescriptor(fd)) {
            fprintf(stderr, "Failed reinflate test data '%s'.\n",fname());
            assert(0);
        }

        close(fd);

        std::cout << "reinflated: " << reinflated.DebugString() << std::endl;
    }

    using google::protobuf::Reflection;
    using google::protobuf::FieldDescriptor;
    using google::protobuf::Descriptor;

    // now reflect on this message
    const Reflection* refl = reinflated.GetReflection();

    const Descriptor* desc = reinflated.GetDescriptor();
    printf("description of message from .proto file '%s'  and message type name is: '%s'\n",
           desc->file()->name().c_str(),  desc->full_name().c_str());

    std::vector< const FieldDescriptor* > vfd;
    refl->ListFields(reinflated,   &vfd);

    /*
enum FieldDescriptor::Type {
  TYPE_DOUBLE = 1,
  TYPE_FLOAT = 2,
  TYPE_INT64 = 3,
  TYPE_UINT64 = 4,
  TYPE_INT32 = 5,
  TYPE_FIXED64 = 6,
  TYPE_FIXED32 = 7,
  TYPE_BOOL = 8,
  TYPE_STRING = 9,
  TYPE_GROUP = 10,
  TYPE_MESSAGE = 11,
  TYPE_BYTES = 12,
  TYPE_UINT32 = 13,
  TYPE_ENUM = 14,
  TYPE_SFIXED32 = 15,
  TYPE_SFIXED64 = 16,
  TYPE_SINT32 = 17,
  TYPE_SINT64 = 18,
  MAX_TYPE = 18
}

enum FieldDescriptor::CppType {
  CPPTYPE_INT32 = 1,
  CPPTYPE_INT64 = 2,
  CPPTYPE_UINT32 = 3,
  CPPTYPE_UINT64 = 4,
  CPPTYPE_DOUBLE = 5,
  CPPTYPE_FLOAT = 6,
  CPPTYPE_BOOL = 7,
  CPPTYPE_ENUM = 8,
  CPPTYPE_STRING = 9,
  CPPTYPE_MESSAGE = 10,
  MAX_CPPTYPE = 10
}

     */

    //    FieldDescriptor::Type ty = 

    for (::uint i = 0; i < vfd.size(); ++i ) {
        printf("filed %02d  is : '%s'  with type %d\n", i ,  vfd[i]->name().c_str(),  vfd[i]->cpp_type());

    }

}

#if 0   // ndef _NOMAIN_

int main(int argc, char** argv) {
  return l3ts_server_main (argc, argv);
}
#endif


int l3ts_server_main (int argc, char** argv)
{
  // allocate received request classes
  l3ts::pb_msg_header  header_recvd;
  l3ts::timeseries timeseries_recvd;

  // the repliy to send back
  l3ts::pb_msg_header  header_tosend;
  l3ts::timeseries timeseries_tosend;

  load_test_timeseries_data();

  void *context = zmq_init (1);

  //  Socket to talk to clients
  void *responder = zmq_socket (context, ZMQ_REP);
  zmq_bind (responder, "tcp://*:4444");

  while (1) {
      //  Wait for next request from client
      zmq_msg_t request;
      zmq_msg_init (&request);
      if (0 != zmq_recv (responder, &request, 0)) {
          throw "zmq_recv failed";
      }
      printf ("Received Hello\n");
      sleep(1);
      
    // begin server code.

    // then the strings which the classes have to deserialize from
    //
    // use this ctor  -- std::string ( const char * s, size_t n );
    // Content is initialized to a copy of the string formed by the first n characters in the array of characters pointed by s.
    void* pdata = zmq_msg_data(&request);
    string header_recvd_string((char*)pdata,  hlen);

    assert(header_recvd.ParseFromString(header_recvd_string));
    size_t payload_recvd_len = header_recvd.npayload_bytes();

    std::cout << "pbserver: Received request header:\n" << header_recvd.DebugString() << std::endl;

    printf("payload_recvd_len is %lu\n", payload_recvd_len);

    string payload_recvd_string(((char*)pdata) + hlen, payload_recvd_len);

    // parses a message from the given string.
    assert(timeseries_recvd.ParseFromString(payload_recvd_string));

    std::cout << "pbserver: Received request, here is the pb dump of timeseries_recvd:\n" << timeseries_recvd.DebugString() << std::endl;
    
    zmq_msg_close (&request);


    //  Do some 'work'
    sleep (4);



    //  Send reply back to client
    zmq_msg_t msg_reply_tosend;


    // replace this:    zmq_msg_init_data (&reply, (void*)"World", 5, NULL, NULL);

    // begin constructing server "reply" code, which is just like the client "request" message construction.

    // for now we just send  back what we got.
    
    timeseries_tosend = timeseries_recvd;

    // strings to hold serialized versions of header and variable length payload
    string header_tosend_string;
    string payload_tosend_string;

    // serializes the message and stores the bytes in the given string. Note that the bytes are binary, not text; we only use the string class as a convenient container.
    assert(timeseries_tosend.SerializeToString(&payload_tosend_string));
    size_t payload_len = payload_tosend_string.size();

    // header uses a fixed size message as header.
    header_tosend.set_npayload_bytes(payload_len);
    assert(header_tosend.SerializeToString(&header_tosend_string));
    size_t header_tosend_len = header_tosend_string.size();
    
    std::cout << "\npbserver, sever reply is prepared, here is the pb dump of header (type l3ts::pb_msg_header):\n" << header_tosend.DebugString() << std::endl;
    std::cout << "header_tosend_len on sender side is: " << header_tosend_len << endl;

    std::cout << "\npbserver, server reply is prepared, here is the pb dump of timeseries   (type l3ts::timeseries):\n"          << timeseries_tosend.DebugString() << std::endl;
    std::cout << "payload_len on sender side is: " << payload_len << endl;


    // terminating '\0' is never counted, in either header or payload.
    size_t msg_size = header_tosend_len + payload_len;
    
    char* data = (char*)malloc (msg_size);
    assert (data);
    bzero(data,msg_size);
    memcpy (data, header_tosend_string.c_str() ,header_tosend_len);

    string recovery_header_string((char*)data, header_tosend_len);

    void* payload_start_addr = ((char*)data + header_tosend_len);
    memcpy (payload_start_addr, payload_tosend_string.c_str(), payload_len);

    string recovery_payload_string((char*)data + header_tosend_len, payload_len);

    //    zmq_msg_t msg_reply_tosend;
    int rc = zmq_msg_init_data (&msg_reply_tosend, data, msg_size, my_free, NULL);
    assert (rc == 0);

    printf ("Sending Server reply msg ...\n");
    zmq_send (responder, &msg_reply_tosend, 0);
    zmq_msg_close (&msg_reply_tosend);

  }
  //  We never get here but if we did, this would be how we end
  zmq_close (responder);
  zmq_term (context);
  return 0;
}


L3METHOD(make_new_nv)
{
   // make the nameval / nv

   l3path clsname("nv");
   l3path objname("nv");
   l3obj* nobj = 0;
   long extra = sizeof(l3ts::nameval);
   make_new_captag((l3obj*)&objname, extra,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown,ifp);

   nobj->_type = t_nv3;
   nobj->_parent_env  = env;

   l3ts::nameval* my_nv = (l3ts::nameval*)(nobj->_pdb);
   my_nv = new(my_nv) l3ts::nameval; // placement new, so we can compose objects

   *retval = nobj;

}
L3END(make_new_nv)


// l3obj wrapper for namevalue

L3METHOD(nv3)
{
   arity = num_children(exp);
   l3path sexps(exp);
   l3obj* vv  = 0;
   if (arity>0) {
       // screens for unresolved references, gets values up front.
       any_k_arg_op(0,arity,exp,env,&vv,owner,curfo,etyp,retown,ifp);
   }

   long N = 0;
   if (vv) {
       N = ptrvec_size(vv);
   }

   make_new_nv(L3STDARGS);
   l3obj* nobj = *retval;

   l3ts::nameval* my_nv = (l3ts::nameval*)(nobj->_pdb);

#if 0 
   // make the nameval / nv

   l3path clsname("nv");
   l3path objname("nv");
   long extra = sizeof(l3ts::nameval);
   make_new_captag((l3obj*)&objname, extra,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown);

   nobj->_type = t_nv3;
   nobj->_parent_env  = env;

   l3ts::nameval* my_nv = (l3ts::nameval*)(nobj->_pdb);
   my_nv = new(my_nv) l3ts::nameval; // placement new, so we can compose objects

#endif

   //
   // and pushback the contents requested by the initialization
   //
   l3obj* ele = 0;
   //sexp_t* nex = exp->list->next->next;
   for (long i = 0; i < N; ++i) {
        ptrvec_get(vv,i,&ele);

        // gotta do this *before* the nv_pushback, or else the
        //  llref / target cleanup will delete from the nv. not what we want.
        //
        // if vv is the owner, then transfer ownership to the dstaq,
        // so that command line entered values are retained.
        //

        if (ele->_owner == vv->_mytag || ele->_owner->_parent == vv->_mytag) {
            ele->_owner->generic_release_to(ele, nobj->_mytag);
        }
#if 0
        dq_pushback_api(nobj,ele,0);
#endif
    }

#if 1
    load_test_nameval(*my_nv);


    //    load_test_timeseries_data();
#endif

   nobj->_dtor = &nv3_dtor;
   nobj->_cpctor = &nv3_cpctor;
   

   *retval = nobj;

   if (vv) { 
       generic_delete(vv, L3STDARGS_OBJONLY);
   }
}
L3END(nv3)


// nv nameval

L3METHOD(nv3_dtor)
{
    assert(obj->_type == t_nv3);
    l3ts::nameval* nv = (l3ts::nameval*)(obj->_pdb);   
    nv->l3ts::nameval::~nameval();
}
L3END(nv3_dtor)



L3METHOD(nv3_cpctor)
{

    LIVEO(obj);
    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    
    Tag* nv_tag = nobj->_mytag;

    // assert we are a captag pair
    assert( nv_tag );
    assert( pred_is_captag_obj(nobj) );
    
    l3ts::nameval* nv_src = (l3ts::nameval*)(src->_pdb);
    l3ts::nameval* nv_new    = (l3ts::nameval*)(nobj->_pdb);

    // diagnostic:
    
    // don't know how to do "placement copy constructor", so we'll 
    // do it in 2 steps: placement new followed by operator= assignemnt to overwrite.
    nv_new = new(nv_new) l3ts::nameval; 

    // use the underlying assignment operator invocation.
    *nv_new = *nv_src;

    // *retval was already set for us, by the base copy ctor.
}
L3END(nv3_cpctor)

//
// obj   = l3obj* printme 
// exp   = char* indent
// arity = stopset* stoppers
//
// use:  nv_print(printme,L3STD_PRINTCALL);
//
L3METHOD(nv3_print)
{
    assert (obj->_type == t_nv3);

    // obj  is what we want to print

    char*    indent      = (char*)exp;
    stopset* stoppers = 0;

    if (arity != 0) {
        stoppers = (stopset*)arity;
    }

    if (stoppers) {
        stopset_push(stoppers,obj);
    }

    l3ts::nameval* nv  = (l3ts::nameval*)(obj->_pdb);

    l3path s(nv->DebugString().c_str());
    s.outln_indent(indent);
    
}
L3END(nv3_print)


// nv_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize
// arity = fd to write to (already open if not -1). If arity == -1, we'll open the path given by exp and close it when done.
// exp   = char* name of path that fd represents, for diagnostic msg if fd is non-zero.
//
L3METHOD(nv3_save)
{
    assert (obj->_type == t_nv3);

    int   fd = arity;

    l3path outpath;
    if (fd == -1) {
        if (exp) {
            fd = prep_out_path((char*)exp);
            outpath.reinit((char*)exp);
        } else {
            printf("error: no outpath in exp supplied and fd set to -1. No path to save to in nv3_save().\n");
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    l3ts::nameval* nv  = (l3ts::nameval*)(obj->_pdb);

    if (!nv->SerializeToFileDescriptor(fd)) {
        printf("error: failed to serialize nameval message to fd '%s'.\n",outpath());
        perror("errno indicates:");
        l3throw(XABORT_TO_TOPLEVEL);
    }

    *retval = gtrue;
}
L3END(nv3_save)


// obj = an l3ts::Any* that has__nameval() object to read from
// 
// env = an l3ts:: nameval* to read from, overrides the obj if present.
//
// *retval = gets the result
//
// nv3_load just dispatches to on the nameval subtype to the appropriate handler.
//
L3METHOD(nv3_load)
{
    l3ts::nameval* nv = 0;
    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__nameval());
        
        nv = any->mutable__nameval();
    }

    // decode subtype
    t_typ nv_subtype = nameval_to_qtype(nv);
    t_typ recognized = 0;


    DV(printf("debug: nv3_load sees namevalue subtype of '%s'.\n", nv_subtype));
    
    quicktype* qt = 0;
    
    recognized = qtypesys->which_type(nv_subtype, &qt);
    if (recognized && qt->_load) {
        // call the nv subtype loader: which should expect an nameval in obj
        qt->_load(0,  arity,exp, (l3obj*)nv,retval,owner,  curfo,etyp,retown,ifp);
        
    } else {
        printf("error: nv3_load() nv-subtype '%s' does not have a _load op defined, or was not recognized.\n", nv_subtype);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    DV(
       if (retval && *retval) {
           printf("deubg: in nv3_load(): we got back non-zero *retval of:\n");     
           print(*retval,"",0);
       } else {
           printf("deubg: in nv3_load(): we got back *retval of: 0x0 null\n");
       }
       );
    

}
L3END(nv3_load)


//
// (save obj "pathtosaveto")
//
L3KARG(save,2)
{
   l3obj* saveme = 0;
   ptrvec_get(vv,0,&saveme);

   l3obj* opath = 0;
   ptrvec_get(vv,1,&opath);

   if (opath->_type != t_str) {
       printf("error: in call (save obj path), the path specified was not a string.\n");
       if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv = 0; }
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   l3path outpath;
   string_get(opath,0,&outpath);

   outpath.dquote_strip();

   quicktype* qt = 0;
   t_typ recognized = 0;
   *retval = gnil; // default
   int fd = -1;

   l3ts::Any*  any = 0;

   XTRY
      case XCODE:

            any = new Any;
            fd = prep_out_path(outpath());

            // there must now be a save method in the type table
            //
            qt = 0;
            recognized = qtypesys->which_type(saveme->_type, &qt);
            if (recognized && qt->_save) {

                // call it, passing in the type necessary
                curfo = 0; // no stopset to start with here.
                env   = 0;
                qt->_save(saveme,-1,(sexp_t*)any,  env,retval,owner, 0,etyp,retown,ifp);

            } else {
                
                printf("error: save operation not implemented for type '%s'\n",saveme->_type);
                l3throw(XABORT_TO_TOPLEVEL);
            }


            if (!any->SerializeToFileDescriptor(fd)) {
                printf("error: failed to serialize nameval message to fd '%s'.\n",outpath());
                perror("errno indicates:");
                l3throw(XABORT_TO_TOPLEVEL);
            }

        break;
      case XFINALLY:
            if (fd >= 0) { close(fd); }
            if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
            if (any) delete any;
    XENDX

}
L3END(save)

//
//
//
t_typ any_to_l3ts_qtype(l3ts::Any*  any) {
    assert(any);
    
    if (any->has__qts())           return t_qts;

    if (any->has__nameval())       return t_nv3;
    if (any->has__object())        return t_ob3;
    if (any->has__timept())        return t_tm3;

    if (any->has__geoloc())        return t_gl3;
    if (any->has__tmplace())       return t_tp3;
    if (any->has__datapt())        return t_da3;

    if (any->has__timeseries())    return t_ts3;
    if (any->has__beginend())      return t_be3;
    if (any->has__samplespec())    return t_si3;

    if (any->has__beginendset())   return t_bs3;
    if (any->has__when())          return t_wh3;
    if (any->has__tsid())          return t_id3;

    if (any->has__tsdesc())        return t_ds3;
    if (any->has__pb_msg_header()) return t_he3;
    if (any->has__qtreepb())       return t_qt3;

    printf("critical out-of-sync build error: unknown sub-type for l3ts::Any, in any_to_l3ts_qtype()!\n");
    assert(0);
    exit(1);

    return 0;
}

//
// load() translates nv to t_typ using: 
//
t_typ nameval_to_qtype(l3ts::nameval*  nv) {

    assert(nv);

    l3ts::nameval_Type nvTy = nv->nvtyp();

    switch(nvTy) {

    case  nameval_Type_STR: { return t_str; } break;

    case  nameval_Type_DOU: { return t_dou; } break;

    case  nameval_Type_LON: { return t_lng; } break;

    case  nameval_Type_BYT: { return t_byt; } break;

    case  nameval_Type_OBJ:    
        { 
            if (nv->dim_size() >0) {
                return t_vvc;
            }
            return t_obj;
        } 
        break;

    default:
        assert(0); exit(1);
    }

    return 0;
};


//
// (load "pathtoloadfrom")
//
L3KARG(load,1)
{
   l3obj* opath = 0;
   ptrvec_get(vv,0,&opath);

   if (opath->_type != t_str) {
       printf("error: in call (load obj path), the path specified was not a string.\n");
       if (vv) {    generic_delete(vv, L3STDARGS_OBJONLY); }
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   l3path loadpath;
   string_get(opath,0,&loadpath);
   loadpath.dquote_strip();

   l3obj* loadme = 0;
   quicktype* qt = 0;
   t_typ recognized = 0;

   // always load an any... then dispatch on type.

   errno = 0;
   int fd = prep_in_path(loadpath());
   if (-1 == fd) {
       printf("error in load: path '%s' could not be opened.\n",loadpath());
       l3throw(XABORT_TO_TOPLEVEL);
   }

   l3ts::Any*  any = new Any;
   t_typ ty = 0;

   XTRY
      case XCODE:

   errno = 0;
   if (!any->ParseFromFileDescriptor(fd)) {
       printf("error: failed to load l3ts::any from  '%s'.\n",loadpath());
       perror("errno indicates:");
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   DV(printf("report:load read any: \n%s\n",any->DebugString().c_str()));

   // see if there is a load method in the type table
   ty = any_to_l3ts_qtype(any);

   if (!ty) {
       printf("error: load could not translate any type from path '%s' into a quicktype.\n",loadpath());
       l3throw(XABORT_TO_TOPLEVEL);
   }

   recognized = qtypesys->which_type(ty, &qt);
   if (recognized && qt->_load) {
       // call it
       qt->_load((l3obj*)any,-1,(sexp_t*)loadpath(),  0,retval,owner,  curfo,etyp,retown,ifp);

   } else {       
       printf("error: load operation not implemented for type '%s'\n",loadme->_type);
       l3throw(XABORT_TO_TOPLEVEL);
   }
   
   DV(
      if (retval && *retval) {
          printf("debug: in load(): we got back *retval of: \n");
          print(*retval,"",0);
      } else {
          printf("debug: in load(): we got back *retval of: null 0x0\n");
      }
      );

          break;
    case XFINALLY:
        if (any) delete any;
        if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv = 0;}
    XENDX

}
L3END(load)



void ptr_to_uuid_pbsn(void* ptr, l3path& pbsn) {
    uuidstruct u;
    serialfactory->ptr_to_uuid(ptr,&u);
    uuid_unparse(u.uuid, pbsn.buf);
    pbsn.set_p_from_strlen();
}


/// dou_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize, an l3obj* of t_dou
//
// exp   = l3ts::Any*     <- store into this for saving, if set; unless curfo is set.
// env = l3ts::nameval* <- store into this for saving, takes precedence over exp.
// curfo = stopset, but not used for doubles.
//
L3METHOD(dou_save)
{
    assert(obj);
    assert(exp || env);
    assert (obj->_type == t_dou);

    l3ts::Any*    any = 0;
    l3ts::nameval* nv = 0;

    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (exp) {
        any = (l3ts::Any*) exp;
        nv = any->mutable__nameval();

    } else {
        assert(0); exit(1);
    }

    nv->set_nvtyp(l3ts::nameval::DOU);
    l3path pbsn;
    ptr_to_uuid_pbsn(obj, pbsn);

    nv->set_pbsn(pbsn());

    // now use the sparse methods, double_first and double_next
    //    long sz = double_size(obj);
    //    for (long i = 0; i < sz; ++i ) {
    //        nv->add_dou(double_get(obj,i));
    //    }

    long  idx = 0;
    double next = 0;
    bool  sparse = double_is_sparse(obj);

    if (double_first(obj,&next,&idx)) {
        nv->add_dou(next);
        if (sparse) { nv->add_idx(idx); }
        
        while(double_next(obj,&next,&idx)) {
            nv->add_dou(next);
            if (sparse) { nv->add_idx(idx); }
        }
    }
    


    *retval = gtrue;
}
L3END(dou_save)

//
// obj = an l3ts::Any* to read from
//
// env = an l3ts::nameval* to read from
//
L3METHOD(dou_load)
{
    l3ts::nameval* nv = 0;
    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__nameval());
        
        nv = any->mutable__nameval();
    }

    DV(printf("dou_load, nv: \n%s\n",nv->DebugString().c_str()));

    assert(nv->has_nvtyp());
    l3ts::nameval_Type nvTy = nv->nvtyp();
    if (nvTy != nameval_Type_DOU) {
        printf("error: dou_load called by nameval subtype is not DOU, but rather unexpected enum value: %d\n", nvTy);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    // check first to see if we already have the double vector/matrix in memory by uuid.
    // if so, no need to load it again.
    
    std::string pbsn = nv->pbsn();
    if (pbsn.size()) {
        l3obj* alreadyhere = (l3obj*)serialfactory->uuid_to_ptr(pbsn);
        if (alreadyhere) {
            LIVEO(alreadyhere);
            *retval = alreadyhere;
            return 0;
        }
    }


    // make a new t_nv3 l3obj wrapper object. refer to it with nobj.
    l3path newnm("dou_load_double_obj");
    l3obj* nobj = make_new_double_obj(NAN,retown, newnm());

    //    long sz = nv->dou_size();
    //    for (long i = 0; i < sz; ++i) {
    //        double_set(nobj,i, nv->dou(i));
    //    }

    bool sparse = (nv->idx_size() > 0);

    // sanity check
    if (sparse) {
        long idx_sz = nv->idx_size();
        long dou_sz = nv->dou_size();
        if (idx_sz != dou_sz) {
            
            printf("error: dou_load found sparse double array "
                   "(idx_sz is %ld) however we have "
                   "non-matching dou_sz of %ld.\n",
                   idx_sz, 
                   dou_sz
                   );
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    long sz = nv->dou_size();
    long idx = 0;
    for (long i = 0; i < sz; ++i) {
        idx = i;
        if (sparse) {
            idx = nv->idx(i);
        }
        double_set(nobj,idx,nv->dou(i));
    }



    if (nv->has_pbsn()) {
        serialfactory->link_ptr_to_uuid(nobj, nv->pbsn());
    }

    *retval = nobj;
}
L3END(dou_load)


//
// obj = l3path* with filename to open and reinflate protobuf objects from.
//
L3METHOD(open_nv)
{
    l3path* fname = (l3path*)obj;
    
    int fd = open((*fname)(), O_RDONLY);
    
    l3ts::nameval* reinflated  = new l3ts::nameval;
    
    if (!reinflated->ParseFromFileDescriptor(fd)) {
        fprintf(stderr, "Failed reinflate test data '%s'.\n",(*fname)());
        delete reinflated;
        l3throw(XABORT_TO_TOPLEVEL);
    }

    close(fd);
    
    std::cout << "reinflated: " << reinflated->DebugString() << std::endl;

    using google::protobuf::Reflection;
    using google::protobuf::FieldDescriptor;
    using google::protobuf::Descriptor;

    // now reflect on this message
    const Reflection* refl = reinflated->GetReflection();

    const Descriptor* desc = reinflated->GetDescriptor();
    printf("description of message from .proto file '%s'  and message type name is: '%s'\n",
           desc->file()->name().c_str(),  desc->full_name().c_str());

    std::vector< const FieldDescriptor* > vfd;
    refl->ListFields(*reinflated,   &vfd);


    /*
enum FieldDescriptor::Type {
  TYPE_DOUBLE = 1,
  TYPE_FLOAT = 2,
  TYPE_INT64 = 3,
  TYPE_UINT64 = 4,
  TYPE_INT32 = 5,
  TYPE_FIXED64 = 6,
  TYPE_FIXED32 = 7,
  TYPE_BOOL = 8,
  TYPE_STRING = 9,
  TYPE_GROUP = 10,
  TYPE_MESSAGE = 11,
  TYPE_BYTES = 12,
  TYPE_UINT32 = 13,
  TYPE_ENUM = 14,
  TYPE_SFIXED32 = 15,
  TYPE_SFIXED64 = 16,
  TYPE_SINT32 = 17,
  TYPE_SINT64 = 18,
  MAX_TYPE = 18
}

enum FieldDescriptor::CppType {
  CPPTYPE_INT32 = 1,
  CPPTYPE_INT64 = 2,
  CPPTYPE_UINT32 = 3,
  CPPTYPE_UINT64 = 4,
  CPPTYPE_DOUBLE = 5,
  CPPTYPE_FLOAT = 6,
  CPPTYPE_BOOL = 7,
  CPPTYPE_ENUM = 8,
  CPPTYPE_STRING = 9,
  CPPTYPE_MESSAGE = 10,
  MAX_CPPTYPE = 10
}

     */

    //    FieldDescriptor::Type ty = 

    for (::uint i = 0; i < vfd.size(); ++i ) {
        printf("filed %02d  is : '%s'  with type %d\n", i ,  vfd[i]->name().c_str(),  vfd[i]->cpp_type());

    }

}
L3END(open_nv)


L3METHOD(discern_msg_type)
{

    DynamicMessageFactory* dynf = new DynamicMessageFactory;

    std::string data;

    {
        // Create a message and serialize it.
        Foo foo;
        foo.set_text("Hello World!");
        foo.add_numbers(1);
        foo.add_numbers(5);
        foo.add_numbers(42);
        
        foo.SerializeToString(&data);
    }

    {
        //   Parse the serialized message and check that it contains the  correct data.
        Foo foo;
        foo.ParseFromString(data);
        
        assert(foo.text() == "Hello World!");
        assert(foo.numbers_size() == 3);
        assert(foo.numbers(0) == 1);
        assert(foo.numbers(1) == 5);
        assert(foo.numbers(2) == 42);
    }



    //        Same as the last block, but do it dynamically via the Message reflection interface.

    {
        Message* foo = new Foo;
        const Descriptor* descriptor = foo->GetDescriptor();
    


        const Message* msg = dynf->GetPrototype(descriptor);
        
        // get a mutable Foo type, pointed to by base class pointer.
        Message* foomsg = msg->New();
    
        std::cout << "foomsg: " << foomsg->DebugString() << std::endl;

        // Get the descriptors for the fields we're interested in and verify their types.
        
        const FieldDescriptor* text_field = descriptor->FindFieldByName("text");
        
        assert(text_field != NULL);
        assert(text_field->type() == FieldDescriptor::TYPE_STRING);
        assert(text_field->label() == FieldDescriptor::LABEL_OPTIONAL);
        
        
        const FieldDescriptor* numbers_field = descriptor->FindFieldByName("numbers");
        assert(numbers_field != NULL);
        assert(numbers_field->type() == FieldDescriptor::TYPE_INT32);
        assert(numbers_field->label() == FieldDescriptor::LABEL_REPEATED);
        
        //  Parse the message.
        foo->ParseFromString(data);
        
        // Use the reflection interface to examine the contents.
        const Reflection* reflection = foo->GetReflection();
        
        assert(reflection->GetString(*foo, text_field) == "Hello World!");
        assert(reflection->FieldSize(*foo, numbers_field) == 3);
        
        assert(reflection->GetRepeatedInt32(*foo, numbers_field, 0) == 1);
        assert(reflection->GetRepeatedInt32(*foo, numbers_field, 1) == 5);
        assert(reflection->GetRepeatedInt32(*foo, numbers_field, 2) == 42);
        
        delete foo;
        
    }
}
L3END(discern_msg_type)




//
// obj = l3path* with filename to open and reinflate protobuf objects from.
//
L3METHOD(open_any)
{
    l3path* fname = (l3path*)obj;
    
    int fd = open((*fname)(), O_RDONLY);
    
    l3ts::Any* reinflated  = new l3ts::Any;
    
    if (!reinflated->ParseFromFileDescriptor(fd)) {
        fprintf(stderr, "Failed reinflate test data '%s'.\n",(*fname)());
        delete reinflated;
        l3throw(XABORT_TO_TOPLEVEL);
    }

    close(fd);
    
    DV(std::cout << "reinflated: " << reinflated->DebugString() << std::endl);

    using google::protobuf::Reflection;
    using google::protobuf::FieldDescriptor;
    using google::protobuf::Descriptor;

    // now reflect on this message
    const Reflection* refl = reinflated->GetReflection();

    const Descriptor* desc = reinflated->GetDescriptor();
    printf("description of message from .proto file '%s'  and message type name is: '%s'\n",
           desc->file()->name().c_str(),  desc->full_name().c_str());

    std::vector< const FieldDescriptor* > vfd;
    refl->ListFields(*reinflated,   &vfd);


    /*
enum FieldDescriptor::Type {
  TYPE_DOUBLE = 1,
  TYPE_FLOAT = 2,
  TYPE_INT64 = 3,
  TYPE_UINT64 = 4,
  TYPE_INT32 = 5,
  TYPE_FIXED64 = 6,
  TYPE_FIXED32 = 7,
  TYPE_BOOL = 8,
  TYPE_STRING = 9,
  TYPE_GROUP = 10,
  TYPE_MESSAGE = 11,
  TYPE_BYTES = 12,
  TYPE_UINT32 = 13,
  TYPE_ENUM = 14,
  TYPE_SFIXED32 = 15,
  TYPE_SFIXED64 = 16,
  TYPE_SINT32 = 17,
  TYPE_SINT64 = 18,
  MAX_TYPE = 18
}

enum FieldDescriptor::CppType {
  CPPTYPE_INT32 = 1,
  CPPTYPE_INT64 = 2,
  CPPTYPE_UINT32 = 3,
  CPPTYPE_UINT64 = 4,
  CPPTYPE_DOUBLE = 5,
  CPPTYPE_FLOAT = 6,
  CPPTYPE_BOOL = 7,
  CPPTYPE_ENUM = 8,
  CPPTYPE_STRING = 9,
  CPPTYPE_MESSAGE = 10,
  MAX_CPPTYPE = 10
}

     */

    //    FieldDescriptor::Type ty = 

    for (::uint i = 0; i < vfd.size(); ++i ) {
        printf("filed %02d  is : '%s'  with type %d\n", i ,  vfd[i]->name().c_str(),  vfd[i]->cpp_type());

    }

}
L3END(open_any)


L3METHOD(write_any)
{
    assert (obj->_type == t_nv3);

    int   fd = arity;

    l3path outpath;
    if (fd == -1) {
        if (exp) {
            fd = prep_out_path((char*)exp);
            outpath.reinit((char*)exp);
        } else {
            printf("error: no outpath in exp supplied and fd set to -1. No path to save to in nv3_save().\n");
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }


    l3ts::Any  any;
    
    l3ts::nameval* nv = any.mutable__nameval();
    nv->set_nvtyp(l3ts::nameval::DOU);
    nv->add_dou(43.0);

    if (!nv->SerializeToFileDescriptor(fd)) {
        printf("error: failed to serialize nameval message to fd '%s'.\n",outpath());
        perror("errno indicates:");
        l3throw(XABORT_TO_TOPLEVEL);
    }

}
L3END(write_any)


#define STUB3_NOLOAD_NOSAVE(tname) \
L3METHOD(tname)                       \
{ \
   printf("method: " #tname "() stub called!\n"); \
} \
L3END(tname) \
\
L3METHOD(tname##_print)                       \
{ \
   printf("method: " #tname "_print stub called!\n"); \
} \
L3END(tname##_print) \
\
L3METHOD(tname##_cpctor)                       \
{ \
   printf("method: " #tname "_cpctor stub called!\n"); \
} \
L3END(tname##_cpctor) \
\
L3METHOD(tname##_dtor)                       \
{ \
   printf("method: " #tname "_dtor stub called!\n"); \
} \
L3END(tname##_dtor) \


#define STUB3_NOLOAD(tname) \
    STUB3_NOLOAD_NOSAVE(tname) \
L3METHOD(tname##_save)                       \
{ \
   printf("method: " #tname "_save stub called!\n"); \
} \
L3END(tname##_save) \



#define STUB3(tname) \
     STUB3_NOLOAD(tname) \
     L3METHOD(tname##_load)                     \
     {                                               \
         printf("method: " #tname "_load stub called!\n");  \
     }                                                      \
     L3END(tname##_load) \


//
// obj = l3ts::Any*, which has__object()
//
// env = l3ts::nameval* nv with obj_size() > 0, which overrides obj/any if present.
//
L3METHOD(ob3_load)
{
    DV(printf("method: ob3_load called!\n"));

    l3ts::Any* any = 0;
    l3ts::object* o = 0;
    char* nv_pbsn = 0;

    if (env) {
        l3ts::nameval* nv = (l3ts::nameval*) env;
        if (nv->obj_size() <= 0) {
            // really nothing to load here
            printf("warning in ob3_load(): hmmm, no obj set on the nameval... error in dispatching to us?\n");
            return 0;
        } else {
            o = nv->mutable_obj(0);

            if (nv->has_pbsn()) {
                nv_pbsn = (char*)(nv->pbsn().c_str());
            }
        }

    } else if (obj) {

        any = (l3ts::Any*)obj;
        assert(any->has__object());

        o = any->mutable__object();
        assert(o);        
    }

    DV(printf("ob3_load, o: \n%s\n",o->DebugString().c_str()));

    l3path oname("ob3_loaded");
    l3path cname("ob3_loaded");

    if (o->has_objname()) {
        oname.reinit(o->objname().c_str());
    }

    if (o->has_clsname()) {
        cname.reinit(o->clsname().c_str());        
    }


    // make a new t_nv3 l3obj wrapper object. refer to it with nobj.
    l3path newnm("ob3_load_obj");
    make_new_obj(cname(),oname(), retown, 0, retval);
    
    l3obj* nobj = *retval;
    assert(nobj->_type == t_obj);

    // and link to nv_pbsn if available
    if (nv_pbsn) {
        serialfactory->link_ptr_to_uuid(nobj, nv_pbsn);
    }

    // load the values first, so the keys will later know who to refer to

    long N = o->nv_size();
    l3ts::nameval* nv = 0;
    l3obj*  nextval = 0;
    for(long i =0; i < N; ++i) {
        nv = o->mutable_nv(i);

        DV(printf("ob3_load, nv[%ld]: \n%s\n", i, nv->DebugString().c_str()));

        nv3_load(0, arity,exp, (l3obj*)nv,&nextval, owner,  curfo,etyp, nobj->_mytag,ifp);
    }


    N = o->key_size();
    l3ts::key* k = 0;
    for(long i =0; i < N; ++i) {
        k = o->mutable_key(i);

        DV(printf("ob3_load, key[%ld]: \n%s\n", i, k->DebugString().c_str()));

        if (k->has_strkey() && k->has_pbsn_target()) {
            const std::string& strkey = k->strkey();
            const std::string& pbsn   = k->pbsn_target();
            
            l3obj* ptr     = (l3obj*)serialfactory->uuid_to_ptr(pbsn);
            if (ptr) {
                LIVEO(ptr);
                add_alias_eno(nobj,(char*)strkey.c_str(),ptr);
            } else {
                // this will happen all the time when the original top level
                // variable is no longer present, or upon first arival. Don't make a big deal about it.
                // Really? actually this was a real bug and now it's fixed. Turn back on the warning.
                printf("warning in ob3_load(): upon reinflation, could not locate ser# corresponding to uuid '%s'.\n",pbsn.c_str());
            }

        } else {
            // not sure what to do with 
            // (a) a key that points nowhere
            // (b) a value with no name.
            //
            // should we require them in the key?

        }
    }


} 
L3END(ob3_load) 


STUB3_NOLOAD(ob3)


STUB3_NOLOAD_NOSAVE(tm3)

STUB3_NOLOAD_NOSAVE(qts)


STUB3(gl3)
STUB3(tp3)
STUB3(da3)

STUB3(ts3)
STUB3(be3)
STUB3(si3)

STUB3(bs3)
STUB3(wh3)
STUB3(id3)

STUB3(ds3)
STUB3(he3)
STUB3(qt3)

// nameval subtypes:
//
//     case  nameval_Type_STR: { return t_str; } break;

STUB3_NOLOAD_NOSAVE(str)



//
// obj = an l3ts::Any* to read from
//
// env = an l3ts::nameval* to read from
//
L3METHOD(str_load)
{

   printf("method: str_load called!\n");

    l3ts::nameval* nv = 0;
    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__nameval());
        
        nv = any->mutable__nameval();
    }

    //DV
    printf("str_load, nv: \n%s\n",nv->DebugString().c_str());

    assert(nv->has_nvtyp());
    l3ts::nameval_Type nvTy = nv->nvtyp();
    if (nvTy != nameval_Type_STR) {
        printf("error: str_load called by nameval subtype is not STR, but rather unexpected enum value: %d\n", nvTy);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    // check first to see if we already have the double vector/matrix in memory by uuid.
    // if so, no need to load it again.
    
    std::string pbsn = nv->pbsn();
    if (pbsn.size()) {
        l3obj* alreadyhere = (l3obj*)serialfactory->uuid_to_ptr(pbsn);
        if (alreadyhere) {
            LIVEO(alreadyhere);
            *retval = alreadyhere;
            return 0;
        }
    }


    // make a new t_nv3 l3obj wrapper object. refer to it with nobj.
    l3path newnm("str_load_string_obj");
    l3obj* nobj = make_new_string_obj(0,retown, newnm());

    bool sparse = (nv->idx_size() > 0);

    // sanity check
    if (sparse) {
        long idx_sz = nv->idx_size();
        long str_sz = nv->str_size();
        if (idx_sz != str_sz) {
            
            printf("error: str_load found sparse string array "
                   "(idx_sz is %ld) however we have "
                   "non-matching str_sz of %ld.\n",
                   idx_sz, 
                   str_sz
                   );
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    long sz = nv->str_size();
    long idx = 0;
    for (long i = 0; i < sz; ++i) {
        idx = i;
        if (sparse) {
            idx = nv->idx(i);
        }
        string_set(nobj,idx, (char*)nv->str(i).c_str());
    }

    if (nv->has_pbsn()) {
        serialfactory->link_ptr_to_uuid(nobj, nv->pbsn());
    }

    *retval = nobj;
}
L3END(str_load)



/// str_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize, an l3obj* of t_str
//
// exp   = l3ts::Any*     <- store into this for saving, if set; unless curfo is set.
// env = l3ts::nameval* <- store into this for saving, takes precedence over exp.
// curfo = stopset, but not used for strings
//
L3METHOD(str_save)
{
    assert(obj);
    assert(exp || env);
    assert (obj->_type == t_str);

    l3ts::Any*    any = 0;
    l3ts::nameval* nv = 0;

    if (env) {
        nv = (l3ts::nameval*)env;

    } else if (exp) {
        any = (l3ts::Any*) exp;
        nv = any->mutable__nameval();

    } else {
        assert(0); exit(1);
    }

    nv->set_nvtyp(l3ts::nameval::STR);
    l3path pbsn;
    ptr_to_uuid_pbsn(obj, pbsn);

    nv->set_pbsn(pbsn());

    long  idx = 0;
    char* next = 0;
    bool  sparse = string_is_sparse(obj);

    if (string_first(obj,&next,&idx)) {
        nv->add_str(next);
        if (sparse) { nv->add_idx(idx); }
        
        while(string_next(obj,&next,&idx)) {
            nv->add_str(next);
            if (sparse) { nv->add_idx(idx); }
        }
    }
    
    *retval = gtrue;
}
L3END(str_save)



//    case  nameval_Type_DOU: { return t_dou; } break;
// already defined manually above: STUB3(dou) 



//    case  nameval_Type_LON: { return t_lng; } break;
STUB3(lng)


//    case  nameval_Type_BYT: { return t_byt; } break;
STUB3(byt)


//    case  nameval_Type_OBJ, with dim_size()==0:    { return t_obj; } break;
// STUB3(obj)

//    case  nameval_Type_OBJ, with dim_size()>0: { return t_vvc; } break;
STUB3(vvc)


L3METHOD(obj_load)                       
{ 
   printf("method: obj_load stub called!\n"); 
} 
L3END(obj_load) 



// obj = saveme
// exp = any to save into, if set
// env = nameval to save into (over-ride exp) if set.
// curfo = stoppers
//
L3METHOD(obj_save)
{
    DV(printf("method: obj_save called!\n")); 

    stopset* stoppers = (stopset*)curfo;

    l3obj* saveme = obj;
    assert (saveme->_type == t_obj);
    l3ts::Any* any = 0;
    l3ts::object* o = 0;
    l3ts::nameval*  nv=0;       
    l3path pbsn;
    
    if (env) {
        nv = (l3ts::nameval*)env;
        o = nv->add_obj();
        nv->set_nvtyp(l3ts::nameval_Type_OBJ);

        ptr_to_uuid_pbsn(saveme, pbsn);
        nv->set_pbsn(pbsn());

    } else if (exp) {
        any = (l3ts::Any*)exp;
        o = any->mutable__object();

    }

    assert(o);

    o->set_objname( get_objname(saveme));
    o->set_clsname( get_clsname(saveme));
    
    size_t      sz =0;

    PWord_t      PValue = 0;                   // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.
    char    indent[] = "      ";

    Index[0] = '\0';                    // start with smallest string.
    JSLF(PValue, obj->_judyS->_judyS, Index);       // get first string
    char buf[BUFSIZ];
    int count = 0;
    t_typ known_type = 0;
    quicktype* qt = 0;
    l3ts::nameval*  new_nv = 0;

    l3ts::key* nkey = 0;

    bool isstop = false;
    t_typ ty = 0;
    while (PValue != NULL)
    {
        bzero(buf,BUFSIZ);
        llref* llre = (llref*)(*PValue);
        LIVEREF(llre);

        l3obj* ele  = llre->_obj;
        assert(ele);

        if (stoppers) {
            isstop = obj_in_stopset(stoppers,ele);
        }

        // allow gVerboseFunction == 3 to show all builtin vars.
        if (is_sysbuiltin(llre)) {
            goto NEXT_JUDYS_STRING;
        }

        ty = ele->_type;

        nkey = o->add_key();
        nkey->set_strkey((char*)Index);

        pbsn.clear();
        ptr_to_uuid_pbsn(ele, pbsn);
        nkey->set_pbsn_target(pbsn());

        new_nv = o->add_nv();

        if (ele && !isstop) {
            LIVEO(ele);
            // handle user-defined, table driven extensible types
            qt = 0;
            known_type = qtypesys->which_type(ty, &qt);
            if (!known_type) {
                printf("error in print: unknown type '%s' in object %p ser# %ld.\n", ty, ele, ele->_ser);
                l3throw(XABORT_TO_TOPLEVEL);
            }
            
            if (qt && qt->_save) {
                
                // user defined type system... invoke the defined print method.
                printf("%s%02d:%s -> (%p ser# %ld %s):\n", indent, count, Index, ele, ele->_ser, ele->_type);
                qt->_save(ele, -1, 0, (l3obj*)new_nv, retval,owner, (l3obj*)stoppers,etyp,retown,ifp);
                
            }

        NEXT_JUDYS_STRING:

            JSLN(PValue, obj->_judyS->_judyS, Index);   // get next string
            ++sz;
            ++count;

        } // end if ele && !stop
    } // end while PValue != NULL


    DV(
       if (o) { std::cout << "obj_save, o : \n" << o->DebugString() << std::endl; }
       if (any) { std::cout << "obj_save, any: \n" << any->DebugString() << std::endl; }
       );
    *retval = gtrue;
} 
L3END(obj_save) 




// extern t_typ   t_tm3; //   ="t_tm3"; // l3ts::timept




//
// obj = an l3ts::Any* to read from
//
// env = an l3ts::timept* to read from
//
L3METHOD(tm3_load)
{

   printf("method: tm3_load called!\n");

    l3ts::timept* tm = 0;
    if (env) {
        tm = (l3ts::timept*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__timept());
        
        tm = any->mutable__timept();
    } else {
        assert(0);
    }

    //DV
    printf("tm3_load, tm: \n%s\n",tm->DebugString().c_str());



    //    *retval = nobj;
}
L3END(tm3_load)



/// tm3_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize, an l3obj* of t_str
//
// exp   = l3ts::Any*     <- store into this for saving, if set; unless curfo is set.
// env = l3ts::timept* <- store into this for saving, takes precedence over exp.
// curfo = stopset, but not used for strings
//
L3METHOD(tm3_save)
{
    assert(obj);
    assert(exp || env);
    assert (obj->_type == t_str);

    l3ts::Any*    any = 0;
    l3ts::timept* tm = 0;

    if (env) {
        tm = (l3ts::timept*)env;

    } else if (exp) {
        any = (l3ts::Any*) exp;
        tm = any->mutable__timept();

    } else {
        assert(0); exit(1);
    }


    *retval = gnil;
}
L3END(tm3_save)

////////////////////////////////////
//
// qts = quick time series. small, compact, bare-bones time series.
//


//
// obj = an l3ts::Any* to read from
//
// env = an l3ts::timept* to read from
//
L3METHOD(qts_load)
{

   printf("method: qts_load called!\n");

    l3ts::qts* my_qts = 0;
    if (env) {
        my_qts = (l3ts::qts*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__qts());
        
        my_qts = any->mutable__qts();
    } else {
        assert(0);
    }

    //DV
    printf("qts_load, my_qts: \n%s\n",my_qts->DebugString().c_str());



    //    *retval = nobj;
}
L3END(qts_load)



/// qts_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize, an l3obj* of t_str
//
// exp = l3ts::Any*     <- store into this for saving, if set; unless curfo is set.
// env = l3ts::qts* <- store into this for saving, takes precedence over exp.
// curfo = stopset, but not used for strings
//
L3METHOD(qts_save)
{
    assert(obj);
    assert(exp || env);
    assert (obj->_type == t_str);

    l3ts::Any*    any = 0;
    l3ts::qts* my_qts = 0;

    if (env) {
        my_qts = (l3ts::qts*)env;

    } else if (exp) {
        any = (l3ts::Any*) exp;
        my_qts = any->mutable__qts();

    } else {
        assert(0); exit(1);
    }


    *retval = gnil;
}
L3END(qts_save)




////////////////////////////////////
//
// qtreepb :
//    protobuf serialized form of qtree node: code as data. a function, func or FUN.
//
/*
message qtreepb {

    optional qtreepb  _head  = 1;
    required string   _val   = 2;
    required string   _ty    = 3;
    repeated qtreepb  _chld  = 4;

    optional int32    _lbp   = 5;
    optional int32    _rbp   = 6;

    optional bool     _prefix  = 7 [default = false];
    optional bool     _postfix = 8 [default = false];

    optional qtreepb  _body  = 9;
    optional string   _span  = 20;
    optional string   _closer = 21;

}
*/


//
// obj = an l3ts::Any* to read from
//
// env = an l3ts::qtreepb* to read from
//
L3METHOD(qtreepb_load)
{

   printf("method: qtreepb_load called!\n");

    l3ts::qtreepb* my_qtreepb = 0;
    if (env) {
        my_qtreepb = (l3ts::qtreepb*)env;

    } else if (obj) {
        l3ts::Any* any = (l3ts::Any*) obj;
        assert(any->has__qtreepb());
        
        my_qtreepb = any->mutable__qtreepb();
    } else {
        assert(0);
    }

    


    //DV
    printf("qtreepb_load, my_qtreepb: \n%s\n",my_qtreepb->DebugString().c_str());

    //    *retval = nobj;
}
L3END(qtreepb_load)



/// qtreepb_save : internal method, shouldn't be expected to handle sexp_t input. To be called by a generic (save) method.
//
// obj = what to serialize, a qtree*
//
// exp = l3ts::Any*     <- store into this for saving, if set; unless curfo is set.
// env = l3ts::qtreepb* <- store into this for saving, takes precedence over exp.
// curfo = stopset, but not used for strings
//
L3METHOD(qtreepb_save)
{
    assert(obj);
    assert(exp || env);

    qtree* saveme = (qtree*)obj;
    assert(saveme->_type == t_qtr);

    l3ts::Any*    any = 0;
    l3ts::qtreepb* my_qtreepb = 0;

    if (env) {
        my_qtreepb = (l3ts::qtreepb*)env;

    } else if (exp) {
        any = (l3ts::Any*) exp;
        my_qtreepb = any->mutable__qtreepb();
        assert(my_qtreepb);

    } else {
        assert(0); exit(1);
    }

    saveme->serialize(my_qtreepb);

    *retval = gnil;
}
L3END(qtreepb_save)

