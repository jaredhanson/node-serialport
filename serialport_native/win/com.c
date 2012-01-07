#include <stdio.h>
#include <uv.h>
#include "internal.h"

#define UV_FS_LAST_ERROR_SET     0x0020

#define SET_UV_LAST_ERROR_FROM_REQ(req)                                     \
  if (req->flags & UV_FS_LAST_ERROR_SET) {                                  \
    uv_serialport__set_sys_error(req->loop, (uv_err_code)req->last_error);                          \
  } else if (req->result == -1) {                                           \
    uv_serialport__set_error(req->loop, (uv_err_code)req->errorno, req->last_error);   \
  }

#define SET_REQ_RESULT(req, result_value)                                   \
  req->result = (result_value);                                             \
  if (req->result == -1) {                                                  \
    req->last_error = _doserrno;                                            \
    req->errorno = uv_serialport_translate_sys_error(req->last_error);                 \
  }

#define VERIFY_UV_FILE(file, req)                                           \
  if (file == -1) {                                                         \
    req->result = -1;                                                       \
    req->errorno = UV_EBADF;                                                \
    req->last_error = ERROR_SUCCESS;                                        \
    return;                                                                 \
  }


static void uv_com_req_init_sync(uv_loop_t* loop, uv_fs_t* req,
    uv_fs_type fs_type) {
  uv_serialport_req_init(loop, (uv_req_t*) req);
  req->type = UV_FS;
  req->loop = loop;
  req->flags = 0;
  req->fs_type = fs_type;
  req->result = 0;
  req->ptr = NULL;
  req->path = NULL;
  req->pathw = NULL;
  req->errorno = 0;
}


void com__read(uv_fs_t* req, uv_file file, void *buf, size_t length,
    off_t offset) {
  HANDLE handle;
  OVERLAPPED overlapped, *overlapped_ptr;
  LARGE_INTEGER offset_;
  DWORD bytes;

  VERIFY_UV_FILE(file, req);

  // Attempting to associate a HANDLE to a communications resource (a.k.a.
  // serial port) with a file descriptor using _open_osfhandle / _get_osfhandle
  // causes a crash.  Because uv_fs_* functions do just that, they cannot be
  // used here.
  handle = (HANDLE) file;
  if (handle == INVALID_HANDLE_VALUE) {
    SET_REQ_RESULT(req, -1);
    return;
  }

  if (length > INT_MAX) {
    SET_REQ_ERROR(req, ERROR_INSUFFICIENT_BUFFER);
    return;
  }

  if (offset != -1) {
    memset(&overlapped, 0, sizeof overlapped);

    offset_.QuadPart = offset;
    overlapped.Offset = offset_.LowPart;
    overlapped.OffsetHigh = offset_.HighPart;

    overlapped_ptr = &overlapped;
  } else {
    overlapped_ptr = NULL;
  }

  if (ReadFile(handle, buf, length, &bytes, overlapped_ptr)) {
    SET_REQ_RESULT(req, bytes);
  } else {
    SET_REQ_ERROR(req, GetLastError());
  }
}


int uv_com_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf,
    size_t length, off_t offset, uv_fs_cb cb) {
  if (cb) {
  	fprintf(stdout, "TODO: Async Support Not Implemented\n");
    //uv_fs_req_init_async(loop, req, UV_FS_READ, NULL, NULL, cb);
    //WRAP_REQ_ARGS4(req, file, buf, length, offset);
    //QUEUE_FS_TP_JOB(loop, req);
  } else {
    uv_com_req_init_sync(loop, req, UV_FS_READ);
    com__read(req, file, buf, length, offset);
    SET_UV_LAST_ERROR_FROM_REQ(req);
    return req->result;
  }

  return 0;
}
