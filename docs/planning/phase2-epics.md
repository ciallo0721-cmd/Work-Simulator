# Phase 2 Epic/Story 拆分计划

> **产出者**: 程基岩（游戏工程主管）  
> **GDD参考**: 《打工模拟器 v4.0 玩法改造设计》第4节（系统互联）、第8节（实现路线图）  
> **源码基线**: 打工模拟器.cpp v4.0, SAVE_VERSION=7, 5589行  

---

## 实现顺序总览

```
顺序: Epic 4 → Epic 1 → Epic 2 → Epic 3
原因: 每周挑战(独立) → 聊天室隐藏机会(复用同意/拒绝) → 宠物协助(已有基础) → 好友组队(最复杂)
            ↓                    ↓                        ↓                    ↓
       [无依赖]            [依赖Epic4经验]          [依赖do_task模式]    [依赖好友系统]
```

**并行可能性**: Epic 1 和 Epic 2 可在 Epic 4 完成后并行开发（它们只读 do_task，互不冲突）

---

## Epic 1：聊天室隐藏机会 (P0)

**GDD引用**: 第4.2节 - "交互3：聊天室→隐藏机会"  
**改动量估算**: ~230行（新增~150 + 修改~80）  
**风险等级**: 🟢 低  
**存档影响**: Player +16 bytes，SAVE_VERSION → 8

### Story 1.1：NPC线索消息注入
- **Story ID**: CHAT-OPP-001
- **描述**: 扩展 NPC 自动发言池和 @交互对话，加入"股票内幕"和"限时打工机会"的暗示消息
- **改动量**: ~60行（小）
- **GDD需求ID**: 4.2-交互3（聊天室消息类型表）
- **具体实现**:
  1. 在 `show_chat_room()` 的 `auto_messages[]` 数组中添加4-6条线索型消息（如"听说XX股票要涨"、"开发区那边在招临时工"）
  2. 在 `ai_chatter_response()` 中为骗子类型（投资大师/内部消息）添加"透露内幕"响应分支
  3. 为黑产类型添加"限时活儿"响应
- **验收标准**:
  - [ ] 聊天室中 NPC 自发消息包含至少4种线索
  - [ ] @投资大师或@内部消息 谈论"股票"时，有概率触发"透露消息"
  - [ ] @神秘人 谈论"赚钱"时，有概率触发"限时打工线索"
- **测试证据路径**: `tests/chat_opportunity/test_story_1_1.md`
- **风险**: 低。仅扩展已有消息池和响应分支。
- **依赖**: 无

### Story 1.2：隐藏机会数据结构与Player字段
- **Story ID**: CHAT-OPP-002
- **描述**: 在 Player 结构体末尾新增隐藏机会追踪字段，实现机会的"生成→过期"生命周期管理
- **改动量**: ~40行（小）
- **GDD需求ID**: 4.2-交互3（分辨真假需智力检定）
- **具体实现**:
  1. Player 新增字段：`int pending_opportunity_type`（0=无,1=股票内幕,2=限时打工,3=商城折扣）、`int pending_opportunity_param`（关联参数：股票索引/任务ID/商城物品ID）、`time_t opportunity_expiry`（过期时间，当日结束失效）
  2. `init_player()` 中初始化这些字段
  3. SAVE_VERSION → 8，`load_game()` 添加 v7→v8 迁移
- **验收标准**:
  - [ ] 存档从v7升级到v8后，新字段正确初始化为0
  - [ ] 新存档正确保存/加载隐藏机会状态
  - [ ] 机会过期（超过当天）后自动清除
- **测试证据路径**: `tests/chat_opportunity/test_story_1_2.md`
- **风险**: 🔴 高（存档兼容性！）。必须实现版本迁移。
- **依赖**: Story 1.1

### Story 1.3：股票内幕机会
- **Story ID**: CHAT-OPP-003
- **描述**: 实现"股票内幕"隐藏机会：NPC透露某支股票会涨/跌，玩家可选择相信/不信，结果有真有假
- **改动量**: ~80行（中等）
- **GDD需求ID**: 4.2-交互3（"听说XX股票要涨"→该股票当日波动率+30%）
- **具体实现**:
  1. 新增函数 `process_stock_tip(chatter_index, stock_index, is_true)`：在聊天室触发"内幕消息"
  2. 消息格式：`"%s悄悄告诉你：「%s的股票明天必涨，信我！」"`
  3. 提供选项：1.相信（购买该股票，波动率变化） 2.不信 3.反向操作
  4. 真消息概率 = 40% + 智力检定（`SKILL_INTELLIGENCE`等级×5%），最高80%
  5. 若为真消息：该股票 price_change 方向偏正面；若为假：偏负面
  6. `update_stock_prices()` 中读取 pending_opportunity 状态应用波动加成
