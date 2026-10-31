/*
 * 打工模拟器 v4.1 - 坤城篇 (Web版)
 * 世界：坤城 —— 一座虚构的当代中国沿海城市，300万人口的欲望都市
 * 叙事主题："在一个人人都想往上爬的城市里，你要成为什么样的人？"
 * 编译环境: Emscripten / Windows / Linux, C11/C++兼容
 * 编译命令: emcc -o 打工模拟器.html work_web.cpp -lm -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY -O2
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

#ifdef __EMSCRIPTEN__
#include "emscripten_compat.h"
#include <unistd.h>
#elif defined(_WIN32)
#include <conio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <direct.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#define _getch() getchar()
#endif
 
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
#define SAVE_VERSION 8  // v4.1
 
 // 货币限制常量
 #define MAX_KUN_COINS 9223372036854775807LL
 
 // 聊天室常量
 #define MAX_CHATTERS 8
 #define MAX_MESSAGES 50
 #define CHAT_HISTORY_FILE "chat_history.txt"
 
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
 
 #define CLEAR_SCREEN() printf("\033[2J\033[H")
 
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
 #define FORBES_FILE "forbes.list"
 
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
 
 typedef struct {
     char name[50];
     int base_price;
     int current_price;
     int price_change;
     int volatility;
     time_t last_update;
 } Stock;
 
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
     Stock stocks[5];
     int invested_stocks[5];
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
     va_list args;
     va_start(args, format);
     printf("%s", color);
     vprintf(format, args);
     printf("%s", COLOR_RESET);
     va_end(args);
 }
 
 void print_bold(const char* format, ...) {
     va_list args;
     va_start(args, format);
     printf("%s", COLOR_BOLD);
     vprintf(format, args);
     printf("%s", COLOR_RESET);
     va_end(args);
 }
 
 void print_centered(const char* text) {
     int width = 80;
     int padding = (width - (int)strlen(text)) / 2;
     if (padding < 0) padding = 0;
     printf("%*s%s\n", padding, "", text);
 }
 
 void draw_separator() {
     printf(COLOR_CYAN "═══════════════════════════════════════════════════════════════════════\n" COLOR_RESET);
 }
 
 void draw_double_separator() {
     printf(COLOR_YELLOW "═══════════════════════════════════════════════════════════════════════\n" COLOR_RESET);
 }
 
 void draw_box(const char* title) {
     printf("\n");
     draw_double_separator();
     print_centered(title);
     draw_double_separator();
     printf("\n");
 }
 
 int get_valid_input(int min, int max) {
     int input;
     char buffer[100];
     while (1) {
         printf("请输入选项 (%d-%d): ", min, max);
         if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
#ifdef __EMSCRIPTEN__
             clearerr(stdin);
             emscripten_sleep(100);
#endif
             continue;
         }
         if (sscanf(buffer, "%d", &input) != 1) {
             printf("输入无效，请输入数字！\n");
             continue;
         }
         if (input < min || input > max) {
             printf("请输入%d到%d之间的数字！\n", min, max);
             continue;
         }
         return input;
     }
 }

 int get_valid_input_with_cancel(int min, int max, int cancel_code) {
     int input;
     char buffer[100];
     while (1) {
         printf("请输入选项 (%d-%d, %d取消): ", min, max, cancel_code);
         if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
#ifdef __EMSCRIPTEN__
             clearerr(stdin);
             emscripten_sleep(100);
#endif
             continue;
         }
         if (sscanf(buffer, "%d", &input) != 1) {
             printf("输入无效，请输入数字！\n");
             continue;
         }
         if (input == cancel_code) {
             return cancel_code;
         }
         if (input < min || input > max) {
             printf("请输入%d到%d之间的数字！\n", min, max);
             continue;
         }
         return input;
     }
 }
 
 void press_any_key() {
     printf("\n" COLOR_GREEN "按任意键继续..." COLOR_RESET);
#ifdef __EMSCRIPTEN__
     getchar();  // Emscripten: need Enter
#else
     _getch();
#endif
     printf("\n");
 }
 
 void show_skills() {
     CLEAR_SCREEN();
     draw_box("?? 技能系统");
     
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
#ifdef __EMSCRIPTEN__
        getchar();
#else
         system("pause");
#endif
     }
 }
 
 void show_loading_screen(int duration) {
     CLEAR_SCREEN();
     print_centered("打工模拟器 v4.1 - 坤城篇 (Web版)");
     printf("\n\n");
     print_centered("加载中...");
     printf("\n    ");
     
     for (int i = 0; i < 50; i++) {
         printf(COLOR_GREEN "█" COLOR_RESET);
         fflush(stdout);
#ifdef __EMSCRIPTEN__
        emscripten_sleep(duration / 50);
#elif defined(_WIN32)
         Sleep(duration / 50);
#else
         usleep((duration * 1000) / 50);
#endif
     }
     printf("\n\n");
 }
 
 void print_progress_bar(int current, int max, int width) {
     int filled = (int)((float)current / max * width);
     printf("[");
     for (int i = 0; i < width; i++) {
         if (i < filled) {
             printf(COLOR_GREEN "█" COLOR_RESET);
         } else {
             printf("?");
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
             print_colored(COLOR_MAGENTA, "?? 福利：坤币调整为 500,000,000\n");
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
         CLEAR_SCREEN();
         printf(COLOR_YELLOW COLOR_BOLD);
         printf("\n  ╔═══════════════════════════════════════════════════════════╗\n");
         printf("  ║                                                           ║\n");
         printf("  ║            欢迎来到 打工模拟器 v4.0 — 坤城篇               ║\n");
         printf("  ║              Welcome to Worker Simulator!                  ║\n");
         printf("  ║                                                           ║\n");
         printf("  ╚═══════════════════════════════════════════════════════════╝\n");
         printf(COLOR_RESET);

         printf("\n");
         print_colored(COLOR_CYAN, "  「坤城不问你的过去，只问你的价格。」\n\n");
         printf("  你来到了坤城——一座没有明确地理位置的沿海都市，约300万人口。\n");
         printf("  这里的人来自五湖四海：有人在CBD穿着西装谈百万生意，\n");
         printf("  有人在老城区出租屋里算着明天的饭钱。\n");
         printf("  今天还在路边摆摊的人，明天可能开奔驰——但也可能反过来。\n\n");
         printf("  你兜里只有500坤币，租住在老城区的一间小屋里。\n");
         printf("  手上有一部手机，一个聊天室账号，和一颗想往上爬的心。\n\n");
         print_colored(COLOR_YELLOW, "  坤城的故事，从今天开始。\n\n");

         printf("  请输入你的名字（最多49个字符）: ");
         char input_name[50];
         if (fgets(input_name, sizeof(input_name), stdin) != NULL) {
             // 去除换行符
             input_name[strcspn(input_name, "\n")] = 0;
             if (strlen(input_name) > 0) {
                 safe_strcpy(player.player_name, input_name, 50);
             } else {
                 safe_strcpy(player.player_name, "匿名玩家", 50);
             }
         } else {
             safe_strcpy(player.player_name, "匿名玩家", 50);
         }

         print_colored(COLOR_GREEN, "\n  欢迎，%s！\n", player.player_name);
         press_any_key();

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
                 print_colored(COLOR_MAGENTA, "?? 连续登录 %d 天奖励：额外获得 500 坤币和 1 张彩票！\n", 
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
    // 坤城股票市场 —— 5支代表性股票
    const char* stock_names[] = {"坤坤科技", "外卖速递", "坤城地产", "坤能集团", "星光传媒"};
    int base_prices[] = {100, 50, 200, 150, 80};
    int volatilities[] = {10, 15, 8, 12, 20};
     
     for (int i = 0; i < 5; i++) {
         safe_strcpy(player.stocks[i].name, stock_names[i], 50);
         player.stocks[i].base_price = base_prices[i];
         player.stocks[i].current_price = base_prices[i];
         player.stocks[i].price_change = 0;
         player.stocks[i].volatility = volatilities[i];
         player.stocks[i].last_update = time(NULL);
         player.invested_stocks[i] = 0;
     }
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
    draw_box("?? 坤城暗巷");
    
    print_colored(COLOR_RED, "?? %s 向你发来了一条加密消息...\n", player.chatters[chatter_index].name);
    printf("\n");
    print_colored(COLOR_RED, "?? \"有好东西。试一口，什么烦恼都没了。\"\n");
    printf("\n");
    print_colored(COLOR_YELLOW, "1. 同意尝试（进入幻觉状态，高风险）\n");
    print_colored(COLOR_GREEN, "2. 拒绝（坚守底线）\n");
    draw_separator();
     
     int choice = get_valid_input(1, 2);
     
     if (choice == 1) {
         // 同意OD
         print_colored(COLOR_RED, "\n?? 你尝试了对方给的东西...\n");
         print_colored(COLOR_MAGENTA, "?? 意识开始模糊...\n");
         
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
             print_colored(COLOR_RED, "??? 游戏结束！\n");
             print_colored(COLOR_YELLOW, "??? 可以通过存档恢复...\n");
             
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
    draw_box("?? 坤城之夜");
    
    print_colored(COLOR_MAGENTA, "?? %s 发来了一条私信...\n", player.chatters[chatter_index].name);
    printf("\n");
    print_colored(COLOR_MAGENTA, "?? \"今晚有空吗？一个人在这座城市挺孤单的……\"\n");
    printf("\n");
    print_colored(COLOR_YELLOW, "1. 同意见面\n");
    print_colored(COLOR_GREEN, "2. 礼貌拒绝\n");
    draw_separator();
     
     int choice = get_valid_input(1, 2);
     
    if (choice == 1) {
        print_colored(COLOR_MAGENTA, "\n?? 坤城的夜晚灯火通明……\n");
        print_colored(COLOR_CYAN, "?? 有人在身边，这座城市的孤独感好像淡了一点。\n");
        
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
 
 //================ 存档系统 (Web版单文件存档) ================

 void save_game() {
     // 写入单文件存档
     FILE* f = fopen(SAVE_FILE, "wb");
     if (f) {
         int v = SAVE_VERSION;
         fwrite(&v, sizeof(int), 1, f);
         fwrite(&player, sizeof(Player), 1, f);
         fwrite(tasks, sizeof(Task), MAX_TASKS, f);
         fwrite(player.tasks_unlocked, sizeof(bool), MAX_TASKS, f);
         fwrite(player.inventory, sizeof(InventoryItem), MAX_INVENTORY, f);
         fwrite(market, sizeof(MarketItem), MAX_MARKET, f);
         fwrite(player.achievements, sizeof(Achievement), MAX_ACHIEVEMENTS, f);
         fclose(f);
         print_colored(COLOR_GREEN, "\n?? 游戏存档已保存！\n");
     } else {
         print_colored(COLOR_RED, "\n? 存档写入失败！\n");
     }
     
     // 更新福布斯排行榜
     update_forbes_ranking();
 }

 void load_game() {
     FILE* f = fopen(SAVE_FILE, "rb");
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
     
     // 读取全部数据
     fread(&player, sizeof(Player), 1, f);
     fread(tasks, sizeof(Task), MAX_TASKS, f);
     fread(player.tasks_unlocked, sizeof(bool), MAX_TASKS, f);
     fread(player.inventory, sizeof(InventoryItem), MAX_INVENTORY, f);
     fread(market, sizeof(MarketItem), MAX_MARKET, f);
     fread(player.achievements, sizeof(Achievement), MAX_ACHIEVEMENTS, f);
     fclose(f);
     
    // v4.1 存档安全性检查：清除过期/损坏的事件数据
    if (player.active_event != EVENT_NONE) {
        time_t now = time(NULL);
        if (player.event_end_time <= 0 || player.event_end_time <= now) {
            print_colored(COLOR_YELLOW, "\n?? 检测到过期事件（%s），已自动清除。\n", get_event_name(player.active_event));
            player.active_event = EVENT_NONE;
            player.event_end_time = 0;
        }
    }
    
    print_colored(COLOR_GREEN, "\n?? 游戏存档已加载！\n");
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
                         print_colored(COLOR_GREEN, "  福布斯排名: 第 %d 名 (共 %d 名玩家)\n", i + 1, forbes_count);
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
             print_colored(COLOR_RED, "?? 警告：过度兴奋可能危及生命！\n");
             
             // 猝死风险
             int risk = (player.spirit - 78) * 2;
             if (rand() % 100 < risk) {
                 print_colored(COLOR_RED, "\n??? 你的心脏承受不了这种刺激...\n");
                 print_colored(COLOR_RED, "??? 游戏结束！\n");
                 print_colored(COLOR_YELLOW, "??? 从存档恢复...\n");
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
                print_colored(COLOR_RED, "? 你非常饥饿！快去买食物！\n");
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
     draw_box("?? 购买食物");
     
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
                 print_colored(COLOR_GREEN, "? 购买了面包，饱食度+15\n");
             } else {
                 print_colored(COLOR_RED, "? 坤币不足！\n");
             }
             break;
         case 2:
             if (remove_coins(30)) {
                 player.hunger += 30;
                 if (player.hunger > 100) player.hunger = 100;
                 print_colored(COLOR_GREEN, "? 购买了盒饭，饱食度+30\n");
             } else {
                 print_colored(COLOR_RED, "? 坤币不足！\n");
             }
             break;
         case 3:
             if (remove_coins(100)) {
                 player.hunger += 60;
                 if (player.hunger > 100) player.hunger = 100;
                 print_colored(COLOR_GREEN, "? 购买了大餐，饱食度+60\n");
             } else {
                 print_colored(COLOR_RED, "? 坤币不足！\n");
             }
             break;
         default:
             break;
     }
     
     press_any_key();
 }
 
 void eat_food() {
     CLEAR_SCREEN();
     draw_box("?? 背包食物");
     
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
         
         print_colored(COLOR_GREEN, "? 食用了 %s，饱食度+%d\n", item->name, item->effect_value);
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
            print_colored(COLOR_YELLOW, "??? 好在被人及时发现，送进了医院。\n");
            print_colored(COLOR_GREEN, "?? 从最近的存档恢复……\n");
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
        print_colored(COLOR_RED, "? 你还没有手机！\n");
        print_colored(COLOR_YELLOW, "? 可以在商城中购买手机。\n");
        press_any_key();
        return;
    }
    
    while (1) {
        CLEAR_SCREEN();
        draw_box("?? 手机");
        
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
                draw_box("?? 聊天记录");
                show_chat_messages();
                press_any_key();
                break;
            case 0:
                return;
        }
    }
}
 
void show_chat_room() {
    char input[200];
    static time_t last_auto_chat = 0;
    static int waiting_for_agreement = -1;  // -1表示没有等待同意，否则是chatter索引
    
    // 自动发言消息池 —— 坤城聊天室 v4.0
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
    int num_auto_messages = sizeof(auto_messages) / sizeof(auto_messages[0]);
    
    // 系统刷新消息池
    const char* system_messages[] = {
        "系统：聊天室定时清理完成",
        "系统：欢迎新用户加入",
        "系统：请注意文明聊天",
        "系统：服务器即将维护，请保存聊天记录"
    };
    int num_system_messages = sizeof(system_messages) / sizeof(system_messages[0]);
    
    while (1) {
        CLEAR_SCREEN();
        draw_box("?? 聊天室");
        
        // 显示在线人数
        int online = 0;
        for (int i = 0; i < player.chatter_count; i++) {
            if (player.chatters[i].is_online) online++;
        }
        printf("在线: %d人\n\n", online);
        
        // 显示最近消息
        show_chat_messages();
        
        // ===== 功能1：自动有人说话 =====
        time_t now = time(NULL);
        if (now - last_auto_chat >= 8 || (rand() % 100 < 35 && now - last_auto_chat >= 3)) {
            int chatter_idx = rand() % player.chatter_count;
            if (player.chatters[chatter_idx].is_online) {
                int msg_idx = rand() % num_auto_messages;
                add_chat_message(player.chatters[chatter_idx].name, auto_messages[msg_idx], player.chatters[chatter_idx].type);
            }
            last_auto_chat = now;
        }
        
        // ===== 功能4：随机刷新 =====
        if (rand() % 100 < 15) {
            int msg_idx = rand() % num_system_messages;
            add_chat_message("系统", system_messages[msg_idx], CHATTER_NORMAL);
        }
        
        draw_separator();
        
        // 如果正在等待同意/拒绝，提示用户
        if (waiting_for_agreement >= 0) {
            printf(COLOR_YELLOW "【%s 正在等待你的回复...】\n" COLOR_RESET, player.chatters[waiting_for_agreement].name);
            printf("请输入：同意 或 拒绝 | /quit 退出\n");
        } else {
            printf("命令：@用户名 内容 | 同意/拒绝 | /quit 退出\n");
        }
        printf("输入消息: ");
        
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';
        
        // ===== 功能3：不能发送空白 =====
        // 检查是否是空白消息（空或只有空格）
        bool all_spaces = true;
        for (int i = 0; input[i] != '\0'; i++) {
            if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n' && input[i] != '\r') {
                all_spaces = false;
                break;
            }
        }
        if (strlen(input) == 0 || all_spaces) {
            add_chat_message("系统", "? 不能发送空白消息！", CHATTER_NORMAL);
            continue;
        }
        
        if (strcmp(input, "/quit") == 0) break;
        
        // ===== 功能2：@人物同意机制 =====
        // 处理等待同意/拒绝的状态
        if (waiting_for_agreement >= 0) {
            if (strcmp(input, "同意") == 0 || strcmp(input, "同意") == 0) {
                add_chat_message("你", "同意", CHATTER_NORMAL);
                
                // 根据chatter类型处理同意后的逻辑
                ChatterType type = player.chatters[waiting_for_agreement].type;
                if (type == CHATTER_SCAMMER) {
                    // 骗子：扣100 coin
                    if (player.kun_coins >= 100) {
                        player.kun_coins -= 100;
                        add_chat_message(player.chatters[waiting_for_agreement].name, "哈哈，谢谢你的100坤币！你真是个好人~", type);
                        print_colored(COLOR_RED, "? 你被骗子骗了100坤币！\n");
                    } else {
                        add_chat_message(player.chatters[waiting_for_agreement].name, "哼，没钱还说什么同意？滚！", type);
                    }
                } else if (type == CHATTER_BLACKMARKET) {
                    process_od_offer(waiting_for_agreement);
                } else if (type == CHATTER_TONGCHENG) {
                    process_tongcheng_offer(waiting_for_agreement);
                } else {
                    add_chat_message(player.chatters[waiting_for_agreement].name, "谢谢你的同意！你真好~", type);
                }
                
                waiting_for_agreement = -1;
                press_any_key();
                continue;
            } else if (strcmp(input, "拒绝") == 0 || strcmp(input, "拒绝") == 0) {
                add_chat_message("你", "拒绝", CHATTER_NORMAL);
                add_chat_message(player.chatters[waiting_for_agreement].name, "好吧，没关系...", player.chatters[waiting_for_agreement].type);
                waiting_for_agreement = -1;
                press_any_key();
                continue;
            } else {
                add_chat_message("系统", "? 请输入 同意 或 拒绝", CHATTER_NORMAL);
                continue;
            }
        }
        
        // 添加到聊天历史
        add_chat_message("你", input, CHATTER_NORMAL);
        
        // 检查是否@某人
        if (input[0] == '@') {
            char target_name[30];
            char message[170];
            
            // 尝试解析 @用户名 内容
            if (sscanf(input, "@%s %[^\n]", target_name, message) >= 1) {
                // 查找目标
                for (int i = 0; i < player.chatter_count; i++) {
                    if (strcmp(player.chatters[i].name, target_name) == 0) {
                        // AI响应
                        if (strlen(message) > 0) {
                            ai_chatter_response(i, message);
                        } else {
                            // 只@了人名，没有消息
                            ai_chatter_response(i, "你好");
                        }
                        
                        // ===== @人物同意机制 =====
                        // 50%概率让对方提出需要同意的请求
                        if (rand() % 100 < 50) {
                            const char* requests[] = {
                                "我能向你借点钱吗？",
                                "你能帮我做个任务吗？",
                                "我想和你做个交易，可以吗？",
                                "你能帮我个忙吗？"
                            };
                            int req_idx = rand() % (sizeof(requests) / sizeof(requests[0]));
                            add_chat_message(player.chatters[i].name, requests[req_idx], player.chatters[i].type);
                            waiting_for_agreement = i;
                        }
                        
                        // 特殊处理：黑产的OD提议
                        if (player.chatters[i].type == CHATTER_BLACKMARKET &&
                            (strstr(message, "OD") || strstr(message, "药物") || 
                             strstr(message, "试试") || strstr(message, "东西"))) {
                            process_od_offer(i);
                        }
                        
                        // 特殊处理：同城的见面邀请
                        if (player.chatters[i].type == CHATTER_TONGCHENG &&
                            (strstr(message, "约") || strstr(message, "见面") ||
                             strstr(message, "出来") || strstr(message, "一起"))) {
                            process_tongcheng_offer(i);
                        }
                        
                        break;
                    }
                }
            } else {
                add_chat_message("系统", "? 格式错误：请使用 @用户名 内容", CHATTER_NORMAL);
            }
        } else {
            // 随机AI回复（10%概率）
            if (rand() % 100 < 10) {
                int chatter_idx = rand() % player.chatter_count;
                if (player.chatters[chatter_idx].is_online) {
                    ai_chatter_response(chatter_idx, input);
                }
            }
        }
        
        press_any_key();
    }
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
     draw_box("?? 成就系统");
     
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
             printf("? %s\n", player.achievements[i].name);
             printf("   %s\n", player.achievements[i].description);
         }
     }
     draw_separator();
 }
 
 void show_achievement_progress() {
     // 已在成就界面展示
 }
 
 //================ 股票市场更新 ================
 void update_stock_prices() {
     time_t now = time(NULL);
     for (int i = 0; i < 5; i++) {
         if (now - player.stocks[i].last_update >= 60) {
             int change = (rand() % (player.stocks[i].volatility * 2 + 1)) - player.stocks[i].volatility;
             player.stocks[i].current_price += change;
             if (player.stocks[i].current_price < 10) player.stocks[i].current_price = 10;
             player.stocks[i].price_change = change;
             player.stocks[i].last_update = now;
         }
     }
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
        print_colored(COLOR_YELLOW, "?? 微事件——%s\n", event->title);
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
        print_colored(COLOR_YELLOW, "?? 微事件——%s\n", event->title);
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
        print_colored(COLOR_YELLOW, "?? 微事件——%s\n", event->title);
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
        print_colored(COLOR_YELLOW, "?? 微事件——%s\n", event->title);
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
    snprintf(title_buf, sizeof(title_buf), "?? 坤城日报 — 第 %d 天", player.day_count);
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
            print_colored(COLOR_GREEN, "?? 解锁新任务：%s！\n", task->name);
        } else {
            print_colored(COLOR_RED, "? 需要 %d 坤币解锁此任务！\n", task->unlock_cost);
            return;
        }
    }
    
    if (player.level < task->required_level) {
        print_colored(COLOR_RED, "? 需要等级 %d 才能执行此任务！\n", task->required_level);
        return;
    }
    
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            if (player.skills[task->required_skills[i]].level < task->required_skill_level[i]) {
                print_colored(COLOR_RED, "? 需要 %s 技能等级 %d！\n", 
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
        print_colored(COLOR_RED, "? 今天的时间已经用完了！睡一觉明天再来吧。\n");
        day_end();
        return;
    }
    
    // === v4.0 选择打工难度 ===
    printf("\n");
    draw_separator();
    print_colored(COLOR_CYAN, "?? 选择打工模式：\n");
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
        print_colored(COLOR_RED, "? 时间不够！需要 %d 个时间槽，当前剩余 %d 个。\n", 
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
            print_colored(COLOR_RED, "?? 你差点猝死在坤城的街头！幸运地捡回一条命...\n");
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
        print_colored(COLOR_YELLOW, "?? 坤城的天空暗了下来，一天又过去了...\n");
        day_end();
    }
}
 
 void buy_item(int index) {
     if (index < 0 || index >= MAX_MARKET) return;
     
     MarketItem* item = &market[index];
     
     if (player.level < item->required_level) {
         print_colored(COLOR_RED, "? 需要等级 %d 才能购买此物品！\n", item->required_level);
         return;
     }
     
     if (item->is_limited && item->available_until < time(NULL)) {
         print_colored(COLOR_RED, "? 此限定物品已过期！\n");
         return;
     }
     
     if (player.furniture_count >= MAX_FURNITURE) {
         print_colored(COLOR_RED, "? 家具数量已达到上限！\n");
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
         print_colored(COLOR_RED, "? 坤币不足！需要 %d 坤币\n", price);
     }
 }
 
 void upgrade_skill(int skill_index) {
     if (skill_index < 0 || skill_index >= MAX_SKILLS) return;
     
     Skill* skill = &player.skills[skill_index];
     
     if (skill->level >= skill->max_level) {
         print_colored(COLOR_RED, "? 技能已达到最大等级！\n");
         return;
     }
     
     if (player.skill_points <= 0) {
         print_colored(COLOR_RED, "? 没有足够的技能点！\n");
         return;
     }
     
     int cost = skill->level * 100;
     cost = (int)(cost * (1.0f - player.prestige_level * 0.02f));
     if (cost < 10) cost = 10;
     
     if (!remove_coins(cost)) {
         print_colored(COLOR_RED, "? 升级需要 %d 坤币！\n", cost);
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
         print_colored(COLOR_RED, "? 宠物未解锁！\n");
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
         print_colored(COLOR_RED, "? 没有宠物食品！\n");
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
         print_colored(COLOR_RED, "? 宠物未解锁！\n");
         return;
     }
     
     if (player.energy < 5) {
         print_colored(COLOR_RED, "? 能量不足！\n");
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
         print_colored(COLOR_RED, "? 宠物数量已达上限！\n");
         return;
     }
     
     int unlock_level = 0;
     money_t unlock_cost = 0;
     const char* pet_type_name = "";
     
     switch (type) {
         case PET_CAT:
             if (player.pets[0].is_unlocked) {
                 print_colored(COLOR_RED, "? 已拥有此宠物！\n");
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
         print_colored(COLOR_RED, "? 需要等级 %d 才能领养此宠物！\n", unlock_level);
         return;
     }
     
     if (!remove_coins(unlock_cost)) {
         print_colored(COLOR_RED, "? 需要 %lld 坤币！\n", unlock_cost);
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
         print_colored(COLOR_RED, "? 公司已达到最高等级！\n");
         return;
     }
     
     CompanyLevel* next_level = &player.company_levels[player.current_company_level + 1];
     
     if (!remove_coins(next_level->required_coins)) {
         print_colored(COLOR_RED, "? 升级需要 %lld 坤币！\n", next_level->required_coins);
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
 
 void buy_stock(int stock_index, int amount) {
     if (stock_index < 0 || stock_index >= 5) return;
     if (amount <= 0) return;
     
     Stock* stock = &player.stocks[stock_index];
     int total_cost = stock->current_price * amount;
     
     // v4.1 全梭哈确认：当花费超过当前坤币50%时弹出确认
     if (total_cost > 0 && player.kun_coins > 0 && total_cost > player.kun_coins / 2) {
         printf("\n?? 警告：此操作将花费 %d 坤币（当前余额：%lld 坤币）\n", total_cost, player.kun_coins);
         printf("?? 确认要大手笔买入吗？(1=确认, 0=取消): ");
         int confirm = get_valid_input(0, 1);
         if (confirm == 0) {
             print_colored(COLOR_YELLOW, "? 已取消买入。\n");
             return;
         }
     }
     
     if (!remove_coins(total_cost)) {
         print_colored(COLOR_RED, "? 坤币不足！需要 %d 坤币\n", total_cost);
         return;
     }
     
     player.total_expenses += total_cost;
     player.invested_stocks[stock_index] += amount;
     
     print_colored(COLOR_GREEN, "\n?? 购买股票：%s × %d\n", stock->name, amount);
     printf("   单价：%d 坤币\n", stock->current_price);
     printf("   总价：%d 坤币\n", total_cost);
     
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
 
 void sell_stock(int stock_index, int amount) {
     if (stock_index < 0 || stock_index >= 5) return;
     if (amount <= 0) return;
     if (player.invested_stocks[stock_index] < amount) {
         print_colored(COLOR_RED, "? 持股不足！\n");
         return;
     }
     
     Stock* stock = &player.stocks[stock_index];
     int total_value = stock->current_price * amount;
     
     add_coins(total_value);
     player.total_income += total_value;
     player.invested_stocks[stock_index] -= amount;
     
     print_colored(COLOR_GREEN, "\n?? 出售股票：%s × %d\n", stock->name, amount);
     printf("   单价：%d 坤币\n", stock->current_price);
     printf("   总价：%d 坤币\n", total_value);
 }
 
 void buy_property(int property_index) {
     if (property_index < 0 || property_index >= 10) return;
     if (available_properties[property_index].owned) {
         print_colored(COLOR_RED, "? 已拥有此房产！\n");
         return;
     }
     
     Property* prop = &available_properties[property_index];
     
     if (!remove_coins(prop->coins_needed)) {
         print_colored(COLOR_RED, "? 坤币不足！需要 %lld 坤币\n", prop->coins_needed);
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
         print_colored(COLOR_RED, "? 没有彩票！\n");
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
         print_colored(COLOR_RED, "? 没有神秘钥匙！\n");
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
         print_colored(COLOR_RED, "? 当前没有BOSS挑战事件！\n");
         return;
     }
     
     if (player.energy < 50) {
         print_colored(COLOR_RED, "? 能量不足，需要50能量！\n");
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
         print_colored(COLOR_RED, "? 当前不是节日庆典！\n");
         return;
     }
     
     if (player.energy < 20) {
         print_colored(COLOR_RED, "? 能量不足！\n");
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
     draw_box("?? 物品合成");
     
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
                 print_colored(COLOR_GREEN, "? 合成成功！获得能量大补丸\n");
             } else {
                 print_colored(COLOR_RED, "? 能量饮料不足！需要2个\n");
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
                 print_colored(COLOR_GREEN, "? 合成成功！获得高级宠物粮\n");
             } else {
                 print_colored(COLOR_RED, "? 宠物食品不足！需要3个\n");
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
                 print_colored(COLOR_GREEN, "? 合成成功！获得1把神秘钥匙\n");
             } else {
                 print_colored(COLOR_RED, "? 彩票不足！需要5张\n");
             }
             break;
         }
         case 4:
             print_colored(COLOR_YELLOW, "? 该配方正在开发中...\n");
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
         print_colored(COLOR_RED, "? 好感度不足50，无法交易！\n");
         return;
     }
     
     printf("输入交易坤币数量：");
     char buffer[100];
     if (fgets(buffer, sizeof(buffer), stdin) == NULL) return;
     
     int amount;
     if (sscanf(buffer, "%d", &amount) != 1 || amount <= 0) {
         print_colored(COLOR_RED, "? 输入无效！\n");
         return;
     }
     
     if (!remove_coins(amount)) {
         print_colored(COLOR_RED, "? 坤币不足！\n");
         return;
     }
     
     player.total_expenses += amount;
     int exp_gain = amount / 10;
     player.kun_exp += exp_gain;
     f->friendship_level += 5;
     if (f->friendship_level > 100) f->friendship_level = 100;
     
     print_colored(COLOR_GREEN, "? 交易成功！获得 %d 经验，好感度+5\n", exp_gain);
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
         print_colored(COLOR_RED, "? 没有可赠送的礼物！\n");
         return;
     }
     
     f->friendship_level += 20;
     if (f->friendship_level > 100) f->friendship_level = 100;
     print_colored(COLOR_GREEN, "?? 赠送成功！好感度+20\n");
 }
 
 void prestige() {
     if (player.prestige_level == 0) {
         int unlocked_achievements = 0;
         for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
             if (player.achievements[i].unlocked) unlocked_achievements++;
         }
         
         if (player.level < 50 || player.kun_coins < 1000000 || unlocked_achievements < 20) {
             print_colored(COLOR_RED, "? 不满足声望重置条件！\n");
             printf("   需要：等级50、100万坤币、20个成就\n");
             return;
         }
     }
     
     printf("?? 确定要进行声望重置吗？这将重置大部分进度，但获得永久加成。\n");
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
        print_colored(COLOR_RED, "? 坤币不足！需要 %d\n", cost);
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
     draw_box("?? 好友系统");
     
     printf("当前好友：%d/%d\n\n", player.friend_count, MAX_FRIENDS);
     
     for (int i = 0; i < player.friend_count; i++) {
         Friend* f = &player.friends[i];
         printf("%d. %s\n", i + 1, f->username);
         printf("   等级：%d\n", f->level);
         printf("   好感度：%.0f/100\n", f->friendship_level);
         printf("   状态：%s\n", f->is_online ? "?? 在线" : "? 离线");
         printf("\n");
     }
     
     if (player.friend_count < MAX_FRIENDS) {
         printf("? 可添加好友：\n");
         for (int i = 0; i < MAX_FRIENDS; i++) {
             if (!potential_friends[i].is_friend && potential_friends[i].level > 0) {
                 printf("   %s (等级%d)\n", potential_friends[i].username, potential_friends[i].level);
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
             print_colored(COLOR_RED, "? 好友已满！\n");
         } else {
             for (int i = 0; i < MAX_FRIENDS; i++) {
                 if (!potential_friends[i].is_friend) {
                     player.friends[player.friend_count++] = potential_friends[i];
                     potential_friends[i].is_friend = true;
                     print_colored(COLOR_GREEN, "? 添加好友成功！\n");
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
     draw_box("?? 游戏统计");
     
     printf(COLOR_BOLD "?? 游戏时长\n" COLOR_RESET);
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
     for (int i = 0; i < 5; i++) {
         stock_total += player.invested_stocks[i];
     }
     printf("   股票持仓：%d股\n", stock_total);
     
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
     draw_box("?? 会员商城");
     
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
                 print_colored(COLOR_RED, "? 需要声望等级1！\n");
             }
             break;
         default:
             break;
     }
     press_any_key();
 }
 
 void show_settings() {
     CLEAR_SCREEN();
     draw_box("?? 游戏设置");
     
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
             printf("?? 确定重置所有进度？(y/n): ");
#ifdef __EMSCRIPTEN__
            confirm = getchar();
#else
             confirm = _getch();
#endif
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
                 print_colored(COLOR_GREEN, "? 游戏进度已重置！\n");
             }
             break;
         case 4:
             printf("?? 确定删除存档文件？(y/n): ");
#ifdef __EMSCRIPTEN__
            confirm = getchar();
#else
             confirm = _getch();
#endif
             printf("\n");
             if (confirm == 'y' || confirm == 'Y') {
                 if (remove(SAVE_FILE) == 0) {
                     print_colored(COLOR_GREEN, "? 存档已删除！\n");
                 } else {
                     print_colored(COLOR_RED, "? 删除失败！\n");
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
     draw_box("?? 游戏指南");
     
     printf(COLOR_BOLD "?? 游戏目标\n" COLOR_RESET);
     printf("在坤城这座欲望都市里活下去，闯出名堂，定义你自己的成功。\n");
     printf("通过打工赚钱、结交朋友、投资理财、经营公司，成为你想成为的人。\n\n");
     
     printf(COLOR_BOLD "?? 世界观\n" COLOR_RESET);
     printf("坤城是一座虚构的中国当代沿海城市，约300万人口。\n");
     printf("城区分为：老城区（租金低）、CBD（白领聚集）、开发区（工厂+物流）、\n");
     printf("大学城、富人区。你初始住在老城区出租屋。\n");
     printf("坤城不问你的过去，只问你的价格。\n\n");
     
     printf(COLOR_BOLD "?? 核心玩法\n" COLOR_RESET);
     printf("1. 打工赚钱：在坤城各行各业打工，从出租车司机到企业家\n");
     printf("2. 商城购物：购买家具和物品提升生活质量\n");
     printf("3. 技能升级：提升10项技能增加任务收入\n");
     printf("4. 成就系统：完成特定目标获得奖励（叙事型成就）\n");
     printf("5. 随机事件：坤城每天都有新鲜事发生\n\n");
     
     printf(COLOR_BOLD "? 能量系统\n" COLOR_RESET);
     printf("? 每个任务消耗能量\n");
     printf("? 能量每10分钟恢复1点\n");
     printf("? 使用能量饮料可快速恢复\n\n");
     
     printf(COLOR_BOLD "? 特色功能\n" COLOR_RESET);
     printf("? 季节系统：春夏秋冬影响坤城的打工和投资环境\n");
     printf("? 宠物系统：猫咪/狗狗/鹦鹉/龙/凤凰提供额外加成\n");
     printf("? 公司经营：从个体户到传奇财团，一步步建立你的商业帝国\n");
     printf("? 股票市场：投资坤坤科技等5支股票获得收益\n");
     printf("? 房产投资：在坤城各区购买房产获得稳定收入\n");
     printf("? 声望系统：重置游戏获得永久加成\n");
     printf("? 合成系统：合成高级物品\n");
     printf("? 好友系统：与小明、小红等坤城居民互动\n");
     printf("? 会员系统：购买会员享特权\n");
     printf("? 手机聊天室：与路人甲、小妹、老陈等坤城人物社交\n");
     printf("? 状态系统：饱食度/血量/精神影响你的坤城生活\n");
     
     draw_separator();
 }
 
void show_status() {
     CLEAR_SCREEN();
     draw_box("?? 玩家状态");

     printf(COLOR_BOLD "?? 基本信息\n" COLOR_RESET);
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
     draw_box("?? 背包物品");
     
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
         print_colored(COLOR_GREEN, "? 背包已整理！\n");
     } else if (choice == 3) {
         craft_item();
     }
     press_any_key();
 }
 
 void use_inventory_item(int item_index) {
     if (item_index < 0 || item_index >= player.inventory_count) return;
     
     InventoryItem* item = &player.inventory[item_index];
     
     if (item->quantity <= 0) {
         print_colored(COLOR_RED, "? 物品数量不足！\n");
         return;
     }
     
     item->quantity--;
     
     if (strcmp(item->name, "能量饮料") == 0) {
         player.energy += 20;
         if (player.energy > player.max_energy) player.energy = player.max_energy;
         print_colored(COLOR_GREEN, "? 使用能量饮料，恢复20点能量！\n");
     } else if (strcmp(item->name, "能量大补丸") == 0) {
         player.energy += 50;
         if (player.energy > player.max_energy) player.energy = player.max_energy;
         print_colored(COLOR_GREEN, "? 使用能量大补丸，恢复50点能量！\n");
     } else if (strcmp(item->name, "彩票") == 0) {
         player.lottery_tickets++;
         print_colored(COLOR_GREEN, "?? 获得1张彩票！\n");
     } else if (strcmp(item->name, "宠物食品") == 0) {
         print_colored(COLOR_GREEN, "?? 请在宠物界面喂食\n");
         item->quantity++;
         return;
     } else if (strcmp(item->name, "高级宠物粮") == 0) {
         print_colored(COLOR_GREEN, "?? 请在宠物界面喂食\n");
         item->quantity++;
         return;
     } else if (strcmp(item->name, "面包") == 0) {
         player.hunger += 15;
         if (player.hunger > 100) player.hunger = 100;
         print_colored(COLOR_GREEN, "? 食用面包，饱食度+15\n");
     } else if (strcmp(item->name, "盒饭") == 0) {
         player.hunger += 30;
         if (player.hunger > 100) player.hunger = 100;
         print_colored(COLOR_GREEN, "? 食用盒饭，饱食度+30\n");
     } else {
         print_colored(COLOR_YELLOW, "? 该物品无法使用\n");
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
     draw_box("?? 任务列表");
     
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
             printf(COLOR_GREEN "? %s\n" COLOR_RESET, quest->name);
             printf("   已完成！\n");
         } else if (quest->deadline != 0 && quest->deadline < time(NULL)) {
             printf(COLOR_RED "? %s\n" COLOR_RESET, quest->name);
             printf("   已过期\n");
         } else {
             if (player.is_hallucinating && (rand() % 2)) {
                 printf(COLOR_RED "?? ????\n" COLOR_RESET);
                 printf("   ￥#@%% !!!\n");
             } else {
                 printf(COLOR_CYAN "?? %s\n" COLOR_RESET, quest->name);
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
     draw_box("?? 宠物系统");
     
     printf("已解锁宠物：%d/%d\n\n", player.pet_count, MAX_PETS);
     
     const char* pet_type_names[] = {"猫咪", "狗狗", "鹦鹉", "龙", "凤凰"};
     
     for (int i = 0; i < MAX_PETS; i++) {
         Pet* pet = &player.pets[i];
         
         if (pet->is_unlocked) {
             printf("?? %s\n", pet->name);
             printf("   类型：%s\n", pet_type_names[pet->type]);
             printf("   等级：%d\n", pet->level);
             printf("   快乐度：");
             print_progress_bar(pet->happiness, 100, 20);
             printf("\n   饱食度：");
             print_progress_bar(pet->hunger, 100, 20);
             printf("\n   加成倍数：%.2f\n\n", pet->bonus_multiplier);
         } else {
             printf("?? 未解锁宠物\n");
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
             print_colored(COLOR_RED, "? 没有宠物！\n");
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
     draw_box("?? 公司管理");
     
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
 
 void show_stock_market() {
     CLEAR_SCREEN();
     draw_box("?? 股票市场");
     
     update_stock_prices();
     
     time_t now = time(NULL);
     printf("更新时间：%s", ctime(&now));
     printf("\n");
     
     for (int i = 0; i < 5; i++) {
         Stock* stock = &player.stocks[i];
         
         printf("%s\n", stock->name);
         printf("   当前价格：%d 坤币\n", stock->current_price);
         if (stock->price_change > 0) {
             printf(COLOR_GREEN "   涨跌：+%d (%.1f%%)\n" COLOR_RESET, 
                    stock->price_change, (float)stock->price_change / stock->current_price * 100);
         } else if (stock->price_change < 0) {
             printf(COLOR_RED "   涨跌：%d (%.1f%%)\n" COLOR_RESET, 
                    stock->price_change, (float)stock->price_change / stock->current_price * 100);
         } else {
             printf("   涨跌：0\n");
         }
         printf("   波动率：%d%%\n", stock->volatility);
         printf("   持有数量：%d\n", player.invested_stocks[i]);
         if (player.invested_stocks[i] > 0) {
             int total_value = player.invested_stocks[i] * stock->current_price;
             printf("   当前价值：%d 坤币\n", total_value);
         }
         printf("\n");
     }
     
     draw_separator();
     printf("1. 购买股票   2. 出售股票   3. 刷新行情   0. 返回\n");
     draw_separator();
     
     int choice = get_valid_input(0, 3);
     if (choice == 1 || choice == 2) {
         printf("选择股票 (1-5): ");
         int stock_choice = get_valid_input(1, 5);
         printf("输入数量: ");
         int amount = get_valid_input(1, 10000);
         
         if (choice == 1) {
             buy_stock(stock_choice - 1, amount);
         } else {
             sell_stock(stock_choice - 1, amount);
         }
     } else if (choice == 3) {
         for (int i = 0; i < 5; i++) {
             player.stocks[i].last_update = time(NULL) - 61;
             player.stocks[i].price_change = 0;
         }
         update_stock_prices();
         print_colored(COLOR_GREEN, "? 行情已刷新！\n");
     }
     press_any_key();
 }
 
 void show_properties() {
     CLEAR_SCREEN();
     draw_box("?? 房产投资");
     
     printf("已拥有房产：%d/10\n\n", player.property_count);
     
     float daily_passive = 0;
     for (int i = 0; i < player.property_count; i++) {
         daily_passive += player.properties[i].passive_income;
     }
     printf("当前每日被动收入：%.0f 坤币\n\n", daily_passive);
     
     for (int i = 0; i < 10; i++) {
         Property* prop = &available_properties[i];
         
         if (prop->owned) {
             printf(COLOR_GREEN "?? %s\n" COLOR_RESET, prop->name);
             printf("   %s\n", prop->description);
             printf("   被动收入：%.0f 坤币/天\n", prop->passive_income);
         } else {
             printf(COLOR_WHITE "?? %s\n" COLOR_RESET, prop->name);
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
        draw_box("?? 坤城人民医院");
        
        printf(COLOR_BOLD "  欢迎来到坤城人民医院！\n" COLOR_RESET);
        printf("  ─────────────────────────────────────\n");
        printf("  你的当前状态：\n");
        printf("  ?? 血量: %d/100\n", player.health);
        printf("  ?? 精神: %d/100\n", player.spirit);
        printf("  ?? 当前坤币: %lld\n", player.kun_coins);
        printf("  ─────────────────────────────────────\n\n");
        
        printf("  请选择治疗项目：\n\n");
        printf("  1. 基础包扎 — 50坤币 (恢复30血量 + 10精神)\n");
        printf("  2. 标准诊疗 — 100坤币 (恢复60血量 + 20精神)\n");
        printf("  3. 全面康复 — 200坤币 (恢复100血量 + 40精神)\n");
        printf("  4. 心理疏导 — 80坤币 (恢复50精神)\n");
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
                printf("  ?? 血量 +%d (当前: %d/100)\n", heal_hp, player.health);
            }
            if (heal_spirit > 0) {
                player.spirit += heal_spirit;
                if (player.spirit > 100) player.spirit = 100;
                printf("  ?? 精神 +%d (当前: %d/100)\n", heal_spirit, player.spirit);
            }
            printf("  ?? 花费了 %d 坤币\n", cost);
        } else {
            print_colored(COLOR_RED, "\n  ? 坤币不够！治疗需要 %d 坤币，你只有 %lld 坤币。\n", cost, player.kun_coins);
        }
        press_any_key();
    }
}

void show_main_menu() {
    CLEAR_SCREEN();
    
    // 幻觉效果影响菜单显示
    if (player.is_hallucinating) {
        printf(COLOR_RED);
        printf("  ╔════════════════════════════════════════════════════════════════════════════╗\n");
        printf("  ║                              工???? ???? v4.0                            ║\n");
        printf("  ║                                                                            ║\n");
        printf("  ╚════════════════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET "\n");
        
        printf(COLOR_BOLD "?? ?????????" COLOR_RESET);
        printf("??? %d | ???? %lld | ???? %d/%d | ???? %d\n\n", 
               player.level, player.kun_coins, player.daily_time_slots, player.max_time_slots, player.prestige_level);
    } else {
        printf(COLOR_YELLOW COLOR_BOLD);
        printf("  ╔════════════════════════════════════════════════════════════════════════════╗\n");
        printf("  ║                                                                            ║\n");
        printf("  ║                         打工模拟器 v4.0 — 坤城篇                              ║\n");
        printf("  ║                                                                            ║\n");
        printf("  ╚════════════════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET "\n");
        
        // ===== 显示所有状态值 =====
        printf(COLOR_BOLD "?? 玩家信息：" COLOR_RESET);
        printf("等级 %d | 坤币 %lld | 时间槽 %d/%d | 第%d天 | 声望 %d\n", 
               player.level, player.kun_coins, player.daily_time_slots, player.max_time_slots, player.day_count, player.prestige_level);
        
        // 显示饱食度（带进度条）
        printf("  饱食度: ");
        int bar_len = 20;
        int filled = player.hunger * bar_len / 100;
        for (int i = 0; i < bar_len; i++) {
            if (i < filled) printf("#");
            else printf("-");
        }
        if (player.hunger < 20) printf(COLOR_RED " %d/100 [饥饿!]" COLOR_RESET, player.hunger);
        else if (player.hunger < 50) printf(COLOR_YELLOW " %d/100" COLOR_RESET, player.hunger);
        else printf(COLOR_GREEN " %d/100" COLOR_RESET, player.hunger);
        printf("\n");
        
        // 显示血量（带进度条）
        printf("  血量:   ");
        filled = player.health * bar_len / 100;
        for (int i = 0; i < bar_len; i++) {
            if (i < filled) printf("#");
            else printf("-");
        }
        if (player.health < 30) printf(COLOR_RED " %d/100 [重伤!]" COLOR_RESET, player.health);
        else if (player.health < 60) printf(COLOR_YELLOW " %d/100" COLOR_RESET, player.health);
        else printf(COLOR_GREEN " %d/100" COLOR_RESET, player.health);
        printf("\n");
        
        // 显示精神值（带进度条）
        printf("  精神:   ");
        filled = player.spirit * bar_len / 100;
        for (int i = 0; i < bar_len; i++) {
            if (i < filled) printf("#");
            else printf("-");
        }
        if (player.spirit < 20) printf(COLOR_RED " %d/100 [崩溃!]" COLOR_RESET, player.spirit);
        else if (player.spirit > 78) printf(COLOR_MAGENTA " %d/100 [兴奋]" COLOR_RESET, player.spirit);
        else printf(COLOR_CYAN " %d/100" COLOR_RESET, player.spirit);
        printf("\n");
        
        // 显示经验值（带进度条）
        printf("  经验:   ");
        int exp_max = player.level * 100;
        filled = (exp_max > 0) ? player.kun_exp * bar_len / exp_max : 0;
        if (filled > bar_len) filled = bar_len;
        for (int i = 0; i < bar_len; i++) {
            if (i < filled) printf("#");
            else printf("-");
        }
        printf(COLOR_CYAN " %d/%d" COLOR_RESET, player.kun_exp, exp_max);
        printf("\n\n");
    }
    
    draw_separator();
    printf(COLOR_BOLD "?? 主菜单\n" COLOR_RESET);
    printf("┌─────────────────────┬─────────────────────┬─────────────────────┐\n");
    printf("│ 1. ?? 查看状态      │ 2. ?? 打工赚钱      │ 3. ?? 商城购物      │\n");
    printf("│ 4. ?? 技能系统      │ 5. ?? 背包物品      │ 6. ?? 任务列表      │\n");
    printf("│ 7. ?? 宠物系统      │ 8. ?? 公司管理      │ 9. ?? 股票市场      │\n");
    printf("│10. ?? 房产投资      │11. ?? 成就系统      │12. ? 声望系统      │\n");
    printf("│13. ?? 好友系统      │14. ?? 游戏统计      │15. ?? 会员商城      │\n");
    printf("│16. ?? 游戏设置      │17. ?? 帮助指南      │18. ?? 保存游戏      │\n");
    printf("│19. ?? 手机          │20. ?? 福布斯排行    │21. ?? 睡觉休息      │\n");
    printf("│22. ?? 医院治疗      │ 0. ?? 退出游戏      │    (结束今天)      │\n");
    printf("└─────────────────────┴─────────────────────┴─────────────────────┘\n");
    draw_separator();
    
    show_active_events();
    
    // 显示状态警告（增强版）
    if (player.health <= 0) {
        print_colored(COLOR_RED, "??? 警告：你已经昏迷了！血量0/100，必须去医院抢救！\n");
    }
    if (player.hunger < 20) {
        print_colored(COLOR_RED, "? 警告：你非常饥饿！饱食度%d/100，请尽快去商城买食物！\n", player.hunger);
    } else if (player.hunger < 40) {
        print_colored(COLOR_YELLOW, "? 提示：你有点饿了（饱食度%d/100），建议去商城买食物。\n", player.hunger);
    }
    if (player.health < 30 && player.health > 0) {
        print_colored(COLOR_RED, "? 警告：你身受重伤！血量%d/100，请及时治疗！\n", player.health);
    } else if (player.health < 60 && player.health > 0) {
        print_colored(COLOR_YELLOW, "? 提示：你的健康状况不佳（血量%d/100），建议注意安全。\n", player.health);
    }
    if (player.spirit < 20) {
        print_colored(COLOR_RED, "? 警告：你的精神处于崩溃边缘！精神%d/100\n", player.spirit);
    } else if (player.spirit > 78) {
        print_colored(COLOR_YELLOW, "? 警告：你过于兴奋！小心身体！（精神%d/100）\n", player.spirit);
    }
    if (player.daily_time_slots <= 0 && !player.day_ended) {
        print_colored(COLOR_RED, "? 警告：今天的时间用完了！请结束一天或等待明天。\n");
    }
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
     // 强制切换控制台为UTF-8编码，解决中文乱码
     system("chcp 65001 > nul");
     anti_analysis_check();
#elif defined(__EMSCRIPTEN__)
     // Web版不需要反调试
#else
     // Linux等其他平台
#endif
     show_loading_screen(2000);
     
     init_game();
     
     time_t last_passive_time = time(NULL);
     time_t last_save_time = time(NULL);
     time_t last_health_time = time(NULL);
     
     while (1) {
         show_main_menu();
         
        printf("请输入选项: ");
        int choice = get_valid_input(0, 22);
         
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
        
        switch (choice) {
             case 1:
                 show_status();
                 press_any_key();
                 break;
             case 2: {
                 CLEAR_SCREEN();
                 draw_box("?? 任务中心");
                 
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
                    draw_box("?? 商城");
                    
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
                            draw_box("?? 家具商店");
                            
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
                            draw_box("?? 食物商店");
                            
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
                                    print_colored(COLOR_GREEN, "? 购买了%s，饱食度+%d\n", food_name, food_value);
                                } else {
                                    print_colored(COLOR_RED, "? 坤币不足！\n");
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
                draw_box("?? 睡觉休息");
                printf("  你决定提前结束今天的工作，好好休息一觉...\n\n");
                press_any_key();
                day_end();
                press_any_key();
                break;
            case 22:
                hospital_menu();
                break;
            case 0:
                 CLEAR_SCREEN();
                 printf(COLOR_YELLOW "\n");
                 printf("  ╔════════════════════════════════════════════════════════════╗\n");
                 printf("  ║                                                                ║\n");
                 printf("  ║                    感谢游玩打工模拟器 v4.0 — 坤城篇！                  ║\n");
                 printf("  ║                                                                ║\n");
                 printf("  ║                  坤城会记得每一个认真生活的人。                     ║\n");
                 printf("  ║                                                                ║\n");
                 printf("  ╚════════════════════════════════════════════════════════════╝\n");
                 printf(COLOR_RESET "\n");
                 save_game();
                 return 0;
         }
         
         random_event();
         
         static time_t last_update = 0;
         if (last_update == 0) last_update = now;
         player.total_play_time += (int)(now - last_update);
         last_update = now;
#ifdef __EMSCRIPTEN__
        emscripten_sleep(5);  // 主循环 yield，让浏览器处理事件
#endif
     }
     
     return 0;
 } // 结束了喵~
