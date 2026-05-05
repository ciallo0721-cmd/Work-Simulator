#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
打工模拟器存档修改器 v2.1
支持修改 worker_sim_save.dat 文件
基于 Player 结构体精确偏移量计算 (Windows x64 MSVC)

存档格式: [4字节版本号(int)] + [Player结构体]
存档版本: v4
"""

import struct
import os
import sys
import time
from typing import Dict, Any, Optional

# ==================== 结构体大小常量 ====================
ACHIEVEMENT_SIZE = 280
SKILL_SIZE = 148
QUEST_SIZE = 304
PET_SIZE = 72
COMPANY_LEVEL_SIZE = 160
INVENTORY_ITEM_SIZE = 176
STOCK_SIZE = 56
PROPERTY_SIZE = 160
FRIEND_SIZE = 56
CHAT_MESSAGE_SIZE = 240
CHATTER_SIZE = 144
PLAYER_SIZE = 38360  # 更新：新增了hunger, health, spirit等字段
SAVE_VERSION = 4
SAVE_HEADER_SIZE = 4

# ==================== 字段偏移量 (相对于Player结构体起始位置) ====================
OFFSETS = {
    # 原有字段
    'kun_coins': 0,
    'kun_exp': 8,
    'level': 12,
    'energy': 16,
    'max_energy': 20,
    'last_login': 24,
    'furniture': 32,
    'furniture_count': 2532,
    'is_billionaire': 2536,
    'active_event': 2540,
    'event_end_time': 2544,
    'tasks_unlocked': 2552,
    'achievements': 2568,
    'consecutive_days': 10968,
    'game_start_time': 10976,
    'total_play_time': 10984,
    'skill_points': 10988,
    'skills': 10992,
    'active_quests': 12472,
    'pets': 15512,
    'pet_count': 15872,
    'company_levels': 15880,
    'current_company_level': 17480,
    'inventory': 17484,
    'inventory_count': 35084,
    'stocks': 35088,
    'invested_stocks': 35368,
    'properties': 35392,
    'property_count': 36992,
    'friends': 37000,
    'friend_count': 37560,
    'daily_task_completions': 37564,
    'weekly_task_completions': 37568,
    'lifetime_task_completions': 37572,
    'total_income': 37576,
    'total_expenses': 37584,
    'current_season': 37592,
    'day_count': 37596,
    'prestige_level': 37600,
    'prestige_points': 37604,
    'has_premium': 37608,
    'premium_until': 37616,
    'achievement_points': 37624,
    'lottery_tickets': 37628,
    'mystery_keys': 37632,
    'boss_defeats': 37636,
    'festival_participations': 37640,
    'craft_count': 37644,
    
    # 新增字段 (v4)
    'hunger': 37648,           # 饱食度
    'health': 37652,          # 血量
    'spirit': 37656,          # 精神
    'is_hallucinating': 37660,# 幻觉状态
    'hallucination_end': 37664,# 幻觉结束时间
    'sudden_death_chance': 37672, # 猝死几率
    'hallu_count': 37676,     # 幻觉触发次数
    'has_phone': 37680,       # 是否有手机
    'in_chat_room': 37684,    # 是否在聊天室
    'chat_history': 37688,    # 聊天历史 (50*240=12000)
    'chat_message_count': 49688, # 消息数量
    'chatters': 49692,        # 聊天室人物 (8*144=1152)
    'chatter_count': 50844,   # 人物数量
}

# ==================== 字段类型定义 ====================
FIELD_TYPES = {
    # 8字节字段
    'kun_coins': 'q',
    'last_login': 'q',
    'event_end_time': 'q',
    'game_start_time': 'q',
    'total_income': 'q',
    'total_expenses': 'q',
    'premium_until': 'q',
    'hallucination_end': 'q',

    # 4字节字段
    'kun_exp': 'i',
    'level': 'i',
    'energy': 'i',
    'max_energy': 'i',
    'furniture_count': 'i',
    'active_event': 'i',
    'consecutive_days': 'i',
    'total_play_time': 'i',
    'skill_points': 'i',
    'current_company_level': 'i',
    'inventory_count': 'i',
    'property_count': 'i',
    'friend_count': 'i',
    'pet_count': 'i',
    'daily_task_completions': 'i',
    'weekly_task_completions': 'i',
    'lifetime_task_completions': 'i',
    'current_season': 'i',
    'day_count': 'i',
    'prestige_level': 'i',
    'prestige_points': 'i',
    'achievement_points': 'i',
    'lottery_tickets': 'i',
    'mystery_keys': 'i',
    'boss_defeats': 'i',
    'festival_participations': 'i',
    'craft_count': 'i',
    'hunger': 'i',           # 新增
    'health': 'i',           # 新增
    'spirit': 'i',           # 新增
    'sudden_death_chance': 'i',  # 新增
    'hallu_count': 'i',      # 新增
    'has_phone': 'i',        # 新增
    'in_chat_room': 'i',     # 新增
    'chat_message_count': 'i', # 新增
    'chatter_count': 'i',    # 新增

    # 1字节字段
    'is_billionaire': '?',
    'has_premium': '?',
    'is_hallucinating': '?', # 新增
}

# ==================== 可读写的基本字段列表 ====================
BASIC_FIELDS = [
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
    def __init__(self, save_file: str = "worker_sim_save.dat"):
        self.save_file = save_file
        self.data = None
        self.version = None

    def _player_offset(self, field: str) -> int:
        """获取字段在存档文件中的绝对偏移量"""
        if field not in OFFSETS:
            raise ValueError(f"未知字段: {field}")
        return SAVE_HEADER_SIZE + OFFSETS[field]

    def load_save(self) -> bool:
        """加载存档文件"""
        if not os.path.exists(self.save_file):
            print(f"错误: 存档文件 {self.save_file} 不存在")
            return False

        try:
            with open(self.save_file, 'rb') as f:
                self.data = bytearray(f.read())

            if len(self.data) < SAVE_HEADER_SIZE:
                print("错误: 存档文件损坏 (太小)")
                return False

            self.version = struct.unpack('<i', self.data[:SAVE_HEADER_SIZE])[0]
            expected_size = SAVE_HEADER_SIZE + PLAYER_SIZE

            if len(self.data) < expected_size:
                print(f"警告: 存档文件大小 ({len(self.data)}) 小于预期 ({expected_size})")
                print("  可能是旧版本存档，部分字段可能无法读取")

            if self.version != SAVE_VERSION:
                print(f"警告: 存档版本为 v{self.version}，当前为 v{SAVE_VERSION}")
                if self.version == 3:
                    print("  提示: v3存档可以继续使用，但新字段将使用默认值")

            print(f"成功加载存档，文件大小: {len(self.data)} 字节，版本: v{self.version}")
            return True
        except Exception as e:
            print(f"加载存档失败: {e}")
            return False

    def save_save(self) -> bool:
        """保存修改后的存档"""
        if self.data is None:
            print("错误: 没有加载存档数据")
            return False

        try:
            # 确保文件大小足够
            expected_size = SAVE_HEADER_SIZE + PLAYER_SIZE
            if len(self.data) < expected_size:
                self.data.extend(b'\x00' * (expected_size - len(self.data)))

            # 更新版本号
            struct.pack_into('<i', self.data, 0, SAVE_VERSION)

            # 备份原文件
            if os.path.exists(self.save_file):
                backup_file = self.save_file + '.bak'
                with open(self.save_file, 'rb') as src, open(backup_file, 'wb') as dst:
                    dst.write(src.read())
                print(f"已创建备份: {backup_file}")

            with open(self.save_file, 'wb') as f:
                f.write(self.data)

            print("存档已保存!")
            return True
        except Exception as e:
            print(f"保存存档失败: {e}")
            return False

    def read_value(self, field: str) -> Optional[Any]:
        """读取指定字段的值"""
        if self.data is None:
            print("错误: 请先加载存档")
            return None

        if field not in FIELD_TYPES:
            print(f"错误: 不支持读取字段 {field}")
            return None

        try:
            offset = self._player_offset(field)
            fmt = FIELD_TYPES[field]
            
            # 检查偏移量是否在数据范围内
            if offset + struct.calcsize('<' + fmt) > len(self.data):
                print(f"警告: 字段 {field} 超出数据范围，返回默认值")
                return 0 if fmt != '?' else False
            
            value = struct.unpack_from('<' + fmt, self.data, offset)[0]
            return value
        except Exception as e:
            print(f"读取字段 {field} 失败: {e}")
            return None

    def write_value(self, field: str, value: Any) -> bool:
        """写入指定字段的值"""
        if self.data is None:
            print("错误: 请先加载存档")
            return False

        if field not in FIELD_TYPES:
            print(f"错误: 不支持写入字段 {field}")
            return False

        try:
            fmt = FIELD_TYPES[field]

            # 类型转换
            if fmt == '?':
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
                value = int(value)
                # 值域限制
                if field in ['hunger', 'health', 'spirit']:
                    if value < 0: value = 0
                    if value > 100: value = 100

            offset = self._player_offset(field)
            
            # 检查偏移量
            if offset + struct.calcsize('<' + fmt) > len(self.data):
                # 扩展数据
                needed = offset + struct.calcsize('<' + fmt) - len(self.data)
                self.data.extend(b'\x00' * needed)
            
            packed = struct.pack('<' + fmt, value)
            self.data[offset:offset + len(packed)] = packed

            print(f"成功设置 {field} = {value}")
            return True
        except Exception as e:
            print(f"写入字段 {field} 失败: {e}")
            return False

    def read_furniture(self, index: int = 0) -> Optional[str]:
        """读取家具名称"""
        if index < 0 or index >= 50:
            print("错误: 家具索引必须在 0-49 之间")
            return None

        offset = self._player_offset('furniture') + index * 50
        try:
            if offset + 50 > len(self.data):
                return None
            name_bytes = self.data[offset:offset + 50]
            end = name_bytes.find(b'\0')
            if end != -1:
                name_bytes = name_bytes[:end]
            return name_bytes.decode('utf-8', errors='ignore')
        except Exception as e:
            return None

    def write_furniture(self, index: int, name: str) -> bool:
        """写入家具名称"""
        if index < 0 or index >= 50:
            print("错误: 家具索引必须在 0-49 之间")
            return False

        offset = self._player_offset('furniture') + index * 50
        try:
            name_bytes = name.encode('utf-8')[:49]
            if offset + 50 > len(self.data):
                needed = offset + 50 - len(self.data)
                self.data.extend(b'\x00' * needed)
            self.data[offset:offset + len(name_bytes)] = name_bytes
            self.data[offset + len(name_bytes):offset + 50] = b'\0' * (50 - len(name_bytes))
            print(f"成功设置家具[{index}] = {name}")
            return True
        except Exception as e:
            print(f"写入家具失败: {e}")
            return False

    def get_furniture_count(self) -> int:
        """获取家具数量"""
        count = self.read_value('furniture_count')
        return count if count is not None else 0

    def show_status(self):
        """显示当前状态"""
        if self.data is None:
            print("请先加载存档")
            return

        print("=" * 60)
        print("  当前存档状态")
        print("=" * 60)

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
            print("  存档修改器 v2.1 - 交互模式")
            print("=" * 60)
            print("  1. 显示当前状态")
            print("  2. 修改基本属性")
            print("  3. 修改家具")
            print("  4. 应用预设作弊")
            print("  5. 修改状态属性 (饱食度/血量/精神)")
            print("  6. 保存并退出")
            print("  7. 退出不保存")
            print("=" * 60)

            choice = input("  请选择 (1-7): ").strip()

            if choice == '1':
                self.show_status()

            elif choice == '2':
                print("\n  可修改的基本属性:")
                for i, (field, name) in enumerate(BASIC_FIELDS, 1):
                    value = self.read_value(field)
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
                if self.save_save():
                    print("  修改已保存!")
                break

            elif choice == '7':
                print("  修改已丢弃")
                break

            else:
                print("  无效选择")


def find_save_file() -> str:
    """自动查找存档文件"""
    possible_paths = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'worker_sim_save.dat'),
        'worker_sim_save.dat',
        os.path.join(os.path.dirname(os.path.abspath(__file__)), 'worker_sim_save.dat'),
        'kun_save.dat',  # 兼容旧名称
    ]
    for path in possible_paths:
        if os.path.exists(path):
            return os.path.normpath(path)
    return 'worker_sim_save.dat'


def main():
    print("=" * 60)
    print("  打工模拟器存档修改器 v2.1")
    print(f"  目标存档大小: {SAVE_HEADER_SIZE + PLAYER_SIZE} 字节 (v{SAVE_VERSION})")
    print("  支持新特性: 饱食度/血量/精神系统")
    print("=" * 60)

    save_file = find_save_file()
    if os.path.exists(save_file):
        print(f"  找到存档文件: {save_file}")
    else:
        print(f"  未找到默认存档文件")
        save_file = input("  请输入存档文件路径: ").strip()
        if not save_file:
            save_file = "worker_sim_save.dat"

    editor = KunSaveEditor(save_file)

    if not editor.load_save():
        print("  无法加载存档，创建新存档吗？ (y/n): ", end='')
        if input().strip().lower() == 'y':
            editor.data = bytearray(SAVE_HEADER_SIZE + PLAYER_SIZE)
            struct.pack_into('<i', editor.data, 0, SAVE_VERSION)
            print(f"  已创建新存档数据 ({len(editor.data)} 字节)")
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