- **验收标准**:
  - [ ] 聊天室中"投资大师"或"内部消息"NPC触发内幕消息
  - [ ] 玩家可选择相信/不信/反向操作
  - [ ] 高智力玩家识别真假的成功率明显更高
  - [ ] 相信假消息会导致亏损
  - [ ] 成就追踪 `player.insider_profit` 正确累加
- **测试证据路径**: `tests/chat_opportunity/test_story_1_3.md`
- **风险**: 🟡 中。涉及股票系统改动，需确保不破坏现有股票逻辑。
- **依赖**: Story 1.2

### Story 1.4：限时打工机会
- **Story ID**: CHAT-OPP-004
- **描述**: 实现"限时打工"隐藏机会：NPC透露临时高收益任务，仅当日有效
- **改动量**: ~50行（小）
- **GDD需求ID**: 4.2-交互3（"XX公司在招人"→临时解锁高收益任务）
- **具体实现**:
  1. 重新定义 Task 数组的第9位（索引8，当前未使用或可覆盖的低优先级任务）为"限时任务"
  2. 隐藏机会触发时：设置 `tasks[8]` 为高收益临时任务（收入=玩家等级×100 × 1.5），`tasks[8].is_unlocked = true`
  3. day_end() 时自动重置：`tasks[8].is_unlocked = false`
  4. 新增函数 `activate_timed_task(income, hours, name)` 动态设置限时任务参数
- **验收标准**:
  - [ ] 聊天室获得机会后，打工菜单出现限时高收益任务
  - [ ] 限时任务仅当日有效（day_end后消失）
  - [ ] 限时任务收入和风险平衡（至少比同级任务高50%收益）
  - [ ] 如果有多个机会同时存在，最近的一个覆盖旧的
- **测试证据路径**: `tests/chat_opportunity/test_story_1_4.md`
- **风险**: 低。复用已有 Task 系统和 do_task 流程。
- **依赖**: Story 1.2

---

## Epic 2：宠物协助打工 (P1)

**GDD引用**: 第4.2节 - "交互1：宠物协助打工"  
**改动量估算**: ~150行（新增~100 + 修改~50）  
**风险等级**: 🟢 低  
**存档影响**: Pet +8 bytes, Player +4 bytes

### Story 2.1：宠物协助能力定义
- **Story ID**: PET-ASST-001
- **描述**: 为每种宠物类型定义协助能力，扩展 Pet 结构体
- **改动量**: ~40行（小）
- **GDD需求ID**: 4.2-交互1（宠物协助效果表）
- **具体实现**:
  1. Pet 结构体新增字段：`int assist_cooldown`（协助冷却天数，0=可用）、`int times_assisted`（累计协助次数，用于成就追踪）
  2. Player 新增：`int daily_pet_assists`（每日已使用协助次数）
  3. 在 `init_pets()` 中初始化协助能力：
     - 猫：程序员任务(索引4)成功率+15%
     - 狗：外卖(索引2)/快递任务收入+20%
     - 鹦鹉：社交类任务魅力检定+10
     - 龙：任意任务5%概率双倍收入
     - 凤凰：失败时血量不扣
  4. 新增枚举 `PetAssistType`：`ASSIST_SUCCESS_RATE, ASSIST_INCOME, ASSIST_CHARISMA, ASSIST_DOUBLE, ASSIST_HEALTH_GUARD`
- **验收标准**:
  - [ ] 5种宠物各有独特的协助效果
  - [ ] Pet 结构体新字段正确序列化
  - [ ] 存档升级到v8后宠物协助数据正确
- **测试证据路径**: `tests/pet_assist/test_story_2_1.md`
- **风险**: 低。仅扩展结构体+初始化。
- **依赖**: Epic 1 的 SAVE_VERSION → 8（如果 Epic 1 先完成，则直接使用 v8；否则需自己升级版本）

