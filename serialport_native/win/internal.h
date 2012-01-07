#ifndef UV_SERIALPORT_WIN_INTERNAL_
#define UV_SERIALPORT_WIN_INTERNAL_

#include <uv.h>
#include "winapi.h"

void uv_serialport_req_init(uv_loop_t* loop, uv_req_t* req);

uv_err_code uv_serialport_translate_sys_error(int sys_errno);
void uv_serialport__set_error(uv_loop_t* loop, uv_err_code code, int sys_error);
void uv_serialport__set_sys_error(uv_loop_t* loop, int sys_error);


#define SET_REQ_STATUS(req, status)                                     \
   (req)->overlapped.Internal = (ULONG_PTR) (status)

#define SET_REQ_ERROR(req, error)                                       \
  SET_REQ_STATUS((req), NTSTATUS_FROM_WIN32((error)))

#define SET_REQ_SUCCESS(req)                                            \
  SET_REQ_STATUS((req), STATUS_SUCCESS)

#endif /* UV_SERIALPORT_WIN_INTERNAL_ */
