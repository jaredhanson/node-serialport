#include <uv.h>
#include "internal.h"

void uv_serialport_req_init(uv_loop_t* loop, uv_req_t* req) {
  loop->counters.req_init++;
  req->type = UV_UNKNOWN_REQ;
  SET_REQ_SUCCESS(req);
}