### Story 2.2：打工流程中注入宠物协助
- **Story ID**: PET-ASST-002
- **描述**: 在 do_task() 中，打工开始前询问是否让宠物协助，并应用相应效果
- **改动量**: ~80行（中等）
- **GDD需求ID**: 4.2-交互1
- **具体实现**:
  1. 在 `do_task()` 难度选择后、成功率计算前（行3214前），插入宠物协助检查：
     ```
     if (player.pet_count > 0 && player.daily_pet_assists < 1) {
         列出可协助的宠物（happiness>50, assist_cooldown==0）
         提示"是否让宠物协助？（每天限1次）"
     }
     ```
  2. 根据 `PetAssistType` 应用效果到 `multiplier` 或 `success_chance`
  3. 协助后设置 `pet.assist_cooldown = 2`（冷却2天），`player.daily_pet_assists++`
  4. 龙的特殊效果：成功判定后额外5%概率触发"双倍收入"
  5. 凤凰的效果：失败分支中，如果凤凰协助了，跳过血量扣除
  6. `day_end()` 中：所有宠物的 `assist_cooldown--`（最小0），重置 `daily_pet_assists`
- **验收标准**:
  - [ ] 打工时可选宠物协助（每天限1次）
  - [ ] 猫协助程序员任务时成功率显示正确提升
  - [ ] 狗协助外卖任务时最终收入+20%
  - [ ] 龙协助时5%概率触发双倍收入（测试需多次验证）
  - [ ] 凤凰协助失败时不扣血
  - [ ] 协助后宠物进入2天冷却
  - [ ] 不快乐(happiness≤50)的宠物不出现在协助选项中
- **测试证据路径**: `tests/pet_assist/test_story_2_2.md`
- **风险**: 🟡 中。涉及 do_task 核心流程改动，需确保不破坏现有收入计算链。
- **依赖**: Story 2.1

### Story 2.3：宠物协助UI与反馈
- **Story ID**: PET-ASST-003
- **描述**: 添加宠物协助的视觉反馈：协助动画文字、协助后状态变化提示、成就追踪
- **改动量**: ~30行（小）
- **GDD需求ID**: 7.2（叙事型成就扩展）
- **具体实现**:
  1. 协助时的文字描述：`"%s 跳上你的肩膀，用头蹭了蹭你的手。今天它决定帮你一把。"`（根据宠物类型变化）
  2. 协助后在日报(`show_daily_report`)中记录："🐾 宠物协助：%s 帮你完成了1次打工"
  3. 新增追踪字段 `Pet::times_assisted`，每次协助+1
  4. `check_achievements()` 中添加成就："最佳搭档"（宠物累计协助20次）
- **验收标准**:
  - [ ] 协助触发时有专属文字动画
  - [ ] 日报中显示宠物协助记录
  - [ ] "最佳搭档"成就在宠物协助20次后解锁
- **测试证据路径**: `tests/pet_assist/test_story_2_3.md`
- **风险**: 低。纯展示层改动。
- **依赖**: Story 2.2

---

## Epic 3：好友组队 (P2)

**GDD引用**: 第4.2节 - "交互2：好友组队打工"  
**改动量估算**: ~280行（新增~200 + 修改~80）  
**风险等级**: 🟡 中  
**存档影响**: Friend +12 bytes, Player +8 bytes

### Story 3.1：好友组队能力定义
- **Story ID**: TEAM-WORK-001
- **描述**: 扩展 Friend 结构体，定义组队机制和冷却
- **改动量**: ~50行（小）
- **GDD需求ID**: 4.2-交互2
- **具体实现**:
  1. Friend 结构体新增字段：
     - `int team_cooldown`：组队冷却天数
     - `int times_teamed`：累计组队次数
     - `float team_bonus`：组队加成系数（基于好感度：50-69好感=1.2x, 70-89=1.4x, 90-100=1.6x）
  2. Player 新增字段：`int daily_team_count`（每日组队次数限制，上限1）、`int total_team_count`（总计组队次数）
  3. `init_friends()` 中初始化组队相关字段
- **验收标准**:
  - [ ] 好友结构体新字段正确序列化
  - [ ] 好感度不同等级的组队加成不同
  - [ ] 存档升级正确初始化组队字段
- **测试证据路径**: `tests/team_work/test_story_3_1.md`
- **风险**: 🟡 中。存档兼容性 + 好友数据结构变化。
- **依赖**: SAVE_VERSION → 8（需协调）

