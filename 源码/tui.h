/*==============================================================================
 *  tui.h  —  坤城 TUI 引擎  v1.0
 *  打工模拟器 v5.0 的终端界面层
 *
 *  设计定位：比 CLI 高一级（有框架、面板、方向键导航、实时图形），
 *            比 GUI 轻（纯字符终端，零依赖，单文件头）。
 *
 *  能力：
 *    · 双缓冲 + 行级差异刷新 —— 不闪屏
 *    · UTF-8 / CJK 宽字符正确对齐
 *    · 256 色 ANSI（Win10 conhost / Windows Terminal / VSCode 终端均支持）
 *    · 阻塞 / 非阻塞按键，方向键 Home/End PgUp/PgDn
 *    · 折线图、迷你走势图、进度条、面板边框
 *    · printf 流式捕获 —— 老代码零改动即可渲染进 TUI 面板
 *
 *  注意：本头文件会在末尾 #define printf tui_printf，
 *        必须放在所有系统头文件之后 include。
 *============================================================================*/
#ifndef TUI_ENGINE_H
#define TUI_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#  include <windows.h>
#  include <conio.h>
#else
#  include <unistd.h>
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <fcntl.h>
#endif

/*==============================================================================
 *  调色板（ANSI 256 色索引）
 *============================================================================*/
#define TC_BG        234   /* 主背景    深灰 */
#define TC_BG2       236   /* 面板底 */
#define TC_BG3       238   /* 高亮底 */
#define TC_BGHI      240   /* 选中底 */
#define TC_BORDER    240   /* 边框 */
#define TC_BORDER_HI 45    /* 高亮边框 */
#define TC_TEXT      252   /* 正文 */
#define TC_DIM       245   /* 次要文字 */
#define TC_FAINT     241   /* 极弱 */
#define TC_CYAN      51
#define TC_CYAND     37
#define TC_YELLOW    220
#define TC_YELLOWD   178
#define TC_GREEN     46
#define TC_GREEND    35
#define TC_RED       203
#define TC_REDD      131
#define TC_MAGENTA   213
#define TC_BLUE      75
#define TC_ORANGE    208
#define TC_PINK      219
#define TC_WHITE     255
#define TC_BLACK     232

/*==============================================================================
 *  画布
 *============================================================================*/
typedef struct {
    char          ch[5];   /* UTF-8 序列（NUL 结尾） */
    unsigned char w;       /* 显示宽度：0=被宽字符占位，1=半角，2=全角 */
    unsigned char fg;
    unsigned char bg;
} TuiCell;

typedef struct {
    TuiCell* c;
    int      w, h;
} TuiCanvas;

static TuiCanvas tui_cv;                       /* 后缓冲（本帧） */
static TuiCell*  tui_shadow = NULL;            /* 前缓冲（上帧） */
static int       tui_sw = 0, tui_sh = 0;
static int       tui_inited = 0;
static int       tui_dirty_all = 1;

/*==============================================================================
 *  UTF-8 / 宽度
 *============================================================================*/
