#include "kernel/cpu.h"
#include "kernel/printf.h"


#define STB_SPRINTF_IMPLEMENTATION

#include "lib/stb_sprintf.h"

static kprint_callback_t active_kprint_sink;

void set_output_sink(kprint_callback_t new_sink) {
    active_kprint_sink = new_sink;
}

void vprintf(const char* format, va_list args) {
    if (!active_kprint_sink) return;

    char tmp[STB_SPRINTF_MIN];
    
    cpu_status_t flags;
    irq_save(flags);

    stbsp_vsprintfcb(active_kprint_sink, NULL, tmp, format, args);

    irq_restore(flags);
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void dprintf(const char* format, ...) { // TODO: Move to VFS
    va_list args;
    va_start(args, format);
    vprintf(format, args); // just doing this for now...
    va_end(args);
}

int snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = stbsp_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return result;
}

int vsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
    return stbsp_vsnprintf(buf, size, fmt, args);
}