### Story 3.2：组队选项集成到打工流程
- **Story ID**: TEAM-WORK-002
- **描述**: 在 do_task() 中插入组队选择，应用组队加成
- **改动量**: ~100行（中等）
- **GDD需求ID**: 4.2-交互2（出租车+好友=双人拼车×1.4，程序员+好友=结对编程成功率+20%）
- **具体实现**:
  1. 在 `do_task()` 难度选择后（行3212附近），插入组队检查：
     ```
     if (player.friend_count > 0 && player.daily_team_count < 1) {
         列出好感度≥50且team_cooldown==0的好友
         提示"是否邀请好友组队？（每日限1次）"
     }
     ```
  2. 选择好友后应用组队加成：
     - 出租车(0)+组队：收入×额外1.4
     - 程序员(4)+组队：成功率+20%（硬加成，不参与上限95%限制）
     - 服务生(3)+组队：收入×额外1.3
     - 外卖(2)+组队：收入×额外1.5
  3. 组队后：
     - `friend.team_cooldown = 3`（3天冷却）
     - `player.daily_team_count++`
     - `friend.friendship_level += 3`（一起打工增进感情）
     - 该好友当日"标记为已使用"：显示为"今天已经帮你忙了，明天再说吧"
  4. day_end() 中重置 `daily_team_count`，所有好友 `team_cooldown--`
- **验收标准**:
  - [ ] 好感度≥50的好友出现在组队选项中
  - [ ] 出租车+好友组队收入为基准×1.4
  - [ ] 程序员+好友组队成功率提升20%（可超过95%上限）
  - [ ] 组队后好友进入3天冷却
  - [ ] 组队后好友好感度+3
  - [ ] 每日限1次组队
  - [ ] 好感度<50的好友不显示组队选项
- **测试证据路径**: `tests/team_work/test_story_3_2.md`
- **风险**: 🟡 中。do_task第二次改动（Epic 2也改了它），需注意合并顺序。
- **依赖**: Story 3.1

### Story 3.3：组队UI与好友系统集成
- **Story ID**: TEAM-WORK-003
- **描述**: 在好友界面添加"组队"选项，组队邀请/确认流程
- **改动量**: ~80行（中等）
- **GDD需求ID**: 4.2-交互2
- **具体实现**:
  1. `show_friends()` 菜单添加选项："3. 邀请组队"（已有0-3选项，需重新编号）
  2. 选择好友后进入组队确认流程：显示好友加成预览 → 确认 → 进入打工任务选择
  3. 组队状态下打工菜单直接跳转到 do_task
  4. 如果当天已组队或目标好友冷却中，显示友好提示
  5. `show_statistics()` 中添加 `total_team_count` 统计
- **验收标准**:
  - [ ] 好友菜单新增"邀请组队"选项
  - [ ] 组队成功后直接进入打工
  - [ ] 冷却中的好友显示"冷却中（剩余X天）"
  - [ ] 日报显示"今日与%s组队完成打工"
  - [ ] 统计面板显示累计组队次数
- **测试证据路径**: `tests/team_work/test_story_3_3.md`
- **风险**: 低。UI层改动，逻辑复用 Story 3.2。
- **依赖**: Story 3.2

### Story 3.4：组队叙事事件
- **Story ID**: TEAM-WORK-004
- **描述**: 组队打工时有概率触发专属微型事件（结对编程趣事、拼车奇遇等）
- **改动量**: ~50行（小）
- **GDD需求ID**: 4.2-交互2（叙事扩展）
- **具体实现**:
  1. `trigger_work_micro_event()` 中检测 `if (player.teamed_friend_index >= 0)` 时使用扩展事件池
  2. 添加3-5个组队专属微事件：
     - "结对编程": 好友帮你找到了一个bug，收入+15%
     - "拼车奇遇": 乘客是个有趣的人，给了额外小费
     - "队友掉链子": 好友临时有事中途离开，但你还是拿到了部分加成
  3. 专属事件完成后 `player.teamed_friend_index = -1`
- **验收标准**:
  - [ ] 组队打工时有概率（40%）触发组队专属微事件
  - [ ] 至少3种不同的组队专属事件
  - [ ] 组队事件完成后清除 teamed_friend_index
- **测试证据路径**: `tests/team_work/test_story_3_4.md`
- **风险**: 低。在已有微事件框架上扩展。
- **依赖**: Story 3.2