static int tui_u8len(unsigned char c)
{
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static unsigned int tui_u8dec(const char* s, int* used)
{
    const unsigned char* p = (const unsigned char*)s;
    if (p[0] < 0x80)            { *used = 1; return p[0]; }
    if ((p[0] & 0xE0) == 0xC0)  { *used = 2; return ((p[0] & 0x1Fu) << 6)  | (p[1] & 0x3Fu); }
    if ((p[0] & 0xF0) == 0xE0)  { *used = 3; return ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu); }
    if ((p[0] & 0xF8) == 0xF0)  { *used = 4; return ((p[0] & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu); }
    *used = 1; return '?';
}

/* 字符显示宽度（0=组合符，2=东亚全角/emoji，1=其他） */
static int tui_cpwidth(unsigned int cp)
{
    if (cp == 0) return 0;
    if (cp < 32) return 0;
    if (cp < 127) return 1;
    /* 组合符号 */
    if (cp >= 0x0300 && cp <= 0x036F) return 0;
    if (cp >= 0x200B && cp <= 0x200F) return 0;
    /* CJK 及全角 */
    if (cp >= 0x1100 && cp <= 0x115F) return 2;
    if (cp >= 0x2E80 && cp <= 0x303E) return 2;
    if (cp >= 0x3041 && cp <= 0x33FF) return 2;
    if (cp >= 0x3400 && cp <= 0x4DBF) return 2;
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;
    if (cp >= 0xA000 && cp <= 0xA4CF) return 2;
    if (cp >= 0xA960 && cp <= 0xA97F) return 2;
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 2;
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;
    if (cp >= 0xFE10 && cp <= 0xFE19) return 2;
    if (cp >= 0xFE30 && cp <= 0xFE6F) return 2;
    if (cp >= 0xFF00 && cp <= 0xFF60) return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;
    if (cp >= 0x1F300 && cp <= 0x1F64F) return 2;
    if (cp >= 0x1F900 && cp <= 0x1F9FF) return 2;
    if (cp >= 0x20000 && cp <= 0x3FFFD) return 2;
    /* 常用制表符/块元素/几何图形是半角 */
    if (cp >= 0x2500 && cp <= 0x25FF) return 1;
    if (cp >= 0x2000 && cp <= 0x206F) return 1;
    return 1;
}

/* 字符串显示宽度 */
static int tui_strwidth(const char* s)
{
    int w = 0;
    while (*s) { int u; unsigned int cp = tui_u8dec(s, &u); w += tui_cpwidth(cp); s += u; }
    return w;
}

/* 单个 UTF-8 字符的显示宽度 */
static int tui_glyph_w(const char* u8)
{
    int used = 0;
    unsigned int cp = tui_u8dec(u8, &used);
    return tui_cpwidth(cp);
}

/*==============================================================================
 *  平台层
 *============================================================================*/
static void tui_sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static void tui_getsize(int* w, int* h)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bi)) {
        *w = bi.srWindow.Right - bi.srWindow.Left + 1;
        *h = bi.srWindow.Bottom - bi.srWindow.Top + 1;
        return;
    }
    *w = 100; *h = 40;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        *w = ws.ws_col; *h = ws.ws_row;
    } else { *w = 100; *h = 40; }
#endif
}

static void tui_resize_canvas(int w, int h)
{
    int i;
    if (w < 40) w = 40;
    if (h < 16) h = 16;

    free(tui_cv.c);
    tui_cv.c = (TuiCell*)calloc((size_t)w * h, sizeof(TuiCell));
    tui_cv.w = w; tui_cv.h = h;

    free(tui_shadow);
    tui_shadow = (TuiCell*)calloc((size_t)w * h, sizeof(TuiCell));
    tui_sw = w; tui_sh = h;

    for (i = 0; i < w * h; i++) {
        tui_cv.c[i].ch[0] = ' ';
        tui_cv.c[i].w = 1;
        tui_cv.c[i].fg = TC_TEXT;
        tui_cv.c[i].bg = TC_BG;
        tui_shadow[i] = tui_cv.c[i];
    }
    tui_dirty_all = 1;
}

static void tui_raw_on(void)
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode))
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    fputs("\033[?1049h", stdout);   /* 备用屏幕 */
    fputs("\033[?25l", stdout);     /* 隐藏光标 */
    fputs("\033[2J", stdout);
    fflush(stdout);
#else
    fputs("\033[?1049h\033[?25l\033[2J", stdout);
    fflush(stdout);
    { struct termios t; tcgetattr(0, &t); t.c_lflag &= ~(ICANON|ECHO); tcsetattr(0, TCSANOW, &t); }
#endif
}

static void tui_raw_off(void)
{
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
}

/*==============================================================================
 *  绘制原语
 *============================================================================*/
static void tc_clear(int fg, int bg)
{
    int i, n = tui_cv.w * tui_cv.h;
    for (i = 0; i < n; i++) {
        tui_cv.c[i].ch[0] = ' '; tui_cv.c[i].ch[1] = 0;
        tui_cv.c[i].w = 1; tui_cv.c[i].fg = (unsigned char)fg; tui_cv.c[i].bg = (unsigned char)bg;
    }
}

static void tc_fill(int x, int y, int w, int h, const char* ch, int fg, int bg)
{
    int i, j;
    if (w <= 0 || h <= 0) return;
    for (j = y; j < y + h; j++) {
        if (j < 0 || j >= tui_cv.h) continue;
        for (i = x; i < x + w; i++) {
            if (i < 0 || i >= tui_cv.w) continue;
            TuiCell* c = &tui_cv.c[j * tui_cv.w + i];
            strncpy(c->ch, ch, 3); c->ch[3] = 0;
            c->w = 1; c->fg = (unsigned char)fg; c->bg = (unsigned char)bg;
        }
    }
}

