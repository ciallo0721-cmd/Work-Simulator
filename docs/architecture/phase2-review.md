# Phase 2 代码审查与技术评估报告

> **审查人**: 程基岩（游戏工程主管）  
> **审查日期**: 2026-07  
> **源码**: 打工模拟器.cpp（5589行，C/C++，单文件）  
> **引擎**: GCC/MinGW 控制台，SAVE_VERSION=7  

---

## 一、数据结构全景

### 1.1 核心结构体索引

| 结构体 | 位置(行) | 关键字段 | Phase 2 相关度 |
|--------|---------|---------|---------------|
| `Player` | 298-396 | kun_coins, day_count, pets[5], friends[10], chatters[8], chat_history[50], insider_profit, daily_time_slots, task_completion_counts[10] | **核心改点** |
| `Task` | 266-282 | base_income, required_skills[3], success_rate, completion_count, is_unlocked | **仅读取** |
| `Pet` | 206-216 | type, happiness, is_unlocked, bonus_multiplier | **需新增字段** |
| `Friend` | 257-264 | friendship_level, is_online, is_friend | **需新增字段** |
| `Chatter` | 154-159 | type, is_online, personality | 读取（不改） |
| `ChatMessage` | 146-151 | sender, content, type, timestamp | 读取（不改） |
| `ChatterType` | 137-143 | NORMAL/SCAMMER/BLACKMARKET/TONGCHENG | 可能需扩展 |
| `QuestType` | 113-119 | DAILY/WEEKLY/MAIN/SIDE/SPECIAL | **需扩展 WEEKLY_CHALLENGE** |
| `Quest` | 193-204 | type, requirement, reward, progress, deadline | **需复用** |
| `Stock` | 238-245 | current_price, volatility | 读取（不改） |
| `WorkMicroEvent` | 2847-2855 | task_id, title, description, coin_bonus | 读取（不改） |

### 1.2 Player 结构体 Phase 2 扩展风险矩阵

```
当前大小估算（粗略）:
  char player_name[50]            = 50
  5× money_t                      = 40
  多种 int/float                   ≈ 40 × 4 = 160
  char furniture[50][50]          = 2500
  Achievement[30]                 ≈ 30 × 270 = 8100
  Skill[10]                       ≈ 10 × 140 = 1400
  Quest[10]                       ≈ 10 × 365 = 3650
  Pet[5]                          ≈ 5 × 80 = 400
  CompanyLevel[10]                ≈ 10 × 150 = 1500
  InventoryItem[100]              ≈ 100 × 200 = 20000
  Stock[5]                        ≈ 5 × 70 = 350
  Property[10]                    ≈ 10 × 180 = 1800
  Friend[10]                      ≈ 10 × 65 = 650
  ChatMessage[50]                 ≈ 50 × 270 = 13500
  Chatter[8]                      ≈ 8 × 135 = 1080
  其他追踪字段                     ≈ 500
  ─────────────────────────────────────
  估算总计                        ≈ 55640 字节
  
  → sizeof(Player) ≈ 55-60KB，fwrite/fread 直接序列化
```

> **关键结论**：save/load 使用 `fwrite(&player, sizeof(Player), 1, file)` 整体序列化。
> **任何 Player 结构体的字段增加/删除/重排都会破坏存档兼容性。**

---

## 二、关键函数入口分析

### 2.1 直接参与 Phase 2 的函数

| 函数 | 行号 | 职责 | Phase 2 改动类型 |
|------|------|------|----------------|
| `do_task(task_index)` | 3105-3386 | 打工主逻辑：时间槽检查→难度选择→成功率计算→收入计算→微事件→宠物状态→结算 | **核心改动**：需注入宠物协助、好友组队加成 |
| `show_chat_room()` | 2433-2632 | 聊天室主循环：自动消息→消息显示→@交互→同意/拒绝 | **核心改动**：需注入隐藏机会触发逻辑 |
| `day_end()` | 3047-3102 | 每日结算：生活成本扣除→日报→睡眠恢复→季节更替→随机事件 | **改动**：需添加每周挑战检查与结算 |
| `show_daily_report()` | 3006-3045 | 日报展示：收入→活动→状态 | **轻改动**：增加挑战进度展示 |
| `trigger_work_micro_event(task_index)` | 2884-3004 | 打工微事件：40%触发→事件池筛选→分支选择→效果应用 | **可复用模式** |
| `ai_chatter_response(chatter_index, msg)` | 1545-1691 | AI对话：基于角色类型的规则式响应 | **扩展**：增加隐藏机会线索注入 |
| `process_od_offer(chatter_index)` | 1693-1749 | 黑产OD提议处理 | **可复用模式** |
| `process_tongcheng_offer(chatter_index)` | 1750-1821 | 同城邀约处理 | **可复用模式** |
| `save_game()` | 1787-1800 | fwrite 整个 Player 结构体 | **不改**，但Player结构体变化影响存档 |
| `load_game()` | 1802-1817 | fread + SAVE_VERSION 校验 | **版本号需升级**（如果改Player结构体） |

