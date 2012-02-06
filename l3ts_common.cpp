#include <stdio.h>
#include <stdlib.h>
#include "l3ts_common.h"
#include "l3ts.pb.h"

// determine the size of the header by creating a header and serializing it
//
size_t get_header_len() {
  std::string sizing_header_string;
  l3ts::pb_msg_header sizing_header;
  sizing_header.set_npayload_bytes(99);
  assert(sizing_header.SerializeToString(&sizing_header_string));
  size_t header_len = sizing_header_string.size();
  return header_len;
}


void my_free (void *data, void *hint)
{
  free (data);
}