/* 写一个 UTF-8 字符（自动处理宽字符占位） */
static int tc_put(int x, int y, const char* u8, int fg, int bg)
{
    int used = 0;
    unsigned int cp = tui_u8dec(u8, &used);
    int cw = tui_cpwidth(cp);
    if (cw == 0) return 0;
    if (y < 0 || y >= tui_cv.h) return cw;
    if (x < 0 || x + cw > tui_cv.w) return cw;

    TuiCell* c = &tui_cv.c[y * tui_cv.w + x];
    memcpy(c->ch, u8, used); c->ch[used] = 0;
    c->w = (unsigned char)cw; c->fg = (unsigned char)fg; c->bg = (unsigned char)bg;
    if (cw == 2) {
        TuiCell* c2 = &tui_cv.c[y * tui_cv.w + x + 1];
        c2->ch[0] = 0; c2->w = 0; c2->fg = (unsigned char)fg; c2->bg = (unsigned char)bg;
    }
    return cw;
}

/* 写字符串，返回结束列 */
static int tc_puts(int x, int y, const char* s, int fg, int bg)
{
    while (*s) {
        int used = 0;
        tui_u8dec(s, &used);
        x += tc_put(x, y, s, fg, bg);
        s += used;
    }
    return x;
}

/* 写字符串，限制最大显示宽度 */
static int tc_putsn(int x, int y, int maxw, const char* s, int fg, int bg)
{
    int done = 0;
    while (*s && done < maxw) {
        int used = 0;
        unsigned int cp = tui_u8dec(s, &used);
        int cw = tui_cpwidth(cp);
        if (cw == 0) { s += used; continue; }
        if (done + cw > maxw) break;
        tc_put(x + done, y, s, fg, bg);
        done += cw;
        s += used;
    }
    return x + done;
}

/* 居中写 */
static void tc_puts_center(int x, int y, int w, const char* s, int fg, int bg)
{
    int sw = tui_strwidth(s);
    int px = x + (w - sw) / 2;
    if (px < x) px = x;
    tc_putsn(px, y, w, s, fg, bg);
}

/* 边框（单线 / 双线） */
static void tc_frame(int x, int y, int w, int h, int fg, int bg, int dbl)
{
    const char *tl, *tr, *bl, *br, *hz, *vt;
    if (dbl) { tl = "╔"; tr = "╗"; bl = "╚"; br = "╝"; hz = "═"; vt = "║"; }
    else     { tl = "┌"; tr = "┐"; bl = "└"; br = "┘"; hz = "─"; vt = "│"; }
    int i;
    if (w < 2 || h < 2) return;
    tc_put(x, y, tl, fg, bg);           tc_put(x + w - 1, y, tr, fg, bg);
    tc_put(x, y + h - 1, bl, fg, bg);   tc_put(x + w - 1, y + h - 1, br, fg, bg);
    for (i = 1; i < w - 1; i++) {
        tc_put(x + i, y, hz, fg, bg);
        tc_put(x + i, y + h - 1, hz, fg, bg);
    }
    for (i = 1; i < h - 1; i++) {
        tc_put(x, y + i, vt, fg, bg);
        tc_put(x + w - 1, y + i, vt, fg, bg);
    }
}

/* 带标题的面板 */
static void tc_panel(int x, int y, int w, int h, const char* title, int bfg, int bbg, int tfg)
{
    tc_fill(x, y, w, h, " ", TC_TEXT, TC_BG2);
    tc_frame(x, y, w, h, bfg, bbg, 0);
    if (title && *title) {
        char buf[128];
        snprintf(buf, sizeof buf, " %s ", title);
        tc_putsn(x + 2, y, w - 4, buf, tfg, bbg);
    }
}

/* 水平分隔线 */
static void tc_hline(int x, int y, int w, int fg, int bg)
{
    int i;
    for (i = 0; i < w; i++) tc_put(x + i, y, "─", fg, bg);
}

/*==============================================================================
 *  组件：进度条 / 迷你走势 / 折线图
 *============================================================================*/
/* 进度条 */
static void tc_bar(int x, int y, int w, int cur, int max, int lowfg, int midfg, int hifg, int bg)
{
    int filled, i;
    int fg = hifg;
    if (max <= 0) max = 1;
    if (cur < 0) cur = 0;
    if (cur > max) cur = max;
    filled = (int)((long long)cur * w / max);

    /* 颜色随比例变化 */
    int pct = (int)((long long)cur * 100 / max);
    if (pct < 25) fg = lowfg;
    else if (pct < 60) fg = midfg;
    else fg = hifg;

    for (i = 0; i < w; i++) {
        if (i < filled) tc_put(x + i, y, "█", fg, bg);
        else            tc_put(x + i, y, "░", TC_BG3, bg);
    }
}

