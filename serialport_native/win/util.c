#include <uv.h>

int uv_serialport_utf8_to_utf16(const char* utf8Buffer, wchar_t* utf16Buffer,
    size_t utf16Size) {
  return MultiByteToWideChar(CP_UTF8,
                             0,
                             utf8Buffer,
                             -1,
                             utf16Buffer,
                             utf16Size);
}
