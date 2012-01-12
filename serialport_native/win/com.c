#include <stdio.h>
#include <assert.h>
#include <uv.h>
#include "internal.h"

#define UV_FS_ASYNC_QUEUED       0x0001
#define UV_FS_LAST_ERROR_SET     0x0020

#define UTF8_TO_UTF16(s, t)                                                 \
  size = uv_serialport_utf8_to_utf16(s, NULL, 0) * sizeof(wchar_t);                    \
  t = (wchar_t*)malloc(size);                                               \
  if (!t) {                                                                 \
    uv_serialport_fatal_error(ERROR_OUTOFMEMORY, "malloc");                            \
  }                                                                         \
  if (!uv_serialport_utf8_to_utf16(s, t, size / sizeof(wchar_t))) {                    \
    uv_serialport__set_sys_error(loop, GetLastError());                                \
    return -1;                                                              \
  }

#define WRAP_REQ_ARGS1(req, a0)                                             \
  req->arg0 = (void*)a0;

#define WRAP_REQ_ARGS2(req, a0, a1)                                         \
  WRAP_REQ_ARGS1(req, a0)                                                   \
  req->arg1 = (void*)a1;

#define WRAP_REQ_ARGS3(req, a0, a1, a2)                                     \
  WRAP_REQ_ARGS2(req, a0, a1)                                               \
  req->arg2 = (void*)a2;

#define WRAP_REQ_ARGS4(req, a0, a1, a2, a3)                                 \
  WRAP_REQ_ARGS3(req, a0, a1, a2)                                           \
  req->arg3 = (void*)a3;

#define QUEUE_FS_TP_JOB(loop, req)                                          \
  if (!QueueUserWorkItem(&uv_com_thread_proc,                                \
                         req,                                               \
                         WT_EXECUTELONGFUNCTION)) {                         \
    uv_serialport__set_sys_error((loop), GetLastError());                              \
    return -1;                                                              \
  }                                                                         \
  req->flags |= UV_FS_ASYNC_QUEUED;                                         \
  uv_ref((loop));

#define SET_UV_LAST_ERROR_FROM_REQ(req)                                     \
  if (req->flags & UV_FS_LAST_ERROR_SET) {                                  \
    uv_serialport__set_sys_error(req->loop, (uv_err_code)req->last_error);                          \
  } else if (req->result == -1) {                                           \
    uv_serialport__set_error(req->loop, (uv_err_code)req->errorno, req->last_error);   \
  }

#define SET_REQ_LAST_ERROR(req, error)                                      \
  req->last_error = error;                                                  \
  req->flags |= UV_FS_LAST_ERROR_SET;

#define SET_REQ_RESULT(req, result_value)                                   \
  req->result = (result_value);                                             \
  if (req->result == -1) {                                                  \
    req->last_error = _doserrno;                                            \
    req->errorno = uv_serialport_translate_sys_error(req->last_error);                 \
  }

#define SET_REQ_RESULT_WIN32_ERROR(req, sys_errno)                          \
  req->result = -1;                                                         \
  req->errorno = uv_serialport_translate_sys_error(sys_errno);                         \
  SET_REQ_LAST_ERROR(req, sys_errno);

#define VERIFY_UV_FILE(file, req)                                           \
  if (file == -1) {                                                         \
    req->result = -1;                                                       \
    req->errorno = UV_EBADF;                                                \
    req->last_error = ERROR_SUCCESS;                                        \
    return;                                                                 \
  }


