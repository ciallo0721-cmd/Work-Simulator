/*
 * emscripten_compat.h - Emscripten兼容层
 * 只补充 conio.h 中 Emscripten 缺失的部分
 */
#ifndef EMSCRIPTEN_COMPAT_H
#define EMSCRIPTEN_COMPAT_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Emscripten 没有 conio.h，用 getchar 替代 _getch
#define _getch() getchar()
#define _kbhit() 0

// system("pause") 替代
#define PAUSE() do { \
    printf("\n\033[32m按 Enter 继续...\033[0m"); \
    while(getchar() != '\n'); \
    printf("\n"); \
} while(0)

#endif // __EMSCRIPTEN__

#endif // EMSCRIPTEN_COMPAT_H