### 2.2 间接相关的函数

| 函数 | 影响 |
|------|------|
| `show_main_menu()` (5098+) | 如新增菜单入口（每周挑战）需改 |
| `show_phone_menu()` (2394+) | 聊天室入口不变 |
| `update_stock_prices()` | 内幕消息需影响股价 |
| `buy_stock()` | 内幕消息可能影响购买行为 |
| `check_achievements()` | 新成就追踪 |
| `init_player()` | 新字段初始化 |

---

## 三、Phase 2 改动热点分析

### 3.1 Feature 1：聊天室隐藏机会 (P0)

**改动区域**：
1. **`show_chat_room()`** (~200行)：在 NPC 自动发言和 @交互中注入"隐藏机会"线索消息
2. **`ai_chatter_response()`** (~146行)：扩展现有规则匹配，让特定 NPC 在不同语境下"透露"机会
3. **`Player` 结构体**：新增字段追踪已触发的隐藏机会（如 `int hidden_opportunity_id`、`time_t opportunity_expiry`）
4. **新增函数**：`process_hidden_opportunity()` —— 处理机会的接受/拒绝/过期逻辑

**复用已有模式**：
- `process_od_offer()` / `process_tongcheng_offer()` 的"同意/拒绝"交互模式完全可复用
- `waiting_for_agreement` 状态机机制可扩展

### 3.2 Feature 2：宠物协助打工 (P1)

**改动区域**：
1. **`do_task()`** 的成功率/收入计算段（行 3214-3264）：注入宠物协助检查
   - 已有 `player.pets[i].bonus_multiplier` 逻辑(行3249-3253)，可直接扩展
2. **`Pet` 结构体**：新增 `bool can_assist`、`int assist_cooldown`（协助冷却）
3. **新增菜单**：打工前询问"是否让宠物协助？"
4. **`init_pets()`**：初始化协助能力

**复用已有模式**：
- `do_task()` 行 3249-3253 已有基础宠物加成逻辑
- `Pet::bonus_multiplier` 字段可直接复用

### 3.3 Feature 3：好友组队 (P2)

**改动区域**：
1. **`do_task()`**：在难度选择后、成功率计算前，插入组队选择逻辑
2. **`Friend` 结构体**：新增 `int team_cooldown`、`bool can_team`、`float team_bonus`
3. **`Player` 结构体**：新增 `int teamed_friend_index`、`int daily_team_count`
4. **`show_friends()`** / **`trade_with_friend()`**：新增组队入口

**依赖**：好友系统（show_friends 行 4261 已存在），但好友交互目前只有交易/赠礼。

### 3.4 Feature 4：每周挑战 (P2)

**改动区域**：
1. **`Player` 结构体**：新增 `Quest weekly_challenge`、`int week_count`、`int weekly_challenge_progress`
2. **`day_end()`**：新增每周日检查 → 结算旧挑战 → 生成新挑战
3. **`show_main_menu()`** / **`show_daily_report()`**：展示挑战进度
4. **新增函数**：`generate_weekly_challenge()`、`check_weekly_challenge()`、`claim_weekly_reward()`
5. **`do_task()`**：更新挑战进度（如"本周完成N次打工"）

**依赖**：时间系统（`player.day_count` 已存在，行 340）

---

## 四、代码质量评估

### 4.1 可复用的设计模式

| 模式 | 位置 | 复用价值 |
|------|------|---------|
| **"同意/拒绝"交互状态机** | `show_chat_room()` waiting_for_agreement | ★★★★★ 核心交互模式，聊天室隐藏机会完全复用 |
| **微事件触发+筛选+分支** | `trigger_work_micro_event()` | ★★★★☆ 事件系统可扩展为通用模式 |
| **规则式NPC响应** | `ai_chatter_response()` | ★★★★☆ 可扩展添加"线索泄露"响应 |
| **时间槽每日重置** | `day_end()` + `do_task()` l3141-3148 | ★★★☆☆ 每周挑战的时间触发可复用 |
| **Quest追踪模式** | `Quest::progress` + 任务完成时回调 | ★★★★☆ 每周挑战可直接复用Quest结构 |
| **特殊提议处理** | `process_od_offer()` / `process_tongcheng_offer()` | ★★★★★ 隐藏机会处理模式完全复用 |

### 4.2 技术债务