/* 迷你走势（八级方块） */
static void tc_spark(int x, int y, int w, const int* d, int n, int base, int fg_up, int fg_dn, int bg)
{
    static const char* blocks[8] = { "▁","▂","▃","▄","▅","▆","▇","█" };
    int i, lo = d[0], hi = d[0], col;
    if (n <= 0 || w <= 0) return;
    for (i = 1; i < n; i++) { if (d[i] < lo) lo = d[i]; if (d[i] > hi) hi = d[i]; }
    if (hi == lo) { hi = lo + 1; }

    for (col = 0; col < w; col++) {
        int idx = (n <= w) ? (col < n ? col : n - 1)
                           : (int)((float)col / (w - 1) * (n - 1) + 0.5f);
        if (idx < 0) idx = 0;
        if (idx >= n) idx = n - 1;
        int lvl = (int)((float)(d[idx] - lo) / (hi - lo) * 7.0f + 0.5f);
        if (lvl < 0) lvl = 0; if (lvl > 7) lvl = 7;
        int fg = (d[idx] >= base) ? fg_up : fg_dn;
        tc_put(x + col, y, blocks[lvl], fg, bg);
    }
}

/* 折线图：带 Y 轴刻度、网格、现值高亮 —— 涨红跌绿（A 股习惯由调用方传入颜色） */
static void tc_linechart(int x, int y, int w, int h, const int* d, int n,
                         const char* title, int line_fg, int ylab_fg, int bg,
                         int show_last_mark)
{
    int i, col;
    int ax_x = x + 7;                     /* 绘图区左边界（留 y 轴标签） */
    int ax_w = w - 9;
    int ax_y = y + 1;
    int ax_h = h - 2;
    int lo, hi, prev_py = -1;

    if (w < 16 || h < 6 || n <= 0 || ax_w < 4 || ax_h < 3) return;

    lo = hi = d[0];
    for (i = 1; i < n; i++) { if (d[i] < lo) lo = d[i]; if (d[i] > hi) hi = d[i]; }
    if (hi == lo) { hi = lo + 2; lo = lo - 1; }
    else { int pad = (hi - lo) / 6 + 1; hi += pad; lo -= pad; if (lo < 0) lo = 0; }
    if (hi == lo) hi = lo + 1;

    /* 边框 */
    tc_frame(x, y, w, h, TC_BORDER, bg, 0);
    if (title && *title) {
        char buf[128];
        snprintf(buf, sizeof buf, " %s ", title);
        tc_putsn(x + 2, y, w - 4, buf, TC_YELLOW, bg);
    }

    /* 横向网格 + Y 轴刻度 */
    for (i = 0; i <= 3; i++) {
        int gy = ax_y + (ax_h - 1) * i / 3;
        int gval = hi - (hi - lo) * i / 3;
        char lab[16];
        for (col = ax_x; col < ax_x + ax_w; col++)
            if (col % 2 == 0) tc_put(col, gy, ".", TC_BG3 + 2, bg);
        snprintf(lab, sizeof lab, "%5d", gval);
        tc_putsn(x + 1, gy, 6, lab, ylab_fg, bg);
    }
    /* 网格竖线 */
    for (i = 1; i < 4; i++) {
        int gx = ax_x + ax_w * i / 4;
        for (col = ax_y; col < ax_y + ax_h; col++)
            if (col % 2 == 0) tc_put(gx, col, ".", TC_BG3 + 2, bg);
    }

    /* 折线：把 n 个点映射到 ax_w 列 */
    for (col = 0; col < ax_w; col++) {
        int idx = (n <= ax_w) ? (int)((float)col / (ax_w - 1) * (n - 1) + 0.5f)
                              : (int)((float)col / (ax_w - 1) * (n - 1) + 0.5f);
        if (idx < 0) idx = 0;
        if (idx >= n) idx = n - 1;
        int val = d[idx];
        int py = ax_y + ax_h - 1 - (int)((float)(val - lo) / (hi - lo) * (ax_h - 1) + 0.5f);
        if (py < ax_y) py = ax_y;
        if (py > ax_y + ax_h - 1) py = ax_y + ax_h - 1;

        int fg = line_fg;
        (void)fg;

        tc_put(ax_x + col, py, "*", fg, bg);
        /* 与前一列之间补竖线，形成连续折线 */
        if (prev_py >= 0 && prev_py != py) {
            int a = prev_py < py ? prev_py : py;
            int b = prev_py < py ? py : prev_py;
            int k;
            for (k = a + 1; k < b; k++) tc_put(ax_x + col, k, "|", TC_BG3 + 3, bg);
        }
        prev_py = py;
    }

    /* 现值标记 */
    if (show_last_mark && n > 0) {
        int val = d[n - 1];
        int py = ax_y + ax_h - 1 - (int)((float)(val - lo) / (hi - lo) * (ax_h - 1) + 0.5f);
        if (py >= ax_y && py < ax_y + ax_h)
            tc_put(ax_x + ax_w - 1, py, "#", TC_WHITE, bg);
    }
}

