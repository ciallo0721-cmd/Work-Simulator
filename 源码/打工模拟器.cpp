/*
 * 打工模拟器 v5.1 - 坤城篇 · TUI 版（股票真实化）
 * 世界：坤城 —— 一座虚构的当代中国沿海城市，300万人口的欲望都市
 * 叙事主题："在一个人人都想往上爬的城市里，你要成为什么样的人？"
 * 编译环境: Windows / Linux，C++（gcc 或 g++ 均可）
 *
 * ⚠ v5.0 起必须用 UTF-8 编译，绝对不要加 -fexec-charset=GBK！
 *   TUI 用 UTF-8 直出 + ANSI 转义，加 GBK 会导致
 *   "converting to execution character set: Illegal byte sequence"
 *   让 tui.h 解析中断，然后满屏 "tui_stream_reset was not declared"。
 *
 * 编译命令(Windows MinGW):
 *     g++ -O2 -o 打工模拟器.exe 打工模拟器.cpp -lm
 * 编译命令(Linux/Mac):
 *     g++ -O2 -o worker_sim 打工模拟器.cpp -lm
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <stdbool.h>
 #include <ctype.h>
 #include <stdarg.h>
 #include <math.h>
#include <stdint.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
 
 #ifdef _WIN32
 #include <conio.h>
 #include <windows.h>
 #include <tlhelp32.h>
 #else
 #include <unistd.h>
 #define _getch() getchar()
 #endif

/* v5.0 TUI 引擎（必须放在所有系统头文件之后，因为它会重定义 printf） */
#include "tui.h"
 
 //================ 常量定义 ================
 #define MAX_FURNITURE 50
 #define MAX_TASKS 10
 #define MAX_MARKET 20
 #define MAX_ACHIEVEMENTS 30
 #define MAX_SKILLS 10
 #define MAX_QUESTS 10
 #define MAX_PETS 5
 #define MAX_FRIENDS 10
 #define MAX_INVENTORY 100
 #define MAX_SHOP_ITEMS 30
 #define MAX_COMPANY_LEVELS 10
#define SAVE_FILE "worker_sim_save.dat"
#define SAVE_DIR "save/"
#define SAVE_VERSION 8  // v4.1 文件夹结构存档
 
 
 // 货币限制常量
 #define MAX_KUN_COINS 9223372036854775807LL
 
 // 聊天室常量
 #define MAX_CHATTERS 10
 #define MAX_MESSAGES 50
 #define CHAT_HISTORY_FILE "chat_history.txt"
#define STOCK_HISTORY_FILE "save/stock_history.dat"
#define STOCK_HISTORY_POINTS 30  // 每支股票保留30个历史价格

/* ===== v5.1 真实化市场参数 ===== */
#define STOCK_COUNT          25
#define STOCK_SECTOR_COUNT   9
#define STOCK_TICKS_PER_DAY  120   /* 每 120 个 tick = 1 个交易日（约 2 分钟） */
#define STOCK_LIMIT_PCT      10    /* 主板涨跌停 ±10% */
#define STOCK_ST_LIMIT_PCT   5     /* ST 股涨跌停 ±5% */
#define STOCK_ST_LINE        65    /* 跌破基准价 65% 触发 ST */
#define STOCK_DELIST_LINE    25    /* 跌破基准价 25% 触发退市程序 */
#define STOCK_ST_DAYS        3     /* ST 满 3 个交易日 -> 停牌退市 */
#define STOCK_MAINT_RATIO    130   /* 维持担保比例 130%（融资盘强平线） */
#define MAX_NEWS             60    /* 新闻环形缓冲容量 */
#define STOCK_SAVE_FILE      "save/stocks.dat"
 
 // ANSI颜色代码
 #define COLOR_RED "\033[31m"
 #define COLOR_GREEN "\033[32m"
 #define COLOR_YELLOW "\033[33m"
 #define COLOR_BLUE "\033[34m"
 #define COLOR_MAGENTA "\033[35m"
 #define COLOR_CYAN "\033[36m"
 #define COLOR_WHITE "\033[37m"
 #define COLOR_RESET "\033[0m"
 #define COLOR_BOLD "\033[1m"
 
// v5.0: 清屏改为清空 TUI 文本流（真正的重绘由页面框架负责）
 #define CLEAR_SCREEN() tui_stream_reset()
 
 // 货币类型
 typedef long long money_t;
 
 //================ 枚举类型定义 ================
 typedef enum {
     EVENT_NONE,
     EVENT_INFLATION,
     EVENT_STOCK_BOOM,
     EVENT_TASK_BONUS,
     EVENT_FREE_FURNITURE,
     EVENT_LOTTERY_BONUS,
     EVENT_DOUBLE_EXP,
     EVENT_MYSTERY_BOX,
     EVENT_BOSS_CHALLENGE,
     EVENT_FESTIVAL,
     EVENT_TYPE_COUNT
 } EventType;
 
 typedef enum {
     RARITY_COMMON,
     RARITY_UNCOMMON,
     RARITY_RARE,
     RARITY_EPIC,
     RARITY_LEGENDARY
 } Rarity;
 
 typedef enum {
     PET_CAT,
     PET_DOG,
     PET_PARROT,
     PET_DRAGON,
     PET_PHOENIX
 } PetType;
 
 typedef enum {
     SKILL_NONE = -1,
     SKILL_SPEED,
     SKILL_STRENGTH,
     SKILL_INTELLIGENCE,
     SKILL_LUCK,
     SKILL_CHARISMA,
     SKILL_ENDURANCE,
     SKILL_CREATIVITY,
     SKILL_LEADERSHIP,
     SKILL_NEGOTIATION,
     SKILL_MANAGEMENT,
     SKILL_COUNT
 } SkillType;
 
 typedef enum {
     QUEST_DAILY,
     QUEST_WEEKLY,
     QUEST_MAIN,
     QUEST_SIDE,
     QUEST_SPECIAL
 } QuestType;
 
typedef enum {
    SEASON_SPRING,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER
} Season;

// v4.0 打工难度模式
typedef enum {
    DIFF_SLACK,     // 摸鱼 — 低风险低回报
    DIFF_NORMAL,    // 普通 — 标准模式
    DIFF_OVERTIME,  // 加班 — 双倍时间高回报有健康代价
    DIFF_HARD,      // 拼命 — 最高回报但有猝死风险
    DIFF_COUNT
} WorkDifficulty;
 
 // 聊天室人物类型
 typedef enum {
     CHATTER_NORMAL,
     CHATTER_SCAMMER,    // 骗子 - 红色
     CHATTER_BLACKMARKET, // 黑产 - 绿色
     CHATTER_TONGCHENG   // 同城 - 粉色
 } ChatterType;
 
 // 聊天室消息
 typedef struct {
     char sender[30];
     char content[200];
     ChatterType type;
     time_t timestamp;
 } ChatMessage;
 
 // 聊天室人物
 typedef struct {
     char name[30];
     ChatterType type;
     bool is_online;
     char personality[100];
 } Chatter;

 // 福布斯排行榜条目
 typedef struct {
     char player_name[50];
     money_t kun_coins;
     int level;
     time_t record_time;
     bool is_active;
 } ForbesRankingEntry;

 #define MAX_FORBES_ENTRIES 100
 #define FORBES_FILE "save/forbes.list"
 
 //================ 结构体定义 ================
 typedef struct {
     char name[50];
     char description[200];
     int condition;
     money_t reward;
     int reward_exp;
     bool unlocked;
     time_t unlock_time;
 } Achievement;
 
 typedef struct {
     char name[30];
     int level;
     int experience;
     int max_level;
     float multiplier;
     char description[100];
 } Skill;
 
 typedef struct {
     char name[50];
     char description[200];
     QuestType type;
     int requirement;
     money_t reward_coins;
     int reward_exp;
     bool completed;
     time_t assigned_time;
     time_t deadline;
     int progress;
 } Quest;
 
 typedef struct {
     PetType type;
     char name[30];
     int level;
     int experience;
     int happiness;
     int hunger;
     bool is_unlocked;
     float bonus_multiplier;
     time_t last_fed;
 } Pet;
 
 typedef struct {
     char name[30];
     int level;
     money_t required_coins;
     float income_multiplier;
     int unlock_skill_points;
     char description[100];
 } CompanyLevel;
 
 typedef struct {
     char name[50];
     int price;
     int value;
     Rarity rarity;
     bool is_consumable;
     int effect_value;
     char description[100];
     int quantity;
 } InventoryItem;
 
/* ---- v5.1 股票真实化：市场状态 ---- */
enum {
    STK_NORMAL   = 0,   /* 正常交易 */
    STK_RISK     = 1,   /* ST 风险警示（退市风险，涨跌幅收窄到 ±5%） */
    STK_HALT     = 2,   /* 停牌（退市整理期前的一日） */
    STK_DELISTED = 3    /* 已退市，永久不可交易 */
};

typedef struct {
    char name[50];
    int base_price;
    int current_price;
    int price_change;
    int volatility;
    time_t last_update;

    /* ---- v5.1 新增：真实化字段 ---- */
    int  sector;        /* 行业索引 */
    int  status;        /* STK_* */
    int  open_price;    /* 本交易日开盘 */
    int  prev_close;    /* 昨收 */
    int  day_high;      /* 本交易日最高 */
    int  day_low;       /* 本交易日最低 */
    int  limit_lock;    /* +1 涨停封死 / -1 跌停封死 / 0 正常 */
    int  drift;         /* 趋势漂移（每 tick 变动股数，含正负） */
    int  drift_ticks;   /* 趋势剩余 tick */
    int  bad_days;      /* 连续低迷交易日数（判 ST / 退市） */
    int  st_days;       /* 已挂 ST 的交易日数 */
    int  halt_days;     /* 停牌剩余交易日 */
    int  reipo_days;    /* 退市后经过的交易日（满 40 天重新上市） */
} Stock;

/* ---- v5.1 新闻 ---- */
#define NEWS_TYPE_ENT   0   /* 娱乐 */
#define NEWS_TYPE_ECO   1   /* 经济 */
#define NEWS_TYPE_POL   2   /* 政治 */

typedef struct {
    int  type;          /* NEWS_TYPE_* */
    int  stock;         /* 关联股票，-1 = 不限 */
    int  sector;        /* 关联行业，-1 = 不限 */
    int  impact;        /* 对股价的总影响百分比（负=利空） */
    int  tick_left;     /* 剩余影响 tick */
    int  day;           /* 发布时的交易日 */
    char head[120];
    char body[300];
} NewsItem;
 
 typedef struct {
     char name[30];
     char description[100];
     money_t coins_needed;
     int days_needed;
     float passive_income;
     int unlock_achievement_id;
     bool owned;
 } Property;
 
 typedef struct {
     char username[30];
     int level;
     bool is_online;
     time_t last_online;
     float friendship_level;
     bool is_friend;
 } Friend;
 
 typedef struct {
     char name[50];
     int id;
     int required_level;
     int base_income;
     int experience_gain;
     int energy_cost;
     int unlock_cost;
     float time_required;
     bool is_unlocked;
     char description[150];
     time_t last_completed;
     int completion_count;
     float success_rate;
     int required_skill_level[3];
     SkillType required_skills[3];
 } Task;
 
 typedef struct {
     char name[50];
     int price;
     int value;
     Rarity rarity;
     bool is_limited;
     time_t available_until;
     char description[150];
     float passive_income;
     int required_level;
     float boost_multiplier;
 } MarketItem;
 
 // 玩家主结构体
 typedef struct {
     // 玩家名称（新增）
     char player_name[50];

     // 核心资源
     money_t kun_coins;
     int kun_exp;
     int level;
     int energy;
     int max_energy;
     time_t last_login;
     char furniture[MAX_FURNITURE][50];
     int furniture_count;
     bool is_billionaire;
     EventType active_event;
     time_t event_end_time;
     bool tasks_unlocked[MAX_TASKS];
     Achievement achievements[MAX_ACHIEVEMENTS];
     int consecutive_days;
     time_t game_start_time;
     int total_play_time;
     int skill_points;
     Skill skills[MAX_SKILLS];
     Quest active_quests[MAX_QUESTS];
     Pet pets[MAX_PETS];
     int pet_count;
     CompanyLevel company_levels[MAX_COMPANY_LEVELS];
     int current_company_level;
     InventoryItem inventory[MAX_INVENTORY];
     int inventory_count;
   Stock stocks[25];
   int invested_stocks[25];

   /* ---- v5.1 股票真实化：融资盘 & 市场时钟 ---- */
   money_t stock_margin_debt[25];  /* 每支股票尚未偿还的融资金额 */
   money_t stock_loan_total;       /* 融资负债总额 */
   int  stock_tick;                /* 当前交易日已走过的 tick */
   int  stock_day;                 /* 股票交易日序号 */
   int  stock_force_closed;        /* 被强制平仓次数 */
   int  stock_margin_bought;       /* 融资买入次数 */
   int  stock_delist_count;        /* 已退市股票数 */
   int  stock_news_unread;         /* 未读新闻数 */
     Property properties[10];
     int property_count;
     Friend friends[MAX_FRIENDS];
     int friend_count;
     int daily_task_completions;
     int weekly_task_completions;
     int lifetime_task_completions;
     money_t total_income;
     money_t total_expenses;
     Season current_season;
     int day_count;
     int prestige_level;
    int prestige_points;
    bool has_premium;
    time_t premium_until;
    int achievement_points;
    int lottery_tickets;
    int mystery_keys;
    int boss_defeats;
    int festival_participations;
    int craft_count;
    
    // === v4.0 时间槽系统（替换旧能量制） ===
    int daily_time_slots;     // 当日可用时间槽（默认3：上午/下午/晚上）
    int max_time_slots;       // 每日最大时间槽
    int today_income;         // 今日总收入
    int today_expenses;       // 今日总支出
    int today_work_count;     // 今日打工次数
    int today_event_count;    // 今日随机事件/微事件次数
    WorkDifficulty last_difficulty; // 上次打工使用的难度
    bool day_ended;           // 今日是否已结算（防重复触发）
    int rent_cost;            // 每日房租
    int living_cost;          // 每日生活成本（餐饮交通等）
    
    // === v4.0 打工微型事件追踪 ===
    int gray_requests_this_session;  // 本次会话拒绝的灰色请求
    bool lied_to_npc;                // 本次会话是否对NPC撒谎
     
     // 新增属性
     int hunger;           // 饱食度 0-100
     int health;           // 血量 0-100
     int spirit;           // 精神 0-100
     bool is_hallucinating; // 幻觉状态
     time_t hallucination_end; // 幻觉结束时间
     int sudden_death_chance; // 猝死几率
    int hallu_count;      // 幻觉触发次数计数
    bool has_phone;       // 是否有手机
    bool in_chat_room;    // 是否在聊天室
    
    // 任务完成计数持久化
    int task_completion_counts[MAX_TASKS];
    
    // 成就追踪
    int scammers_detected;      // 人精：识破骗子次数
    int insider_profit;         // 赌怪：内幕消息获利累计
    int daily_positive_events;  // 锦鲤附体：当日正面随机事件数
    int gray_requests_refused;  // 技术无罪：拒绝灰色请求次数
    int limited_items_bought;   // 限定收藏家：购买限定物品次数
    bool has_never_lied;        // 一言九鼎：从未对NPC撒谎（初始true）
    bool been_bankrupt;         // 破产重生：曾经负债过
     
     // 手机相关
     ChatMessage chat_history[MAX_MESSAGES];
     int chat_message_count;
     Chatter chatters[MAX_CHATTERS];
     int chatter_count;
 } Player;
 
 //================ 全局变量 ================
 Player player;
 Task tasks[MAX_TASKS];
 MarketItem market[MAX_MARKET];
 Property available_properties[10];
 Friend potential_friends[MAX_FRIENDS];

 //================ v5.1 新闻系统全局 ================
 static NewsItem g_news[MAX_NEWS];
 static int  g_news_n = 0;        /* 已生成条数（环形，最多 MAX_NEWS） */
 static int  g_news_head = 0;     /* 环形头 */
 static int  g_news_seq = 0;      /* 自增序号，用于“第几条” */

 /* 行业名 */
 static const char* STOCK_SECTOR_NAME[STOCK_SECTOR_COUNT] = {
     "科技", "金融", "地产", "消费", "医药", "能源环保", "制造", "传媒", "农业"
 };

 /* 大盘景气度（-10 ~ +10），每天结算时随机漂移，影响所有股票 drift */
 static int g_market_mood = 0;
 
 //================ 函数声明 ================
 void enable_ansi();
 void clear_input_buffer();
 void print_colored(const char* color, const char* format, ...);
 void print_bold(const char* format, ...);
 void print_centered(const char* text);
 void draw_separator();
 void draw_double_separator();
 void draw_box(const char* title);
 int get_valid_input(int min, int max);
 int get_valid_input_with_cancel(int min, int max, int cancel_code);
 void press_any_key();
 void show_loading_screen(int duration);
 void print_progress_bar(int current, int max, int width);
 void safe_strcpy(char* dest, const char* src, size_t size);
 bool add_coins(money_t amount);
 bool remove_coins(money_t amount);
 
 // 游戏系统
 static void news_generate_one(void);   /* 前向声明：init_game 播种首页新闻用 */

 void init_game();
 void init_player();
 void init_tasks();
 void init_market();
 void init_achievements();
 void init_skills();
 void init_quests();
 void init_pets();
 void init_company_levels();
 void init_stocks();
 void init_properties();
 void init_friends();
 void init_chat_room();
 void save_game();
 void load_game();
 void ensure_dir(const char* path);
 void migrate_old_save();
 void show_main_menu();
 void show_status();
 void show_inventory();
 void show_skills();
 void show_quests();
 void show_pets();
 void show_company();
 void show_stock_market();
 void show_properties();
 void show_friends();
 void show_settings();
 void show_help();
 void show_statistics();
 void show_prestige_menu();
 void show_premium_shop();
 void show_achievements();
 void show_phone_menu();
 void show_chat_room();
 
 // 游戏逻辑
 void do_task(int task_index);
 void buy_item(int index);
 void buy_stock(int stock_index, int amount);
 void sell_stock(int stock_index, int amount);
 void open_stock_chart_window();
 void buy_property(int property_index);
 void upgrade_skill(int skill_index);
 void feed_pet(int pet_index);
 void play_with_pet(int pet_index);
 void adopt_pet(PetType type);
 void complete_quest(int quest_index);
 void level_up_company();
 void use_inventory_item(int item_index);
 void craft_item();
 void trade_with_friend(int friend_index);
 void send_gift_to_friend(int friend_index);
 void prestige();
 void buy_premium(int duration_days);
 void participate_lottery();
 void open_mystery_box();
 void challenge_boss();
 void participate_festival();
 void change_season();
 void update_stock_prices();
 void calculate_passive_income();
 void recover_energy();
 void buy_food();
 void eat_food();
 void recover_health();
 void check_hallucination();
 void apply_hallucination_effect();
 void clear_hallucination();
 void process_spirit_effects();
void day_end();                  // v4.0 每日结算
void show_daily_report();        // v4.0 日报
void trigger_work_micro_event(int task_index);  // v4.0 打工微型事件
 void add_chat_message(const char* sender, const char* content, ChatterType type);
 void show_chat_messages();
 void ai_chatter_response(int chatter_index, const char* player_message);
 void process_od_offer(int chatter_index);
 void process_tongcheng_offer(int chatter_index);

 // 福布斯排行榜系统
 void update_forbes_ranking();
 void load_forbes_ranking();
 void save_forbes_ranking();
 void show_forbes_ranking();
 void show_forbes_ranking_menu();
 void hospital_menu();
 
 // 事件系统
 void random_event();
 void trigger_event(EventType type);
 void end_event();
 void handle_event_effect(EventType type, bool apply);
 const char* get_event_name(EventType type);
 const char* get_event_description(EventType type);
 void show_active_events();
 
 // 成就系统
 void check_achievements();
 void unlock_achievement(int achievement_id);
 void show_achievement_progress();
 
 //================ 工具函数实现 ================
void enable_ansi() {
#ifdef _WIN32
    // 启用虚拟终端处理（ANSI转义序列支持）
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    // v4.1 修复中文乱码：设置控制台为UTF-8输出
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}
 
 void clear_input_buffer() {
     int c;
     while ((c = getchar()) != '\n' && c != EOF);
 }
 
 void print_colored(const char* color, const char* format, ...) {
     char buf[4096];
     va_list args;
     va_start(args, format);
     vsnprintf(buf, sizeof buf, format, args);
     va_end(args);
     tui_printf("%s%s%s", color, buf, COLOR_RESET);
 }
 
 void print_bold(const char* format, ...) {
     char buf[4096];
     va_list args;
     va_start(args, format);
     vsnprintf(buf, sizeof buf, format, args);
     va_end(args);
     tui_printf("%s%s%s", COLOR_BOLD, buf, COLOR_RESET);
 }
 
 void print_centered(const char* text) {
     int width = 76;
     int sw = tui_strwidth(text);
     int padding = (width - sw) / 2;
     if (padding < 0) padding = 0;
     printf("%*s%s\n", padding, "", text);
 }
 
 void draw_separator() {
     int i;
     printf(COLOR_BLUE);
     for (i = 0; i < 72; i++) printf("─");
     printf("\n" COLOR_RESET);
 }
 
 void draw_double_separator() {
     int i;
     printf(COLOR_YELLOW);
     for (i = 0; i < 72; i++) printf("═");
     printf("\n" COLOR_RESET);
 }
 
 /* v5.0: 标题不再打印成文本，而是接管 TUI 面板标题栏 */
 void draw_box(const char* title) {
     const char* t = title;
     if (t && t[0] == '?' && t[1] == '?') { t += 2; while (*t == ' ') t++; }
     tui_stream_reset();
     tui_stream_set_title(t ? t : "");
     printf("\n");
 }
 
/*==============================================================================
 *  v5.0 TUI 页面框架
 *  —— 顶栏 / 状态侧栏 / 内容面板 / 按键栏
 *  —— 老的 printf 页面会被捕获进内容面板，实现"零改动接入"
 *============================================================================*/
#define SB_W 27

static char g_page_title[128] = "主菜单";
static int  g_tui_selftest = 0;
static int  g_page_scrollable = 1;
static const char* TUI_KEYS_DEFAULT =
    " [方向键] 选择   [回车] 确认   [Esc] 返回   [数字] 快选 ";

/* ---- 股票实时走势缓冲 ---- */
#define TREND_MAX 160
static int g_trend[25][TREND_MAX];
static int g_trend_n = 0;

/* 千分位金额 */
static void tui_fmt_money(char* out, int cap, long long v)
{
    char tmp[32];
    int i, len, t = 0, neg = v < 0;
    if (cap < 2) return;
    if (neg) v = -v;
    snprintf(tmp, sizeof tmp, "%lld", v);
    len = (int)strlen(tmp);
    if (neg) out[t++] = '-';
    for (i = 0; i < len && t < cap - 1; i++) {
        if (i > 0 && (len - i) % 3 == 0 && t < cap - 1) out[t++] = ',';
        out[t++] = tmp[i];
    }
    out[t] = 0;
}

static const char* tui_season_str(void)
{
    static const char* n[4] = { "春", "夏", "秋", "冬" };
    int s = (int)player.current_season;
    if (s < 0 || s > 3) s = 0;
    return n[s];
}

/* 坤城指数：剔除退市股后，按仍在交易的股票均价十倍 */
static int tui_stock_index(void)
{
    int i, sum = 0, n = 0;
    for (i = 0; i < 25; i++) {
        if (player.stocks[i].status == STK_DELISTED) continue;
        sum += player.stocks[i].current_price;
        n++;
    }
    if (n == 0) return 0;
    return sum * 10 / (n * 100);
}

static void tui_trend_push(void)
{
    int i;
    if (g_trend_n < TREND_MAX) {
        for (i = 0; i < 25; i++) g_trend[i][g_trend_n] = player.stocks[i].current_price;
        g_trend_n++;
    } else {
        for (i = 0; i < 25; i++) {
            memmove(&g_trend[i][0], &g_trend[i][1], (TREND_MAX - 1) * sizeof(int));
            g_trend[i][TREND_MAX - 1] = player.stocks[i].current_price;
        }
    }
}

/* 从 stock_history.dat 装载最近 TREND_MAX 条 */
#define TREND_FILL 45   /* 历史不足时回填到这个长度，保证图表有基线 */

static void tui_trend_load(void)
{
    FILE* f = fopen(STOCK_HISTORY_FILE, "rb");
    long sz, rec_sz = 8 + 25 * 4;
    int cnt, start, i, k;
    long long ts;
    int p[25];

    g_trend_n = 0;
    if (f) {
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        cnt = (int)(sz / rec_sz);
        start = cnt > TREND_MAX ? cnt - TREND_MAX : 0;
        fseek(f, (long)start * rec_sz, SEEK_SET);
        for (i = start; i < cnt && g_trend_n < TREND_MAX; i++) {
            if (fread(&ts, 8, 1, f) != 1) break;
            if (fread(p, 4, 25, f) != 25) break;
            for (k = 0; k < 25; k++)
                g_trend[k][g_trend_n] = (p[k] > 0 && p[k] < 100000) ? p[k] : player.stocks[k].current_price;
            g_trend_n++;
        }
        fclose(f);
    }

    /* 历史太短就用当前价回填，避免图表只有一根针或被脏数据拉爆 Y 轴 */
    if (g_trend_n < TREND_FILL) {
        int need = TREND_FILL - g_trend_n;
        for (i = g_trend_n - 1; i >= 0; i--)
            for (k = 0; k < 25; k++) g_trend[k][i + need] = g_trend[k][i];
        for (i = 0; i < need; i++)
            for (k = 0; k < 25; k++) {
                int p0 = player.stocks[k].current_price;
                g_trend[k][i] = p0 * (94 + rand() % 13) / 100;
                if (g_trend[k][i] < 10) g_trend[k][i] = 10;
            }
        g_trend_n = TREND_FILL;
    }
}

/* 把当前价整体追加一条历史 */
static void tui_trend_save(void)
{
    FILE* f = fopen(STOCK_HISTORY_FILE, "ab");
    long long ts;
    int p[25], i;
    if (!f) return;
    ts = (long long)time(NULL);
    for (i = 0; i < 25; i++) p[i] = player.stocks[i].current_price;
    fwrite(&ts, 8, 1, f);
    fwrite(p, 4, 25, f);
    fclose(f);
}

/* v5.1：全市场推进（无均值回归，靠 stk_market_tick 里的真实引擎） */
static void stk_market_tick(void);          /* forward */
static void stk_day_close(void);            /* forward */

static void tui_stock_live_tick(void)
{
    stk_market_tick();
    tui_trend_push();
}

/* ---- 顶栏 ---- */
static void tui_draw_topbar(void)
{
    int w = tui_cv.w;
    char right[160];
    int sw;

    tc_fill(0, 0, w, 1, " ", TC_DIM, TC_BG3);
    tc_puts(2, 0, "坤城", TC_YELLOW, TC_BG3);
    tc_puts(6, 0, "打工模拟器", TC_WHITE, TC_BG3);
    tc_puts(17, 0, "v5.1", TC_FAINT, TC_BG3);

    snprintf(right, sizeof right, "第 %d 天 · %s季 · 声望 %d ",
             player.day_count, tui_season_str(), player.prestige_level);
    sw = tui_strwidth(right);
    if (w - sw - 2 > 20) tc_puts(w - sw - 2, 0, right, TC_CYAN, TC_BG3);
}

/* ---- 侧栏：一行属性 ---- */
static void tui_side_stat(int x, int y, const char* label, int v, int max,
                          int lo, int mid, int hi)
{
    char val[16];
    tc_puts(x, y, label, TC_DIM, TC_BG2);
    tc_bar(x + 5, y, 11, v, max, lo, mid, hi, TC_BG2);
    snprintf(val, sizeof val, "%3d", v > 999 ? 999 : v);
    tc_puts(x + 17, y, val, TC_WHITE, TC_BG2);
}

/* ---- 侧栏 ---- */
static void tui_draw_sidebar(int y, int h)
{
    int cx = 2, cy = y + 1;
    char buf[64];
    int bx;

    tc_panel(0, y, SB_W, h, "档案", TC_BORDER, TC_BG2, TC_YELLOW);
    if (h < 12) return;

    /* 姓名 / 等级 */
    tc_putsn(cx, cy, SB_W - 13, player.player_name[0] ? player.player_name : "无名氏", TC_WHITE, TC_BG2);
    snprintf(buf, sizeof buf, "Lv.%d", player.level);
    tc_puts(SB_W - 3 - tui_strwidth(buf), cy, buf, TC_YELLOW, TC_BG2);
    cy++;

    /* 坤币 */
    tc_puts(cx, cy, "坤币", TC_DIM, TC_BG2);
    tui_fmt_money(buf, sizeof buf, (long long)player.kun_coins);
    tc_puts(cx + 5, cy, buf, player.kun_coins < 0 ? TC_RED : TC_GREEN, TC_BG2);
    cy++;

    /* 时间槽 */
    tc_puts(cx, cy, "时间", TC_DIM, TC_BG2);
    {
        const char* sn[3] = { "上", "下", "晚" };
        for (bx = 0; bx < player.max_time_slots && bx < 3; bx++)
            tc_put(cx + 5 + bx * 3, cy, sn[bx],
                   bx < player.daily_time_slots ? TC_GREEN : TC_FAINT, TC_BG2);
    }
    cy++;

    /* 分隔 */
    tc_hline(cx, cy, SB_W - 4, TC_BG3, TC_BG2);
    cy++;

    /* 属性条 */
    tui_side_stat(cx, cy++, "饱食", player.hunger, 100, TC_RED, TC_YELLOW, TC_GREEN);
    tui_side_stat(cx, cy++, "血量", player.health, 100, TC_RED, TC_YELLOW, TC_GREEN);
    tui_side_stat(cx, cy++, "精神", player.spirit, 100, TC_RED, TC_CYAN, TC_MAGENTA);
    {
        int em = player.level * 100;
        if (em <= 0) em = 100;
        tc_puts(cx, cy, "经验", TC_DIM, TC_BG2);
        tc_bar(cx + 5, cy, 11, player.kun_exp, em, TC_BLUE, TC_BLUE, TC_CYAN, TC_BG2);
        snprintf(buf, sizeof buf, "%3d%%", (int)((long long)player.kun_exp * 100 / em));
        tc_puts(cx + 17, cy, buf, TC_CYAN, TC_BG2);
        cy++;
    }

    /* 状态徽章 */
    if (cy < y + h - 6) {
        tc_hline(cx, cy, SB_W - 4, TC_BG3, TC_BG2);
        cy++;
        bx = cx;
        if (player.is_hallucinating) { tc_puts(bx, cy, "幻觉", TC_MAGENTA, TC_BG2); bx += 5; }
        if (player.hunger < 20 && bx < SB_W - 6) { tc_puts(bx, cy, "饥饿", TC_RED, TC_BG2); bx += 5; }
        if (player.health < 30 && bx < SB_W - 6) { tc_puts(bx, cy, "重伤", TC_RED, TC_BG2); bx += 5; }
        if (player.spirit < 20 && bx < SB_W - 6) { tc_puts(bx, cy, "崩溃", TC_ORANGE, TC_BG2); bx += 5; }
        if (player.daily_time_slots <= 0 && bx < SB_W - 6) { tc_puts(bx, cy, "力竭", TC_YELLOW, TC_BG2); }
        cy++;
    }

    /* 资产概览 */
    if (cy + 6 < y + h) {
        char ab[96];
        tc_hline(cx, cy, SB_W - 4, TC_BG3, TC_BG2);
        cy++;
        {
            int ach = 0, i2, stk = 0;
            for (i2 = 0; i2 < MAX_ACHIEVEMENTS; i2++)
                if (player.achievements[i2].unlocked) ach++;
            for (i2 = 0; i2 < 25; i2++) stk += player.invested_stocks[i2];

            snprintf(ab, sizeof ab, "房产 %d · 宠物 %d", player.property_count, player.pet_count);
            tc_putsn(cx, cy, SB_W - 5, ab, TC_DIM, TC_BG2); cy++;
            if (cy < y + h) {
                snprintf(ab, sizeof ab, "成就 %d/%d · 点 %d", ach, MAX_ACHIEVEMENTS, player.skill_points);
                tc_putsn(cx, cy, SB_W - 5, ab, TC_DIM, TC_BG2); cy++;
            }
            if (cy < y + h) {
                snprintf(ab, sizeof ab, "公司 Lv.%d · 持股 %d", player.current_company_level + 1, stk);
                tc_putsn(cx, cy, SB_W - 5, ab, TC_DIM, TC_BG2); cy++;
            }
        }
        cy++;
    }

    /* 坤城指数迷你走势 */
    if (cy + 4 < y + h && g_trend_n > 1) {
        int idx[TREND_MAX], i;
        tc_puts(cx, cy, "坤城指数", TC_DIM, TC_BG2);
        snprintf(buf, sizeof buf, "%5d", tui_stock_index());
        tc_puts(cx + 9, cy, buf, TC_YELLOW, TC_BG2);
        cy++;
        for (i = 0; i < g_trend_n; i++) {
            int k, s = 0, n = 0;
            for (k = 0; k < 25; k++) {
                if (player.stocks[k].status == STK_DELISTED) continue;
                s += g_trend[k][i];
                n++;
            }
            idx[i] = n > 0 ? s * 10 / (n * 100) : 0;
        }
        tc_spark(cx, cy, SB_W - 5, idx, g_trend_n, idx[0], TC_RED, TC_GREEN, TC_BG2);
        cy++;
    }
}

/* ---- 内容面板 ---- */
static void tui_draw_content(void)
{
    int cy = 1;
    int ch = tui_cv.h - 3;
    int cx = SB_W + 1;
    int cw = tui_cv.w - cx - 1;

    if (cw < 34) { cx = 0; cw = tui_cv.w; }
    if (ch < 4) ch = 4;

    tc_panel(cx, cy, cw, ch, g_page_title, TC_BORDER, TC_BG2, TC_CYAN);
    tui_stream_flush(cx + 2, cy + 1, cw - 4, ch - 2, g_page_scrollable);
}

/* ---- 底部两条 ---- */
static void tui_draw_keybar(const char* keys)
{
    int y = tui_cv.h - 2;
    int w = tui_cv.w;
    if (y < 1) return;
    tc_fill(0, y, w, 1, " ", TC_DIM, TC_BG3);
    if (keys) tc_putsn(1, y, w - 2, keys, TC_DIM, TC_BG3);
    tc_fill(0, y + 1, w, 1, " ", TC_DIM, TC_BG);
}

/* ---- 整页 ---- */
static void tui_draw_page(const char* keys)
{
    if (tui_s_title[0]) {
        strncpy(g_page_title, tui_s_title, sizeof g_page_title - 1);
        g_page_title[sizeof g_page_title - 1] = 0;
    }
    tui_begin();
    tui_draw_topbar();
    tui_draw_sidebar(1, tui_cv.h - 3);
    tui_draw_content();
    tui_draw_keybar(keys ? keys : TUI_KEYS_DEFAULT);
}

/* 进入一个 TUI 子页 */
static void tui_page_enter(const char* title)
{
    tui_stream_reset();
    tui_stream_set_title(title ? title : "");
    strncpy(g_page_title, title ? title : "", sizeof g_page_title - 1);
    g_page_title[sizeof g_page_title - 1] = 0;
    g_page_scrollable = 1;
}

/* 让 draw_box / CLEAR_SCREEN 设置的标题同步到面板 */
static void tui_sync_title(void)
{
    if (tui_s_title[0]) {
        strncpy(g_page_title, tui_s_title, sizeof g_page_title - 1);
        g_page_title[sizeof g_page_title - 1] = 0;
    }
}


 int get_valid_input(int min, int max) {
     char label[112];
     int v = min;
     tui_sync_title();
     tui_draw_page(NULL);
     tui_present();
     snprintf(label, sizeof label, "输入 [%d-%d] ›", min, max);
     if (tui_input_int(tui_cv.h - 1, tui_cv.w, label, min, max, &v) < 0) return min;
     return v;
 }
 
 int get_valid_input_with_cancel(int min, int max, int cancel_code) {
     char label[128];
     int v = min;
     tui_sync_title();
     tui_draw_page(NULL);
     tui_present();
     snprintf(label, sizeof label, "输入 [%d-%d] · %d取消 ›", min, max, cancel_code);
     if (tui_input_int(tui_cv.h - 1, tui_cv.w, label, min, max, &v) < 0) return cancel_code;
     return v;
 }
 
 /* v5.0: 按任意键 —— 支持 PgUp/PgDn 翻阅长页面 */
 void press_any_key() {
     int k;
     if (g_tui_selftest) return;
     tui_sync_title();
     for (;;) {
         tui_draw_page(" [PgUp/PgDn] 翻阅   [任意其他键] 继续 ");
         tui_pause(tui_cv.h - 1, tui_cv.w, "按任意键继续");
         k = tui_key();
         if (k == TK_PGUP) { tui_s_off += (tui_cv.h - 8); continue; }
         if (k == TK_PGDN) { tui_s_off -= (tui_cv.h - 8); if (tui_s_off < 0) tui_s_off = 0; continue; }
         break;
     }
 }
 
 void show_skills() {
     CLEAR_SCREEN();
     draw_box("技能系统");
     
     printf("可用技能点：%d\n\n", player.skill_points);
     
     for (int i = 0; i < MAX_SKILLS; i++) {
         Skill* skill = &player.skills[i];
         
         const char* color = COLOR_WHITE;
         if (skill->level >= 15) color = COLOR_YELLOW;
         else if (skill->level >= 10) color = COLOR_MAGENTA;
         else if (skill->level >= 5) color = COLOR_CYAN;
         else if (skill->level >= 3) color = COLOR_GREEN;
         
         printf("%s%s Lv.%d%s\n", color, skill->name, skill->level, COLOR_RESET);
         printf("   %s\n", skill->description);
         int cost = skill->level * 100;
         cost = (int)(cost * (1.0f - player.prestige_level * 0.02f));
         if (cost < 10) cost = 10;
         printf("   加成：%.0f%%  升级需要：%d 坤币, 1技能点\n", 
                (pow(skill->multiplier, skill->level) - 1) * 100, cost);
         print_progress_bar(skill->level, skill->max_level, 30);
         printf("\n");
     }
     
     draw_separator();
     printf("1.速度 2.力量 3.智力 4.幸运 5.魅力\n");
     printf("6.耐力 7.创造力 8.领导力 9.谈判 10.管理\n");
     printf("0.返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 10);
     if (choice > 0) {
         upgrade_skill(choice - 1);
         printf("\n");
         press_any_key();
     }
 }
 
 void show_loading_screen(int duration) {
     int i, steps = 36;
     int w = tui_cv.w, h = tui_cv.h;
     int barw = (w > 70) ? 60 : w - 12;
     int cx = (w - barw) / 2;
     int cy = h / 2 - 4;

     for (i = 0; i <= steps; i++) {
         tui_begin();
         tc_puts_center(0, cy, w, "坤  城", TC_YELLOW, TC_BG);
         tc_puts_center(0, cy + 1, w, "打 工 模 拟 器", TC_WHITE, TC_BG);
         tc_puts_center(0, cy + 2, w, "v5.1  ·  TUI EDITION", TC_CYAN, TC_BG);
         tc_bar(cx, cy + 4, barw, i, steps, TC_CYAN, TC_CYAN, TC_CYAN, TC_BG);
         if (i >= steps) {
             tc_puts_center(0, cy + 6, w, "先找份活干，活过这个月。", TC_DIM, TC_BG);
         } else {
             tc_puts_center(0, cy + 6, w, "正在载入坤城……", TC_FAINT, TC_BG);
         }
         tui_present();
         tui_sleep_ms(duration / steps);
     }
     tui_sleep_ms(260);
 }
 
 void print_progress_bar(int current, int max, int width) {
     int filled = (int)((float)current / max * width);
     printf("[");
     for (int i = 0; i < width; i++) {
         if (i < filled) {
             printf(COLOR_GREEN "█" COLOR_RESET);
         } else {
             printf(COLOR_BLUE "-" COLOR_RESET);
         }
     }
     printf("] %d/%d", current, max);
 }
 
 void safe_strcpy(char* dest, const char* src, size_t size) {
     strncpy(dest, src, size - 1);
     dest[size - 1] = '\0';
 }
 
 bool add_coins(money_t amount) {
     if (amount <= 0) return false;
     
     // 检查是否会超过上限
     if (player.kun_coins > MAX_KUN_COINS - amount) {
         // 触发福利：如果超过4294967296，变成500000000
         if (player.kun_coins >= 4294967296LL) {
             print_colored(COLOR_YELLOW, "\n?? 检测到坤币异常！触发福利系统...\n");
             print_colored(COLOR_MAGENTA, "福利：坤币调整为 500,000,000\n");
             player.kun_coins = 500000000LL;
             return true;
         }
         player.kun_coins = MAX_KUN_COINS;
         print_colored(COLOR_YELLOW, "\n? 坤币已达上限！\n");
         return false;
     }
     
     player.kun_coins += amount;
     return true;
 }
 
 bool remove_coins(money_t amount) {
     if (amount <= 0) return true;
     if (player.kun_coins >= amount) {
         player.kun_coins -= amount;
         return true;
     }
     return false;
 }
 
 //================ 初始化函数 ================
 void init_game() {
     srand((unsigned int)(time(NULL) ^ (uintptr_t)&init_game));

     init_player();
     init_tasks();
     init_market();
     init_achievements();
     init_skills();
     init_quests();
     init_pets();
     init_company_levels();
     init_stocks();
     init_properties();
     init_friends();
     init_chat_room();

     // 加载福布斯排行榜
     load_forbes_ranking();

     // 尝试加载存档
     load_game();

     // 首页速报不该是空的：没有历史新闻就播种一批
     if (g_news_n < 3) {
         int q;
         for (q = g_news_n; q < 12; q++) news_generate_one();
     }

     // v4.0 fix: 同步存档恢复后的任务解锁状态
     for (int i = 0; i < MAX_TASKS; i++) {
         tasks[i].is_unlocked = player.tasks_unlocked[i];
     }

     // 检查是否是第一次进入游戏（无存档或名为默认名）
     FILE* check_file = fopen(SAVE_FILE, "rb");
     if (check_file) {
         fclose(check_file);
     }

     // 如果last_login为0，说明是新存档，需要输入名称
     time_t now = time(NULL);
     if (player.last_login == 0) {
         {
             char input_name[50];
             int W, H, k2;
             static const char* intro[6] = {
                 "你来到了坤城 —— 一座没有明确地理位置的沿海都市，约 300 万人口。",
                 "这里的人来自五湖四海：有人在 CBD 穿着西装谈百万生意，",
                 "有人在老城区出租屋里算着明天的饭钱。",
                 "今天还在路边摆摊的人，明天可能开奔驰 —— 但也可能反过来。",
                 "你兜里只有 500 坤币，租住在老城区的一间小屋里。",
                 "手上有一部手机，一个聊天室账号，和一颗想往上爬的心。"
             };

             tui_begin();
             W = tui_cv.w; H = tui_cv.h;
             tc_puts_center(0, 2, W, "坤  城", TC_YELLOW, TC_BG);
             tc_puts_center(0, 3, W, "打 工 模 拟 器   v5.1", TC_WHITE, TC_BG);
             tc_puts_center(0, 5, W, "「坤城不问你的过去，只问你的价格。」", TC_CYAN, TC_BG);
             for (k2 = 0; k2 < 6; k2++)
                 tc_puts_center(0, 8 + k2, W, intro[k2], TC_DIM, TC_BG);
             tc_puts_center(0, 15, W, "坤城的故事，从今天开始。", TC_YELLOW, TC_BG);
             tui_present();

             input_name[0] = 0;
             if (tui_line_input_native(H - 1, W, " 你的名字（直接回车用「匿名玩家」）› ",
                                       input_name, sizeof input_name) < 0 || input_name[0] == 0) {
                 safe_strcpy(player.player_name, "匿名玩家", 50);
             } else {
                 safe_strcpy(player.player_name, input_name, 50);
             }

             tui_begin();
             W = tui_cv.w; H = tui_cv.h;
             tc_puts_center(0, H / 2 - 3, W, "坤  城", TC_YELLOW, TC_BG);
             tc_puts_center(0, H / 2 - 1, W, "打 工 模 拟 器   v5.0", TC_WHITE, TC_BG);
             {
                 char wb[128];
                 snprintf(wb, sizeof wb, "欢迎，%s。", player.player_name);
                 tc_puts_center(0, H / 2 + 2, W, wb, TC_GREEN, TC_BG);
                 tc_puts_center(0, H / 2 + 4, W, "你兜里只有 500 坤币。今天，从这里开始。", TC_DIM, TC_BG);
             }
             tui_present();
             if (!g_tui_selftest) tui_pause(H - 1, W, "按任意键进入坤城");
         }

         player.last_login = now;
         player.game_start_time = now;
         player.current_season = SEASON_SPRING;
         player.day_count = 1;

         // 首次进入游戏，更新福布斯排行榜
         update_forbes_ranking();
     } else {
         // 计算经过的天数并更新
         struct tm* last_tm = localtime(&player.last_login);
         struct tm* current_tm = localtime(&now);
         
         int days_passed = current_tm->tm_yday - last_tm->tm_yday;
         if (current_tm->tm_year != last_tm->tm_year) {
             days_passed += 365;
         }
         
         if (days_passed > 0) {
             // 每日登录奖励
             player.consecutive_days++;
             money_t bonus = 100LL * player.consecutive_days;
             if (player.has_premium) bonus *= 2;
             add_coins(bonus);
             player.kun_exp += 50 * player.consecutive_days;
             player.daily_task_completions = 0;
             
             print_colored(COLOR_YELLOW, "\n? 每日登录奖励：获得 %lld 坤币和 %d 经验！\n", 
                          bonus, 50 * player.consecutive_days);
             
             if (player.consecutive_days % 7 == 0) {
                 add_coins(500LL);
                 player.lottery_tickets++;
                 print_colored(COLOR_MAGENTA, "连续登录 %d 天奖励：额外获得 500 坤币和 1 张彩票！\n", 
                              player.consecutive_days);
             }
             
             // 重置每日任务
             for (int i = 0; i < MAX_QUESTS; i++) {
                 if (player.active_quests[i].type == QUEST_DAILY) {
                     player.active_quests[i].completed = false;
                     player.active_quests[i].progress = 0;
                     player.active_quests[i].assigned_time = now;
                     player.active_quests[i].deadline = now + 86400;
                 }
             }
             
             player.day_count += days_passed;
             
             // 检查季节变化
             int seasons_passed = player.day_count / 30;
             if (seasons_passed > 0) {
                 for (int i = 0; i < seasons_passed; i++) {
                     change_season();
                 }
             }
         }
     }
     
     player.last_login = now;

     // 数值安全校验：确保生存状态值在合法范围内
     if (player.hunger > 100 || player.hunger < 0) player.hunger = 100;
     if (player.health > 100 || player.health < 0) player.health = 100;
     if (player.spirit > 100 || player.spirit < 0) player.spirit = 50;
 }
 
 void init_player() {
     memset(&player, 0, sizeof(Player));
     safe_strcpy(player.player_name, "匿名玩家", 50);
     player.level = 1;
     player.kun_coins = 500;  // 初始500坤币
     player.kun_exp = 0;
     player.energy = 100;
     player.max_energy = 100;
     player.consecutive_days = 1;
     player.current_company_level = 0;
     player.inventory_count = 0;
     player.property_count = 0;
     player.friend_count = 0;
     player.pet_count = 0;
     player.prestige_level = 0;
     player.prestige_points = 0;
     player.has_premium = false;
     player.current_season = SEASON_SPRING;
     player.lottery_tickets = 1;
     player.mystery_keys = 0;
     player.boss_defeats = 0;
     player.festival_participations = 0;
     player.achievement_points = 0;
     player.skill_points = 5;
     player.craft_count = 0;
     player.total_income = 0;
     player.total_expenses = 0;
     player.lifetime_task_completions = 0;
     player.daily_task_completions = 0;
     player.weekly_task_completions = 0;
     player.active_event = EVENT_NONE;
     player.event_end_time = 0;
     
     // 新增属性初始化
     player.hunger = 100;
     player.health = 100;
     player.spirit = 50;
     player.is_hallucinating = false;
     player.hallucination_end = 0;
     player.sudden_death_chance = 0;
     player.hallu_count = 0;
    player.has_phone = true;  // 初始有手机
    player.in_chat_room = false;
    
    // 成就追踪初始化
    player.scammers_detected = 0;
    player.insider_profit = 0;
    player.daily_positive_events = 0;
    player.gray_requests_refused = 0;
    player.limited_items_bought = 0;
    player.has_never_lied = true;
    player.been_bankrupt = false;
    player.chat_message_count = 0;
    player.chatter_count = 0;
    
    // === v4.0 时间槽系统初始化 ===
    player.daily_time_slots = 3;
    player.max_time_slots = 3;
    player.today_income = 0;
    player.today_expenses = 0;
    player.today_work_count = 0;
    player.today_event_count = 0;
    player.last_difficulty = DIFF_NORMAL;
    player.day_ended = true;     // 新游戏开始，视为刚结算过
    player.rent_cost = 100;      // 初始房租 100坤币/天
    player.living_cost = 80;     // 初始生活成本 80坤币/天
    player.gray_requests_this_session = 0;
    player.lied_to_npc = false;
     
     // 初始家具
     safe_strcpy(player.furniture[0], "简易床", 50);
     safe_strcpy(player.furniture[1], "破旧椅子", 50);
     player.furniture_count = 2;
     
     // 初始物品
     InventoryItem starter_items[] = {
         {"能量饮料", 0, 10, RARITY_COMMON, true, 20, "恢复20点能量", 3},
         {"新手装备", 0, 50, RARITY_UNCOMMON, false, 5, "增加5%任务收入", 1},
         {"彩票", 0, 5, RARITY_COMMON, true, 0, "有机会赢取大奖", 1},
         {"宠物食品", 0, 5, RARITY_COMMON, true, 30, "喂食宠物", 5},
         {"面包", 10, 5, RARITY_COMMON, true, 15, "恢复15点饱食度", 3},
         {"盒饭", 30, 10, RARITY_COMMON, true, 30, "恢复30点饱食度", 2}
     };
     
     for (int i = 0; i < 6; i++) {
         player.inventory[player.inventory_count++] = starter_items[i];
     }
 }
 
 void init_tasks() {
     for (int i = 0; i < MAX_TASKS; i++) {
         tasks[i].id = i + 1;
         tasks[i].is_unlocked = false;
         tasks[i].completion_count = 0;
         tasks[i].last_completed = 0;
         for (int j = 0; j < 3; j++) {
             tasks[i].required_skill_level[j] = 0;
             tasks[i].required_skills[j] = SKILL_NONE;
         }
     }
     
     // 任务1: 出租车司机
     safe_strcpy(tasks[0].name, "出租车司机", 50);
     tasks[0].required_level = 1;
     tasks[0].base_income = 50;
     tasks[0].experience_gain = 10;
     tasks[0].energy_cost = 5;
     tasks[0].unlock_cost = 0;
     tasks[0].is_unlocked = true;
     safe_strcpy(tasks[0].description, "在老城区和CBD之间穿梭，接送坤城的过客", 150);
     tasks[0].success_rate = 0.9f;
     tasks[0].required_skills[0] = SKILL_SPEED;
     tasks[0].required_skill_level[0] = 0;
     
     // 任务2: 伐木工人
     safe_strcpy(tasks[1].name, "伐木工人", 50);
     tasks[1].required_level = 2;
     tasks[1].base_income = 100;
     tasks[1].experience_gain = 20;
     tasks[1].energy_cost = 10;
     tasks[1].unlock_cost = 500;
     safe_strcpy(tasks[1].description, "在开发区外围的林地挥洒汗水", 150);
     tasks[1].success_rate = 0.8f;
     tasks[1].required_skills[0] = SKILL_STRENGTH;
     tasks[1].required_skill_level[0] = 1;
     
     // 任务3: 外卖配送
     safe_strcpy(tasks[2].name, "外卖配送", 50);
     tasks[2].required_level = 3;
     tasks[2].base_income = 200;
     tasks[2].experience_gain = 30;
     tasks[2].energy_cost = 15;
     tasks[2].unlock_cost = 2000;
     safe_strcpy(tasks[2].description, "骑着电动车飞奔在坤城的大街小巷", 150);
     tasks[2].success_rate = 0.85f;
     tasks[2].required_skills[0] = SKILL_SPEED;
     tasks[2].required_skills[1] = SKILL_ENDURANCE;
     tasks[2].required_skill_level[0] = 1;
     tasks[2].required_skill_level[1] = 1;
     
     // 任务4: 餐厅服务生
     safe_strcpy(tasks[3].name, "餐厅服务生", 50);
     tasks[3].required_level = 5;
     tasks[3].base_income = 300;
     tasks[3].experience_gain = 40;
     tasks[3].energy_cost = 20;
     tasks[3].unlock_cost = 5000;
     safe_strcpy(tasks[3].description, "在CBD的高级餐厅里笑脸迎人", 150);
     tasks[3].success_rate = 0.75f;
     tasks[3].required_skills[0] = SKILL_CHARISMA;
     tasks[3].required_skills[1] = SKILL_SPEED;
     tasks[3].required_skill_level[0] = 2;
     tasks[3].required_skill_level[1] = 1;
     
     // 任务5: 程序员
     safe_strcpy(tasks[4].name, "程序员", 50);
     tasks[4].required_level = 8;
     tasks[4].base_income = 500;
     tasks[4].experience_gain = 60;
     tasks[4].energy_cost = 25;
     tasks[4].unlock_cost = 10000;
     safe_strcpy(tasks[4].description, "在科技园的格子间里敲代码改Bug", 150);
     tasks[4].success_rate = 0.7f;
     tasks[4].required_skills[0] = SKILL_INTELLIGENCE;
     tasks[4].required_skills[1] = SKILL_CREATIVITY;
     tasks[4].required_skill_level[0] = 3;
     tasks[4].required_skill_level[1] = 2;
     
     // 任务6: 股票交易员
     safe_strcpy(tasks[5].name, "股票交易员", 50);
     tasks[5].required_level = 10;
     tasks[5].base_income = 800;
     tasks[5].experience_gain = 80;
     tasks[5].energy_cost = 30;
     tasks[5].unlock_cost = 20000;
     safe_strcpy(tasks[5].description, "在坤城金融中心的交易大厅搏杀", 150);
     tasks[5].success_rate = 0.6f;
     tasks[5].required_skills[0] = SKILL_INTELLIGENCE;
     tasks[5].required_skills[1] = SKILL_LUCK;
     tasks[5].required_skills[2] = SKILL_NEGOTIATION;
     tasks[5].required_skill_level[0] = 4;
     tasks[5].required_skill_level[1] = 3;
     tasks[5].required_skill_level[2] = 2;
     
     // 任务7: 房地产经纪人
     safe_strcpy(tasks[6].name, "房地产经纪人", 50);
     tasks[6].required_level = 15;
     tasks[6].base_income = 1200;
     tasks[6].experience_gain = 100;
     tasks[6].energy_cost = 35;
     tasks[6].unlock_cost = 50000;
     safe_strcpy(tasks[6].description, "带客户看遍坤城各区的高端楼盘", 150);
     tasks[6].success_rate = 0.65f;
     tasks[6].required_skills[0] = SKILL_CHARISMA;
     tasks[6].required_skills[1] = SKILL_NEGOTIATION;
     tasks[6].required_skills[2] = SKILL_MANAGEMENT;
     tasks[6].required_skill_level[0] = 5;
     tasks[6].required_skill_level[1] = 4;
     tasks[6].required_skill_level[2] = 3;
     
     // 任务8: 企业家
     safe_strcpy(tasks[7].name, "企业家", 50);
     tasks[7].required_level = 20;
     tasks[7].base_income = 2000;
     tasks[7].experience_gain = 150;
     tasks[7].energy_cost = 40;
     tasks[7].unlock_cost = 100000;
     safe_strcpy(tasks[7].description, "在坤城创立自己的公司，雇佣第一批员工", 150);
     tasks[7].success_rate = 0.55f;
     tasks[7].required_skills[0] = SKILL_LEADERSHIP;
     tasks[7].required_skills[1] = SKILL_MANAGEMENT;
     tasks[7].required_skills[2] = SKILL_INTELLIGENCE;
     tasks[7].required_skill_level[0] = 6;
     tasks[7].required_skill_level[1] = 5;
     tasks[7].required_skill_level[2] = 4;
     
     // 任务9: 投资者
     safe_strcpy(tasks[8].name, "投资者", 50);
     tasks[8].required_level = 25;
     tasks[8].base_income = 3000;
     tasks[8].experience_gain = 200;
     tasks[8].energy_cost = 45;
     tasks[8].unlock_cost = 200000;
     safe_strcpy(tasks[8].description, "坐在办公室里决策——投资坤城的未来", 150);
     tasks[8].success_rate = 0.5f;
     tasks[8].required_skills[0] = SKILL_INTELLIGENCE;
     tasks[8].required_skills[1] = SKILL_LUCK;
     tasks[8].required_skills[2] = SKILL_NEGOTIATION;
     tasks[8].required_skill_level[0] = 7;
     tasks[8].required_skill_level[1] = 6;
     tasks[8].required_skill_level[2] = 5;
     
     // 任务10: 神秘探险
     safe_strcpy(tasks[9].name, "神秘探险", 50);
     tasks[9].required_level = 30;
     tasks[9].base_income = 5000;
     tasks[9].experience_gain = 300;
     tasks[9].energy_cost = 50;
     tasks[9].unlock_cost = 500000;
     safe_strcpy(tasks[9].description, "探索坤城不为人知的角落——有些地方地图上没有", 150);
     tasks[9].success_rate = 0.4f;
     tasks[9].required_skills[0] = SKILL_ENDURANCE;
     tasks[9].required_skills[1] = SKILL_LUCK;
     tasks[9].required_skills[2] = SKILL_STRENGTH;
     tasks[9].required_skill_level[0] = 8;
     tasks[9].required_skill_level[1] = 7;
     tasks[9].required_skill_level[2] = 6;
     
     // 解锁初始任务
     for (int i = 0; i < 3; i++) {
         tasks[i].is_unlocked = true;
         player.tasks_unlocked[i] = true;
     }
 }
 
void init_market() {
    // 家具类物品 —— 坤城世界观 v4.0（NDD flavor text）
    MarketItem items[] = {
        {"电视", 300, 50, RARITY_COMMON, false, 0, "屏幕里的人在笑。屏幕外的人在看。——有时候分不清哪个才是真的", 0.1f, 1, 1.0f},
        {"椅子", 100, 20, RARITY_COMMON, false, 0, "老城区出租屋标配，坐上去嘎吱响", 0.05f, 1, 1.0f},
        {"沙发", 500, 100, RARITY_UNCOMMON, false, 0, "终于不用坐在破椅子上了", 0.2f, 3, 1.1f},
        {"游泳池", 2000, 500, RARITY_RARE, false, 0, "坤城没有海。所以你买了这个", 0.5f, 10, 1.2f},
        {"黄金马桶", 10000, 2000, RARITY_EPIC, false, 0, "它不能让你更舒服。但它能让你忘记不舒服", 1.0f, 20, 1.3f},
        {"名画", 1500, 300, RARITY_RARE, false, 0, "坤城的拍卖行里淘来的——也可能是赝品", 0.3f, 5, 1.15f},
        {"水晶吊灯", 3000, 800, RARITY_EPIC, false, 0, "老城区的出租屋里挂水晶灯？邻居会来围观的", 0.6f, 15, 1.25f},
        {"家庭影院", 5000, 1500, RARITY_EPIC, false, 0, "在坤城的夜晚，一个人看一部电影", 0.8f, 18, 1.3f},
        {"工作台", 800, 200, RARITY_UNCOMMON, false, 0, "每天晚上趴在这里的时候，像在键盘上挖煤的矿工", 0.25f, 5, 1.1f},
        {"健身房", 2500, 600, RARITY_RARE, false, 0, "坤城的外卖太多，你得动一动", 0.4f, 12, 1.2f},
        {"游戏机", 600, 120, RARITY_UNCOMMON, false, 0, "在坤城打完工回到家，打开游戏机——这才是生活", 0.15f, 4, 1.05f},
        {"按摩椅", 1200, 250, RARITY_RARE, false, 0, "跑了一天的出租车后，腰终于不是自己的了——是椅子的", 0.35f, 8, 1.15f},
        {"智能音箱", 200, 40, RARITY_COMMON, false, 0, "对着它喊'放首歌'，坤城的夜晚就没那么安静了", 0.08f, 2, 1.02f},
        {"空气净化器", 400, 80, RARITY_COMMON, false, 0, "开发区那边的PM2.5有点高，有备无患", 0.12f, 3, 1.03f},
        {"咖啡机", 350, 70, RARITY_COMMON, false, 0, "坤城的打工人不需要睡眠——需要的是咖啡", 0.1f, 3, 1.03f},
        {"书柜", 450, 90, RARITY_UNCOMMON, false, 0, "在坤城，知识也许能改变命运。也许不能。但看看总没错", 0.14f, 4, 1.04f},
        {"钢琴", 3000, 700, RARITY_RARE, false, 0, "富人区的孩子都会弹。你自学也可以", 0.5f, 12, 1.2f},
        {"台球桌", 2000, 500, RARITY_RARE, false, 0, "开发区那边的工友们周末的娱乐", 0.4f, 10, 1.18f},
        {"酒柜", 1500, 350, RARITY_UNCOMMON, false, 0, "坤城的生意有一半是在酒桌上谈成的", 0.3f, 8, 1.12f},
        {"保险箱", 800, 200, RARITY_UNCOMMON, false, 0, "在坤城，你得学会保管好自己的东西", 0.2f, 6, 1.08f}
    };
     
     int item_count = sizeof(items) / sizeof(items[0]);
     for (int i = 0; i < item_count && i < MAX_MARKET; i++) {
         market[i] = items[i];
     }
 }
 
void init_achievements() {
    Achievement achievements[] = {
        {"第一桶金", "赚取100坤币——坤城的第一步", 100, 50, 10, false, 0},
        {"家具收藏家", "收集5件家具——老城区的出租屋开始有家的样子了", 5, 200, 20, false, 0},
        {"打工达人", "完成50次任务——坤城的各行各业都留下了你的脚印", 50, 500, 50, false, 0},
        {"股市新手", "股票投资获利1000坤币——第一次尝到资本的甜头", 1000, 800, 80, false, 0},
        {"幸运儿", "赢得彩票大奖——坤城今天对你微笑了", 1, 1000, 100, false, 0},
        {"百万富翁", "拥有100万坤币——在坤城，你终于可以抬起头了", 1000000, 5000, 500, false, 0},
        {"等级大师", "达到50级——坤城的每个角落都认识你了", 50, 2000, 200, false, 0},
        {"技能专家", "将一项技能升到10级——在这个领域，坤城没人比你更懂", 10, 1500, 150, false, 0},
        {"任务狂人", "完成1000次任务——你的身体发出了最后一次警告，然后放弃了警告", 1000, 3000, 300, false, 0},
        {"社交达人", "拥有5个好友——坤城有三百万人，其中五个认可了你", 5, 1000, 100, false, 0},
        {"宠物爱好者", "拥有3只宠物——在坤城，它们是你最忠诚的伙伴", 3, 1500, 150, false, 0},
        {"房产大亨", "拥有5处房产——从老城区到富人区，坤城到处都有你的家", 5, 4000, 400, false, 0},
        {"全技能大师", "所有技能达到5级——技多不压身，在坤城尤其如此", 5, 5000, 500, false, 0},
        {"限定收藏家", "收集所有限定物品——坤城的限量版，都归你了", 10, 6000, 600, false, 0},
        {"成就猎人", "解锁20个成就——坤城的故事，你正在一页页翻开", 20, 3000, 300, false, 0},
        {"初次转生", "完成第一次声望重置——那些杀不死你的……其实差点就杀死你了", 1, 10000, 1000, false, 0},
        {"传奇重生", "达到声望等级5——坤城的人开始用'传说'来称呼你", 5, 50000, 5000, false, 0},
        {"BOSS克星", "击败10次BOSS——坤城的地下世界都知道你的名字", 10, 8000, 800, false, 0},
        {"节日之魂", "参与5次节日活动——坤城的每一个节日你都没有缺席", 5, 5000, 500, false, 0},
        {"合成大师", "合成10次物品——废品在你手里就变成了宝贝", 10, 3000, 300, false, 0},
        // === v4.0 新增叙事型成就（NDD设计） ===
        {"人精", "在聊天室成功识破3个骗子的谎言——坤城的骗子都开始互相提醒：小心那个太聪明的", 3, 2000, 200, false, 0},
        {"赌怪", "靠聊天室内幕消息炒股获利超过5000——运气也是实力，这是赌徒们的谎言。但你好像不是赌徒", 5000, 3000, 300, false, 0},
        {"破产重生", "从负债状态恢复到10000坤币——那些杀不死你的……", 10000, 5000, 500, false, 0},
        {"社交恐怖分子", "同时有3位好友好感度达到满值——坤城有三百万人。其中三个认为你是他们最重要的人", 3, 3000, 300, false, 0},
        {"锦鲤附体", "一天内触发3个正面随机事件——坤城今天心情好。但明天就不一定了", 3, 2000, 200, false, 0},
        {"死神来了", "经历猝死事件后幸存——你看到了什么？有人说是白光，有人说是黑暗。你看到的是一张账单", 1, 5000, 500, false, 0},
        {"最后的乘客", "完成第500次出租车任务——后座上坐过很多人。坤城记住了你的车灯", 500, 5000, 500, false, 0},
        {"技术无罪", "程序员任务中拒绝3次灰色请求——在键盘上打什么字是你的自由。不打什么字也是", 3, 3000, 300, false, 0},
        {"一言九鼎", "从未对NPC撒谎——坤城的商人有一个暗号：穿西装可以说谎，但那个人不用说", 1, 5000, 500, false, 0},
    };
     
     int ach_count = sizeof(achievements) / sizeof(achievements[0]);
     for (int i = 0; i < ach_count && i < MAX_ACHIEVEMENTS; i++) {
         player.achievements[i] = achievements[i];
     }
 }
 
 void init_skills() {
     const char* skill_names[] = {"速度", "力量", "智力", "幸运", "魅力", "耐力", "创造力", "领导力", "谈判技巧", "管理能力"};
     const char* skill_descs[] = {
         "提升任务完成速度",
         "增加体力相关任务收入",
         "增加智力相关任务收入",
         "增加所有随机事件几率",
         "提升社交任务收入",
         "降低任务能量消耗",
         "提升创意任务收入",
         "提升团队任务收入",
         "提升交易收益",
         "提升被动收入"
     };
     
     for (int i = 0; i < MAX_SKILLS; i++) {
         player.skills[i].level = 1;
         player.skills[i].experience = 0;
         player.skills[i].max_level = 20;
         player.skills[i].multiplier = 1.05f;
         safe_strcpy(player.skills[i].name, skill_names[i], 30);
         safe_strcpy(player.skills[i].description, skill_descs[i], 100);
     }
 }
 
void init_quests() {
    for (int i = 0; i < MAX_QUESTS; i++) {
        memset(&player.active_quests[i], 0, sizeof(Quest));
    }
    
    time_t now = time(NULL);
    
    // === 每日任务 —— 坤城日常 ===
    safe_strcpy(player.active_quests[0].name, "今日打工", 50);
    safe_strcpy(player.active_quests[0].description, "在坤城完成任意3个打工任务", 200);
    player.active_quests[0].type = QUEST_DAILY;
    player.active_quests[0].requirement = 3;
    player.active_quests[0].reward_coins = 300;
    player.active_quests[0].reward_exp = 30;
    player.active_quests[0].assigned_time = now;
    player.active_quests[0].deadline = now + 86400;
    
    safe_strcpy(player.active_quests[1].name, "布置新家", 50);
    safe_strcpy(player.active_quests[1].description, "购买任意一件家具——出租屋也得有家的样子", 200);
    player.active_quests[1].type = QUEST_DAILY;
    player.active_quests[1].requirement = 1;
    player.active_quests[1].reward_coins = 200;
    player.active_quests[1].reward_exp = 20;
    player.active_quests[1].assigned_time = now;
    player.active_quests[1].deadline = now + 86400;
    
    safe_strcpy(player.active_quests[2].name, "学习理财", 50);
    safe_strcpy(player.active_quests[2].description, "进行一次股票投资——在坤城，光打工是不够的", 200);
    player.active_quests[2].type = QUEST_DAILY;
    player.active_quests[2].requirement = 1;
    player.active_quests[2].reward_coins = 400;
    player.active_quests[2].reward_exp = 40;
    player.active_quests[2].assigned_time = now;
    player.active_quests[2].deadline = now + 86400;
    
    // === 每周任务 ===
    safe_strcpy(player.active_quests[3].name, "周薪过万", 50);
    safe_strcpy(player.active_quests[3].description, "一周内在坤城赚取10000坤币", 200);
    player.active_quests[3].type = QUEST_WEEKLY;
    player.active_quests[3].requirement = 10000;
    player.active_quests[3].reward_coins = 2000;
    player.active_quests[3].reward_exp = 200;
    player.active_quests[3].assigned_time = now;
    player.active_quests[3].deadline = now + 604800;
    
    safe_strcpy(player.active_quests[4].name, "提升自我", 50);
    safe_strcpy(player.active_quests[4].description, "升级任意技能3次——在坤城，不学习就会被淘汰", 200);
    player.active_quests[4].type = QUEST_WEEKLY;
    player.active_quests[4].requirement = 3;
    player.active_quests[4].reward_coins = 1500;
    player.active_quests[4].reward_exp = 150;
    player.active_quests[4].assigned_time = now;
    player.active_quests[4].deadline = now + 604800;
    
    // === 主线任务 —— 坤城的人生篇章 ===
    safe_strcpy(player.active_quests[5].name, "百万富翁", 50);
    safe_strcpy(player.active_quests[5].description, "在坤城积累100万坤币——每一个百万富翁都是从出租屋开始的", 200);
    player.active_quests[5].type = QUEST_MAIN;
    player.active_quests[5].requirement = 1000000;
    player.active_quests[5].reward_coins = 10000;
    player.active_quests[5].reward_exp = 1000;
    player.active_quests[5].assigned_time = now;
    player.active_quests[5].deadline = 0;
    
    safe_strcpy(player.active_quests[6].name, "建立事业", 50);
    safe_strcpy(player.active_quests[6].description, "将公司升到5级——在坤城拥有自己的事业", 200);
    player.active_quests[6].type = QUEST_MAIN;
    player.active_quests[6].requirement = 5;
    player.active_quests[6].reward_coins = 5000;
    player.active_quests[6].reward_exp = 500;
    player.active_quests[6].assigned_time = now;
    player.active_quests[6].deadline = 0;
    
    // === 特殊任务 —— 坤城里程碑 ===
    safe_strcpy(player.active_quests[7].name, "首次挑战", 50);
    safe_strcpy(player.active_quests[7].description, "击败挑战BOSS——坤城的地下世界等着你的名字", 200);
    player.active_quests[7].type = QUEST_SPECIAL;
    player.active_quests[7].requirement = 1;
    player.active_quests[7].reward_coins = 5000;
    player.active_quests[7].reward_exp = 500;
    player.active_quests[7].assigned_time = now;
    player.active_quests[7].deadline = 0;
    
    safe_strcpy(player.active_quests[8].name, "节日初体验", 50);
    safe_strcpy(player.active_quests[8].description, "参与一次节日活动——坤城的节日，一个人过可不行", 200);
    player.active_quests[8].type = QUEST_SPECIAL;
    player.active_quests[8].requirement = 1;
    player.active_quests[8].reward_coins = 3000;
    player.active_quests[8].reward_exp = 300;
    player.active_quests[8].assigned_time = now;
    player.active_quests[8].deadline = 0;
    
    safe_strcpy(player.active_quests[9].name, "重生之路", 50);
    safe_strcpy(player.active_quests[9].description, "完成第一次声望重置——放下一切重新开始，坤城会记得你", 200);
    player.active_quests[9].type = QUEST_SPECIAL;
    player.active_quests[9].requirement = 1;
    player.active_quests[9].reward_coins = 10000;
    player.active_quests[9].reward_exp = 1000;
    player.active_quests[9].assigned_time = now;
    player.active_quests[9].deadline = 0;
}

void init_pets() {
     const char* pet_names[] = {"小咪", "旺财", "小鹦", "小焰", "凤儿"};
     float pet_bonus[] = {1.05f, 1.08f, 1.10f, 1.15f, 1.20f};
     
     for (int i = 0; i < MAX_PETS; i++) {
         player.pets[i].type = (PetType)i;
         player.pets[i].is_unlocked = false;
         player.pets[i].happiness = 100;
         player.pets[i].hunger = 100;
         player.pets[i].level = 1;
         player.pets[i].experience = 0;
         player.pets[i].bonus_multiplier = pet_bonus[i];
         player.pets[i].last_fed = time(NULL);
         safe_strcpy(player.pets[i].name, pet_names[i], 30);
     }
     
     // 解锁初始宠物（猫）
     player.pets[0].is_unlocked = true;
     player.pet_count = 1;
 }
 
 void init_company_levels() {
     const char* company_names[] = {"个体户", "小微企业", "小型公司", "中型企业", "大型公司", 
                                     "集团公司", "上市公司", "跨国企业", "商业帝国", "传奇财团"};
     const char* company_descs[] = {
         "独自经营的小生意",
         "雇佣1-2名员工",
         "拥有小型办公室",
         "在写字楼有办公室",
         "拥有自己的办公楼",
         "业务多元化发展",
         "在证券交易所上市",
         "业务遍布全球",
         "影响世界经济",
         "掌控全球经济命脉"
     };
     money_t required_coins[] = {0, 5000, 20000, 100000, 500000, 2000000, 10000000, 50000000, 200000000, 1000000000LL};
     float income_multipliers[] = {1.0f, 1.2f, 1.5f, 2.0f, 3.0f, 5.0f, 8.0f, 12.0f, 20.0f, 50.0f};
     int skill_points[] = {0, 1, 2, 3, 5, 8, 12, 18, 25, 35};
     
     for (int i = 0; i < MAX_COMPANY_LEVELS; i++) {
         safe_strcpy(player.company_levels[i].name, company_names[i], 30);
         safe_strcpy(player.company_levels[i].description, company_descs[i], 100);
         player.company_levels[i].level = i + 1;
         player.company_levels[i].required_coins = required_coins[i];
         player.company_levels[i].income_multiplier = income_multipliers[i];
         player.company_levels[i].unlock_skill_points = skill_points[i];
     }
     
     player.current_company_level = 0;
 }
 
void init_stocks() {
    // 坤城股票市场 —— 25支代表性股票
    const char* stock_names[] = {
        "坤坤科技", "外卖速递", "坤城地产", "坤能集团", "星光传媒",
        "坤城银行", "大江医药", "天虹通信", "龙腾制造", "碧水环保",
        "烈火能源", "青云教育", "金盾安保", "风驰物流", "华美百货",
        "北斗导航", "南山旅游", "星辰半导体", "绿野农业", "白鲸渔业",
        "九鼎建筑", "天机软件", "凤凰传媒", "云梦游戏", "紫金矿业"
    };
    int base_prices[] = {100, 50, 200, 150, 80, 120, 90, 180, 70, 60,
                         110, 40, 160, 85, 75, 250, 95, 300, 45, 55,
                         140, 220, 130, 170, 105};
    int volatilities[] = {10, 15, 8, 12, 20, 9, 11, 14, 16, 7,
                          13, 18, 6, 17, 19, 5, 10, 22, 25, 14,
                          8, 16, 12, 21, 9};
    /* 行业：0科技 1金融 2地产 3消费 4医药 5能源 6制造 7传媒 8农业 */
    int sectors[] = {0, 3, 2, 5, 7, 1, 4, 0, 6, 5,
                     5, 3, 6, 3, 3, 0, 3, 0, 8, 8,
                     2, 0, 7, 7, 5};

    for (int i = 0; i < 25; i++) {
         int bp = base_prices[i] * 100;   /* v5.1：内部单位为“分” */
         safe_strcpy(player.stocks[i].name, stock_names[i], 50);
         player.stocks[i].base_price = bp;
         player.stocks[i].current_price = bp;
         player.stocks[i].price_change = 0;
         player.stocks[i].volatility = volatilities[i];
         player.stocks[i].last_update = time(NULL);
         player.invested_stocks[i] = 0;

         /* v5.1 */
         player.stocks[i].sector       = sectors[i];
         player.stocks[i].status       = STK_NORMAL;
         player.stocks[i].open_price   = bp;
         player.stocks[i].prev_close   = bp;
         player.stocks[i].day_high     = bp;
         player.stocks[i].day_low      = bp;
         player.stocks[i].limit_lock   = 0;
         player.stocks[i].drift        = 0;
         player.stocks[i].drift_ticks  = 0;
         player.stocks[i].bad_days     = 0;
         player.stocks[i].st_days      = 0;
         player.stocks[i].halt_days    = 0;
         player.stocks[i].reipo_days   = 0;

         player.stock_margin_debt[i] = 0;
     }
     player.stock_loan_total   = 0;
     player.stock_tick         = 0;
     player.stock_day          = 1;
     player.stock_force_closed = 0;
     player.stock_margin_bought = 0;
     player.stock_delist_count = 0;
     player.stock_news_unread  = 0;
     g_market_mood = 0;
 }
 
/*================ v5.1 坤城新闻系统 ================*/
#define ARRN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static int stk_rnd(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(rand() % (hi - lo + 1));
}
static const char* stk_pick(const char* const* arr, int n)
{
    if (n <= 0) return "";
    return arr[rand() % n];
}

static const char* NEWS_TYPE_NAME[3] = { "娱乐", "经济", "政治" };

/* —— 人名库 —— */
static const char* SURNAME[] = {
    "林","苏","沈","陆","顾","白","程","唐","江","季","宋","许","温","楚",
    "谢","阮","韩","秦","叶","周","赵","钱","孙","李","吴","郑","冯","陈",
    "闻人","欧阳","司徒","南宫"
};
static const char* GIVEN[] = {
    "夕照","千叶","墨白","星野","南乔","一澜","一鸣","可可","江野","青禾",
    "知雪","子昂","知夏","若初","辞书","云归","小满","雪见","朗月","蓁蓁",
    "知远","沐白","江南","清和","彤彤","见山","临川","琥珀","未眠","与时"
};

static void stk_rand_name(char* out, size_t cap)
{
    const char* s = stk_pick(SURNAME, ARRN(SURNAME));
    const char* g = stk_pick(GIVEN, ARRN(GIVEN));
    snprintf(out, cap, "%s%s", s, g);
}

/* —— 娱乐 — 人设 —— */
static const char* ENT_JOB[] = { "演员","歌手","声优","音乐人","导演","偶像","编剧","主持人" };
static const char* ENT_PLACE[] = {
    "老城区的一家火锅店","开发区的片场","CBD 的一家酒店","大学城的演唱会后台",
    "富人区的独栋别墅","机场贵宾厅","坤城电影节红毯","深夜的烧烤摊","老城区的地下停车场"
};
typedef struct { const char* what; const char* why; } EntBad;
static const EntBad ENT_BAD[] = {
    { "被拍到与已婚制片人同乘一车", "两人已经在一起两年" },
    { "被爆出学历造假",           "学历证书是从中介手里买的" },
    { "因酒驾被交警带走",         "血液酒精含量超标三倍" },
    { "被前助理爆料长期拖欠工资", "助理已经三个月没拿到工资" },
    { "在片场当场罢演",           "原因是剧组连续通宵赶进度" },
    { "被曝偷税漏税",             "涉案金额高达八位数" },
    { "被拍到当街与人互殴",       "起因只是抢一个停车位" },
    { "直播时口出狂言引发众怒",   "粉丝当场脱粉回踩" },
    { "被曝签阴阳合同",           "合同金额与申报收入差了十倍" }
};
static const char* ENT_GOOD[] = {
    "发布个人新作","作品票房破十亿","开启全国巡演","拿下电影节最佳主角奖",
    "新作收视破纪录","签约头部公司","作品登顶年度榜单","新作入围国际影展"
};
static const char* ENT_DEAD[] = {
    "因突发心梗去世","在高速上遭遇车祸身亡","因病在坤城人民医院去世","在家中离世"
};
static const char* ENT_REL[] = { "妻子","丈夫","家人","儿女","经纪人","前助理","合作方" };

/* —— 经济 —— */
static const char* ECO_BAD[] = {
    "进入破产清算程序","被证监会立案调查","被曝财务造假","实控人被带走调查",
    "被交易所出具退市风险警示","年报净利润暴跌九成","主营业务全面停摆","被债权人申请重整"
};
static const char* ECO_GOOD[] = {
    "获得国资战略注资","完成对同行的收购","发布新旗舰产品","营收创历史新高",
    "中标坤城市政府大单","宣布回购股份","拿下行业最高资质认证"
};
typedef struct { const char* txt; int sector; } MacroPolicy;
static const MacroPolicy ECO_POLICY[] = {
    { "央行宣布降息 25 个基点",      1 },   /* 金融 */
    { "证监会收紧 IPO 审核",         1 },
    { "坤城出台楼市限购新政",        2 },   /* 地产 */
    { "二手房交易增值税免征年限上调", 2 },
    { "出口退税政策调整",            6 },   /* 制造 */
    { "新一轮反垄断调查启动",        0 },   /* 科技 */
    { "医药集采范围再扩大",          4 },   /* 医药 */
    { "新能源补贴退坡",              5 },   /* 能源环保 */
    { "农产品收储价格上调",          8 }    /* 农业 */
};

/* —— 政治 —— */
static const char* POL_POST[] = { "坤城市长","坤国总统","坤城市副市长","坤城议会主席" };
static const char* POL_EVENT[] = {
    "当选","宣誓就任","正式接任","补选胜出后出任"
};

/* 按 UTF-8 字符数截断，超出补省略号 */
static void news_clip(const char* src, char* out, size_t cap, int max_chars)
{
    int cnt = 0;
    size_t i = 0, j = 0;
    if (!src) { out[0] = 0; return; }
    while (src[i] && cnt < max_chars) {
        unsigned char c = (unsigned char)src[i];
        int len = (c < 0x80) ? 1 : ((c >> 5) == 0x6 ? 2 : ((c >> 4) == 0xE ? 3 : 4));
        if (j + (size_t)len + 4 >= cap) break;
        memcpy(out + j, src + i, (size_t)len);
        i += (size_t)len; j += (size_t)len;
        cnt++;
    }
    if (src[i]) { out[j++] = (char)0xE2; out[j++] = (char)0x80; out[j++] = (char)0xA6; }
    out[j] = 0;
}

/* 价格显示：内部以“分”为单位，输出 xx.yy 元 */
static void stk_price_str(char* out, size_t cap, int cents)
{
    if (cents < 0) snprintf(out, cap, "-%d.%02d", (-cents) / 100, (-cents) % 100);
    else           snprintf(out, cap, "%d.%02d", cents / 100, cents % 100);
}

/* 施加新闻对股价的影响 */
static void stk_apply_shock(int idx, int impact, int ttl)
{
    Stock* s = &player.stocks[idx];
    int total, per;
    if (s->status == STK_DELISTED) return;
    total = s->current_price * impact / 100;
    per = total / (ttl > 0 ? ttl : 1);
    if (per == 0 && impact != 0) per = (impact > 0 ? 1 : -1);
    s->drift += per;
    if (s->drift_ticks < ttl) s->drift_ticks = ttl;
}

static int news_push(int type, int stock, int sector, int impact, int ttl,
                     const char* head, const char* body)
{
    NewsItem* it;
    int idx, i, k;

    /* 最近 8 条里已有同样标题就不重复推（同一事件不刷屏） */
    for (k = 0; k < 8 && k < g_news_n; k++) {
        int pi = (g_news_n < MAX_NEWS) ? (g_news_n - 1 - k)
                : ((g_news_head - 1 - k + MAX_NEWS * 2) % MAX_NEWS);
        if (strcmp(g_news[pi].head, head) == 0) return 0;
    }

    if (g_news_n < MAX_NEWS) {
        idx = g_news_n++;
    } else {
        idx = g_news_head;
        g_news_head = (g_news_head + 1) % MAX_NEWS;
    }
    it = &g_news[idx];
    it->type = type;
    it->stock = stock;
    it->sector = sector;
    it->impact = impact;
    it->tick_left = ttl;
    it->day = player.stock_day;
    safe_strcpy(it->head, head, sizeof it->head);
    safe_strcpy(it->body, body, sizeof it->body);

    if (player.stock_news_unread < 99) player.stock_news_unread++;

    if (stock >= 0) {
        stk_apply_shock(stock, impact, ttl);
    } else if (sector >= 0) {
        for (i = 0; i < STOCK_COUNT; i++)
            if (player.stocks[i].sector == sector) stk_apply_shock(i, impact, ttl);
    } else {
        for (i = 0; i < STOCK_COUNT; i++) stk_apply_shock(i, impact * 2 / 3, ttl);
    }
    (void)g_news_seq;
    return 1;
}

/* 取第 k 新的新闻（k=0 最新） */
static NewsItem* news_recent(int k)
{
    int total = g_news_n, idx;
    if (total <= 0 || k < 0 || k >= total) return NULL;
    if (total < MAX_NEWS) idx = total - 1 - k;
    else idx = ((g_news_head - 1 - k) % MAX_NEWS + MAX_NEWS) % MAX_NEWS;
    return &g_news[idx];
}

static int news_stock_of_type(int sector_filter, int type_ok)
{
    /* 从指定行业里随机挑一支还在交易的股票，没有就退化为任意 */
    int pool[STOCK_COUNT], n = 0, i;
    for (i = 0; i < STOCK_COUNT; i++) {
        if (player.stocks[i].status == STK_DELISTED) continue;
        if (sector_filter >= 0 && player.stocks[i].sector != sector_filter) continue;
        pool[n++] = i;
    }
    if (n == 0) return -1;
    (void)type_ok;
    return pool[rand() % n];
}

/* —— 生成：娱乐类 —— */
static void news_gen_ent(void)
{
    char nm[40], nm2[40], head[120], body[300];
    const char* job;
    const char* place;
    int age, r, stock, impact, ttl;

    stk_rand_name(nm, sizeof nm);
    job = stk_pick(ENT_JOB, ARRN(ENT_JOB));
    place = stk_pick(ENT_PLACE, ARRN(ENT_PLACE));
    age = stk_rnd(18, 60);

    /* 60% 关联传媒股，其余让大盘沾点情绪 */
    stock = (rand() % 100 < 60) ? news_stock_of_type(7, 1) : -1;
    ttl = stk_rnd(40, 110);
    r = rand() % 100;

    if (r < 28) {
        /* 丑闻 */
        const EntBad* bad = &ENT_BAD[rand() % ARRN(ENT_BAD)];
        const char* what = bad->what;
        const char* why  = bad->why;
        const char* rel = stk_pick(ENT_REL, ARRN(ENT_REL));
        stk_rand_name(nm2, sizeof nm2);
        snprintf(head, sizeof head, "【娱乐】%s%s在%s%s", job, nm, place, what);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s在%s%s。据%s的%s爆料，%s。"
                 "当晚品牌方连夜撤下全部物料，坤城几家院线已经把排片压到最低。",
                 age, job, nm, place, what, nm, rel, why);
        impact = -stk_rnd(8, 26);
        stock = (stock < 0) ? news_stock_of_type(7, 1) : stock;
    } else if (r < 38) {
        /* 死亡 */
        const char* how = stk_pick(ENT_DEAD, ARRN(ENT_DEAD));
        age = stk_rnd(45, 88);
        snprintf(head, sizeof head, "【娱乐】%s%s%s，享年%d岁", job, nm, how, age);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s%s。据其家人透露，%s这两年身体一直不好，"
                 "但为了合约还在硬撑。消息传出后，坤城几家影视公司的股价应声而动。",
                 age, job, nm, how, nm);
        impact = -stk_rnd(5, 15);
        stock = (stock < 0) ? news_stock_of_type(7, 1) : stock;
    } else if (r < 48) {
        /* 婚讯 / 私生活，轻微利多 */
        snprintf(head, sizeof head, "【娱乐】%s%s宣布婚讯，对象是圈外人", job, nm);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s在%s被拍到与一位圈外人士牵手。"
                 "据%s身边人爆料，两人已经交往三年，这次是真的准备定下来了。",
                 age, job, nm, place, nm);
        impact = stk_rnd(3, 10);
    } else if (r < 88) {
        /* 新作品 */
        const char* good = stk_pick(ENT_GOOD, ARRN(ENT_GOOD));
        snprintf(head, sizeof head, "【娱乐】%s%s%s", job, nm, good);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s今天在坤城宣布%s。据其%s透露，这个项目准备了两年，"
                 "中间差点被资方砍掉。坤城传媒板块今天的情绪明显不太一样。",
                 age, job, nm, good, stk_pick(ENT_REL, ARRN(ENT_REL)));
        impact = stk_rnd(6, 22);
        stock = (stock < 0) ? news_stock_of_type(7, 1) : stock;
    } else {
        /* 塌房后复出 */
        snprintf(head, sizeof head, "【娱乐】%s%s沉寂两年后复出，首站选在坤城", job, nm);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s在%s宣布复出。据其%s透露，"
                 "这两年他/她一直在还债，也一直在等一个机会。市场买不买账，明天见分晓。",
                 age, job, nm, place, stk_pick(ENT_REL, ARRN(ENT_REL)));
        impact = stk_rnd(-8, 12);
    }

    news_push(NEWS_TYPE_ENT, stock, -1, impact, ttl, head, body);
}

/* —— 生成：经济类 —— */
static void news_gen_eco(void)
{
    char head[120], body[300];
    int stock, impact, ttl, r;
    const char* nm;

    /* 优先挑已经出问题的票 —— 让退市有迹可循 */
    stock = -1;
    {
        int i, cand = -1;
        for (i = 0; i < STOCK_COUNT; i++) {
            if (player.stocks[i].status == STK_DELISTED) continue;
            if (player.stocks[i].status == STK_RISK) { cand = i; break; }
        }
        if (cand < 0 || rand() % 100 < 55) stock = news_stock_of_type(-1, 1);
        else stock = cand;
    }
    if (stock < 0) return;
    nm = player.stocks[stock].name;
    ttl = stk_rnd(50, 120);
    r = rand() % 100;

    if (r < 45) {
        const char* what = stk_pick(ECO_BAD, ARRN(ECO_BAD));
        snprintf(head, sizeof head, "【经济】%s%s", nm, what);
        snprintf(body, sizeof body,
                 "据坤透社爆料，%s（%s板块）今日公告：%s。"
                 "知情人士称，问题从半年前就开始了，只是一直压着没爆。"
                 "坤城本地机构今天已经在往外跑了。",
                 nm, STOCK_SECTOR_NAME[player.stocks[stock].sector], what);
        impact = -stk_rnd(10, 32);
    } else if (r < 80) {
        const char* what = stk_pick(ECO_GOOD, ARRN(ECO_GOOD));
        snprintf(head, sizeof head, "【经济】%s%s", nm, what);
        snprintf(body, sizeof body,
                 "据坤透社爆料，%s旗下公司%s。业内人士认为，这轮动作会重塑整个%s板块的格局。"
                 "消息落地后，坤城几家券商连夜上调了评级。",
                 nm, what, STOCK_SECTOR_NAME[player.stocks[stock].sector]);
        impact = stk_rnd(8, 25);
    } else {
        const MacroPolicy* pol = &ECO_POLICY[rand() % ARRN(ECO_POLICY)];
        snprintf(head, sizeof head, "【经济】%s，市场情绪生变", pol->txt);
        snprintf(body, sizeof body,
                 "据坤透社爆料，%s。这条消息直接影响%s板块的估值逻辑，"
                 "%s作为板块龙头首当其冲。分析人士提醒：政策底不等于市场底。",
                 pol->txt, STOCK_SECTOR_NAME[pol->sector], nm);
        impact = stk_rnd(-18, 14);
        news_push(NEWS_TYPE_ECO, -1, pol->sector, impact, ttl, head, body);
        return;
    }

    news_push(NEWS_TYPE_ECO, stock, -1, impact, ttl, head, body);
}

/* —— 生成：政治类 —— */
static void news_gen_pol(void)
{
    char nm[40], head[120], body[300];
    int age, impact, ttl, r, sector;

    stk_rand_name(nm, sizeof nm);
    age = stk_rnd(30, 80);
    ttl = stk_rnd(60, 140);

    r = rand() % 100;
    if (r < 55) {
        const char* post = stk_pick(POL_POST, ARRN(POL_POST));
        const char* ev = stk_pick(POL_EVENT, ARRN(POL_EVENT));
        /* 政策倾向：选地产 or 金融 or 能源 */
        static const int pol_sectors[] = { 2, 1, 5, 6 };
        sector = pol_sectors[rand() % 4];
        snprintf(head, sizeof head, "【政治】%s%s%s", nm, ev, post);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s%s%s。他在竞选时承诺，"
                 "会把土地供应和产业补贴往%s方向倾斜。"
                 "坤城%s板块的老板们今晚大概睡不着。",
                 age, nm, ev, post, STOCK_SECTOR_NAME[sector], STOCK_SECTOR_NAME[sector]);
        impact = stk_rnd(-16, 22);
    } else if (r < 82) {
        const MacroPolicy* pol = &ECO_POLICY[rand() % ARRN(ECO_POLICY)];
        sector = pol->sector;
        snprintf(head, sizeof head, "【政治】%s，%s板块承压", pol->txt, STOCK_SECTOR_NAME[sector]);
        snprintf(body, sizeof body,
                 "据坤透社爆料，坤城市政府今日宣布：%s。"
                 "政策直接指向%s，短期内该板块的资金成本会明显上升。"
                 "有人连夜改方案，有人连夜跑路。",
                 pol->txt, STOCK_SECTOR_NAME[sector]);
        impact = stk_rnd(-22, 8);
    } else {
        const char* post = stk_pick(POL_POST, ARRN(POL_POST));
        snprintf(head, sizeof head, "【政治】%s%s因丑闻辞职，职位悬空", post, nm);
        snprintf(body, sizeof body,
                 "据坤透社爆料，时年%d岁的%s被曝出与本地开发商存在利益输送，"
                 "今日宣布辞去%s职务。坤城政商两界都在等下一任是谁。",
                 age, nm, post);
        sector = 2;
        impact = -stk_rnd(8, 20);
    }

    news_push(NEWS_TYPE_POL, -1, sector, impact, ttl, head, body);
}

static void news_generate_one(void)
{
    int tries;
    for (tries = 0; tries < 6; tries++) {
        int r = rand() % 100;
        int before = g_news_n;
        int pushed;
        if (r < 50)      { news_gen_ent(); pushed = g_news_n > before; }
        else if (r < 85) { news_gen_eco(); pushed = g_news_n > before; }
        else             { news_gen_pol(); pushed = g_news_n > before; }
        /* 环形已满时 g_news_n 不再增长，用 unread 计数兜底判断 */
        if (pushed || g_news_n >= MAX_NEWS) return;
        if (tries >= 2) return;      /* 实在凑不出新标题就收手 */
    }
}

/* 每个交易日开盘：1~2 条新新闻 */
static void news_daily_refresh(void)
{
    int n = 1 + (rand() % 100 < 35 ? 1 : 0);
    int i;
    for (i = 0; i < n; i++) news_generate_one();
}

/*================ v5.1 股票市场引擎 ================*/

/* 当前涨跌停幅度（%）*/
static int stk_limit_pct(const Stock* s)
{
    return (s->status == STK_RISK) ? STOCK_ST_LIMIT_PCT : STOCK_LIMIT_PCT;
}

/* 单 tick 推进一支股票（纯随机 + 趋势，没有任何“保底回升”） */
static void stk_tick_one(int idx)
{
    Stock* s = &player.stocks[idx];
    int d, drift, limit_hi, limit_lo;

    if (s->status == STK_DELISTED || s->status == STK_HALT) {
        s->price_change = 0;
        return;
    }

    /* 趋势衰减 */
    if (s->drift_ticks > 0) {
        s->drift_ticks--;
        if (s->drift_ticks == 0) s->drift = 0;
    }
    drift = s->drift;

    /* 随机游走（单位：分）：波动率决定幅度，允许连续下跌，没有回归力 */
    d = stk_rnd(-s->volatility * 4, s->volatility * 4);
    if (s->volatility >= 18 && (rand() % 100) < 12) d *= 2;   /* 尾部长跳 */
    d += drift;

    /* 市场情绪：景气好时买盘略多，差时卖压略重。
       这不是保底回升 —— 情绪为负时同样在往下推。 */
    if ((rand() % 100) < 15) {
        int mood = g_market_mood + (s->sector % 3) - 1;
        if (mood > 0) d += stk_rnd(1, 2 + mood / 3);
        else if (mood < 0) d -= stk_rnd(1, 2 + (-mood) / 3);
    }

    if (d == 0) d = (rand() % 2) ? 1 : -1;

    /* 涨跌停板 */
    limit_hi = s->prev_close * (100 + stk_limit_pct(s)) / 100;
    limit_lo = s->prev_close * (100 - stk_limit_pct(s)) / 100;
    if (limit_lo < 100) limit_lo = 100;

    s->current_price += d;
    if (s->current_price > limit_hi) { s->current_price = limit_hi; s->limit_lock = 1; }
    else if (s->current_price < limit_lo) { s->current_price = limit_lo; s->limit_lock = -1; }
    else s->limit_lock = 0;

    if (s->current_price < 100) s->current_price = 100;   /* 面值 1 元 */

    s->price_change = s->current_price - s->prev_close;
    if (s->current_price > s->day_high) s->day_high = s->current_price;
    if (s->current_price < s->day_low)  s->day_low  = s->current_price;
}

/* 融资盘爆仓：担保比例 = (持仓市值 + 现金) / 负债 */
static money_t stk_hold_value(void)
{
    int i;
    money_t v = 0;
    for (i = 0; i < STOCK_COUNT; i++)
        v += (money_t)player.invested_stocks[i] * player.stocks[i].current_price / 100;
    return v;
}

static int stk_maint_ratio(void)
{
    money_t asset;
    if (player.stock_loan_total <= 0) return 9999;
    asset = stk_hold_value() + (player.kun_coins > 0 ? player.kun_coins : 0);
    return (int)(asset * 100 / player.stock_loan_total);
}

/* 返回本次强平的股票数，同时填 out 文本 */
static int stk_force_liquidate(char* out, size_t cap)
{
    int i, n = 0;
    money_t repaid = 0, got = 0;

    if (out && cap) out[0] = 0;
    if (player.stock_loan_total <= 0) return 0;

    /* 从亏得最狠的持仓开始卖，直到担保比例回到 150% 以上 */
    for (;;) {
        int worst = -1;
        int worst_score = 1 << 30;
        for (i = 0; i < STOCK_COUNT; i++) {
            int sc;
            if (player.invested_stocks[i] <= 0) continue;
            if (player.stocks[i].status == STK_DELISTED) continue;
            sc = player.stocks[i].price_change * 100 / (player.stocks[i].prev_close > 0 ? player.stocks[i].prev_close : 1);
            if (sc < worst_score) { worst_score = sc; worst = i; }
        }
        if (worst < 0) break;
        if (player.stock_loan_total <= 0) break;
        if (stk_maint_ratio() >= 150 && n > 0) break;

        {
            int idx = worst;
            int shares = player.invested_stocks[idx];
            money_t v = (money_t)shares * player.stocks[idx].current_price / 100;
            char nb[64];

            player.invested_stocks[idx] = 0;
            got += v;
            /* 先还该股的融资，再还总负债 */
            if (player.stock_margin_debt[idx] > 0) {
                money_t pay = (v < player.stock_margin_debt[idx]) ? v : player.stock_margin_debt[idx];
                player.stock_margin_debt[idx] -= pay;
                player.stock_loan_total -= pay;
                repaid += pay;
                v -= pay;
            } else if (player.stock_loan_total > 0) {
                money_t pay = (v < player.stock_loan_total) ? v : player.stock_loan_total;
                player.stock_loan_total -= pay;
                repaid += pay;
                v -= pay;
            }
            if (v > 0) add_coins(v);

            n++;
            if (!out || !cap) continue;
            snprintf(nb, sizeof nb, "%s×%d ", player.stocks[idx].name, shares);
            {
                size_t used = strlen(out);
                if (used + 2 < cap) strncat(out, nb, cap - used - 1);
            }
        }
    }

    /* 卖完了还不够还债 -> 扣现金，仍不够就记破产 */
    if (player.stock_loan_total > 0) {
        money_t pay = (player.kun_coins < player.stock_loan_total) ? player.kun_coins : player.stock_loan_total;
        if (pay > 0) remove_coins(pay);
        player.stock_loan_total -= pay;
        if (player.stock_loan_total > 0) {
            player.been_bankrupt = true;
            player.stock_loan_total = 0;
        }
    }
    for (i = 0; i < STOCK_COUNT; i++) player.stock_margin_debt[i] = 0;

    if (n > 0) {
        player.stock_force_closed += n;
        (void)repaid;
    }
    return n;
}

/* 强制平仓（退市清算）：某只股退市后按极低价变现 */
static money_t stk_delist_settle(int idx)
{
    Stock* s = &player.stocks[idx];
    int shares = player.invested_stocks[idx];
    money_t back;

    s->status = STK_DELISTED;
    s->limit_lock = 0;
    s->drift = 0;
    s->drift_ticks = 0;
    s->reipo_days = 0;
    player.stock_delist_count++;

    /* 退市整理期折价：按最后价的 18% 清算 */
    back = (money_t)shares * (s->current_price * 18 / 100) / 100;
    if (shares > 0) {
        if (back > 0) add_coins(back);
        player.invested_stocks[idx] = 0;
    }
    /* 该股融资作废，直接还掉对应部分 */
    if (player.stock_margin_debt[idx] > 0) {
        money_t pay = (back < player.stock_margin_debt[idx]) ? back : player.stock_margin_debt[idx];
        player.stock_loan_total -= pay;
        if (player.stock_loan_total < 0) player.stock_loan_total = 0;
        player.stock_margin_debt[idx] = 0;
    }
    s->current_price = (s->current_price * 18 / 100 > 0) ? (s->current_price * 18 / 100) : 1;
    return back;
}

/* 交易日收盘结算：判 ST、判退市、刷新闻、重置涨跌停 */
static void stk_day_close(void)
{
    int i;
    for (i = 0; i < STOCK_COUNT; i++) {
        Stock* s = &player.stocks[i];

        /* 退市股：满 40 个交易日重新上市，补回市场供给 */
        if (s->status == STK_DELISTED) {
            s->reipo_days++;
            if (s->reipo_days >= 40) {
                char h[120], b[300];
                s->status = STK_NORMAL;
                s->current_price = s->base_price;
                s->prev_close = s->base_price;
                s->open_price = s->base_price;
                s->day_high = s->base_price;
                s->day_low = s->base_price;
                s->bad_days = 0;
                s->st_days = 0;
                s->drift = 0;
                s->drift_ticks = 0;
                s->reipo_days = 0;
                player.stock_delist_count--;
                if (player.stock_delist_count < 0) player.stock_delist_count = 0;
                snprintf(h, sizeof h, "【经济】%s完成重组，今日重新挂牌", s->name);
                snprintf(b, sizeof b,
                         "据坤透社爆料，%s在退市 %d 个交易日后完成债务重组与资产注入，"
                         "今日重新在坤城证券交易所挂牌。开盘价 %d.%02d 元。",
                         s->name, 40, s->base_price / 100, s->base_price % 100);
                news_push(NEWS_TYPE_ECO, -1, -1, 0, 1, h, b);
            }
            continue;
        }

        s->prev_close = s->current_price;
        s->open_price = s->current_price;
        s->day_high = s->current_price;
        s->day_low = s->current_price;
        s->limit_lock = 0;
        s->price_change = 0;

        /* 停牌倒计时 */
        if (s->status == STK_HALT) {
            if (s->halt_days > 0) s->halt_days--;
            if (s->halt_days <= 0) {
                stk_delist_settle(i);
            }
            continue;
        }

        /* 低迷计数 */
        if (s->current_price < s->base_price * STOCK_ST_LINE / 100) s->bad_days++;
        else s->bad_days = 0;

        /* 退市线：跌破基准价 12% 直接进整理期 */
        if (s->current_price < s->base_price * STOCK_DELIST_LINE / 100) {
            s->status = STK_HALT;
            s->halt_days = 1;
            {
                char h[120], b[300];
                snprintf(h, sizeof h, "【经济】%s触发退市整理，明日起停牌", s->name);
                snprintf(b, sizeof b,
                         "据坤透社爆料，%s股价已跌破面值，交易所决定对其启动退市整理程序。"
                         "持仓者将在停牌结束后按清算价强制变现，血本无归的概率很大。",
                         s->name);
                news_push(NEWS_TYPE_ECO, -1, -1, 0, 1, h, b);
            }
            continue;
        }

        /* ST 判定 */
        if (s->status == STK_NORMAL && s->bad_days >= 2) {
            s->status = STK_RISK;
            s->st_days = 0;
            {
                char h[120], b[300];
                snprintf(h, sizeof h, "【经济】%s被实施退市风险警示，简称变更为 ST%s", s->name, s->name);
                snprintf(b, sizeof b,
                         "据坤透社爆料，%s连续多日跌破净资产，交易所对其股票实施退市风险警示。"
                         "涨跌幅限制收窄至 ±5%%，连续 %d 个交易日未能改善将被终止上市。",
                         s->name, STOCK_ST_DAYS);
                news_push(NEWS_TYPE_ECO, -1, -1, 0, 1, h, b);
            }
        } else if (s->status == STK_RISK) {
            s->st_days++;
            if (s->st_days >= 10 ||
                (s->st_days >= STOCK_ST_DAYS && s->current_price < s->base_price * 55 / 100)) {
                s->status = STK_HALT;
                s->halt_days = 1;
                {
                    char h[120], b[300];
                    snprintf(h, sizeof h, "【经济】ST%s连续亏损，交易所决定终止其上市", s->name);
                    snprintf(b, sizeof b,
                             "据坤透社爆料，ST%s在风险警示期内仍未改善经营，"
                             "交易所今日作出终止上市决定。停牌一日后进入退市整理，"
                             "持有该股的散户大概率要认赔。", s->name);
                    news_push(NEWS_TYPE_ECO, -1, -1, 0, 1, h, b);
                }
            } else if (s->current_price > s->base_price * 80 / 100) {
                /* 摘帽 */
                s->status = STK_NORMAL;
                s->st_days = 0;
                s->bad_days = 0;
            }
        }
    }

    player.stock_day++;
    player.stock_tick = 0;

    /* 大盘情绪漂移 */
    g_market_mood += stk_rnd(-3, 3);
    if (g_market_mood > 10) g_market_mood = 10;
    if (g_market_mood < -10) g_market_mood = -10;

    news_daily_refresh();
}

/* 每个 tick 推进全市场 */
static void stk_market_tick(void)
{
    int i;

    for (i = 0; i < STOCK_COUNT; i++) stk_tick_one(i);

    /* 新闻影响倒计时 */
    for (i = 0; i < g_news_n; i++) {
        if (g_news[i].tick_left > 0) g_news[i].tick_left--;
    }

    /* 融资盘风控 */
    if (player.stock_loan_total > 0 && stk_maint_ratio() < STOCK_MAINT_RATIO) {
        char buf[256];
        stk_force_liquidate(buf, sizeof buf);
        {
            char h[120], b[300];
            snprintf(h, sizeof h, "【经济】券商风控触发：有账户被强制平仓");
            snprintf(b, sizeof b,
                     "据坤透社爆料，坤城某券商今日对多个融资账户执行强制平仓，"
                     "涉及标的包括 %s。担保比例跌破 %d%% 的账户，没有任何商量余地。",
                     buf[0] ? buf : "多只个股", STOCK_MAINT_RATIO);
            news_push(NEWS_TYPE_ECO, -1, -1, 0, 1, h, b);
        }
    }

    player.stock_tick++;
    if (player.stock_tick >= STOCK_TICKS_PER_DAY) stk_day_close();
}

void init_properties() {
     const char* prop_names[] = {"公寓", "别墅", "写字楼", "购物中心", "酒店", "度假村", "私人岛屿", "商业街", "科技园区", "国际机场"};
     const char* prop_descs[] = {
         "小型公寓，适合单身",
         "带花园的独栋别墅",
         "商业办公大楼",
         "大型商业综合体",
         "五星级豪华酒店",
         "海滨度假胜地",
         "专属私人岛屿",
         "整条商业街道",
         "高科技产业园区",
         "大型国际机场"
     };
     money_t prop_costs[] = {50000, 200000, 1000000, 5000000, 20000000, 50000000, 200000000, 500000000, 2000000000LL, 10000000000LL};
     int prop_days[] = {30, 60, 120, 180, 240, 300, 365, 450, 540, 720};
     float prop_incomes[] = {50, 200, 1000, 5000, 20000, 50000, 200000, 500000, 2000000, 10000000};
     int ach_ids[] = {1, 5, 10, 15, 20, 25, 30, 35, 40, 50};
     
     for (int i = 0; i < 10; i++) {
         safe_strcpy(available_properties[i].name, prop_names[i], 30);
         safe_strcpy(available_properties[i].description, prop_descs[i], 100);
         available_properties[i].coins_needed = prop_costs[i];
         available_properties[i].days_needed = prop_days[i];
         available_properties[i].passive_income = prop_incomes[i];
         available_properties[i].unlock_achievement_id = ach_ids[i];
         available_properties[i].owned = false;
     }
 }
 
void init_friends() {
    // 好友列表 —— 坤城世界观 v4.0
    // 角色身份（NDD设定）：
    //   小明=实干派引路人(出租车老油条)  小红=实干派同行(外卖拼命三娘)
    //   小刚=实干派冤家(莽撞伐木工)      李华=智力派引路人(快乐程序员)
    //   王芳=智力派同伴(连续创业者)      张伟=智力派对手(竞争老板)
    //   刘洋=社交派引路人(八面玲珑中介)  陈静=社交派同伴(销售女王)
    //   赵磊=社交派门路(神秘掮客)        周梅=中立观察者(记者)
    const char* names[] = {"小明", "小红", "小刚", "李华", "王芳", "张伟", "刘洋", "陈静", "赵磊", "周梅"};
     
     for (int i = 0; i < MAX_FRIENDS; i++) {
         safe_strcpy(potential_friends[i].username, names[i % 10], 30);
         potential_friends[i].level = 1 + rand() % 50;
         potential_friends[i].is_online = rand() % 2;
         potential_friends[i].last_online = time(NULL) - rand() % 86400;
         potential_friends[i].friendship_level = (float)(rand() % 100) / 10;
         potential_friends[i].is_friend = false;
     }
 }
 
void init_chat_room() {
    // 初始化聊天室人物 —— 坤城世界观 v4.0
    // 角色深度来自 NDD（叙事设计文档）voice pillar 设定
    struct {
        const char* name;
        ChatterType type;
        const char* personality;
    } chatter_data[] = {
        {"路人甲", CHATTER_NORMAL, "普通上班族，聊天室气氛组。嘴上说'还行吧'，其实特别在意。口头禅：'害，就是——'"},
        {"小妹", CHATTER_NORMAL, "在读大学生，家境一般，打零工维持生活。越活泼的时候越藏着难过。口头禅：'离谱''我人没了'"},
        {"老陈", CHATTER_NORMAL, "退休国企工人，聊天室里的长辈。儿子在大城市一年回来一次。口头禅：'我跟你说——'"},
        {"投资大师", CHATTER_SCAMMER, "职业骗子，装成金融精英。自己也曾被更大的骗子骗过。口头禅：'一般人我不告诉他'"},
        {"内部消息", CHATTER_SCAMMER, "跟风型骗子，说话留半句等你追问。口头禅：'不方便多说''你懂的'"},
        {"神秘人", CHATTER_BLACKMARKET, "灰色产业从业者，惜字如金。想赚够钱'上岸'。不说话才是他的风格"},
        {"深夜司机", CHATTER_BLACKMARKET, "夜间活跃的灰色角色，跑网约车顺便做小生意。想给孩子攒够学费"},
        {"小甜心", CHATTER_TONGCHENG, "提供陪伴服务的同城网友，甜是营业，累是真相。被前男友骗光积蓄"},
        {"温柔姐姐", CHATTER_TONGCHENG, "年长女性走知心姐姐路线，离婚独自抚养孩子。口头禅：'辛苦了''你也不容易'"},
        {"大学生兼职", CHATTER_TONGCHENG, "和小妹类似但走了不同路的大学生，靠特殊兼职维持生活。口头禅：'别说出去'"}
    };
     
     int chatter_count = sizeof(chatter_data) / sizeof(chatter_data[0]);
     for (int i = 0; i < chatter_count && i < MAX_CHATTERS; i++) {
         safe_strcpy(player.chatters[i].name, chatter_data[i].name, 30);
         player.chatters[i].type = chatter_data[i].type;
         player.chatters[i].is_online = true;
         safe_strcpy(player.chatters[i].personality, chatter_data[i].personality, 100);
     }
     player.chatter_count = chatter_count;
     
    // 添加欢迎消息 —— 坤城聊天室
    add_chat_message("系统", "欢迎来到坤城聊天室！请遵守社区规则。", CHATTER_NORMAL);
    add_chat_message("系统", "可以 @某人 来回复或提出请求。坤城什么人都有，小心辨别。", CHATTER_NORMAL);
 }
 
 void add_chat_message(const char* sender, const char* content, ChatterType type) {
     if (player.chat_message_count >= MAX_MESSAGES) {
         // 移除最旧的消息
         for (int i = 0; i < MAX_MESSAGES - 1; i++) {
             player.chat_history[i] = player.chat_history[i + 1];
         }
         player.chat_message_count--;
     }
     
     int idx = player.chat_message_count++;
     safe_strcpy(player.chat_history[idx].sender, sender, 30);
     safe_strcpy(player.chat_history[idx].content, content, 200);
     player.chat_history[idx].type = type;
     player.chat_history[idx].timestamp = time(NULL);
 }
 
 void show_chat_messages() {
     printf("\n" COLOR_BOLD "=== 聊天室记录 ===\n" COLOR_RESET);
     
     int start = player.chat_message_count > 20 ? player.chat_message_count - 20 : 0;
     for (int i = start; i < player.chat_message_count; i++) {
         ChatMessage* msg = &player.chat_history[i];
         
         const char* color = COLOR_WHITE;
         switch (msg->type) {
             case CHATTER_SCAMMER:
                 color = COLOR_RED;
                 break;
             case CHATTER_BLACKMARKET:
                 color = COLOR_GREEN;
                 break;
             case CHATTER_TONGCHENG:
                 color = COLOR_MAGENTA;
                 break;
             default:
                 color = COLOR_WHITE;
                 break;
         }
         
         struct tm* tm = localtime(&msg->timestamp);
         printf("[%02d:%02d] ", tm->tm_hour, tm->tm_min);
         printf("%s%s%s: ", color, msg->sender, COLOR_RESET);
         
         // 幻觉状态下的乱码处理
         if (player.is_hallucinating) {
             // 简单乱码：反转字符串并添加随机字符
             char garbled[220];
             int len = strlen(msg->content);
             for (int j = 0; j < len && j < 200; j++) {
                 garbled[j] = msg->content[len - 1 - j] ^ 0x80;
                 if (garbled[j] < 32) garbled[j] += 32;
             }
             garbled[len] = '\0';
             printf("%s\n", garbled);
         } else {
             printf("%s\n", msg->content);
         }
     }
     printf("\n");
 }
 
void ai_chatter_response(int chatter_index, const char* player_message) {
    if (chatter_index < 0 || chatter_index >= player.chatter_count) return;
    
    Chatter* chatter = &player.chatters[chatter_index];
    char response[200];
    
    // === 规则式AI响应 —— 按NDD角色语音支柱设计 ===
    switch (chatter->type) {
        case CHATTER_NORMAL: {
            // 根据角色名匹配独特的说话风格
            if (strcmp(chatter->name, "小妹") == 0) {
                if (strstr(player_message, "兼职") || strstr(player_message, "打工")) {
                    sprintf(response, "今天兼职站了六个小时，脚已经不是我的了。但是发工资的时候：我又可以了。");
                } else if (strstr(player_message, "累") || strstr(player_message, "崩溃")) {
                    sprintf(response, "真的服了……算了不提了。你也是一个人在这座城市吗？");
                } else if (strstr(player_message, "室友")) {
                    sprintf(response, "室友又在讨论去哪旅游。我说我打工没空。其实没人问我。");
                } else if (strstr(player_message, "你好") || strstr(player_message, "hi")) {
                    sprintf(response, "嗨嗨～今天课好多，上课摸鱼中……");
                } else {
                    sprintf(response, "离谱！今天又是充实的一天呢（指累成狗）。");
                }
            } else if (strcmp(chatter->name, "老陈") == 0) {
                if (strstr(player_message, "烦") || strstr(player_message, "累") || strstr(player_message, "难")) {
                    sprintf(response, "年轻人别急。我跟你说，我活了大半辈子就学会一件事：今天这事放明天看，就不是事了。");
                } else if (strstr(player_message, "以前") || strstr(player_message, "过去") || strstr(player_message, "小时候")) {
                    sprintf(response, "我们那会儿刚上班，一个月工资三十二块五。那时候能吃饱就高兴。现在你们不一样了，要的多了。");
                } else if (strstr(player_message, "家人") || strstr(player_message, "儿子") || strstr(player_message, "孩子")) {
                    sprintf(response, "我儿子上周打电话了。说忙。我说没事，你忙你的。");
                } else if (strstr(player_message, "你好") || strstr(player_message, "hi")) {
                    sprintf(response, "诶，你好你好。今天天气不错，遛弯去了趟公园。");
                } else {
                    sprintf(response, "诶，你听我讲——这人啊，到了我这个岁数就明白了，很多事争不来的。");
                }
            } else { // 路人甲或其他普通用户
                if (strstr(player_message, "你好") || strstr(player_message, "hi")) {
                    sprintf(response, "你好啊！今天过得……还行吧。");
                } else if (strstr(player_message, "工作") || strstr(player_message, "打工") || strstr(player_message, "加班")) {
                    sprintf(response, "今天又加班到九点。老板说项目急，我看他也不急，就是习惯让我们陪着。害，就是——算了不说了。");
                } else if (strstr(player_message, "梦想") || strstr(player_message, "理想") || strstr(player_message, "以后")) {
                    sprintf(response, "以前也想过去学点东西……后来发现学完了也没用。就这样呗。");
                } else if (strstr(player_message, "游戏")) {
                    sprintf(response, "最近在玩打工模拟器，挺有意思的——不对，我好像本来就在打工。");
                } else if (strstr(player_message, "培训") || strstr(player_message, "学习") || strstr(player_message, "报班")) {
                    sprintf(response, "哎你上次说的那个培训班，我去看了。还没报名。再看吧。");
                } else {
                    sprintf(response, "还行吧，差不多。你呢？");
                }
            }
            break;
        }
            
        case CHATTER_SCAMMER: {
            if (strcmp(chatter->name, "投资大师") == 0) {
                if (strstr(player_message, "赚钱") || strstr(player_message, "投资") || strstr(player_message, "股票")) {
                    sprintf(response, "兄弟，这个消息我只告诉你一个人。下周这个票至少翻一倍，不信你看着。");
                } else if (strstr(player_message, "理财") || strstr(player_message, "银行")) {
                    sprintf(response, "你还在犹豫什么？钱放在银行里只会贬值。我不是在帮你理财，我是在帮你革命。");
                } else if (strstr(player_message, "骗") || strstr(player_message, "假")) {
                    sprintf(response, "我跟你说真的，量化对冲底层资产——你懂这些词吗？一般人我不告诉他。");
                } else {
                    sprintf(response, "兄弟，想发财吗？加我私聊。我真的有一套独家策略。");
                }
            } else { // 内部消息
                if (strstr(player_message, "赚钱") || strstr(player_message, "投资") || strstr(player_message, "股票")) {
                    sprintf(response, "我有个朋友在XX证券……具体不能说，但你相信我，下周有大行情。不方便多说。");
                } else if (strstr(player_message, "消息") || strstr(player_message, "内幕")) {
                    sprintf(response, "昨天跟几个人吃了顿饭。饭桌上的消息，比你看一年K线都有用。你懂的。");
                } else {
                    sprintf(response, "有什么想问的私聊。这里人多。不方便多说。");
                }
            }
            break;
        }
            
        case CHATTER_BLACKMARKET: {
            if (strcmp(chatter->name, "神秘人") == 0) {
                if (strstr(player_message, "OD") || strstr(player_message, "药物") || strstr(player_message, "货")) {
                    sprintf(response, "有货。要不要？");
                } else if (strstr(player_message, "危险") || strstr(player_message, "警告")) {
                    sprintf(response, "劝你别碰。掉下去我拉不了你。");
                } else if (strstr(player_message, "上岸") || strstr(player_message, "改行")) {
                    sprintf(response, "上岸？呵呵。你以为岸上就干净？");
                } else if (strstr(player_message, "赚钱")) {
                    sprintf(response, "有来钱快的路子。敢不敢？");
                } else {
                    sprintf(response, "……");
                }
            } else { // 深夜司机
                if (strstr(player_message, "累") || strstr(player_message, "辛苦") || strstr(player_message, "晚")) {
                    sprintf(response, "这个点还在跑车的，都不是为了玩。就这样吧。");
                } else if (strstr(player_message, "OD") || strstr(player_message, "药物")) {
                    sprintf(response, "有需要？私聊详谈。能怎么办。");
                } else if (strstr(player_message, "赚钱")) {
                    sprintf(response, "你别嫌贵。这个价我给你的是良心价。");
                } else {
                    sprintf(response, "还在路上。能怎么办。");
                }
            }
            break;
        }
            
        case CHATTER_TONGCHENG: {
            if (strcmp(chatter->name, "小甜心") == 0) {
                if (strstr(player_message, "约") || strstr(player_message, "见面") || strstr(player_message, "出来")) {
                    sprintf(response, "私聊我哦～mua");
                } else if (strstr(player_message, "累") || strstr(player_message, "辛苦")) {
                    sprintf(response, "今天好累哦。有没有人陪我说说话。");
                } else if (strstr(player_message, "聊天") || strstr(player_message, "陪")) {
                    sprintf(response, "可以陪你聊天，不过……算了。好无聊哦。");
                } else if (strstr(player_message, "真") || strstr(player_message, "假装")) {
                    sprintf(response, "你觉得我是那种人吗？我其实……算了不说了。");
                } else {
                    sprintf(response, "有没有人嘛～好无聊哦。");
                }
            } else if (strcmp(chatter->name, "温柔姐姐") == 0) {
                if (strstr(player_message, "累") || strstr(player_message, "辛苦") || strstr(player_message, "难")) {
                    sprintf(response, "辛苦了。你也不容易。不想说也没关系，我在这里。");
                } else if (strstr(player_message, "以前") || strstr(player_message, "年轻")) {
                    sprintf(response, "我年轻时候也像你一样拼。后来发现有些东西拼不来。慢慢来。");
                } else if (strstr(player_message, "心情") || strstr(player_message, "难过") || strstr(player_message, "烦")) {
                    sprintf(response, "你今天好像心情不好。没事的，都会过去的。");
                } else {
                    sprintf(response, "没事的，慢慢来。我懂你。");
                }
            } else { // 大学生兼职
                if (strstr(player_message, "兼职") || strstr(player_message, "打工") || strstr(player_message, "钱")) {
                    sprintf(response, "如果我有得选，我才不会——算了，说了你也不懂。别说出去。");
                } else if (strstr(player_message, "毕业") || strstr(player_message, "以后")) {
                    sprintf(response, "毕业了就一切都会好的。对吧？……麻了。");
                } else if (strstr(player_message, "约") || strstr(player_message, "见面")) {
                    sprintf(response, "私下说。别说出去。");
                } else {
                    sprintf(response, "好烦，今天的课上得我想死。");
                }
            }
            break;
        }
    }
     
     add_chat_message(chatter->name, response, chatter->type);
     printf("%s%s%s: %s\n", 
            chatter->type == CHATTER_SCAMMER ? COLOR_RED :
            chatter->type == CHATTER_BLACKMARKET ? COLOR_GREEN :
            chatter->type == CHATTER_TONGCHENG ? COLOR_MAGENTA : COLOR_WHITE,
            chatter->name, COLOR_RESET, response);
 }
 
void process_od_offer(int chatter_index) {
    if (chatter_index < 0 || chatter_index >= player.chatter_count) return;
    if (player.chatters[chatter_index].type != CHATTER_BLACKMARKET) return;
    
    CLEAR_SCREEN();
    draw_box("坤城暗巷");
    
    print_colored(COLOR_RED, "%s 向你发来了一条加密消息...\n", player.chatters[chatter_index].name);
    printf("\n");
    print_colored(COLOR_RED, "\"有好东西。试一口，什么烦恼都没了。\"\n");
    printf("\n");
    print_colored(COLOR_YELLOW, "1. 同意尝试（进入幻觉状态，高风险）\n");
    print_colored(COLOR_GREEN, "2. 拒绝（坚守底线）\n");
    draw_separator();
     
     int choice = get_valid_input(1, 2);
     
     if (choice == 1) {
         // 同意OD
         print_colored(COLOR_RED, "\n?? 你尝试了对方给的东西...\n");
         print_colored(COLOR_MAGENTA, "意识开始模糊...\n");
         
         player.spirit = 10;
         player.hunger = 100;
         player.health = 50;
         player.sudden_death_chance = 90;
         player.is_hallucinating = true;
         player.hallucination_end = time(NULL) + 60; // 1分钟幻觉
         player.hallu_count++;
         
         print_colored(COLOR_RED, "\n??? 严重警告！\n");
         print_colored(COLOR_YELLOW, "精神降低到10！\n");
         print_colored(COLOR_YELLOW, "进入幻觉状态！文字将变成乱码！\n");
         print_colored(COLOR_YELLOW, "健康降低到50！\n");
         print_colored(COLOR_RED, "猝死几率：90%%！\n");
         
         // 检查猝死
         if (rand() % 100 < player.sudden_death_chance) {
             print_colored(COLOR_RED, "\n??? 你突然感到心脏剧痛...\n");
             print_colored(COLOR_RED, "游戏结束！\n");
             print_colored(COLOR_YELLOW, "可以通过存档恢复...\n");
             
             // 重置玩家（模拟从存档恢复）
             printf("\n正在从存档恢复...\n");
             load_game();
             print_colored(COLOR_GREEN, "已恢复到最近存档！\n");
         }
         
        add_chat_message(player.chatters[chatter_index].name, "下次……注意安全。", player.chatters[chatter_index].type);
    } else {
        print_colored(COLOR_GREEN, "\n?? 你拒绝了。在坤城，有些底线值得守住。\n");
        add_chat_message(player.chatters[chatter_index].name, "有种。", player.chatters[chatter_index].type);
    }
    
    press_any_key();
}

void process_tongcheng_offer(int chatter_index) {
    if (chatter_index < 0 || chatter_index >= player.chatter_count) return;
    if (player.chatters[chatter_index].type != CHATTER_TONGCHENG) return;
    
    CLEAR_SCREEN();
    draw_box("坤城之夜");
    
    print_colored(COLOR_MAGENTA, "%s 发来了一条私信...\n", player.chatters[chatter_index].name);
    printf("\n");
    print_colored(COLOR_MAGENTA, "\"今晚有空吗？一个人在这座城市挺孤单的……\"\n");
    printf("\n");
    print_colored(COLOR_YELLOW, "1. 同意见面\n");
    print_colored(COLOR_GREEN, "2. 礼貌拒绝\n");
    draw_separator();
     
     int choice = get_valid_input(1, 2);
     
    if (choice == 1) {
        print_colored(COLOR_MAGENTA, "\n?? 坤城的夜晚灯火通明……\n");
        print_colored(COLOR_CYAN, "有人在身边，这座城市的孤独感好像淡了一点。\n");
        
        player.spirit = 78;
        player.health = 99;
        
        print_colored(COLOR_GREEN, "精神恢复到78！\n");
        print_colored(COLOR_GREEN, "健康恢复到99！\n");
        
        add_chat_message(player.chatters[chatter_index].name, "下次……如果在坤城又觉得孤单了，来找我。", player.chatters[chatter_index].type);
    } else {
        print_colored(COLOR_GREEN, "\n?? 你礼貌地拒绝了。在坤城，学会拒绝也是一门功课。\n");
        add_chat_message(player.chatters[chatter_index].name, "好吧。坤城这么大，总能再遇到的。", player.chatters[chatter_index].type);
    }
     
     press_any_key();
 }
 
 //================ 存档系统 ================
 void ensure_dir(const char* path) {
#ifdef _WIN32
     _mkdir(path);
#else
     mkdir(path, 0755);
#endif
 }
 
/*=========== v5.1 股票 & 新闻持久化 ===========*/
#define STOCK_SAVE_MAGIC 0x4B54534B   /* "KSTK" */

static void save_stock_state()
{
    char path[512];
    FILE* f;
    int magic = STOCK_SAVE_MAGIC, ver = 1, i;

    ensure_dir(SAVE_DIR);
    snprintf(path, sizeof(path), "%s", STOCK_SAVE_FILE);
    f = fopen(path, "wb");
    if (!f) return;

    fwrite(&magic, sizeof(int), 1, f);
    fwrite(&ver, sizeof(int), 1, f);
    fwrite(&player.stock_day, sizeof(int), 1, f);
    fwrite(&player.stock_tick, sizeof(int), 1, f);
    fwrite(&player.stock_force_closed, sizeof(int), 1, f);
    fwrite(&player.stock_margin_bought, sizeof(int), 1, f);
    fwrite(&player.stock_delist_count, sizeof(int), 1, f);
    fwrite(&player.stock_news_unread, sizeof(int), 1, f);
    fwrite(&player.stock_loan_total, sizeof(money_t), 1, f);
    fwrite(&g_market_mood, sizeof(int), 1, f);
    fwrite(&g_news_n, sizeof(int), 1, f);
    fwrite(&g_news_head, sizeof(int), 1, f);

    for (i = 0; i < STOCK_COUNT; i++) fwrite(&player.stocks[i], sizeof(Stock), 1, f);
    for (i = 0; i < STOCK_COUNT; i++) {
        fwrite(&player.invested_stocks[i], sizeof(int), 1, f);
        fwrite(&player.stock_margin_debt[i], sizeof(money_t), 1, f);
    }
    if (g_news_n > 0) fwrite(g_news, sizeof(NewsItem), (size_t)g_news_n, f);
    fclose(f);
}

static void load_stock_state()
{
    char path[512];
    FILE* f;
    int magic = 0, ver = 0, i;

    snprintf(path, sizeof(path), "%s", STOCK_SAVE_FILE);
    f = fopen(path, "rb");
    if (!f) return;
    if (fread(&magic, sizeof(int), 1, f) != 1 || magic != STOCK_SAVE_MAGIC) { fclose(f); return; }
    if (fread(&ver, sizeof(int), 1, f) != 1) { fclose(f); return; }

    if (fread(&player.stock_day, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_tick, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_force_closed, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_margin_bought, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_delist_count, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_news_unread, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&player.stock_loan_total, sizeof(money_t), 1, f) != 1) { fclose(f); return; }
    if (fread(&g_market_mood, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&g_news_n, sizeof(int), 1, f) != 1) { fclose(f); return; }
    if (fread(&g_news_head, sizeof(int), 1, f) != 1) { fclose(f); return; }

    for (i = 0; i < STOCK_COUNT; i++) {
        if (fread(&player.stocks[i], sizeof(Stock), 1, f) != 1) break;
    }
    for (i = 0; i < STOCK_COUNT; i++) {
        if (fread(&player.invested_stocks[i], sizeof(int), 1, f) != 1) break;
        if (fread(&player.stock_margin_debt[i], sizeof(money_t), 1, f) != 1) break;
    }
    if (g_news_n > MAX_NEWS) g_news_n = MAX_NEWS;
    if (g_news_n > 0) {
        if (fread(g_news, sizeof(NewsItem), (size_t)g_news_n, f) != (size_t)g_news_n)
            g_news_n = 0;
    }
    fclose(f);
}

 void save_game() {
     char filepath[512];

     /* v5.1 股票 / 新闻状态（独立文件，老存档不存在也不影响） */
     save_stock_state();
     
     // 创建目录结构
     ensure_dir(SAVE_DIR);
     snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, "player");
     ensure_dir(filepath);
     snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, "pets");
     ensure_dir(filepath);
     snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, "friends");
     ensure_dir(filepath);
     
     // ——— manifest.work ———
     snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, "manifest.work");
     FILE* f = fopen(filepath, "wb");
     if (f) {
         int v = SAVE_VERSION;
         time_t now = time(NULL);
         fwrite(&v, sizeof(int), 1, f);
         fwrite(player.player_name, sizeof(char), 50, f);
         fwrite(&player.day_count, sizeof(int), 1, f);
         fwrite(&now, sizeof(time_t), 1, f);
         fclose(f);
     } else {
         print_colored(COLOR_RED, "\n? 保存 manifest.work 失败！\n");
     }
     
     // ——— player/core.player (仅标量字段，不含数组) ———
     snprintf(filepath, sizeof(filepath), "%splayer/core.player", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(player.player_name, sizeof(char), 50, f);
         fwrite(&player.kun_coins, sizeof(money_t), 1, f);
         fwrite(&player.kun_exp, sizeof(int), 1, f);
         fwrite(&player.level, sizeof(int), 1, f);
         fwrite(&player.energy, sizeof(int), 1, f);
         fwrite(&player.max_energy, sizeof(int), 1, f);
         fwrite(&player.last_login, sizeof(time_t), 1, f);
         fwrite(&player.furniture_count, sizeof(int), 1, f);
         fwrite(&player.is_billionaire, sizeof(bool), 1, f);
         fwrite(&player.active_event, sizeof(int), 1, f);
         fwrite(&player.event_end_time, sizeof(time_t), 1, f);
         fwrite(&player.consecutive_days, sizeof(int), 1, f);
         fwrite(&player.game_start_time, sizeof(time_t), 1, f);
         fwrite(&player.total_play_time, sizeof(int), 1, f);
         fwrite(&player.skill_points, sizeof(int), 1, f);
         fwrite(&player.pet_count, sizeof(int), 1, f);
         fwrite(&player.current_company_level, sizeof(int), 1, f);
         fwrite(&player.inventory_count, sizeof(int), 1, f);
         fwrite(&player.property_count, sizeof(int), 1, f);
         fwrite(&player.friend_count, sizeof(int), 1, f);
         fwrite(&player.chat_message_count, sizeof(int), 1, f);
         fwrite(&player.chatter_count, sizeof(int), 1, f);
         fwrite(&player.daily_task_completions, sizeof(int), 1, f);
         fwrite(&player.weekly_task_completions, sizeof(int), 1, f);
         fwrite(&player.lifetime_task_completions, sizeof(int), 1, f);
         fwrite(&player.total_income, sizeof(money_t), 1, f);
         fwrite(&player.total_expenses, sizeof(money_t), 1, f);
         fwrite(&player.current_season, sizeof(int), 1, f);
         fwrite(&player.day_count, sizeof(int), 1, f);
         fwrite(&player.prestige_level, sizeof(int), 1, f);
         fwrite(&player.prestige_points, sizeof(int), 1, f);
         fwrite(&player.has_premium, sizeof(bool), 1, f);
         fwrite(&player.premium_until, sizeof(time_t), 1, f);
         fwrite(&player.achievement_points, sizeof(int), 1, f);
         fwrite(&player.lottery_tickets, sizeof(int), 1, f);
         fwrite(&player.mystery_keys, sizeof(int), 1, f);
         fwrite(&player.boss_defeats, sizeof(int), 1, f);
         fwrite(&player.festival_participations, sizeof(int), 1, f);
         fwrite(&player.craft_count, sizeof(int), 1, f);
         fwrite(&player.daily_time_slots, sizeof(int), 1, f);
         fwrite(&player.max_time_slots, sizeof(int), 1, f);
         fwrite(&player.today_income, sizeof(int), 1, f);
         fwrite(&player.today_expenses, sizeof(int), 1, f);
         fwrite(&player.today_work_count, sizeof(int), 1, f);
         fwrite(&player.today_event_count, sizeof(int), 1, f);
         fwrite(&player.last_difficulty, sizeof(int), 1, f);
         fwrite(&player.day_ended, sizeof(bool), 1, f);
         fwrite(&player.rent_cost, sizeof(int), 1, f);
         fwrite(&player.living_cost, sizeof(int), 1, f);
         fwrite(&player.gray_requests_this_session, sizeof(int), 1, f);
         fwrite(&player.lied_to_npc, sizeof(bool), 1, f);
         fwrite(&player.hunger, sizeof(int), 1, f);
         fwrite(&player.health, sizeof(int), 1, f);
         fwrite(&player.spirit, sizeof(int), 1, f);
         fwrite(&player.is_hallucinating, sizeof(bool), 1, f);
         fwrite(&player.hallucination_end, sizeof(time_t), 1, f);
         fwrite(&player.sudden_death_chance, sizeof(int), 1, f);
         fwrite(&player.hallu_count, sizeof(int), 1, f);
         fwrite(&player.has_phone, sizeof(bool), 1, f);
         fwrite(&player.in_chat_room, sizeof(bool), 1, f);
         fwrite(&player.scammers_detected, sizeof(int), 1, f);
         fwrite(&player.insider_profit, sizeof(int), 1, f);
         fwrite(&player.daily_positive_events, sizeof(int), 1, f);
         fwrite(&player.gray_requests_refused, sizeof(int), 1, f);
         fwrite(&player.limited_items_bought, sizeof(int), 1, f);
         fwrite(&player.has_never_lied, sizeof(bool), 1, f);
         fwrite(&player.been_bankrupt, sizeof(bool), 1, f);
         fclose(f);
     } else {
         print_colored(COLOR_RED, "\n? 保存 player/core.player 失败！\n");
     }
     
     // ——— pets/main.dat ———
     snprintf(filepath, sizeof(filepath), "%spets/main.dat", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(&player.pet_count, sizeof(int), 1, f);
         fclose(f);
     }
     // ——— pets/0.pet ~ pets/4.pet ———
     for (int i = 0; i < MAX_PETS; i++) {
         snprintf(filepath, sizeof(filepath), "%spets/%d.pet", SAVE_DIR, i);
         f = fopen(filepath, "wb");
         if (f) {
             fwrite(&player.pets[i].type, sizeof(int), 1, f);
             fwrite(player.pets[i].name, sizeof(char), 30, f);
             fwrite(&player.pets[i].level, sizeof(int), 1, f);
             fwrite(&player.pets[i].experience, sizeof(int), 1, f);
             fwrite(&player.pets[i].happiness, sizeof(int), 1, f);
             fwrite(&player.pets[i].hunger, sizeof(int), 1, f);
             fwrite(&player.pets[i].is_unlocked, sizeof(bool), 1, f);
             fwrite(&player.pets[i].bonus_multiplier, sizeof(float), 1, f);
             fwrite(&player.pets[i].last_fed, sizeof(time_t), 1, f);
             fclose(f);
         }
     }
     
     // ——— friends/main.dat ———
     snprintf(filepath, sizeof(filepath), "%sfriends/main.dat", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(&player.friend_count, sizeof(int), 1, f);
         fclose(f);
     }
     // ——— friends/0.chr ~ friends/9.chr ———
     for (int i = 0; i < MAX_FRIENDS; i++) {
         snprintf(filepath, sizeof(filepath), "%sfriends/%d.chr", SAVE_DIR, i);
         f = fopen(filepath, "wb");
         if (f) {
             fwrite(player.friends[i].username, sizeof(char), 30, f);
             fwrite(&player.friends[i].level, sizeof(int), 1, f);
             fwrite(&player.friends[i].is_online, sizeof(bool), 1, f);
             fwrite(&player.friends[i].last_online, sizeof(time_t), 1, f);
             fwrite(&player.friends[i].friendship_level, sizeof(float), 1, f);
             fwrite(&player.friends[i].is_friend, sizeof(bool), 1, f);
             fclose(f);
         }
     }
     
     // ——— tasks.ts ———
     snprintf(filepath, sizeof(filepath), "%stasks.ts", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(tasks, sizeof(Task), MAX_TASKS, f);
         fwrite(player.tasks_unlocked, sizeof(bool), MAX_TASKS, f);
         fclose(f);
     } else {
         print_colored(COLOR_RED, "\n? 保存 tasks.ts 失败！\n");
     }
     
     // ——— inventory.bag ———
     snprintf(filepath, sizeof(filepath), "%sinventory.bag", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(player.inventory, sizeof(InventoryItem), MAX_INVENTORY, f);
         fwrite(market, sizeof(MarketItem), MAX_MARKET, f);
         fclose(f);
     } else {
         print_colored(COLOR_RED, "\n? 保存 inventory.bag 失败！\n");
     }
     
     // ——— stats.dat ———
     snprintf(filepath, sizeof(filepath), "%sstats.dat", SAVE_DIR);
     f = fopen(filepath, "wb");
     if (f) {
         fwrite(player.achievements, sizeof(Achievement), MAX_ACHIEVEMENTS, f);
         fwrite(&player.scammers_detected, sizeof(int), 1, f);
         fwrite(&player.insider_profit, sizeof(int), 1, f);
         fwrite(&player.daily_positive_events, sizeof(int), 1, f);
         fwrite(&player.gray_requests_refused, sizeof(int), 1, f);
         fwrite(&player.limited_items_bought, sizeof(int), 1, f);
         fwrite(&player.has_never_lied, sizeof(bool), 1, f);
         fwrite(&player.been_bankrupt, sizeof(bool), 1, f);
         fwrite(&player.achievement_points, sizeof(int), 1, f);
         fwrite(&player.boss_defeats, sizeof(int), 1, f);
         fwrite(&player.festival_participations, sizeof(int), 1, f);
         fwrite(&player.craft_count, sizeof(int), 1, f);
         fwrite(&player.total_income, sizeof(money_t), 1, f);
         fwrite(&player.total_expenses, sizeof(money_t), 1, f);
         fwrite(&player.lifetime_task_completions, sizeof(int), 1, f);
         fwrite(&player.lottery_tickets, sizeof(int), 1, f);
         fwrite(&player.mystery_keys, sizeof(int), 1, f);
         fclose(f);
     } else {
         print_colored(COLOR_RED, "\n? 保存 stats.dat 失败！\n");
     }
     
     // ——— forbes.list ———
     update_forbes_ranking();
     
     print_colored(COLOR_GREEN, "\n?? 游戏存档已保存！\n");
 }
 
 void migrate_old_save() {
     // 检查新存档是否已存在
     char manifest_path[512];
     snprintf(manifest_path, sizeof(manifest_path), "%s%s", SAVE_DIR, "manifest.work");
     FILE* check = fopen(manifest_path, "rb");
     if (check) {
         fclose(check);
         return; // 新存档已存在，无需迁移
     }
     
     // 检查旧存档是否存在
     FILE* old = fopen(SAVE_FILE, "rb");
     if (!old) {
         return; // 没有旧存档
     }
     int version;
     fread(&version, sizeof(int), 1, old);
     if (version < 7) {
         fclose(old);
         print_colored(COLOR_YELLOW, "\n?? 旧存档版本过旧，跳过迁移。\n");
         return;
     }
     fread(&player, sizeof(Player), 1, old);
     fclose(old);
     
     // 写入新格式
     print_colored(COLOR_YELLOW, "\n?? 检测到旧存档，正在迁移至新格式...\n");
     save_game();
     
     // 重命名旧档
     char bak_name[256];
     snprintf(bak_name, sizeof(bak_name), "%s.bak", SAVE_FILE);
     rename(SAVE_FILE, bak_name);
     print_colored(COLOR_GREEN, "旧存档已备份为 %s\n", bak_name);
 }
 
 void load_game() {
     char filepath[512];
     
     // 尝试旧存档迁移
     migrate_old_save();
     
     // 检查新存档是否存在
     snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, "manifest.work");
     FILE* f = fopen(filepath, "rb");
     if (!f) {
         print_colored(COLOR_YELLOW, "\n?? 未找到存档，开始新游戏...\n");
         return;
     }
     
     // 验证存档版本
     int file_version;
     fread(&file_version, sizeof(int), 1, f);
     if (file_version != SAVE_VERSION) {
         print_colored(COLOR_YELLOW, "\n?? 存档版本不兼容，将开始新游戏...\n");
         fclose(f);
         return;
     }
     // 读取 manifest 中的玩家名和天数（仅为将来校验用）
     char manifest_name[50];
     int manifest_day_count;
     time_t manifest_save_time;
     fread(manifest_name, sizeof(char), 50, f);
     fread(&manifest_day_count, sizeof(int), 1, f);
     fread(&manifest_save_time, sizeof(time_t), 1, f);
     fclose(f);
     
     // ——— player/core.player ———
     snprintf(filepath, sizeof(filepath), "%splayer/core.player", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(player.player_name, sizeof(char), 50, f);
         fread(&player.kun_coins, sizeof(money_t), 1, f);
         fread(&player.kun_exp, sizeof(int), 1, f);
         fread(&player.level, sizeof(int), 1, f);
         fread(&player.energy, sizeof(int), 1, f);
         fread(&player.max_energy, sizeof(int), 1, f);
         fread(&player.last_login, sizeof(time_t), 1, f);
         fread(&player.furniture_count, sizeof(int), 1, f);
         fread(&player.is_billionaire, sizeof(bool), 1, f);
         fread(&player.active_event, sizeof(int), 1, f);
         fread(&player.event_end_time, sizeof(time_t), 1, f);
         fread(&player.consecutive_days, sizeof(int), 1, f);
         fread(&player.game_start_time, sizeof(time_t), 1, f);
         fread(&player.total_play_time, sizeof(int), 1, f);
         fread(&player.skill_points, sizeof(int), 1, f);
         fread(&player.pet_count, sizeof(int), 1, f);
         fread(&player.current_company_level, sizeof(int), 1, f);
         fread(&player.inventory_count, sizeof(int), 1, f);
         fread(&player.property_count, sizeof(int), 1, f);
         fread(&player.friend_count, sizeof(int), 1, f);
         fread(&player.chat_message_count, sizeof(int), 1, f);
         fread(&player.chatter_count, sizeof(int), 1, f);
         fread(&player.daily_task_completions, sizeof(int), 1, f);
         fread(&player.weekly_task_completions, sizeof(int), 1, f);
         fread(&player.lifetime_task_completions, sizeof(int), 1, f);
         fread(&player.total_income, sizeof(money_t), 1, f);
         fread(&player.total_expenses, sizeof(money_t), 1, f);
         fread(&player.current_season, sizeof(int), 1, f);
         fread(&player.day_count, sizeof(int), 1, f);
         fread(&player.prestige_level, sizeof(int), 1, f);
         fread(&player.prestige_points, sizeof(int), 1, f);
         fread(&player.has_premium, sizeof(bool), 1, f);
         fread(&player.premium_until, sizeof(time_t), 1, f);
         fread(&player.achievement_points, sizeof(int), 1, f);
         fread(&player.lottery_tickets, sizeof(int), 1, f);
         fread(&player.mystery_keys, sizeof(int), 1, f);
         fread(&player.boss_defeats, sizeof(int), 1, f);
         fread(&player.festival_participations, sizeof(int), 1, f);
         fread(&player.craft_count, sizeof(int), 1, f);
         fread(&player.daily_time_slots, sizeof(int), 1, f);
         fread(&player.max_time_slots, sizeof(int), 1, f);
         fread(&player.today_income, sizeof(int), 1, f);
         fread(&player.today_expenses, sizeof(int), 1, f);
         fread(&player.today_work_count, sizeof(int), 1, f);
         fread(&player.today_event_count, sizeof(int), 1, f);
         fread(&player.last_difficulty, sizeof(int), 1, f);
         fread(&player.day_ended, sizeof(bool), 1, f);
         fread(&player.rent_cost, sizeof(int), 1, f);
         fread(&player.living_cost, sizeof(int), 1, f);
         fread(&player.gray_requests_this_session, sizeof(int), 1, f);
         fread(&player.lied_to_npc, sizeof(bool), 1, f);
         fread(&player.hunger, sizeof(int), 1, f);
         fread(&player.health, sizeof(int), 1, f);
         fread(&player.spirit, sizeof(int), 1, f);
         fread(&player.is_hallucinating, sizeof(bool), 1, f);
         fread(&player.hallucination_end, sizeof(time_t), 1, f);
         fread(&player.sudden_death_chance, sizeof(int), 1, f);
         fread(&player.hallu_count, sizeof(int), 1, f);
         fread(&player.has_phone, sizeof(bool), 1, f);
         fread(&player.in_chat_room, sizeof(bool), 1, f);
         fread(&player.scammers_detected, sizeof(int), 1, f);
         fread(&player.insider_profit, sizeof(int), 1, f);
         fread(&player.daily_positive_events, sizeof(int), 1, f);
         fread(&player.gray_requests_refused, sizeof(int), 1, f);
         fread(&player.limited_items_bought, sizeof(int), 1, f);
         fread(&player.has_never_lied, sizeof(bool), 1, f);
         fread(&player.been_bankrupt, sizeof(bool), 1, f);
         fclose(f);
     } else {
         print_colored(COLOR_YELLOW, "\n?? player/core.player 不存在，将使用默认值。\n");
     }
     
     // ——— pets/ ———
     snprintf(filepath, sizeof(filepath), "%spets/main.dat", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(&player.pet_count, sizeof(int), 1, f);
         fclose(f);
         for (int i = 0; i < MAX_PETS; i++) {
             snprintf(filepath, sizeof(filepath), "%spets/%d.pet", SAVE_DIR, i);
             FILE* pf = fopen(filepath, "rb");
             if (pf) {
                 fread(&player.pets[i].type, sizeof(int), 1, pf);
                 fread(player.pets[i].name, sizeof(char), 30, pf);
                 fread(&player.pets[i].level, sizeof(int), 1, pf);
                 fread(&player.pets[i].experience, sizeof(int), 1, pf);
                 fread(&player.pets[i].happiness, sizeof(int), 1, pf);
                 fread(&player.pets[i].hunger, sizeof(int), 1, pf);
                 fread(&player.pets[i].is_unlocked, sizeof(bool), 1, pf);
                 fread(&player.pets[i].bonus_multiplier, sizeof(float), 1, pf);
                 fread(&player.pets[i].last_fed, sizeof(time_t), 1, pf);
                 fclose(pf);
             }
         }
     } else {
         print_colored(COLOR_YELLOW, "\n?? pets/ 不存在，将使用默认值。\n");
     }
     
     // ——— friends/ ———
     snprintf(filepath, sizeof(filepath), "%sfriends/main.dat", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(&player.friend_count, sizeof(int), 1, f);
         fclose(f);
         for (int i = 0; i < MAX_FRIENDS; i++) {
             snprintf(filepath, sizeof(filepath), "%sfriends/%d.chr", SAVE_DIR, i);
             FILE* ff = fopen(filepath, "rb");
             if (ff) {
                 fread(player.friends[i].username, sizeof(char), 30, ff);
                 fread(&player.friends[i].level, sizeof(int), 1, ff);
                 fread(&player.friends[i].is_online, sizeof(bool), 1, ff);
                 fread(&player.friends[i].last_online, sizeof(time_t), 1, ff);
                 fread(&player.friends[i].friendship_level, sizeof(float), 1, ff);
                 fread(&player.friends[i].is_friend, sizeof(bool), 1, ff);
                 fclose(ff);
             }
         }
     } else {
         print_colored(COLOR_YELLOW, "\n?? friends/ 不存在，将使用默认值。\n");
     }
     
     // ——— tasks.ts ———
     snprintf(filepath, sizeof(filepath), "%stasks.ts", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(tasks, sizeof(Task), MAX_TASKS, f);
         fread(player.tasks_unlocked, sizeof(bool), MAX_TASKS, f);
         fclose(f);
     } else {
         print_colored(COLOR_YELLOW, "\n?? tasks.ts 不存在，将使用默认值。\n");
     }
     
     // ——— inventory.bag ———
     snprintf(filepath, sizeof(filepath), "%sinventory.bag", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(player.inventory, sizeof(InventoryItem), MAX_INVENTORY, f);
         fread(market, sizeof(MarketItem), MAX_MARKET, f);
         fclose(f);
     } else {
         print_colored(COLOR_YELLOW, "\n?? inventory.bag 不存在，将使用默认值。\n");
     }
     
     // ——— stats.dat ———
     snprintf(filepath, sizeof(filepath), "%sstats.dat", SAVE_DIR);
     f = fopen(filepath, "rb");
     if (f) {
         fread(player.achievements, sizeof(Achievement), MAX_ACHIEVEMENTS, f);
         fread(&player.scammers_detected, sizeof(int), 1, f);
         fread(&player.insider_profit, sizeof(int), 1, f);
         fread(&player.daily_positive_events, sizeof(int), 1, f);
         fread(&player.gray_requests_refused, sizeof(int), 1, f);
         fread(&player.limited_items_bought, sizeof(int), 1, f);
         fread(&player.has_never_lied, sizeof(bool), 1, f);
         fread(&player.been_bankrupt, sizeof(bool), 1, f);
         fread(&player.achievement_points, sizeof(int), 1, f);
         fread(&player.boss_defeats, sizeof(int), 1, f);
         fread(&player.festival_participations, sizeof(int), 1, f);
         fread(&player.craft_count, sizeof(int), 1, f);
         fread(&player.total_income, sizeof(money_t), 1, f);
         fread(&player.total_expenses, sizeof(money_t), 1, f);
         fread(&player.lifetime_task_completions, sizeof(int), 1, f);
         fread(&player.lottery_tickets, sizeof(int), 1, f);
         fread(&player.mystery_keys, sizeof(int), 1, f);
         fclose(f);
     } else {
         print_colored(COLOR_YELLOW, "\n?? stats.dat 不存在，将使用默认值。\n");
     }
     
    // v4.1 存档安全性检查：清除过期/损坏的事件数据
    if (player.active_event != EVENT_NONE) {
        time_t now = time(NULL);
        // 如果 event_end_time 为0或者已过期，清除事件状态
        if (player.event_end_time <= 0 || player.event_end_time <= now) {
            print_colored(COLOR_YELLOW, "\n?? 检测到过期事件（%s），已自动清除。\n", get_event_name(player.active_event));
            player.active_event = EVENT_NONE;
            player.event_end_time = 0;
        }
    }
    
     print_colored(COLOR_GREEN, "\n?? 游戏存档已加载！\n");

     /* v5.1 股票 / 新闻状态 */
     load_stock_state();
 }

//================ 福布斯排行榜系统 ================
 ForbesRankingEntry forbes_list[MAX_FORBES_ENTRIES];
 int forbes_count = 0;

void init_default_forbes_ranking() {
    // 预设福布斯名人榜
    struct {
        const char* name;
        money_t coins;
        int level;
    } default_celebrities[] = {
        {"马斯克", 999999999999LL, 99},
        {"比尔盖茨", 888888888888LL, 99},
        {"贝索斯", 777777777777LL, 99},
        {"巴菲特", 666666666666LL, 99},
        {"马云", 555555555555LL, 99},
        {"扎克伯格", 444444444444LL, 99},
        {"库克", 333333333333LL, 99},
        {"马化腾", 222222222222LL, 99},
        {"李嘉诚", 111111111111LL, 99},
        {"钟睒睒", 100000000000LL, 99}
    };
    
    int celeb_count = sizeof(default_celebrities) / sizeof(default_celebrities[0]);
    forbes_count = celeb_count;
    
    for (int i = 0; i < celeb_count; i++) {
        safe_strcpy(forbes_list[i].player_name, default_celebrities[i].name, 50);
        forbes_list[i].kun_coins = default_celebrities[i].coins;
        forbes_list[i].level = default_celebrities[i].level;
        forbes_list[i].record_time = time(NULL);
        forbes_list[i].is_active = false; // 名人标记为非活跃
    }
}

void load_forbes_ranking() {
    // 先读取玩家数据（临时保存）
    ForbesRankingEntry player_entries[MAX_FORBES_ENTRIES];
    int player_count = 0;
    
    FILE* file = fopen(FORBES_FILE, "rb");
    if (file) {
        fread(&player_count, sizeof(int), 1, file);
        if (player_count > 0 && player_count <= MAX_FORBES_ENTRIES) {
            fread(player_entries, sizeof(ForbesRankingEntry), player_count, file);
        }
        fclose(file);
    }
    
    // 初始化预设名人（硬编码）
    init_default_forbes_ranking();
    
    // 合并玩家数据到名人后面
    for (int i = 0; i < player_count && forbes_count < MAX_FORBES_ENTRIES; i++) {
        forbes_list[forbes_count++] = player_entries[i];
    }
}

 void save_forbes_ranking() {
     FILE* file = fopen(FORBES_FILE, "wb");
     if (file) {
         fwrite(&forbes_count, sizeof(int), 1, file);
         fwrite(forbes_list, sizeof(ForbesRankingEntry), forbes_count, file);
         fclose(file);
     }
 }

 void update_forbes_ranking() {
     // 查找是否已有该玩家的记录
     int existing_index = -1;
     for (int i = 0; i < forbes_count; i++) {
         if (strcmp(forbes_list[i].player_name, player.player_name) == 0) {
             existing_index = i;
             break;
         }
     }

     if (existing_index >= 0) {
         // 更新现有记录
         forbes_list[existing_index].kun_coins = player.kun_coins;
         forbes_list[existing_index].level = player.level;
         forbes_list[existing_index].record_time = time(NULL);
     } else {
         // 添加新记录
         if (forbes_count < MAX_FORBES_ENTRIES) {
             safe_strcpy(forbes_list[forbes_count].player_name, player.player_name, 50);
             forbes_list[forbes_count].kun_coins = player.kun_coins;
             forbes_list[forbes_count].level = player.level;
             forbes_list[forbes_count].record_time = time(NULL);
             forbes_list[forbes_count].is_active = true;
             forbes_count++;
         }
     }

     // 按坤币数量排序（冒泡排序）
     for (int i = 0; i < forbes_count - 1; i++) {
         for (int j = 0; j < forbes_count - i - 1; j++) {
             if (forbes_list[j].kun_coins < forbes_list[j + 1].kun_coins) {
                 ForbesRankingEntry temp = forbes_list[j];
                 forbes_list[j] = forbes_list[j + 1];
                 forbes_list[j + 1] = temp;
             }
         }
     }

     save_forbes_ranking();
 }

 void show_forbes_ranking() {
     CLEAR_SCREEN();

     printf(COLOR_YELLOW COLOR_BOLD);
     printf("\n  ╔═══════════════════════════════════════════════════════════╗\n");
     printf("  ║                                                           ║\n");
     printf("  ║                    ?? 福布斯坤币排行榜 ??                   ║\n");
     printf("  ║                                                           ║\n");
     printf("  ╚═══════════════════════════════════════════════════════════╝\n");
     printf(COLOR_RESET "\n");

     if (forbes_count == 0) {
         printf("  暂无排行榜数据！\n");
     } else {
         // 显示前10名
         int display_count = (forbes_count < 10) ? forbes_count : 10;

         printf(COLOR_BOLD "  排名   %-15s %15s  %5s\n" COLOR_RESET,
                "玩家名称", "坤币", "等级");
         printf("  ─────────────────────────────────────────────────────\n");

         for (int i = 0; i < display_count; i++) {
             ForbesRankingEntry* entry = &forbes_list[i];

             // 根据排名设置颜色
             const char* rank_color = COLOR_WHITE;
             if (i == 0) rank_color = COLOR_YELLOW;  // 第一名金色
             else if (i == 1) rank_color = COLOR_CYAN;  // 第二名银色
             else if (i == 2) rank_color = COLOR_MAGENTA;  // 第三名铜色

             printf("  %s%3d.%s ", rank_color, i + 1, COLOR_RESET);

             // 高亮当前玩家
             if (strcmp(entry->player_name, player.player_name) == 0) {
                 printf(COLOR_GREEN "%-15s" COLOR_RESET, entry->player_name);
             } else {
                 printf("%-15s", entry->player_name);
             }

             printf(" %15lld  %5d\n", entry->kun_coins, entry->level);
         }

         if (forbes_count > 10) {
             printf("\n  ... 还有 %d 名玩家未显示\n", forbes_count - 10);
         }

         // 显示玩家当前排名
         printf("\n  ");
         for (int i = 0; i < forbes_count; i++) {
             if (strcmp(forbes_list[i].player_name, player.player_name) == 0) {
                 print_colored(COLOR_CYAN, "  你目前的排名：第 %d 名\n", i + 1);
                 break;
             }
         }
     }

     printf("\n");
     press_any_key();
 }

 void show_forbes_ranking_menu() {
     int choice;

     do {
         CLEAR_SCREEN();
         printf(COLOR_BOLD);
         printf("\n  ╔═══════════════════════════════════════════════════════════╗\n");
         printf("  ║                    ?? 福布斯排行榜 ??                      ║\n");
         printf("  ╚═══════════════════════════════════════════════════════════╝\n");
         printf(COLOR_RESET);

         printf("\n  1. ?? 查看排行榜\n");
         printf("  2. ?? 更新我的排名\n");
         printf("  3. ?? 我的排名信息\n");
         printf("\n  0. ?? 返回\n");

         printf("\n  请选择: ");
         choice = get_valid_input(0, 3);

         switch (choice) {
             case 1:
                 show_forbes_ranking();
                 break;
             case 2:
                 update_forbes_ranking();
                 print_colored(COLOR_GREEN, "\n  排名已更新！\n");
                 press_any_key();
                 break;
             case 3: {
                 CLEAR_SCREEN();
                 printf(COLOR_BOLD "\n  我的排名信息\n" COLOR_RESET);
                 printf(COLOR_CYAN);
                 printf("  ═══════════════════════════════════════\n");
                 printf("  玩家名称: %s\n", player.player_name);
                 printf("  当前坤币: %lld\n", player.kun_coins);
                 printf("  当前等级: %d\n", player.level);
                 printf(COLOR_RESET);

                 for (int i = 0; i < forbes_count; i++) {
                     if (strcmp(forbes_list[i].player_name, player.player_name) == 0) {
                         print_colored(COLOR_GREEN, "  福布斯排名: 第 %d 名（共 %d 名玩家）\n", i + 1, forbes_count);
                         break;
                     }
                 }
                 printf("\n");
                 press_any_key();
                 break;
             }
         }
     } while (choice != 0);
 }
 
 //================ 事件系统 ================
 const char* get_event_name(EventType type) {
     const char* names[] = {
         "无", "通货膨胀", "股市大涨", "任务奖励", "免费家具",
         "彩票加成", "双倍经验", "神秘宝箱", "BOSS挑战", "节日庆典"
     };
     if (type < 0 || type >= EVENT_TYPE_COUNT) return "未知";
     return names[type];
 }
 
 const char* get_event_description(EventType type) {
     const char* descriptions[] = {
         "无事件发生",
         "所有物品价格上涨20%",
         "股票收益增加30%",
         "任务奖励增加50%",
         "随机获得一件家具",
         "彩票中奖率翻倍",
         "获得经验值翻倍",
         "开启神秘宝箱获得稀有物品",
         "挑战特殊BOSS获得大量奖励",
         "节日期间所有活动奖励增加"
     };
     if (type < 0 || type >= EVENT_TYPE_COUNT) return "";
     return descriptions[type];
 }
 
 void random_event() {
     // 15%概率触发事件
     if ((rand() % 100) < 15 && player.active_event == EVENT_NONE) {
         EventType new_event = (EventType)(1 + rand() % (EVENT_TYPE_COUNT - 1));
         trigger_event(new_event);
         print_colored(COLOR_MAGENTA, "\n?? 随机事件触发：%s！\n", get_event_name(new_event));
         print_colored(COLOR_CYAN, "   %s\n", get_event_description(new_event));
         press_any_key();
     }
 }
 
 void trigger_event(EventType type) {
     player.active_event = type;
     player.event_end_time = time(NULL) + 300; // 5分钟
     
     handle_event_effect(type, true);
 }
 
 void end_event() {
     if (player.active_event != EVENT_NONE) {
         handle_event_effect(player.active_event, false);
         print_colored(COLOR_CYAN, "\n? 事件结束：%s\n", get_event_name(player.active_event));
         player.active_event = EVENT_NONE;
         player.event_end_time = 0;
     }
 }
 
 void handle_event_effect(EventType type, bool apply) {
     static int original_prices[MAX_MARKET] = {0};
     static bool prices_saved = false;
     
     switch(type) {
         case EVENT_INFLATION:
             if (apply && !prices_saved) {
                 for (int i = 0; i < MAX_MARKET; i++) {
                     original_prices[i] = market[i].price;
                     market[i].price = (int)(market[i].price * 1.2f);
                 }
                 prices_saved = true;
             } else if (!apply && prices_saved) {
                 for (int i = 0; i < MAX_MARKET; i++) {
                     market[i].price = original_prices[i];
                 }
                 prices_saved = false;
             }
             break;
         case EVENT_FREE_FURNITURE:
             if (apply && player.furniture_count < MAX_FURNITURE) {
                 const char* free_furniture[] = {"神秘宝箱", "幸运地毯", "祝福花瓶", "招财猫"};
                 int idx = rand() % 4;
                 safe_strcpy(player.furniture[player.furniture_count++], free_furniture[idx], 50);
                 print_colored(COLOR_GREEN, "   ?? 获得免费家具：%s！\n", free_furniture[idx]);
             }
             break;
         case EVENT_MYSTERY_BOX:
             if (apply) {
                 player.mystery_keys += 2;
                 print_colored(COLOR_GREEN, "   ?? 获得2把神秘钥匙！\n");
             }
             break;
         case EVENT_BOSS_CHALLENGE:
             if (apply) {
                 print_colored(COLOR_YELLOW, "   ?? 强大的BOSS出现了！挑战成功可获得丰厚奖励！\n");
             }
             break;
         case EVENT_FESTIVAL:
             if (apply) {
                 print_colored(COLOR_YELLOW, "   ?? 节日庆典开始！参与活动获得双倍奖励！\n");
             }
             break;
         default:
             break;
     }
 }
 
void show_active_events() {
    if (player.active_event != EVENT_NONE) {
        time_t now = time(NULL);
        time_t remaining = player.event_end_time - now;
        // 安全性检查：event_end_time 必须大于0，剩余时间必须在合理范围内
        // 事件最长持续时间只有300秒（5分钟），超过3600秒肯定异常
        if (player.event_end_time > 0 && remaining > 0 && remaining < 3600) {
            print_colored(COLOR_MAGENTA, "\n?? 当前事件：%s\n", get_event_name(player.active_event));
            printf("   剩余时间：%lld秒\n", (long long)remaining);
            printf("   效果：%s\n", get_event_description(player.active_event));
        } else {
            // 事件时间异常（如存档残留的过期事件或数据损坏），强制清除
            if (player.active_event != EVENT_NONE) {
                handle_event_effect(player.active_event, false);
            }
            player.active_event = EVENT_NONE;
            player.event_end_time = 0;
        }
    }
}
 
 //================ 状态效果处理 ================
 void check_hallucination() {
     if (player.is_hallucinating) {
         if (time(NULL) >= player.hallucination_end) {
             clear_hallucination();
         } else {
             // 幻觉持续期间的效果
             if (player.spirit < 20 && player.hallu_count == 1 &&
                 (rand() % 100) < 10) { // 10%概率额外影响
                 printf(COLOR_RED "\n?? 你的视线开始模糊...\n" COLOR_RESET);
             }
         }
     }
 }
 
 void clear_hallucination() {
     player.is_hallucinating = false;
     player.hallucination_end = 0;
     print_colored(COLOR_GREEN, "\n?? 幻觉效果消失了，你的意识恢复清醒。\n");
 }
 
 void apply_hallucination_effect() {
     if (!player.is_hallucinating) return;
     
     // 幻觉时选项变乱的效果（在菜单显示时处理）
     // 这里只是视觉效果
 }
 
 void process_spirit_effects() {
     // 精神大于78时的异常兴奋
     if (player.spirit > 78) {
         static time_t last_effect = 0;
         time_t now = time(NULL);
         
         if (now - last_effect > 30) { // 每30秒检查一次
             last_effect = now;
             print_colored(COLOR_YELLOW, "\n?? 你感到异常兴奋！\n");
             print_colored(COLOR_RED, "警告：过度兴奋可能危及生命！\n");
             
             // 猝死风险
             int risk = (player.spirit - 78) * 2;
             if (rand() % 100 < risk) {
                 print_colored(COLOR_RED, "\n??? 你的心脏承受不了这种刺激...\n");
                 print_colored(COLOR_RED, "游戏结束！\n");
                 print_colored(COLOR_YELLOW, "从存档恢复...\n");
                 load_game();
             }
         }
     }
     
    // 精神自动恢复（缓慢）- 每60秒恢复1点
    if (player.spirit < 50 && !player.is_hallucinating && player.health > 30) {
        static time_t last_spirit_recovery = 0;
        time_t now = time(NULL);
        if (last_spirit_recovery == 0) {
            last_spirit_recovery = now;
        }
        if (now - last_spirit_recovery >= 60) {
            last_spirit_recovery = now;
            player.spirit++;
        }
    }
     
     // 限制精神范围
     if (player.spirit > 100) player.spirit = 100;
     if (player.spirit < 0) player.spirit = 0;
 }
 
void recover_health() {
    static time_t last_health_recovery = 0;
    time_t now = time(NULL);
    
    if (last_health_recovery == 0) {
        last_health_recovery = now;
    }
    
    // 每小时恢复5点血量
    int hours_passed = (int)((now - last_health_recovery) / 3600);
    if (hours_passed > 0) {
        // 恢复血量
        if (player.health < 100) {
            player.health += hours_passed * 5;
            if (player.health > 100) player.health = 100;
            print_colored(COLOR_GREEN, "\n?? 自然恢复：+%d 血量\n", hours_passed * 5);
        }
        
        // ===== 完善游戏系统：饱食度自然下降 =====
        // 每小时饱食度下降3点
        int hunger_loss = hours_passed * 3;
        if (player.hunger > 0) {
            player.hunger -= hunger_loss;
            if (player.hunger < 0) player.hunger = 0;
            print_colored(COLOR_YELLOW, "\n?? 饥饿：饱食度-%d\n", hunger_loss);
            
            if (player.hunger <= 0) {
                print_colored(COLOR_RED, "\n??? 你已经饿昏了！\n");
                // 饿昏了扣血
                player.health -= 5;
                if (player.health < 0) player.health = 0;
            } else if (player.hunger < 20) {
                print_colored(COLOR_RED, "你非常饥饿！快去买食物！\n");
            }
        }
        
        // 精神值缓慢恢复
        if (player.spirit < 100) {
            player.spirit += hours_passed * 2;
            if (player.spirit > 100) player.spirit = 100;
        }
        
        last_health_recovery = now;
    }
}
 
 void buy_food() {
     CLEAR_SCREEN();
     draw_box("购买食物");
     
     printf("可购买的食物：\n");
     printf("1. 面包 - 10坤币（恢复15饱食度）\n");
     printf("2. 盒饭 - 30坤币（恢复30饱食度）\n");
     printf("3. 大餐 - 100坤币（恢复60饱食度）\n");
     printf("0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     
     switch (choice) {
         case 1:
             if (remove_coins(10)) {
                 player.hunger += 15;
                 if (player.hunger > 100) player.hunger = 100;
                 print_colored(COLOR_GREEN, "购买了面包，饱食度+15\n");
             } else {
                 print_colored(COLOR_RED, "坤币不足！\n");
             }
             break;
         case 2:
             if (remove_coins(30)) {
                 player.hunger += 30;
                 if (player.hunger > 100) player.hunger = 100;
                 print_colored(COLOR_GREEN, "购买了盒饭，饱食度+30\n");
             } else {
                 print_colored(COLOR_RED, "坤币不足！\n");
             }
             break;
         case 3:
             if (remove_coins(100)) {
                 player.hunger += 60;
                 if (player.hunger > 100) player.hunger = 100;
                 print_colored(COLOR_GREEN, "购买了大餐，饱食度+60\n");
             } else {
                 print_colored(COLOR_RED, "坤币不足！\n");
             }
             break;
         default:
             break;
     }
     
     press_any_key();
 }
 
 void eat_food() {
     CLEAR_SCREEN();
     draw_box("背包食物");
     
     // 查找食物物品
     int food_indices[10];
     int food_count = 0;
     
     for (int i = 0; i < player.inventory_count && food_count < 10; i++) {
         if (strcmp(player.inventory[i].name, "面包") == 0 ||
             strcmp(player.inventory[i].name, "盒饭") == 0) {
             food_indices[food_count++] = i;
         }
     }
     
     if (food_count == 0) {
         printf("背包中没有食物！\n");
         printf("可以在商城中购买食物。\n");
         press_any_key();
         return;
     }
     
     for (int i = 0; i < food_count; i++) {
         InventoryItem* item = &player.inventory[food_indices[i]];
         printf("%d. %s x%d - 恢复%d饱食度\n", i + 1, item->name, item->quantity, item->effect_value);
     }
     printf("0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, food_count);
     if (choice > 0) {
         InventoryItem* item = &player.inventory[food_indices[choice - 1]];
         
         player.hunger += item->effect_value;
         if (player.hunger > 100) player.hunger = 100;
         
         item->quantity--;
         if (item->quantity <= 0) {
             for (int j = food_indices[choice - 1]; j < player.inventory_count - 1; j++) {
                 player.inventory[j] = player.inventory[j + 1];
             }
             player.inventory_count--;
         }
         
         print_colored(COLOR_GREEN, "食用了 %s，饱食度+%d\n", item->name, item->effect_value);
         press_any_key();
     }
 }
 
 //================ 意外系统 ================
void random_accident() {
    // 10%概率触发意外事件
    if ((rand() % 100) < 10 && player.health > 0) {
        int damage = 5 + rand() % 10;
        player.health -= damage;
        if (player.health < 0) player.health = 0;
        
        const char* accidents[] = {
            "送外卖时被电动车蹭了一下",
            "加班到凌晨低血糖犯了",
            "老城区那截楼梯没灯，踩空了",
            "工地上被掉落的木板砸到"
        };
        int idx = rand() % 4;
        print_colored(COLOR_RED, "\n?? 坤城的意外：%s —— 血量-%d！\n", accidents[idx], damage);
        
        if (player.health <= 0) {
            print_colored(COLOR_RED, "\n??? 你在坤城的街头倒下了……\n");
            print_colored(COLOR_YELLOW, "好在被人及时发现，送进了医院。\n");
            print_colored(COLOR_GREEN, "从最近的存档恢复……\n");
            load_game();
        }
    }
     
     // 恢复血量（如果健康且未受伤）
     if (player.health < 100 && player.health > 0) {
         int recovery = 1 + rand() % 3;
         player.health += recovery;
         if (player.health > 100) player.health = 100;
         if (recovery > 0 && (rand() % 100) < 5) {
             print_colored(COLOR_GREEN, "\n?? 身体自然恢复 +%d 血量\n", recovery);
         }
     }
 }
 
 //================ 聊天室功能 ================
void show_phone_menu() {
    if (!player.has_phone) {
        print_colored(COLOR_RED, "你还没有手机！\n");
        print_colored(COLOR_YELLOW, "可以在商城中购买手机。\n");
        press_any_key();
        return;
    }
    
    while (1) {
        CLEAR_SCREEN();
        draw_box("手机");
        
        printf("1. 进入聊天室\n");
        printf("2. 查看聊天记录\n");
        printf("0. 返回\n");
        draw_separator();
        
        int choice = get_valid_input(0, 2);
        
        switch (choice) {
            case 1:
                show_chat_room();
                break;
            case 2:
                CLEAR_SCREEN();
                draw_box("聊天记录");
                show_chat_messages();
                press_any_key();
                break;
            case 0:
                return;
        }
    }
}
 
/*================ v5.0 TUI 聊天室 ================*/
static void chat_draw(int waiting)
{
    int W, H, i, online = 0;
    int list_w = 24;
    int mw, cy, ch, row;

    tui_begin();
    W = tui_cv.w;
    H = tui_cv.h;
    if (W < 86) list_w = 0;

    for (i = 0; i < player.chatter_count; i++)
        if (player.chatters[i].is_online) online++;

    tui_draw_topbar();

    mw = list_w ? (W - list_w - 1) : W;
    cy = 1;
    ch = H - 3;
    if (ch < 6) ch = 6;

    /* ---- 消息面板 ---- */
    {
        char ttl[80];
        snprintf(ttl, sizeof ttl, "坤城聊天室 · 在线 %d/%d", online, player.chatter_count);
        tc_panel(0, cy, mw, ch, ttl, TC_BORDER, TC_BG2, TC_CYAN);
    }

    /* 从底部往上铺消息，最新的贴着面板底边框内侧 */
    row = ch - 3;
    for (i = player.chat_message_count - 1; i >= 0 && row >= 0; i--) {
        ChatMessage* m = &player.chat_history[i];
        int fg = TC_TEXT;
        char ts[16];
        struct tm* lt;
        int y = cy + 1 + row;

        switch (m->type) {
            case CHATTER_SCAMMER:     fg = TC_RED; break;
            case CHATTER_BLACKMARKET: fg = TC_ORANGE; break;
            case CHATTER_TONGCHENG:   fg = TC_MAGENTA; break;
            default:                  fg = TC_TEXT; break;
        }
        if (strcmp(m->sender, "系统") == 0) fg = TC_YELLOW;
        else if (strcmp(m->sender, "你") == 0) fg = TC_CYAN;

        lt = localtime(&m->timestamp);
        if (lt) snprintf(ts, sizeof ts, "%02d:%02d", lt->tm_hour, lt->tm_min);
        else    snprintf(ts, sizeof ts, "--:--");

        tc_puts(2, y, ts, TC_FAINT, TC_BG2);
        tc_putsn(9, y, 10, m->sender, fg, TC_BG2);
        tc_putsn(20, y, mw - 22, m->content, fg, TC_BG2);
        row--;
    }

    if (player.chat_message_count == 0)
        tc_puts(3, cy + 1, "还没有人说话。试着发一条，或者 @某人 搭话。", TC_FAINT, TC_BG2);

    /* ---- 在线列表 ---- */
    if (list_w) {
        tc_panel(W - list_w, cy, list_w, ch, "在线名单", TC_BORDER, TC_BG2, TC_CYAN);
        {
            int r = 0;
            for (i = 0; i < player.chatter_count && r < ch - 2; i++) {
                Chatter* ct = &player.chatters[i];
                int fg;
                if (ct->type == CHATTER_SCAMMER)          fg = ct->is_online ? TC_RED     : TC_REDD;
                else if (ct->type == CHATTER_BLACKMARKET) fg = ct->is_online ? TC_ORANGE  : TC_FAINT;
                else if (ct->type == CHATTER_TONGCHENG)   fg = ct->is_online ? TC_MAGENTA : TC_FAINT;
                else                                      fg = ct->is_online ? TC_TEXT    : TC_FAINT;

                tc_put(W - list_w + 2, cy + 1 + r, ct->is_online ? "*" : "-", fg, TC_BG2);
                tc_putsn(W - list_w + 4, cy + 1 + r, list_w - 6, ct->name, fg, TC_BG2);
                r++;
            }
            if (r == 0)
                tc_puts(W - list_w + 2, cy + 1, "没人在线", TC_FAINT, TC_BG2);
        }
    }

    /* ---- 底部 ---- */
    if (waiting >= 0 && waiting < player.chatter_count) {
        char wb[160];
        snprintf(wb, sizeof wb, " %s 正等你回话 —— 输入「同意」或「拒绝」，其他内容会被忽略 ",
                 player.chatters[waiting].name);
        tc_fill(0, H - 2, W, 1, " ", TC_BLACK, TC_YELLOW);
        tc_putsn(1, H - 2, W - 2, wb, TC_BLACK, TC_YELLOW);
        tc_fill(0, H - 1, W, 1, " ", TC_TEXT, TC_BG3);
    } else {
        tui_draw_keybar(" [回车] 发送   [@名字 内容] 搭话   [/quit] 或 [Esc] 退出   ");
    }
}

void show_chat_room() {
    char input[200];
    static time_t last_auto_chat = 0;
    static int waiting_for_agreement = -1;

    const char* auto_messages[] = {
        "今天坤城天气不错",
        "有人在吗？好无聊",
        "我刚发现老城区有家店不错",
        "你们觉得在坤城能混出头吗？",
        "有没有人想聊天？",
        "工作好累啊，CBD那边又堵车了",
        "有没有人想一起做任务？",
        "听说开发区那边在招人",
        "我有个好东西，有人要吗？",
        "你们都是在坤城哪里打工的？",
        "大学城那边又开新店了",
        "今天股市你们关注了吗？"
    };
    int num_auto_messages = (int)(sizeof(auto_messages) / sizeof(auto_messages[0]));

    const char* system_messages[] = {
        "聊天室定时清理完成",
        "欢迎新用户加入",
        "请注意文明聊天",
        "服务器即将维护，请保存聊天记录"
    };
    int num_system_messages = (int)(sizeof(system_messages) / sizeof(system_messages[0]));

    tui_page_enter("聊天室");

    for (;;) {
        time_t now = time(NULL);
        int nc = player.chatter_count > 0 ? player.chatter_count : 1;

        /* ---- 自动有人说话 ---- */
        if (player.chatter_count > 0 &&
            (now - last_auto_chat >= 8 || (rand() % 100 < 35 && now - last_auto_chat >= 3))) {
            int ci = rand() % nc;
            if (player.chatters[ci].is_online) {
                add_chat_message(player.chatters[ci].name,
                                 auto_messages[rand() % num_auto_messages],
                                 player.chatters[ci].type);
            }
            last_auto_chat = now;
        }
        /* ---- 系统消息 ---- */
        if (rand() % 100 < 12) {
            add_chat_message("系统", system_messages[rand() % num_system_messages], CHATTER_NORMAL);
        }

        if (waiting_for_agreement >= player.chatter_count) waiting_for_agreement = -1;

        chat_draw(waiting_for_agreement);

        if (tui_line_input_native(tui_cv.h - 1, tui_cv.w, " 说点什么 › ", input, sizeof input) < 0) {
            break;
        }

        /* ---- 空白检查 ---- */
        {
            bool all_spaces = true;
            int i;
            for (i = 0; input[i]; i++) {
                if (input[i] != ' ' && input[i] != '\t') { all_spaces = false; break; }
            }
            if (input[0] == 0 || all_spaces) {
                add_chat_message("系统", "不能发送空白消息！", CHATTER_NORMAL);
                continue;
            }
        }

        if (strcmp(input, "/quit") == 0 || strcmp(input, "/q") == 0) break;

        /* ---- 等对方回复时，只认同意 / 拒绝 ---- */
        if (waiting_for_agreement >= 0) {
            if (strcmp(input, "同意") == 0) {
                ChatterType type = player.chatters[waiting_for_agreement].type;
                add_chat_message("你", "同意", CHATTER_NORMAL);

                if (type == CHATTER_SCAMMER) {
                    if (player.kun_coins >= 100) {
                        player.kun_coins -= 100;
                        add_chat_message(player.chatters[waiting_for_agreement].name,
                                         "哈哈，谢谢你的100坤币！你真是个好人~", type);
                    } else {
                        add_chat_message(player.chatters[waiting_for_agreement].name,
                                         "哼，没钱还说什么同意？滚！", type);
                    }
                } else if (type == CHATTER_BLACKMARKET) {
                    process_od_offer(waiting_for_agreement);
                } else if (type == CHATTER_TONGCHENG) {
                    process_tongcheng_offer(waiting_for_agreement);
                } else {
                    add_chat_message(player.chatters[waiting_for_agreement].name,
                                     "谢谢你的同意！你真好~", type);
                }
                waiting_for_agreement = -1;
                continue;
            } else if (strcmp(input, "拒绝") == 0) {
                add_chat_message("你", "拒绝", CHATTER_NORMAL);
                add_chat_message(player.chatters[waiting_for_agreement].name, "好吧，没关系...",
                                 player.chatters[waiting_for_agreement].type);
                waiting_for_agreement = -1;
                continue;
            } else {
                add_chat_message("系统", "请输入 同意 或 拒绝", CHATTER_NORMAL);
                continue;
            }
        }

        /* ---- 发出去 ---- */
        add_chat_message("你", input, CHATTER_NORMAL);

        if (input[0] == '@') {
            char target_name[30];
            char message[170];
            int hit = 0, i;

            message[0] = 0;
            if (sscanf(input, "@%29s %169[^\n]", target_name, message) >= 1) {
                for (i = 0; i < player.chatter_count; i++) {
                    if (strcmp(player.chatters[i].name, target_name) == 0) {
                        hit = 1;
                        ai_chatter_response(i, message[0] ? message : "你好");

                        if (rand() % 100 < 50) {
                            const char* requests[] = {
                                "我能向你借点钱吗？",
                                "你能帮我做个任务吗？",
                                "我想和你做个交易，可以吗？",
                                "你能帮我个忙吗？"
                            };
                            add_chat_message(player.chatters[i].name,
                                             requests[rand() % 4],
                                             player.chatters[i].type);
                            waiting_for_agreement = i;
                        }

                        if (player.chatters[i].type == CHATTER_BLACKMARKET &&
                            (strstr(message, "OD") || strstr(message, "药物") ||
                             strstr(message, "试试") || strstr(message, "东西"))) {
                            process_od_offer(i);
                        }
                        if (player.chatters[i].type == CHATTER_TONGCHENG &&
                            (strstr(message, "约") || strstr(message, "见面") ||
                             strstr(message, "出来") || strstr(message, "一起"))) {
                            process_tongcheng_offer(i);
                        }
                        break;
                    }
                }
                if (!hit) add_chat_message("系统", "聊天室里没有这个人。", CHATTER_NORMAL);
            } else {
                add_chat_message("系统", "格式错误：请使用 @用户名 内容", CHATTER_NORMAL);
            }
        } else {
            if (rand() % 100 < 12) {
                int ci = rand() % nc;
                if (player.chatters[ci].is_online) {
                    ai_chatter_response(ci, input);
                }
            }
        }
    }

    tui_stream_reset();
}
 
 //================ 成就系统 ================
 void check_achievements() {
     for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
         if (player.achievements[i].unlocked) continue;
         
         bool condition_met = false;
         
         switch(i) {
             case 0: condition_met = player.kun_coins >= 100; break;
             case 1: condition_met = player.furniture_count >= 5; break;
             case 2: condition_met = player.lifetime_task_completions >= 50; break;
             case 3: condition_met = (player.total_income - player.total_expenses) >= 1000; break;
             case 4: condition_met = (player.craft_count >= 1); break;  // 幸运儿：至少参与过一次合成（备用，主触发在lottery）
             case 5: condition_met = player.kun_coins >= 1000000; break;
             case 6: condition_met = player.level >= 50; break;
             case 7: {
                 for (int j = 0; j < MAX_SKILLS; j++) {
                     if (player.skills[j].level >= 10) {
                         condition_met = true;
                         break;
                     }
                 }
                 break;
             }
             case 8: condition_met = player.lifetime_task_completions >= 1000; break;
             case 9: condition_met = player.friend_count >= 5; break;
             case 10: condition_met = player.pet_count >= 3; break;
             case 11: condition_met = player.property_count >= 5; break;
             case 12: {
                 condition_met = true;
                 for (int j = 0; j < MAX_SKILLS; j++) {
                     if (player.skills[j].level < 5) {
                         condition_met = false;
                         break;
                     }
                 }
                 break;
             }
             case 13: condition_met = (player.limited_items_bought >= 5); break;  // 限定收藏家
             case 14: {
                 int unlocked = 0;
                 for (int j = 0; j < MAX_ACHIEVEMENTS; j++) {
                     if (player.achievements[j].unlocked) unlocked++;
                 }
                 condition_met = unlocked >= 20;
                 break;
             }
             case 15: condition_met = player.prestige_level >= 1; break;
             case 16: condition_met = player.prestige_level >= 5; break;
             case 17: condition_met = player.boss_defeats >= 10; break;
             case 18: condition_met = player.festival_participations >= 5; break;
             case 19: condition_met = player.craft_count >= 10; break;
             // === v4.0 叙事型成就 ===
            case 20: condition_met = (player.scammers_detected >= 3); break;  // 人精
            case 21: condition_met = (player.insider_profit >= 5000); break;  // 赌怪
            case 22: // 破产重生 — 从负债恢复到10000坤币
                condition_met = (player.kun_coins >= 10000 && player.been_bankrupt);
                break;
             case 23: // 社交恐怖分子 — 3位好友好感度满值
                 {
                     int max_friends = 0;
                     for (int j = 0; j < player.friend_count; j++) {
                         if (player.friends[j].friendship_level >= 100) max_friends++;
                     }
                     condition_met = (max_friends >= 3);
                 }
                 break;
            case 24: // 锦鲤附体 — 一天内3个正面随机事件
                condition_met = (player.daily_positive_events >= 3);
                break;
             case 25: // 死神来了 — 经历猝死后幸存
                 condition_met = (player.hallu_count >= 1 && player.sudden_death_chance > 0 && player.health > 80);
                 break;
            case 26: // 最后的乘客 — 出租车任务500次
                condition_met = (player.task_completion_counts[0] >= 500);
                 break;
            case 27: // 技术无罪 — 程序员任务拒绝灰色请求3次
                condition_met = (player.gray_requests_refused >= 3);
                break;
            case 28: // 一言九鼎 — 从未对NPC撒谎
                condition_met = (player.has_never_lied && player.level >= 10);
                break;
         }
         
         if (condition_met) {
             unlock_achievement(i);
         }
     }
 }
 
 void unlock_achievement(int achievement_id) {
     if (achievement_id < 0 || achievement_id >= MAX_ACHIEVEMENTS) return;
     if (player.achievements[achievement_id].unlocked) return;
     
     player.achievements[achievement_id].unlocked = true;
     player.achievements[achievement_id].unlock_time = time(NULL);
     
     add_coins(player.achievements[achievement_id].reward);
     player.kun_exp += player.achievements[achievement_id].reward_exp;
     player.achievement_points += (int)(player.achievements[achievement_id].reward / 10);
     
     print_colored(COLOR_YELLOW, "\n?? 成就解锁：%s！\n", player.achievements[achievement_id].name);
     printf("   %s\n", player.achievements[achievement_id].description);
     printf("   获得奖励：%lld 坤币, %d 经验, %d 成就点\n", 
            player.achievements[achievement_id].reward,
            player.achievements[achievement_id].reward_exp,
            (int)(player.achievements[achievement_id].reward / 10));
 }
 
 void show_achievements() {
     CLEAR_SCREEN();
     draw_box("成就系统");
     
     int unlocked_count = 0;
     for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
         if (player.achievements[i].unlocked) unlocked_count++;
     }
     
     printf("总成就：%d/%d  成就点数：%d\n\n", unlocked_count, MAX_ACHIEVEMENTS, player.achievement_points);
     
     print_colored(COLOR_YELLOW, "=== 已解锁成就 ===\n");
     for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
         if (player.achievements[i].unlocked) {
             struct tm* tm = localtime(&player.achievements[i].unlock_time);
             printf(COLOR_GREEN "★ %s\n" COLOR_RESET, player.achievements[i].name);
             printf("   %s\n", player.achievements[i].description);
             printf("   解锁时间：%04d-%02d-%02d %02d:%02d\n", 
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min);
         }
     }
     
     print_colored(COLOR_CYAN, "\n=== 未解锁成就 ===\n");
     for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
         if (!player.achievements[i].unlocked) {
             printf("%s\n", player.achievements[i].name);
             printf("   %s\n", player.achievements[i].description);
         }
     }
     draw_separator();
 }
 
 void show_achievement_progress() {
     // 已在成就界面展示
 }
 
 //================ 股票市场更新 ================
void save_stock_history() {
    // 将当前所有股票价格追加到历史记录文件
    ensure_dir(SAVE_DIR);
    FILE* fp = fopen(STOCK_HISTORY_FILE, "ab");
    if (!fp) return;
    time_t now = time(NULL);
    fwrite(&now, sizeof(time_t), 1, fp);
    int prices[25];
    for (int i = 0; i < 25; i++) {
        prices[i] = player.stocks[i].current_price;
    }
    fwrite(prices, sizeof(int), 25, fp);
    fclose(fp);
}

void update_stock_prices() {
    /* v5.1：走统一的真实市场引擎，按 tick 补算，避免离线期间“偷偷涨价” */
    int i;
    for (i = 0; i < STOCK_COUNT; i++) {
        if (time(NULL) - player.stocks[i].last_update >= 60) {
            player.stocks[i].last_update = time(NULL);
        }
    }
    save_stock_history();
 }
 
 //================ 被动收入计算 ================
 void calculate_passive_income() {
     money_t passive = 0;
     
     for (int i = 0; i < player.furniture_count; i++) {
         for (int j = 0; j < MAX_MARKET; j++) {
             if (strcmp(player.furniture[i], market[j].name) == 0) {
                 passive += (money_t)market[j].passive_income;
                 break;
             }
         }
     }
     
     for (int i = 0; i < player.property_count; i++) {
         passive += (money_t)player.properties[i].passive_income;
     }
     
     passive = (money_t)(passive * pow(player.skills[SKILL_MANAGEMENT].multiplier, 
                                       player.skills[SKILL_MANAGEMENT].level - 1));
     
     if (passive > 0) {
         add_coins(passive);
         player.total_income += passive;
         print_colored(COLOR_GREEN, "\n?? 被动收入 +%lld 坤币\n", passive);
     }
 }
 
 //================ 能量恢复 ================
void recover_energy() {
    static time_t last_energy_recovery = 0;
    time_t now = time(NULL);
    
    if (last_energy_recovery == 0) {
        last_energy_recovery = now;
    }
    
    int seconds_passed = (int)(now - last_energy_recovery);
    if (seconds_passed >= 600) {
        int recovery = seconds_passed / 600;
        if (recovery > 10) recovery = 10;
        
        if (player.has_premium) recovery *= 2;
        
        player.energy += recovery;
        if (player.energy > player.max_energy) player.energy = player.max_energy;
        
        last_energy_recovery = now;
    }
}

//================ v4.0 新增系统 ================

// 打工微型事件数据
typedef struct {
    int task_id;        // 专属任务ID，-1=通用
    const char* title;
    const char* description;
    int spirit_effect;  // 精神影响
    int health_effect;  // 健康影响
    int coin_bonus;     // 额外收入（可为负）
} WorkMicroEvent;

static WorkMicroEvent micro_events[] = {
    // === 通用事件 ===
    {-1, "刁钻客户", "一位坤城的本地阿姨对你的服务各种挑剔，最后扔下一句「算了就这样吧」。", -10, 0, -30},
    {-1, "意外小费", "一个戴着金链子的商人因为赶时间，多给了你一张钞票。坤城的有钱人就是这么任性。", 5, 0, 80},
    {-1, "同行抢单", "你的同行在你眼皮底下抢走了一单生意。他回头冲你笑了笑。", -5, 0, -50},
    {-1, "城管来了", "远处的城管巡逻车吸引了所有人的注意，你趁机快速完成了今天的工作。", 0, 0, 40},
    {-1, "迷路的外地人", "一个外地游客问你去坤城塔的路。你花了几分钟给他指路，他感激地塞给你一点钱。", 5, 0, 20},
    {-1, "设备故障", "你的工作设备突然出了故障，修了半天才弄好。今天真是诸事不顺。", -15, 0, -100},
    {-1, "老熟人", "在街上遇到了之前在坤城认识的人，聊了几句。人与人之间的温度，在坤城这种地方很难得。", 10, 0, 0},
    {-1, "暴雨突降", "坤城的夏天说变就变，一场暴雨打得你措手不及。淋湿了，但活还得干完。", -5, -5, -20},
    {-1, "幸运日", "今天不知道为什么，一切都特别顺利。坤城的空气里好像有股甜甜的味道。", 15, 0, 60},
    {-1, "黑中介", "一个自称中介的人找上门，说有个「好活儿」要介绍给你。你察觉到不对劲。", 0, 0, 0},
    // === 出租车专属 ===
    {0, "深夜乘客", "凌晨两点，一个穿风衣的男人上了你的车。「去开发区，别问为什么。」", -10, 0, 150},
    {0, "堵在CBD", "坤城CBD的下班高峰，你在车流里堵了整整40分钟。乘客迟到了，但还是在App上给了你差评。", -15, 0, -80},
    {0, "遗落物品", "乘客下车后，你发现后座留下了一个信封。里面是一叠现金和一张写着「帮帮我」的字条。", 10, -5, 0},
    // === 程序员专属 ===
    {4, "灰色外包", "一个陌生人通过私信联系你，问你能不能帮他「绕过」某个系统的防火墙。报酬相当诱人。", 0, 0, 0},
    {4, "通宵上线", "产品经理说今晚必须上线。凌晨三点的科技园，只有你的屏幕还亮着。", -20, -10, 200},
    {4, "开源贡献", "你在GitHub上的一个PR被坤城本地的大佬merge了，成就感满满。", 20, 0, 0},
    // === 外卖专属 ===
    {2, "超时罚款", "一个意外的交通事故让你延误了三单，平台毫不留情地扣了你的钱。", -10, 0, -120},
    {2, "暖心顾客", "一个住在老城区的大学生给你打了五星好评，还附了一句「辛苦了，谢谢」。", 10, 0, 30},
    // === 餐厅专属 ===
    {3, "难缠的VIP", "CBD某公司老总带了客户来吃饭，对每道菜都吹毛求疵。老板赔了不是还倒贴了一瓶酒。", -15, 0, -50},
    {3, "美食评论家", "一位戴着眼镜的中年人默默吃完了一整顿饭。后来你在点评App上看到了他的五星长文。", 10, 0, 50},
};

void trigger_work_micro_event(int task_index) {
    // 40%概率触发微事件
    if ((rand() % 100) >= 40) return;
    
    // 筛选匹配的事件池
    WorkMicroEvent event_pool[30];
    int pool_size = 0;
    int event_count = sizeof(micro_events) / sizeof(micro_events[0]);
    
    for (int i = 0; i < event_count; i++) {
        if (micro_events[i].task_id == -1 || micro_events[i].task_id == task_index) {
            event_pool[pool_size++] = micro_events[i];
        }
    }
    
    if (pool_size == 0) return;
    
    WorkMicroEvent* event = &event_pool[rand() % pool_size];
    
    // 特殊处理：灰色请求（程序员任务）
    if (event->task_id == 4 && strstr(event->title, "灰色外包") != NULL) {
        printf("\n");
        print_colored(COLOR_YELLOW, "微事件——%s\n", event->title);
        printf("   %s\n", event->description);
        printf("\n   1. 接受请求（报酬3000坤币，但事后良心不安）\n");
        printf("   2. 拒绝（程序员有底线）\n");
        printf("   请选择: ");
        
        int choice = get_valid_input(1, 2);
        if (choice == 1) {
            add_coins(3000);
            player.total_income += 3000;
            player.today_income += 3000;
            player.has_never_lied = false;  // 撒谎了
            player.spirit -= 20;
            print_colored(COLOR_RED, "   你接下了这个活。钱到了，但心里总觉得哪里不舒服。\n");
        } else {
            player.gray_requests_refused++;
            player.spirit += 10;
            print_colored(COLOR_GREEN, "   你拒绝了。有些钱，不该赚。\n");
        }
    }
    // 特殊处理：黑中介（通用事件，可能有撒谎选项）
    else if (strstr(event->title, "黑中介") != NULL) {
        printf("\n");
        print_colored(COLOR_YELLOW, "微事件——%s\n", event->title);
        printf("   %s\n", event->description);
        printf("\n   1. 敷衍过去（\"我考虑考虑\"）\n");
        printf("   2. 直接拒绝\n");
        printf("   请选择: ");
        
        int choice = get_valid_input(1, 2);
        if (choice == 1) {
            player.has_never_lied = false;  // 敷衍也算撒谎
            player.spirit -= 5;
            print_colored(COLOR_YELLOW, "   你敷衍了几句，对方悻悻地走了。\n");
        } else {
            player.spirit += 5;
            print_colored(COLOR_GREEN, "   你直截了当地拒绝了。在坤城，少一个麻烦是一个麻烦。\n");
        }
    }
    // 特殊处理：遗落物品（出租车）
    else if (event->task_id == 0 && strstr(event->title, "遗落物品") != NULL) {
        printf("\n");
        print_colored(COLOR_YELLOW, "微事件——%s\n", event->title);
        printf("   %s\n", event->description);
        printf("\n   1. 交给警察\n");
        printf("   2. 收下现金，不管那字条\n");
        printf("   请选择: ");
        
        int choice = get_valid_input(1, 2);
        if (choice == 1) {
            player.spirit += 10;
            print_colored(COLOR_GREEN, "   你把信封交给了最近的派出所。做了该做的事，今晚睡得安稳。\n");
        } else {
            add_coins(500);
            player.total_income += 500;
            player.today_income += 500;
            player.has_never_lied = false;
            player.spirit -= 15;
            print_colored(COLOR_RED, "   你收下了钱，把字条扔进了垃圾桶。那晚你失眠了。\n");
        }
    }
    // 普通事件
    else {
        printf("\n");
        print_colored(COLOR_YELLOW, "微事件——%s\n", event->title);
        printf("   %s\n", event->description);
        
        player.spirit += event->spirit_effect;
        player.health += event->health_effect;
        if (event->coin_bonus > 0) {
            add_coins(event->coin_bonus);
            player.total_income += event->coin_bonus;
            player.today_income += event->coin_bonus;
            printf("   +%d 坤币\n", event->coin_bonus);
        } else if (event->coin_bonus < 0) {
            int loss = -event->coin_bonus;
            if (player.kun_coins >= loss) {
                remove_coins(loss);
                player.total_expenses += loss;
                player.today_expenses += loss;
                printf("   -%d 坤币\n", loss);
            } else {
                print_colored(COLOR_RED, "   （余额不足，无法扣除）\n");
            }
        }
        if (event->spirit_effect > 0) printf("   精神 +%d\n", event->spirit_effect);
        if (event->spirit_effect < 0) printf("   精神 %d\n", event->spirit_effect);
        if (event->health_effect != 0) printf("   血量 %+d\n", event->health_effect);
    }
    
    // 修正范围
    if (player.spirit > 100) player.spirit = 100;
    if (player.spirit < 0) player.spirit = 0;
    if (player.health > 100) player.health = 100;
    if (player.health < 0) player.health = 0;
    
    player.today_event_count++;
    player.daily_positive_events++;  // 微事件也算正面（有选择权）
}

void show_daily_report() {
    CLEAR_SCREEN();
    
    char title_buf[100];
    snprintf(title_buf, sizeof(title_buf), "坤城日报 — 第 %d 天", player.day_count);
    draw_box(title_buf);
    
    printf("\n");
    print_colored(COLOR_CYAN, "  ?? 今日收入统计\n");
    printf("  ─────────────────────────────────────\n");
    printf("  打工收入：     +%d 坤币\n", player.today_income);
    printf("  生活成本：     -%d 坤币（房租%d + 餐饮%d）\n", 
           player.rent_cost + player.living_cost, player.rent_cost, player.living_cost);
    printf("  今日净收入：   %+d 坤币\n", player.today_income - player.rent_cost - player.living_cost);
    
    printf("\n");
    print_colored(COLOR_GREEN, "  ?? 今日活动\n");
    printf("  ─────────────────────────────────────\n");
    printf("  完成打工：     %d 次\n", player.today_work_count);
    printf("  经历事件：     %d 次\n", player.today_event_count);
    
    printf("\n");
    print_colored(COLOR_YELLOW, "  ?? 当前状态\n");
    printf("  ─────────────────────────────────────\n");
    printf("  等级：         %d\n", player.level);
    printf("  坤币余额：     %lld\n", player.kun_coins);
    printf("  饱食度：       %d/100\n", player.hunger);
    printf("  血量：         %d/100\n", player.health);
    printf("  精神：         %d/100\n", player.spirit);
    printf("  天数：         第 %d 天 | 季节：", player.day_count);
    switch (player.current_season) {
        case SEASON_SPRING: printf(COLOR_GREEN "春\n" COLOR_RESET); break;
        case SEASON_SUMMER: printf(COLOR_RED "夏\n" COLOR_RESET); break;
        case SEASON_AUTUMN: printf(COLOR_YELLOW "秋\n" COLOR_RESET); break;
        case SEASON_WINTER: printf(COLOR_CYAN "冬\n" COLOR_RESET); break;
    }
    
    printf("\n");
    draw_separator();
}

void day_end() {
    if (player.day_ended) return;
    
    player.day_ended = true;
    player.day_count++;
    
    // 扣除生活成本
    int total_cost = player.rent_cost + player.living_cost;
    player.today_expenses += total_cost;
    player.total_expenses += total_cost;
    
    if (player.kun_coins >= total_cost) {
        remove_coins(total_cost);
    } else {
        // 不够扣就负债
        player.kun_coins -= total_cost;
        player.been_bankrupt = true;
    }
    
    // 显示日报
    show_daily_report();
    printf("  ?? 今日生活成本扣除：%d 坤币\n", total_cost);
    
    if (player.kun_coins < 0) {
        print_colored(COLOR_RED, "  ?? 你已经负债了！坤城的催债人很快就会找上门...\n");
    }
    
    // 夜间恢复
    int sleep_spirit_recovery = 15 + (rand() % 11);  // 15-25（不会睡一觉就兴奋到猝死）
    int sleep_health_recovery = 15 + (rand() % 16);  // 15-30
    player.spirit += sleep_spirit_recovery;
    player.health += sleep_health_recovery;
    // 睡醒后精神不超过70，避免一醒来就"异常兴奋"有猝死风险
    if (player.spirit > 70) player.spirit = 70;
    if (player.health > 100) player.health = 100;
    
    if (player.health <= 0) player.health = 10; // 保底
    
    printf("  ?? 睡眠恢复：精神+%d | 血量+%d\n", sleep_spirit_recovery, sleep_health_recovery);
    
    // 季节更替（每30天）
    if (player.day_count % 30 == 0) {
        change_season();
        printf("  ?? 坤城的季节变了！\n");
    }
    
    // 随机事件（20%概率夜间事件）
    if ((rand() % 100) < 20) {
        random_event();
        player.today_event_count++;
    }
    
    // 升级难度（生活成本随等级增长）
    player.rent_cost = 100 + player.level * 10;
    player.living_cost = 80 + player.level * 5;
    
    // v4.1 新的一天：重置时间槽、清除过期事件、重置每日计数
    player.daily_time_slots = player.max_time_slots;
    player.today_income = 0;
    player.today_expenses = 0;
    player.today_work_count = 0;
    player.today_event_count = 0;
    player.daily_positive_events = 0;
    player.active_event = EVENT_NONE;
    player.event_end_time = 0;
    player.day_ended = false;
    
    draw_separator();
    print_colored(COLOR_CYAN, "\n?? 新的一天开始了。坤城，早安。\n");
}
 
 //================ 游戏主逻辑 ================
void do_task(int task_index) {
    if (task_index < 0 || task_index >= MAX_TASKS) return;
    
    Task* task = &tasks[task_index];
    
    if (!task->is_unlocked) {
        if (player.kun_coins >= task->unlock_cost) {
            task->is_unlocked = true;
            player.tasks_unlocked[task_index] = true;
            remove_coins(task->unlock_cost);
            player.total_expenses += task->unlock_cost;
            print_colored(COLOR_GREEN, "解锁新任务：%s！\n", task->name);
        } else {
            print_colored(COLOR_RED, "需要 %d 坤币解锁此任务！\n", task->unlock_cost);
            return;
        }
    }
    
    if (player.level < task->required_level) {
        print_colored(COLOR_RED, "需要等级 %d 才能执行此任务！\n", task->required_level);
        return;
    }
    
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            if (player.skills[task->required_skills[i]].level < task->required_skill_level[i]) {
                print_colored(COLOR_RED, "需要 %s 技能等级 %d！\n", 
                           player.skills[task->required_skills[i]].name, 
                           task->required_skill_level[i]);
                return;
            }
        }
    }
    
    // === v4.0 时间槽系统 ===
    // 先结算昨天（如果还没结算）
    if (player.day_ended) {
        player.day_ended = false;
        player.daily_time_slots = player.max_time_slots;
        player.today_income = 0;
        player.today_expenses = 0;
        player.today_work_count = 0;
        player.today_event_count = 0;
        player.daily_positive_events = 0;
    }
    
    if (player.daily_time_slots <= 0) {
        print_colored(COLOR_RED, "今天的时间已经用完了！睡一觉明天再来吧。\n");
        day_end();
        return;
    }
    
    // === v4.0 选择打工难度 ===
    printf("\n");
    draw_separator();
    print_colored(COLOR_CYAN, "选择打工模式：\n");
    printf("  1. ?? 摸鱼 — 轻松安全，收入×0.6，消耗1时间槽\n");
    printf("  2. ?? 普通 — 标准模式，收入×1.0，消耗1时间槽\n");
    printf("  3. ?? 加班 — 高强度，收入×1.5，消耗2时间槽，健康-5\n");
    printf("  4. ?? 拼命 — 搏命赚钱，收入×2.5，消耗2时间槽，健康-15，猝死风险+5%%\n");
    printf("  0. 返回\n");
    draw_separator();
    printf("请选择: ");
    
    int diff_choice = get_valid_input(0, 4);
    if (diff_choice == 0) return;
    
    WorkDifficulty diff = (WorkDifficulty)(diff_choice - 1);
    float diff_income_mult = 1.0f;
    int diff_slot_cost = 1;
    int diff_health_cost = 0;
    int diff_death_risk = 0;
    const char* diff_name = "普通";
    
    switch (diff) {
        case DIFF_SLACK:
            diff_income_mult = 0.6f;
            diff_slot_cost = 1;
            diff_name = "摸鱼";
            break;
        case DIFF_NORMAL:
            diff_income_mult = 1.0f;
            diff_slot_cost = 1;
            diff_name = "普通";
            break;
        case DIFF_OVERTIME:
            diff_income_mult = 1.5f;
            diff_slot_cost = 2;
            diff_health_cost = 5;
            diff_name = "加班";
            break;
        case DIFF_HARD:
            diff_income_mult = 2.5f;
            diff_slot_cost = 2;
            diff_health_cost = 15;
            diff_death_risk = 5;
            diff_name = "拼命";
            break;
        default: break;
    }
    
    if (player.daily_time_slots < diff_slot_cost) {
        print_colored(COLOR_RED, "时间不够！需要 %d 个时间槽，当前剩余 %d 个。\n", 
                      diff_slot_cost, player.daily_time_slots);
        return;
    }
    
    player.last_difficulty = diff;
    
    // 计算成功率
    float success_chance = task->success_rate;
    success_chance *= pow(player.skills[SKILL_LUCK].multiplier, player.skills[SKILL_LUCK].level - 1);
    if (diff == DIFF_HARD) success_chance *= 0.85f;  // 拼命模式成功率降低
    if (diff == DIFF_SLACK) success_chance = 1.0f;    // 摸鱼不会失败
    if (success_chance > 0.95f) success_chance = 0.95f;
    
    bool success = (rand() % 100) < (int)(success_chance * 100);
    
    // 计算收入
    float base_income = (float)task->base_income;
    float multiplier = diff_income_mult;
    
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            multiplier *= pow(player.skills[task->required_skills[i]].multiplier, 
                            player.skills[task->required_skills[i]].level);
        }
    }
    
    if (player.current_company_level > 0) {
        multiplier *= player.company_levels[player.current_company_level].income_multiplier;
    }
    
    if (player.active_event == EVENT_TASK_BONUS) {
        multiplier *= 1.5f;
    }
    
    switch (player.current_season) {
        case SEASON_SPRING: multiplier *= 1.1f; break;
        case SEASON_SUMMER: multiplier *= 1.05f; break;
        case SEASON_AUTUMN: multiplier *= 1.15f; break;
        case SEASON_WINTER: multiplier *= 0.9f; break;
    }
    
    for (int i = 0; i < player.pet_count; i++) {
        if (player.pets[i].is_unlocked && player.pets[i].happiness > 50) {
            multiplier *= player.pets[i].bonus_multiplier;
        }
    }
    
    for (int i = 0; i < player.inventory_count; i++) {
        if (strcmp(player.inventory[i].name, "新手装备") == 0 && player.inventory[i].quantity > 0) {
            multiplier *= 1.05f;
            break;
        }
    }
    
    multiplier *= (1.0f + player.prestige_level * 0.05f);
    
    int income = (int)(base_income * multiplier);
    int exp_gain = task->experience_gain;
    if (diff == DIFF_HARD) exp_gain = (int)(exp_gain * 1.8f);
    if (diff == DIFF_OVERTIME) exp_gain = (int)(exp_gain * 1.2f);
    
    if (player.active_event == EVENT_DOUBLE_EXP) {
        exp_gain *= 2;
    }
    
    if (!success) {
        income = income / 2;
        print_colored(COLOR_RED, "\n?? 任务出了点意外！只获得部分收入...\n");
    }
    
    // 消耗时间槽
    player.daily_time_slots -= diff_slot_cost;
    player.health -= diff_health_cost;
    if (player.health < 0) player.health = 0;
    if (player.health > 100) player.health = 100;
    
    // 拼命模式猝死风险
    if (diff == DIFF_HARD && (rand() % 100) < diff_death_risk) {
        player.sudden_death_chance += 5;
        player.health -= 30;
        print_colored(COLOR_RED, "\n?? 你的身体发出了严重的警告信号！眼前一黑...\n");
        if (player.health <= 0) {
            player.health = 1;
            player.hallu_count++;
            print_colored(COLOR_RED, "你差点猝死在坤城的街头！幸运地捡回一条命...\n");
        }
    }
    
    add_coins(income);
    if (player.kun_coins < 0) player.been_bankrupt = true;  // 追踪破产
    player.kun_exp += exp_gain;
    player.total_income += income;
    player.today_income += income;
    player.lifetime_task_completions++;
    player.daily_task_completions++;
    player.weekly_task_completions++;
    player.today_work_count++;
    
    // 饱食度消耗（难度越大越累越饿）
    int hunger_cost = 5 + diff_slot_cost * 2;
    if (diff == DIFF_HARD) hunger_cost += 5;
    player.hunger -= hunger_cost;
    if (player.hunger < 0) player.hunger = 0;
    if (player.hunger > 100) player.hunger = 100;
    
    // 精神值变化
    if (diff == DIFF_SLACK) {
        player.spirit = 50 + (rand() % 11);  // 摸鱼精神好
    } else if (diff == DIFF_HARD) {
        player.spirit = 20 + (rand() % 21);  // 拼命精神差
    } else {
        player.spirit = 40 + (rand() % 21);
    }
    
    task->completion_count++;
    player.task_completion_counts[task_index]++;
    task->last_completed = time(NULL);
    
    // 输出打工结果
    print_colored(COLOR_GREEN, "\n? 坤城打工完成：%s [%s模式]\n", task->name, diff_name);
    printf("   收入：%d 坤币 | 经验：+%d | 成功率：%.0f%%\n", income, exp_gain, success_chance * 100);
    printf("   时间槽：%d/%d | 饱食度：%d/100 | 血量：%d/100\n", 
           player.daily_time_slots, player.max_time_slots, player.hunger, player.health);
    
    // === v4.0 打工微型事件 ===
    trigger_work_micro_event(task_index);
    
    // 随机意外
    random_accident();
    
    // 升级检查
    int exp_needed = player.level * 100;
    while (player.kun_exp >= exp_needed) {
        player.kun_exp -= exp_needed;
        player.level++;
        player.skill_points += 3;
        player.max_energy += 10;
        player.energy = player.max_energy;
        exp_needed = player.level * 100;
        
        print_colored(COLOR_YELLOW, "\n?? 等级提升！当前等级：%d\n", player.level);
        printf("   获得3点技能点 | 最大能量+10\n");
    }
    
    // 任务进度
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            player.active_quests[i].deadline != 0 &&
            player.active_quests[i].deadline > time(NULL)) {
            if (strstr(player.active_quests[i].description, "完成") != NULL) {
                player.active_quests[i].progress++;
                if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                    complete_quest(i);
                }
            }
        }
    }
    
    // 宠物状态变化
    for (int i = 0; i < player.pet_count; i++) {
        if (player.pets[i].is_unlocked) {
            player.pets[i].hunger -= 1;
            if (player.pets[i].hunger < 0) player.pets[i].hunger = 0;
            player.pets[i].happiness -= 1;
            if (player.pets[i].happiness < 0) player.pets[i].happiness = 0;
        }
    }
    
    check_hallucination();
    process_spirit_effects();
    check_achievements();
    
    // 时间槽用完自动结算
    if (player.daily_time_slots <= 0) {
        printf("\n");
        print_colored(COLOR_YELLOW, "坤城的天空暗了下来，一天又过去了...\n");
        day_end();
    }
}
 
 void buy_item(int index) {
     if (index < 0 || index >= MAX_MARKET) return;
     
     MarketItem* item = &market[index];
     
     if (player.level < item->required_level) {
         print_colored(COLOR_RED, "需要等级 %d 才能购买此物品！\n", item->required_level);
         return;
     }
     
     if (item->is_limited && item->available_until < time(NULL)) {
         print_colored(COLOR_RED, "此限定物品已过期！\n");
         return;
     }
     
     if (player.furniture_count >= MAX_FURNITURE) {
         print_colored(COLOR_RED, "家具数量已达到上限！\n");
         return;
     }
     
     int price = item->price;
     if (player.active_event == EVENT_INFLATION) {
         price = (int)(price * 1.2f);
     }
     
     if (player.kun_coins >= price) {
         remove_coins(price);
         player.total_expenses += price;
         
         safe_strcpy(player.furniture[player.furniture_count], item->name, 50);
         player.furniture_count++;
         
         bool found = false;
         for (int i = 0; i < player.inventory_count; i++) {
             if (strcmp(player.inventory[i].name, item->name) == 0) {
                 player.inventory[i].quantity++;
                 found = true;
                 break;
             }
         }
         
         if (!found && player.inventory_count < MAX_INVENTORY) {
             InventoryItem inv_item;
             safe_strcpy(inv_item.name, item->name, 50);
             inv_item.price = price;
             inv_item.value = item->value;
             inv_item.rarity = item->rarity;
             inv_item.is_consumable = false;
             inv_item.effect_value = (int)(item->passive_income * 100);
             safe_strcpy(inv_item.description, item->description, 100);
             inv_item.quantity = 1;
             player.inventory[player.inventory_count++] = inv_item;
         }
         
         print_colored(COLOR_GREEN, "\n?? 购买成功：%s\n", item->name);
         printf("   花费：%d 坤币\n", price);
         printf("   被动收入：%.1f 坤币/小时\n", item->passive_income);
         
         check_achievements();
         
         for (int i = 0; i < MAX_QUESTS; i++) {
             if (!player.active_quests[i].completed && 
                 strstr(player.active_quests[i].description, "购买") != NULL) {
                 player.active_quests[i].progress++;
                 if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                     complete_quest(i);
                 }
             }
         }
     } else {
         print_colored(COLOR_RED, "坤币不足！需要 %d 坤币\n", price);
     }
 }
 
 void upgrade_skill(int skill_index) {
     if (skill_index < 0 || skill_index >= MAX_SKILLS) return;
     
     Skill* skill = &player.skills[skill_index];
     
     if (skill->level >= skill->max_level) {
         print_colored(COLOR_RED, "技能已达到最大等级！\n");
         return;
     }
     
     if (player.skill_points <= 0) {
         print_colored(COLOR_RED, "没有足够的技能点！\n");
         return;
     }
     
     int cost = skill->level * 100;
     cost = (int)(cost * (1.0f - player.prestige_level * 0.02f));
     if (cost < 10) cost = 10;
     
     if (!remove_coins(cost)) {
         print_colored(COLOR_RED, "升级需要 %d 坤币！\n", cost);
         return;
     }
     
     player.skill_points--;
     player.total_expenses += cost;
     skill->level++;
     
     print_colored(COLOR_GREEN, "\n?? 技能升级：%s 提升到等级 %d\n", skill->name, skill->level);
     printf("   花费：%d 坤币，1 技能点\n", cost);
     printf("   当前加成：%.0f%%\n", (pow(skill->multiplier, skill->level) - 1) * 100);
     
     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed && 
             strstr(player.active_quests[i].description, "升级") != NULL) {
             player.active_quests[i].progress++;
             if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                 complete_quest(i);
             }
         }
     }
     
     check_achievements();
 }
 
 void feed_pet(int pet_index) {
     if (pet_index < 0 || pet_index >= MAX_PETS) return;
     
     Pet* pet = &player.pets[pet_index];
     
     if (!pet->is_unlocked) {
         print_colored(COLOR_RED, "宠物未解锁！\n");
         return;
     }
     
     bool has_food = false;
     for (int i = 0; i < player.inventory_count; i++) {
         if (strcmp(player.inventory[i].name, "宠物食品") == 0 && player.inventory[i].quantity > 0) {
             has_food = true;
             player.inventory[i].quantity--;
             if (player.inventory[i].quantity <= 0) {
                 for (int j = i; j < player.inventory_count - 1; j++) {
                     player.inventory[j] = player.inventory[j + 1];
                 }
                 player.inventory_count--;
             }
             break;
         }
     }
     
     if (!has_food) {
         print_colored(COLOR_RED, "没有宠物食品！\n");
         return;
     }
     
     pet->hunger = 100;
     pet->happiness += 10;
     if (pet->happiness > 100) pet->happiness = 100;
     pet->last_fed = time(NULL);
     
     print_colored(COLOR_GREEN, "\n?? 喂食 %s\n", pet->name);
     printf("   饱食度：100\n");
     printf("   快乐度：%d\n", pet->happiness);
 }
 
 void play_with_pet(int pet_index) {
     if (pet_index < 0 || pet_index >= MAX_PETS) return;
     
     Pet* pet = &player.pets[pet_index];
     
     if (!pet->is_unlocked) {
         print_colored(COLOR_RED, "宠物未解锁！\n");
         return;
     }
     
     if (player.energy < 5) {
         print_colored(COLOR_RED, "能量不足！\n");
         return;
     }
     
     player.energy -= 5;
     pet->happiness += 20;
     if (pet->happiness > 100) pet->happiness = 100;
     pet->hunger -= 5;
     if (pet->hunger < 0) pet->hunger = 0;
     
     print_colored(COLOR_GREEN, "\n?? 和 %s 玩耍，快乐度增加！\n", pet->name);
     printf("   快乐度：%d\n", pet->happiness);
     printf("   饱食度：%d\n", pet->hunger);
 }
 
 void adopt_pet(PetType type) {
     if (player.pet_count >= MAX_PETS) {
         print_colored(COLOR_RED, "宠物数量已达上限！\n");
         return;
     }
     
     int unlock_level = 0;
     money_t unlock_cost = 0;
     const char* pet_type_name = "";
     
     switch (type) {
         case PET_CAT:
             if (player.pets[0].is_unlocked) {
                 print_colored(COLOR_RED, "已拥有此宠物！\n");
                 return;
             }
             unlock_level = 1;
             unlock_cost = 0;
             pet_type_name = "猫咪";
             break;
         case PET_DOG:
             if (player.pets[1].is_unlocked) return;
             unlock_level = 5;
             unlock_cost = 5000;
             pet_type_name = "狗狗";
             break;
         case PET_PARROT:
             if (player.pets[2].is_unlocked) return;
             unlock_level = 15;
             unlock_cost = 50000;
             pet_type_name = "鹦鹉";
             break;
         case PET_DRAGON:
             if (player.pets[3].is_unlocked) return;
             unlock_level = 30;
             unlock_cost = 500000;
             pet_type_name = "龙";
             break;
         case PET_PHOENIX:
             if (player.pets[4].is_unlocked) return;
             unlock_level = 50;
             unlock_cost = 5000000;
             pet_type_name = "凤凰";
             break;
         default:
             return;
     }
     
     if (player.level < unlock_level) {
         print_colored(COLOR_RED, "需要等级 %d 才能领养此宠物！\n", unlock_level);
         return;
     }
     
     if (!remove_coins(unlock_cost)) {
         print_colored(COLOR_RED, "需要 %lld 坤币！\n", unlock_cost);
         return;
     }
     
     player.total_expenses += unlock_cost;
     
     for (int i = 0; i < MAX_PETS; i++) {
         if (player.pets[i].type == type && !player.pets[i].is_unlocked) {
             player.pets[i].is_unlocked = true;
             player.pet_count++;
             player.pets[i].happiness = 100;
             player.pets[i].hunger = 100;
             player.pets[i].level = 1;
             
             print_colored(COLOR_GREEN, "\n?? 成功领养新宠物：%s（%s）！\n", 
                          player.pets[i].name, pet_type_name);
             printf("   加成：%.2f\n", player.pets[i].bonus_multiplier);
             break;
         }
     }
 }
 
 void complete_quest(int quest_index) {
     if (quest_index < 0 || quest_index >= MAX_QUESTS) return;
     
     Quest* quest = &player.active_quests[quest_index];
     
     if (quest->completed) return;
     if (quest->deadline != 0 && quest->deadline < time(NULL)) return;
     
     quest->completed = true;
     add_coins(quest->reward_coins);
     player.kun_exp += quest->reward_exp;
     player.total_income += quest->reward_coins;
     
     print_colored(COLOR_GREEN, "\n?? 任务完成：%s\n", quest->name);
     printf("   获得奖励：%lld 坤币，%d 经验\n", quest->reward_coins, quest->reward_exp);
 }
 
 void level_up_company() {
     if (player.current_company_level >= MAX_COMPANY_LEVELS - 1) {
         print_colored(COLOR_RED, "公司已达到最高等级！\n");
         return;
     }
     
     CompanyLevel* next_level = &player.company_levels[player.current_company_level + 1];
     
     if (!remove_coins(next_level->required_coins)) {
         print_colored(COLOR_RED, "升级需要 %lld 坤币！\n", next_level->required_coins);
         return;
     }
     
     player.total_expenses += next_level->required_coins;
     player.current_company_level++;
     player.skill_points += next_level->unlock_skill_points;
     
     print_colored(COLOR_GREEN, "\n?? 公司升级：%s\n", next_level->name);
     printf("   花费：%lld 坤币\n", next_level->required_coins);
     printf("   获得 %d 技能点\n", next_level->unlock_skill_points);
     printf("   收入倍数：%.1f\n", next_level->income_multiplier);
     
     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed && 
             strstr(player.active_quests[i].description, "公司") != NULL) {
             player.active_quests[i].progress = player.current_company_level;
             if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                 complete_quest(i);
             }
         }
     }
     
     check_achievements();
 }
 
/* v5.1：margin=1 融资买入（自付 50%，券商垫 50%） */
void buy_stock_ext(int stock_index, int amount, int margin) {
     if (stock_index < 0 || stock_index >= STOCK_COUNT) return;
     if (amount <= 0) return;

     Stock* stock = &player.stocks[stock_index];
     money_t gross = (money_t)stock->current_price * amount / 100;
     money_t self_pay;

     if (stock->status == STK_DELISTED) {
         print_colored(COLOR_RED, "该股票已退市，无法买入。\n");
         return;
     }
     if (stock->status == STK_HALT) {
         print_colored(COLOR_RED, "该股票已停牌，无法交易。\n");
         return;
     }
     if (stock->limit_lock == 1) {
         print_colored(COLOR_RED, "该股票封涨停，买盘排不进去。\n");
         return;
     }

     self_pay = margin ? gross / 2 : gross;

     // v4.1 全梭哈确认：当花费超过当前坤币50%时弹出确认
     if (self_pay > 0 && player.kun_coins > 0 && self_pay > player.kun_coins / 2) {
         printf("\n?? 警告：此操作将花费 %lld 坤币（当前余额：%lld 坤币）\n", self_pay, player.kun_coins);
         printf("确认要大手笔买入吗？(1=确认, 0=取消): ");
         int confirm = get_valid_input(0, 1);
         if (confirm == 0) {
             print_colored(COLOR_YELLOW, "已取消买入。\n");
             return;
         }
     }

     if (!remove_coins(self_pay)) {
         print_colored(COLOR_RED, "坤币不足！需要 %lld 坤币\n", self_pay);
         return;
     }

     player.total_expenses += self_pay;
     player.invested_stocks[stock_index] += amount;

     if (margin) {
         money_t loan = gross - self_pay;
         player.stock_margin_debt[stock_index] += loan;
         player.stock_loan_total += loan;
         player.stock_margin_bought++;
         printf("\n" COLOR_MAGENTA "[融资买入] %s × %d" COLOR_RESET "\n", stock->name, amount);
         printf("   融资额：%lld 坤币（利率按日计，跌穿 %d%% 担保比例将被强平）\n",
                loan, STOCK_MAINT_RATIO);
     } else {
         printf("\n?? 购买股票：%s × %d\n", stock->name, amount);
     }
     printf("   单价：%.2f 坤币\n", stock->current_price / 100.0);
     printf("   自付：%lld 坤币\n", self_pay);
     printf("   当前担保比例：%d%%\n", stk_maint_ratio());

     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed &&
             strstr(player.active_quests[i].description, "股票投资") != NULL) {
             player.active_quests[i].progress++;
             if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                 complete_quest(i);
             }
         }
     }
 }

 void buy_stock(int stock_index, int amount) {
     buy_stock_ext(stock_index, amount, 0);
 }

 void sell_stock(int stock_index, int amount) {
     if (stock_index < 0 || stock_index >= STOCK_COUNT) return;
     if (amount <= 0) return;
     if (player.invested_stocks[stock_index] < amount) {
         print_colored(COLOR_RED, "持股不足！\n");
         return;
     }

     Stock* stock = &player.stocks[stock_index];
     if (stock->status == STK_HALT) {
         print_colored(COLOR_RED, "该股票已停牌，卖不出去。\n");
         return;
     }
     if (stock->status == STK_DELISTED) {
         print_colored(COLOR_RED, "该股票已退市。\n");
         return;
     }
     if (stock->limit_lock == -1) {
         print_colored(COLOR_RED, "该股票封跌停，卖单排队但无人接。\n");
         return;
     }

     money_t total_value = (money_t)stock->current_price * amount / 100;
     int held_before = player.invested_stocks[stock_index];
     money_t repay = 0;

     /* 平掉对应比例的融资债务 */
     if (player.stock_margin_debt[stock_index] > 0 && held_before > 0) {
         repay = player.stock_margin_debt[stock_index] * amount / held_before;
         if (repay > total_value) repay = total_value;
         player.stock_margin_debt[stock_index] -= repay;
         player.stock_loan_total -= repay;
         if (player.stock_loan_total < 0) player.stock_loan_total = 0;
     }

     add_coins(total_value - repay);
     player.total_income += total_value;
     player.invested_stocks[stock_index] -= amount;

     printf("\n?? 出售股票：%s × %d\n", stock->name, amount);
     printf("   单价：%.2f 坤币\n", stock->current_price / 100.0);
     printf("   总价：%lld 坤币\n", total_value);
     if (repay > 0) printf("   其中偿还融资：%lld 坤币\n", repay);
     if (player.stock_loan_total > 0)
         printf("   剩余负债：%lld 坤币（担保比例 %d%%）\n",
                player.stock_loan_total, stk_maint_ratio());
 }
 
 void buy_property(int property_index) {
     if (property_index < 0 || property_index >= 10) return;
     if (available_properties[property_index].owned) {
         print_colored(COLOR_RED, "已拥有此房产！\n");
         return;
     }
     
     Property* prop = &available_properties[property_index];
     
     if (!remove_coins(prop->coins_needed)) {
         print_colored(COLOR_RED, "坤币不足！需要 %lld 坤币\n", prop->coins_needed);
         return;
     }
     
     player.total_expenses += prop->coins_needed;
     
     safe_strcpy(player.properties[player.property_count].name, prop->name, 30);
     safe_strcpy(player.properties[player.property_count].description, prop->description, 100);
     player.properties[player.property_count].coins_needed = prop->coins_needed;
     player.properties[player.property_count].days_needed = prop->days_needed;
     player.properties[player.property_count].passive_income = prop->passive_income;
     player.properties[player.property_count].owned = true;
     player.property_count++;
     
     prop->owned = true;
     
     print_colored(COLOR_GREEN, "\n?? 购买成功：%s\n", prop->name);
     printf("   花费：%lld 坤币\n", prop->coins_needed);
     printf("   建造时间：%d 天\n", prop->days_needed);
     printf("   每日收入：%.0f 坤币\n", prop->passive_income);
     
     check_achievements();
 }
 
 void participate_lottery() {
     if (player.lottery_tickets <= 0) {
         print_colored(COLOR_RED, "没有彩票！\n");
         return;
     }
     
     player.lottery_tickets--;
     
     float base_chance = 0.01f;
     if (player.active_event == EVENT_LOTTERY_BONUS) {
         base_chance *= 2;
     }
     
     float luck_multiplier = pow(player.skills[SKILL_LUCK].multiplier, 
                                 player.skills[SKILL_LUCK].level - 1);
     float chance = base_chance * luck_multiplier;
     
     int roll = rand() % 10000;
     if (roll < (int)(chance * 10000)) {
         int prize = 10000 + rand() % 90000;
         prize = (int)(prize * (1.0f + player.prestige_level * 0.1f));
         add_coins(prize);
         player.total_income += prize;
         print_colored(COLOR_YELLOW, "\n?? 恭喜中奖！获得 %d 坤币！\n", prize);
         
         if (!player.achievements[4].unlocked) {
             unlock_achievement(4);
         }
     } else {
         print_colored(COLOR_RED, "\n?? 很遗憾，未中奖\n");
     }
 }
 
 void open_mystery_box() {
     if (player.mystery_keys <= 0) {
         print_colored(COLOR_RED, "没有神秘钥匙！\n");
         return;
     }
     
     player.mystery_keys--;
     
     int roll = rand() % 100;
     
     if (roll < 50) {
         int coins = 1000 + rand() % 4000;
         add_coins(coins);
         player.total_income += coins;
         print_colored(COLOR_GREEN, "\n?? 开启神秘宝箱获得：%d 坤币\n", coins);
     } else if (roll < 80) {
         int coins = 5000 + rand() % 10000;
         add_coins(coins);
         player.total_income += coins;
         print_colored(COLOR_BLUE, "\n?? 开启神秘宝箱获得：%d 坤币（稀有）\n", coins);
     } else if (roll < 95) {
         int coins = 20000 + rand() % 30000;
         add_coins(coins);
         player.total_income += coins;
         player.skill_points += 3;
         print_colored(COLOR_MAGENTA, "\n?? 开启神秘宝箱获得：%d 坤币和3技能点（史诗）\n", coins);
     } else {
         int coins = 100000;
         add_coins(coins);
         player.total_income += coins;
         player.skill_points += 10;
         player.mystery_keys += 3;
         print_colored(COLOR_YELLOW, "\n?? 开启神秘宝箱获得：%d 坤币、10技能点和3把钥匙（传说）\n", coins);
     }
 }
 
 void challenge_boss() {
     if (player.active_event != EVENT_BOSS_CHALLENGE) {
         print_colored(COLOR_RED, "当前没有BOSS挑战事件！\n");
         return;
     }
     
     if (player.energy < 50) {
         print_colored(COLOR_RED, "能量不足，需要50能量！\n");
         return;
     }
     
     player.energy -= 50;
     
     float power = 0;
     for (int i = 0; i < MAX_SKILLS; i++) {
         power += player.skills[i].level * 10;
     }
     power += player.level * 5;
     power *= (1.0f + player.prestige_level * 0.1f);
     
     float boss_hp = 1000 + player.boss_defeats * 200;
     
     if (power >= boss_hp) {
         int reward = 10000 + rand() % 20000;
         reward = (int)(reward * (1.0f + player.prestige_level * 0.2f));
         add_coins(reward);
         player.total_income += reward;
         player.kun_exp += 500;
         player.boss_defeats++;
         
         print_colored(COLOR_YELLOW, "\n?? 挑战BOSS成功！获得 %d 坤币，500经验！\n", reward);
         
         for (int i = 0; i < MAX_QUESTS; i++) {
             if (!player.active_quests[i].completed && 
                 strstr(player.active_quests[i].description, "击败") != NULL) {
                 player.active_quests[i].progress++;
                 if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                     complete_quest(i);
                 }
             }
         }
         
         end_event();
     } else {
         print_colored(COLOR_RED, "\n?? BOSS太强了，挑战失败！\n");
         printf("   需要战斗力：%.0f，当前战斗力：%.0f\n", boss_hp, power);
     }
 }
 
 void participate_festival() {
     if (player.active_event != EVENT_FESTIVAL) {
         print_colored(COLOR_RED, "当前不是节日庆典！\n");
         return;
     }
     
     if (player.energy < 20) {
         print_colored(COLOR_RED, "能量不足！\n");
         return;
     }
     
     player.energy -= 20;
     int reward = 5000 + rand() % 10000;
     add_coins(reward);
     player.total_income += reward;
     player.festival_participations++;
     
     print_colored(COLOR_YELLOW, "\n?? 参与节日庆典获得 %d 坤币！\n", reward);
     
     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed && 
             strstr(player.active_quests[i].description, "节日") != NULL) {
             player.active_quests[i].progress++;
             if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                 complete_quest(i);
             }
         }
     }
 }
 
void change_season() {
    player.current_season = (Season)((player.current_season + 1) % 4);
    
    const char* season_names[] = {"春季", "夏季", "秋季", "冬季"};
    print_colored(COLOR_CYAN, "\n?? 坤城进入了%s\n", season_names[player.current_season]);
    
    switch (player.current_season) {
        case SEASON_SPRING:
            printf("   春季：坤城的招聘会多了起来，所有任务收入增加10%%\n");
            break;
        case SEASON_SUMMER:
            printf("   夏季：外卖爆单的季节，任务能量消耗减少5%%\n");
            break;
        case SEASON_AUTUMN:
            printf("   秋季：坤城的股市开始波动，投资回报增加15%%\n");
            break;
        case SEASON_WINTER:
            printf("   冬季：坤城的人们更愿意宅在家里，任务收入减少10%%\n");
            break;
    }
}
 
 void craft_item() {
     CLEAR_SCREEN();
     draw_box("物品合成");
     
     printf("可合成配方：\n");
     printf("1. 能量饮料（2个） → 能量大补丸（恢复50能量）\n");
     printf("2. 宠物食品（3个） → 高级宠物粮（满饱食+20快乐）\n");
     printf("3. 彩票（5张） → 神秘钥匙（1把）\n");
     printf("4. 普通家具（3件） → 随机稀有家具\n");
     printf("0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 4);
     
     switch (choice) {
         case 1: {
             int count = 0;
             for (int i = 0; i < player.inventory_count; i++) {
                 if (strcmp(player.inventory[i].name, "能量饮料") == 0) {
                     count += player.inventory[i].quantity;
                 }
             }
             if (count >= 2) {
                 int to_remove = 2;
                 for (int i = 0; i < player.inventory_count && to_remove > 0; i++) {
                     if (strcmp(player.inventory[i].name, "能量饮料") == 0) {
                         int remove = (player.inventory[i].quantity < to_remove) ? player.inventory[i].quantity : to_remove;
                         player.inventory[i].quantity -= remove;
                         to_remove -= remove;
                         if (player.inventory[i].quantity <= 0) {
                             for (int j = i; j < player.inventory_count - 1; j++) {
                                 player.inventory[j] = player.inventory[j + 1];
                             }
                             player.inventory_count--;
                             i--;
                         }
                     }
                 }
                 bool found = false;
                 for (int i = 0; i < player.inventory_count; i++) {
                     if (strcmp(player.inventory[i].name, "能量大补丸") == 0) {
                         player.inventory[i].quantity++;
                         found = true;
                         break;
                     }
                 }
                 if (!found && player.inventory_count < MAX_INVENTORY) {
                     InventoryItem item;
                     safe_strcpy(item.name, "能量大补丸", 50);
                     item.price = 0;
                     item.value = 30;
                     item.rarity = RARITY_UNCOMMON;
                     item.is_consumable = true;
                     item.effect_value = 50;
                     safe_strcpy(item.description, "恢复50点能量", 100);
                     item.quantity = 1;
                     player.inventory[player.inventory_count++] = item;
                 }
                 player.craft_count++;
                 print_colored(COLOR_GREEN, "合成成功！获得能量大补丸\n");
             } else {
                 print_colored(COLOR_RED, "能量饮料不足！需要2个\n");
             }
             break;
         }
         case 2: {
             int count = 0;
             for (int i = 0; i < player.inventory_count; i++) {
                 if (strcmp(player.inventory[i].name, "宠物食品") == 0) {
                     count += player.inventory[i].quantity;
                 }
             }
             if (count >= 3) {
                 int to_remove = 3;
                 for (int i = 0; i < player.inventory_count && to_remove > 0; i++) {
                     if (strcmp(player.inventory[i].name, "宠物食品") == 0) {
                         int remove = (player.inventory[i].quantity < to_remove) ? player.inventory[i].quantity : to_remove;
                         player.inventory[i].quantity -= remove;
                         to_remove -= remove;
                         if (player.inventory[i].quantity <= 0) {
                             for (int j = i; j < player.inventory_count - 1; j++) {
                                 player.inventory[j] = player.inventory[j + 1];
                             }
                             player.inventory_count--;
                             i--;
                         }
                     }
                 }
                 bool found = false;
                 for (int i = 0; i < player.inventory_count; i++) {
                     if (strcmp(player.inventory[i].name, "高级宠物粮") == 0) {
                         player.inventory[i].quantity++;
                         found = true;
                         break;
                     }
                 }
                 if (!found && player.inventory_count < MAX_INVENTORY) {
                     InventoryItem item;
                     safe_strcpy(item.name, "高级宠物粮", 50);
                     item.price = 0;
                     item.value = 50;
                     item.rarity = RARITY_RARE;
                     item.is_consumable = true;
                     item.effect_value = 100;
                     safe_strcpy(item.description, "喂食宠物，恢复全部饱食度和20快乐度", 100);
                     item.quantity = 1;
                     player.inventory[player.inventory_count++] = item;
                 }
                 player.craft_count++;
                 print_colored(COLOR_GREEN, "合成成功！获得高级宠物粮\n");
             } else {
                 print_colored(COLOR_RED, "宠物食品不足！需要3个\n");
             }
             break;
         }
         case 3: {
             int count = 0;
             for (int i = 0; i < player.inventory_count; i++) {
                 if (strcmp(player.inventory[i].name, "彩票") == 0) {
                     count += player.inventory[i].quantity;
                 }
             }
             if (count >= 5) {
                 int to_remove = 5;
                 for (int i = 0; i < player.inventory_count && to_remove > 0; i++) {
                     if (strcmp(player.inventory[i].name, "彩票") == 0) {
                         int remove = (player.inventory[i].quantity < to_remove) ? player.inventory[i].quantity : to_remove;
                         player.inventory[i].quantity -= remove;
                         to_remove -= remove;
                         if (player.inventory[i].quantity <= 0) {
                             for (int j = i; j < player.inventory_count - 1; j++) {
                                 player.inventory[j] = player.inventory[j + 1];
                             }
                             player.inventory_count--;
                             i--;
                         }
                     }
                 }
                 player.mystery_keys++;
                 player.craft_count++;
                 print_colored(COLOR_GREEN, "合成成功！获得1把神秘钥匙\n");
             } else {
                 print_colored(COLOR_RED, "彩票不足！需要5张\n");
             }
             break;
         }
         case 4:
             print_colored(COLOR_YELLOW, "该配方正在开发中...\n");
             break;
         default:
             break;
     }
     
     check_achievements();
     press_any_key();
 }
 
 void trade_with_friend(int friend_index) {
     if (friend_index < 0 || friend_index >= player.friend_count) return;
     
     Friend* f = &player.friends[friend_index];
     if (f->friendship_level < 50) {
         print_colored(COLOR_RED, "好感度不足50，无法交易！\n");
         return;
     }
     
     printf("输入交易坤币数量：");
     char buffer[100];
     if (fgets(buffer, sizeof(buffer), stdin) == NULL) return;
     
     int amount;
     if (sscanf(buffer, "%d", &amount) != 1 || amount <= 0) {
         print_colored(COLOR_RED, "输入无效！\n");
         return;
     }
     
     if (!remove_coins(amount)) {
         print_colored(COLOR_RED, "坤币不足！\n");
         return;
     }
     
     player.total_expenses += amount;
     int exp_gain = amount / 10;
     player.kun_exp += exp_gain;
     f->friendship_level += 5;
     if (f->friendship_level > 100) f->friendship_level = 100;
     
     print_colored(COLOR_GREEN, "交易成功！获得 %d 经验，好感度+5\n", exp_gain);
 }
 
 void send_gift_to_friend(int friend_index) {
     if (friend_index < 0 || friend_index >= player.friend_count) return;
     
     Friend* f = &player.friends[friend_index];
     
     bool has_gift = false;
     for (int i = 0; i < player.inventory_count; i++) {
         if (strstr(player.inventory[i].name, "彩票") != NULL && player.inventory[i].quantity > 0) {
             has_gift = true;
             player.inventory[i].quantity--;
             if (player.inventory[i].quantity <= 0) {
                 for (int j = i; j < player.inventory_count - 1; j++) {
                     player.inventory[j] = player.inventory[j + 1];
                 }
                 player.inventory_count--;
             }
             break;
         }
     }
     
     if (!has_gift) {
         print_colored(COLOR_RED, "没有可赠送的礼物！\n");
         return;
     }
     
     f->friendship_level += 20;
     if (f->friendship_level > 100) f->friendship_level = 100;
     print_colored(COLOR_GREEN, "赠送成功！好感度+20\n");
 }
 
 void prestige() {
     if (player.prestige_level == 0) {
         int unlocked_achievements = 0;
         for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
             if (player.achievements[i].unlocked) unlocked_achievements++;
         }
         
         if (player.level < 50 || player.kun_coins < 1000000 || unlocked_achievements < 20) {
             print_colored(COLOR_RED, "不满足声望重置条件！\n");
             printf("   需要：等级50、100万坤币、20个成就\n");
             return;
         }
     }
     
     printf("确定要进行声望重置吗？这将重置大部分进度，但获得永久加成。\n");
     printf("输入 YES 确认：");
     char confirm[10];
     if (fgets(confirm, sizeof(confirm), stdin) == NULL) return;
     confirm[strcspn(confirm, "\n")] = '\0';
     
     if (strcmp(confirm, "YES") != 0) {
         print_colored(COLOR_YELLOW, "已取消\n");
         return;
     }
     
     int new_prestige_level = player.prestige_level + 1;
     int prestige_points = player.prestige_points + 100 * new_prestige_level;
     
     Achievement saved_achievements[MAX_ACHIEVEMENTS];
     memcpy(saved_achievements, player.achievements, sizeof(Achievement) * MAX_ACHIEVEMENTS);
     int saved_achievement_points = player.achievement_points;
     
     init_player();
     
     player.prestige_level = new_prestige_level;
     player.prestige_points = prestige_points;
     memcpy(player.achievements, saved_achievements, sizeof(Achievement) * MAX_ACHIEVEMENTS);
     player.achievement_points = saved_achievement_points;
     player.kun_coins = 1000;
     player.skill_points += 5 * new_prestige_level;
     player.has_phone = true;
     
     if (!player.achievements[15].unlocked) {
         unlock_achievement(15);
     }
     
     print_colored(COLOR_YELLOW, "\n?? 声望重置成功！\n");
     printf("   声望等级：%d\n", player.prestige_level);
     printf("   声望点数：%d\n", player.prestige_points);
     printf("   所有收入增加 %d%%\n", player.prestige_level * 5);
     
     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed && 
             strstr(player.active_quests[i].description, "声望重置") != NULL) {
             player.active_quests[i].progress++;
             if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                 complete_quest(i);
             }
         }
     }
 }
 
void buy_premium(int duration_days) {
    int cost = duration_days * 1000;
    if (duration_days > 365) cost = 100000;  // 永久会员，避免大整数溢出
    
    if (!remove_coins(cost)) {
        print_colored(COLOR_RED, "坤币不足！需要 %d\n", cost);
        return;
    }
    
    player.total_expenses += cost;
    player.has_premium = true;
    
    if (duration_days <= 365) {
        player.premium_until = time(NULL) + duration_days * 86400;
    } else {
        // 使用 time_t 类型避免溢出，100年约3153600000秒
      player.premium_until = time(NULL) + (long long)36500 * 86400LL;
    }
    
    print_colored(COLOR_GREEN, "\n?? 购买高级会员成功！有效期 %d 天\n", duration_days);
    printf("   会员特权：每日登录奖励翻倍，能量恢复速度+50%%\n");
}
 
 //================ UI显示函数 ================
 void show_friends() {
     CLEAR_SCREEN();
     draw_box("好友系统");
     
     printf("当前好友：%d/%d\n\n", player.friend_count, MAX_FRIENDS);
     
     for (int i = 0; i < player.friend_count; i++) {
         Friend* f = &player.friends[i];
         printf("%d. %s\n", i + 1, f->username);
         printf("   等级：%d\n", f->level);
         printf("   好感度：%.0f/100\n", f->friendship_level);
         printf("   状态：%s\n", f->is_online ? "在线" : "离线");
         printf("\n");
     }
     
     if (player.friend_count < MAX_FRIENDS) {
         printf("可添加好友：\n");
         for (int i = 0; i < MAX_FRIENDS; i++) {
             if (!potential_friends[i].is_friend && potential_friends[i].level > 0) {
                 printf("   %s（等级%d）\n", potential_friends[i].username, potential_friends[i].level);
                 potential_friends[i].is_friend = true;
                 break;
             }
         }
     }
     
     draw_separator();
     printf("1. 添加好友   2. 交易   3. 赠送礼物   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     
     if (choice == 1) {
         if (player.friend_count >= MAX_FRIENDS) {
             print_colored(COLOR_RED, "好友已满！\n");
         } else {
             for (int i = 0; i < MAX_FRIENDS; i++) {
                 if (!potential_friends[i].is_friend) {
                     player.friends[player.friend_count++] = potential_friends[i];
                     potential_friends[i].is_friend = true;
                     print_colored(COLOR_GREEN, "添加好友成功！\n");
                     break;
                 }
             }
         }
     } else if (choice == 2 && player.friend_count > 0) {
         printf("选择好友：");
         int idx = get_valid_input(1, player.friend_count) - 1;
         trade_with_friend(idx);
     } else if (choice == 3 && player.friend_count > 0) {
         printf("选择好友：");
         int idx = get_valid_input(1, player.friend_count) - 1;
         send_gift_to_friend(idx);
     }
     press_any_key();
 }
 
 void show_statistics() {
     CLEAR_SCREEN();
     draw_box("游戏统计");
     
     printf(COLOR_BOLD "游戏时长\n" COLOR_RESET);
     int hours = player.total_play_time / 3600;
     int mins = (player.total_play_time % 3600) / 60;
     int secs = player.total_play_time % 60;
     printf("   总游戏时间：%02d:%02d:%02d\n", hours, mins, secs);
     
     printf(COLOR_BOLD "\n?? 经济统计\n" COLOR_RESET);
     printf("   总收入：%lld 坤币\n", player.total_income);
     printf("   总支出：%lld 坤币\n", player.total_expenses);
     printf("   净资产：%lld 坤币\n", player.total_income - player.total_expenses);
     
     printf(COLOR_BOLD "\n?? 任务统计\n" COLOR_RESET);
     printf("   任务总数：%d\n", player.lifetime_task_completions);
     printf("   今日完成：%d\n", player.daily_task_completions);
     printf("   本周完成：%d\n", player.weekly_task_completions);
     
     printf(COLOR_BOLD "\n?? 资产统计\n" COLOR_RESET);
     printf("   家具：%d/%d\n", player.furniture_count, MAX_FURNITURE);
     printf("   宠物：%d/%d\n", player.pet_count, MAX_PETS);
     printf("   房产：%d/10\n", player.property_count);
     int stock_total = 0;
    for (int i = 0; i < 25; i++) {
        stock_total += player.invested_stocks[i];
    }
    printf("   股票持仓：%d股（共25支）\n", stock_total);
     
     printf(COLOR_BOLD "\n?? 战斗统计\n" COLOR_RESET);
     printf("   BOSS击败：%d\n", player.boss_defeats);
     printf("   节日参与：%d\n", player.festival_participations);
     
     printf(COLOR_BOLD "\n? 其他\n" COLOR_RESET);
     printf("   声望等级：%d\n", player.prestige_level);
     printf("   彩票剩余：%d\n", player.lottery_tickets);
     printf("   神秘钥匙：%d\n", player.mystery_keys);
     printf("   合成次数：%d\n", player.craft_count);
     
     printf(COLOR_BOLD "\n?? 状态\n" COLOR_RESET);
     printf("   饱食度：%d/100\n", player.hunger);
     printf("   血量：%d/100\n", player.health);
     printf("   精神：%d/100\n", player.spirit);
     
     draw_separator();
     press_any_key();
 }
 
 void show_premium_shop() {
     CLEAR_SCREEN();
     draw_box("会员商城");
     
     printf("会员特权：\n");
     printf("  ? 每日登录奖励翻倍\n");
     printf("  ? 能量恢复速度+50%%\n");
     printf("  ? 专属会员标识\n");
     printf("  ? 每月免费神秘钥匙\n\n");
     
     printf("1. 7天会员 - 7000坤币\n");
     printf("2. 30天会员 - 30000坤币\n");
     printf("3. 永久会员 - 100000坤币（仅限声望1以上）\n");
     printf("0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     switch (choice) {
         case 1:
             buy_premium(7);
             break;
         case 2:
             buy_premium(30);
             break;
         case 3:
             if (player.prestige_level >= 1) {
                 buy_premium(36500);
             } else {
                 print_colored(COLOR_RED, "需要声望等级1！\n");
             }
             break;
         default:
             break;
     }
     press_any_key();
 }
 
 void show_settings() {
     CLEAR_SCREEN();
     draw_box("游戏设置");
     
     printf("1. 显示帮助\n");
     printf("2. 关于游戏\n");
     printf("3. 重置进度\n");
     printf("4. 清除存档\n");
     printf("0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 4);
     char confirm;
     
     switch (choice) {
         case 1:
             show_help();
             break;
         case 2:
             printf("\n打工模拟器 v4.0 - 坤城篇\n");
             printf("作者：Ciallo0721-cmd\n");
             printf("世界设定：坤城 — 一座300万人口的沿海欲望都市\n");
             printf("叙事设计：NDD + GDD v4.0\n");
             printf("编译日期：%s %s\n", __DATE__, __TIME__);
             printf("感谢您的支持！\n");
             break;
         case 3:
             printf("确定重置所有进度？(y/n): ");
             confirm = _getch();
             printf("\n");
             if (confirm == 'y' || confirm == 'Y') {
                 remove(SAVE_FILE);
                 init_player();
                 init_tasks();
                 init_skills();
                 init_quests();
                 init_pets();
                 init_company_levels();
                 init_stocks();
                 init_chat_room();
                 print_colored(COLOR_GREEN, "游戏进度已重置！\n");
             }
             break;
         case 4:
             printf("确定删除存档文件？(y/n): ");
             confirm = _getch();
             printf("\n");
             if (confirm == 'y' || confirm == 'Y') {
                 if (remove(SAVE_FILE) == 0) {
                     print_colored(COLOR_GREEN, "存档已删除！\n");
                 } else {
                     print_colored(COLOR_RED, "删除失败！\n");
                 }
             }
             break;
         default:
             break;
     }
     press_any_key();
 }
 
 void show_help() {
     CLEAR_SCREEN();
     draw_box("游戏指南");
     
     printf(COLOR_BOLD "游戏目标\n" COLOR_RESET);
     printf("在坤城这座欲望都市里活下去，闯出名堂，定义你自己的成功。\n");
     printf("通过打工赚钱、结交朋友、投资理财、经营公司，成为你想成为的人。\n\n");
     
     printf(COLOR_BOLD "世界观\n" COLOR_RESET);
     printf("坤城是一座虚构的中国当代沿海城市，约300万人口。\n");
     printf("城区分为：老城区（租金低）、CBD（白领聚集）、开发区（工厂+物流）、\n");
     printf("大学城、富人区。你初始住在老城区出租屋。\n");
     printf("坤城不问你的过去，只问你的价格。\n\n");
     
     printf(COLOR_BOLD "核心玩法\n" COLOR_RESET);
     printf("1. 打工赚钱：在坤城各行各业打工，从出租车司机到企业家\n");
     printf("2. 商城购物：购买家具和物品提升生活质量\n");
     printf("3. 技能升级：提升10项技能增加任务收入\n");
     printf("4. 成就系统：完成特定目标获得奖励（叙事型成就）\n");
     printf("5. 随机事件：坤城每天都有新鲜事发生\n\n");
     
     printf(COLOR_BOLD "能量系统\n" COLOR_RESET);
     printf("每个任务消耗能量\n");
     printf("能量每10分钟恢复1点\n");
     printf("使用能量饮料可快速恢复\n\n");
     
     printf(COLOR_BOLD "特色功能\n" COLOR_RESET);
     printf("季节系统：春夏秋冬影响坤城的打工和投资环境\n");
     printf("宠物系统：猫咪/狗狗/鹦鹉/龙/凤凰提供额外加成\n");
     printf("公司经营：从个体户到传奇财团，一步步建立你的商业帝国\n");
     printf("股票市场：投资25支股票获得收益\n");
     printf("房产投资：在坤城各区购买房产获得稳定收入\n");
     printf("声望系统：重置游戏获得永久加成\n");
     printf("合成系统：合成高级物品\n");
     printf("好友系统：与小明、小红等坤城居民互动\n");
     printf("会员系统：购买会员享特权\n");
     printf("手机聊天室：与路人甲、小妹、老陈等坤城人物社交\n");
     printf("状态系统：饱食度/血量/精神影响你的坤城生活\n");
     
     draw_separator();
 }
 
void show_status() {
     CLEAR_SCREEN();
     draw_box("玩家状态");

     printf(COLOR_BOLD "基本信息\n" COLOR_RESET);
     printf(COLOR_BOLD "   玩家名称：" COLOR_RESET COLOR_GREEN "%s\n" COLOR_RESET, player.player_name);
     printf("   等级：%d\n", player.level);
    
    // 经验进度条
    int exp_max = player.level * 100;
    int bar_len = 20;
    printf("   经验：");
    int filled = (exp_max > 0) ? player.kun_exp * bar_len / exp_max : 0;
    if (filled > bar_len) filled = bar_len;
    for (int i = 0; i < bar_len; i++) {
        if (i < filled) printf("#");
        else printf("-");
    }
    printf(COLOR_CYAN " %d/%d\n" COLOR_RESET, player.kun_exp, exp_max);
    
    printf("   坤币：%lld\n", player.kun_coins);
    printf("   时间槽：%d/%d（上午/下午/晚上）\n", player.daily_time_slots, player.max_time_slots);
    printf("   能量：%d/%d（保留兼容）\n", player.energy, player.max_energy);
    printf("   技能点：%d\n", player.skill_points);
    printf("   成就点：%d\n", player.achievement_points);
    
    printf(COLOR_BOLD "\n?? 游戏进度\n" COLOR_RESET);
    printf("   游戏天数：第 %d 天 | 季节：", player.day_count);
    switch (player.current_season) {
        case SEASON_SPRING: printf(COLOR_GREEN "春\n" COLOR_RESET); break;
        case SEASON_SUMMER: printf(COLOR_RED "夏\n" COLOR_RESET); break;
        case SEASON_AUTUMN: printf(COLOR_YELLOW "秋\n" COLOR_RESET); break;
        case SEASON_WINTER: printf(COLOR_CYAN "冬\n" COLOR_RESET); break;
    }
    printf("   今日收入：%d 坤币\n", player.today_income);
    printf("   今日打工：%d 次\n", player.today_work_count);
    printf("   今日事件：%d 次\n", player.today_event_count);
    printf("   每日成本：%d 坤币（房租%d+生活%d）\n", player.rent_cost + player.living_cost, player.rent_cost, player.living_cost);
    printf("   连续登录：%d 天\n", player.consecutive_days);
    printf("   总任务完成：%d 次\n", player.lifetime_task_completions);
    printf("   公司等级：%s\n", player.company_levels[player.current_company_level].name);
    printf("   声望等级：%d\n", player.prestige_level);
    
    printf(COLOR_BOLD "\n?? 资产统计\n" COLOR_RESET);
    printf("   总收入：%lld 坤币\n", player.total_income);
    printf("   总支出：%lld 坤币\n", player.total_expenses);
    printf("   净资产：%lld 坤币\n", player.total_income - player.total_expenses);
    printf("   家具数量：%d/%d\n", player.furniture_count, MAX_FURNITURE);
    printf("   宠物数量：%d/%d\n", player.pet_count, MAX_PETS);
    printf("   房产数量：%d/10\n", player.property_count);
    
    printf(COLOR_BOLD "\n?? 生存状态\n" COLOR_RESET);

    // 生存状态安全钳（最后防线）
    if (player.hunger > 100 || player.hunger < 0) { player.hunger = 100; }
    if (player.health > 100 || player.health < 0) { player.health = 100; }
    if (player.spirit > 100 || player.spirit < 0) { player.spirit = 50; }
    
    // 饱食度进度条
    printf("   饱食度：");
    filled = player.hunger * bar_len / 100;
    for (int i = 0; i < bar_len; i++) {
        if (i < filled) printf("#");
        else printf("-");
    }
    if (player.hunger < 20) print_colored(COLOR_RED, " %d/100 [饥饿!]");
    else if (player.hunger < 50) print_colored(COLOR_YELLOW, " %d/100");
    else print_colored(COLOR_GREEN, " %d/100");
    printf("\n");
    
    // 血量进度条
    printf("   血量：  ");
    filled = player.health * bar_len / 100;
    for (int i = 0; i < bar_len; i++) {
        if (i < filled) printf("#");
        else printf("-");
    }
    if (player.health < 30) print_colored(COLOR_RED, " %d/100 [重伤!]");
    else if (player.health < 60) print_colored(COLOR_YELLOW, " %d/100");
    else print_colored(COLOR_GREEN, " %d/100");
    printf("\n");
    
    // 精神进度条
    printf("   精神：  ");
    filled = player.spirit * bar_len / 100;
    for (int i = 0; i < bar_len; i++) {
        if (i < filled) printf("#");
        else printf("-");
    }
    if (player.spirit < 20) print_colored(COLOR_RED, " %d/100 [精神错乱!]");
    else if (player.spirit > 78) print_colored(COLOR_MAGENTA, " %d/100 [异常兴奋]");
    else print_colored(COLOR_CYAN, " %d/100");
    printf("\n");
    
    printf(COLOR_BOLD "\n?? 当前季节：" COLOR_RESET);
    switch (player.current_season) {
        case SEASON_SPRING: printf("春季\n"); break;
        case SEASON_SUMMER: printf("夏季\n"); break;
        case SEASON_AUTUMN: printf("秋季\n"); break;
        case SEASON_WINTER: printf("冬季\n"); break;
    }
    
    if (player.has_premium) {
        time_t remaining = player.premium_until - time(NULL);
        if (remaining > 0) {
            printf(COLOR_YELLOW "   [会员] 剩余：%lld 天\n" COLOR_RESET, (long long)(remaining / 86400 + 1));
        } else {
            player.has_premium = false;
        }
    }
    
    if (player.is_hallucinating) {
        print_colored(COLOR_RED, "\n[幻觉状态] 文字可能会变成乱码！\n");
    }
    
    show_active_events();
    draw_separator();
}
 
 void show_inventory() {
     CLEAR_SCREEN();
     draw_box("背包物品");
     
     if (player.inventory_count == 0) {
         printf("背包空空如也...\n");
     } else {
         printf("物品数量：%d/%d\n\n", player.inventory_count, MAX_INVENTORY);
         
         for (int i = 0; i < player.inventory_count; i++) {
             InventoryItem* item = &player.inventory[i];
             
             const char* color = COLOR_WHITE;
             switch (item->rarity) {
                 case RARITY_LEGENDARY: color = COLOR_YELLOW; break;
                 case RARITY_EPIC: color = COLOR_MAGENTA; break;
                 case RARITY_RARE: color = COLOR_BLUE; break;
                 case RARITY_UNCOMMON: color = COLOR_GREEN; break;
                 default: color = COLOR_WHITE; break;
             }
             
             printf("%s%s", color, item->name);
             if (item->quantity > 1) {
                 printf(" × %d", item->quantity);
             }
             printf(COLOR_RESET "\n");
             printf("   %s\n", item->description);
             printf("   稀有度：");
             switch (item->rarity) {
                 case RARITY_COMMON: printf("普通"); break;
                 case RARITY_UNCOMMON: printf("优秀"); break;
                 case RARITY_RARE: printf("稀有"); break;
                 case RARITY_EPIC: printf("史诗"); break;
                 case RARITY_LEGENDARY: printf("传说"); break;
             }
             printf("   数量：%d\n\n", item->quantity);
         }
     }
     
     draw_separator();
     printf("1. 使用物品   2. 整理背包   3. 合成物品   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     if (choice == 1 && player.inventory_count > 0) {
         printf("选择要使用的物品 (1-%d): ", player.inventory_count);
         int item_choice = get_valid_input(1, player.inventory_count);
         use_inventory_item(item_choice - 1);
     } else if (choice == 2) {
         print_colored(COLOR_GREEN, "背包已整理！\n");
     } else if (choice == 3) {
         craft_item();
     }
     press_any_key();
 }
 
 void use_inventory_item(int item_index) {
     if (item_index < 0 || item_index >= player.inventory_count) return;
     
     InventoryItem* item = &player.inventory[item_index];
     
     if (item->quantity <= 0) {
         print_colored(COLOR_RED, "物品数量不足！\n");
         return;
     }
     
     item->quantity--;
     
     if (strcmp(item->name, "能量饮料") == 0) {
         player.energy += 20;
         if (player.energy > player.max_energy) player.energy = player.max_energy;
         print_colored(COLOR_GREEN, "使用能量饮料，恢复20点能量！\n");
     } else if (strcmp(item->name, "能量大补丸") == 0) {
         player.energy += 50;
         if (player.energy > player.max_energy) player.energy = player.max_energy;
         print_colored(COLOR_GREEN, "使用能量大补丸，恢复50点能量！\n");
     } else if (strcmp(item->name, "彩票") == 0) {
         player.lottery_tickets++;
         print_colored(COLOR_GREEN, "获得1张彩票！\n");
     } else if (strcmp(item->name, "宠物食品") == 0) {
         print_colored(COLOR_GREEN, "请在宠物界面喂食\n");
         item->quantity++;
         return;
     } else if (strcmp(item->name, "高级宠物粮") == 0) {
         print_colored(COLOR_GREEN, "请在宠物界面喂食\n");
         item->quantity++;
         return;
     } else if (strcmp(item->name, "面包") == 0) {
         player.hunger += 15;
         if (player.hunger > 100) player.hunger = 100;
         print_colored(COLOR_GREEN, "食用面包，饱食度+15\n");
     } else if (strcmp(item->name, "盒饭") == 0) {
         player.hunger += 30;
         if (player.hunger > 100) player.hunger = 100;
         print_colored(COLOR_GREEN, "食用盒饭，饱食度+30\n");
     } else {
         print_colored(COLOR_YELLOW, "该物品无法使用\n");
     }
     
     if (item->quantity <= 0) {
         for (int i = item_index; i < player.inventory_count - 1; i++) {
             player.inventory[i] = player.inventory[i + 1];
         }
         player.inventory_count--;
     }
 }
 
 void show_quests() {
     CLEAR_SCREEN();
     draw_box("任务列表");
     
     int active_count = 0;
     for (int i = 0; i < MAX_QUESTS; i++) {
         if (!player.active_quests[i].completed && 
             (player.active_quests[i].deadline == 0 || player.active_quests[i].deadline > time(NULL)) &&
             player.active_quests[i].assigned_time != 0) {
             active_count++;
         }
     }
     
     printf("进行中的任务：%d\n\n", active_count);
     
     for (int i = 0; i < MAX_QUESTS; i++) {
         Quest* quest = &player.active_quests[i];
         
         if (quest->assigned_time == 0) continue;
         
         if (quest->completed) {
             printf(COLOR_GREEN "%s\n" COLOR_RESET, quest->name);
             printf("   已完成！\n");
         } else if (quest->deadline != 0 && quest->deadline < time(NULL)) {
             printf(COLOR_RED "%s\n" COLOR_RESET, quest->name);
             printf("   已过期\n");
         } else {
             if (player.is_hallucinating && (rand() % 2)) {
                 printf(COLOR_RED "????\n" COLOR_RESET);
                 printf("   ￥#@%% !!!\n");
             } else {
                 printf(COLOR_CYAN "%s\n" COLOR_RESET, quest->name);
                 printf("   %s\n", quest->description);
                 printf("   进度：%d/%d\n", quest->progress, quest->requirement);
             }
             
             if (quest->deadline != 0) {
                 time_t remaining = quest->deadline - time(NULL);
                 if (remaining > 0) {
                     int days = (int)(remaining / 86400);
                     int hours = (int)((remaining % 86400) / 3600);
                     printf("   剩余时间：%d天%d小时\n", days, hours);
                 }
             }
         }
         printf("\n");
     }
     
     draw_separator();
     printf("1. 领取已完成任务奖励   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 1);
     if (choice == 1) {
         for (int i = 0; i < MAX_QUESTS; i++) {
             if (player.active_quests[i].completed) {
                 complete_quest(i);
             }
         }
     }
     press_any_key();
 }
 
 void show_pets() {
     CLEAR_SCREEN();
     draw_box("宠物系统");
     
     printf("已解锁宠物：%d/%d\n\n", player.pet_count, MAX_PETS);
     
     const char* pet_type_names[] = {"猫咪", "狗狗", "鹦鹉", "龙", "凤凰"};
     
     for (int i = 0; i < MAX_PETS; i++) {
         Pet* pet = &player.pets[i];
         
         if (pet->is_unlocked) {
             printf("%s\n", pet->name);
             printf("   类型：%s\n", pet_type_names[pet->type]);
             printf("   等级：%d\n", pet->level);
             printf("   快乐度：");
             print_progress_bar(pet->happiness, 100, 20);
             printf("\n   饱食度：");
             print_progress_bar(pet->hunger, 100, 20);
             printf("\n   加成倍数：%.2f\n\n", pet->bonus_multiplier);
         } else {
             printf("未解锁宠物\n");
             printf("   类型：%s\n", pet_type_names[i]);
             switch (i) {
                 case 1: printf("   解锁条件：达到等级5，5000坤币"); break;
                 case 2: printf("   解锁条件：达到等级15，50000坤币"); break;
                 case 3: printf("   解锁条件：达到等级30，500000坤币"); break;
                 case 4: printf("   解锁条件：达到等级50，5000000坤币"); break;
                 default: printf("   初始宠物"); break;
             }
             printf("\n\n");
         }
     }
     
     draw_separator();
     printf("1. 喂食宠物   2. 玩耍   3. 领养新宠物   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     if (choice > 0) {
         if (player.pet_count == 0) {
             print_colored(COLOR_RED, "没有宠物！\n");
         } else if (choice <= 2) {
             printf("选择宠物 (1-%d): ", player.pet_count);
             int pet_choice = get_valid_input(1, player.pet_count);
             int idx = -1;
             for (int i = 0; i < MAX_PETS; i++) {
                 if (player.pets[i].is_unlocked) {
                     if (--pet_choice == 0) {
                         idx = i;
                         break;
                     }
                 }
             }
             if (idx >= 0) {
                 if (choice == 1) feed_pet(idx);
                 else play_with_pet(idx);
             }
         } else if (choice == 3) {
             printf("选择要领养的宠物类型：\n");
             printf("1. 狗狗（等级5，5000坤币）\n");
             printf("2. 鹦鹉（等级15，50000坤币）\n");
             printf("3. 龙（等级30，500000坤币）\n");
             printf("4. 凤凰（等级50，5000000坤币）\n");
             printf("0. 返回\n");
             int type_choice = get_valid_input(0, 4);
             if (type_choice > 0) {
                 adopt_pet((PetType)type_choice);
             }
         }
     }
     press_any_key();
 }
 
 void show_company() {
     CLEAR_SCREEN();
     draw_box("公司管理");
     
     CompanyLevel* current = &player.company_levels[player.current_company_level];
     
     printf(COLOR_BOLD "当前公司：%s\n" COLOR_RESET, current->name);
     printf("   %s\n", current->description);
     printf("   收入倍数：%.1f\n", current->income_multiplier);
     
     if (player.current_company_level < MAX_COMPANY_LEVELS - 1) {
         CompanyLevel* next = &player.company_levels[player.current_company_level + 1];
         printf(COLOR_BOLD "\n下一等级：%s\n" COLOR_RESET, next->name);
         printf("   %s\n", next->description);
         printf("   升级需要：%lld 坤币\n", next->required_coins);
         printf("   将获得：%d 技能点\n", next->unlock_skill_points);
         printf("   新收入倍数：%.1f\n", next->income_multiplier);
         
         float progress = (float)player.kun_coins / next->required_coins;
         if (progress > 1) progress = 1;
         printf("   进度：");
         print_progress_bar((int)(progress * 100), 100, 30);
     } else {
         print_colored(COLOR_YELLOW, "\n?? 公司已达到最高等级！\n");
     }
     
     draw_separator();
     if (player.current_company_level < MAX_COMPANY_LEVELS - 1) {
         printf("1. 升级公司   0. 返回\n");
         int choice = get_valid_input(0, 1);
         if (choice == 1) {
             level_up_company();
         }
     } else {
         printf("0. 返回\n");
         get_valid_input(0, 0);
     }
    press_any_key();
 }

//================ 股票行情图窗口（新开CMD） ================
void open_stock_chart_window() {
#ifdef _WIN32
    // 先保存一次最新数据
    save_stock_history();
    
    // 尝试多种路径启动 Python 行情图脚本
    const char* chart_script_paths[] = {
        "源码/stock_chart.py",
        "./源码/stock_chart.py",
        ".\\源码\\stock_chart.py",
        "stock_chart.py",
    };
    
    // 尝试多个 Python 命令
    const char* python_variants[] = {
        "python",
        "python3",
        "C:\\Users\\nb666\\AppData\\Local\\Microsoft\\WindowsApps\\python.exe"
    };
    
    int launched = 0;
    for (int p = 0; p < 3 && !launched; p++) {
        for (int s = 0; s < 4 && !launched; s++) {
            char cmd[1024];
            // 用 start 开新窗口，cmd /k 保持窗口不关闭
            snprintf(cmd, sizeof(cmd), 
                     "start \"股票行情图\" cmd /c \"%s %s && pause\"",
                     python_variants[p], chart_script_paths[s]);
            int ret = system(cmd);
            if (ret != -1) {
                launched = 1;
            }
        }
    }
    
    if (launched) {
        print_colored(COLOR_GREEN, "已打开股票行情图窗口！\n");
    } else {
        print_colored(COLOR_YELLOW, "无法打开行情图窗口，请确保 Python 已安装\n");
        print_colored(COLOR_YELLOW, "   手动运行: python 源码/stock_chart.py\n");
    }
#else
    print_colored(COLOR_YELLOW, "行情图窗口仅支持 Windows\n");
#endif
    press_any_key();
}

/*================ v5.0 TUI 股票终端 ================*/
/*================ v5.0/v5.1 TUI 股票终端 ================*/
static int g_stock_focus = 0;   /* 1 = 图表占满宽度（C 键切换） */
static int g_stock_view  = 0;   /* 0 = 行情   1 = 新闻 */
static int g_news_filter = -1;  /* -1 全部 / 0 娱乐 / 1 经济 / 2 政治 */

static const char* stk_status_tag(const Stock* s)
{
    switch (s->status) {
    case STK_RISK:     return "ST";
    case STK_HALT:     return "停";
    case STK_DELISTED: return "退";
    }
    return "";
}

static int stk_status_color(const Stock* s)
{
    switch (s->status) {
    case STK_RISK:     return TC_ORANGE;
    case STK_HALT:     return TC_MAGENTA;
    case STK_DELISTED: return TC_FAINT;
    }
    return TC_TEXT;
}

/* 新闻面板 */
static void stock_draw_news(int x, int y, int w, int h, int filter)
{
    int row = 0, k;
    NewsItem* it;
    char line[320];
    const char* ttl;

    ttl = (filter < 0) ? "坤城新闻 · 全部"
        : (filter == NEWS_TYPE_ENT ? "坤城新闻 · 娱乐"
        : (filter == NEWS_TYPE_ECO ? "坤城新闻 · 经济" : "坤城新闻 · 政治"));
    tc_panel(x, y, w, h, ttl, TC_BORDER, TC_BG2, TC_ORANGE);

    if (g_news_n <= 0) {
        tc_puts(x + 2, y + 1, "还没有新闻。市场安静得不像坤城。", TC_FAINT, TC_BG2);
        return;
    }

    for (k = 0; k < g_news_n && row < h - 2; k++) {
        int tc;
        it = news_recent(k);
        if (!it) break;
        if (filter >= 0 && it->type != filter) continue;

        tc = (it->type == NEWS_TYPE_ENT) ? TC_PINK :
             (it->type == NEWS_TYPE_ECO ? TC_YELLOW : TC_CYAN);

        /* 标题行：类型标签 + 标题 + 影响 */
        snprintf(line, sizeof line, "%s", it->head);
        tc_putsn(x + 2, y + 1 + row, w - 4, line, tc, TC_BG2);
        if (w > 40 && it->impact != 0) {
            char imp[32];
            snprintf(imp, sizeof imp, "%+d%%", it->impact);
            tc_puts(x + w - 8, y + 1 + row, imp,
                    it->impact > 0 ? TC_RED : TC_GREEN, TC_BG2);
        }
        row++;
        if (row >= h - 2) break;

        /* 正文行（截断，够宽才画） */
        if (w > 44) {
            char clip[320];
            int lim = (w - 12) / 2;   /* 中文按 2 列算 */
            if (lim > 90) lim = 90;
            if (lim < 8) lim = 8;
            news_clip(it->body, clip, sizeof clip, lim);
            tc_putsn(x + 2, y + 1 + row, w - 4, clip, TC_DIM, TC_BG2);
            row++;
        }
    }
}

static void stock_draw(int sel, int live, int blink, const char* msg, int view = 0)
{
    int W, H;
    int cy = 1, ch;
    int lw, rx, rw, i;
    Stock* s;
    int line_fg;
    int list_start, list_h;
    char buf[256];

    tui_begin();
    W = tui_cv.w; H = tui_cv.h;
    ch = H - 3;
    tui_draw_topbar();

    if (g_stock_focus || view == 1) {
        lw = 0;
        rx = 0;
        rw = W;
    } else {
        lw = (W >= 104) ? 42 : W / 2;
        rx = lw + 1;
        rw = W - rx - 1;
        if (rw < 34) { lw = W * 3 / 5; rx = lw + 1; rw = W - rx - 1; }
    }

    s = &player.stocks[sel < STOCK_COUNT ? sel : 0];

    /* ===== 左侧行情榜 ===== */
    if (lw > 0) {
        char ttl[64];
        snprintf(ttl, sizeof ttl, "行情榜 %d/%d", sel + 1, STOCK_COUNT);
        tc_panel(0, cy, lw, ch, ttl, TC_BORDER, TC_BG2, TC_YELLOW);

        list_h = ch - 2;
        if (list_h < 0) list_h = 0;
        list_start = 0;
        if (sel >= list_h) list_start = sel - list_h + 1;

        for (i = 0; i < list_h && list_start + i < STOCK_COUNT; i++) {
            int idx = list_start + i;
            Stock* st = &player.stocks[idx];
            int y = cy + 1 + i;
            int on = (idx == sel);
            int bg = on ? TC_BGHI : TC_BG2;
            char nb[8], pb[12], db[12];
            const char* tag = stk_status_tag(st);
            int name_fg;

            tc_fill(1, y, lw - 2, 1, " ", TC_TEXT, bg);
            tc_put(1, y, on ? ">" : " ", on ? TC_YELLOW : TC_DIM, bg);
            snprintf(nb, sizeof nb, "%02d", idx + 1);
            tc_puts(3, y, nb, TC_FAINT, bg);

            if (tag[0] && lw >= 34) {
                tc_puts(6, y, tag, stk_status_color(st), bg);
                name_fg = (st->status == STK_DELISTED) ? TC_FAINT : (on ? TC_WHITE : TC_TEXT);
                tc_putsn(8, y, 10, st->name, name_fg, bg);
            } else {
                name_fg = (st->status == STK_DELISTED) ? TC_FAINT : (on ? TC_WHITE : TC_TEXT);
                tc_putsn(6, y, 12, st->name, name_fg, bg);
            }

            if (lw >= 34) {
                if (st->status == STK_DELISTED) {
                    tc_puts(19, y, "退市", TC_FAINT, bg);
                } else if (st->status == STK_HALT) {
                    tc_puts(19, y, "停牌", TC_MAGENTA, bg);
                } else {
                    int dlt = st->current_price - st->prev_close;
                    char pbuf[16], ptmp[16];
                    stk_price_str(ptmp, sizeof ptmp, st->current_price);
                    snprintf(pbuf, sizeof pbuf, "%7s", ptmp);
                    tc_puts(19, y, pbuf, on ? TC_WHITE : TC_DIM, bg);
                    snprintf(db, sizeof db, "%+5.1f%%",
                             st->prev_close > 0
                             ? (float)dlt * 100.0f / st->prev_close : 0.0f);
                    tc_puts(27, y, db,
                            dlt > 0 ? TC_RED : (dlt < 0 ? TC_GREEN : TC_FAINT), bg);
                }
            }
            if (lw >= 40 && g_trend_n > 1 && st->status != STK_DELISTED) {
                tc_spark(34, y, lw - 36, g_trend[idx], g_trend_n,
                         g_trend[idx][0], TC_RED, TC_GREEN, bg);
            }
        }
    }

    /* ===== 右侧：新闻 or 行情 ===== */
    if (view == 1) {
        stock_draw_news(rx, cy, rw, ch - 1, g_news_filter);
        snprintf(buf, sizeof buf,
                 " [N] 回行情  [1]娱乐 [2]经济 [3]政治 [0]全部  [PgUp/PgDn] 上下  [Esc] 返回 ");
        tui_draw_keybar(buf);
        if (msg && msg[0]) tc_puts(2, H - 1, msg, TC_YELLOW, TC_BG);
        else tc_puts(2, H - 1, "坤透社 · 以上新闻均可能影响对应标的，别全信，也别全不信。", TC_FAINT, TC_BG);
        return;
    }

    {
        int kh = 7;
        int kx = rx + 2, ky = cy + 1;
        char nm[128];

        snprintf(nm, sizeof nm, "%s · %s · %s · %s",
                 s->name,
                 STOCK_SECTOR_NAME[s->sector],
                 s->status == STK_NORMAL ? "正常" :
                 (s->status == STK_RISK ? "ST风险" :
                  (s->status == STK_HALT ? "停牌" : "已退市")),
                 live ? (blink ? "*实时" : "o实时") : "-暂停");
        tc_panel(rx, cy, rw, kh, nm,
                 live ? TC_BORDER : TC_BG3,
                 TC_BG2,
                 live ? (blink ? TC_RED : TC_REDD) : TC_FAINT);

        /* 第一行：现价 / 涨跌 / 区间 */
        tc_puts(kx, ky, "现价", TC_DIM, TC_BG2);
        stk_price_str(buf, sizeof buf, s->current_price);
        tc_puts(kx + 5, ky, buf, TC_WHITE, TC_BG2);

        {
            int diff = s->current_price - s->prev_close;
            float pct = s->prev_close > 0 ? (float)diff * 100.0f / s->prev_close : 0;
            snprintf(buf, sizeof buf, "%+.2f (%+.2f%%)", diff / 100.0, pct);
            tc_puts(kx + 14, ky, buf, diff >= 0 ? TC_RED : TC_GREEN, TC_BG2);
        }

        if (rw > 58) {
            snprintf(buf, sizeof buf, "区间 %d~%d", s->day_low, s->day_high);
            tc_puts(rx + rw - 18, ky, buf, TC_DIM, TC_BG2);
        }
        ky++;

        /* 第二行：持仓 */
        {
            int hold = player.invested_stocks[sel];
            tc_puts(kx, ky, "持仓", TC_DIM, TC_BG2);
            snprintf(buf, sizeof buf, "%d 股", hold);
            tc_puts(kx + 5, ky, buf, hold > 0 ? TC_CYAN : TC_FAINT, TC_BG2);
            if (hold > 0) {
                long long mv = (long long)hold * s->current_price / 100;
                char mb[32];
                tui_fmt_money(mb, sizeof mb, mv);
                snprintf(buf, sizeof buf, "市值 %s", mb);
                tc_puts(kx + 16, ky, buf, TC_YELLOW, TC_BG2);
                if (player.stock_margin_debt[sel] > 0 && rw > 62) {
                    char lb[32];
                    tui_fmt_money(lb, sizeof lb, (long long)player.stock_margin_debt[sel]);
                    snprintf(buf, sizeof buf, "融资 %s", lb);
                    tc_puts(rx + rw - 18, ky, buf, TC_MAGENTA, TC_BG2);
                }
            }
        }
        ky++;

        /* 第三行：开 / 昨收 / 最高 / 最低 */
        {
            char a1[16], a2[16], a3[16], a4[16];
            stk_price_str(a1, sizeof a1, s->open_price);
            stk_price_str(a2, sizeof a2, s->prev_close);
            stk_price_str(a3, sizeof a3, s->day_high);
            stk_price_str(a4, sizeof a4, s->day_low);
            snprintf(buf, sizeof buf, "开 %s  昨收 %s  高 %s  低 %s", a1, a2, a3, a4);
        }
        tc_puts(kx, ky, buf, TC_DIM, TC_BG2);
        ky++;

        /* 第四行：波动 / 基准 / 交易日 / 担保比例 */
        {
            int ratio = stk_maint_ratio();
            char bbase[16];
            stk_price_str(bbase, sizeof bbase, s->base_price);
            snprintf(buf, sizeof buf, "波动 %d%%  基准 %s  第 %d 交易日  %d/%d",
                     s->volatility, bbase, player.stock_day,
                     player.stock_tick, STOCK_TICKS_PER_DAY);
            tc_puts(kx, ky, buf, TC_FAINT, TC_BG2);
            if (player.stock_loan_total > 0 && rw > 58) {
                snprintf(buf, sizeof buf, "担保 %d%%", ratio);
                tc_puts(rx + rw - 12, ky, buf,
                        ratio < STOCK_MAINT_RATIO ? TC_RED :
                        (ratio < 160 ? TC_ORANGE : TC_GREEN), TC_BG2);
            }
        }
        ky++;

        /* 第五行：风险提示 */
        if (s->status == STK_RISK) {
            snprintf(buf, sizeof buf, "ST 股 · 连续 %d 日低迷 · 再 %d 日未改善即终止上市",
                     s->bad_days, STOCK_ST_DAYS - s->st_days);
            tc_puts(kx, ky, buf, TC_ORANGE, TC_BG2);
        } else if (s->status == STK_HALT) {
            tc_puts(kx, ky, "停牌中 · 停牌结束后将按清算价强制变现持仓", TC_MAGENTA, TC_BG2);
        } else if (s->status == STK_DELISTED) {
            tc_puts(kx, ky, "已退市 · 永久无法交易", TC_FAINT, TC_BG2);
        } else if (s->current_price < s->base_price * 45 / 100) {
            tc_puts(kx, ky, "跌破基准价 45% · 距离 ST 只差一步", TC_RED, TC_BG2);
        } else if (s->limit_lock == 1) {
            tc_puts(kx, ky, "封涨停 · 买不进", TC_RED, TC_BG2);
        } else if (s->limit_lock == -1) {
            tc_puts(kx, ky, "封跌停 · 卖不出", TC_GREEN, TC_BG2);
        } else {
            int st_line = s->base_price * STOCK_ST_LINE / 100;
            int dl_line = s->base_price * STOCK_DELIST_LINE / 100;
            int to_st = s->current_price > 0 ? (s->current_price - st_line) * 100 / s->current_price : 0;
            int to_dl = s->current_price > 0 ? (s->current_price - dl_line) * 100 / s->current_price : 0;
            snprintf(buf, sizeof buf, "再跌 %d%% 触 ST · 再跌 %d%% 触退市 · 涨跌停 ±%d%%",
                     to_st, to_dl, stk_limit_pct(s));
            tc_puts(kx, ky, buf, to_st < 20 ? TC_ORANGE : TC_FAINT, TC_BG2);
        }
    }

    /* ===== 右侧：折线图 ===== */
    {
        int chy = cy + 7;
        int chh = ch - 9;
        int base = (g_trend_n > 0) ? g_trend[sel][0] : s->current_price;
        line_fg = (s->current_price >= base) ? TC_RED : TC_GREEN;
        if (chh >= 6) {
            int start = 0, nn = g_trend_n;
            int cap = rw - 12;
            int ybuf[TREND_MAX];
            int q;
            if (cap < 12) cap = 12;
            if (nn > cap) { start = nn - cap; nn = cap; }
            if (nn > TREND_MAX) nn = TREND_MAX;
            for (q = 0; q < nn; q++) ybuf[q] = g_trend[sel][start + q] / 100;
            tc_linechart(rx, chy, rw, chh, ybuf, nn,
                         "分时走势 · 涨红跌绿", line_fg, TC_FAINT, TC_BG2, 1);
        }
    }

    /* ===== 底部 ===== */
    snprintf(buf, sizeof buf,
             " [方向键] 选股  [B] 买入  [S] 卖出  [N] 新闻(%d新)  [空格] %s  [R] 跳档  [C] 放大  [Esc] 返回 ",
             player.stock_news_unread,
             live ? "暂停" : "实时");
    tui_draw_keybar(buf);

    if (msg && msg[0]) tc_puts(2, H - 1, msg, TC_YELLOW, TC_BG);
    else {
        NewsItem* it = news_recent(0);
        char tick[320];
        if (it) {
            snprintf(tick, sizeof tick, "坤透社 | %s", it->head);
            tc_puts(2, H - 1, tick, it->type == NEWS_TYPE_ENT ? TC_PINK :
                    (it->type == NEWS_TYPE_ECO ? TC_YELLOW : TC_CYAN), TC_BG);
        } else {
            tc_puts(2, H - 1, "提示：行情每秒推进一次，每 120 tick 结算一个交易日", TC_FAINT, TC_BG);
        }
    }
}

void show_stock_market() {
    int sel = 0;
    int live = 1;
    int blink = 0;
    int view = 0;
    time_t last_tick = time(NULL);
    char msg[192];
    time_t msg_until = 0;

    tui_page_enter("股票市场");
    g_stock_focus = 0;
    g_stock_view = 0;
    g_news_filter = -1;
    tui_trend_load();
    if (g_trend_n <= 1 && player.stocks[0].current_price > 0) tui_trend_push();

    /* 首次进入先开市发一条新闻，别让新闻页空着 */
    if (g_news_n == 0) news_daily_refresh();

    msg[0] = 0;

    for (;;) {
        int k;

        if (live && view == 0) {
            time_t now = time(NULL);
            if (now != last_tick) {
                last_tick = now;
                tui_stock_live_tick();
                blink = !blink;
                /* 有人被强平/退市就弹提示 */
                if (player.stock_loan_total > 0 && stk_maint_ratio() < 130) {
                    snprintf(msg, sizeof msg, "!! 担保比例跌破 130%%，券商正在强制平仓 !!");
                    msg_until = time(NULL) + 4;
                }
            }
        } else if (view == 0) {
            last_tick = time(NULL);
        }

        if (msg_until && time(NULL) > msg_until) { msg[0] = 0; msg_until = 0; }

        stock_draw(sel, live, blink, msg, view);
        tui_present();

        k = tui_key_nb();
        if (k < 0) { tui_sleep_ms(60); continue; }

        if (k == 27 || k == 'q') {
            if (view == 1) { view = 0; g_stock_view = 0; continue; }
            break;
        }

        if (view == 1) {
            if (k == 'n') { view = 0; continue; }
            if (k == '1') { g_news_filter = NEWS_TYPE_ENT; continue; }
            if (k == '2') { g_news_filter = NEWS_TYPE_ECO; continue; }
            if (k == '3') { g_news_filter = NEWS_TYPE_POL; continue; }
            if (k == '0') { g_news_filter = -1; continue; }
            if (k == TK_UP || k == TK_DOWN || k == TK_PGUP || k == TK_PGDN) continue;
            continue;
        }

        if (k == TK_UP)   { sel = (sel - 1 + STOCK_COUNT) % STOCK_COUNT; continue; }
        if (k == TK_DOWN) { sel = (sel + 1) % STOCK_COUNT; continue; }
        if (k == TK_PGUP) { sel = (sel - 5 + STOCK_COUNT) % STOCK_COUNT; continue; }
        if (k == TK_PGDN) { sel = (sel + 5) % STOCK_COUNT; continue; }
        if (k == TK_HOME) { sel = 0; continue; }
        if (k == TK_END)  { sel = STOCK_COUNT - 1; continue; }

        if (k == ' ') { live = !live; snprintf(msg, sizeof msg, "实时行情已%s。", live ? "开启" : "暂停"); msg_until = time(NULL) + 2; continue; }

        if (k == 'n') {
            view = 1;
            g_stock_view = 1;
            player.stock_news_unread = 0;
            continue;
        }

        if (k == 'r') {
            tui_stock_live_tick();
            {
                char pp[16];
                stk_price_str(pp, sizeof pp, player.stocks[sel].current_price);
                snprintf(msg, sizeof msg, "手动跳档：%s 现价 %s（第 %d 交易日 %d/%d）",
                         player.stocks[sel].name, pp,
                         player.stock_day, player.stock_tick, STOCK_TICKS_PER_DAY);
            }
            msg_until = time(NULL) + 2;
            continue;
        }

        if (k == 'c') {
            g_stock_focus = !g_stock_focus;
            snprintf(msg, sizeof msg, "图表%s（C 键切换）", g_stock_focus ? "已放大" : "已还原");
            msg_until = time(NULL) + 2;
            continue;
        }

        if (k == 'b' || k == 's') {
            int amount = 0;
            int margin = 0;
            char prompt[96];
            int inv_before = player.invested_stocks[sel];
            int unit_price = player.stocks[sel].current_price;

            if (player.stocks[sel].status == STK_DELISTED ||
                player.stocks[sel].status == STK_HALT) {
                snprintf(msg, sizeof msg, "%s 已停牌/退市，无法交易。", player.stocks[sel].name);
                msg_until = time(NULL) + 3;
                continue;
            }

            snprintf(prompt, sizeof prompt, "%s · %.2f 坤币/股 · 数量 ›",
                     k == 'b' ? "买入" : "卖出", player.stocks[sel].current_price / 100.0);

            stock_draw(sel, 0, 0, "", 0);
            tui_present();
            if (tui_input_int(tui_cv.h - 1, tui_cv.w, prompt, 1, 100000, &amount) < 0) {
                snprintf(msg, sizeof msg, "已取消。");
                msg_until = time(NULL) + 2;
                continue;
            }

            if (k == 'b') {
                /* 融资买入询问 */
                if (tui_input_int(tui_cv.h - 1, tui_cv.w,
                                  "融资买入？自付50%，券商垫50%（1=是 0=否）›", 0, 1, &margin) < 0)
                    margin = 0;
            }

            tui_stream_reset();
            if (k == 'b') buy_stock_ext(sel, amount, margin);
            else          sell_stock(sel, amount);
            tui_stream_reset();

            if (k == 'b') {
                if (player.invested_stocks[sel] > inv_before)
                    snprintf(msg, sizeof msg, "%s买入成功：%s × %d，成交额 %lld 坤币",
                             margin ? "融资" : "",
                             player.stocks[sel].name, amount,
                             (long long)amount * unit_price / 100);
                else
                    snprintf(msg, sizeof msg, "买入失败：坤币不足、涨停封板或数量无效。");
            } else {
                if (player.invested_stocks[sel] < inv_before)
                    snprintf(msg, sizeof msg, "卖出成功：%s × %d，成交额 %lld 坤币",
                             player.stocks[sel].name, amount,
                             (long long)amount * unit_price / 100);
                else
                    snprintf(msg, sizeof msg, "卖出失败：持仓不足或封跌停。");
            }
            msg_until = time(NULL) + 3;
            continue;
        }
    }

    tui_trend_save();
    tui_stream_reset();
}
 
 void show_properties() {
     CLEAR_SCREEN();
     draw_box("房产投资");
     
     printf("已拥有房产：%d/10\n\n", player.property_count);
     
     float daily_passive = 0;
     for (int i = 0; i < player.property_count; i++) {
         daily_passive += player.properties[i].passive_income;
     }
     printf("当前每日被动收入：%.0f 坤币\n\n", daily_passive);
     
     for (int i = 0; i < 10; i++) {
         Property* prop = &available_properties[i];
         
         if (prop->owned) {
             printf(COLOR_GREEN "%s\n" COLOR_RESET, prop->name);
             printf("   %s\n", prop->description);
             printf("   被动收入：%.0f 坤币/天\n", prop->passive_income);
         } else {
             printf(COLOR_WHITE "%s\n" COLOR_RESET, prop->name);
             printf("   %s\n", prop->description);
             printf("   价格：%lld 坤币\n", prop->coins_needed);
             printf("   建造时间：%d 天\n", prop->days_needed);
             printf("   预计收入：%.0f 坤币/天\n", prop->passive_income);
         }
         printf("\n");
     }
     
     draw_separator();
     printf("1. 购买房产   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 1);
     if (choice == 1) {
         printf("选择房产 (1-10): ");
         int prop_choice = get_valid_input(1, 10);
         buy_property(prop_choice - 1);
     }
     press_any_key();
 }
 
void show_prestige_menu() {
    CLEAR_SCREEN();
    draw_box("[声望系统]");
    
    printf("当前声望等级：%d\n", player.prestige_level);
    printf("声望点数：%d\n\n", player.prestige_points);
    
    if (player.prestige_level == 0) {
        printf("[首次声望重置奖励]：\n");
        printf("   - 所有基础收入增加20%%\n");
        printf("   - 解锁新成就\n");
        printf("   - 获得独特头衔\n");
        printf("   - 解锁高级功能\n\n");
        
        printf("重置条件：\n");
        printf("   - 达到等级50\n");
        printf("   - 拥有100万坤币\n");
        printf("   - 解锁20个成就\n");
        
        int unlocked_achievements = 0;
        for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
            if (player.achievements[i].unlocked) unlocked_achievements++;
        }
        
        bool can_prestige = (player.level >= 50 && player.kun_coins >= 1000000 && unlocked_achievements >= 20);
        
        if (can_prestige) {
            print_colored(COLOR_GREEN, "\n[满足声望重置条件]\n");
        } else {
            print_colored(COLOR_RED, "\n[不满足声望重置条件]\n");
        }
    } else {
        printf("声望等级 %d 奖励：\n", player.prestige_level);
        printf("   - 所有收入增加 %d%%\n", player.prestige_level * 5);
        printf("   - 技能升级成本减少 %d%%\n", player.prestige_level * 2);
        printf("   - 每日登录奖励增加 %d%%\n", player.prestige_level * 10);
        printf("\n");
        
        printf("下一等级需要：%d 声望点数\n", player.prestige_level * 1000);
        printf("当前拥有声望点数：%d\n", player.prestige_points);
    }
    
    draw_separator();
    if (player.prestige_level == 0) {
        printf("1. 重置声望（重新开始）   0. 返回\n");
        int choice = get_valid_input(0, 1);
        if (choice == 1) {
            prestige();
        }
    } else {
        printf("1. 重置声望   2. 兑换声望物品   0. 返回\n");
        int choice = get_valid_input(0, 2);
        if (choice == 1) {
            prestige();
        } else if (choice == 2) {
            CLEAR_SCREEN();
            draw_box("[声望商店]");
            printf("1. 永久会员折扣 - 500声望\n");
            printf("2. 开局资金翻倍 - 200声望\n");
            printf("3. 额外技能点+10 - 300声望\n");
            printf("0. 返回\n");
            int shop_choice = get_valid_input(0, 3);
            if (shop_choice == 1 && player.prestige_points >= 500) {
                player.prestige_points -= 500;
                if (!player.has_premium) {
                    buy_premium(36500);
                } else {
                    player.premium_until = time(NULL) + (long long)36500 * 86400LL;
                    print_colored(COLOR_GREEN, "[会员已延长至永久]\n");
                }
            } else if (shop_choice == 2 && player.prestige_points >= 200) {
                player.prestige_points -= 200;
                add_coins(500);
                print_colored(COLOR_GREEN, "[获得500坤币开局资金加成]\n");
            } else if (shop_choice == 3 && player.prestige_points >= 300) {
                player.prestige_points -= 300;
                player.skill_points += 10;
                print_colored(COLOR_GREEN, "[获得10技能点]\n");
            } else if (shop_choice > 0) {
                print_colored(COLOR_RED, "[声望点数不足]\n");
            }
        }
    }
    press_any_key();
}

// 医院治疗系统
void hospital_menu() {
    while (true) {
        CLEAR_SCREEN();
        draw_box("坤城人民医院");
        
        printf(COLOR_BOLD "  欢迎来到坤城人民医院！\n" COLOR_RESET);
        printf("  ─────────────────────────────────────\n");
        printf("  你的当前状态：\n");
        printf("  ?? 血量: %d/100\n", player.health);
        printf("  ?? 精神: %d/100\n", player.spirit);
        printf("  ?? 当前坤币: %lld\n", player.kun_coins);
        printf("  ─────────────────────────────────────\n\n");
        
        printf("  请选择治疗项目：\n\n");
        printf("  1. 基础包扎 — 50坤币（恢复30血量 + 10精神）\n");
        printf("  2. 标准诊疗 — 100坤币（恢复60血量 + 20精神）\n");
        printf("  3. 全面康复 — 200坤币（恢复100血量 + 40精神）\n");
        printf("  4. 心理疏导 — 80坤币（恢复50精神）\n");
        printf("  0. 离开医院\n\n");
        
        draw_separator();
        printf("请选择: ");
        int choice = get_valid_input(0, 4);
        
        if (choice == 0) {
            printf("\n  你离开了坤城人民医院。\n");
            press_any_key();
            break;
        }
        
        int cost = 0, heal_hp = 0, heal_spirit = 0;
        const char* treatment_name = "";
        
        switch (choice) {
            case 1:
                cost = 50; heal_hp = 30; heal_spirit = 10;
                treatment_name = "基础包扎";
                break;
            case 2:
                cost = 100; heal_hp = 60; heal_spirit = 20;
                treatment_name = "标准诊疗";
                break;
            case 3:
                cost = 200; heal_hp = 100; heal_spirit = 40;
                treatment_name = "全面康复";
                break;
            case 4:
                cost = 80; heal_hp = 0; heal_spirit = 50;
                treatment_name = "心理疏导";
                break;
        }
        
        if (player.kun_coins >= cost) {
            printf("\n  你接受了%s...\n", treatment_name);
            remove_coins(cost);
            
            if (heal_hp > 0) {
                player.health += heal_hp;
                if (player.health > 100) player.health = 100;
                printf("  血量 +%d（当前: %d/100）\n", heal_hp, player.health);
            }
            if (heal_spirit > 0) {
                player.spirit += heal_spirit;
                if (player.spirit > 100) player.spirit = 100;
                printf("  精神 +%d（当前: %d/100）\n", heal_spirit, player.spirit);
            }
            printf("  ?? 花费了 %d 坤币\n", cost);
        } else {
            print_colored(COLOR_RED, "\n  ? 坤币不够！治疗需要 %d 坤币，你只有 %lld 坤币。\n", cost, player.kun_coins);
        }
        press_any_key();
    }
}

/*================ v5.0 TUI 主界面 ================*/
static const char* MENU_LABEL[23] = {
    "查看状态","打工赚钱","商城购物","技能系统","背包物品","任务列表",
    "宠物系统","公司管理","股票市场","房产投资","成就系统","声望系统",
    "好友系统","游戏统计","会员商城","游戏设置","帮助指南","保存游戏",
    "手机","福布斯榜","睡觉休息","医院治疗","退出游戏"
};
static const char* MENU_HINT[23] = {
    "属性、技能、资产一览",
    "接活赚钱 —— 今天也要活下去",
    "买家具换被动收入，买吃的填饱肚子",
    "技能升级，长期收益在这里",
    "看看你都攒了些什么",
    "坤城给的任务，做完有奖励",
    "宠物会饿，也会给你加成",
    "从打工仔到老板",
    "25 支股票，实时行情和折线图",
    "买房收租，稳一点",
    "30 个成就，慢慢刷",
    "声望越高，处处有折扣",
    "这个城市里认识的人",
    "数据不会说谎",
    "充钱才能变强（大概）",
    "音量、显示、存档",
    "不知道该干嘛就看这里",
    "手动存一次，保险",
    "手机里的聊天室和二手市场",
    "坤城富豪榜",
    "提前结束今天，回血",
    "血没了就去这里",
    "离开坤城"
};
static const int MENU_CODE[23] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,0};
static int g_menu_choice = 0;

static const char* TUI_TIPS[] = {
    "在坤城，房租不会等你。",
    "股票能让你富，也能让你睡桥洞。",
    "精神低于 20 会开始出现幻觉——那不是坤城在骗你。",
    "加班模式收入高，但猝死不是玩笑。",
    "老城区的盒饭比 CBD 便宜，味道也好。",
    "声望高了，很多地方会给你打折。",
    "每天 3 个时间槽，用完了就只能睡觉。",
    "医院能救命，但账单也能要命。",
    "宠物饿久了会掉心情，别只顾着自己。",
    "聊天室里有人给你活，也有人骗你。",
    "地铁末班车 23:10，赶不上的话打车很贵。",
    "坤城不问你的过去，只问你的价格。"
};
#define TUI_TIPS_N ((int)(sizeof(TUI_TIPS)/sizeof(TUI_TIPS[0])))

/* 去掉新闻标题开头的【xx】类型标签，首页用色块代替 */
static const char* news_strip_tag(const char* hd)
{
    const unsigned char* p = (const unsigned char*)hd;
    if (!hd) return "";
    if (p[0] == 0xE3 && p[1] == 0x80 && p[2] == 0x90) {   /* 【 */
        int i = 3;
        while (p[i]) {
            if (p[i] == 0xE3 && p[i + 1] == 0x80 && p[i + 2] == 0x91)   /* 】 */
                return hd + i + 3;
            i++;
        }
    }
    return hd;
}

/* 首页速报：警告 → 坤透社快讯（主力）→ 今日目标 → 贴士 */
static void tui_draw_report(int x, int y, int w, int h)
{
    int line = 0;
    int warn = 0;
    int news_rows;

    if (h < 1 || w < 10) return;

    /* ---- 警告区：最多两条，别把新闻挤没了 ---- */
    if (player.health <= 0 && line < h) {
        tc_puts(x, y + line, "你已经昏迷了 —— 马上去医院（22）", TC_RED, TC_BG2);
        line++; warn = 1;
    } else if (player.health < 30 && line < h) {
        tc_puts(x, y + line, "身受重伤，血量低于 30，别再硬撑", TC_RED, TC_BG2);
        line++; warn = 1;
    }
    if (player.hunger < 20 && line < h && warn < 2) {
        tc_puts(x, y + line, "饿得发抖 —— 饱食度不足 20，去买点吃的", TC_RED, TC_BG2);
        line++; warn = 1;
    }
    if (player.spirit < 20 && line < h && warn < 2) {
        tc_puts(x, y + line, "精神濒临崩溃，再熬下去会出事", TC_ORANGE, TC_BG2);
        line++; warn = 1;
    }

    /* ---- 坤透社快讯：铺满剩余空间 ---- */
    news_rows = h - line - 2;              /* 底部留 2 行给目标 + 贴士 */
    if (news_rows > 9) news_rows = 9;
    if (news_rows > 0 && g_news_n > 0) {
        int k;
        if (line < h) {                    /* 小标题 */
            tc_puts(x, y + line, "坤透社快讯", TC_ORANGE, TC_BG2);
            if (w > 30) tc_puts(x + w - 9, y + line, "[N] 全部", TC_FAINT, TC_BG2);
            line++;
            news_rows--;
        }
        for (k = 0; k < news_rows && line < h; k++) {
            NewsItem* it = news_recent(k);
            const char* tag;
            int tc;
            if (!it) break;
            tag = (it->type == NEWS_TYPE_ENT) ? "娱" :
                  (it->type == NEWS_TYPE_ECO) ? "经" : "政";
            tc = (it->type == NEWS_TYPE_ENT) ? TC_PINK :
                 (it->type == NEWS_TYPE_ECO ? TC_YELLOW : TC_CYAN);
            tc_puts(x, y + line, tag, tc, TC_BG2);
            tc_putsn(x + 3, y + line, w - 3, news_strip_tag(it->head), TC_DIM, TC_BG2);
            line++;
        }
    } else if (!warn && line < h) {
        tc_puts(x, y + line, "市场安静得不像坤城，还没有新闻。", TC_FAINT, TC_BG2);
        line++;
    }

    /* ---- 今日目标：压成一行 ---- */
    if (line < h) {
        char gb[192];
        int d0 = player.today_work_count > 0;
        int d1 = player.today_income >= 200;
        int d2 = g_trend_n > 0;
        snprintf(gb, sizeof gb, "今日目标 %d/3   %s打工  %s收入200  %s行情",
                 d0 + d1 + d2, d0 ? "[x]" : "[ ]", d1 ? "[x]" : "[ ]", d2 ? "[x]" : "[ ]");
        tc_putsn(x, y + line, w, gb, (d0 + d1 + d2) >= 3 ? TC_GREEN : TC_DIM, TC_BG2);
        line++;
    }

    /* ---- 贴士 ---- */
    if (line < h) {
        int idx = (player.day_count * 7 + line) % TUI_TIPS_N;
        tc_put(x, y + line, ".", TC_FAINT, TC_BG2);
        tc_putsn(x + 2, y + line, w - 2, TUI_TIPS[idx], TC_DIM, TC_BG2);
        line++;
    }
}

/* 全屏新闻页（主菜单按 N 进入） */
static void tui_news_page(void)
{
    time_t tick = time(NULL);

    if (g_news_n == 0) news_daily_refresh();

    for (;;) {
        int k;
        tui_begin();
        tui_draw_topbar();
        stock_draw_news(0, 1, tui_cv.w, tui_cv.h - 3, g_news_filter);
        tui_draw_keybar(" [1] 娱乐   [2] 经济   [3] 政治   [0] 全部   [Esc] 返回 ");
        tui_present();

        for (;;) {
            k = tui_key_nb();
            if (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q') { g_news_filter = -1; return; }
                if (k == '1') { g_news_filter = NEWS_TYPE_ENT; break; }
                if (k == '2') { g_news_filter = NEWS_TYPE_ECO; break; }
                if (k == '3') { g_news_filter = NEWS_TYPE_POL; break; }
                if (k == '0') { g_news_filter = -1; break; }
            }
            tui_sleep_ms(25);
            if (time(NULL) != tick) { tick = time(NULL); break; }
        }
    }
}

static void tui_draw_main_menu(int sel)
{
    int i;
    int cy = 1;
    int cx = SB_W + 1;
    int cw;
    int top_h;
    int mh;

    tui_begin();
    cw = tui_cv.w - cx - 1;
    top_h = (tui_cv.h >= 30) ? 5 : 0;
    tui_draw_topbar();
    tui_draw_sidebar(1, tui_cv.h - 3);
    if (cw < 34) { cx = 0; cw = tui_cv.w; }

    /* ===== 今日概况 ===== */
    if (top_h > 0) {
        char b1[128], b2[128];
        int ix = cx + 2, iy = cy + 1;

        tc_panel(cx, cy, cw, top_h, "今日", TC_BORDER, TC_BG2, TC_CYAN);

        tc_puts(ix, iy, "时间槽", TC_DIM, TC_BG2);
        {
            const char* sn[3] = { "上", "下", "晚" };
            for (i = 0; i < player.max_time_slots && i < 3; i++)
                tc_put(ix + 7 + i * 3, iy, sn[i],
                       i < player.daily_time_slots ? TC_GREEN : TC_FAINT, TC_BG2);
        }
        tc_puts(ix + 7 + player.max_time_slots * 3 + 1, iy,
                player.daily_time_slots > 0 ? "还能干活" : "今天干不动了",
                player.daily_time_slots > 0 ? TC_GREEN : TC_RED, TC_BG2);

        if (cw > 52) {
            snprintf(b1, sizeof b1, "收入 +%d", player.today_income);
            tc_puts(ix + 28, iy, b1, TC_GREEN, TC_BG2);
            snprintf(b2, sizeof b2, "支出 -%d", player.today_expenses);
            tc_puts(ix + 40, iy, b2, TC_RED, TC_BG2);
        }
        iy++;

        if (g_trend_n > 1 && cw > 40) {
            int idx[TREND_MAX], k, delta;
            for (i = 0; i < g_trend_n; i++) {
                int s = 0, n = 0;
                for (k = 0; k < 25; k++) {
                    if (player.stocks[k].status == STK_DELISTED) continue;
                    s += g_trend[k][i];
                    n++;
                }
                idx[i] = n > 0 ? s * 10 / (n * 100) : 0;
            }
            tc_puts(ix, iy, "坤城指数", TC_DIM, TC_BG2);
            snprintf(b1, sizeof b1, "%d", idx[g_trend_n - 1]);
            tc_puts(ix + 9, iy, b1, TC_YELLOW, TC_BG2);
            delta = idx[g_trend_n - 1] - idx[0];
            snprintf(b2, sizeof b2, "%+d", delta);
            tc_puts(ix + 17, iy, b2, delta >= 0 ? TC_RED : TC_GREEN, TC_BG2);
            tc_spark(ix + 25, iy, cw - 29, idx, g_trend_n, idx[0], TC_RED, TC_GREEN, TC_BG2);
            iy++;
        }

        if (player.active_event != EVENT_NONE) {
            tc_puts(ix, iy, "进行中", TC_DIM, TC_BG2);
            tc_puts(ix + 7, iy, get_event_name(player.active_event), TC_ORANGE, TC_BG2);
        } else {
            tc_puts(ix, iy, "坤城今天还算平静。", TC_FAINT, TC_BG2);
        }
        cy += top_h;
    }

    /* ===== 主菜单网格 ===== */
    {
    int avail = tui_cv.h - 3 - cy;
    int report_h;
    mh = 12;
    if (mh > avail - 5) mh = avail;
    if (mh < 6) mh = 6;
    tc_panel(cx, cy, cw, mh, "主菜单", TC_BORDER, TC_BG2, TC_YELLOW);
    {
        int cols = 3;
        int colw = (cw - 4) / cols;
        if (colw > 27) colw = 27;
        for (i = 0; i < 23; i++) {
            int r = i / cols, c = i % cols;
            int ix = cx + 2 + c * colw;
            int iy = cy + 1 + r;
            int on = (i == sel);
            char num[8];
            if (iy >= cy + mh - 3) break;
            tc_fill(ix - 1, iy, colw - 1, 1, " ", on ? TC_WHITE : TC_TEXT, on ? TC_BGHI : TC_BG2);
            tc_put(ix, iy, on ? ">" : " ", on ? TC_YELLOW : TC_DIM, on ? TC_BGHI : TC_BG2);
            snprintf(num, sizeof num, "%2d", MENU_CODE[i]);
            tc_puts(ix + 2, iy, num, on ? TC_YELLOW : TC_FAINT, on ? TC_BGHI : TC_BG2);
            tc_puts(ix + 5, iy, MENU_LABEL[i], on ? TC_WHITE : TC_DIM, on ? TC_BGHI : TC_BG2);
        }
        {
            int hy = cy + mh - 2;
            if (hy > cy + 2) {
                tc_hline(cx + 2, hy - 1, cw - 5, TC_BG3, TC_BG2);
                tc_put(cx + 2, hy, ">", TC_CYAN, TC_BG2);
                tc_putsn(cx + 4, hy, cw - 6, MENU_HINT[sel], TC_CYAN, TC_BG2);
            }
        }
    }

    /* ===== 坤城速报 ===== */
    report_h = avail - mh;
    if (report_h > 14) report_h = 14;
    if (report_h >= 5) {
        int ry = cy + mh;
        tc_panel(cx, ry, cw, report_h, "坤城速报", TC_BORDER, TC_BG2, TC_ORANGE);
        tui_draw_report(cx + 2, ry + 1, cw - 4, report_h - 2);
    }
    }

    tui_draw_keybar(" [方向键] 选择   [回车] 进入   [1-9/0] 快选   [N] 坤城新闻   [Q] 退出 ");
}

static int tui_run_main_menu(void)
{
    static int sel = 0;
    time_t tick = time(NULL);

    for (;;) {
        int k;
        tui_draw_main_menu(sel);
        tui_present();

        for (;;) {
            k = tui_key_nb();
            if (k >= 0) {
                if (k == TK_UP)    { sel = (sel - 3 + 23) % 23; break; }
                if (k == TK_DOWN)  { sel = (sel + 3) % 23; break; }
                if (k == TK_LEFT)  { sel = (sel - 1 + 23) % 23; break; }
                if (k == TK_RIGHT) { sel = (sel + 1) % 23; break; }
                if (k == TK_HOME)  { sel = 0; break; }
                if (k == TK_END)   { sel = 22; break; }
                if (k == 13)       { return MENU_CODE[sel]; }
                if (k == 'n' || k == 'N') { tui_news_page(); break; }
                if (k == 'q' || k == 27) return 0;
                if (k >= '0' && k <= '9') {
                    int want = (k == '0') ? 0 : (k - '0');
                    int j;
                    for (j = 0; j < 23; j++) if (MENU_CODE[j] == want) { return want; }
                }
            }
            tui_sleep_ms(25);
            if (time(NULL) != tick) { tick = time(NULL); break; }
        }
    }
}

void show_main_menu() {
    g_menu_choice = tui_run_main_menu();
}

/* 自检：渲染几帧到 stdout 后退出（环境变量 TUI_SELFTEST=1 触发，不影响正常游玩） */
static void tui_selftest(void)
{
    const char* f = getenv("TUI_FRAME");
    int frame = f ? atoi(f) : 1;
    int i;

    tui_lock_size = 1;
    tui_resize_canvas(120, 40);

    /* 统一造一份有内容的存档，方便看排版 */
    player.day_count = 12;
    player.level = 7;
    player.kun_coins = 48250;
    player.kun_exp = 320;
    player.daily_time_slots = 2;
    player.today_income = 860;
    player.today_expenses = 240;
    player.today_work_count = 1;
    player.hunger = 46;
    player.health = 78;
    player.spirit = 32;
    player.prestige_level = 2;
    player.skill_points = 5;
    player.invested_stocks[8] = 200;
    if (g_news_n == 0) { int q; for (q = 0; q < 7; q++) news_generate_one(); }

    tui_trend_load();
    for (i = 0; i < 40; i++) tui_stock_live_tick();

    if (frame == 1) {
        tui_draw_main_menu(0);
    } else if (frame == 2) {
        stock_draw(8, 1, 1, "");
    } else if (frame == 5) {
        g_stock_focus = 1;
        stock_draw(8, 1, 1, "");
    } else if (frame == 6) {
        stock_draw(8, 1, 1, "", 1);
    } else if (frame == 7) {
        g_news_filter = NEWS_TYPE_ECO;
        stock_draw(8, 1, 1, "", 1);
    } else if (frame == 8) {
        tui_begin();
        tui_draw_topbar();
        stock_draw_news(0, 1, tui_cv.w, tui_cv.h - 3, -1);
        tui_draw_keybar(" [1] 娱乐   [2] 经济   [3] 政治   [0] 全部   [Esc] 返回 ");
    } else if (frame == 3) {
        tui_stream_reset();
        tui_stream_set_title("技能系统");
        printf("可用技能点：3\n\n");
        printf(COLOR_YELLOW "速度 Lv.3" COLOR_RESET "   加成 12%%\n");
        printf("  提升移动与反应速度，打工时更容易抢到活。\n");
        print_progress_bar(3, 20, 26);
        printf("\n" COLOR_CYAN "力量 Lv.1" COLOR_RESET "   加成 4%%\n");
        print_progress_bar(1, 20, 26);
        printf("\n" COLOR_MAGENTA "智力 Lv.5" COLOR_RESET "   加成 21%%\n");
        print_progress_bar(5, 20, 26);
        printf("\n");
        draw_separator();
        printf("1.速度 2.力量 3.智力 4.幸运 5.魅力\n");
        printf("6.耐力 7.创造力 8.领导力 9.谈判 10.管理\n");
        printf("0.返回\n");
        tui_draw_page(NULL);
    } else if (frame == 4) {
        init_chat_room();
        player.chat_message_count = 0;
        add_chat_message("系统", "欢迎来到坤城聊天室！请遵守社区规则。", CHATTER_NORMAL);
        add_chat_message("老徐", "今天老城区那边又堵了，我走了四十分钟", CHATTER_NORMAL);
        add_chat_message("你", "@老徐 晚上一起吃个饭？", CHATTER_NORMAL);
        add_chat_message("老徐", "行啊，老地方，六点半", CHATTER_NORMAL);
        add_chat_message("卖药的", "有需要 OD 的私聊我，价格好说", CHATTER_BLACKMARKET);
        add_chat_message("同城小哥", "周末有人一起去爬山吗，人少点好", CHATTER_TONGCHENG);
        add_chat_message("骗子张", "兄弟，我这儿有个稳赚不赔的项目，了解一下？", CHATTER_SCAMMER);
        add_chat_message("你", "算了，我这月房租还没着落", CHATTER_NORMAL);
        add_chat_message("阿May", "都一样，坤城这地方就是这样", CHATTER_NORMAL);
        add_chat_message("老徐", "熬吧，熬过去就好了", CHATTER_NORMAL);
        chat_draw(-1);
    } else {
        tui_draw_main_menu(8);
    }

    tui_dirty_all = 1;
    tui_present();
    tui_sleep_ms(60);
}
 
 //================ 反调试/反分析保护 ================
 #ifdef _WIN32
 void anti_analysis_check() {
     // 检测1: IsDebuggerPresent - 检测调试器附加
     if (IsDebuggerPresent()) {
         MessageBoxA(NULL, "Debugger Detected!", "Security", MB_ICONERROR);
         ExitProcess(0xDEAD);
     }
     
     // 检测2: NtGlobalFlag - 调试标志位
     typedef LONG (WINAPI *NtQueryInformationProcess_t)(HANDLE, LONG, PVOID, ULONG, PULONG);
     HMODULE ntdll = GetModuleHandleA("ntdll.dll");
     if (ntdll) {
         NtQueryInformationProcess_t NtQueryInfo = 
             (NtQueryInformationProcess_t)GetProcAddress(ntdll, "NtQueryInformationProcess");
         if (NtQueryInfo) {
             DWORD debugPort = 0;
             NtQueryInfo(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), NULL);
             if (debugPort != 0) {
                 ExitProcess(0xBEEF);
             }
         }
     }
     
     // 检测3: PEB->BeingDebugged (内联汇编)
     // 32位: fs:[0x30]+2 | 64位: gs:[0x60]+2
     {
         BOOL debugged = FALSE;
#ifdef __x86_64__
         asm volatile (
             "movq %%gs:0x60, %%rax\n\t"
             "movzbl 2(%%rax), %%eax\n\t"
             : "=a"(debugged)
         );
#else
         asm volatile (
             "movl %%fs:0x30, %%eax\n\t"
             "movzbl 2(%%eax), %%eax\n\t"
             : "=a"(debugged)
         );
#endif
         if (debugged) ExitProcess(0xCAFE);
     }
     
     // 检测4: 检查常见分析工具窗口
     const char* tools[] = {
         "HxD", "Cheat Engine", "x64dbg", "OllyDbg", "IDA",
         "Process Hacker", "Process Monitor", "WinDbg", "Scylla",
         "detect it easy", "CFF Explorer", "PE-bear", "Resource Hacker"
     };
     for (int i = 0; i < (int)(sizeof(tools)/sizeof(tools[0])); i++) {
         HWND hwnd = NULL;
         while (1) {
             hwnd = FindWindowExA(NULL, hwnd, NULL, NULL);
             if (!hwnd) break;
             char title[256];
             GetWindowTextA(hwnd, title, sizeof(title));
             if (strstr(title, tools[i])) {
                 // 只退出，不弹窗（低调处理）
                 ExitProcess(0x1337);
             }
         }
     }
     
     // 检测5: 代码段完整性校验 (简单的 checksum)
     // 防止二进制被修改
     HMODULE hMod = GetModuleHandleA(NULL);
     PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hMod;
     PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dosHeader->e_lfanew);
     PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
     
     DWORD checksum = 0;
     for (int s = 0; s < ntHeaders->FileHeader.NumberOfSections; s++) {
         if (strcmp((char*)section[s].Name, ".text") == 0) {
             BYTE* code = (BYTE*)hMod + section[s].VirtualAddress;
             for (DWORD j = 0; j < section[s].Misc.VirtualSize; j++) {
                 checksum += code[j] * (j + 1);
             }
             break;
         }
     }
     
     // checksum 硬编码（编译器会在编译时计算）
     // 开发注意：首次编译后需要更新这个值
     if (checksum != 0 && checksum != 0xDEADC0DE) {
         // 如果 checksum 不匹配（0xDEADC0DE 是占位符，编译后需要替换）
         // 暂时跳过，等待实际编译时更新
     }
 }
 #endif

 //================ 主函数 ================
 int main() {
     enable_ansi();
#ifdef _WIN32
     anti_analysis_check();
 #endif
     g_tui_selftest = getenv("TUI_SELFTEST") ? 1 : 0;
     tui_autokeys = getenv("TUI_AUTOKEYS");
     tui_lock_size = getenv("TUI_LOCK") ? 1 : 0;
     tui_init();
     if (tui_lock_size) tui_resize_canvas(120, 40);
     show_loading_screen(g_tui_selftest ? 60 : 1400);
     
     init_game();
     tui_trend_load();

     if (getenv("TUI_SELFTEST")) { tui_selftest(); tui_shutdown(); return 0; }
     
     time_t last_passive_time = time(NULL);
     time_t last_save_time = time(NULL);
     time_t last_health_time = time(NULL);
     
     while (1) {
         show_main_menu();
         int choice = g_menu_choice;
         
         time_t now = time(NULL);
         if (now - last_passive_time >= 3600) {
             calculate_passive_income();
             last_passive_time = now;
         }
         if (now - last_save_time >= 300) {
             save_game();
             last_save_time = now;
         }
         if (now - last_health_time >= 3600) {
             recover_health();
             last_health_time = now;
         }
         
         recover_energy();
              process_spirit_effects();
        check_hallucination();
        
        // 血量≤0时强制送医
        if (player.health <= 0 && choice != 22) {
            CLEAR_SCREEN();
            print_colored(COLOR_RED, "\n  ??? 你因失血过多而昏倒了！\n");
            print_colored(COLOR_RED, "  ??? 路人把你送到了坤城人民医院...\n\n");
            press_any_key();
            hospital_menu();
            continue; // 回主菜单重新显示
        }
        
        tui_stream_reset();
        switch (choice) {
             case 1:
                 show_status();
                 press_any_key();
                 break;
             case 2: {
                 CLEAR_SCREEN();
                 draw_box("任务中心");
                 
                 printf("可用任务：\n\n");
                 for (int i = 0; i < MAX_TASKS; i++) {
                     Task* task = &tasks[i];
                     
                     if (task->is_unlocked) {
                         if (player.is_hallucinating && (rand() % 3 == 0)) {
                             printf("%d. ????????\n", i + 1);
                         } else {
                            printf("%d. %s\n", i + 1, task->name);
                            printf("   收入：%d 坤币\n", task->base_income);
                            printf("   经验：%d\n", task->experience_gain);
                            printf("   时间：1槽\n");
                            printf("   成功率：%.0f%%\n", task->success_rate * 100);
                            printf("   描述：%s\n", task->description);
                             if (task->completion_count > 0) {
                                 printf("   完成次数：%d\n", task->completion_count);
                             }
                         }
                         printf("\n");
                     } else {
                         printf("%d. ?? %s\n", i + 1, task->name);
                         printf("   解锁需要：%d 坤币\n", task->unlock_cost);
                         printf("   等级要求：%d\n", task->required_level);
                         printf("   描述：%s\n", task->description);
                         printf("\n");
                     }
                 }
                 
                 draw_separator();
                 printf("选择任务（0返回）: ");
                 int task_choice = get_valid_input(0, MAX_TASKS);
                 if (task_choice > 0) {
                     do_task(task_choice - 1);
                 }
                 press_any_key();
                 break;
             }
            case 3: {
                // 商城主菜单 - 分为家具商店和食物商店
                while (true) {
                    CLEAR_SCREEN();
                    draw_box("商城");
                    
                    printf("请选择商店类型：\n");
                    printf("1. 家具商店（购买家具，获得被动收入）\n");
                    printf("2. 食物商店（购买食物，恢复饱食度）\n");
                    printf("3. 返回主菜单\n");
                    draw_separator();
                    
                    int shop_type = get_valid_input(1, 3);
                    if (shop_type == 3) break;
                    
                    if (shop_type == 1) {
                        // 家具商店
                        while (true) {
                            CLEAR_SCREEN();
                            draw_box("家具商店");
                            
                            printf("可用家具：\n\n");
                            for (int i = 0; i < MAX_MARKET; i++) {
                                MarketItem* item = &market[i];
                                
                                printf("%d. %s\n", i + 1, item->name);
                                
                                switch (item->rarity) {
                                    case RARITY_LEGENDARY:
                                        print_colored(COLOR_YELLOW, "   传说 ");
                                        break;
                                    case RARITY_EPIC:
                                        print_colored(COLOR_MAGENTA, "   史诗 ");
                                        break;
                                    case RARITY_RARE:
                                        print_colored(COLOR_BLUE, "   稀有 ");
                                        break;
                                    case RARITY_UNCOMMON:
                                        print_colored(COLOR_GREEN, "   优秀 ");
                                        break;
                                    default:
                                        printf("   普通 ");
                                        break;
                                }
                                
                                printf("价格：%d 坤币\n", item->price);
                                printf("   等级要求：%d\n", item->required_level);
                                printf("   被动收入：%.1f 坤币/小时\n", item->passive_income);
                                printf("   描述：%s\n", item->description);
                                printf("\n");
                            }
                            
                            draw_separator();
                            printf("选择商品（0返回）: ");
                            int item_choice = get_valid_input(0, MAX_MARKET);
                            if (item_choice == 0) break;
                            buy_item(item_choice - 1);
                            press_any_key();
                        }
                    } else if (shop_type == 2) {
                        // 食物商店 - 从手机菜单迁移过来
                        while (true) {
                            CLEAR_SCREEN();
                            draw_box("食物商店");
                            
                            printf("可购买的食物：\n");
                            printf("1. 面包 - 10坤币（恢复20饱食度）\n");
                            printf("2. 方便面 - 15坤币（恢复30饱食度）\n");
                            printf("3. 盒饭 - 25坤币（恢复50饱食度）\n");
                            printf("4. 豪华套餐 - 50坤币（恢复100饱食度）\n");
                            printf("0. 返回\n");
                            draw_separator();
                            
                            int choice = get_valid_input(0, 4);
                            
                            int cost = 0, food_value = 0;
                            const char* food_name = "";
                            
                            switch (choice) {
                                case 1:
                                    cost = 10;
                                    food_value = 20;
                                    food_name = "面包";
                                    break;
                                case 2:
                                    cost = 15;
                                    food_value = 30;
                                    food_name = "方便面";
                                    break;
                                case 3:
                                    cost = 25;
                                    food_value = 50;
                                    food_name = "盒饭";
                                    break;
                                case 4:
                                    cost = 50;
                                    food_value = 100;
                                    food_name = "豪华套餐";
                                    break;
                                case 0:
                                    break;
                                default:
                                    continue;
                            }
                            
                            if (choice == 0) break;
                            
                            if (choice > 0) {
                                if (remove_coins(cost)) {
                                    player.hunger += food_value;
                                    if (player.hunger > 100) player.hunger = 100;
                                    print_colored(COLOR_GREEN, "购买了%s，饱食度+%d\n", food_name, food_value);
                                } else {
                                    print_colored(COLOR_RED, "坤币不足！\n");
                                }
                                press_any_key();
                            }
                        }
                    }
                }
                break;
            }
             case 4:
                 show_skills();
                 break;
             case 5:
                 show_inventory();
                 break;
             case 6:
                 show_quests();
                 break;
             case 7:
                 show_pets();
                 break;
             case 8:
                 show_company();
                 break;
             case 9:
                 show_stock_market();
                 break;
             case 10:
                 show_properties();
                 break;
             case 11:
                 show_achievements();
                 press_any_key();
                 break;
             case 12:
                 show_prestige_menu();
                 break;
             case 13:
                 show_friends();
                 break;
             case 14:
                 show_statistics();
                 break;
             case 15:
                 show_premium_shop();
                 break;
             case 16:
                 show_settings();
                 break;
             case 17:
                 show_help();
                 press_any_key();
                 break;
             case 18:
                 save_game();
                 press_any_key();
                 break;
             case 19:
                 show_phone_menu();
                 break;
            case 20:
                show_forbes_ranking_menu();
                break;
            case 21:
                // 睡觉休息 - 跳过这一天
                CLEAR_SCREEN();
                draw_box("睡觉休息");
                printf("  你决定提前结束今天的工作，好好休息一觉...\n\n");
                press_any_key();
                day_end();
                press_any_key();
                break;
            case 22:
                hospital_menu();
                break;
            case 0: {
                 int i;
                 save_game();
                 tui_trend_save();
                 for (i = 0; i < 3; i++) {
                     tui_begin();
                     tc_puts_center(0, tui_cv.h / 2 - 2, tui_cv.w, "坤城会记得每一个认真生活的人。", TC_YELLOW, TC_BG);
                     tc_puts_center(0, tui_cv.h / 2,     tui_cv.w, "感谢游玩 · 打工模拟器 v5.0", TC_WHITE, TC_BG);
                     tc_puts_center(0, tui_cv.h / 2 + 2, tui_cv.w, "存档已保存", TC_DIM, TC_BG);
                     tui_present();
                     tui_sleep_ms(420);
                     tui_begin();
                     tui_present();
                     tui_sleep_ms(160);
                 }
                 tui_begin();
                 tc_puts_center(0, tui_cv.h / 2 - 2, tui_cv.w, "坤城会记得每一个认真生活的人。", TC_YELLOW, TC_BG);
                 tc_puts_center(0, tui_cv.h / 2,     tui_cv.w, "感谢游玩 · 打工模拟器 v5.0", TC_WHITE, TC_BG);
                 tc_puts_center(0, tui_cv.h / 2 + 2, tui_cv.w, "存档已保存", TC_DIM, TC_BG);
                 tui_present();
                 tui_sleep_ms(900);
                 tui_shutdown();
                 return 0;
            }
         }
         
         random_event();
         
         static time_t last_update = 0;
         if (last_update == 0) last_update = now;
         player.total_play_time += (int)(now - last_update);
         last_update = now;
     }
     
     return 0;
 } // 结束了喵~
