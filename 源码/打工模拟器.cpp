/*
 * 打工模拟器 v3.0 - 完整版
 * 修复所有bug并完善功能
 * 编译环境: Windows/Linux, C11/C++兼容
 * 编译命令: gcc -o worker_sim worker_simulator.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#define _getch() getchar()
#endif

//================ 常量定义 ================
#define MAX_FURNITURE 50
#define MAX_TASKS 15
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
#define SAVE_VERSION 3

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
    int craft_count;  // 新增：合成次数统计
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
void save_game();
void load_game();
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
void start_quest(int quest_index);
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
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
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
    _getch();
    printf("\n");
}

void show_loading_screen(int duration) {
    CLEAR_SCREEN();
    print_centered("打工模拟器 v3.0");
    printf("\n\n");
    print_centered("加载中...");
    printf("\n    ");
    
    for (int i = 0; i < 50; i++) {
        printf(COLOR_GREEN "█" COLOR_RESET);
        fflush(stdout);
#ifdef _WIN32
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

//================ 初始化函数 ================
void init_game() {
    srand((unsigned int)(time(NULL) ^ (unsigned long)&init_game));
    
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
    
    // 尝试加载存档
    load_game();
    
    // 更新游戏状态
    time_t now = time(NULL);
    if (player.last_login == 0) {
        player.last_login = now;
        player.game_start_time = now;
        player.current_season = SEASON_SPRING;
        player.day_count = 1;
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
            player.kun_coins += bonus;
            player.kun_exp += 50 * player.consecutive_days;
            player.daily_task_completions = 0;
            
            print_colored(COLOR_YELLOW, "\n? 每日登录奖励：获得 %lld 坤币和 %d 经验！\n", 
                         bonus, 50 * player.consecutive_days);
            
            if (player.consecutive_days % 7 == 0) {
                player.kun_coins += 500LL;
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
}

void init_player() {
    memset(&player, 0, sizeof(Player));
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
    player.lottery_tickets = 1;  // 初始1张彩票
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
    
    // 初始家具
    safe_strcpy(player.furniture[0], "简易床", 50);
    safe_strcpy(player.furniture[1], "破旧椅子", 50);
    player.furniture_count = 2;
    
    // 初始物品
    InventoryItem starter_items[] = {
        {"能量饮料", 0, 10, RARITY_COMMON, true, 20, "恢复20点能量", 3},
        {"新手装备", 0, 50, RARITY_UNCOMMON, false, 5, "增加5%任务收入", 1},
        {"彩票", 0, 5, RARITY_COMMON, true, 0, "有机会赢取大奖", 1},
        {"宠物食品", 0, 5, RARITY_COMMON, true, 30, "喂食宠物", 5}
    };
    
    for (int i = 0; i < 4; i++) {
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
    safe_strcpy(tasks[0].description, "驾驶出租车接送客人", 150);
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
    safe_strcpy(tasks[1].description, "在森林中砍伐树木", 150);
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
    safe_strcpy(tasks[2].description, "配送外卖订单", 150);
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
    safe_strcpy(tasks[3].description, "在高级餐厅服务", 150);
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
    safe_strcpy(tasks[4].description, "编写软件代码", 150);
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
    safe_strcpy(tasks[5].description, "进行股票交易", 150);
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
    safe_strcpy(tasks[6].description, "销售高端房产", 150);
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
    safe_strcpy(tasks[7].description, "创办自己的企业", 150);
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
    safe_strcpy(tasks[8].description, "投资创业公司", 150);
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
    safe_strcpy(tasks[9].description, "探索未知领域", 150);
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
    // 家具类物品
    MarketItem items[] = {
        {"电视", 300, 50, RARITY_COMMON, false, 0, "基本的娱乐设备", 0.1f, 1, 1.0f},
        {"椅子", 100, 20, RARITY_COMMON, false, 0, "舒适的座椅", 0.05f, 1, 1.0f},
        {"沙发", 500, 100, RARITY_UNCOMMON, false, 0, "豪华沙发", 0.2f, 3, 1.1f},
        {"游泳池", 2000, 500, RARITY_RARE, false, 0, "私人游泳池", 0.5f, 10, 1.2f},
        {"黄金马桶", 10000, 2000, RARITY_EPIC, false, 0, "奢华的马桶", 1.0f, 20, 1.3f},
        {"名画", 1500, 300, RARITY_RARE, false, 0, "著名画作", 0.3f, 5, 1.15f},
        {"水晶吊灯", 3000, 800, RARITY_EPIC, false, 0, "华丽的水晶灯", 0.6f, 15, 1.25f},
        {"家庭影院", 5000, 1500, RARITY_EPIC, false, 0, "顶级影音系统", 0.8f, 18, 1.3f},
        {"工作台", 800, 200, RARITY_UNCOMMON, false, 0, "提升工作效率", 0.25f, 5, 1.1f},
        {"健身房", 2500, 600, RARITY_RARE, false, 0, "锻炼身体", 0.4f, 12, 1.2f},
        {"游戏机", 600, 120, RARITY_UNCOMMON, false, 0, "娱乐游戏设备", 0.15f, 4, 1.05f},
        {"按摩椅", 1200, 250, RARITY_RARE, false, 0, "放松身心", 0.35f, 8, 1.15f},
        {"智能音箱", 200, 40, RARITY_COMMON, false, 0, "智能家居设备", 0.08f, 2, 1.02f},
        {"空气净化器", 400, 80, RARITY_COMMON, false, 0, "净化空气", 0.12f, 3, 1.03f},
        {"咖啡机", 350, 70, RARITY_COMMON, false, 0, "制作咖啡", 0.1f, 3, 1.03f},
        {"书柜", 450, 90, RARITY_UNCOMMON, false, 0, "存放书籍", 0.14f, 4, 1.04f},
        {"钢琴", 3000, 700, RARITY_RARE, false, 0, "音乐乐器", 0.5f, 12, 1.2f},
        {"台球桌", 2000, 500, RARITY_RARE, false, 0, "娱乐设施", 0.4f, 10, 1.18f},
        {"酒柜", 1500, 350, RARITY_UNCOMMON, false, 0, "存放美酒", 0.3f, 8, 1.12f},
        {"保险箱", 800, 200, RARITY_UNCOMMON, false, 0, "存放贵重物品", 0.2f, 6, 1.08f}
    };
    
    int item_count = sizeof(items) / sizeof(items[0]);
    for (int i = 0; i < item_count && i < MAX_MARKET; i++) {
        market[i] = items[i];
    }
}

void init_achievements() {
    Achievement achievements[] = {
        {"第一桶金", "赚取100坤币", 100, 50, 10, false, 0},
        {"家具收藏家", "收集5件家具", 5, 200, 20, false, 0},
        {"打工达人", "完成50次任务", 50, 500, 50, false, 0},
        {"股市之神", "股票投资获利1000坤币", 1000, 800, 80, false, 0},
        {"幸运儿", "赢得彩票大奖", 1, 1000, 100, false, 0},
        {"百万富翁", "拥有100万坤币", 1000000, 5000, 500, false, 0},
        {"等级大师", "达到50级", 50, 2000, 200, false, 0},
        {"技能专家", "将一项技能升到10级", 10, 1500, 150, false, 0},
        {"任务狂人", "完成1000次任务", 1000, 3000, 300, false, 0},
        {"社交达人", "拥有5个好友", 5, 1000, 100, false, 0},
        {"宠物爱好者", "拥有3只宠物", 3, 1500, 150, false, 0},
        {"房产大亨", "拥有5处房产", 5, 4000, 400, false, 0},
        {"全技能大师", "所有技能达到5级", 5, 5000, 500, false, 0},
        {"限定收藏家", "收集所有限定物品", 10, 6000, 600, false, 0},
        {"成就猎人", "解锁20个成就", 20, 3000, 300, false, 0},
        {"初次转生", "完成第一次声望重置", 1, 10000, 1000, false, 0},
        {"传奇重生", "达到声望等级5", 5, 50000, 5000, false, 0},
        {"BOSS克星", "击败10次BOSS", 10, 8000, 800, false, 0},
        {"节日之魂", "参与5次节日活动", 5, 5000, 500, false, 0},
        {"合成大师", "合成10次物品", 10, 3000, 300, false, 0}
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
    
    // 每日任务
    safe_strcpy(player.active_quests[0].name, "完成3个任务", 50);
    safe_strcpy(player.active_quests[0].description, "完成任意3个任务", 200);
    player.active_quests[0].type = QUEST_DAILY;
    player.active_quests[0].requirement = 3;
    player.active_quests[0].reward_coins = 300;
    player.active_quests[0].reward_exp = 30;
    player.active_quests[0].assigned_time = now;
    player.active_quests[0].deadline = now + 86400;
    
    safe_strcpy(player.active_quests[1].name, "购买家具", 50);
    safe_strcpy(player.active_quests[1].description, "购买任意一件家具", 200);
    player.active_quests[1].type = QUEST_DAILY;
    player.active_quests[1].requirement = 1;
    player.active_quests[1].reward_coins = 200;
    player.active_quests[1].reward_exp = 20;
    player.active_quests[1].assigned_time = now;
    player.active_quests[1].deadline = now + 86400;
    
    safe_strcpy(player.active_quests[2].name, "投资股票", 50);
    safe_strcpy(player.active_quests[2].description, "进行一次股票投资", 200);
    player.active_quests[2].type = QUEST_DAILY;
    player.active_quests[2].requirement = 1;
    player.active_quests[2].reward_coins = 400;
    player.active_quests[2].reward_exp = 40;
    player.active_quests[2].assigned_time = now;
    player.active_quests[2].deadline = now + 86400;
    
    // 每周任务
    safe_strcpy(player.active_quests[3].name, "赚取1万坤币", 50);
    safe_strcpy(player.active_quests[3].description, "一周内赚取10000坤币", 200);
    player.active_quests[3].type = QUEST_WEEKLY;
    player.active_quests[3].requirement = 10000;
    player.active_quests[3].reward_coins = 2000;
    player.active_quests[3].reward_exp = 200;
    player.active_quests[3].assigned_time = now;
    player.active_quests[3].deadline = now + 604800;
    
    safe_strcpy(player.active_quests[4].name, "升级技能", 50);
    safe_strcpy(player.active_quests[4].description, "升级任意技能3次", 200);
    player.active_quests[4].type = QUEST_WEEKLY;
    player.active_quests[4].requirement = 3;
    player.active_quests[4].reward_coins = 1500;
    player.active_quests[4].reward_exp = 150;
    player.active_quests[4].assigned_time = now;
    player.active_quests[4].deadline = now + 604800;
    
    // 主线任务
    safe_strcpy(player.active_quests[5].name, "成为百万富翁", 50);
    safe_strcpy(player.active_quests[5].description, "积累100万坤币", 200);
    player.active_quests[5].type = QUEST_MAIN;
    player.active_quests[5].requirement = 1000000;
    player.active_quests[5].reward_coins = 10000;
    player.active_quests[5].reward_exp = 1000;
    player.active_quests[5].assigned_time = now;
    player.active_quests[5].deadline = 0;
    
    safe_strcpy(player.active_quests[6].name, "建立公司", 50);
    safe_strcpy(player.active_quests[6].description, "将公司升到5级", 200);
    player.active_quests[6].type = QUEST_MAIN;
    player.active_quests[6].requirement = 5;
    player.active_quests[6].reward_coins = 5000;
    player.active_quests[6].reward_exp = 500;
    player.active_quests[6].assigned_time = now;
    player.active_quests[6].deadline = 0;
    
    // 特殊任务
    safe_strcpy(player.active_quests[7].name, "击败BOSS", 50);
    safe_strcpy(player.active_quests[7].description, "首次击败挑战BOSS", 200);
    player.active_quests[7].type = QUEST_SPECIAL;
    player.active_quests[7].requirement = 1;
    player.active_quests[7].reward_coins = 5000;
    player.active_quests[7].reward_exp = 500;
    player.active_quests[7].assigned_time = now;
    player.active_quests[7].deadline = 0;
    
    safe_strcpy(player.active_quests[8].name, "节日参与", 50);
    safe_strcpy(player.active_quests[8].description, "参与一次节日活动", 200);
    player.active_quests[8].type = QUEST_SPECIAL;
    player.active_quests[8].requirement = 1;
    player.active_quests[8].reward_coins = 3000;
    player.active_quests[8].reward_exp = 300;
    player.active_quests[8].assigned_time = now;
    player.active_quests[8].deadline = 0;
    
    safe_strcpy(player.active_quests[9].name, "声望之路", 50);
    safe_strcpy(player.active_quests[9].description, "完成第一次声望重置", 200);
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
    const char* stock_names[] = {"坤坤科技", "外卖速递", "房地产集团", "能源公司", "娱乐传媒"};
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

//================ 存档系统 ================
void save_game() {
    FILE* file = fopen(SAVE_FILE, "wb");
    if (file) {
        // 写入版本号
        int version = SAVE_VERSION;
        fwrite(&version, sizeof(int), 1, file);
        fwrite(&player, sizeof(Player), 1, file);
        fclose(file);
        print_colored(COLOR_GREEN, "\n?? 游戏存档已保存！\n");
    } else {
        print_colored(COLOR_RED, "\n? 保存失败！\n");
    }
}

void load_game() {
    FILE* file = fopen(SAVE_FILE, "rb");
    if (file) {
        int version;
        fread(&version, sizeof(int), 1, file);
        if (version == SAVE_VERSION) {
            fread(&player, sizeof(Player), 1, file);
            print_colored(COLOR_GREEN, "\n?? 游戏存档已加载！\n");
        } else {
            print_colored(COLOR_YELLOW, "\n?? 存档版本不兼容，将开始新游戏...\n");
        }
        fclose(file);
    } else {
        print_colored(COLOR_YELLOW, "\n?? 未找到存档，开始新游戏...\n");
    }
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
        time_t remaining = player.event_end_time - time(NULL);
        if (remaining > 0) {
            print_colored(COLOR_MAGENTA, "\n?? 当前事件：%s\n", get_event_name(player.active_event));
            printf("   剩余时间：%ld秒\n", remaining);
            printf("   效果：%s\n", get_event_description(player.active_event));
        } else {
            end_event();
        }
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
            case 4: condition_met = false; break; // 彩票中奖单独触发
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
            case 13: break; // 限定收藏品暂不实现
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
    
    player.kun_coins += player.achievements[achievement_id].reward;
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
    
    // 家具被动收入
    for (int i = 0; i < player.furniture_count; i++) {
        for (int j = 0; j < MAX_MARKET; j++) {
            if (strcmp(player.furniture[i], market[j].name) == 0) {
                passive += (money_t)market[j].passive_income;
                break;
            }
        }
    }
    
    // 房产被动收入
    for (int i = 0; i < player.property_count; i++) {
        passive += (money_t)player.properties[i].passive_income;
    }
    
    // 管理技能加成
    passive = (money_t)(passive * pow(player.skills[SKILL_MANAGEMENT].multiplier, 
                                      player.skills[SKILL_MANAGEMENT].level - 1));
    
    if (passive > 0) {
        player.kun_coins += passive;
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
    if (seconds_passed >= 600) {  // 每10分钟恢复1点
        int recovery = seconds_passed / 600;
        if (recovery > 10) recovery = 10;
        
        if (player.has_premium) recovery *= 2;
        
        player.energy += recovery;
        if (player.energy > player.max_energy) player.energy = player.max_energy;
        
        last_energy_recovery = now;
    }
}

//================ 游戏主逻辑 ================
void do_task(int task_index) {
    if (task_index < 0 || task_index >= MAX_TASKS) return;
    
    Task* task = &tasks[task_index];
    
    // 检查是否解锁
    if (!task->is_unlocked) {
        if (player.kun_coins >= task->unlock_cost) {
            task->is_unlocked = true;
            player.tasks_unlocked[task_index] = true;
            player.kun_coins -= task->unlock_cost;
            player.total_expenses += task->unlock_cost;
            print_colored(COLOR_GREEN, "?? 解锁新任务：%s！\n", task->name);
        } else {
            print_colored(COLOR_RED, "? 需要 %d 坤币解锁此任务！\n", task->unlock_cost);
            return;
        }
    }
    
    // 检查等级要求
    if (player.level < task->required_level) {
        print_colored(COLOR_RED, "? 需要等级 %d 才能执行此任务！\n", task->required_level);
        return;
    }
    
    // 检查技能要求
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
    
    // 计算实际能量消耗（耐力技能减免）
    int actual_energy_cost = task->energy_cost;
    if (player.skills[SKILL_ENDURANCE].level > 0) {
        float reduction = 1.0f - player.skills[SKILL_ENDURANCE].level * 0.02f;
        if (reduction < 0.5f) reduction = 0.5f;
        actual_energy_cost = (int)(task->energy_cost * reduction);
        if (actual_energy_cost < 1) actual_energy_cost = 1;
    }
    
    // 检查能量
    if (player.energy < actual_energy_cost) {
        print_colored(COLOR_RED, "? 能量不足！需要 %d 能量\n", actual_energy_cost);
        return;
    }
    
    // 成功率判定
    float success_chance = task->success_rate;
    success_chance *= pow(player.skills[SKILL_LUCK].multiplier, player.skills[SKILL_LUCK].level - 1);
    if (success_chance > 0.95f) success_chance = 0.95f;
    
    bool success = (rand() % 100) < (int)(success_chance * 100);
    
    // 计算收入
    float base_income = (float)task->base_income;
    float multiplier = 1.0f;
    
    // 技能加成
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            multiplier *= pow(player.skills[task->required_skills[i]].multiplier, 
                            player.skills[task->required_skills[i]].level);
        }
    }
    
    // 公司等级加成
    if (player.current_company_level > 0) {
        multiplier *= player.company_levels[player.current_company_level].income_multiplier;
    }
    
    // 事件加成
    if (player.active_event == EVENT_TASK_BONUS) {
        multiplier *= 1.5f;
    }
    
    // 季节加成
    switch (player.current_season) {
        case SEASON_SPRING: multiplier *= 1.1f; break;
        case SEASON_SUMMER: multiplier *= 1.05f; break;
        case SEASON_AUTUMN: multiplier *= 1.15f; break;
        case SEASON_WINTER: multiplier *= 0.9f; break;
    }
    
    // 宠物加成
    for (int i = 0; i < player.pet_count; i++) {
        if (player.pets[i].is_unlocked && player.pets[i].happiness > 50) {
            multiplier *= player.pets[i].bonus_multiplier;
        }
    }
    
    // 装备加成
    for (int i = 0; i < player.inventory_count; i++) {
        if (strcmp(player.inventory[i].name, "新手装备") == 0 && player.inventory[i].quantity > 0) {
            multiplier *= 1.05f;
            break;
        }
    }
    
    // 声望加成
    multiplier *= (1.0f + player.prestige_level * 0.05f);
    
    // 最终收入
    int income = (int)(base_income * multiplier);
    int exp_gain = task->experience_gain;
    
    // 双倍经验事件
    if (player.active_event == EVENT_DOUBLE_EXP) {
        exp_gain *= 2;
    }
    
    if (!success) {
        income = income / 2;
        print_colored(COLOR_RED, "\n?? 任务失败！只获得部分收入...\n");
    }
    
    // 执行任务
    player.energy -= actual_energy_cost;
    player.kun_coins += income;
    player.kun_exp += exp_gain;
    player.total_income += income;
    player.lifetime_task_completions++;
    player.daily_task_completions++;
    player.weekly_task_completions++;
    
    // 更新任务统计
    task->completion_count++;
    task->last_completed = time(NULL);
    
    print_colored(COLOR_GREEN, "\n? 完成任务：%s\n", task->name);
    printf("   获得收入：%d 坤币\n", income);
    printf("   获得经验：%d\n", exp_gain);
    printf("   消耗能量：%d (原%d)\n", actual_energy_cost, task->energy_cost);
    printf("   剩余能量：%d/%d\n", player.energy, player.max_energy);
    
    // 检查升级
    int exp_needed = player.level * 100;
    while (player.kun_exp >= exp_needed) {
        player.kun_exp -= exp_needed;
        player.level++;
        player.skill_points += 3;
        player.max_energy += 10;
        player.energy = player.max_energy;
        exp_needed = player.level * 100;
        
        print_colored(COLOR_YELLOW, "\n?? 等级提升！当前等级：%d\n", player.level);
        printf("   获得3点技能点\n");
        printf("   最大能量增加10点\n");
    }
    
    // 检查任务进度
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
    
    // 宠物状态更新
    for (int i = 0; i < player.pet_count; i++) {
        if (player.pets[i].is_unlocked) {
            player.pets[i].hunger -= 2;
            if (player.pets[i].hunger < 0) player.pets[i].hunger = 0;
            player.pets[i].happiness -= 1;
            if (player.pets[i].happiness < 0) player.pets[i].happiness = 0;
        }
    }
    
    check_achievements();
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
        player.kun_coins -= price;
        player.total_expenses += price;
        
        safe_strcpy(player.furniture[player.furniture_count], item->name, 50);
        player.furniture_count++;
        
        // 添加到背包
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
        
        // 检查每日任务
        for (int i = 0; i < MAX_QUESTS; i++) {
            if (!player.active_quests[i].completed && 
                strstr(player.active_quests[i].description, "购买家具") != NULL) {
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
    // 声望折扣
    cost = (int)(cost * (1.0f - player.prestige_level * 0.02f));
    if (cost < 10) cost = 10;
    
    if (player.kun_coins < cost) {
        print_colored(COLOR_RED, "? 升级需要 %d 坤币！\n", cost);
        return;
    }
    
    player.skill_points--;
    player.kun_coins -= cost;
    player.total_expenses += cost;
    skill->level++;
    
    print_colored(COLOR_GREEN, "\n?? 技能升级：%s 提升到等级 %d\n", skill->name, skill->level);
    printf("   花费：%d 坤币，1 技能点\n", cost);
    printf("   当前加成：%.0f%%\n", (pow(skill->multiplier, skill->level) - 1) * 100);
    
    // 检查每周任务
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "升级技能") != NULL) {
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
    
    // 查找宠物食品
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
    
    // 检查解锁条件
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
    
    if (player.kun_coins < unlock_cost) {
        print_colored(COLOR_RED, "? 需要 %lld 坤币！\n", unlock_cost);
        return;
    }
    
    player.kun_coins -= unlock_cost;
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
    player.kun_coins += quest->reward_coins;
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
    
    if (player.kun_coins < next_level->required_coins) {
        print_colored(COLOR_RED, "? 升级需要 %lld 坤币！\n", next_level->required_coins);
        return;
    }
    
    player.kun_coins -= next_level->required_coins;
    player.total_expenses += next_level->required_coins;
    player.current_company_level++;
    player.skill_points += next_level->unlock_skill_points;
    
    print_colored(COLOR_GREEN, "\n?? 公司升级：%s\n", next_level->name);
    printf("   花费：%lld 坤币\n", next_level->required_coins);
    printf("   获得 %d 技能点\n", next_level->unlock_skill_points);
    printf("   收入倍数：%.1f\n", next_level->income_multiplier);
    
    // 检查主线任务
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
    
    if (player.kun_coins < total_cost) {
        print_colored(COLOR_RED, "? 坤币不足！需要 %d 坤币\n", total_cost);
        return;
    }
    
    player.kun_coins -= total_cost;
    player.total_expenses += total_cost;
    player.invested_stocks[stock_index] += amount;
    
    print_colored(COLOR_GREEN, "\n?? 购买股票：%s × %d\n", stock->name, amount);
    printf("   单价：%d 坤币\n", stock->current_price);
    printf("   总价：%d 坤币\n", total_cost);
    
    // 检查每日任务
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "投资股票") != NULL) {
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
    
    player.kun_coins += total_value;
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
    
    if (player.kun_coins < prop->coins_needed) {
        print_colored(COLOR_RED, "? 坤币不足！需要 %lld 坤币\n", prop->coins_needed);
        return;
    }
    
    player.kun_coins -= prop->coins_needed;
    player.total_expenses += prop->coins_needed;
    
    // 添加到已拥有房产
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
        player.kun_coins += prize;
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
        player.kun_coins += coins;
        player.total_income += coins;
        print_colored(COLOR_GREEN, "\n?? 开启神秘宝箱获得：%d 坤币\n", coins);
    } else if (roll < 80) {
        int coins = 5000 + rand() % 10000;
        player.kun_coins += coins;
        player.total_income += coins;
        print_colored(COLOR_BLUE, "\n?? 开启神秘宝箱获得：%d 坤币（稀有）\n", coins);
    } else if (roll < 95) {
        int coins = 20000 + rand() % 30000;
        player.kun_coins += coins;
        player.total_income += coins;
        player.skill_points += 3;
        print_colored(COLOR_MAGENTA, "\n?? 开启神秘宝箱获得：%d 坤币和3技能点（史诗）\n", coins);
    } else {
        int coins = 100000;
        player.kun_coins += coins;
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
    
    // 计算战斗力
    float power = 0;
    for (int i = 0; i < MAX_SKILLS; i++) {
        power += player.skills[i].level * 10;
    }
    power += player.level * 5;
    power *= (1.0f + player.prestige_level * 0.1f);
    
    // BOSS强度
    float boss_hp = 1000 + player.boss_defeats * 200;
    
    if (power >= boss_hp) {
        int reward = 10000 + rand() % 20000;
        reward = (int)(reward * (1.0f + player.prestige_level * 0.2f));
        player.kun_coins += reward;
        player.total_income += reward;
        player.kun_exp += 500;
        player.boss_defeats++;
        
        print_colored(COLOR_YELLOW, "\n?? 挑战BOSS成功！获得 %d 坤币，500经验！\n", reward);
        
        // 检查任务
        for (int i = 0; i < MAX_QUESTS; i++) {
            if (!player.active_quests[i].completed && 
                strstr(player.active_quests[i].description, "击败BOSS") != NULL) {
                player.active_quests[i].progress++;
                if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                    complete_quest(i);
                }
            }
        }
        
        // 结束BOSS事件
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
    player.kun_coins += reward;
    player.total_income += reward;
    player.festival_participations++;
    
    print_colored(COLOR_YELLOW, "\n?? 参与节日庆典获得 %d 坤币！\n", reward);
    
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "节日参与") != NULL) {
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
    print_colored(COLOR_CYAN, "\n?? 季节变换：现在是%s\n", season_names[player.current_season]);
    
    switch (player.current_season) {
        case SEASON_SPRING:
            printf("   春季加成：所有任务收入增加10%%\n");
            break;
        case SEASON_SUMMER:
            printf("   夏季加成：任务能量消耗减少5%%\n");
            break;
        case SEASON_AUTUMN:
            printf("   秋季加成：投资回报增加15%%\n");
            break;
        case SEASON_WINTER:
            printf("   冬季影响：任务收入减少10%%\n");
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
                // 添加能量大补丸
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
                // 添加高级宠物粮
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
    
    if (amount > player.kun_coins) {
        print_colored(COLOR_RED, "? 坤币不足！\n");
        return;
    }
    
    player.kun_coins -= amount;
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
    
    // 保存永久数据
    int new_prestige_level = player.prestige_level + 1;
    int prestige_points = player.prestige_points + 100 * new_prestige_level;
    
    // 重置玩家数据（保留声望和成就）
    Achievement saved_achievements[MAX_ACHIEVEMENTS];
    memcpy(saved_achievements, player.achievements, sizeof(Achievement) * MAX_ACHIEVEMENTS);
    int saved_achievement_points = player.achievement_points;
    
    init_player();
    
    // 恢复声望和成就数据
    player.prestige_level = new_prestige_level;
    player.prestige_points = prestige_points;
    memcpy(player.achievements, saved_achievements, sizeof(Achievement) * MAX_ACHIEVEMENTS);
    player.achievement_points = saved_achievement_points;
    player.kun_coins = 1000;
    player.skill_points += 5 * new_prestige_level;
    
    // 解锁声望成就
    if (!player.achievements[15].unlocked) {
        unlock_achievement(15);
    }
    
    print_colored(COLOR_YELLOW, "\n?? 声望重置成功！\n");
    printf("   声望等级：%d\n", player.prestige_level);
    printf("   声望点数：%d\n", player.prestige_points);
    printf("   所有收入增加 %d%%\n", player.prestige_level * 5);
    
    // 检查声望任务
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
    if (duration_days > 365) cost = 100000; // 永久会员
    
    if (player.kun_coins < cost) {
        print_colored(COLOR_RED, "? 坤币不足！需要 %d\n", cost);
        return;
    }
    
    player.kun_coins -= cost;
    player.total_expenses += cost;
    player.has_premium = true;
    
    if (duration_days <= 365) {
        player.premium_until = time(NULL) + duration_days * 86400;
    } else {
        player.premium_until = time(NULL) + (time_t)36500 * 86400; // 100年
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
            printf("\n打工模拟器 v3.0 - 完整版\n");
            printf("作者：Ciallo0721-cmd\n");
            printf("编译日期：%s %s\n", __DATE__, __TIME__);
            printf("感谢您的支持！\n");
            break;
            //这sbdeep seek,让他改个片段,他给作者改了
        case 3:
            printf("?? 确定重置所有进度？(y/n): ");
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
                print_colored(COLOR_GREEN, "? 游戏进度已重置！\n");
            }
            break;
        case 4:
            printf("?? 确定删除存档文件？(y/n): ");
            confirm = _getch();
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
    printf("通过打工赚钱，购买物品，升级技能，成为最富有的打工皇帝！\n\n");
    
    printf(COLOR_BOLD "?? 核心玩法\n" COLOR_RESET);
    printf("1. 打工赚钱：完成各种任务获得坤币和经验\n");
    printf("2. 商城购物：购买家具和物品提升生活质量\n");
    printf("3. 技能升级：提升技能增加任务收入\n");
    printf("4. 成就系统：完成特定目标获得奖励\n");
    printf("5. 随机事件：游戏中的各种惊喜和挑战\n\n");
    
    printf(COLOR_BOLD "? 能量系统\n" COLOR_RESET);
    printf("? 每个任务消耗能量\n");
    printf("? 能量每10分钟恢复1点\n");
    printf("? 使用能量饮料可快速恢复\n\n");
    
    printf(COLOR_BOLD "? 特色功能\n" COLOR_RESET);
    printf("? 季节系统：不同季节有不同加成\n");
    printf("? 宠物系统：宠物提供额外加成\n");
    printf("? 公司经营：升级公司获得被动收入\n");
    printf("? 股票市场：投资股票获得收益\n");
    printf("? 房产投资：购买房产获得稳定收入\n");
    printf("? 声望系统：重置游戏获得永久加成\n");
    printf("? 合成系统：合成高级物品\n");
    printf("? 好友系统：与好友互动\n");
    printf("? 会员系统：购买会员享特权\n\n");
    
    draw_separator();
}

void show_status() {
    CLEAR_SCREEN();
    draw_box("?? 玩家状态");
    
    printf(COLOR_BOLD "?? 基本信息\n" COLOR_RESET);
    printf("   等级：%d\n", player.level);
    printf("   经验：%d/%d\n", player.kun_exp, player.level * 100);
    printf("   坤币：%lld\n", player.kun_coins);
    printf("   能量：%d/%d\n", player.energy, player.max_energy);
    printf("   技能点：%d\n", player.skill_points);
    printf("   成就点：%d\n", player.achievement_points);
    
    printf(COLOR_BOLD "\n?? 游戏进度\n" COLOR_RESET);
    printf("   连续登录：%d 天\n", player.consecutive_days);
    printf("   游戏天数：%d 天\n", player.day_count);
    printf("   总任务完成：%d 次\n", player.lifetime_task_completions);
    printf("   今日任务：%d\n", player.daily_task_completions);
    printf("   公司等级：%s\n", player.company_levels[player.current_company_level].name);
    printf("   声望等级：%d\n", player.prestige_level);
    
    printf(COLOR_BOLD "\n?? 资产统计\n" COLOR_RESET);
    printf("   总收入：%lld 坤币\n", player.total_income);
    printf("   总支出：%lld 坤币\n", player.total_expenses);
    printf("   净资产：%lld 坤币\n", player.total_income - player.total_expenses);
    printf("   家具数量：%d/%d\n", player.furniture_count, MAX_FURNITURE);
    printf("   宠物数量：%d/%d\n", player.pet_count, MAX_PETS);
    printf("   房产数量：%d/10\n", player.property_count);
    
    printf(COLOR_BOLD "\n?? 当前季节：" COLOR_RESET);
    switch (player.current_season) {
        case SEASON_SPRING: printf("春季 ??\n"); break;
        case SEASON_SUMMER: printf("夏季 ??\n"); break;
        case SEASON_AUTUMN: printf("秋季 ??\n"); break;
        case SEASON_WINTER: printf("冬季 ??\n"); break;
    }
    
    if (player.has_premium) {
        time_t remaining = player.premium_until - time(NULL);
        if (remaining > 0) {
            printf(COLOR_YELLOW "   ?? 会员剩余：%ld 天\n" COLOR_RESET, remaining / 86400 + 1);
        } else {
            player.has_premium = false;
        }
    }
    
    show_active_events();
    draw_separator();
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
        press_any_key();
    }
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
        item->quantity++; // 不减，因为没使用
        return;
    } else if (strcmp(item->name, "高级宠物粮") == 0) {
        print_colored(COLOR_GREEN, "?? 请在宠物界面喂食\n");
        item->quantity++;
        return;
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
            printf(COLOR_CYAN "?? %s\n" COLOR_RESET, quest->name);
            printf("   %s\n", quest->description);
            printf("   进度：%d/%d\n", quest->progress, quest->requirement);
            
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
    
    // 计算被动收入
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
    draw_box("? 声望系统");
    
    printf("当前声望等级：%d\n", player.prestige_level);
    printf("声望点数：%d\n\n", player.prestige_points);
    
    if (player.prestige_level == 0) {
        printf("?? 首次声望重置奖励：\n");
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
            print_colored(COLOR_GREEN, "\n? 满足声望重置条件！\n");
        } else {
            print_colored(COLOR_RED, "\n? 不满足声望重置条件\n");
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
            draw_box("?? 声望商店");
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
                    player.premium_until = time(NULL) + (time_t)36500 * 86400;
                    print_colored(COLOR_GREEN, "? 会员已延长至永久！\n");
                }
            } else if (shop_choice == 2 && player.prestige_points >= 200) {
                player.prestige_points -= 200;
                player.kun_coins += 500;
                print_colored(COLOR_GREEN, "? 获得500坤币开局资金加成！\n");
            } else if (shop_choice == 3 && player.prestige_points >= 300) {
                player.prestige_points -= 300;
                player.skill_points += 10;
                print_colored(COLOR_GREEN, "? 获得10技能点！\n");
            } else if (shop_choice > 0) {
                print_colored(COLOR_RED, "? 声望点数不足！\n");
            }
        }
    }
    press_any_key();
}

void show_main_menu() {
    CLEAR_SCREEN();
    printf(COLOR_YELLOW COLOR_BOLD);
    printf("  ╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                                            ║\n");
    printf("  ║                         打工模拟器 v3.0 - 完整版                           ║\n");
    printf("  ║                                                                            ║\n");
    printf("  ╚════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    
    printf(COLOR_BOLD "?? 玩家状态：" COLOR_RESET);
    printf("等级 %d | 坤币 %lld | 能量 %d/%d | 声望 %d\n\n", 
           player.level, player.kun_coins, player.energy, player.max_energy, player.prestige_level);
    
    draw_separator();
    printf(COLOR_BOLD "?? 主菜单\n" COLOR_RESET);
    printf("┌─────────────────────┬─────────────────────┬─────────────────────┐\n");
    printf("│ 1. ?? 查看状态      │ 2. ?? 打工赚钱      │ 3. ?? 商城购物      │\n");
    printf("│ 4. ?? 技能系统      │ 5. ?? 背包物品      │ 6. ?? 任务列表      │\n");
    printf("│ 7. ?? 宠物系统      │ 8. ?? 公司管理      │ 9. ?? 股票市场      │\n");
    printf("│10. ?? 房产投资      │11. ?? 成就系统      │12. ? 声望系统      │\n");
    printf("│13. ?? 好友系统      │14. ?? 游戏统计      │15. ?? 会员商城      │\n");
    printf("│16. ?? 游戏设置      │17. ?? 帮助指南      │18. ?? 保存游戏      │\n");
    printf("│                     │ 0. ?? 退出游戏      │                     │\n");
    printf("└─────────────────────┴─────────────────────┴─────────────────────┘\n");
    draw_separator();
    
    show_active_events();
}

//================ 主函数 ================
int main() {
    enable_ansi();
    show_loading_screen(2000);
    
    init_game();
    
    time_t last_passive_time = time(NULL);
    time_t last_save_time = time(NULL);
    
    while (1) {
        show_main_menu();
        
        printf("请输入选项: ");
        int choice = get_valid_input(0, 18);
        
        // 自动保存和被动收入
        time_t now = time(NULL);
        if (now - last_passive_time >= 3600) {
            calculate_passive_income();
            last_passive_time = now;
        }
        if (now - last_save_time >= 300) {  // 每5分钟自动保存
            save_game();
            last_save_time = now;
        }
        
        // 能量恢复
        recover_energy();
        
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
                        printf("%d. %s\n", i + 1, task->name);
                        printf("   收入：%d 坤币\n", task->base_income);
                        printf("   经验：%d\n", task->experience_gain);
                        printf("   能量：%d\n", task->energy_cost);
                        printf("   成功率：%.0f%%\n", task->success_rate * 100);
                        printf("   描述：%s\n", task->description);
                        if (task->completion_count > 0) {
                            printf("   完成次数：%d\n", task->completion_count);
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
                CLEAR_SCREEN();
                draw_box("?? 商城");
                
                printf("可用物品：\n\n");
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
                if (item_choice > 0) {
                    buy_item(item_choice - 1);
                }
                press_any_key();
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
            case 0:
                CLEAR_SCREEN();
                printf(COLOR_YELLOW "\n");
                printf("  ╔════════════════════════════════════════════════════════════╗\n");
                printf("  ║                                                                ║\n");
                printf("  ║                    感谢游玩打工模拟器 v3.0！                   ║\n");
                printf("  ║                                                                ║\n");
                printf("  ║                      记得常回来打工哦！                         ║\n");
                printf("  ║                                                                ║\n");
                printf("  ╚════════════════════════════════════════════════════════════╝\n");
                printf(COLOR_RESET "\n");
                save_game();
                return 0;
        }
        
        // 随机事件
        random_event();
        
        // 更新游戏时间
        static time_t last_update = 0;
        if (last_update == 0) last_update = now;
        player.total_play_time += (int)(now - last_update);
        last_update = now;
    }
    
    return 0;
}
