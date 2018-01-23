#include <string.h>

extern "C"
{
    _declspec(dllexport) void hello(char* str, int size)
    {
        strncpy(str, "hello world!", size);
    }
}
