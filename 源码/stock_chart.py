# -*- coding: utf-8 -*-
"""
打工模拟器 - 股票折线统计图
读取 stock_history.dat 并显示ASCII折线图
"""

import os
import struct
import sys
import time

STOCK_FILE = "save/stock_history.dat"
STOCK_NAMES = [
    "坤坤科技", "外卖速递", "坤城地产", "坤能集团", "星光传媒",
    "坤城银行", "大江医药", "天虹通信", "龙腾制造", "碧水环保",
    "烈火能源", "青云教育", "金盾安保", "风驰物流", "华美百货",
    "北斗导航", "南山旅游", "星辰半导体", "绿野农业", "白鲸渔业",
    "九鼎建筑", "天机软件", "凤凰传媒", "云梦游戏", "紫金矿业"
]

def read_history():
    """读取股票历史数据，返回 [[prices...], ...] 每支股票一个列表"""
    if not os.path.exists(STOCK_FILE):
        return None
    
    with open(STOCK_FILE, "rb") as f:
        data = f.read()
    
    records = []
    offset = 0
    record_size = struct.calcsize("=q25i")  # time_t(8bytes) + 25*int(4bytes)
    
    while offset + record_size <= len(data):
        ts, *prices = struct.unpack_from(f"=q25i", data, offset)
        # v5.1：内部单位为“分”，显示时换算成元
        prices = [round(v / 100.0, 2) for v in prices]
        records.append(prices)
        offset += record_size
    
    # 转置：每支股票的所有历史价格
    if not records:
        return None
    
    history = []
    for i in range(25):
        history.append([r[i] for r in records])
    return history


def draw_chart(prices, name, width=50, height=16):
    """画一支股票的ASCII折线图"""
    if len(prices) < 2:
        return f"  {name}：数据不足\n"
    
    # 取最近 width 个点
    pts = prices[-width:]
    n = len(pts)
    min_p = min(pts)
    max_p = max(pts)
    if max_p == min_p:
        max_p = min_p + 1
    
    # 构建折线
    chart = [[' ' for _ in range(n + 2)] for _ in range(height + 2)]
    
    # 画边框
    for x in range(n + 2):
        chart[height + 1][x] = '─'
    for y in range(height + 2):
        chart[y][0] = '│'
        chart[y][n + 1] = '│'
    chart[height + 1][0] = '└'
    chart[height + 1][n + 1] = '┘'
    
    # 画数据点
    for x in range(n):
        val = pts[x]
        y = int((val - min_p) / (max_p - min_p) * (height - 1))
        y = height - 1 - y  # 上下翻转
        chart[y][x + 1] = '●'
        
        # 连线
        if x > 0:
            prev_y = int((pts[x-1] - min_p) / (max_p - min_p) * (height - 1))
            prev_y = height - 1 - prev_y
            if prev_y != y:
                mid_y = (prev_y + y) // 2
                if chart[mid_y][x] == ' ':
                    chart[mid_y][x] = '│'
    
    # 标价格刻度
    chart[0][n // 2] = ' '
    chart[height][n // 2] = ' '
    
    lines = [f"  {name}  ( {min_p} ~~ {max_p} )"]
    for y in range(height + 2):
        line = ''.join(chart[y])
        lines.append(line)
    
    # 标数字
    lines.append(f"  最低: {min_p}    最高: {max_p}    最新: {pts[-1]}")
    
    return '\n'.join(lines)


def main():
    os.system("title 股票行情图 - 打工模拟器")
    
    # 切换到程序目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)  # 回到项目根目录
    os.chdir(parent_dir)
    
    while True:
        history = read_history()
        
        if history is None:
            print("\n  ? 暂无股票数据，请先进入游戏股票市场")
        else:
            print("\n" + "=" * 70)
            print("  ? 坤城证券交易所 - 实时行情图")
            print("=" * 70)
            
            # 显示前10支股票的图表
            for i in range(min(10, 25)):
                if i < len(history) and len(history[i]) >= 2:
                    print(draw_chart(history[i], STOCK_NAMES[i]))
                    print()
            
            if len(history[0]) >= 2:
                latest = history[0][-1]
            
            print("\n" + "-" * 70)
        
        print("\n[ 按 R 刷新 | 按 Q 退出 ] ", end="")
        c = input().strip().lower()
        if c == 'q':
            break
        # R 刷新或任意键继续
        os.system("cls" if os.name == "nt" else "clear")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n  ? 行情图已关闭")
    except Exception as e:
        print(f"\n  ! 错误: {e}")
        input("按回车退出...")
