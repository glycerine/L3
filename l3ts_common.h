#ifndef _L3TS_COMMON_H_
#define _L3TS_COMMON_H_

// common l3ts zmq / protobuf routines.


int l3ts_server_main (int argc, char** argv);
int l3ts_client_main (int argc, char** argv);

// determine the size of the header by creating a header and serializing it
//
size_t get_header_len();

void my_free (void *data, void *hint);



#endif /* _L3TS_COMMON_H_ */
