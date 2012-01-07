#include <uv.h>
#include "internal.h"

void uv_serialport__set_error(uv_loop_t* loop, uv_err_code code, int sys_error) {
  loop->last_err.code = code;
  loop->last_err.sys_errno_ = sys_error;
}

void uv_serialport__set_sys_error(uv_loop_t* loop, int sys_error) {
  loop->last_err.code = uv_serialport_translate_sys_error(sys_error);
  loop->last_err.sys_errno_ = sys_error;
}
