#include <uv.h>

uv_err_code uv_serialport_translate_sys_error(int sys_errno) {
  switch (sys_errno) {
    case ERROR_SUCCESS:                     return UV_OK;
    case ERROR_FILE_NOT_FOUND:              return UV_ENOENT;
    case ERROR_PATH_NOT_FOUND:              return UV_ENOENT;
    case ERROR_ACCESS_DENIED:               return UV_EACCES;
    case ERROR_NOACCESS:                    return UV_EACCES;
    case WSAEACCES:                         return UV_EACCES;
    case ERROR_ADDRESS_ALREADY_ASSOCIATED:  return UV_EADDRINUSE;
    case WSAEADDRINUSE:                     return UV_EADDRINUSE;
    case WSAEADDRNOTAVAIL:                  return UV_EADDRNOTAVAIL;
    case WSAEAFNOSUPPORT:                   return UV_EAFNOSUPPORT;
    case WSAEWOULDBLOCK:                    return UV_EAGAIN;
    case WSAEALREADY:                       return UV_EALREADY;
    case ERROR_CONNECTION_ABORTED:          return UV_ECONNABORTED;
    case WSAECONNABORTED:                   return UV_ECONNABORTED;
    case ERROR_CONNECTION_REFUSED:          return UV_ECONNREFUSED;
    case WSAECONNREFUSED:                   return UV_ECONNREFUSED;
    case ERROR_NETNAME_DELETED:             return UV_ECONNRESET;
    case WSAECONNRESET:                     return UV_ECONNRESET;
    case WSAEFAULT:                         return UV_EFAULT;
    case ERROR_HOST_UNREACHABLE:            return UV_EHOSTUNREACH;
    case WSAEHOSTUNREACH:                   return UV_EHOSTUNREACH;
    case ERROR_INVALID_DATA:                return UV_EINVAL;
    case WSAEINVAL:                         return UV_EINVAL;
    case ERROR_TOO_MANY_OPEN_FILES:         return UV_EMFILE;
    case WSAEMFILE:                         return UV_EMFILE;
    case WSAEMSGSIZE:                       return UV_EMSGSIZE;
    case ERROR_NETWORK_UNREACHABLE:         return UV_ENETUNREACH;
    case WSAENETUNREACH:                    return UV_ENETUNREACH;
    case ERROR_OUTOFMEMORY:                 return UV_ENOMEM;
    case ERROR_NOT_CONNECTED:               return UV_ENOTCONN;
    case WSAENOTCONN:                       return UV_ENOTCONN;
    case ERROR_NOT_SUPPORTED:               return UV_ENOTSUP;
    case ERROR_INSUFFICIENT_BUFFER:         return UV_EINVAL;
    case ERROR_INVALID_FLAGS:               return UV_EBADF;
    case ERROR_INVALID_PARAMETER:           return UV_EINVAL;
    case ERROR_NO_UNICODE_TRANSLATION:      return UV_ECHARSET;
    case ERROR_BROKEN_PIPE:                 return UV_EOF;
    case ERROR_PIPE_BUSY:                   return UV_EBUSY;
    case ERROR_SEM_TIMEOUT:                 return UV_ETIMEDOUT;
    case ERROR_ALREADY_EXISTS:              return UV_EEXIST;
    case WSAHOST_NOT_FOUND:                 return UV_ENOENT;
    default:                                return UV_UNKNOWN;
  }
}
