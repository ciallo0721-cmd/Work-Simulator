"""
Convert 打工模拟器.cpp to WebAssembly-compatible work_web.cpp
Performs surgical text replacements on the main file.
"""
import re

def read_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(path, content):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def find_fn_range(lines, fn_name, start_search=0):
    """Find function definition range [start_line, end_line) by name."""
    for i in range(start_search, len(lines)):
        if fn_name in lines[i] and lines[i].strip().startswith('void ' + fn_name):
            # Find opening brace
            brace_count = 0
            start = i
            if '{' not in lines[i]:
                i += 1
                while i < len(lines) and '{' not in lines[i]:
                    i += 1
            if i < len(lines):
                brace_count = lines[i].count('{') - lines[i].count('}')
            else:
                return None, None
            end = i + 1
            while end < len(lines) and brace_count > 0:
                brace_count += lines[end].count('{') - lines[end].count('}')
                end += 1
            return start, end
    return None, None

def main():
    src = r"G:\EmoScan Pro\Work-Simulator\源码\打工模拟器.cpp"
    dst = r"G:\EmoScan Pro\Work-Simulator\源码\web版\work_web.cpp"
    
    content = read_file(src)
    lines = content.split('\n')
    print(f"Read {len(lines)} lines from source")
    
    # ===== CHANGE 1: Header section =====
    # Replace the #ifdef _WIN32/#else headers with 3-way #ifdef __EMSCRIPTEN__/#elif _WIN32/#else
    old_hdr = ''' #ifdef _WIN32
 #include <conio.h>
 #include <windows.h>
 #include <tlhelp32.h>
 #else
 #include <unistd.h>
 #define _getch() getchar()
 #endif'''
    new_hdr = ''' #ifdef __EMSCRIPTEN__
 #include "emscripten_compat.h"
 #include <unistd.h>
 #elif defined(_WIN32)
 #include <conio.h>
 #include <windows.h>
 #include <tlhelp32.h>
 #else
 #include <unistd.h>
 #define _getch() getchar()
 #endif'''
    content = content.replace(old_hdr, new_hdr)
    
    # ===== CHANGE 2: Title/comment =====
    content = content.replace(
        ' * 编译环境: Windows/Linux, C11/C++兼容\n * 编译命令: gcc -o worker_sim worker_simulator.c -lm',
        ' * 编译环境: Emscripten / Windows / Linux, C11/C++兼容\n * 编译命令: emcc -o worker_sim.js work_web.cpp -lm -s WASM=1 -s FORCE_FILESYSTEM=1 -O2'
    )
    
    # ===== CHANGE 3: SAVE_DIR removal, FORBES_FILE =====
    content = content.replace(
        '#define SAVE_FILE "worker_sim_save.dat"\n#define SAVE_DIR "save/"\n#define SAVE_VERSION 8  // v4.1 文件夹结构存档',
        '#define SAVE_FILE "worker_sim_save.dat"\n#define SAVE_VERSION 8  // v4.1 Web版单文件存档'
    )
    content = content.replace(
        '#define FORBES_FILE "save/forbes.list"',
        '#define FORBES_FILE "forbes_ranking.dat"'
    )
    
    # ===== CHANGE 4: Replace ensure_dir with empty stub =====
    # Find and remove the ensure_dir function entirely
    lines = content.split('\n')
    new_lines = []
    i = 0
    in_ensure_dir = False
    brace_depth = 0
    
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        if stripped == 'void ensure_dir(const char* path) {' or stripped.startswith('void ensure_dir('):
            in_ensure_dir = True
            brace_depth = line.count('{') - line.count('}')
            i += 1
            continue
        
        if in_ensure_dir:
            brace_depth += line.count('{') - line.count('}')
            if brace_depth <= 0:
                in_ensure_dir = False
            i += 1
            continue
        
        new_lines.append(line)
        i += 1
    
    content = '\n'.join(new_lines)
    lines = content.split('\n')
    
    # ===== CHANGE 5: Replace save_game() with single-file version =====
    save_start, save_end = find_fn_range(lines, 'save_game')
    if save_start:
        simple_save = [
            'void save_game() {',
            '    FILE* f = fopen(SAVE_FILE, "wb");',
            '    if (!f) {',
            '        print_colored(COLOR_RED, "\\n? 保存存档失败！\\n");',
            '        return;',
            '    }',
            '',
            '    int v = SAVE_VERSION;',
            '    fwrite(&v, sizeof(int), 1, f);',
            '    fwrite(&player, sizeof(Player), 1, f);',
            '    fwrite(tasks, sizeof(Task), MAX_TASKS, f);',
            '    fwrite(market, sizeof(MarketItem), MAX_MARKET, f);',
            '',
            '    fclose(f);',
            '',
            '    // ——— forbes.list ———',
            '    update_forbes_ranking();',
            '',
            '    print_colored(COLOR_GREEN, "\\n?? 游戏存档已保存！\\n");',
            '}'
        ]
        lines[save_start:save_end] = simple_save
        content = '\n'.join(lines)
        lines = content.split('\n')
    
    # ===== CHANGE 6: Remove migrate_old_save() =====
    new_lines = []
    i = 0
    in_migrate = False
    brace_depth = 0
    
    while i < len(lines):
        line = lines[i]
        if 'void migrate_old_save()' in line and line.strip().startswith('void migrate_old_save'):
            in_migrate = True
            brace_depth = line.count('{') - line.count('}')
            i += 1
            while i < len(lines) and brace_depth > 0:
                brace_depth += lines[i].count('{') - lines[i].count('}')
                i += 1
            continue
        if not in_migrate:
            new_lines.append(line)
        i += 1
    
    content = '\n'.join(new_lines)
    lines = content.split('\n')
    
    # ===== CHANGE 7: Replace load_game() with single-file version =====
    load_start, load_end = find_fn_range(lines, 'load_game')
    if load_start:
        simple_load = [
            'void load_game() {',
            '    // 尝试加载单文件存档',
            '    FILE* f = fopen(SAVE_FILE, "rb");',
            '    if (!f) {',
            '        print_colored(COLOR_YELLOW, "\\n?? 未找到存档，开始新游戏...\\n");',
            '        return;',
            '    }',
            '',
            '    // 验证存档版本',
            '    int file_version;',
            '    fread(&file_version, sizeof(int), 1, f);',
            '    if (file_version != SAVE_VERSION) {',
            '        print_colored(COLOR_YELLOW, "\\n?? 存档版本不兼容，将开始新游戏...\\n");',
            '        fclose(f);',
            '        return;',
            '    }',
            '',
            '    fread(&player, sizeof(Player), 1, f);',
            '    fread(tasks, sizeof(Task), MAX_TASKS, f);',
            '    fread(market, sizeof(MarketItem), MAX_MARKET, f);',
            '',
            '    fclose(f);',
            '',
            '   // v4.1 存档安全性检查：清除过期/损坏的事件数据',
            '   if (player.active_event != EVENT_NONE) {',
            '       time_t now = time(NULL);',
            '       // 如果 event_end_time 为0或者已过期，清除事件状态',
            '       if (player.event_end_time <= 0 || player.event_end_time <= now) {',
            '           print_colored(COLOR_YELLOW, "\\n?? 检测到过期事件（%s），已自动清除。\\n", get_event_name(player.active_event));',
            '           player.active_event = EVENT_NONE;',
            '           player.event_end_time = 0;',
            '       }',
            '   }',
            '',
            '    print_colored(COLOR_GREEN, "\\n?? 游戏存档已加载！\\n");',
            '}'
        ]
        lines[load_start:load_end] = simple_load
        content = '\n'.join(lines)
        lines = content.split('\n')
    
    # ===== CHANGE 8, 9: get_valid_input and get_valid_input_with_cancel - add clearerr =====
    # For get_valid_input: find the implementation
    gvi_start, gvi_end = find_fn_range(lines, 'get_valid_input', 0)
    if gvi_start:
        # Find the fgets -> NULL handling
        for i in range(gvi_start, gvi_end):
            line = lines[i]
            if 'fgets(buffer, sizeof(buffer), stdin) == NULL' in line:
                # Check if next line already has clearerr
                if i+1 < len(lines) and 'clearerr' not in lines[i+1]:
                    # Insert clearerr + emscripten_sleep after the continue
                    for j in range(i, min(i+8, gvi_end)):
                        if 'continue;' in lines[j] and '#ifdef' not in lines[j]:
                            indent = lines[j][:len(lines[j]) - len(lines[j].lstrip())]
                            lines[j] = lines[j]
                            insert = [
                                f'{indent}#ifdef __EMSCRIPTEN__',
                                f'{indent}    clearerr(stdin);',
                                f'{indent}    emscripten_sleep(100);',
                                f'{indent}#endif'
                            ]
                            # Insert after the line with "continue;"
                            lines[j:j] = insert
                            gvi_end += len(insert)
                            break
                break
    
    # For get_valid_input_with_cancel
    gvic_start, gvic_end = find_fn_range(lines, 'get_valid_input_with_cancel', 0)
    if gvic_start:
        for i in range(gvic_start, gvic_end):
            line = lines[i]
            if 'fgets(buffer, sizeof(buffer), stdin) == NULL' in line:
                if i+1 < len(lines) and 'clearerr' not in lines[i+1]:
                    for j in range(i, min(i+8, gvic_end)):
                        if 'continue;' in lines[j] and '#ifdef' not in lines[j]:
                            indent = lines[j][:len(lines[j]) - len(lines[j].lstrip())]
                            insert = [
                                f'{indent}#ifdef __EMSCRIPTEN__',
                                f'{indent}    clearerr(stdin);',
                                f'{indent}    emscripten_sleep(100);',
                                f'{indent}#endif'
                            ]
                            lines[j:j] = insert
                            gvic_end += len(insert)
                            break
                break
    
    content = '\n'.join(lines)
    
    # ===== CHANGE 10: press_any_key - emscripten getchar =====
    content = content.replace(
        'void press_any_key() {\n    printf("\\n" COLOR_GREEN "按任意键继续..." COLOR_RESET);\n    _getch();\n    printf("\\n");\n}',
        '''void press_any_key() {
    printf("\\n" COLOR_GREEN "按任意键继续..." COLOR_RESET);
#ifdef __EMSCRIPTEN__
    getchar();
#else
    _getch();
#endif
    printf("\\n");
}'''
    )
    
    # ===== CHANGE 11: show_skills - system("pause") -> getchar for Emscripten =====
    lines = content.split('\n')
    show_skills_found = False
    i = 0
    while i < len(lines):
        if not show_skills_found and 'void show_skills()' in lines[i]:
            show_skills_found = True
        if show_skills_found and 'system("pause")' in lines[i]:
            indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
            lines[i] = f'{indent}#ifdef __EMSCRIPTEN__'
            lines.insert(i+1, f'{indent}        getchar();')
            lines.insert(i+2, f'{indent}#else')
            lines.insert(i+3, f'{indent}        system("pause");')
            lines.insert(i+4, f'{indent}#endif')
            break
        i += 1
    
    content = '\n'.join(lines)
    
    # ===== CHANGE 12: show_loading_screen - emscripten_sleep =====
    content = content.replace(
        ' #ifdef _WIN32\n        Sleep(duration / 50);\n #else\n        usleep((duration * 1000) / 50);\n #endif',
        '''#ifdef __EMSCRIPTEN__
        emscripten_sleep(duration / 50);
#elif defined(_WIN32)
        Sleep(duration / 50);
#else
        usleep((duration * 1000) / 50);
#endif'''
    )
    
    # ===== CHANGE 13: Main loop yield =====
    # Find `last_update = now;` in the main while(1) loop and add emscripten_sleep(5) after it
    content = content.replace(
        '        player.total_play_time += (int)(now - last_update);\n        last_update = now;\n    }',
        '''        player.total_play_time += (int)(now - last_update);
        last_update = now;
#ifdef __EMSCRIPTEN__
        emscripten_sleep(5);  // 主循环 yield，让浏览器处理事件
#endif
    }'''
    )
    
    # ===== CHANGE 14: anti_analysis_check - disable for Emscripten =====
    content = content.replace(
        '#ifdef _WIN32\n    // 强制切换控制台为UTF-8编码，解决中文乱码\n    system("chcp 65001 > nul");\n    anti_analysis_check();\n #endif',
        '#if defined(_WIN32) && !defined(__EMSCRIPTEN__)\n    // 强制切换控制台为UTF-8编码，解决中文乱码\n    system("chcp 65001 > nul");\n    anti_analysis_check();\n #endif'
    )
    
    # ===== CHANGE 15: Fix enable_ansi for Emscripten =====
    # enable_ansi should be no-op under Emscripten since there's no Windows console
    # The existing #ifdef _WIN32 already handles this correctly
    
    # ===== CHANGE 16: CLEAR_SCREEN is already ANSI in the main file =====
    # #define CLEAR_SCREEN() printf("\033[2J\033[H") - already correct
    
    # Write output
    write_file(dst, content)
    new_lines = len(content.split('\n'))
    print(f"Written {new_lines} lines to {dst}")
    print("Conversion complete!")

if __name__ == '__main__':
    main()
