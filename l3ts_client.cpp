//
//  Hello Protocol Buffers client
//  Connects REQ socket to tcp://localhost:4444
//  Sends "Hello" to server, expects "World" back
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

#include <zmq.h>
#include "l3ts.pb.h"
#include "l3path.h"
#include "l3ts_common.h"

using std::string;
using std::cout;
using std::endl;

using namespace l3ts;

#if 0   // moved to l3ts_common.cpp

void my_free (void *data, void *hint)
{
  free (data);
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

#if  0   // ndef _NOMAIN_

int main(int argc, char** argv) {
  return l3ts_client_main (argc, argv);
}
#endif



int l3ts_client_main (int argc, char** argv)
{
  void *context = zmq_init (1);

  //  Socket to talk to server
  printf ("Connecting to hello world server...\n");
  void *requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect (requester, "tcp://localhost:4444");


  // allocate received reply classes: for de-marshalling
  l3ts::pb_msg_header  header_recvd;
  l3ts::timeseries timeseries_recvd;

  // allocate transmission/request classes: for marshalling
  l3ts::pb_msg_header header;
  l3ts::timeseries n;


  int request_nbr;
  for (request_nbr = 0; request_nbr != 10; request_nbr++) {

    // clear out the old timeseries
    n.Clear();
#if 0
    l3ts::datapt* dp = n.mutable_datapt();

    l3ts::tspoints* tspt = mutable_tspoints();

    l3ts::When*   wh = new l3ts::


    // begin new send code

    ts->set_id("45");
    ts->set_me("I'm me!");
    //ts->mutable_uid()->add_v("my uuid");
    ts->add_anno("my annotation is here!");
    ts->add_anno("my second annotation is here!");
    
    ts->add_dv(request_nbr);
    ts->add_dv(1.2);
    
#endif

    // strings to hold serialized versions of header and variable length payload
    string header_string;
    string payload_string;

    // serializes the message and stores the bytes in the given string. Note that the bytes are binary, not text; we only use the string class as a convenient container.
    assert(n.SerializeToString(&payload_string));
    size_t payload_len = payload_string.size();

    // header uses a fixed size message as header.
    header.set_npayload_bytes(payload_len);
    assert(header.SerializeToString(&header_string));
    size_t header_len = header_string.size();
    
    std::cout << "\npbclient, client request prepared, here is the pb dump of header (type l3ts::pb_msg_header):\n" << header.DebugString() << std::endl;
    std::cout << "header_len on sender side is: " << header_len << endl;

    std::cout << "\npbclient, client request prepared, here is the pb dump of timeseries   (type l3ts::timeseries):\n"          << n.DebugString() << std::endl;
    std::cout << "payload_len on sender side is: " << payload_len << endl;


    // terminating '\0' is never counted, in either header or payload.
    size_t msg_size = header_len + payload_len;
    
    char* data = (char*)malloc (msg_size);
    assert (data);
    bzero(data,msg_size);
    memcpy (data, header_string.c_str() ,header_len);

    string recovery_header_string((char*)data, header_len);
    assert(recovery_header_string == header_string);
    for (uint i = 0; i < header_len; ++i) {
      assert( header_string[i] == recovery_header_string[i]);
    }

    void* payload_start_addr = ((char*)data + header_len);
    memcpy (payload_start_addr, payload_string.c_str(), payload_len);

    string recovery_payload_string((char*)data + header_len, payload_len);
    for (uint i = 0; i < payload_len; ++i) {
      assert( payload_string[i] == recovery_payload_string[i]);
    }
    assert(recovery_payload_string == payload_string);

    zmq_msg_t msg_to_send;
    int rc = zmq_msg_init_data (&msg_to_send, data, msg_size, my_free, NULL);
    assert (rc == 0);

    printf ("Sending Client request msg %d...\n", request_nbr);
    zmq_send (requester, &msg_to_send, 0);
    zmq_msg_close (&msg_to_send);


    // begin receive code


    zmq_msg_t reply;
    zmq_msg_init (&reply);
    if (0 != zmq_recv (requester, &reply, 0)) {
      throw "zmq_recv failed";
    }

    // begin processing the reply from the server
    void* pdata = zmq_msg_data(&reply);
    string header_recvd_string((char*)pdata,  hlen);

    assert(header_recvd.ParseFromString(header_recvd_string));

    size_t payload_recvd_len = header_recvd.npayload_bytes();
    printf("payload_recvd_len is %lu\n", payload_recvd_len);

    std::cout << "pbclient: Received reply from server, where is the pb dump of header is:\n" << header_recvd.DebugString() << std::endl;
    string payload_recvd_string(((char*)pdata) + hlen, payload_recvd_len);

    // parses a message from the given string.
    assert(timeseries_recvd.ParseFromString(payload_recvd_string));

    std::cout << "pbclient: Received reply from server on request # " << request_nbr <<  ", here is the pb dump of timeseries_recvd:\n" << timeseries_recvd.DebugString() << std::endl;
    
    zmq_msg_close (&reply);

    } // end request_nbr loop

  zmq_close (requester);
  zmq_term (context);
  return 0;
}