| 问题 | 严重度 | 影响 | 建议 |
|------|--------|------|------|
| **单文件架构** | 🔴 高 | 5589行全部在一个cpp文件，任何修改都有合并冲突风险 | Phase 2后逐步拆分模块（chat_room.c, task.c, etc.） |
| **fwrite整体序列化** | 🔴 高 | Player结构体任何改动都破坏存档，SAVE_VERSION跳号 | 添加Player字段时必须：①SAVE_VERSION++ ②旧版load逻辑 ③迁移函数 |
| **全局可变状态** | 🟡 中 | `Player player` 是全局变量，所有函数直接修改，无封装 | 暂可接受（控制台程序），但需注意测试隔离 |
| **硬编码数组大小** | 🟡 中 | `#define MAX_xxx` 散落各处，MAX_TASKS=10, MAX_CHATTERS=8 | 不改，维持稳定性 |
| **缺乏自动化测试** | 🟡 中 | 无测试框架，所有验证靠人工 | Phase 2 至少添加关键路径的手动测试清单 |
| **幻数(魔法数字)** | 🟢 低 | 如 `rand() % 100 < 35`(聊天室自动发言概率) | 不影响新功能，逐步常量化 |
| **混合中英文标识符** | 🟢 低 | 函数名英文，数据中文 | 接受现有风格 |

### 4.3 编码规范评估

**优点**：
- K&R风格大括号一致
- 函数间有清晰的 `//=====` 分隔注释
- 枚举类型充分使用（enum而非裸int）
- ANSI颜色宏统一管理

**问题**：
- 部分注释使用emoji，编译时OK但可能在某些终端乱码
- `sprintf` vs `snprintf` 混用（安全风险低，因为缓冲区通常够大）

---

## 五、Phase 2 各Feature改动量估算

| Feature | 新增代码行 | 修改代码行 | 新增结构体字段 | 风险等级 |
|---------|-----------|-----------|--------------|---------|
| 聊天室隐藏机会 | ~150 | ~80 | Player: +8 bytes | 低 |
| 宠物协助打工 | ~100 | ~50 | Pet: +8 bytes, Player: +4 bytes | 低 |
| 好友组队 | ~200 | ~80 | Friend: +12 bytes, Player: +8 bytes | 中 |
| 每周挑战 | ~250 | ~60 | Player: +~400 bytes (Quest复用) | 中 |
| **合计** | **~700** | **~270** | Player: +~420 bytes | — |

---

## 六、存档兼容性影响汇总

当前 Player 结构体约 55-60KB，采用 `fwrite(&player, sizeof(Player), 1, file)` 整体序列化。

**Phase 2 新增字段对 Player 大小的影响**：

| 新字段 | 类型 | 大小 |
|--------|------|------|
| hidden_opportunity_id | int | 4 |
| opportunity_expiry | time_t | 8 |
| opportuniry_type | int | 4 |
| teamed_friend_index | int | 4 |
| daily_team_count | int | 4 |
| week_count | int | 4 |
| weekly_challenge | Quest | ~365 |
| weekly_challenge_progress | int | 4 |
| pet_assist_cooldown[5] | int[5] | 20 |
| **合计** | | **~417 bytes** |

> **强烈建议**：所有 Phase 2 新字段统一加在 Player 结构体末尾，SAVE_VERSION 升级到 8，并实现 load 时的版本迁移逻辑。
> 目前 `load_game()` 只在版本完全匹配时才加载，不匹配直接新游戏——这是**破坏性行为**，应考虑添加迁移路径。

---

## 七、关键架构建议

### 7.1 存档迁移方案（推荐）
```c
#define SAVE_VERSION 8  // Phase 2

void load_game() {
    FILE* file = fopen(SAVE_FILE, "rb");
    if (file) {
        int version;
        fread(&version, sizeof(int), 1, file);
        if (version == SAVE_VERSION) {
            fread(&player, sizeof(Player), 1, file);
        } else if (version == 7) {
            // 读取旧版Player，然后初始化新字段
            fread(&player, sizeof(Player_v7), 1, file);
            init_phase2_fields();  // 初始化v8新增的所有字段
            save_game();           // 立即存为新版本
            print_colored(COLOR_GREEN, "存档已升级到v8！");
        }
        fclose(file);
    }
}
```

### 7.2 实现顺序建议
1. **先做每周挑战**（最独立，不依赖其他系统）
2. **再做聊天室隐藏机会**（复用已有同意/拒绝模式）
3. **然后宠物协助**（do_task已有基础宠物加成）
4. **最后好友组队**（依赖好友系统交互，复杂度最高）

### 7.3 测试策略
- 每个Story实现后立即对照GDD验收标准手动测试
- 存档兼容性：在每个Story完成后进行"旧档升级"测试
- 边界测试：时间槽用完后触发/宠物已全部解锁/好友满员时的行为
