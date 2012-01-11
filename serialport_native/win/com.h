#ifndef UV_SERIALPORT_WIN_COM_
#define UV_SERIALPORT_WIN_COM_

#include <uv.h>

extern "C" {

int uv_com_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags,
    int mode, uv_fs_cb cb);

int uv_com_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf,
    size_t length, off_t offset, uv_fs_cb cb);

}

#endif /* UV_SERIALPORT_WIN_COM_ */