---

## Epic 4：每周挑战 (P2)

**GDD引用**: 第7.3节 - "每周挑战（周日本）"  
**改动量估算**: ~310行（新增~250 + 修改~60）  
**风险等级**: 🟡 中  
**存档影响**: Player +~400 bytes（复用Quest结构体），SAVE_VERSION → 8

### Story 4.1：每周挑战数据结构
- **Story ID**: WEEKLY-001
- **描述**: Player 结构体添加周挑战相关字段，定义挑战模板
- **改动量**: ~60行（小）
- **GDD需求ID**: 7.3-每周挑战类型表
- **具体实现**:
  1. Player 新增字段：
     - `Quest weekly_challenge`：当前周挑战（复用Quest结构体）
     - `int week_number`：当前是第几周（`day_count / 7 + 1`）
     - `int weekly_reset_day`：下次重置的天数（用于检测周日）
     - `int weeks_completed`：已完成周挑战总周数
     - `int weeks_failed`：未完成周挑战周数
  2. 定义5种挑战模板（静态数组）：
     - 赚钱周：`money_t target_income` = 10000
     - 节俭周：`money_t max_expenses` = 500
     - 社交周：`int target_interactions` = 5
     - 学习周：`int target_skill_ups` = 3
     - 冒险周：`int target_events` = 5
  3. `init_player()` 中初始化挑战为空
- **验收标准**:
  - [ ] Player结构体新字段正确序列化
  - [ ] 5种挑战模板定义完整
  - [ ] 周数计算正确（day_count/7+1）
- **测试证据路径**: `tests/weekly_challenge/test_story_4_1.md`
- **风险**: 🟡 中。存档兼容性 + Player结构体再次增长。
- **依赖**: 无（最独立的功能）

### Story 4.2：每周挑战生成与重置
- **Story ID**: WEEKLY-002
- **描述**: 每周日（day_count%7==0）自动生成新挑战，结算旧挑战
- **改动量**: ~80行（中等）
- **GDD需求ID**: 7.3-每周挑战
- **具体实现**:
  1. 新增函数 `generate_weekly_challenge()`：
     - 从5种挑战模板中随机选择（不与上周重复）
     - 根据玩家等级动态调整目标数值：`target = base × (1 + player.level × 0.05)`
     - 记录到 `player.weekly_challenge`
  2. 新增函数 `settle_weekly_challenge()`：
     - 判断 `weekly_challenge.progress >= weekly_challenge.requirement`
     - 成功：发放奖励（坤币/技能点/魅力/钥匙），`weeks_completed++`
     - 失败：无惩罚，`weeks_failed++`
  3. 在 `day_end()` 中（行3084-3088季节检查后）添加：
     ```
     if (player.day_count % 7 == 0 && !player.has_weekly_been_settled) {
         settle_weekly_challenge();
         generate_weekly_challenge();
         player.has_weekly_been_settled = true;
     }
     // 新一周开始
     if (player.day_count % 7 == 1) {
         player.has_weekly_been_settled = false;
     }
     ```
  4. Player 新增 `bool has_weekly_been_settled`
- **验收标准**:
  - [ ] 每周日（day 7, 14, 21...）自动触发结算+生成
  - [ ] 新挑战与上周不重复
  - [ ] 挑战目标随玩家等级动态调整
  - [ ] 日报中显示"本周挑战已更新！"
  - [ ] 第1天进入游戏时生成首个挑战
- **测试证据路径**: `tests/weekly_challenge/test_story_4_2.md`
- **风险**: 🟡 中。day_end() 改动+时间触发需严格测试。
- **依赖**: Story 4.1

### Story 4.3：挑战进度追踪
- **Story ID**: WEEKLY-003
- **描述**: 在关键游戏行为中更新周挑战进度
- **改动量**: ~90行（中等）
- **GDD需求ID**: 7.3-每周挑战类型表
- **具体实现**:
  1. `do_task()` 中（收入结算后）：如果是"赚钱周"→ `weekly_challenge.progress += income`
  2. `buy_item()`、`buy_food()` 等消费函数中：如果是"节俭周"→ 累加支出限制
  3. `trade_with_friend()`、`send_gift_to_friend()`、聊天室@互动中：如果是"社交周"→ `progress++`
  4. `upgrade_skill()` 中：如果是"学习周"→ `progress++`
  5. `trigger_work_micro_event()`、`random_event()` 中：如果是"冒险周"→ `progress++`
  6. 新增函数 `update_weekly_progress(int event_type, int amount)` 统一入口：
     ```
     switch (weekly_challenge.type) {
         case 赚钱周: weekly_challenge.progress += amount; break;
         case 节俭周: weekly_challenge.progress += amount; (若超支则标记失败)
         ...
     }
     ```
