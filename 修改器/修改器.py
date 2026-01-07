#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
打工模拟器存档修改器
支持修改 kun_save.dat 文件
"""

import struct
import os
import sys
from typing import Dict, Any, Optional

class KunSaveEditor:
    def __init__(self, save_file: str = "kun_save.dat"):
        """
        初始化存档编辑器
        
        Args:
            save_file: 存档文件路径
        """
        self.save_file = save_file
        self.data = None
        self.player_struct = None
        
        # Player结构体布局（基于代码分析）
        # 注意：实际偏移可能需要根据编译器和平台调整
        self.offsets = {
            'kun_coins': 0,      # int
            'kun_exp': 4,        # int
            'level': 8,          # int
            'energy': 12,        # int
            'max_energy': 16,    # int
            'last_login': 20,    # time_t (8字节)
            'furniture_count': 28,  # int
            'is_billionaire': 32,   # bool (1字节)
            'active_event': 33,     # EventType (int)
            'event_end_time': 37,   # time_t (8字节)
            'consecutive_days': 45, # int
            'game_start_time': 49,  # time_t (8字节)
            'total_play_time': 57,  # int
            'skill_points': 61,     # int
            'current_company_level': 65,  # int
            'inventory_count': 69,  # int
            'property_count': 73,   # int
            'friend_count': 77,     # int
            'pet_count': 81,        # int
            'daily_task_completions': 85,  # int
            'weekly_task_completions': 89,  # int
            'lifetime_task_completions': 93,  # int
            'total_income': 97,     # int
            'total_expenses': 101,  # int
            'current_season': 105,  # Season (int)
            'day_count': 109,       # int
            'prestige_level': 113,  # int
            'prestige_points': 117, # int
            'has_premium': 121,     # bool (1字节)
            'premium_until': 125,   # time_t (8字节)
            'achievement_points': 133,  # int
            'lottery_tickets': 137,  # int
            'mystery_keys': 141,    # int
            'boss_defeats': 145,    # int
            'festival_participations': 149,  # int
            
            # 家具数组在153字节开始，每个50字节，共50个
            'furniture_start': 153,
            
            # 任务解锁状态数组
            'tasks_unlocked_start': 2653,  # bool[15]
            
            # 成就数组
            'achievements_start': 2668,  # 每个成就结构体大小
        }
        
        # 字段类型映射
        self.types = {
            'int': 'i',      # 4字节整数
            'bool': '?',     # 1字节布尔
            'time_t': 'q',   # 8字节长整数
        }

    def load_save(self) -> bool:
        """
        加载存档文件
        
        Returns:
            bool: 是否成功加载
        """
        if not os.path.exists(self.save_file):
            print(f"错误: 存档文件 {self.save_file} 不存在")
            return False
        
        try:
            with open(self.save_file, 'rb') as f:
                self.data = bytearray(f.read())
            
            print(f"成功加载存档，文件大小: {len(self.data)} 字节")
            return True
        except Exception as e:
            print(f"加载存档失败: {e}")
            return False

    def save_save(self) -> bool:
        """
        保存修改后的存档
        
        Returns:
            bool: 是否成功保存
        """
        if self.data is None:
            print("错误: 没有加载存档数据")
            return False
        
        try:
            # 备份原文件
            if os.path.exists(self.save_file):
                backup_file = self.save_file + '.bak'
                with open(self.save_file, 'rb') as src, open(backup_file, 'wb') as dst:
                    dst.write(src.read())
                print(f"已创建备份: {backup_file}")
            
            # 写入新数据
            with open(self.save_file, 'wb') as f:
                f.write(self.data)
            
            print("存档已保存！")
            return True
        except Exception as e:
            print(f"保存存档失败: {e}")
            return False

    def read_value(self, field: str) -> Optional[Any]:
        """
        读取指定字段的值
        
        Args:
            field: 字段名
            
        Returns:
            字段值，失败返回None
        """
        if field not in self.offsets:
            print(f"错误: 未知字段 {field}")
            return None
        
        if self.data is None:
            print("错误: 请先加载存档")
            return None
        
        offset = self.offsets[field]
        
        # 确定类型
        if field in ['kun_coins', 'kun_exp', 'level', 'energy', 'max_energy',
                    'furniture_count', 'consecutive_days', 'total_play_time',
                    'skill_points', 'current_company_level', 'inventory_count',
                    'property_count', 'friend_count', 'pet_count', 
                    'daily_task_completions', 'weekly_task_completions',
                    'lifetime_task_completions', 'total_income', 'total_expenses',
                    'day_count', 'prestige_level', 'prestige_points',
                    'achievement_points', 'lottery_tickets', 'mystery_keys',
                    'boss_defeats', 'festival_participations']:
            fmt = 'i'  # int
            size = 4
        elif field in ['is_billionaire', 'has_premium']:
            fmt = '?'  # bool
            size = 1
        elif field in ['last_login', 'event_end_time', 'game_start_time', 'premium_until']:
            fmt = 'q'  # time_t (long long)
            size = 8
        elif field in ['active_event', 'current_season']:
            fmt = 'i'  # enum (作为int处理)
            size = 4
        else:
            print(f"错误: 未知字段类型 {field}")
            return None
        
        try:
            value = struct.unpack(fmt, self.data[offset:offset+size])[0]
            return value
        except Exception as e:
            print(f"读取字段 {field} 失败: {e}")
            return None

    def write_value(self, field: str, value: Any) -> bool:
        """
        写入指定字段的值
        
        Args:
            field: 字段名
            value: 要写入的值
            
        Returns:
            bool: 是否成功
        """
        if field not in self.offsets:
            print(f"错误: 未知字段 {field}")
            return False
        
        if self.data is None:
            print("错误: 请先加载存档")
            return False
        
        offset = self.offsets[field]
        
        try:
            # 确定类型和格式
            if field in ['kun_coins', 'kun_exp', 'level', 'energy', 'max_energy',
                        'furniture_count', 'consecutive_days', 'total_play_time',
                        'skill_points', 'current_company_level', 'inventory_count',
                        'property_count', 'friend_count', 'pet_count', 
                        'daily_task_completions', 'weekly_task_completions',
                        'lifetime_task_completions', 'total_income', 'total_expenses',
                        'day_count', 'prestige_level', 'prestige_points',
                        'achievement_points', 'lottery_tickets', 'mystery_keys',
                        'boss_defeats', 'festival_participations']:
                fmt = 'i'
                if not isinstance(value, int):
                    value = int(value)
            elif field in ['is_billionaire', 'has_premium']:
                fmt = '?'
                if isinstance(value, str):
                    value = value.lower() in ['true', '1', 'yes', 'y']
                else:
                    value = bool(value)
            elif field in ['last_login', 'event_end_time', 'game_start_time', 'premium_until']:
                fmt = 'q'
                if value == 0:
                    value = 0
                elif isinstance(value, str) and value.lower() == 'now':
                    import time
                    value = int(time.time())
                else:
                    value = int(value)
            elif field in ['active_event', 'current_season']:
                fmt = 'i'
                value = int(value)
            else:
                print(f"错误: 未知字段类型 {field}")
                return False
            
            # 写入数据
            packed = struct.pack(fmt, value)
            self.data[offset:offset+len(packed)] = packed
            
            print(f"成功设置 {field} = {value}")
            return True
            
        except Exception as e:
            print(f"写入字段 {field} 失败: {e}")
            return False

    def read_furniture(self, index: int = 0) -> Optional[str]:
        """
        读取家具名称
        
        Args:
            index: 家具索引 (0-49)
            
        Returns:
            家具名称，失败返回None
        """
        if index < 0 or index >= 50:
            print(f"错误: 家具索引必须在 0-49 之间")
            return None
        
        offset = self.offsets['furniture_start'] + index * 50
        
        try:
            # 读取50字节，截断到第一个\0
            name_bytes = self.data[offset:offset+50]
            end = name_bytes.find(b'\0')
            if end != -1:
                name_bytes = name_bytes[:end]
            return name_bytes.decode('utf-8', errors='ignore')
        except Exception as e:
            print(f"读取家具失败: {e}")
            return None

    def write_furniture(self, index: int, name: str) -> bool:
        """
        写入家具名称
        
        Args:
            index: 家具索引 (0-49)
            name: 家具名称
            
        Returns:
            bool: 是否成功
        """
        if index < 0 or index >= 50:
            print(f"错误: 家具索引必须在 0-49 之间")
            return False
        
        offset = self.offsets['furniture_start'] + index * 50
        
        try:
            # 编码名称，最多49字节 + 1字节\0
            name_bytes = name.encode('utf-8')[:49]
            # 填充剩余空间
            total_length = 50
            padding = b'\0' * (total_length - len(name_bytes) - 1)
            self.data[offset:offset+len(name_bytes)] = name_bytes
            self.data[offset+len(name_bytes)] = 0  # 终止符
            if padding:
                self.data[offset+len(name_bytes)+1:offset+total_length] = padding
            
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
        print("当前存档状态:")
        print("=" * 60)
        
        fields = [
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
            ('成就点', 'achievement_points'),
            ('彩票', 'lottery_tickets'),
            ('神秘钥匙', 'mystery_keys'),
        ]
        
        for label, field in fields:
            value = self.read_value(field)
            if value is not None:
                print(f"{label:10}: {value}")
        
        # 显示家具
        count = self.get_furniture_count()
        if count > 0:
            print(f"\n家具 ({count}件):")
            for i in range(min(count, 5)):  # 只显示前5个
                name = self.read_furniture(i)
                if name:
                    print(f"  [{i}] {name}")
            if count > 5:
                print(f"  ... 还有 {count-5} 件家具")
        
        print("=" * 60)

    def preset_cheats(self):
        """预设作弊选项"""
        presets = {
            '1': ('无限金钱', {'kun_coins': 999999999}),
            '2': ('满级', {'level': 100, 'kun_exp': 0}),
            '3': ('无限能量', {'energy': 9999, 'max_energy': 9999}),
            '4': ('全技能点', {'skill_points': 999}),
            '5': ('解锁所有', {
                'current_company_level': 9,
                'prestige_level': 5,
                'achievement_points': 9999,
                'lottery_tickets': 99,
                'mystery_keys': 99,
            }),
            '6': ('重置存档', {
                'level': 1,
                'kun_coins': 100,
                'kun_exp': 0,
                'energy': 100,
                'max_energy': 100,
                'skill_points': 0,
            }),
        }
        
        print("\n预设作弊:")
        for key, (name, _) in presets.items():
            print(f"  {key}. {name}")
        
        choice = input("\n选择预设 (1-6) 或直接回车跳过: ").strip()
        if choice in presets:
            name, values = presets[choice]
            print(f"\n应用预设: {name}")
            for field, value in values.items():
                self.write_value(field, value)
            return True
        return False

    def interactive_mode(self):
        """交互式修改模式"""
        while True:
            print("\n" + "=" * 60)
            print("存档修改器 - 交互模式")
            print("=" * 60)
            print("1. 显示当前状态")
            print("2. 修改基本属性")
            print("3. 修改家具")
            print("4. 应用预设作弊")
            print("5. 保存并退出")
            print("6. 退出不保存")
            print("=" * 60)
            
            choice = input("请选择 (1-6): ").strip()
            
            if choice == '1':
                self.show_status()
                
            elif choice == '2':
                print("\n可修改的基本属性:")
                basic_fields = [
                    ('kun_coins', '坤币', 'int'),
                    ('level', '等级', 'int'),
                    ('kun_exp', '经验', 'int'),
                    ('energy', '能量', 'int'),
                    ('max_energy', '最大能量', 'int'),
                    ('skill_points', '技能点', 'int'),
                    ('lifetime_task_completions', '总任务完成', 'int'),
                    ('current_company_level', '公司等级', 'int'),
                    ('prestige_level', '声望等级', 'int'),
                    ('achievement_points', '成就点', 'int'),
                    ('lottery_tickets', '彩票', 'int'),
                    ('mystery_keys', '神秘钥匙', 'int'),
                ]
                
                for i, (field, name, _) in enumerate(basic_fields, 1):
                    value = self.read_value(field)
                    print(f"{i:2}. {name:10} ({field:20}): {value}")
                
                print("\n输入要修改的编号 (或直接输入字段名): ", end='')
                field_input = input().strip()
                
                if field_input.isdigit():
                    idx = int(field_input) - 1
                    if 0 <= idx < len(basic_fields):
                        field = basic_fields[idx][0]
                    else:
                        print("编号无效")
                        continue
                else:
                    field = field_input
                
                if field in [f[0] for f in basic_fields]:
                    current = self.read_value(field)
                    new_value = input(f"当前值: {current}，输入新值: ").strip()
                    if new_value:
                        self.write_value(field, new_value)
                else:
                    print("字段名无效")
                    
            elif choice == '3':
                count = self.get_furniture_count()
                print(f"\n当前有 {count} 件家具")
                
                print("1. 查看家具")
                print("2. 添加家具")
                print("3. 修改家具数量")
                sub_choice = input("请选择 (1-3): ").strip()
                
                if sub_choice == '1':
                    for i in range(min(count, 20)):
                        name = self.read_furniture(i)
                        if name:
                            print(f"  [{i:2}] {name}")
                    if count > 20:
                        print(f"  ... 还有 {count-20} 件家具")
                        
                elif sub_choice == '2':
                    name = input("输入家具名称: ").strip()
                    if name:
                        # 找到第一个空位或添加到末尾
                        for i in range(50):
                            existing = self.read_furniture(i)
                            if not existing or existing.isspace():
                                self.write_furniture(i, name)
                                # 更新家具计数
                                new_count = max(count, i+1)
                                self.write_value('furniture_count', new_count)
                                print(f"已添加家具到位置 {i}")
                                break
                        else:
                            print("家具已满!")
                            
                elif sub_choice == '3':
                    new_count = input(f"当前家具数量: {count}，输入新数量 (0-50): ").strip()
                    if new_count.isdigit():
                        new_count = int(new_count)
                        if 0 <= new_count <= 50:
                            self.write_value('furniture_count', new_count)
                            print(f"家具数量已更新为 {new_count}")
                        else:
                            print("数量必须在 0-50 之间")
                    else:
                        print("请输入数字")
                        
            elif choice == '4':
                self.preset_cheats()
                
            elif choice == '5':
                if self.save_save():
                    print("修改已保存!")
                break
                
            elif choice == '6':
                print("修改已丢弃")
                break
                
            else:
                print("无效选择")


def main():
    """主函数"""
    print("=" * 60)
    print("打工模拟器存档修改器 v1.0")
    print("=" * 60)
    
    # 自动查找存档文件
    save_file = "kun_save.dat"
    possible_paths = [
        "kun_save.dat",
        "./kun_save.dat",
        "../kun_save.dat",
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            save_file = path
            print(f"找到存档文件: {save_file}")
            break
    
    # 如果没找到，询问用户
    if not os.path.exists(save_file):
        print(f"未找到默认存档文件")
        save_file = input("请输入存档文件路径: ").strip()
        if not save_file:
            save_file = "kun_save.dat"
    
    # 创建编辑器实例
    editor = KunSaveEditor(save_file)
    
    # 加载存档
    if not editor.load_save():
        print("无法加载存档，创建新存档吗？ (y/n): ", end='')
        if input().strip().lower() == 'y':
            # 初始化空数据（需要知道Player结构体大小）
            # 这里假设结构体大小为4000字节（根据代码估算）
            editor.data = bytearray(4000)
            print("已创建新存档数据")
        else:
            print("退出程序")
            return
    
    # 进入交互模式
    editor.interactive_mode()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
    except Exception as e:
        print(f"\n程序出错: {e}")
        import traceback
        traceback.print_exc()
