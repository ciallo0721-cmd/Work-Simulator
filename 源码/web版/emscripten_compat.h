/*
 * Emscripten 兼容层
 * 替代 Windows conio.h / windows.h 中的功能
 */

#ifndef EMSCRIPTEN_COMPAT_H
#define EMSCRIPTEN_COMPAT_H

#include <emscripten.h>

// Sleep for ms in async mode (requires -s ASYNCIFY)
static inline void SleepCompat(int ms) {
    emscripten_sleep(ms);
}

#endif // EMSCRIPTEN_COMPAT_H
