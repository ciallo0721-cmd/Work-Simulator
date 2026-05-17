#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
打工模拟器 - 内存调试器/状态模拟器 v1.0
用于调试游戏状态系统，不修改坤币等核心数值
支持注入式内存读取，实时监控和模拟游戏状态
"""

import ctypes
import ctypes.wintypes
import struct
import time
import os
import sys
from typing import Optional, Dict, Any

# Windows API 常量
PROCESS_ALL_ACCESS = 0x1F0FFF
PROCESS_VM_READ = 0x0010
PROCESS_VM_WRITE = 0x0020
PROCESS_VM_OPERATION = 0x0008
PROCESS_QUERY_INFORMATION = 0x0400

# ==================== 结构体偏移量定义 ====================
# 基于 Player 结构体精确偏移量
OFFSETS = {
    # 玩家名称（新增）
    'player_name': 0,         # 玩家名称 (50字节)

    # 基础属性（偏移+50）
    'kun_coins': 50,          # 坤币 (8字节) - 只读，不修改
    'kun_exp': 58,            # 经验 (4字节)
    'level': 62,             # 等级 (4字节) - 只读
    'energy': 66,            # 能量 (4字节)
    'max_energy': 70,        # 最大能量 (4字节)

    # 状态系统 (用于调试)
    'hunger': 37818,         # 饱食度 (4字节)
    'health': 37822,         # 血量 (4字节)
    'spirit': 37826,         # 精神 (4字节)
    'is_hallucinating': 37830,  # 幻觉状态 (1字节)
    'hallucination_end': 37834, # 幻觉结束时间 (8字节)
    'sudden_death_chance': 37842, # 猝死几率 (4字节)
    'hallu_count': 37846,    # 幻觉触发次数 (4字节)
    'has_phone': 37850,      # 是否有手机 (4字节)
    'in_chat_room': 37851,   # 是否在聊天室 (4字节)

    # 其他调试用
    'skill_points': 11038,   # 技能点 (4字节)
    'prestige_level': 37770, # 声望等级 (4字节)
    'craft_count': 37814,    # 合成次数 (4字节)
    'lottery_tickets': 37798, # 彩票 (4字节)
    'mystery_keys': 37802,   # 神秘钥匙 (4字节)
    'boss_defeats': 37806,   # BOSS击杀 (4字节)
}

# 福布斯排行榜文件
FORBES_FILE = "forbes_ranking.dat"
FORBES_ENTRY_SIZE = 72

# 字段类型
FIELD_TYPES = {
    'player_name': 's',
    'kun_coins': 'q',
    'kun_exp': 'i',
    'level': 'i',
    'energy': 'i',
    'max_energy': 'i',
    'hunger': 'i',
    'health': 'i',
    'spirit': 'i',
    'is_hallucinating': '?',
    'hallucination_end': 'q',
    'sudden_death_chance': 'i',
    'hallu_count': 'i',
    'has_phone': 'i',
    'in_chat_room': 'i',
    'skill_points': 'i',
    'prestige_level': 'i',
    'craft_count': 'i',
    'lottery_tickets': 'i',
    'mystery_keys': 'i',
    'boss_defeats': 'i',
}


class GameMemoryDebugger:
    """游戏内存调试器"""
    
    def __init__(self, process_name: str = "打工模拟器.exe"):
        self.process_name = process_name
        self.process_handle = None
        self.process_id = None
        self.base_address = None
        self.player_addr = None
        
        # 需要用到的kernel32函数
        self.kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
        
        # 声明函数原型
        self.kernel32.OpenProcess.argtypes = [ctypes.wintypes.DWORD, ctypes.wintypes.BOOL, ctypes.wintypes.DWORD]
        self.kernel32.OpenProcess.restype = ctypes.wintypes.HANDLE
        
        self.kernel32.CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
        self.kernel32.CloseHandle.restype = ctypes.wintypes.BOOL
        
        self.kernel32.ReadProcessMemory.argtypes = [ctypes.wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        self.kernel32.ReadProcessMemory.restype = ctypes.wintypes.BOOL
        
        self.kernel32.WriteProcessMemory.argtypes = [ctypes.wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        self.kernel32.WriteProcessMemory.restype = ctypes.wintypes.BOOL
        
        # 需要用的psapi函数
        self.psapi = ctypes.WinDLL('psapi', use_last_error=True)
        self.psapi.EnumProcesses.argtypes = [ctypes.POINTER(ctypes.wintypes.DWORD), ctypes.wintypes.DWORD, ctypes.POINTER(ctypes.wintypes.DWORD)]
        self.psapi.EnumProcesses.restype = ctypes.wintypes.BOOL
        
        # 获取模块句柄的辅助函数
        self.kernel32.GetModuleHandleW.argtypes = [ctypes.c_wchar_p]
        self.kernel32.GetModuleHandleW.restype = ctypes.c_void_p
        
        self.kernel32.GetProcAddress.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.kernel32.GetProcAddress.restype = ctypes.c_void_p
    
    def find_process_id(self) -> Optional[int]:
        """通过进程名查找进程ID"""
        process_ids = (ctypes.wintypes.DWORD * 1024)()
        cb_needed = ctypes.wintypes.DWORD()
        
        if not self.psapi.EnumProcesses(process_ids, ctypes.sizeof(process_ids), ctypes.byref(cb_needed)):
            return None
        
        num_processes = cb_needed.value // ctypes.sizeof(ctypes.wintypes.DWORD)
        
        for i in range(num_processes):
            pid = process_ids[i]
            if pid == 0:
                continue
            
            # 打开进程获取模块名
            h_process = self.kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
            if h_process:
                # 获取进程可执行文件路径的简化方法：使用GetModuleBaseName
                try:
                    # 改用QueryFullProcessImageName
                    buf = ctypes.create_unicode_buffer(260)
                    size = ctypes.wintypes.DWORD(260)
                    if hasattr(self.kernel32, 'QueryFullProcessImageNameW'):
                        self.kernel32.QueryFullProcessImageNameW.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD, ctypes.c_wchar_p, ctypes.POINTER(ctypes.wintypes.DWORD)]
                        self.kernel32.QueryFullProcessImageNameW.restype = ctypes.wintypes.BOOL
                        if self.kernel32.QueryFullProcessImageNameW(h_process, 0, buf, ctypes.byref(size)):
                            if self.process_name.lower() in buf.value.lower():
                                self.kernel32.CloseHandle(h_process)
                                return pid
                except:
                    pass
                self.kernel32.CloseHandle(h_process)
        
        return None
    
    def attach_process(self) -> bool:
        """附加到游戏进程"""
        print(f"[*] 正在查找进程: {self.process_name}...")
        self.process_id = self.find_process_id()
        
        if not self.process_id:
            print(f"[-] 未找到进程 {self.process_name}")
            print("[!] 请确保游戏正在运行")
            return False
        
        print(f"[+] 找到进程 PID: {self.process_id}")
        
        self.process_handle = self.kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, self.process_id)
        if not self.process_handle:
            print(f"[-] 无法打开进程，错误码: {ctypes.get_last_error()}")
            return False
        
        print("[+] 成功附加到游戏进程")
        return True
    
    def find_player_address(self) -> bool:
        """查找Player结构体地址（通过特征扫描）"""
        # 由于是C全局变量，我们可以尝试通过特征码搜索
        # 这里使用替代方案：需要用户手动输入或通过符号查找
        
        print("[*] 尝试查找Player结构体地址...")
        print("[!] 注意：需要知道Player结构体的实际内存地址")
        print("[!] 建议：在游戏运行时，通过Cheat Engine等工具找到Player地址")
        print("[!] 或手动输入基址偏移")
        
        # 提供手动输入选项
        addr_input = input("[?] 请输入Player结构体地址 (十六进制，如 0x12345678): ").strip()
        
        if addr_input:
            try:
                if addr_input.startswith('0x'):
                    self.player_addr = int(addr_input, 16)
                else:
                    self.player_addr = int(addr_input)
                print(f"[+] 使用手动指定地址: 0x{self.player_addr:X}")
                return True
            except:
                print("[-] 地址格式错误")
                return False
        
        print("[-] 未输入地址")
        return False
    
    def read_memory(self, address: int, size: int) -> Optional[bytes]:
        """读取内存"""
        if not self.process_handle:
            return None
        
        buffer = ctypes.create_string_buffer(size)
        bytes_read = ctypes.c_size_t()
        
        if self.kernel32.ReadProcessMemory(self.process_handle, address, buffer, size, ctypes.byref(bytes_read)):
            return buffer.raw
        return None
    
    def write_memory(self, address: int, data: bytes) -> bool:
        """写入内存"""
        if not self.process_handle:
            return False
        
        bytes_written = ctypes.c_size_t()
        if self.kernel32.WriteProcessMemory(self.process_handle, address, data, len(data), ctypes.byref(bytes_written)):
            return bytes_written.value == len(data)
        return False
    
    def read_field(self, field_name: str) -> Optional[Any]:
        """读取指定字段的值"""
        if not self.player_addr:
            print("[-] 未设置Player地址")
            return None
        
        if field_name not in OFFSETS:
            print(f"[-] 未知字段: {field_name}")
            return None
        
        offset = OFFSETS[field_name]
        addr = self.player_addr + offset
        fmt_char = FIELD_TYPES.get(field_name, 'i')
        
        size_map = {'q': 8, 'i': 4, '?': 1}
        size = size_map.get(fmt_char, 4)
        
        data = self.read_memory(addr, size)
        if data:
            if fmt_char == 'q':
                return struct.unpack('<q', data)[0]
            elif fmt_char == 'i':
                return struct.unpack('<i', data)[0]
            elif fmt_char == '?':
                return data[0] != 0
        return None
    
    def write_field(self, field_name: str, value: Any) -> bool:
        """写入指定字段的值（仅限调试用字段）"""
        # 白名单：只允许修改调试用字段
        debug_fields = ['hunger', 'health', 'spirit', 'is_hallucinating', 
                       'sudden_death_chance', 'hallu_count']
        
        if field_name not in debug_fields:
            print(f"[-] 禁止修改 {field_name}，仅允许修改调试字段")
            return False
        
        if not self.player_addr:
            print("[-] 未设置Player地址")
            return False
        
        offset = OFFSETS[field_name]
        addr = self.player_addr + offset
        fmt_char = FIELD_TYPES.get(field_name, 'i')
        
        # 值域检查
        if field_name in ['hunger', 'health', 'spirit']:
            value = max(0, min(100, int(value)))
        elif field_name == 'is_hallucinating':
            value = 1 if value else 0
        elif field_name == 'sudden_death_chance':
            value = max(0, min(100, int(value)))
        
        if fmt_char == 'q':
            data = struct.pack('<q', int(value))
        elif fmt_char == 'i':
            data = struct.pack('<i', int(value))
        else:  # '?'
            data = bytes([1 if value else 0])
        
        return self.write_memory(addr, data)

    def read_player_name(self) -> Optional[str]:
        """读取玩家名称"""
        if not self.player_addr:
            print("[-] 未设置Player地址")
            return None

        offset = OFFSETS['player_name']
        addr = self.player_addr + offset
        data = self.read_memory(addr, 50)
        if data:
            end = data.find(b'\0')
            if end != -1:
                data = data[:end]
            return data.decode('utf-8', errors='ignore')
        return None

    def show_forbes_ranking(self):
        """显示福布斯排行榜"""
        # 预设名人名单
        famous_names = {
            "马斯克", "比尔盖茨", "贝索斯", "巴菲特",
            "马云", "扎克伯格", "库克", "马化腾",
            "李嘉诚", "钟睒睒"
        }

        if not os.path.exists(FORBES_FILE):
            print("\n[-] 福布斯排行榜文件不存在！")
            return

        try:
            with open(FORBES_FILE, 'rb') as f:
                count = struct.unpack('<i', f.read(4))[0]
                entries = []
                for _ in range(min(count, 100)):
                    entry_data = f.read(FORBES_ENTRY_SIZE)
                    if len(entry_data) < FORBES_ENTRY_SIZE:
                        break
                    name = entry_data[:50].decode('utf-8', errors='ignore').rstrip('\0')
                    coins = struct.unpack('<q', entry_data[50:58])[0]
                    level = struct.unpack('<i', entry_data[58:62])[0]
                    entries.append((name, coins, level))

            print("\n" + "=" * 50)
            print("  福布斯坤币排行榜")
            print("=" * 50)

            player_name = self.read_player_name()

            for i, (name, coins, level) in enumerate(entries[:10], 1):
                marker = ""
                tag = ""
                if name == player_name:
                    marker = " <-- 你"
                elif name in famous_names:
                    tag = " [名人]"
                rank_color = ""
                if i == 1:
                    rank_color = "\033[33m"
                elif i == 2:
                    rank_color = "\033[36m"
                elif i == 3:
                    rank_color = "\033[35m"
                print(f"  {rank_color}{i:2}.{'\033[0m'} {name:15}{tag} {coins:>15} 坤币  Lv.{level}{marker}")

            print("=" * 50)
        except Exception as e:
            print(f"[-] 读取排行榜失败: {e}")

    def simulate_hunger_system(self):
        """模拟饱食度系统变化"""
        print("\n[模拟] 饱食度系统模拟器")
        print("选项:")
        print("  1. 设置饱食度")
        print("  2. 增加饱食度 (+15)")
        print("  3. 减少饱食度 (-15)")
        print("  4. 模拟饥饿状态 (饱食度=10)")
        print("  5. 模拟正常状态 (饱食度=80)")
        print("  0. 返回")
        
        choice = input("\n选择: ").strip()
        
        if choice == '1':
            val = input("输入饱食度 (0-100): ")
            if val.isdigit():
                self.write_field('hunger', int(val))
        elif choice == '2':
            current = self.read_field('hunger')
            if current is not None:
                self.write_field('hunger', current + 15)
        elif choice == '3':
            current = self.read_field('hunger')
            if current is not None:
                self.write_field('hunger', current - 15)
        elif choice == '4':
            self.write_field('hunger', 10)
        elif choice == '5':
            self.write_field('hunger', 80)
    
    def simulate_health_system(self):
        """模拟血量系统变化"""
        print("\n[模拟] 血量系统模拟器")
        print("选项:")
        print("  1. 设置血量")
        print("  2. 增加血量 (+10)")
        print("  3. 减少血量 (-10)")
        print("  4. 模拟重伤状态 (血量=20)")
        print("  5. 模拟健康状态 (血量=100)")
        print("  0. 返回")
        
        choice = input("\n选择: ").strip()
        
        if choice == '1':
            val = input("输入血量 (0-100): ")
            if val.isdigit():
                self.write_field('health', int(val))
        elif choice == '2':
            current = self.read_field('health')
            if current is not None:
                self.write_field('health', current + 10)
        elif choice == '3':
            current = self.read_field('health')
            if current is not None:
                self.write_field('health', current - 10)
        elif choice == '4':
            self.write_field('health', 20)
        elif choice == '5':
            self.write_field('health', 100)
    
    def simulate_spirit_system(self):
        """模拟精神系统变化"""
        print("\n[模拟] 精神系统模拟器")
        print("选项:")
        print("  1. 设置精神")
        print("  2. 增加精神 (+10)")
        print("  3. 减少精神 (-10)")
        print("  4. 触发幻觉状态 (精神=10)")
        print("  5. 触发兴奋状态 (精神=80)")
        print("  6. 恢复正常 (精神=50)")
        print("  7. 清除幻觉标志")
        print("  0. 返回")
        
        choice = input("\n选择: ").strip()
        
        if choice == '1':
            val = input("输入精神 (0-100): ")
            if val.isdigit():
                self.write_field('spirit', int(val))
        elif choice == '2':
            current = self.read_field('spirit')
            if current is not None:
                self.write_field('spirit', current + 10)
        elif choice == '3':
            current = self.read_field('spirit')
            if current is not None:
                self.write_field('spirit', current - 10)
        elif choice == '4':
            self.write_field('spirit', 10)
        elif choice == '5':
            self.write_field('spirit', 80)
        elif choice == '6':
            self.write_field('spirit', 50)
        elif choice == '7':
            self.write_field('is_hallucinating', 0)
    
    def simulate_od_event(self):
        """模拟OD事件效果"""
        print("\n[模拟] OD事件模拟器")
        print("模拟黑产OD事件的效果 (只影响状态，不改变数值)")
        print("效果: 精神=10, 饱食度=100, 血量=50, 猝死几率=90, 幻觉状态")
        
        confirm = input("确认应用模拟效果? (y/n): ").strip().lower()
        if confirm == 'y':
            self.write_field('spirit', 10)
            self.write_field('hunger', 100)
            self.write_field('health', 50)
            self.write_field('sudden_death_chance', 90)
            self.write_field('is_hallucinating', 1)
            print("[+] OD事件模拟完成")
    
    def simulate_tongcheng_event(self):
        """模拟同城事件效果"""
        print("\n[模拟] 同城事件模拟器")
        print("模拟同城见面事件的效果")
        print("效果: 精神=78, 血量=99")
        
        confirm = input("确认应用模拟效果? (y/n): ").strip().lower()
        if confirm == 'y':
            self.write_field('spirit', 78)
            self.write_field('health', 99)
            print("[+] 同城事件模拟完成")
    
    def freeze_spirit(self, target_spirit: int = 50):
        """冻结精神值（需要持续运行）"""
        print(f"\n[模拟] 冻结精神值于 {target_spirit}")
        print("按 Ctrl+C 停止冻结")
        
        try:
            while True:
                self.write_field('spirit', target_spirit)
                time.sleep(0.5)
        except KeyboardInterrupt:
            print("\n[+] 已停止冻结")
    
    def show_current_status(self):
        """显示当前状态"""
        print("\n" + "=" * 50)
        print("  当前游戏状态 (只读)")
        print("=" * 50)
        
        status_fields = [
            ('level', '等级'),
            ('energy', '能量'),
            ('max_energy', '最大能量'),
            ('hunger', '饱食度'),
            ('health', '血量'),
            ('spirit', '精神'),
            ('is_hallucinating', '幻觉状态'),
            ('skill_points', '技能点'),
            ('prestige_level', '声望等级'),
            ('lottery_tickets', '彩票'),
            ('mystery_keys', '神秘钥匙'),
        ]
        
        for field, label in status_fields:
            value = self.read_field(field)
            if value is not None:
                if isinstance(value, bool):
                    display = "是" if value else "否"
                elif field in ['hunger', 'health', 'spirit']:
                    display = f"{value}/100"
                else:
                    display = str(value)
                print(f"  {label:12}: {display}")
        
        # 读取坤币（只读显示）
        coins = self.read_field('kun_coins')
        if coins is not None:
            print(f"  {'坤币':12}: {coins} (只读)")

        # 读取玩家名称
        player_name = self.read_player_name()
        if player_name:
            print(f"  {'玩家名称':12}: {player_name}")

        print("=" * 50)

    def run_debug_console(self):
        """运行调试控制台"""
        print("\n" + "=" * 50)
        print("  打工模拟器 - 内存调试器/状态模拟器 v1.1")
        print("=" * 50)
        print("[说明] 本工具仅用于调试，不修改坤币等核心数值")
        print("[说明] 支持模拟饱食度/血量/精神系统变化和福布斯排行榜")
        print("=" * 50)

        if not self.attach_process():
            print("[!] 附加进程失败")
            return

        # 强制要求输入Player地址
        print("\n[必要] 需要知道Player结构体的内存地址才能操作。")
        print("[提示] 用Cheat Engine找到坤币的地址（初始500），那就是Player地址。")
        if not self.find_player_address():
            print("[!] 未提供有效地址，调试器无法工作")
            return

        while True:
            print("\n" + "=" * 50)
            print("  调试菜单")
            print("=" * 50)
            print("  1. 显示当前状态")
            print("  2. 模拟饱食度系统")
            print("  3. 模拟血量系统")
            print("  4. 模拟精神系统")
            print("  5. 模拟OD事件")
            print("  6. 模拟同城事件")
            print("  7. 冻结精神值 (调试用)")
            print("  8. 设置幻觉状态")
            print("  9. 清除所有异常状态")
            print("  A. 查看福布斯排行榜")
            print("  0. 退出")
            print("=" * 50)

            choice = input("选择: ").strip().upper()

            if choice == '1':
                self.show_current_status()
            elif choice == '2':
                self.simulate_hunger_system()
            elif choice == '3':
                self.simulate_health_system()
            elif choice == '4':
                self.simulate_spirit_system()
            elif choice == '5':
                self.simulate_od_event()
            elif choice == '6':
                self.simulate_tongcheng_event()
            elif choice == '7':
                val = input("输入要冻结的精神值 (默认50): ").strip()
                target = int(val) if val.isdigit() else 50
                self.freeze_spirit(target)
            elif choice == '8':
                val = input("是否开启幻觉状态? (y/n): ").strip().lower()
                self.write_field('is_hallucinating', 1 if val == 'y' else 0)
            elif choice == '9':
                print("清除所有异常状态...")
                self.write_field('hunger', 80)
                self.write_field('health', 100)
                self.write_field('spirit', 50)
                self.write_field('is_hallucinating', 0)
                self.write_field('sudden_death_chance', 0)
                print("[+] 已恢复所有状态到正常水平")
            elif choice == 'A':
                self.show_forbes_ranking()
            elif choice == '0':
                if self.process_handle:
                    self.kernel32.CloseHandle(self.process_handle)
                print("[+] 已断开连接")
                break
            else:
                print("[-] 无效选项")
    
    def __del__(self):
        if hasattr(self, 'process_handle') and self.process_handle:
            self.kernel32.CloseHandle(self.process_handle)


def main():
    # 检查管理员权限
    try:
        import ctypes
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
        if not is_admin:
            print("[!] 建议以管理员身份运行，以便读取游戏内存")
    except:
        pass
    
    debugger = GameMemoryDebugger("打工模拟器.exe")
    
    try:
        debugger.run_debug_console()
    except KeyboardInterrupt:
        print("\n[+] 程序被用户中断")
    except Exception as e:
        print(f"[-] 程序出错: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
