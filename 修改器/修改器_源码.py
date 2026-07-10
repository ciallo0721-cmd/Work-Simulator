#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
打工模拟器存档修改器 v3.0
适配文件夹结构存档 (v8)
save/manifest.work + player/core.player + pets/*.pet + friends/*.chr + tasks.ts + inventory.bag + stats.dat + forbes.list
"""

import struct
import os
import sys
import time
from typing import Dict, Any, Optional

# ==================== 存档路径常量 ====================
SAVE_DIR = "save/"
FORBES_FILE = os.path.join(SAVE_DIR, "forbes.list")
SAVE_VERSION = 8

# ==================== 字段→文件映射 ====================
# 用有序列表定义 core.player 中所有标量字段及其类型
# engine-lead 写入 core.player 时的字段顺序
CORE_PLAYER_FIELDS = [
    ('player_name', '50s'),
    ('kun_coins', 'q'),
    ('kun_exp', 'i'),
    ('level', 'i'),
    ('energy', 'i'),
    ('max_energy', 'i'),
    ('last_login', 'q'),
    ('furniture_count', 'i'),
    ('is_billionaire', '?'),
    ('active_event', 'i'),
    ('event_end_time', 'q'),
    ('consecutive_days', 'i'),
    ('game_start_time', 'q'),
    ('total_play_time', 'i'),
    ('skill_points', 'i'),
    ('pet_count', 'i'),
    ('current_company_level', 'i'),
    ('inventory_count', 'i'),
    ('property_count', 'i'),
    ('friend_count', 'i'),
    ('chat_message_count', 'i'),
    ('chatter_count', 'i'),
    ('daily_task_completions', 'i'),
    ('weekly_task_completions', 'i'),
    ('lifetime_task_completions', 'i'),
    ('total_income', 'q'),
    ('total_expenses', 'q'),
    ('current_season', 'i'),
    ('day_count', 'i'),
    ('prestige_level', 'i'),
    ('prestige_points', 'i'),
    ('has_premium', '?'),
    ('premium_until', 'q'),
    ('achievement_points', 'i'),
    ('lottery_tickets', 'i'),
    ('mystery_keys', 'i'),
    ('boss_defeats', 'i'),
    ('festival_participations', 'i'),
    ('craft_count', 'i'),
    ('daily_time_slots', 'i'),
    ('max_time_slots', 'i'),
    ('today_income', 'i'),
    ('today_expenses', 'i'),
    ('today_work_count', 'i'),
    ('today_event_count', 'i'),
    ('last_difficulty', 'i'),
    ('day_ended', '?'),
    ('rent_cost', 'i'),
    ('living_cost', 'i'),
    ('gray_requests_this_session', 'i'),
    ('lied_to_npc', '?'),
    ('hunger', 'i'),
    ('health', 'i'),
    ('spirit', 'i'),
    ('is_hallucinating', '?'),
    ('hallucination_end', 'q'),
    ('sudden_death_chance', 'i'),
    ('hallu_count', 'i'),
    ('has_phone', '?'),
    ('in_chat_room', '?'),
    ('scammers_detected', 'i'),
    ('insider_profit', 'i'),
    ('daily_positive_events', 'i'),
    ('gray_requests_refused', 'i'),
    ('limited_items_bought', 'i'),
    ('has_never_lied', '?'),
    ('been_bankrupt', '?'),
]

# 构建 struct 格式和字段偏移表
CORE_PLAYER_FMT = '<' + ''.join(t for _, t in CORE_PLAYER_FIELDS)
CORE_PLAYER_STRUCT = struct.Struct(CORE_PLAYER_FMT)
CORE_PLAYER_SIZE = CORE_PLAYER_STRUCT.size

# 从字段列表构建 {field: (file, offset, fmt)} 映射
FIELD_MAP = {}
_core_offset = 0
for name, fmt in CORE_PLAYER_FIELDS:
    FIELD_MAP[name] = ('player/core.player', _core_offset, fmt)
    _core_offset += struct.calcsize('<' + fmt)

# PET 结构体: name[30] + type(int) + level(int) + exp(int) + happiness(int) + hunger(int) + is_unlocked(?) + bonus_multiplier(f) + last_fed(q)
PET_STRUCT = struct.Struct('<30s i i i i i ? f q')
PET_SIZE = PET_STRUCT.size

# FRIEND 结构体: name[30] + level(int) + is_online(?) + last_online(q) + friendship_level(f) + is_friend(?)
FRIEND_STRUCT = struct.Struct('<30s i ? q f ?')
FRIEND_SIZE = FRIEND_STRUCT.size

# 福布斯条目: name[50] + coins(q) + level(i) + record_time(q) + is_active(?) + 1byte padding
FORBES_ENTRY = struct.Struct('<50s q i q ?')
FORBES_ENTRY_SIZE = FORBES_ENTRY.size

# 字段类型定义（用于类型转换）
FIELD_TYPES = {name: fmt for name, fmt in CORE_PLAYER_FIELDS}

# ==================== 可读写的基本字段列表 ====================
BASIC_FIELDS = [
    ('player_name', '玩家名称'),
    ('kun_coins', '坤币'),
    ('kun_exp', '经验'),
    ('level', '等级'),
    ('energy', '能量'),
    ('max_energy', '最大能量'),
    ('skill_points', '技能点'),
    ('consecutive_days', '连续登录'),
    ('day_count', '游戏天数'),
    ('total_play_time', '总游戏时间'),
    ('lifetime_task_completions', '总任务完成'),
    ('current_company_level', '公司等级'),
    ('prestige_level', '声望等级'),
    ('prestige_points', '声望点数'),
    ('achievement_points', '成就点'),
    ('lottery_tickets', '彩票'),
    ('mystery_keys', '神秘钥匙'),
    ('boss_defeats', 'BOSS击杀'),
    ('festival_participations', '节日参与'),
    ('craft_count', '合成次数'),
    ('has_premium', '高级会员'),
    ('is_billionaire', '亿万富翁'),

    # 新增状态字段
    ('hunger', '饱食度'),
    ('health', '血量'),
    ('spirit', '精神'),
    ('is_hallucinating', '幻觉状态'),
    ('has_phone', '是否有手机'),
]


class KunSaveEditor:
    def __init__(self):
        # self.files: {相对路径: bytes} — 在内存中缓存所有存档文件
        self.files: Dict[str, bytes] = {}
        # self.values: {字段名: 值} — 解析后的标量字段
        self.values: Dict[str, Any] = {}
        self.version = 0
        self.player_name = ""

    def _p(self, *parts: str) -> str:
        """拼接存档目录下的路径"""
        return os.path.join(SAVE_DIR, *parts)

    def _read_file(self, path: str) -> Optional[bytes]:
        """读取存档子文件，返回字节或 None"""
        full = self._p(path)
        if not os.path.exists(full):
            return None
        with open(full, 'rb') as f:
            return f.read()

    def _write_file(self, path: str, data: bytes) -> bool:
        """写入存档子文件，自动创建目录"""
        full = self._p(path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, 'wb') as f:
            f.write(data)
        return True

    def load_save(self) -> bool:
        """从 save/ 文件夹加载存档"""
        if not os.path.exists(self._p("manifest.work")):
            print(f"错误: 存档目录 {SAVE_DIR} 不存在或未找到 manifest.work")
            return False

        try:
            # 1. 读取 manifest.work
            raw = self._read_file("manifest.work")
            if not raw or len(raw) < 48:
                print("错误: manifest.work 损坏")
                return False
            ver, name_b, day_cnt, save_t = struct.unpack_from('<i 32s i q', raw, 0)
            self.version = ver
            self.player_name = name_b.rstrip(b'\0').decode('utf-8', errors='ignore')

            # 2. 读取 core.player → 解析全部标量字段
            core_raw = self._read_file("player/core.player")
            if core_raw and len(core_raw) >= CORE_PLAYER_SIZE:
                vals = CORE_PLAYER_STRUCT.unpack_from(core_raw, 0)
                for (name, _), val in zip(CORE_PLAYER_FIELDS, vals):
                    if isinstance(val, bytes):
                        val = val.rstrip(b'\0').decode('utf-8', errors='ignore')
                    self.values[name] = val
            else:
                print("警告: core.player 缺失，使用默认值")
                for name, fmt in CORE_PLAYER_FIELDS:
                    if fmt == '50s':
                        self.values[name] = self.player_name
                    elif fmt == '?':
                        self.values[name] = False
                    elif fmt == 'q':
                        self.values[name] = 0
                    else:
                        self.values[name] = 0

            # 3. 缓存所有文件到 self.files 供其他方法使用
            for fname in ["tasks.ts", "inventory.bag", "stats.dat", "forbes.list"]:
                data = self._read_file(fname)
                if data:
                    self.files[fname] = data

            # 读取 pets/
            pet_raw = self._read_file("pets/main.dat")
            if pet_raw:
                self.files["pets/main.dat"] = pet_raw
                pet_count = self.values.get('pet_count', 0)
                for i in range(min(pet_count, 5)):
                    d = self._read_file(f"pets/{i}.pet")
                    if d:
                        self.files[f"pets/{i}.pet"] = d

            # 读取 friends/
            fr_raw = self._read_file("friends/main.dat")
            if fr_raw:
                self.files["friends/main.dat"] = fr_raw
                fr_count = self.values.get('friend_count', 0)
                for i in range(min(fr_count, 10)):
                    d = self._read_file(f"friends/{i}.chr")
                    if d:
                        self.files[f"friends/{i}.chr"] = d

            print(f"已加载存档: {self.player_name} (v{self.version}), Day {self.values.get('day_count', 0)}")
            return True
        except Exception as e:
            print(f"加载存档失败: {e}")
            import traceback
            traceback.print_exc()
            return False

    def save_save(self) -> bool:
        """写回 save/ 文件夹"""
        try:
            # manifest.work
            name_b = self.player_name.encode('utf-8')[:31].ljust(32, b'\0')
            manifest = struct.pack('<i 32s i q', SAVE_VERSION, name_b,
                                   self.values.get('day_count', 0), int(time.time()))
            self._write_file("manifest.work", manifest)

            # core.player
            core_vals = []
            for name, fmt in CORE_PLAYER_FIELDS:
                v = self.values.get(name, 0)
                if fmt == '50s':
                    if isinstance(v, str):
                        v = v.encode('utf-8')[:49].ljust(50, b'\0')
                    elif v is None:
                        v = b'\0' * 50
                core_vals.append(v)
            core_data = CORE_PLAYER_STRUCT.pack(*core_vals)
            self._write_file("player/core.player", core_data)

            # pets/
            for i in range(5):
                key = f"pets/{i}.pet"
                if key in self.files:
                    self._write_file(key, self.files[key])

            # friends/
            for i in range(10):
                key = f"friends/{i}.chr"
                if key in self.files:
                    self._write_file(key, self.files[key])

            # 其他文件
            for fname in ["pets/main.dat", "friends/main.dat", "tasks.ts",
                          "inventory.bag", "stats.dat", "forbes.list"]:
                if fname in self.files:
                    self._write_file(fname, self.files[fname])

            print("  存档已保存! (文件夹结构)")
            return True
        except Exception as e:
            print(f"保存存档失败: {e}")
            return False

    def read_value(self, field: str) -> Optional[Any]:
        """读取标量字段值"""
        if field not in FIELD_MAP:
            print(f"错误: 不支持读取字段 {field}")
            return None
        return self.values.get(field)

    def write_value(self, field: str, value: Any) -> bool:
        """写入标量字段值"""
        if field not in FIELD_MAP:
            print(f"错误: 不支持写入字段 {field}")
            return False

        _, _, fmt = FIELD_MAP[field]
        # 类型转换
        if fmt in ('?',):
            if isinstance(value, str):
                value = value.lower() in ['true', '1', 'yes', 'y']
            else:
                value = bool(value)
        elif fmt == 'q':
            if isinstance(value, str) and value.lower() == 'now':
                value = int(time.time())
            else:
                value = int(value)
        else:  # 'i'
            if isinstance(value, str) and value.lower() == 'now':
                value = int(time.time())
            else:
                value = int(value)
            if field in ['hunger', 'health', 'spirit']:
                if value < 0: value = 0
                if value > 100: value = 100

        self.values[field] = value
        print(f"成功设置 {field} = {value}")
        return True

    def read_player_name(self) -> Optional[str]:
        return self.values.get('player_name', self.player_name)

    def write_player_name(self, name: str) -> bool:
        if len(name) > 49:
            name = name[:49]
        self.values['player_name'] = name
        self.player_name = name
        print(f"成功设置玩家名称 = {name}")
        return True

    # ---------- 家具 ----------
    _FURNITURE_OFFSET = 2600  # Player 结构体中家具数组起始的大致位置
    _FURNITURE_ITEM_SIZE = 50
    _FURNITURE_COUNT = 50

    def read_furniture(self, index: int = 0) -> Optional[str]:
        """从 core.player 的家具区域读取"""
        if index < 0 or index >= self._FURNITURE_COUNT:
            return None
        if "player/core.player" not in self._cache_furniture:
            self._load_furniture_cache()
        cache = self._cache_furniture.get("player/core.player")
        if not cache or len(cache) < self._FURNITURE_OFFSET + (index + 1) * self._FURNITURE_ITEM_SIZE:
            return None
        raw = cache[self._FURNITURE_OFFSET + index * self._FURNITURE_ITEM_SIZE:
                    self._FURNITURE_OFFSET + (index + 1) * self._FURNITURE_ITEM_SIZE]
        end = raw.find(b'\0')
        if end != -1:
            raw = raw[:end]
        return raw.decode('utf-8', errors='ignore')

    def _load_furniture_cache(self):
        """从实际文件加载家具缓存"""
        self._cache_furniture = {}
        raw = self._read_file("player/core.player")
        if raw:
            self._cache_furniture["player/core.player"] = raw

    def write_furniture(self, index: int, name: str) -> bool:
        if index < 0 or index >= self._FURNITURE_COUNT:
            return False
        if "player/core.player" not in self._cache_furniture:
            self._load_furniture_cache()
        if "player/core.player" not in self._cache_furniture:
            self._cache_furniture["player/core.player"] = bytearray(37000)
        cache = bytearray(self._cache_furniture["player/core.player"])
        needed = self._FURNITURE_OFFSET + (index + 1) * self._FURNITURE_ITEM_SIZE
        if len(cache) < needed:
            cache.extend(b'\x00' * (needed - len(cache)))
        name_bytes = name.encode('utf-8')[:49]
        pos = self._FURNITURE_OFFSET + index * self._FURNITURE_ITEM_SIZE
        cache[pos:pos + len(name_bytes)] = name_bytes
        cache[pos + len(name_bytes):pos + self._FURNITURE_ITEM_SIZE] = b'\0' * (self._FURNITURE_ITEM_SIZE - len(name_bytes))
        self._cache_furniture["player/core.player"] = bytes(cache)
        self._write_file("player/core.player", bytes(cache))
        print(f"成功设置家具[{index}] = {name}")
        return True

    def get_furniture_count(self) -> int:
        return self.values.get('furniture_count', 0)

    def show_forbes_ranking(self):
        """显示福布斯排行榜"""
        famous_names = {
            "马斯克", "比尔盖茨", "贝索斯", "巴菲特",
            "马云", "扎克伯格", "库克", "马化腾",
            "李嘉诚", "钟睒睒"
        }

        raw = self.files.get("forbes.list")
        if not raw:
            raw = self._read_file("forbes.list")
        if not raw:
            print("\n  福布斯排行榜文件不存在！")
            return

        try:
            count = struct.unpack('<i', raw[:4])[0]
            entries = []
            pos = 4
            for _ in range(min(count, 100)):
                if pos + FORBES_ENTRY_SIZE > len(raw):
                    break
                entry_data = raw[pos:pos + FORBES_ENTRY_SIZE]
                pos += FORBES_ENTRY_SIZE
                name = entry_data[:50].decode('utf-8', errors='ignore').rstrip('\0')
                coins = struct.unpack('<q', entry_data[50:58])[0]
                level = struct.unpack('<i', entry_data[58:62])[0]
                entries.append((name, coins, level))

            print("\n" + "=" * 60)
            print("  福布斯坤币排行榜")
            print("=" * 60)

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

            print("=" * 60)
        except Exception as e:
            print(f"读取排行榜失败: {e}")

    def update_forbes_ranking(self):
        """更新福布斯排行榜"""
        player_name = self.read_player_name()
        kun_coins = self.read_value('kun_coins')
        level = self.read_value('level')

        if player_name is None or kun_coins is None or level is None:
            print("错误: 无法获取玩家数据")
            return

        # 读取现有排行榜
        entries = []
        raw = self.files.get("forbes.list") or self._read_file("forbes.list")
        if raw and len(raw) >= 4:
            try:
                count = struct.unpack('<i', raw[:4])[0]
                pos = 4
                for _ in range(min(count, 100)):
                    if pos + FORBES_ENTRY_SIZE > len(raw):
                        break
                    entries.append(raw[pos:pos + FORBES_ENTRY_SIZE])
                    pos += FORBES_ENTRY_SIZE
            except:
                entries = []

        # 查找并更新或添加玩家记录
        new_entry = player_name.encode('utf-8')[:50].ljust(50, b'\0')
        new_entry += struct.pack('<q', kun_coins)
        new_entry += struct.pack('<i', level)
        new_entry += struct.pack('<q', int(time.time()))
        new_entry += struct.pack('<?', True)

        found = False
        for i, entry in enumerate(entries):
            name = entry[:50].decode('utf-8', errors='ignore').rstrip('\0')
            if name == player_name:
                entries[i] = new_entry
                found = True
                break

        if not found:
            entries.append(new_entry)

        # 按坤币排序
        def get_coins(entry):
            return struct.unpack('<q', entry[50:58])[0]
        entries.sort(key=get_coins, reverse=True)
        entries = entries[:100]

        # 写入
        result = struct.pack('<i', len(entries))
        for entry in entries:
            result += entry
        self.files["forbes.list"] = result
        self._write_file("forbes.list", result)

        print(f"\n  福布斯排行榜已更新！")
        print(f"  玩家: {player_name}")
        print(f"  坤币: {kun_coins}")

    # ==================== 存档修复系统 ====================

    def fix_menu(self):
        """存档修复主菜单"""
        if not self.values:
            print("错误: 请先加载存档")
            return

        print("\n" + "=" * 60)
        print("  存档修复工具")
        print("=" * 60)
        print("  1. 恢复成空存档 (填零)")
        print("  2. 恢复成默认存档")
        print("  3. 修复存档 (智能检测并修复)")
        print("=" * 60)
        choice = input("  请选择 (1-3): ").strip()

        if choice == '1':
            self._reset_empty()
        elif choice == '2':
            self._reset_default()
        elif choice == '3':
            self._repair_submenu()
        else:
            print("  无效选择")

    def _reset_empty(self):
        """恢复成空存档 - 清零所有字段"""
        print("\n  ⚠️ 此操作将清空所有存档数据!")
        confirm = input("  确认清空? (输入 YES 确认): ").strip()
        if confirm != "YES":
            print("  已取消")
            return

        for name, fmt in CORE_PLAYER_FIELDS:
            if fmt == '50s':
                self.values[name] = ''
            elif fmt == '?':
                self.values[name] = False
            elif fmt == 'q':
                self.values[name] = 0
            else:
                self.values[name] = 0
        self.files = {}
        self.version = SAVE_VERSION
        print("  ✅ 已清空所有数据")
        self.save_save()

    def _reset_default(self):
        """恢复成默认存档"""
        print("\n  ⚠️ 此操作将重置为默认存档!")
        confirm = input("  确认重置? (输入 YES 确认): ").strip()
        if confirm != "YES":
            print("  已取消")
            return

        defaults = {
            'player_name': '打工人',
            'kun_coins': 100,
            'kun_exp': 0,
            'level': 1,
            'energy': 100,
            'max_energy': 100,
            'skill_points': 0,
            'hunger': 100,
            'health': 100,
            'spirit': 50,
            'is_hallucinating': False,
            'has_phone': True,
            'in_chat_room': False,
        }
        for name, fmt in CORE_PLAYER_FIELDS:
            if fmt == '50s':
                self.values[name] = defaults.get(name, '')
            elif fmt == '?':
                self.values[name] = defaults.get(name, False)
            elif fmt == 'q':
                self.values[name] = defaults.get(name, 0)
            else:
                self.values[name] = defaults.get(name, 0)

        print("  ✅ 已恢复为默认存档")
        self.save_save()

    def _repair_submenu(self):
        """修复存档子菜单"""
        print("\n" + "-" * 40)
        print("  修复方式:")
        print("  1. 完整修复 (尽可能修复所有数据)")
        print("  2. 高级修复 (严重损坏时使用，暴力重建)")
        print("  3. 自定义修复 (选择性修复指定项目)")
        print("-" * 40)
        choice = input("  请选择 (1-3): ").strip()

        if choice == '1':
            self._repair_full()
        elif choice == '2':
            self._repair_advanced()
        elif choice == '3':
            self._repair_custom_menu()
        else:
            print("  无效选择")

    def _repair_full(self):
        """完整修复 - 扫描并修复所有常见问题"""
        print("\n  🔧 正在进行完整修复...")
        fixes = 0

        # 版本号
        if self.version != SAVE_VERSION:
            self.version = SAVE_VERSION
            print(f"  [修复] 版本号 -> v{SAVE_VERSION}")
            fixes += 1

        # 布尔值
        for field in ['is_billionaire', 'has_premium', 'is_hallucinating']:
            val = self.read_value(field)
            if val is not None and val not in (0, 1, True, False):
                self.write_value(field, False)
                print(f"  [修复] {field} 值异常，已重置")
                fixes += 1

        # 能量
        energy = self.read_value('energy')
        max_e = self.read_value('max_energy')
        if energy is not None and max_e is not None:
            if energy < 0 or energy > 9999:
                self.write_value('energy', min(max_e, 100))
                fixes += 1
            if max_e < 10 or max_e > 9999:
                self.write_value('max_energy', 100)
                fixes += 1
            if energy > max_e:
                self.write_value('energy', max_e)
                print(f"  [修复] 能量 > 最大能量，已修正")
                fixes += 1

        # 状态属性
        for field, (lo, hi) in [('hunger', (0, 100)), ('health', (0, 100)), ('spirit', (0, 100)), ('sudden_death_chance', (0, 100))]:
            val = self.read_value(field)
            if val is not None and (val < lo or val > hi):
                self.write_value(field, max(lo, min(hi, (lo + hi) // 2)))
                print(f"  [修复] {field} 超出范围 ({val})，已修正")
                fixes += 1

        # 等级
        level = self.read_value('level')
        if level is not None and (level < 1 or level > 32767):
            self.write_value('level', 1)
            print(f"  [修复] 等级({level})异常，已修正为 1")
            fixes += 1

        # 坤币
        kun = self.read_value('kun_coins')
        if kun is not None and kun < 0:
            self.write_value('kun_coins', 0)
            print(f"  [修复] 坤币为负，已清零")
            fixes += 1

        # 经验
        exp = self.read_value('kun_exp')
        if exp is not None and exp < 0:
            self.write_value('kun_exp', 0)
            fixes += 1

        # 各种计数器
        cap_limits = {
            'furniture_count': 50, 'inventory_count': 100, 'pet_count': 5,
            'friend_count': 10, 'property_count': 10, 'chat_message_count': 50,
            'chatter_count': 8
        }
        for field, limit in cap_limits.items():
            val = self.read_value(field)
            if val is not None and (val < 0 or val > limit):
                self.write_value(field, 0)
                print(f"  [修复] {field}({val}) 超出范围，已清零")
                fixes += 1

        # 玩家名称
        name = self.read_player_name()
        if name is None or len(name.strip()) == 0:
            self.write_player_name("打工仔")
            print(f"  [修复] 玩家名称为空，已设置默认")
            fixes += 1

        if fixes == 0:
            print("  ✅ 存档状态良好，无需修复")
        else:
            print(f"\n  ✅ 完整修复完成，共修复 {fixes} 个问题")
            self.save_save()

    def _repair_advanced(self):
        """高级修复 - 严重损坏时的暴力重建"""
        print("\n  ⚠️ 高级修复将暴力重建存档结构，可能丢失部分数据!")
        confirm = input("  确认进行高级修复? (输入 YES 确认): ").strip()
        if confirm != "YES":
            print("  已取消")
            return

        print("\n  🔧 正在进行高级修复...")
        self.version = SAVE_VERSION

        safe_fields = {
            'level': 1, 'kun_coins': 100, 'kun_exp': 0,
            'energy': 100, 'max_energy': 100, 'hunger': 100,
            'health': 100, 'spirit': 50, 'skill_points': 0,
            'has_phone': 1, 'is_hallucinating': 0, 'has_premium': 0,
            'is_billionaire': 0,
        }

        fixed = 0
        for field, default in safe_fields.items():
            val = self.read_value(field)
            if val is None:
                self.write_value(field, default)
                print(f"  [重建] {field} -> {default}")
                fixed += 1

        name = self.read_player_name()
        if name is None or len(name.strip()) == 0:
            self.write_player_name("打工人")
            print(f"  [重建] 玩家名称 -> 打工人")
            fixed += 1

        reset_counters = ['furniture_count', 'inventory_count', 'pet_count',
                          'friend_count', 'property_count', 'chat_message_count',
                          'chatter_count', 'craft_count', 'boss_defeats',
                          'festival_participations', 'lottery_tickets', 'mystery_keys',
                          'daily_task_completions', 'weekly_task_completions',
                          'lifetime_task_completions', 'prestige_level', 'prestige_points',
                          'achievement_points']
        for field in reset_counters:
            self.write_value(field, 0)
        fixed += len(reset_counters)

        print(f"\n  ✅ 高级修复完成，重建 {fixed} 个字段")
        self.save_save()

    def _repair_custom_menu(self):
        """自定义修复菜单"""
        while True:
            print("\n" + "-" * 40)
            print("  自定义修复 - 选择要修复的项目:")
            print("  1. 修复坤币")
            print("  2. 修复饱食度")
            print("  3. 修复经验")
            print("  4. 修复能量")
            print("  5. 修复声望")
            print("  6. 修复背包物品")
            print("  7. 修复家具")
            print("  8. 修复宠物")
            print("  9. 修复排行榜")
            print("  0. 返回上级菜单")
            print("-" * 40)
            choice = input("  请选择 (0-9): ").strip()

            if choice == '0':
                break
            elif choice == '1':
                self._fix_kun_coins()
            elif choice == '2':
                self._fix_hunger()
            elif choice == '3':
                self._fix_exp()
            elif choice == '4':
                self._fix_energy()
            elif choice == '5':
                self._fix_prestige()
            elif choice == '6':
                self._fix_inventory()
            elif choice == '7':
                self._fix_furniture()
            elif choice == '8':
                self._fix_pets()
            elif choice == '9':
                self._fix_forbes()
            else:
                print("  无效选择")

    # --- 单项修复 ---
    def _fix_kun_coins(self):
        val = self.read_value('kun_coins')
        if val is None or val < 0:
            self.write_value('kun_coins', 100)
            print(f"  ✅ 坤币已修复 -> 100")
        elif val > 999999999:
            print(f"  当前坤币: {val}")
            if input("  数值异常大，重置为 999999999? (y/n): ").strip().lower() == 'y':
                self.write_value('kun_coins', 999999999)
                print("  ✅ 已修正")
        else:
            print(f"  坤币正常: {val}")

    def _fix_hunger(self):
        val = self.read_value('hunger')
        if val is None or val < 0 or val > 100:
            self.write_value('hunger', 100)
            print(f"  ✅ 饱食度已修复 -> 100")
        else:
            print(f"  饱食度正常: {val}/100")

    def _fix_exp(self):
        val = self.read_value('kun_exp')
        if val is None or val < 0 or val > 99999999:
            self.write_value('kun_exp', 0)
            print(f"  ✅ 经验已修复 -> 0")
        else:
            print(f"  经验正常: {val}")

    def _fix_energy(self):
        e = self.read_value('energy')
        m = self.read_value('max_energy')
        fixed = False
        if e is None or e < 0:
            self.write_value('energy', 100); e = 100; fixed = True
        if m is None or m < 10:
            self.write_value('max_energy', 100); m = 100; fixed = True
        if e > m:
            self.write_value('energy', m); fixed = True
        if fixed:
            print(f"  ✅ 能量已修复 -> {self.read_value('energy')}/{self.read_value('max_energy')}")
        else:
            print(f"  能量正常: {e}/{m}")

    def _fix_prestige(self):
        for field, name in [('prestige_level', '声望等级'), ('prestige_points', '声望点数'), ('achievement_points', '成就点数')]:
            val = self.read_value(field)
            if val is None or val < 0:
                self.write_value(field, 0)
                print(f"  ✅ {name} 已修复 -> 0")
            elif val > 99999:
                self.write_value(field, 9999)
                print(f"  ✅ {name} 异常大，已修正 -> 9999")
        print("  ✅ 声望相关属性已检查")

    def _fix_inventory(self):
        count = self.read_value('inventory_count')
        if count is None or count < 0 or count > 100:
            self.write_value('inventory_count', 0)
            print(f"  ✅ 背包物品数量已清零")
        else:
            print(f"  背包物品数量: {count}")

    def _fix_furniture(self):
        fc = self.read_value('furniture_count')
        actual = 0
        for i in range(50):
            name = self.read_furniture(i)
            if name and name.strip():
                actual = i + 1
        if fc != actual:
            self.write_value('furniture_count', actual)
            print(f"  ✅ 家具数量已修正 -> {actual}")
        else:
            print(f"  家具数量正常: {fc}")

    def _fix_pets(self):
        count = self.read_value('pet_count')
        if count is None or count < 0 or count > 5:
            self.write_value('pet_count', 0)
            print(f"  ✅ 宠物数量已清零")
        else:
            print(f"  宠物数量: {count}")

    def _fix_forbes(self):
        fp = self._p("forbes.list")
        if os.path.exists(fp):
            try:
                with open(fp, 'rb') as f:
                    count = struct.unpack('<i', f.read(4))[0]
                if count < 0 or count > 100:
                    print(f"  排行榜条目数异常({count})")
                    if input("  重置排行榜? (y/n): ").strip().lower() == 'y':
                        os.remove(fp)
                        print("  ✅ 排行榜已删除")
                else:
                    print(f"  排行榜正常: {count} 条记录")
            except:
                print("  ⚠️ 排行榜文件损坏，正在删除...")
                os.remove(fp)
                print("  ✅ 已删除损坏的排行榜")
        else:
            print("  排行榜文件不存在，无需修复")

    def show_status(self):
        """显示当前状态"""
        if not self.values:
            print("请先加载存档")
            return

        print("=" * 60)
        print("  当前存档状态")
        print("=" * 60)

        # 显示玩家名称
        player_name = self.read_player_name()
        if player_name:
            print(f"  {'玩家名称':12}: {player_name}")

        display_fields = [
            ('等级', 'level'),
            ('经验', 'kun_exp'),
            ('坤币', 'kun_coins'),
            ('能量', 'energy'),
            ('最大能量', 'max_energy'),
            ('技能点', 'skill_points'),
            ('连续登录', 'consecutive_days'),
            ('游戏天数', 'day_count'),
            ('总任务完成', 'lifetime_task_completions'),
            ('公司等级', 'current_company_level'),
            ('声望等级', 'prestige_level'),
            ('声望点数', 'prestige_points'),
            ('成就点', 'achievement_points'),
            ('彩票', 'lottery_tickets'),
            ('神秘钥匙', 'mystery_keys'),
            ('BOSS击杀', 'boss_defeats'),
            ('节日参与', 'festival_participations'),
            ('合成次数', 'craft_count'),
            ('高级会员', 'has_premium'),
            ('亿万富翁', 'is_billionaire'),
        ]

        for label, field in display_fields:
            value = self.read_value(field)
            if value is not None:
                if field == 'has_premium':
                    display = "是" if value else "否"
                elif field == 'is_billionaire':
                    display = "是" if value else "否"
                else:
                    display = str(value)
                print(f"  {label:12}: {display}")

        # 显示状态字段
        print(f"\n  {'状态':12}:")
        hunger = self.read_value('hunger')
        health = self.read_value('health')
        spirit = self.read_value('spirit')
        hallucinating = self.read_value('is_hallucinating')
        has_phone = self.read_value('has_phone')
        
        if hunger is not None:
            print(f"    饱食度: {hunger}/100")
        if health is not None:
            print(f"    血量: {health}/100")
        if spirit is not None:
            print(f"    精神: {spirit}/100")
        if hallucinating is not None:
            print(f"    幻觉状态: {'是' if hallucinating else '否'}")
        if has_phone is not None:
            print(f"    手机: {'有' if has_phone else '无'}")

        # 显示家具
        count = self.get_furniture_count()
        if count > 0:
            print(f"\n  家具 ({count}件):")
            for i in range(min(count, 5)):
                name = self.read_furniture(i)
                if name:
                    print(f"    [{i}] {name}")
            if count > 5:
                print(f"    ... 还有 {count - 5} 件家具")

        print("=" * 60)

    def preset_cheats(self):
        """预设作弊选项"""
        presets = {
            '1': ('无限金钱', {'kun_coins': 999999999}),
            '2': ('满级', {'level': 100, 'kun_exp': 0}),
            '3': ('无限能量', {'energy': 9999, 'max_energy': 9999}),
            '4': ('全技能点', {'skill_points': 999}),
            '5': ('全状态满', {'hunger': 100, 'health': 100, 'spirit': 50, 'is_hallucinating': False}),
            '6': ('解锁所有', {
                'current_company_level': 9,
                'prestige_level': 5,
                'prestige_points': 9999,
                'achievement_points': 9999,
                'lottery_tickets': 99,
                'mystery_keys': 99,
                'boss_defeats': 99,
                'craft_count': 999,
                'has_phone': True,
            }),
            '7': ('高级会员', {
                'has_premium': True,
                'premium_until': 'now',
            }),
            '8': ('重置存档', {
                'level': 1,
                'kun_coins': 100,
                'kun_exp': 0,
                'energy': 100,
                'max_energy': 100,
                'skill_points': 0,
                'prestige_level': 0,
                'prestige_points': 0,
                'hunger': 100,
                'health': 100,
                'spirit': 50,
                'is_hallucinating': False,
                'has_phone': True,
            }),
        }

        print("\n  预设作弊:")
        for key, (name, _) in presets.items():
            print(f"    {key}. {name}")

        choice = input("\n  选择预设 (1-8) 或直接回车跳过: ").strip()
        if choice in presets:
            name, values = presets[choice]
            print(f"\n  应用预设: {name}")
            for field, value in values.items():
                self.write_value(field, value)
            return True
        return False

    def interactive_mode(self):
        """交互式修改模式"""
        while True:
            print("\n" + "=" * 60)
            print("  存档修改器 v2.3 - 交互模式")
            print("=" * 60)
            print("  1. 显示当前状态")
            print("  2. 修改基本属性")
            print("  3. 修改家具")
            print("  4. 应用预设作弊")
            print("  5. 修改状态属性 (饱食度/血量/精神)")
            print("  6. 修改玩家名称")
            print("  7. 福布斯排行榜")
            print("  8. 修复存档 (一键修复异常数据)")
            print("  9. 保存并退出")
            print("  10. 退出不保存")
            print("=" * 60)

            choice = input("  请选择 (1-10): ").strip()

            if choice == '1':
                self.show_status()

            elif choice == '2':
                print("\n  可修改的基本属性:")
                for i, (field, name) in enumerate(BASIC_FIELDS, 1):
                    value = self.read_value(field)
                    if field == 'player_name':
                        value = self.read_player_name()
                    elif field in ('has_premium', 'is_billionaire', 'is_hallucinating'):
                        display = "是" if value else "否"
                    else:
                        display = str(value) if value is not None else "N/A"
                    if field != 'player_name':
                        print(f"  {i:2}. {name:12} : {display}")
                    if field in ('has_premium', 'is_billionaire', 'is_hallucinating'):
                        display = "是" if value else "否"
                    else:
                        display = str(value) if value is not None else "N/A"
                    print(f"  {i:2}. {name:12} : {display}")

                print("\n  输入要修改的编号 (或直接输入字段名): ", end='')
                field_input = input().strip()

                if field_input.isdigit():
                    idx = int(field_input) - 1
                    if 0 <= idx < len(BASIC_FIELDS):
                        field = BASIC_FIELDS[idx][0]
                    else:
                        print("  编号无效")
                        continue
                else:
                    field = field_input

                field_names = [f[0] for f in BASIC_FIELDS]
                if field in field_names:
                    current = self.read_value(field)
                    if field in ('has_premium', 'is_billionaire', 'is_hallucinating'):
                        print(f"  当前值: {'是' if current else '否'}，输入新值 (true/false/1/0): ", end='')
                    else:
                        print(f"  当前值: {current}，输入新值: ", end='')
                    new_value = input().strip()
                    if new_value:
                        self.write_value(field, new_value)
                else:
                    print("  字段名无效")

            elif choice == '3':
                count = self.get_furniture_count()
                print(f"\n  当前有 {count} 件家具")

                print("  1. 查看家具")
                print("  2. 添加家具")
                print("  3. 修改家具数量")
                sub_choice = input("  请选择 (1-3): ").strip()

                if sub_choice == '1':
                    for i in range(min(count, 20)):
                        name = self.read_furniture(i)
                        if name:
                            print(f"    [{i:2}] {name}")
                    if count > 20:
                        print(f"    ... 还有 {count - 20} 件家具")

                elif sub_choice == '2':
                    name = input("  输入家具名称: ").strip()
                    if name:
                        for i in range(50):
                            existing = self.read_furniture(i)
                            if not existing or existing.isspace():
                                self.write_furniture(i, name)
                                new_count = max(count, i + 1)
                                self.write_value('furniture_count', new_count)
                                print(f"  已添加家具到位置 {i}")
                                break
                        else:
                            print("  家具已满!")

                elif sub_choice == '3':
                    new_count = input(f"  当前家具数量: {count}，输入新数量 (0-50): ").strip()
                    if new_count.isdigit():
                        new_count = int(new_count)
                        if 0 <= new_count <= 50:
                            self.write_value('furniture_count', new_count)
                            print(f"  家具数量已更新为 {new_count}")
                        else:
                            print("  数量必须在 0-50 之间")
                    else:
                        print("  请输入数字")

            elif choice == '4':
                self.preset_cheats()

            elif choice == '5':
                print("\n  修改状态属性:")
                hunger = self.read_value('hunger')
                health = self.read_value('health')
                spirit = self.read_value('spirit')
                print(f"  当前饱食度: {hunger}/100")
                print(f"  当前血量: {health}/100")
                print(f"  当前精神: {spirit}/100")

                new_hunger = input("  输入新饱食度 (0-100，回车跳过): ").strip()
                if new_hunger and new_hunger.isdigit():
                    self.write_value('hunger', int(new_hunger))

                new_health = input("  输入新血量 (0-100，回车跳过): ").strip()
                if new_health and new_health.isdigit():
                    self.write_value('health', int(new_health))

                new_spirit = input("  输入新精神 (0-100，回车跳过): ").strip()
                if new_spirit and new_spirit.isdigit():
                    self.write_value('spirit', int(new_spirit))

            elif choice == '6':
                # 修改玩家名称
                current_name = self.read_player_name()
                print(f"\n  当前玩家名称: {current_name}")
                new_name = input("  输入新名称 (回车跳过): ").strip()
                if new_name:
                    self.write_player_name(new_name)

            elif choice == '7':
                # 福布斯排行榜
                print("\n  福布斯排行榜管理:")
                print("  1. 查看排行榜")
                print("  2. 更新我的排名")
                sub_choice = input("  请选择 (1-2): ").strip()
                if sub_choice == '1':
                    self.show_forbes_ranking()
                elif sub_choice == '2':
                    self.update_forbes_ranking()

            elif choice == '8':
                self.fix_menu()

            elif choice == '9':
                if self.save_save():
                    print("  修改已保存!")
                break

            elif choice == '10':
                print("  修改已丢弃")
                break

            else:
                print("  无效选择")


def find_save_dir() -> str:
    """自动查找存档目录"""
    if os.path.exists("save/manifest.work"):
        return "save/"
    # 尝试上级目录
    parent = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'save')
    if os.path.exists(os.path.join(parent, "manifest.work")):
        return parent + "/"
    return "save/"


def main():
    print("=" * 60)
    print("  打工模拟器存档修改器 v3.0")
    print("  适配文件夹结构存档 (save/)")
    print("=" * 60)

    save_dir = find_save_dir()
    if os.path.exists(os.path.join(save_dir, "manifest.work")):
        print(f"  找到存档目录: {save_dir}")
    else:
        print(f"  未找到存档目录 (期望 {save_dir}manifest.work)")
        custom = input("  请输入存档目录路径 (直接回车退出): ").strip()
        if not custom:
            return
        save_dir = custom.rstrip('/\\') + '/'

    global SAVE_DIR
    SAVE_DIR = save_dir

    editor = KunSaveEditor()

    if not editor.load_save():
        print("  无法加载存档，创建新存档吗？ (y/n): ", end='')
        if input().strip().lower() == 'y':
            editor.version = SAVE_VERSION
            editor.values['player_name'] = '打工人'
            editor.player_name = '打工人'
            print("  ✅ 已创建新存档数据")
        else:
            print("  退出程序")
            return

    editor.interactive_mode()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n  程序被用户中断")
    except Exception as e:
        print(f"\n  程序出错: {e}")
        import traceback
        traceback.print_exc()