/*==============================================================================
 *  按键
 *============================================================================*/
#define TK_UP     1001
#define TK_DOWN   1002
#define TK_LEFT   1003
#define TK_RIGHT  1004
#define TK_HOME   1005
#define TK_END    1006
#define TK_PGUP   1007
#define TK_PGDN   1008
#define TK_DEL    1009
#define TK_INS    1010
#define TK_F1     1021

static int tui_kbhit(void)
{
#ifdef _WIN32
    return _kbhit();
#else
    struct termios old, nb; int ch, oldf;
    tcgetattr(0, &old); nb = old; nb.c_lflag &= ~ICANON; nb.c_cc[VMIN] = 0; nb.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &nb);
    oldf = fcntl(0, F_GETFL, 0); fcntl(0, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    fcntl(0, F_SETFL, oldf);
    tcsetattr(0, TCSANOW, &old);
    if (ch != EOF) { ungetc(ch, stdin); return 1; }
    return 0;
#endif
}

static int tui_readch(void)
{
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}

/* 按键回放（自检用） */
static const char* tui_autokeys = NULL;
static int tui_autokeys_pos = 0;

/* 非阻塞取键，无键返回 -1 */
static int tui_key_nb(void)
{
    int c;

    if (tui_autokeys) {
        char a = tui_autokeys[tui_autokeys_pos];
        if (!a) return 27;          /* 脚本用完按 Esc，让程序自然退出 */
        tui_autokeys_pos++;
        tui_sleep_ms(120);
        switch (a) {
            case 'U': return TK_UP;
            case 'D': return TK_DOWN;
            case 'L': return TK_LEFT;
            case 'R': return TK_RIGHT;
            case '~': return 13;
            default:  return (unsigned char)a;
        }
    }

    if (!tui_kbhit()) return -1;
    c = tui_readch();

#ifdef _WIN32
    if (c == 0 || c == 224) {
        int c2 = tui_readch();
        switch (c2) {
            case 72: return TK_UP;
            case 80: return TK_DOWN;
            case 75: return TK_LEFT;
            case 77: return TK_RIGHT;
            case 71: return TK_HOME;
            case 79: return TK_END;
            case 73: return TK_PGUP;
            case 81: return TK_PGDN;
            case 83: return TK_DEL;
            case 82: return TK_INS;
            case 59: return TK_F1;
            default: return -1;
        }
    }
#else
    if (c == 27) {
        /* ESC 序列：非阻塞尝试读后续 */
        if (!tui_kbhit()) return 27;
        c = tui_readch();
        if (c == '[') {
            if (!tui_kbhit()) return 27;
            c = tui_readch();
            switch (c) {
                case 'A': return TK_UP;
                case 'B': return TK_DOWN;
                case 'C': return TK_RIGHT;
                case 'D': return TK_LEFT;
                case 'H': return TK_HOME;
                case 'F': return TK_END;
                case '5': tui_readch(); return TK_PGUP;
                case '6': tui_readch(); return TK_PGDN;
                case '3': tui_readch(); return TK_DEL;
                default: return -1;
            }
        }
        return 27;
    }
#endif
    if (c == 13 || c == 10) return 13;
    if (c == 27) return 27;
    if (c == 8 || c == 127) return 8;
    if (c >= 'A' && c <= 'Z') return c + 32;   /* 统一小写 */
    return c;
}

/* 阻塞取键 */
static int tui_key(void)
{
    int k;
    for (;;) {
        k = tui_key_nb();
        if (k >= 0) return k;
        tui_sleep_ms(12);
    }
}

/*==============================================================================
 *  帧
 *============================================================================*/
static int tui_lock_size = 0;   /* 自检时锁死画布尺寸 */

static void tui_begin(void)
{
    int w, h;
    if (!tui_lock_size) {
        tui_getsize(&w, &h);
        if (!tui_inited || w != tui_cv.w || h != tui_cv.h) tui_resize_canvas(w, h);
    }
    tc_clear(TC_TEXT, TC_BG);
}