static void uv_com_req_init_async(uv_loop_t* loop, uv_fs_t* req,
    uv_fs_type fs_type, const char* path, const wchar_t* pathw, uv_fs_cb cb) {
  uv_serialport_req_init(loop, (uv_req_t*) req);
  req->type = UV_FS;
  req->loop = loop;
  req->flags = 0;
  req->fs_type = fs_type;
  req->cb = cb;
  req->result = 0;
  req->ptr = NULL;
  req->path = path ? strdup(path) : NULL;
  req->pathw = (wchar_t*)pathw;
  req->errorno = 0;
  req->last_error = 0;
  memset(&req->overlapped, 0, sizeof(req->overlapped));
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


void com__open(uv_fs_t* req, const wchar_t* path, int flags, int mode) {
  DWORD access;
  DWORD share;
  DWORD disposition;
  DWORD attributes;
  HANDLE file;
  int result;

  access = GENERIC_READ | GENERIC_WRITE;
  share = 0;
  disposition = OPEN_EXISTING;
  attributes = 0;
  // TODO: Does FILE_FLAG_OVERLAPPED need to be set on attributes?
  // attributes = FILE_FLAG_OVERLAPPED;

  file = CreateFileW(path,
                     access,
                     share,
                     NULL,
                     disposition,
                     attributes,
                     NULL);

  if (file == INVALID_HANDLE_VALUE) {
    SET_REQ_RESULT_WIN32_ERROR(req, GetLastError());
    return;
  }
  // Attempting to associate a HANDLE to a communications resource (a.k.a.
  // serial port) with a file descriptor using _open_osfhandle / _get_osfhandle
  // causes a crash.  Because uv_fs_* functions do just that, they cannot be
  // used here.
  result = (int) file;
end:
  SET_REQ_RESULT(req, result);
}

void com__close(uv_fs_t* req, uv_file file) {
  HANDLE handle;
  int result;

  VERIFY_UV_FILE(file, req);

  handle = (HANDLE) file;
  if (CloseHandle(handle)) {
    SET_REQ_RESULT(req, 0);
  } else {
    SET_REQ_ERROR(req, GetLastError());
  }
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

void com__write(uv_fs_t* req, uv_file file, void *buf, size_t length,
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

  if (WriteFile(handle, buf, length, &bytes, overlapped_ptr)) {
    SET_REQ_RESULT(req, bytes);
  } else {
    SET_REQ_ERROR(req, GetLastError());
  }
}


static DWORD WINAPI uv_com_thread_proc(void* parameter) {
  uv_fs_t* req = (uv_fs_t*) parameter;
  uv_loop_t* loop = req->loop;
  
  assert(req != NULL);
  assert(req->type == UV_FS);

  switch (req->fs_type) {
    case UV_FS_OPEN:
      com__open(req, req->pathw, (int)req->arg0, (int)req->arg1);
      break;
    case UV_FS_CLOSE:
      com__close(req, (uv_file)req->arg0);
      break;
    case UV_FS_READ:
      com__read(req,
               (uv_file) req->arg0,
               req->arg1,
               (size_t) req->arg2,
               (off_t) req->arg3);
      break;
    case UV_FS_WRITE:
      com__write(req,
                (uv_file)req->arg0,
                req->arg1,
                (size_t) req->arg2,
                (off_t) req->arg3);
      break;
    default:
      assert(!"bad uv_fs_type");
  }

  POST_COMPLETION_FOR_REQ(loop, req);

  return 0;
}

int uv_com_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags,
    int mode, uv_fs_cb cb) {
  wchar_t* pathw;
  int size;

  /* Convert to UTF16. */
  UTF8_TO_UTF16(path, pathw);

  if (cb) {
    uv_com_req_init_async(loop, req, UV_FS_OPEN, path, pathw, cb);
    WRAP_REQ_ARGS2(req, flags, mode);
    QUEUE_FS_TP_JOB(loop, req);
  } else {
    uv_com_req_init_sync(loop, req, UV_FS_OPEN);
    com__open(req, pathw, flags, mode);
    free(pathw);
    SET_UV_LAST_ERROR_FROM_REQ(req);
    return req->result;
  }

  return 0;
}

int uv_com_close(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb) {
  if (cb) {
    uv_com_req_init_async(loop, req, UV_FS_CLOSE, NULL, NULL, cb);
    WRAP_REQ_ARGS1(req, file);
    QUEUE_FS_TP_JOB(loop, req);
  } else {
    uv_com_req_init_sync(loop, req, UV_FS_CLOSE);
    com__close(req, file);
    SET_UV_LAST_ERROR_FROM_REQ(req);
    return req->result;
  }

  return 0;
}

int uv_com_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf,
    size_t length, off_t offset, uv_fs_cb cb) {
  if (cb) {
    uv_com_req_init_async(loop, req, UV_FS_READ, NULL, NULL, cb);
    WRAP_REQ_ARGS4(req, file, buf, length, offset);
    QUEUE_FS_TP_JOB(loop, req);
  } else {
    uv_com_req_init_sync(loop, req, UV_FS_READ);
    com__read(req, file, buf, length, offset);
    SET_UV_LAST_ERROR_FROM_REQ(req);
    return req->result;
  }

  return 0;
}

int uv_com_write(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf,
    size_t length, off_t offset, uv_fs_cb cb) {
  if (cb) {
    uv_com_req_init_async(loop, req, UV_FS_WRITE, NULL, NULL, cb);
    WRAP_REQ_ARGS4(req, file, buf, length, offset);
    QUEUE_FS_TP_JOB(loop, req);
  } else {
    uv_com_req_init_sync(loop, req, UV_FS_WRITE);
    com__write(req, file, buf, length, offset);
    SET_UV_LAST_ERROR_FROM_REQ(req);
    return req->result;
  }

  return 0;
}