- **验收标准**:
  - [ ] 打工收入正确计入"赚钱周"进度
  - [ ] 消费金额正确计入"节俭周"进度
  - [ ] 好友互动正确计入"社交周"进度  
  - [ ] 技能升级正确计入"学习周"进度
  - [ ] 随机事件正确计入"冒险周"进度
  - [ ] 进度在超出目标时不溢出
- **测试证据路径**: `tests/weekly_challenge/test_story_4_3.md`
- **风险**: 🟡 中。多处代码改动，需仔细追踪所有进度钩子。
- **依赖**: Story 4.2

### Story 4.4：挑战UI与日报集成
- **Story ID**: WEEKLY-004
- **描述**: 主菜单、状态栏、日报中添加每周挑战展示
- **改动量**: ~80行（中等）
- **GDD需求ID**: 7.3（挑战展示格式）
- **具体实现**:
  1. `show_main_menu()` 顶部（状态显示后）添加挑战进度条：
     ```
     📋 本周挑战：赚钱周
     目标：周收入>10000 坤币 | 进度：3,200/10,000
     ───────────────────── ████░░░░░░░░ 32%
     ```
  2. `show_daily_report()` 末尾添加：
     ```
     📋 本周挑战进度
     目标：XXX | 当前：XXX | 剩余天数：X
     ```
  3. 新增独立函数 `show_weekly_challenge()` 展示完整挑战信息（从主菜单12号位或新入口）
  4. 挑战完成/失败时在日报中显示醒目提示
- **验收标准**:
  - [ ] 主菜单顶部或底部持续显示挑战进度条
  - [ ] 日报末尾追加本周挑战进度
  - [ ] 挑战完成时日报用金色文字突出显示
  - [ ] 挑战失败时日报显示鼓励性提示
- **测试证据路径**: `tests/weekly_challenge/test_story_4_4.md`
- **风险**: 低。纯展示层改动。
- **依赖**: Story 4.3

---

## 跨 Epic 依赖与冲突管理

### SAVE_VERSION 协调
```
方案：所有 Epic 共用一个 SAVE_VERSION → 8
做法：
  第1个完成的 Epic 负责升级版本号（预计 Epic 4 或 Epic 1）
  后续 Epic 检测到 version 已是 8，跳过版本迁移
  所有 Epic 的新字段统一追加到 Player 末尾
```

### do_task() 改动冲突
```
Epic 2（宠物协助）改 do_task 的 行3212-3214（加宠物选择）
Epic 3（好友组队）改 do_task 的 行3212-3214（加组队选择）

合并顺序：先加组队选择 → 再加宠物协助（或合并为一个改动点）
建议顺序：组队选择在难度选择之前，宠物协助在难度选择之后
```

### day_end() 改动冲突
```
Epic 4（每周挑战）在 day_end 行3084后加周检查
Epic 1 不需要改 day_end（限时任务重置通过 tasks[8].is_unlocked=false）
Epic 2 在 day_end 中重置 daily_pet_assists 和 assist_cooldown
Epic 3 在 day_end 中重置 daily_team_count 和 team_cooldown

合并：所有 day_end 新增逻辑在行3088季节更替之后依次追加
```

---

## 汇总：工作量与时间估算

| Epic | Stories | 总代码量 | 风险 | 建议工期 |
|------|---------|---------|------|---------|
| Epic 4：每周挑战 | 4 | ~310行 | 🟡 中 | 2-3天 |
| Epic 1：聊天室隐藏机会 | 4 | ~230行 | 🟢 低 | 2天 |
| Epic 2：宠物协助打工 | 3 | ~150行 | 🟢 低 | 1-2天 |
| Epic 3：好友组队 | 4 | ~280行 | 🟡 中 | 2-3天 |
| **总计** | **15** | **~970行** | — | **8-12天** |

> **注意**：上述时间估算是理想连续开发时间。实际开发中应考虑测试、debug、和现有系统集成的额外开销。