static void tui_present(void)
{
    static char* obuf = NULL;
    static int   ocap = 0;
    size_t need = (size_t)tui_cv.w * tui_cv.h * 48 + 8192;
    int x, y, o = 0, lastfg = -1, lastbg = -1;

    if ((size_t)ocap < need) {
        obuf = (char*)realloc(obuf, need);
        ocap = (int)need;
    }
    if (!obuf) return;

    o += sprintf(obuf + o, "\033[?25l");

    for (y = 0; y < tui_cv.h; y++) {
        int changed = tui_dirty_all;
        if (!changed) {
            for (x = 0; x < tui_cv.w; x++) {
                TuiCell* a = &tui_cv.c[y * tui_cv.w + x];
                TuiCell* b = &tui_shadow[y * tui_sw + x];
                if (a->fg != b->fg || a->bg != b->bg || a->w != b->w ||
                    a->ch[0] != b->ch[0] || a->ch[1] != b->ch[1] || a->ch[2] != b->ch[2]) {
                    changed = 1; break;
                }
            }
        }
        if (!changed) continue;

        o += sprintf(obuf + o, "\033[%d;1H", y + 1);
        lastfg = -1; lastbg = -1;
        for (x = 0; x < tui_cv.w; x++) {
            TuiCell* c = &tui_cv.c[y * tui_cv.w + x];
            if (c->w == 0) continue;
            if (c->fg != lastfg || c->bg != lastbg) {
                o += sprintf(obuf + o, "\033[38;5;%d;48;5;%dm", c->fg, c->bg);
                lastfg = c->fg; lastbg = c->bg;
            }
            if (c->ch[0] == 0) { obuf[o++] = ' '; }
            else {
                int l = (int)strlen(c->ch);
                memcpy(obuf + o, c->ch, l); o += l;
            }
        }
    }
    o += sprintf(obuf + o, "\033[0m");

    fwrite(obuf, 1, o, stdout);
    fflush(stdout);

    if (tui_shadow && tui_sw == tui_cv.w && tui_sh == tui_cv.h) {
        memcpy(tui_shadow, tui_cv.c, sizeof(TuiCell) * tui_cv.w * tui_cv.h);
    }
    tui_dirty_all = 0;
}

#ifdef _WIN32
static BOOL WINAPI tui_ctrl_handler(DWORD type)
{
    (void)type;
    /* Ctrl+C / 关窗口时把终端从备用屏幕里救出来 */
    fputs("[0m[?25h[?1049l", stdout);
    fflush(stdout);
    return FALSE;
}
#endif

static void tui_init(void)
{
    int w, h;
#ifdef _WIN32
    SetConsoleCtrlHandler(tui_ctrl_handler, TRUE);
#endif
    tui_raw_on();
    tui_getsize(&w, &h);
    tui_resize_canvas(w, h);
    tui_inited = 1;
}

static void tui_shutdown(void)
{
    if (!tui_inited) return;
    tui_raw_off();
    free(tui_cv.c); tui_cv.c = NULL;
    free(tui_shadow); tui_shadow = NULL;
    tui_inited = 0;
}

/*==============================================================================
 *  printf 流式捕获
 *  —— 让老的 printf 页面自动渲染进 TUI 面板
 *============================================================================*/
#define TUI_STREAM_LINES 400
#define TUI_STREAM_COLS  200

typedef struct { char u8[5]; unsigned char fg; } TuiGlyph;
typedef struct { TuiGlyph g[TUI_STREAM_COLS]; int len; } TuiSLine;

static TuiSLine  tui_sl[TUI_STREAM_LINES];
static int       tui_sl_n   = 0;      /* 已完成的逻辑行数 */
static int       tui_sl_cur = 0;      /* 当前行字符数 */
static int       tui_s_fg   = TC_TEXT;
static int       tui_s_bold = 0;
static int       tui_stream_on = 1;
static int       tui_s_off   = 0;     /* 显示滚动偏移：0=贴底 */
static char      tui_s_title[128] = "";

static void tui_stream_reset(void)
{
    int i;
    for (i = 0; i < TUI_STREAM_LINES; i++) tui_sl[i].len = 0;
    tui_sl_n = 0; tui_sl_cur = 0; tui_s_fg = TC_TEXT; tui_s_bold = 0;
    tui_s_off = 0;
}

static void tui_stream_set_title(const char* t)
{
    if (!t) return;
    strncpy(tui_s_title, t, sizeof tui_s_title - 1);
    tui_s_title[sizeof tui_s_title - 1] = 0;
}

static void tui_stream_newline(void)
{
    tui_sl[tui_sl_n % TUI_STREAM_LINES].len = tui_sl_cur;
    tui_sl_n++;
    if (tui_sl_n > TUI_STREAM_LINES) tui_sl_n = TUI_STREAM_LINES;  /* 环形：保底 */
    tui_sl_cur = 0;
    if (tui_sl_n >= TUI_STREAM_LINES) {
        /* 满了就整体上移一行 */
        int i;
        for (i = 0; i < TUI_STREAM_LINES - 1; i++) tui_sl[i] = tui_sl[i + 1];
        tui_sl_n = TUI_STREAM_LINES - 1;
    }
}

static void tui_stream_putchar(const char* u8, int len)
{
    if (tui_sl_cur >= TUI_STREAM_COLS) tui_stream_newline();
    TuiSLine* L = &tui_sl[tui_sl_n];
    memcpy(L->g[tui_sl_cur].u8, u8, len);
    L->g[tui_sl_cur].u8[len] = 0;
    L->g[tui_sl_cur].fg = (unsigned char)tui_s_fg;
    tui_sl_cur++;
}

/* 解析 SGR 参数串 */
static void tui_sgr_apply(const char* p, int len)
{
    int vals[16], nv = 0, i = 0;
    char tmp[128];
    if (len >= (int)sizeof tmp) len = sizeof tmp - 1;
    memcpy(tmp, p, len); tmp[len] = 0;

    while (i < len && nv < 16) {
        int v = 0, any = 0;
        while (i < len && tmp[i] >= '0' && tmp[i] <= '9') { v = v * 10 + (tmp[i] - '0'); i++; any = 1; }
        vals[nv++] = any ? v : 0;
        if (i < len && tmp[i] == ';') i++;
        else break;
    }
    if (nv == 0) { tui_s_fg = TC_TEXT; tui_s_bold = 0; return; }

    for (i = 0; i < nv; i++) {
        int v = vals[i];
        if (v == 0) { tui_s_fg = TC_TEXT; tui_s_bold = 0; }
        else if (v == 1) { tui_s_bold = 1; }
        else if (v == 22) { tui_s_bold = 0; }
        else if (v == 39) { tui_s_fg = TC_TEXT; }
        else if (v >= 30 && v <= 37) {
            static const int m[8] = { TC_BLACK, TC_RED, TC_GREEN, TC_YELLOW, TC_BLUE, TC_MAGENTA, TC_CYAN, TC_TEXT };
            tui_s_fg = m[v - 30];
            if (tui_s_bold) tui_s_fg = (v == 30) ? TC_DIM : (m[v - 30] + 0);
        }
        else if (v >= 90 && v <= 97) {
            static const int m[8] = { TC_DIM, TC_RED, TC_GREEN, TC_YELLOW, TC_BLUE, TC_MAGENTA, TC_CYAN, TC_WHITE };
            tui_s_fg = m[v - 90];
        }
        else if (v == 38 || v == 48) {
            /* 38;5;n 或 48;5;n */
            if (i + 2 < nv && vals[i + 1] == 5) {
                if (v == 38) tui_s_fg = vals[i + 2];
                i += 2;
            } else if (i + 4 < nv && vals[i + 1] == 2) {
                /* 24bit → 粗略映射到最近的基础色 */
                int r = vals[i + 2], g = vals[i + 3], b = vals[i + 4];
                int lum = (r * 299 + g * 587 + b * 114) / 1000;
                if (v == 38) tui_s_fg = lum > 128 ? TC_TEXT : TC_DIM;
                i += 4;
            }
        }
    }
}

static void tui_stream_feed(const char* s, int n)
{
    int i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        if (c == 27 && i + 1 < n && s[i + 1] == '[') {
            int j = i + 2;
            while (j < n && !((s[j] >= '@' && s[j] <= '~'))) j++;
            if (j < n) {
                if (s[j] == 'm') tui_sgr_apply(s + i + 2, j - i - 2);
                i = j + 1;
            } else i = n;
            continue;
        }
        if (c == '\n') { tui_stream_newline(); i++; continue; }
        if (c == '\r') { i++; continue; }
        if (c == '\t') {
            int k;
            for (k = 0; k < 4; k++) tui_stream_putchar(" ", 1);
            i++; continue;
        }
        {
            int l = tui_u8len(c);
            if (i + l > n) l = 1;
            tui_stream_putchar(s + i, l);
            i += l;
        }
    }
}

/* 真正输出（不捕获） */
static void tui_emit(const char* s, int n)
{
    fwrite(s, 1, n, stdout);
}

static int tui_printf(const char* fmt, ...)
{
    char buf[8192];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return n;
    if (n > (int)sizeof buf - 1) n = (int)sizeof buf - 1;

    if (!tui_stream_on || !tui_inited) { tui_emit(buf, n); return n; }
    tui_stream_feed(buf, n);
    return n;
}

/* 把流内容画到矩形区域，返回总显示行数 */
static int tui_stream_flush(int x, int y, int w, int h, int scrollable)
{
    int i, total = 0, off, cur = 0, drawn = 0;
    int line_rows[TUI_STREAM_LINES];

    if (w <= 0 || h <= 0) return 0;

    /* 第一遍：按显示宽度折行，统计总行数 */
    for (i = 0; i < tui_sl_n; i++) {
        int k, cw = 0;
        for (k = 0; k < tui_sl[i].len; k++) cw += tui_glyph_w(tui_sl[i].g[k].u8);
        line_rows[i] = (tui_sl[i].len == 0) ? 1 : (cw + w - 1) / w;
        if (line_rows[i] < 1) line_rows[i] = 1;
        total += line_rows[i];
    }

    if (scrollable) {
        if (tui_s_off < 0) tui_s_off = 0;
        if (tui_s_off > total) tui_s_off = total;
    } else {
        tui_s_off = 0;
    }
    off = total - h - tui_s_off;
    if (off < 0) off = 0;

    /* 第二遍：绘制 */
    for (i = 0; i < tui_sl_n; i++) {
        TuiSLine* L = &tui_sl[i];
        int r;
        for (r = 0; r < line_rows[i]; r++) {
            if (cur >= off && cur - off < h) {
                int dy = y + (cur - off);
                int col = 0, k, acc = 0;
                for (k = 0; k < L->len; k++) {
                    int cw = tui_glyph_w(L->g[k].u8);
                    if (cw == 0) continue;
                    if (acc < r * w) { acc += cw; continue; }
                    if (acc + cw > (r + 1) * w) break;
                    tc_put(x + col, dy, L->g[k].u8, L->g[k].fg, TC_BG2);
                    col += cw;
                    acc += cw;
                }
                drawn++;
            }
            cur++;
            if (drawn >= h) return total;
        }
    }
    return total;
}

/*==============================================================================
 *  输入行（底部）
 *============================================================================*/
static int tui_input_line(int y, int w, const char* label, char* out, int maxlen, int digits_only)
{
    char buf[128];
    int len = 0, k;
    strncpy(buf, out ? out : "", sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    len = (int)strlen(buf);

    for (;;) {
        tc_fill(0, y, w, 1, " ", TC_TEXT, TC_BG3);
        {
            int x = tc_puts(1, y, label, TC_YELLOW, TC_BG3);
            x = tc_puts(x + 1, y, buf, TC_WHITE, TC_BG3);
            tc_put(x, y, "|", TC_CYAN, TC_BG3);
        }
        tui_present();

        k = tui_key();
        if (k == 13) break;
        if (k == 27) { if (out) out[0] = 0; return -1; }
        if (k == 8) { if (len > 0) buf[--len] = 0; continue; }
        if (k >= 32 && k < 127) {
            if (digits_only && !(k >= '0' && k <= '9')) continue;
            if (len < maxlen && len < (int)sizeof buf - 1) { buf[len++] = (char)k; buf[len] = 0; }
        }
    }
    if (out) { strncpy(out, buf, maxlen); out[maxlen] = 0; }
    return len;
}

/* 整数输入，返回 0 成功，-1 取消 */
static int tui_input_int(int y, int w, const char* label, int min, int max, int* out)
{
    char buf[32];
    for (;;) {
        buf[0] = 0;
        if (tui_input_line(y, w, label, buf, 10, 1) < 0) return -1;
        if (buf[0] == 0) return -1;
        int v = atoi(buf);
        if (v < min || v > max) {
            tc_fill(0, y, w, 1, " ", TC_RED, TC_BG3);
            char msg[128];
            snprintf(msg, sizeof msg, " 范围 %d~%d，重新输入", min, max);
            tc_puts(1, y, msg, TC_RED, TC_BG3);
            tui_present();
            tui_sleep_ms(700);
            continue;
        }
        *out = v;
        return 0;
    }
}

/* 任意键 */
static void tui_pause(int y, int w, const char* msg)
{
    if (!msg) msg = "按任意键继续";
    tc_fill(0, y, w, 1, " ", TC_DIM, TC_BG3);
    char buf[160];
    snprintf(buf, sizeof buf, "  %s ···", msg);
    tc_puts(1, y, buf, TC_GREEN, TC_BG3);
    tui_present();
    tui_key();
}

/* --- 把工程里所有 printf 重定向进 TUI 文本流 ---
 * 这样二十多个老的 printf 页面零改动就能渲染进内容面板，
 * 而不是直接在备用屏幕外面乱刷。
 */
#define printf tui_printf

#endif /* TUI_ENGINE_H */
