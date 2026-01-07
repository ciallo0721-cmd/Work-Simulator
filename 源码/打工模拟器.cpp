#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <conio.h>
#include <stdarg.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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
#define SAVE_FILE "kun_save.dat"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_UNDERLINE "\033[4m"
#define CLEAR_SCREEN() printf("\033[2J\033[H")

//================ 类型定义 ================
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
    int reward;
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
    int reward_coins;
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
    long long required_coins;
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
    long long coins_needed;
    int days_needed;
    float passive_income;
    int unlock_achievement_id;
} Property;

typedef struct {
    char username[30];
    int level;
    int coins;
    bool is_online;
    time_t last_online;
    float friendship_level;
} Friend;

typedef struct {
    char name[50];
    int id;
    int required_level;
    int base_income;
    int experience_gain;
    int energy_cost;
    int unlock_cost;
    float time_required; //小时
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

typedef struct {
    int kun_coins;
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
    int total_income;
    int total_expenses;
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
} Player;

//================ 全局变量 ================
Player player;
Task tasks[MAX_TASKS];
MarketItem market[MAX_MARKET];
Stock stock_market[5];
Property available_properties[10];

//================ 工具函数声明 ================
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
void animate_text(const char* text, int delay_ms);
void print_ascii_art(const char* art_name);

//================ 游戏系统函数声明 ================
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
void load_game();
void save_game();
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

//================ 游戏逻辑函数声明 ================
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

//================ 事件系统函数声明 ================
void random_event();
void trigger_event(EventType type);
void end_event(EventType type);
void handle_event_effect(EventType type, bool apply);
const char* get_event_name(EventType type);
const char* get_event_description(EventType type);
void show_active_events();

//================ 成就系统函数声明 ================
void check_achievements();
void unlock_achievement(int achievement_id);
void show_achievements();
void show_achievement_progress();

//================ 工具函数 ================
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
    int padding = (width - strlen(text)) / 2;
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
    while (1) {
        printf("请输入选项 (%d-%d): ", min, max);
        if (scanf("%d", &input) != 1) {
            clear_input_buffer();
            printf("输入无效，请输入数字！\n");
            continue;
        }
        if (input < min || input > max) {
            printf("请输入%d到%d之间的数字！\n", min, max);
            continue;
        }
        clear_input_buffer();
        return input;
    }
}

int get_valid_input_with_cancel(int min, int max, int cancel_code) {
    int input;
    while (1) {
        printf("请输入选项 (%d-%d, %d取消): ", min, max, cancel_code);
        if (scanf("%d", &input) != 1) {
            clear_input_buffer();
            printf("输入无效，请输入数字！\n");
            continue;
        }
        if (input == cancel_code) {
            clear_input_buffer();
            return cancel_code;
        }
        if (input < min || input > max) {
            printf("请输入%d到%d之间的数字！\n", min, max);
            continue;
        }
        clear_input_buffer();
        return input;
    }
}

void press_any_key() {
    printf("\n" COLOR_GREEN "按任意键继续..." COLOR_RESET);
    #ifdef _WIN32
    _getch();
    #else
    system("read -n1");
    #endif
}

void show_loading_screen(int duration) {
    CLEAR_SCREEN();
    print_centered("打工模拟器 v2.0");
    printf("\n\n");
    print_centered("加载中...");
    printf("\n");
    
    for (int i = 0; i < 50; i++) {
        printf(COLOR_GREEN "▓" COLOR_RESET);
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
    printf("] %d/%d\n", current, max);
}

void animate_text(const char* text, int delay_ms) {
    for (int i = 0; text[i] != '\0'; i++) {
        putchar(text[i]);
        fflush(stdout);
        #ifdef _WIN32
        Sleep(delay_ms);
        #else
        usleep(delay_ms * 1000);
        #endif
    }
    printf("\n");
}

//================ 初始化函数 ================
void init_game() {
    srand(time(NULL));
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
    
    //尝试加载存档
    load_game();
    
    //更新游戏状态
    time_t now = time(NULL);
    if (player.last_login == 0) {
        player.last_login = now;
        player.game_start_time = now;
    }
    
    //检查每日登录奖励
    struct tm *last_login_tm = localtime(&player.last_login);
    struct tm *current_tm = localtime(&now);
    
    if (current_tm->tm_yday != last_login_tm->tm_yday ||
        current_tm->tm_year != last_login_tm->tm_year) {
        player.consecutive_days++;
        player.kun_coins += 100 * player.consecutive_days;
        player.kun_exp += 50 * player.consecutive_days;
        player.daily_task_completions = 0;
        
        print_colored(COLOR_YELLOW, "\n?? 每日登录奖励：获得 %d 坤币和 %d 经验！\n", 
                    100 * player.consecutive_days, 50 * player.consecutive_days);
        
        if (player.consecutive_days % 7 == 0) {
            player.kun_coins += 500;
            player.lottery_tickets++;
            print_colored(COLOR_MAGENTA, "?? 连续登录 %d 天奖励：额外获得 500 坤币和 1 张彩票！\n", player.consecutive_days);
        }
    }
    
    player.last_login = now;
    player.day_count++;
    
    //每30天更换季节
    if (player.day_count % 30 == 0) {
        change_season();
    }
}

void init_player() {
    memset(&player, 0, sizeof(Player));
    player.level = 1;
    player.energy = 100;
    player.max_energy = 100;
    player.consecutive_days = 0;
    player.current_company_level = 0;
    player.inventory_count = 0;
    player.property_count = 0;
    player.friend_count = 0;
    player.pet_count = 0;
    player.prestige_level = 0;
    player.prestige_points = 0;
    player.has_premium = false;
    player.current_season = SEASON_SPRING;
    player.lottery_tickets = 0;
    player.mystery_keys = 0;
    player.boss_defeats = 0;
    player.festival_participations = 0;
    player.achievement_points = 0;
    
    //初始家具
    strcpy(player.furniture[0], "简易床");
    strcpy(player.furniture[1], "破旧椅子");
    player.furniture_count = 2;
    
    //初始物品
    InventoryItem starter_items[] = {
        {"能量饮料", 0, 10, RARITY_COMMON, true, 20, "恢复20点能量", 3},
        {"新手装备", 0, 50, RARITY_UNCOMMON, false, 5, "增加5%任务收入", 1},
        {"彩票", 0, 5, RARITY_COMMON, true, 0, "有机会赢取大奖", 1}
    };
    
    for (int i = 0; i < 3; i++) {
        player.inventory[player.inventory_count++] = starter_items[i];
    }
}

void init_tasks() {
    //初始化任务数组
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
    
    //基础任务
    strcpy(tasks[0].name, "出租车司机");
    tasks[0].required_level = 1;
    tasks[0].base_income = 50;
    tasks[0].experience_gain = 10;
    tasks[0].energy_cost = 5;
    tasks[0].unlock_cost = 0;
    tasks[0].is_unlocked = true;
    strcpy(tasks[0].description, "驾驶出租车接送客人");
    tasks[0].success_rate = 0.9f;
    tasks[0].required_skills[0] = SKILL_SPEED;
    
    strcpy(tasks[1].name, "伐木工人");
    tasks[1].required_level = 2;
    tasks[1].base_income = 100;
    tasks[1].experience_gain = 20;
    tasks[1].energy_cost = 10;
    tasks[1].unlock_cost = 500;
    strcpy(tasks[1].description, "在森林中砍伐树木");
    tasks[1].success_rate = 0.8f;
    tasks[1].required_skills[0] = SKILL_STRENGTH;
    
    strcpy(tasks[2].name, "外卖配送");
    tasks[2].required_level = 3;
    tasks[2].base_income = 200;
    tasks[2].experience_gain = 30;
    tasks[2].energy_cost = 15;
    tasks[2].unlock_cost = 2000;
    strcpy(tasks[2].description, "配送外卖订单");
    tasks[2].success_rate = 0.85f;
    tasks[2].required_skills[0] = SKILL_SPEED;
    tasks[2].required_skills[1] = SKILL_ENDURANCE;
    
    //中级任务
    strcpy(tasks[3].name, "餐厅服务生");
    tasks[3].required_level = 5;
    tasks[3].base_income = 300;
    tasks[3].experience_gain = 40;
    tasks[3].energy_cost = 20;
    tasks[3].unlock_cost = 5000;
    strcpy(tasks[3].description, "在高级餐厅服务");
    tasks[3].success_rate = 0.75f;
    tasks[3].required_skills[0] = SKILL_CHARISMA;
    tasks[3].required_skills[1] = SKILL_SPEED;
    
    strcpy(tasks[4].name, "程序员");
    tasks[4].required_level = 8;
    tasks[4].base_income = 500;
    tasks[4].experience_gain = 60;
    tasks[4].energy_cost = 25;
    tasks[4].unlock_cost = 10000;
    strcpy(tasks[4].description, "编写软件代码");
    tasks[4].success_rate = 0.7f;
    tasks[4].required_skills[0] = SKILL_INTELLIGENCE;
    tasks[4].required_skills[1] = SKILL_CREATIVITY;
    
    strcpy(tasks[5].name, "股票交易员");
    tasks[5].required_level = 10;
    tasks[5].base_income = 800;
    tasks[5].experience_gain = 80;
    tasks[5].energy_cost = 30;
    tasks[5].unlock_cost = 20000;
    strcpy(tasks[5].description, "进行股票交易");
    tasks[5].success_rate = 0.6f;
    tasks[5].required_skills[0] = SKILL_INTELLIGENCE;
    tasks[5].required_skills[1] = SKILL_LUCK;
    tasks[5].required_skills[2] = SKILL_NEGOTIATION;
    
    //高级任务
    strcpy(tasks[6].name, "房地产经纪人");
    tasks[6].required_level = 15;
    tasks[6].base_income = 1200;
    tasks[6].experience_gain = 100;
    tasks[6].energy_cost = 35;
    tasks[6].unlock_cost = 50000;
    strcpy(tasks[6].description, "销售高端房产");
    tasks[6].success_rate = 0.65f;
    tasks[6].required_skills[0] = SKILL_CHARISMA;
    tasks[6].required_skills[1] = SKILL_NEGOTIATION;
    tasks[6].required_skills[2] = SKILL_MANAGEMENT;
    
    strcpy(tasks[7].name, "企业家");
    tasks[7].required_level = 20;
    tasks[7].base_income = 2000;
    tasks[7].experience_gain = 150;
    tasks[7].energy_cost = 40;
    tasks[7].unlock_cost = 100000;
    strcpy(tasks[7].description, "创办自己的企业");
    tasks[7].success_rate = 0.55f;
    tasks[7].required_skills[0] = SKILL_LEADERSHIP;
    tasks[7].required_skills[1] = SKILL_MANAGEMENT;
    tasks[7].required_skills[2] = SKILL_INTELLIGENCE;
    
    strcpy(tasks[8].name, "投资者");
    tasks[8].required_level = 25;
    tasks[8].base_income = 3000;
    tasks[8].experience_gain = 200;
    tasks[8].energy_cost = 45;
    tasks[8].unlock_cost = 200000;
    strcpy(tasks[8].description, "投资创业公司");
    tasks[8].success_rate = 0.5f;
    tasks[8].required_skills[0] = SKILL_INTELLIGENCE;
    tasks[8].required_skills[1] = SKILL_LUCK;
    tasks[8].required_skills[2] = SKILL_NEGOTIATION;
    
    //特殊任务
    strcpy(tasks[9].name, "神秘探险");
    tasks[9].required_level = 30;
    tasks[9].base_income = 5000;
    tasks[9].experience_gain = 300;
    tasks[9].energy_cost = 50;
    tasks[9].unlock_cost = 500000;
    strcpy(tasks[9].description, "探索未知领域");
    tasks[9].success_rate = 0.4f;
    tasks[9].required_skills[0] = SKILL_ENDURANCE;
    tasks[9].required_skills[1] = SKILL_LUCK;
    tasks[9].required_skills[2] = SKILL_STRENGTH;
    
    //设置技能等级要求
    tasks[1].required_skill_level[0] = 1;
    tasks[2].required_skill_level[0] = 1;
    tasks[3].required_skill_level[0] = 2;
    tasks[3].required_skill_level[1] = 1;
    tasks[4].required_skill_level[0] = 3;
    tasks[4].required_skill_level[1] = 2;
    tasks[5].required_skill_level[0] = 4;
    tasks[5].required_skill_level[1] = 3;
    tasks[5].required_skill_level[2] = 2;
    tasks[6].required_skill_level[0] = 5;
    tasks[6].required_skill_level[1] = 4;
    tasks[6].required_skill_level[2] = 3;
    tasks[7].required_skill_level[0] = 6;
    tasks[7].required_skill_level[1] = 5;
    tasks[7].required_skill_level[2] = 4;
    tasks[8].required_skill_level[0] = 7;
    tasks[8].required_skill_level[1] = 6;
    tasks[8].required_skill_level[2] = 5;
    tasks[9].required_skill_level[0] = 8;
    tasks[9].required_skill_level[1] = 7;
    tasks[9].required_skill_level[2] = 6;
    
    //解锁初始任务
    for (int i = 0; i < 3; i++) {
        player.tasks_unlocked[i] = true;
    }
}

void init_market() {
    //家具类
    strcpy(market[0].name, "电视");
    market[0].price = 300;
    market[0].value = 50;
    market[0].rarity = RARITY_COMMON;
    market[0].is_limited = false;
    market[0].available_until = 0;
    strcpy(market[0].description, "基本的娱乐设备");
    market[0].passive_income = 0.1f;
    market[0].required_level = 1;
    market[0].boost_multiplier = 1.0f;
    
    strcpy(market[1].name, "椅子");
    market[1].price = 100;
    market[1].value = 20;
    market[1].rarity = RARITY_COMMON;
    market[1].is_limited = false;
    market[1].available_until = 0;
    strcpy(market[1].description, "舒适的座椅");
    market[1].passive_income = 0.05f;
    market[1].required_level = 1;
    market[1].boost_multiplier = 1.0f;
    
    strcpy(market[2].name, "沙发");
    market[2].price = 500;
    market[2].value = 100;
    market[2].rarity = RARITY_UNCOMMON;
    market[2].is_limited = false;
    market[2].available_until = 0;
    strcpy(market[2].description, "豪华沙发");
    market[2].passive_income = 0.2f;
    market[2].required_level = 3;
    market[2].boost_multiplier = 1.1f;
    
    strcpy(market[3].name, "游泳池");
    market[3].price = 2000;
    market[3].value = 500;
    market[3].rarity = RARITY_RARE;
    market[3].is_limited = false;
    market[3].available_until = 0;
    strcpy(market[3].description, "私人游泳池");
    market[3].passive_income = 0.5f;
    market[3].required_level = 10;
    market[3].boost_multiplier = 1.2f;
    
    strcpy(market[4].name, "黄金马桶");
    market[4].price = 10000;
    market[4].value = 2000;
    market[4].rarity = RARITY_EPIC;
    market[4].is_limited = false;
    market[4].available_until = 0;
    strcpy(market[4].description, "奢华的马桶");
    market[4].passive_income = 1.0f;
    market[4].required_level = 20;
    market[4].boost_multiplier = 1.3f;
    
    //装饰类
    strcpy(market[5].name, "名画");
    market[5].price = 1500;
    market[5].value = 300;
    market[5].rarity = RARITY_RARE;
    market[5].is_limited = false;
    market[5].available_until = 0;
    strcpy(market[5].description, "著名画作");
    market[5].passive_income = 0.3f;
    market[5].required_level = 5;
    market[5].boost_multiplier = 1.15f;
    
    strcpy(market[6].name, "水晶吊灯");
    market[6].price = 3000;
    market[6].value = 800;
    market[6].rarity = RARITY_EPIC;
    market[6].is_limited = false;
    market[6].available_until = 0;
    strcpy(market[6].description, "华丽的水晶灯");
    market[6].passive_income = 0.6f;
    market[6].required_level = 15;
    market[6].boost_multiplier = 1.25f;
    
    strcpy(market[7].name, "家庭影院");
    market[7].price = 5000;
    market[7].value = 1500;
    market[7].rarity = RARITY_EPIC;
    market[7].is_limited = false;
    market[7].available_until = 0;
    strcpy(market[7].description, "顶级影音系统");
    market[7].passive_income = 0.8f;
    market[7].required_level = 18;
    market[7].boost_multiplier = 1.3f;
    
    //功能类
    strcpy(market[8].name, "工作台");
    market[8].price = 800;
    market[8].value = 200;
    market[8].rarity = RARITY_UNCOMMON;
    market[8].is_limited = false;
    market[8].available_until = 0;
    strcpy(market[8].description, "提升工作效率");
    market[8].passive_income = 0.25f;
    market[8].required_level = 5;
    market[8].boost_multiplier = 1.1f;
    
    strcpy(market[9].name, "健身房");
    market[9].price = 2500;
    market[9].value = 600;
    market[9].rarity = RARITY_RARE;
    market[9].is_limited = false;
    market[9].available_until = 0;
    strcpy(market[9].description, "锻炼身体");
    market[9].passive_income = 0.4f;
    market[9].required_level = 12;
    market[9].boost_multiplier = 1.2f;
    
    //限定物品
    strcpy(market[10].name, "圣诞树");
    market[10].price = 1200;
    market[10].value = 400;
    market[10].rarity = RARITY_RARE;
    market[10].is_limited = true;
    market[10].available_until = time(NULL) + 2592000;
    strcpy(market[10].description, "节日限定装饰");
    market[10].passive_income = 0.35f;
    market[10].required_level = 8;
    market[10].boost_multiplier = 1.15f;
    
    strcpy(market[11].name, "龙年雕塑");
    market[11].price = 3000;
    market[11].value = 1000;
    market[11].rarity = RARITY_EPIC;
    market[11].is_limited = true;
    market[11].available_until = time(NULL) + 31536000;
    strcpy(market[11].description, "年度限定物品");
    market[11].passive_income = 0.7f;
    market[11].required_level = 25;
    market[11].boost_multiplier = 1.3f;
    
    //投资类
    strcpy(market[12].name, "股票交易终端");
    market[12].price = 5000;
    market[12].value = 1000;
    market[12].rarity = RARITY_RARE;
    market[12].is_limited = false;
    market[12].available_until = 0;
    strcpy(market[12].description, "提升股票收益");
    market[12].passive_income = 0.5f;
    market[12].required_level = 20;
    market[12].boost_multiplier = 1.25f;
    
    strcpy(market[13].name, "加密货币矿机");
    market[13].price = 8000;
    market[13].value = 2000;
    market[13].rarity = RARITY_EPIC;
    market[13].is_limited = false;
    market[13].available_until = 0;
    strcpy(market[13].description, "挖取数字货币");
    market[13].passive_income = 1.0f;
    market[13].required_level = 30;
    market[13].boost_multiplier = 1.4f;
    
    //收藏品类
    strcpy(market[14].name, "古董花瓶");
    market[14].price = 4000;
    market[14].value = 1200;
    market[14].rarity = RARITY_EPIC;
    market[14].is_limited = false;
    market[14].available_until = 0;
    strcpy(market[14].description, "珍贵的古董");
    market[14].passive_income = 0.6f;
    market[14].required_level = 25;
    market[14].boost_multiplier = 1.25f;
    
    strcpy(market[15].name, "限量版球鞋");
    market[15].price = 2000;
    market[15].value = 600;
    market[15].rarity = RARITY_RARE;
    market[15].is_limited = false;
    market[15].available_until = 0;
    strcpy(market[15].description, "潮流通货");
    market[15].passive_income = 0.3f;
    market[15].required_level = 15;
    market[15].boost_multiplier = 1.15f;
}

void init_achievements() {
    //基础成就
    strcpy(player.achievements[0].name, "第一桶金");
    strcpy(player.achievements[0].description, "赚取100坤币");
    player.achievements[0].condition = 100;
    player.achievements[0].reward = 50;
    player.achievements[0].reward_exp = 10;
    player.achievements[0].unlocked = false;
    
    strcpy(player.achievements[1].name, "家具收藏家");
    strcpy(player.achievements[1].description, "收集5件家具");
    player.achievements[1].condition = 5;
    player.achievements[1].reward = 200;
    player.achievements[1].reward_exp = 20;
    player.achievements[1].unlocked = false;
    
    strcpy(player.achievements[2].name, "打工达人");
    strcpy(player.achievements[2].description, "完成50次任务");
    player.achievements[2].condition = 50;
    player.achievements[2].reward = 500;
    player.achievements[2].reward_exp = 50;
    player.achievements[2].unlocked = false;
    
    strcpy(player.achievements[3].name, "股市之神");
    strcpy(player.achievements[3].description, "股票投资获利1000坤币");
    player.achievements[3].condition = 1000;
    player.achievements[3].reward = 800;
    player.achievements[3].reward_exp = 80;
    player.achievements[3].unlocked = false;
    
    strcpy(player.achievements[4].name, "幸运儿");
    strcpy(player.achievements[4].description, "赢得彩票大奖");
    player.achievements[4].condition = 1;
    player.achievements[4].reward = 1000;
    player.achievements[4].reward_exp = 100;
    player.achievements[4].unlocked = false;
    
    //进度成就
    strcpy(player.achievements[5].name, "百万富翁");
    strcpy(player.achievements[5].description, "拥有100万坤币");
    player.achievements[5].condition = 1000000;
    player.achievements[5].reward = 5000;
    player.achievements[5].reward_exp = 500;
    player.achievements[5].unlocked = false;
    
    strcpy(player.achievements[6].name, "等级大师");
    strcpy(player.achievements[6].description, "达到50级");
    player.achievements[6].condition = 50;
    player.achievements[6].reward = 2000;
    player.achievements[6].reward_exp = 200;
    player.achievements[6].unlocked = false;
    
    strcpy(player.achievements[7].name, "技能专家");
    strcpy(player.achievements[7].description, "将一项技能升到10级");
    player.achievements[7].condition = 10;
    player.achievements[7].reward = 1500;
    player.achievements[7].reward_exp = 150;
    player.achievements[7].unlocked = false;
    
    strcpy(player.achievements[8].name, "任务狂人");
    strcpy(player.achievements[8].description, "完成1000次任务");
    player.achievements[8].condition = 1000;
    player.achievements[8].reward = 3000;
    player.achievements[8].reward_exp = 300;
    player.achievements[8].unlocked = false;
    
    strcpy(player.achievements[9].name, "社交达人");
    strcpy(player.achievements[9].description, "拥有5个好友");
    player.achievements[9].condition = 5;
    player.achievements[9].reward = 1000;
    player.achievements[9].reward_exp = 100;
    player.achievements[9].unlocked = false;
    
    //收集成就
    strcpy(player.achievements[10].name, "宠物爱好者");
    strcpy(player.achievements[10].description, "拥有3只宠物");
    player.achievements[10].condition = 3;
    player.achievements[10].reward = 1500;
    player.achievements[10].reward_exp = 150;
    player.achievements[10].unlocked = false;
    
    strcpy(player.achievements[11].name, "房产大亨");
    strcpy(player.achievements[11].description, "拥有5处房产");
    player.achievements[11].condition = 5;
    player.achievements[11].reward = 4000;
    player.achievements[11].reward_exp = 400;
    player.achievements[11].unlocked = false;
    
    strcpy(player.achievements[12].name, "全技能大师");
    strcpy(player.achievements[12].description, "所有技能达到5级");
    player.achievements[12].condition = 5;
    player.achievements[12].reward = 5000;
    player.achievements[12].reward_exp = 500;
    player.achievements[12].unlocked = false;
    
    strcpy(player.achievements[13].name, "限定收藏家");
    strcpy(player.achievements[13].description, "收集所有限定物品");
    player.achievements[13].condition = 10;
    player.achievements[13].reward = 6000;
    player.achievements[13].reward_exp = 600;
    player.achievements[13].unlocked = false;
    
    strcpy(player.achievements[14].name, "成就猎人");
    strcpy(player.achievements[14].description, "解锁20个成就");
    player.achievements[14].condition = 20;
    player.achievements[14].reward = 3000;
    player.achievements[14].reward_exp = 300;
    player.achievements[14].unlocked = false;
}

void init_skills() {
    for (int i = 0; i < MAX_SKILLS; i++) {
        player.skills[i].level = 1;
        player.skills[i].experience = 0;
        player.skills[i].max_level = 20;
        player.skills[i].multiplier = 1.05f;
    }
    
    strcpy(player.skills[SKILL_SPEED].name, "速度");
    strcpy(player.skills[SKILL_SPEED].description, "提升任务完成速度");
    
    strcpy(player.skills[SKILL_STRENGTH].name, "力量");
    strcpy(player.skills[SKILL_STRENGTH].description, "增加体力相关任务收入");
    
    strcpy(player.skills[SKILL_INTELLIGENCE].name, "智力");
    strcpy(player.skills[SKILL_INTELLIGENCE].description, "增加智力相关任务收入");
    
    strcpy(player.skills[SKILL_LUCK].name, "幸运");
    strcpy(player.skills[SKILL_LUCK].description, "增加所有随机事件几率");
    
    strcpy(player.skills[SKILL_CHARISMA].name, "魅力");
    strcpy(player.skills[SKILL_CHARISMA].description, "提升社交任务收入");
    
    strcpy(player.skills[SKILL_ENDURANCE].name, "耐力");
    strcpy(player.skills[SKILL_ENDURANCE].description, "降低任务能量消耗");
    
    strcpy(player.skills[SKILL_CREATIVITY].name, "创造力");
    strcpy(player.skills[SKILL_CREATIVITY].description, "提升创意任务收入");
    
    strcpy(player.skills[SKILL_LEADERSHIP].name, "领导力");
    strcpy(player.skills[SKILL_LEADERSHIP].description, "提升团队任务收入");
    
    strcpy(player.skills[SKILL_NEGOTIATION].name, "谈判技巧");
    strcpy(player.skills[SKILL_NEGOTIATION].description, "提升交易收益");
    
    strcpy(player.skills[SKILL_MANAGEMENT].name, "管理能力");
    strcpy(player.skills[SKILL_MANAGEMENT].description, "提升被动收入");
}

void init_quests() {
    //清空任务
    for (int i = 0; i < MAX_QUESTS; i++) {
        memset(&player.active_quests[i], 0, sizeof(Quest));
    }
    
    //每日任务
    strcpy(player.active_quests[0].name, "完成3个任务");
    strcpy(player.active_quests[0].description, "完成任意3个任务");
    player.active_quests[0].type = QUEST_DAILY;
    player.active_quests[0].requirement = 3;
    player.active_quests[0].reward_coins = 300;
    player.active_quests[0].reward_exp = 30;
    player.active_quests[0].assigned_time = time(NULL);
    player.active_quests[0].deadline = time(NULL) + 86400;
    
    strcpy(player.active_quests[1].name, "购买家具");
    strcpy(player.active_quests[1].description, "购买任意一件家具");
    player.active_quests[1].type = QUEST_DAILY;
    player.active_quests[1].requirement = 1;
    player.active_quests[1].reward_coins = 200;
    player.active_quests[1].reward_exp = 20;
    player.active_quests[1].assigned_time = time(NULL);
    player.active_quests[1].deadline = time(NULL) + 86400;
    
    strcpy(player.active_quests[2].name, "投资股票");
    strcpy(player.active_quests[2].description, "进行一次股票投资");
    player.active_quests[2].type = QUEST_DAILY;
    player.active_quests[2].requirement = 1;
    player.active_quests[2].reward_coins = 400;
    player.active_quests[2].reward_exp = 40;
    player.active_quests[2].assigned_time = time(NULL);
    player.active_quests[2].deadline = time(NULL) + 86400;
    
    //每周任务
    strcpy(player.active_quests[3].name, "赚取1万坤币");
    strcpy(player.active_quests[3].description, "一周内赚取10000坤币");
    player.active_quests[3].type = QUEST_WEEKLY;
    player.active_quests[3].requirement = 10000;
    player.active_quests[3].reward_coins = 2000;
    player.active_quests[3].reward_exp = 200;
    player.active_quests[3].assigned_time = time(NULL);
    player.active_quests[3].deadline = time(NULL) + 604800;
    
    strcpy(player.active_quests[4].name, "升级技能");
    strcpy(player.active_quests[4].description, "升级任意技能3次");
    player.active_quests[4].type = QUEST_WEEKLY;
    player.active_quests[4].requirement = 3;
    player.active_quests[4].reward_coins = 1500;
    player.active_quests[4].reward_exp = 150;
    player.active_quests[4].assigned_time = time(NULL);
    player.active_quests[4].deadline = time(NULL) + 604800;
    
    //主线任务
    strcpy(player.active_quests[5].name, "成为百万富翁");
    strcpy(player.active_quests[5].description, "积累100万坤币");
    player.active_quests[5].type = QUEST_MAIN;
    player.active_quests[5].requirement = 1000000;
    player.active_quests[5].reward_coins = 10000;
    player.active_quests[5].reward_exp = 1000;
    player.active_quests[5].assigned_time = time(NULL);
    
    strcpy(player.active_quests[6].name, "建立公司");
    strcpy(player.active_quests[6].description, "将公司升到5级");
    player.active_quests[6].type = QUEST_MAIN;
    player.active_quests[6].requirement = 5;
    player.active_quests[6].reward_coins = 5000;
    player.active_quests[6].reward_exp = 500;
    player.active_quests[6].assigned_time = time(NULL);
}

void init_pets() {
    for (int i = 0; i < MAX_PETS; i++) {
        player.pets[i].is_unlocked = false;
        player.pets[i].happiness = 100;
        player.pets[i].hunger = 100;
        player.pets[i].level = 1;
        player.pets[i].experience = 0;
        player.pets[i].bonus_multiplier = 1.0f;
        strcpy(player.pets[i].name, "");
    }
    
    player.pets[0].type = PET_CAT;
    player.pets[1].type = PET_DOG;
    player.pets[2].type = PET_PARROT;
    player.pets[3].type = PET_DRAGON;
    player.pets[4].type = PET_PHOENIX;
    
    //解锁初始宠物
    player.pets[0].is_unlocked = true;
    strcpy(player.pets[0].name, "小咪");
    player.pets[0].bonus_multiplier = 1.1f;
    player.pet_count = 1;
}

void init_company_levels() {
    strcpy(player.company_levels[0].name, "个体户");
    player.company_levels[0].level = 1;
    player.company_levels[0].required_coins = 0;
    player.company_levels[0].income_multiplier = 1.0f;
    player.company_levels[0].unlock_skill_points = 0;
    strcpy(player.company_levels[0].description, "独自经营的小生意");
    
    strcpy(player.company_levels[1].name, "小微企业");
    player.company_levels[1].level = 2;
    player.company_levels[1].required_coins = 5000;
    player.company_levels[1].income_multiplier = 1.2f;
    player.company_levels[1].unlock_skill_points = 1;
    strcpy(player.company_levels[1].description, "雇佣1-2名员工");
    
    strcpy(player.company_levels[2].name, "小型公司");
    player.company_levels[2].level = 3;
    player.company_levels[2].required_coins = 20000;
    player.company_levels[2].income_multiplier = 1.5f;
    player.company_levels[2].unlock_skill_points = 2;
    strcpy(player.company_levels[2].description, "拥有小型办公室");
    
    strcpy(player.company_levels[3].name, "中型企业");
    player.company_levels[3].level = 4;
    player.company_levels[3].required_coins = 100000;
    player.company_levels[3].income_multiplier = 2.0f;
    player.company_levels[3].unlock_skill_points = 3;
    strcpy(player.company_levels[3].description, "在写字楼有办公室");
    
    strcpy(player.company_levels[4].name, "大型公司");
    player.company_levels[4].level = 5;
    player.company_levels[4].required_coins = 500000;
    player.company_levels[4].income_multiplier = 3.0f;
    player.company_levels[4].unlock_skill_points = 5;
    strcpy(player.company_levels[4].description, "拥有自己的办公楼");
    
    strcpy(player.company_levels[5].name, "集团公司");
    player.company_levels[5].level = 6;
    player.company_levels[5].required_coins = 2000000;
    player.company_levels[5].income_multiplier = 5.0f;
    player.company_levels[5].unlock_skill_points = 8;
    strcpy(player.company_levels[5].description, "业务多元化发展");
    
    strcpy(player.company_levels[6].name, "上市公司");
    player.company_levels[6].level = 7;
    player.company_levels[6].required_coins = 10000000;
    player.company_levels[6].income_multiplier = 8.0f;
    player.company_levels[6].unlock_skill_points = 12;
    strcpy(player.company_levels[6].description, "在证券交易所上市");
    
    strcpy(player.company_levels[7].name, "跨国企业");
    player.company_levels[7].level = 8;
    player.company_levels[7].required_coins = 50000000;
    player.company_levels[7].income_multiplier = 12.0f;
    player.company_levels[7].unlock_skill_points = 18;
    strcpy(player.company_levels[7].description, "业务遍布全球");
    
    strcpy(player.company_levels[8].name, "商业帝国");
    player.company_levels[8].level = 9;
    player.company_levels[8].required_coins = 200000000;
    player.company_levels[8].income_multiplier = 20.0f;
    player.company_levels[8].unlock_skill_points = 25;
    strcpy(player.company_levels[8].description, "影响世界经济");
    
    strcpy(player.company_levels[9].name, "传奇财团");
    player.company_levels[9].level = 10;
    player.company_levels[9].required_coins = 1000000000;
    player.company_levels[9].income_multiplier = 50.0f;
    player.company_levels[9].unlock_skill_points = 35;
    strcpy(player.company_levels[9].description, "掌控全球经济命脉");
    
    player.current_company_level = 0;
}

void init_stocks() {
    strcpy(player.stocks[0].name, "坤坤科技");
    player.stocks[0].base_price = 100;
    player.stocks[0].current_price = 100;
    player.stocks[0].price_change = 0;
    player.stocks[0].volatility = 10;
    player.stocks[0].last_update = time(NULL);
    
    strcpy(player.stocks[1].name, "外卖速递");
    player.stocks[1].base_price = 50;
    player.stocks[1].current_price = 50;
    player.stocks[1].price_change = 0;
    player.stocks[1].volatility = 15;
    player.stocks[1].last_update = time(NULL);
    
    strcpy(player.stocks[2].name, "房地产集团");
    player.stocks[2].base_price = 200;
    player.stocks[2].current_price = 200;
    player.stocks[2].price_change = 0;
    player.stocks[2].volatility = 8;
    player.stocks[2].last_update = time(NULL);
    
    strcpy(player.stocks[3].name, "能源公司");
    player.stocks[3].base_price = 150;
    player.stocks[3].current_price = 150;
    player.stocks[3].price_change = 0;
    player.stocks[3].volatility = 12;
    player.stocks[3].last_update = time(NULL);
    
    strcpy(player.stocks[4].name, "娱乐传媒");
    player.stocks[4].base_price = 80;
    player.stocks[4].current_price = 80;
    player.stocks[4].price_change = 0;
    player.stocks[4].volatility = 20;
    player.stocks[4].last_update = time(NULL);
    
    //初始化投资
    for (int i = 0; i < 5; i++) {
        player.invested_stocks[i] = 0;
    }
}

void init_properties() {
    strcpy(available_properties[0].name, "公寓");
    strcpy(available_properties[0].description, "小型公寓，适合单身");
    available_properties[0].coins_needed = 50000;
    available_properties[0].days_needed = 30;
    available_properties[0].passive_income = 50;
    available_properties[0].unlock_achievement_id = 1;
    
    strcpy(available_properties[1].name, "别墅");
    strcpy(available_properties[1].description, "带花园的独栋别墅");
    available_properties[1].coins_needed = 200000;
    available_properties[1].days_needed = 60;
    available_properties[1].passive_income = 200;
    available_properties[1].unlock_achievement_id = 5;
    
    strcpy(available_properties[2].name, "写字楼");
    strcpy(available_properties[2].description, "商业办公大楼");
    available_properties[2].coins_needed = 1000000;
    available_properties[2].days_needed = 120;
    available_properties[2].passive_income = 1000;
    available_properties[2].unlock_achievement_id = 10;
    
    strcpy(available_properties[3].name, "购物中心");
    strcpy(available_properties[3].description, "大型商业综合体");
    available_properties[3].coins_needed = 5000000;
    available_properties[3].days_needed = 180;
    available_properties[3].passive_income = 5000;
    available_properties[3].unlock_achievement_id = 15;
    
    strcpy(available_properties[4].name, "酒店");
    strcpy(available_properties[4].description, "五星级豪华酒店");
    available_properties[4].coins_needed = 20000000;
    available_properties[4].days_needed = 240;
    available_properties[4].passive_income = 20000;
    available_properties[4].unlock_achievement_id = 20;
    
    strcpy(available_properties[5].name, "度假村");
    strcpy(available_properties[5].description, "海滨度假胜地");
    available_properties[5].coins_needed = 50000000;
    available_properties[5].days_needed = 300;
    available_properties[5].passive_income = 50000;
    available_properties[5].unlock_achievement_id = 25;
    
    strcpy(available_properties[6].name, "私人岛屿");
    strcpy(available_properties[6].description, "专属私人岛屿");
    available_properties[6].coins_needed = 200000000;
    available_properties[6].days_needed = 365;
    available_properties[6].passive_income = 200000;
    available_properties[6].unlock_achievement_id = 30;
    
    strcpy(available_properties[7].name, "商业街");
    strcpy(available_properties[7].description, "整条商业街道");
    available_properties[7].coins_needed = 500000000;
    available_properties[7].days_needed = 450;
    available_properties[7].passive_income = 500000;
    available_properties[7].unlock_achievement_id = 35;
    
    strcpy(available_properties[8].name, "科技园区");
    strcpy(available_properties[8].description, "高科技产业园区");
    available_properties[8].coins_needed = 2000000000;
    available_properties[8].days_needed = 540;
    available_properties[8].passive_income = 2000000;
    available_properties[8].unlock_achievement_id = 40;
    
    strcpy(available_properties[9].name, "国际机场");
    strcpy(available_properties[9].description, "大型国际机场");
    available_properties[9].coins_needed = 10000000000LL;
    available_properties[9].days_needed = 720;
    available_properties[9].passive_income = 10000000;
    available_properties[9].unlock_achievement_id = 50;
}

//================ 存档系统 ================
void save_game() {
    FILE *file = fopen(SAVE_FILE, "wb");
    if (file) {
        fwrite(&player, sizeof(Player), 1, file);
        fclose(file);
        print_colored(COLOR_GREEN, "? 游戏存档已保存！\n");
    } else {
        print_colored(COLOR_RED, "? 保存失败！\n");
    }
}

void load_game() {
    FILE *file = fopen(SAVE_FILE, "rb");
    if (file) {
        fread(&player, sizeof(Player), 1, file);
        fclose(file);
        print_colored(COLOR_GREEN, "? 游戏存档已加载！\n");
    } else {
        print_colored(COLOR_YELLOW, "? 未找到存档，开始新游戏...\n");
    }
}

//================ 事件系统 ================
const char* get_event_name(EventType type) {
    static const char* names[] = {
        "无",
        "通货膨胀",
        "股市大涨",
        "任务奖励",
        "免费家具",
        "彩票加成",
        "双倍经验",
        "神秘宝箱",
        "BOSS挑战",
        "节日庆典"
    };
    return names[type];
}

const char* get_event_description(EventType type) {
    static const char* descriptions[] = {
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
    return descriptions[type];
}

void random_event() {
    if ((rand() % 100) < 15) { //15% 几率触发事件
        EventType new_event = (EventType)(1 + rand() % (EVENT_TYPE_COUNT - 1));
        
        if (player.active_event != EVENT_NONE) {
            end_event(player.active_event);
        }
        
        trigger_event(new_event);
        print_colored(COLOR_MAGENTA, "\n?? 随机事件触发：%s！\n", get_event_name(new_event));
        print_colored(COLOR_CYAN, "   %s\n", get_event_description(new_event));
    }
}

void trigger_event(EventType type) {
    player.active_event = type;
    player.event_end_time = time(NULL) + 300; //5分钟
    
    switch (type) {
        case EVENT_FESTIVAL:
            player.festival_participations++;
            break;
        case EVENT_BOSS_CHALLENGE:
            player.boss_defeats++;
            break;
        default:
            break;
    }
    
    handle_event_effect(type, true);
}

void end_event(EventType type) {
    handle_event_effect(type, false);
    player.active_event = EVENT_NONE;
    print_colored(COLOR_CYAN, "\n? 事件结束：%s\n", get_event_name(type));
}

void handle_event_effect(EventType type, bool apply) {
    static int original_prices[MAX_MARKET] = {0};
    
    switch(type) {
        case EVENT_INFLATION:
            if (apply) {
                for (int i = 0; i < MAX_MARKET; i++) {
                    original_prices[i] = market[i].price;
                    market[i].price = (int)(market[i].price * 1.2);
                }
            } else {
                for (int i = 0; i < MAX_MARKET; i++) {
                    market[i].price = original_prices[i];
                }
            }
            break;
            
        case EVENT_STOCK_BOOM:
            //股票收益增加已在计算时处理
            break;
            
        case EVENT_TASK_BONUS:
            //任务奖励增加已在计算时处理
            break;
            
        case EVENT_FREE_FURNITURE:
            if (apply && player.furniture_count < MAX_FURNITURE) {
                const char* free_furniture[] = {"神秘宝箱", "幸运地毯", "祝福花瓶"};
                int idx = rand() % 3;
                strcpy(player.furniture[player.furniture_count++], free_furniture[idx]);
                print_colored(COLOR_GREEN, "   ?? 获得免费家具：%s！\n", free_furniture[idx]);
            }
            break;
            
        case EVENT_DOUBLE_EXP:
            //双倍经验已在计算时处理
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
            end_event(player.active_event);
        }
    }
}

//================ 成就系统 ================
void check_achievements() {
    //检查所有成就条件
    for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
        if (player.achievements[i].unlocked) continue;
        
        bool condition_met = false;
        
        switch(i) {
            case 0: //第一桶金
                condition_met = player.kun_coins >= 100;
                break;
            case 1: //家具收藏家
                condition_met = player.furniture_count >= 5;
                break;
            case 2: //打工达人
                condition_met = player.lifetime_task_completions >= 50;
                break;
            case 3: //股市之神
                //简化检查
                condition_met = player.total_income - player.total_expenses >= 1000;
                break;
            case 4: //幸运儿
                //彩票中奖在开奖时检查
                break;
            case 5: //百万富翁
                condition_met = player.kun_coins >= 1000000;
                break;
            case 6: //等级大师
                condition_met = player.level >= 50;
                break;
            case 7: //技能专家
                for (int j = 0; j < MAX_SKILLS; j++) {
                    if (player.skills[j].level >= 10) {
                        condition_met = true;
                        break;
                    }
                }
                break;
            case 8: //任务狂人
                condition_met = player.lifetime_task_completions >= 1000;
                break;
            case 9: //社交达人
                condition_met = player.friend_count >= 5;
                break;
            case 10: //宠物爱好者
                condition_met = player.pet_count >= 3;
                break;
            case 11: //房产大亨
                condition_met = player.property_count >= 5;
                break;
            case 12: //全技能大师
                condition_met = true;
                for (int j = 0; j < MAX_SKILLS; j++) {
                    if (player.skills[j].level < 5) {
                        condition_met = false;
                        break;
                    }
                }
                break;
            case 14: //成就猎人
                {
                    int unlocked_count = 0;
                    for (int j = 0; j < MAX_ACHIEVEMENTS; j++) {
                        if (player.achievements[j].unlocked) unlocked_count++;
                    }
                    condition_met = unlocked_count >= 20;
                }
                break;
        }
        
        if (condition_met) {
            unlock_achievement(i);
        }
    }
}

void unlock_achievement(int achievement_id) {
    if (achievement_id < 0 || achievement_id >= MAX_ACHIEVEMENTS) return;
    
    player.achievements[achievement_id].unlocked = true;
    player.achievements[achievement_id].unlock_time = time(NULL);
    
    player.kun_coins += player.achievements[achievement_id].reward;
    player.kun_exp += player.achievements[achievement_id].reward_exp;
    player.achievement_points += player.achievements[achievement_id].reward / 10;
    
    print_colored(COLOR_YELLOW, "\n?? 成就解锁：%s！\n", player.achievements[achievement_id].name);
    printf("   %s\n", player.achievements[achievement_id].description);
    printf("   获得奖励：%d 坤币, %d 经验, %d 成就点\n", 
           player.achievements[achievement_id].reward,
           player.achievements[achievement_id].reward_exp,
           player.achievements[achievement_id].reward / 10);
    
    check_achievements(); //递归检查可能解锁的成就
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
            struct tm *tm = localtime(&player.achievements[i].unlock_time);
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
            printf("？ %s\n", player.achievements[i].name);
            printf("   %s\n", player.achievements[i].description);
            printf("   进度：");
            switch(i) {
                case 0:
                    print_progress_bar(player.kun_coins, 100, 20);
                    break;
                case 1:
                    print_progress_bar(player.furniture_count, 5, 20);
                    break;
                case 2:
                    print_progress_bar(player.lifetime_task_completions, 50, 20);
                    break;
                case 5:
                    print_progress_bar(player.kun_coins, 1000000, 20);
                    break;
                case 6:
                    print_progress_bar(player.level, 50, 20);
                    break;
                default:
                    printf("未知\n");
                    break;
            }
        }
    }
    draw_separator();
}

//================ 游戏主逻辑 ================
void do_task(int task_index) {
    if (task_index < 0 || task_index >= MAX_TASKS) return;
    
    Task *task = &tasks[task_index];
    
    //检查是否解锁
    if (!task->is_unlocked) {
        if (player.kun_coins >= task->unlock_cost) {
            task->is_unlocked = true;
            player.tasks_unlocked[task_index] = true;
            player.kun_coins -= task->unlock_cost;
            print_colored(COLOR_GREEN, "? 解锁新任务：%s！\n", task->name);
        } else {
            print_colored(COLOR_RED, "? 需要 %d 坤币解锁此任务！\n", task->unlock_cost);
            return;
        }
    }
    
    //检查等级要求
    if (player.level < task->required_level) {
        print_colored(COLOR_RED, "? 需要等级 %d 才能执行此任务！\n", task->required_level);
        return;
    }
    
    //检查技能要求
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            SkillType skill_type = task->required_skills[i];
            if (player.skills[skill_type].level < task->required_skill_level[i]) {
                print_colored(COLOR_RED, "? 需要 %s 技能等级 %d！\n", 
                           player.skills[skill_type].name, task->required_skill_level[i]);
                return;
            }
        }
    }
    
    //检查能量
    if (player.energy < task->energy_cost) {
        print_colored(COLOR_RED, "? 能量不足！需要 %d 能量\n", task->energy_cost);
        return;
    }
    
    //计算收入
    float base_income = task->base_income;
    float multiplier = 1.0;
    
    //技能加成
    for (int i = 0; i < 3; i++) {
        if (task->required_skills[i] != SKILL_NONE) {
            Skill *skill = &player.skills[task->required_skills[i]];
            multiplier *= pow(skill->multiplier, skill->level);
        }
    }
    
    //公司等级加成
    if (player.current_company_level > 0) {
        multiplier *= player.company_levels[player.current_company_level].income_multiplier;
    }
    
    //事件加成
    if (player.active_event == EVENT_TASK_BONUS) {
        multiplier *= 1.5;
    }
    
    //季节加成
    switch (player.current_season) {
        case SEASON_SPRING:
            multiplier *= 1.1;
            break;
        case SEASON_SUMMER:
            multiplier *= 1.05;
            break;
        case SEASON_AUTUMN:
            multiplier *= 1.15;
            break;
        case SEASON_WINTER:
            multiplier *= 0.9;
            break;
    }
    
    //宠物加成
    for (int i = 0; i < player.pet_count; i++) {
        if (player.pets[i].is_unlocked) {
            multiplier *= player.pets[i].bonus_multiplier;
        }
    }
    
    //计算最终收入
    int income = (int)(base_income * multiplier);
    int exp_gain = task->experience_gain;
    
    //双倍经验事件
    if (player.active_event == EVENT_DOUBLE_EXP) {
        exp_gain *= 2;
    }
    
    //执行任务
    player.energy -= task->energy_cost;
    player.kun_coins += income;
    player.kun_exp += exp_gain;
    player.total_income += income;
    player.lifetime_task_completions++;
    player.daily_task_completions++;
    player.weekly_task_completions++;
    
    //更新任务统计
    task->completion_count++;
    task->last_completed = time(NULL);
    
    print_colored(COLOR_GREEN, "\n? 完成任务：%s\n", task->name);
    printf("   获得收入：%d 坤币\n", income);
    printf("   获得经验：%d\n", exp_gain);
    printf("   消耗能量：%d\n", task->energy_cost);
    printf("   剩余能量：%d/%d\n", player.energy, player.max_energy);
    
    //检查升级
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
    
    //检查任务进度
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "完成任务")) {
            player.active_quests[i].progress++;
            if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                complete_quest(i);
            }
        }
    }
    
    check_achievements();
}

void buy_item(int index) {
    if (index < 0 || index >= MAX_MARKET) return;
    
    MarketItem *item = &market[index];
    
    //检查等级要求
    if (player.level < item->required_level) {
        print_colored(COLOR_RED, "? 需要等级 %d 才能购买此物品！\n", item->required_level);
        return;
    }
    
    //检查是否限定且过期
    if (item->is_limited && item->available_until < time(NULL)) {
        print_colored(COLOR_RED, "? 此限定物品已过期！\n");
        return;
    }
    
    //检查库存
    if (player.furniture_count >= MAX_FURNITURE) {
        print_colored(COLOR_RED, "? 家具数量已达到上限！\n");
        return;
    }
    
    int price = item->price;
    
    //通货膨胀事件
    if (player.active_event == EVENT_INFLATION) {
        price = (int)(price * 1.2);
    }
    
    if (player.kun_coins >= price) {
        player.kun_coins -= price;
        player.total_expenses += price;
        
        //添加到家具列表
        strcpy(player.furniture[player.furniture_count], item->name);
        player.furniture_count++;
        
        //添加到背包
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
            strcpy(inv_item.name, item->name);
            inv_item.price = price;
            inv_item.value = item->value;
            inv_item.rarity = item->rarity;
            inv_item.is_consumable = false;
            inv_item.effect_value = (int)(item->passive_income * 100);
            strcpy(inv_item.description, item->description);
            inv_item.quantity = 1;
            player.inventory[player.inventory_count++] = inv_item;
        }
        
        print_colored(COLOR_GREEN, "\n? 购买成功：%s\n", item->name);
        printf("   花费：%d 坤币\n", price);
        printf("   被动收入：%.1f 坤币/小时\n", item->passive_income);
        
        //检查成就
        check_achievements();
        
        //检查任务进度
        for (int i = 0; i < MAX_QUESTS; i++) {
            if (!player.active_quests[i].completed && 
                strstr(player.active_quests[i].description, "购买家具")) {
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
    
    Skill *skill = &player.skills[skill_index];
    
    if (skill->level >= skill->max_level) {
        print_colored(COLOR_RED, "? 技能已达到最大等级！\n");
        return;
    }
    
    if (player.skill_points <= 0) {
        print_colored(COLOR_RED, "? 没有足够的技能点！\n");
        return;
    }
    
    int cost = skill->level * 100;
    if (player.kun_coins < cost) {
        print_colored(COLOR_RED, "? 升级需要 %d 坤币！\n", cost);
        return;
    }
    
    player.skill_points--;
    player.kun_coins -= cost;
    skill->level++;
    skill->experience = 0;
    
    print_colored(COLOR_GREEN, "\n? 技能升级：%s 提升到等级 %d\n", skill->name, skill->level);
    printf("   花费：%d 坤币，1 技能点\n", cost);
    printf("   当前加成：%.0f%%\n", (pow(skill->multiplier, skill->level) - 1) * 100);
    
    //检查任务进度
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "升级技能")) {
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
    
    Pet *pet = &player.pets[pet_index];
    
    if (!pet->is_unlocked) {
        print_colored(COLOR_RED, "? 宠物未解锁！\n");
        return;
    }
    
    //检查是否有宠物食品
    bool has_food = false;
    for (int i = 0; i < player.inventory_count; i++) {
        if (strcmp(player.inventory[i].name, "宠物食品") == 0 && player.inventory[i].quantity > 0) {
            has_food = true;
            player.inventory[i].quantity--;
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
    
    print_colored(COLOR_GREEN, "\n? 喂食 %s\n", pet->name);
    printf("   饱食度：100\n");
    printf("   快乐度：%d\n", pet->happiness);
}

void complete_quest(int quest_index) {
    if (quest_index < 0 || quest_index >= MAX_QUESTS) return;
    
    Quest *quest = &player.active_quests[quest_index];
    
    if (quest->completed) {
        print_colored(COLOR_RED, "? 任务已完成！\n");
        return;
    }
    
    quest->completed = true;
    player.kun_coins += quest->reward_coins;
    player.kun_exp += quest->reward_exp;
    
    print_colored(COLOR_GREEN, "\n? 任务完成：%s\n", quest->name);
    printf("   获得奖励：%d 坤币，%d 经验\n", quest->reward_coins, quest->reward_exp);
    
    //生成新任务
    if (quest->type == QUEST_DAILY || quest->type == QUEST_WEEKLY) {
        //重置任务
        quest->completed = false;
        quest->progress = 0;
        quest->assigned_time = time(NULL);
        if (quest->type == QUEST_DAILY) {
            quest->deadline = time(NULL) + 86400;
        } else {
            quest->deadline = time(NULL) + 604800;
        }
    }
}

void level_up_company() {
    if (player.current_company_level >= MAX_COMPANY_LEVELS - 1) {
        print_colored(COLOR_RED, "? 公司已达到最高等级！\n");
        return;
    }
    
    CompanyLevel *next_level = &player.company_levels[player.current_company_level + 1];
    
    if (player.kun_coins < next_level->required_coins) {
        print_colored(COLOR_RED, "? 升级需要 %lld 坤币！\n", next_level->required_coins);
        return;
    }
    
    player.kun_coins -= (int)next_level->required_coins;
    player.current_company_level++;
    player.skill_points += next_level->unlock_skill_points;
    
    print_colored(COLOR_GREEN, "\n? 公司升级：%s\n", next_level->name);
    printf("   花费：%lld 坤币\n", next_level->required_coins);
    printf("   获得 %d 技能点\n", next_level->unlock_skill_points);
    printf("   收入倍数：%.1f\n", next_level->income_multiplier);
    
    check_achievements();
}

void buy_stock(int stock_index, int amount) {
    if (stock_index < 0 || stock_index >= 5) return;
    if (amount <= 0) return;
    
    Stock *stock = &player.stocks[stock_index];
    int total_cost = stock->current_price * amount;
    
    if (player.kun_coins < total_cost) {
        print_colored(COLOR_RED, "? 坤币不足！需要 %d 坤币\n", total_cost);
        return;
    }
    
    player.kun_coins -= total_cost;
    player.invested_stocks[stock_index] += amount;
    
    print_colored(COLOR_GREEN, "\n? 购买股票：%s × %d\n", stock->name, amount);
    printf("   单价：%d 坤币\n", stock->current_price);
    printf("   总价：%d 坤币\n", total_cost);
    
    //检查任务进度
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            strstr(player.active_quests[i].description, "投资股票")) {
            player.active_quests[i].progress++;
            if (player.active_quests[i].progress >= player.active_quests[i].requirement) {
                complete_quest(i);
            }
        }
    }
}

void participate_lottery() {
    if (player.lottery_tickets <= 0) {
        print_colored(COLOR_RED, "? 没有彩票！\n");
        return;
    }
    
    player.lottery_tickets--;
    
    //计算中奖几率
    float base_chance = 0.01; //1% 基础中奖率
    if (player.active_event == EVENT_LOTTERY_BONUS) {
        base_chance *= 2;
    }
    
    float luck_multiplier = pow(player.skills[SKILL_LUCK].multiplier, player.skills[SKILL_LUCK].level);
    float chance = base_chance * luck_multiplier;
    
    int roll = rand() % 10000;
    if (roll < chance * 10000) {
        //中奖！
        int prize = 10000 + rand() % 90000; //1万-10万
        prize *= (1 + player.prestige_level);
        
        player.kun_coins += prize;
        print_colored(COLOR_YELLOW, "\n?? 恭喜中奖！获得 %d 坤币！\n", prize);
        
        //检查成就
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
        //普通奖励
        int coins = 1000 + rand() % 4000;
        player.kun_coins += coins;
        print_colored(COLOR_GREEN, "\n?? 开启神秘宝箱获得：%d 坤币\n", coins);
    } else if (roll < 80) {
        //稀有奖励
        int coins = 5000 + rand() % 10000;
        player.kun_coins += coins;
        print_colored(COLOR_BLUE, "\n?? 开启神秘宝箱获得：%d 坤币（稀有）\n", coins);
    } else if (roll < 95) {
        //史诗奖励
        int coins = 20000 + rand() % 30000;
        player.kun_coins += coins;
        player.skill_points += 3;
        print_colored(COLOR_MAGENTA, "\n?? 开启神秘宝箱获得：%d 坤币和3技能点（史诗）\n", coins);
    } else {
        //传说奖励
        int coins = 100000;
        player.kun_coins += coins;
        player.skill_points += 10;
        player.mystery_keys += 3;
        print_colored(COLOR_YELLOW, "\n?? 开启神秘宝箱获得：%d 坤币、10技能点和3把钥匙（传说）\n", coins);
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

//================ 界面函数 ================
void show_status() {
    CLEAR_SCREEN();
    draw_box("玩家状态");
    
    printf(COLOR_BOLD "?? 基本信息\n" COLOR_RESET);
    printf("   等级：%d\n", player.level);
    printf("   经验：%d/%d\n", player.kun_exp, player.level * 100);
    printf("   坤币：%d\n", player.kun_coins);
    printf("   能量：%d/%d\n", player.energy, player.max_energy);
    printf("   技能点：%d\n", player.skill_points);
    printf("   成就点：%d\n", player.achievement_points);
    
    printf(COLOR_BOLD "\n?? 游戏进度\n" COLOR_RESET);
    printf("   连续登录：%d 天\n", player.consecutive_days);
    printf("   游戏天数：%d 天\n", player.day_count);
    printf("   总任务完成：%d 次\n", player.lifetime_task_completions);
    printf("   今日任务：%d/10\n", player.daily_task_completions);
    printf("   公司等级：%s\n", player.company_levels[player.current_company_level].name);
    printf("   声望等级：%d\n", player.prestige_level);
    
    printf(COLOR_BOLD "\n?? 资产统计\n" COLOR_RESET);
    printf("   总收入：%d 坤币\n", player.total_income);
    printf("   总支出：%d 坤币\n", player.total_expenses);
    printf("   净资产：%d 坤币\n", player.total_income - player.total_expenses);
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
    
    show_active_events();
    draw_separator();
}

void show_skills() {
    CLEAR_SCREEN();
    draw_box("技能系统");
    
    printf("可用技能点：%d\n\n", player.skill_points);
    
    for (int i = 0; i < MAX_SKILLS; i++) {
        Skill *skill = &player.skills[i];
        
        //根据等级显示颜色
        const char* color = COLOR_WHITE;
        if (skill->level >= 15) color = COLOR_YELLOW;
        else if (skill->level >= 10) color = COLOR_MAGENTA;
        else if (skill->level >= 5) color = COLOR_CYAN;
        else if (skill->level >= 3) color = COLOR_GREEN;
        
        printf("%s%s Lv.%d%s\n", color, skill->name, skill->level, COLOR_RESET);
        printf("   %s\n", skill->description);
        printf("   加成：%.0f%%  升级需要：%d 坤币\n", 
               (pow(skill->multiplier, skill->level) - 1) * 100,
               skill->level * 100);
        print_progress_bar(skill->level, skill->max_level, 30);
        printf("\n");
    }
    
    draw_separator();
    printf("1. 升级速度技能   2. 升级力量技能   3. 升级智力技能\n");
    printf("4. 升级幸运技能   5. 升级魅力技能   6. 升级耐力技能\n");
    printf("7. 升级创造力技能 8. 升级领导力技能 9. 升级谈判技巧\n");
    printf("10. 升级管理能力  0. 返回\n");
    draw_separator();
    
    int choice = get_valid_input(0, 10);
    if (choice > 0) {
        upgrade_skill(choice - 1);
        press_any_key();
    }
}

void show_inventory() {
    CLEAR_SCREEN();
    draw_box("背包物品");
    
    if (player.inventory_count == 0) {
        printf("背包空空如也...\n");
    } else {
        printf("物品数量：%d/%d\n\n", player.inventory_count, MAX_INVENTORY);
        
        for (int i = 0; i < player.inventory_count; i++) {
            InventoryItem *item = &player.inventory[i];
            
            //根据稀有度显示颜色
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
    printf("1. 使用物品   2. 整理背包   0. 返回\n");
    draw_separator();
    
    int choice = get_valid_input(0, 2);
    if (choice == 1 && player.inventory_count > 0) {
        printf("选择要使用的物品 (1-%d): ", player.inventory_count);
        int item_choice = get_valid_input(1, player.inventory_count);
        use_inventory_item(item_choice - 1);
    } else if (choice == 2) {
        //整理背包逻辑
        print_colored(COLOR_GREEN, "背包已整理！\n");
    }
    press_any_key();
}

void show_quests() {
    CLEAR_SCREEN();
    draw_box("任务列表");
    
    int active_count = 0;
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (!player.active_quests[i].completed && 
            player.active_quests[i].deadline > time(NULL)) {
            active_count++;
        }
    }
    
    printf("进行中的任务：%d\n\n", active_count);
    
    for (int i = 0; i < MAX_QUESTS; i++) {
        Quest *quest = &player.active_quests[i];
        
        if (quest->deadline == 0) continue; //未激活的任务
        
        if (quest->completed) {
            printf(COLOR_GREEN "? %s\n" COLOR_RESET, quest->name);
            printf("   已完成！\n");
        } else if (quest->deadline < time(NULL)) {
            printf(COLOR_RED "? %s\n" COLOR_RESET, quest->name);
            printf("   已过期\n");
        } else {
            //显示任务类型
            printf(COLOR_CYAN "● %s\n" COLOR_RESET, quest->name);
            printf("   %s\n", quest->description);
            printf("   进度：%d/%d\n", quest->progress, quest->requirement);
            
            //显示剩余时间
            time_t remaining = quest->deadline - time(NULL);
            if (remaining > 0) {
                int days = remaining / 86400;
                int hours = (remaining % 86400) / 3600;
                printf("   剩余时间：%d天%d小时\n", days, hours);
            }
        }
        printf("\n");
    }
    
    draw_separator();
    printf("1. 领取奖励   0. 返回\n");
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
    
    for (int i = 0; i < MAX_PETS; i++) {
        Pet *pet = &player.pets[i];
        
        if (pet->is_unlocked) {
            printf("?? %s\n", pet->name);
            printf("   类型：");
            switch (pet->type) {
                case PET_CAT: printf("猫咪"); break;
                case PET_DOG: printf("狗狗"); break;
                case PET_PARROT: printf("鹦鹉"); break;
                case PET_DRAGON: printf("龙"); break;
                case PET_PHOENIX: printf("凤凰"); break;
            }
            printf("   等级：%d\n", pet->level);
            printf("   快乐度："); print_progress_bar(pet->happiness, 100, 20);
            printf("   饱食度："); print_progress_bar(pet->hunger, 100, 20);
            printf("   加成倍数：%.1f\n\n", pet->bonus_multiplier);
        } else {
            printf("?? 未解锁宠物\n");
            printf("   解锁条件：");
            switch (i) {
                case 1: printf("达到等级5"); break;
                case 2: printf("拥有10万坤币"); break;
                case 3: printf("解锁30个成就"); break;
                case 4: printf("击败5次BOSS"); break;
                default: printf("未知"); break;
            }
            printf("\n\n");
        }
    }
    
    draw_separator();
    printf("1. 喂食宠物   2. 玩耍   3. 领养新宠物   0. 返回\n");
    draw_separator();
    
    int choice = get_valid_input(0, 3);
    if (choice > 0 && player.pet_count > 0) {
        printf("选择宠物 (1-%d): ", player.pet_count);
        int pet_choice = get_valid_input(1, player.pet_count);
        
        switch (choice) {
            case 1:
                feed_pet(pet_choice - 1);
                break;
            case 2:
                //play_with_pet函数需要实现
                print_colored(COLOR_GREEN, "和宠物玩耍，快乐度增加！\n");
                break;
            case 3:
                //adopt_pet函数需要实现
                print_colored(COLOR_GREEN, "成功领养新宠物！\n");
                break;
        }
    }
    press_any_key();
}

void show_company() {
    CLEAR_SCREEN();
    draw_box("公司管理");
    
    CompanyLevel *current = &player.company_levels[player.current_company_level];
    
    printf(COLOR_BOLD "当前公司：%s\n" COLOR_RESET, current->name);
    printf("   %s\n", current->description);
    printf("   收入倍数：%.1f\n", current->income_multiplier);
    
    if (player.current_company_level < MAX_COMPANY_LEVELS - 1) {
        CompanyLevel *next = &player.company_levels[player.current_company_level + 1];
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
    draw_box("股票市场");
    
    //获取当前时间并显示
    time_t now = time(NULL);
    printf("当前时间：%s", ctime(&now));
    printf("\n");
    
    for (int i = 0; i < 5; i++) {
        Stock *stock = &player.stocks[i];
        
        //更新股票价格（模拟市场波动）
        if (now - stock->last_update > 60) {
            int change = (rand() % (stock->volatility * 2)) - stock->volatility;
            stock->current_price += change;
            if (stock->current_price < 10) stock->current_price = 10;
            stock->price_change = change;
            stock->last_update = now;
        }
        
        printf("%s\n", stock->name);
        printf("   当前价格：%d 坤币\n", stock->current_price);
        printf("   涨跌：%+d\n", stock->price_change);
        printf("   波动率：%d\n", stock->volatility);
        printf("   持有数量：%d\n", player.invested_stocks[i]);
        if (player.invested_stocks[i] > 0) {
            int total_value = player.invested_stocks[i] * stock->current_price;
            printf("   当前价值：%d 坤币\n", total_value);
        }
        printf("\n");
    }
    
    draw_separator();
    printf("1. 购买股票   2. 出售股票   3. 查看历史   0. 返回\n");
    draw_separator();
    
    int choice = get_valid_input(0, 3);
    if (choice == 1 || choice == 2) {
        printf("选择股票 (1-5): ");
        int stock_choice = get_valid_input(1, 5);
        printf("输入数量: ");
        int amount = get_valid_input(1, 1000);
        
        if (choice == 1) {
            buy_stock(stock_choice - 1, amount);
        } else {
            //sell_stock函数需要实现
            print_colored(COLOR_GREEN, "成功出售股票！\n");
        }
    }
    press_any_key();
}

void show_properties() {
    CLEAR_SCREEN();
    draw_box("房产投资");
    
    printf("已拥有房产：%d/10\n\n", player.property_count);
    
    for (int i = 0; i < 10; i++) {
        Property *prop = &available_properties[i];
        
        bool owned = false;
        for (int j = 0; j < player.property_count; j++) {
            if (strcmp(player.properties[j].name, prop->name) == 0) {
                owned = true;
                break;
            }
        }
        
        if (owned) {
            printf(COLOR_GREEN "? %s\n" COLOR_RESET, prop->name);
            printf("   %s\n", prop->description);
            printf("   被动收入：%d 坤币/天\n", (int)prop->passive_income);
        } else {
            printf(COLOR_WHITE "● %s\n" COLOR_RESET, prop->name);
            printf("   %s\n", prop->description);
            printf("   价格：%lld 坤币\n", prop->coins_needed);
            printf("   建造时间：%d 天\n", prop->days_needed);
            printf("   预计收入：%d 坤币/天\n", (int)prop->passive_income);
        }
        printf("\n");
    }
    
    draw_separator();
    printf("1. 购买房产   2. 查看收益   0. 返回\n");
    draw_separator();
    
    int choice = get_valid_input(0, 2);
    if (choice == 1) {
        printf("选择房产 (1-10): ");
        int prop_choice = get_valid_input(1, 10);
        //buy_property函数需要实现
        print_colored(COLOR_GREEN, "成功购买房产！\n");
    }
    press_any_key();
}

void show_prestige_menu() {
    CLEAR_SCREEN();
    draw_box("声望系统");
    
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
        
        bool can_prestige = player.level >= 50 && 
                           player.kun_coins >= 1000000;
        int unlocked_achievements = 0;
        for (int i = 0; i < MAX_ACHIEVEMENTS; i++) {
            if (player.achievements[i].unlocked) unlocked_achievements++;
        }
        can_prestige = can_prestige && (unlocked_achievements >= 20);
        
        if (can_prestige) {
            print_colored(COLOR_GREEN, "\n? 满足声望重置条件！\n");
        } else {
            print_colored(COLOR_RED, "\n? 不满足声望重置条件\n");
            printf("   需要：等级50，100万坤币，20个成就\n");
        }
    } else {
        printf("声望等级 %d 奖励：\n", player.prestige_level);
        printf("   - 所有收入增加 %d%%\n", player.prestige_level * 5);
        printf("   - 技能升级成本减少 %d%%\n", player.prestige_level * 2);
        printf("   - 每日登录奖励增加 %d%%\n", player.prestige_level * 10);
        printf("\n");
        
        printf("下一等级需要：%d 声望点数\n", player.prestige_level * 1000);
    }
    
    draw_separator();
    if (player.prestige_level == 0) {
        printf("1. 重置声望（重新开始）   0. 返回\n");
        int choice = get_valid_input(0, 1);
        if (choice == 1) {
            //prestige函数需要实现
            print_colored(COLOR_GREEN, "声望重置成功！\n");
        }
    } else {
        printf("1. 兑换声望点数   0. 返回\n");
        int choice = get_valid_input(0, 1);
        if (choice == 1) {
            printf("兑换点数将用于购买特殊物品...\n");
        }
    }
    press_any_key();
}

void show_main_menu() {
    CLEAR_SCREEN();
    
    //显示ASCII艺术标题
    printf(COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║    ███████╗██╗    ██╗ █████╗ ██╗  ██╗███████╗███╗   ███╗     ║\n");
    printf("║    ██╔════╝██║    ██║██╔══██╗██║ ██╔╝██╔════╝████╗ ████║     ║\n");
    printf("║    █████╗  ██║ █╗ ██║███████║█████╔╝ █████╗  ██╔████╔██║     ║\n");
    printf("║    ██╔══╝  ██║███╗██║██╔══██║██╔═██╗ ██╔══╝  ██║╚██╔╝██║     ║\n");
    printf("║    ██║     ╚███╔███╔╝██║  ██║██║  ██╗███████╗██║ ╚═╝ ██║     ║\n");
    printf("║    ╚═╝      ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝     ║\n");
    printf("║                                                              ║\n");
    printf("║                 打工模拟器 v2.0 - 豪华版                      ║\n");
    printf("║                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    printf("\n");
    
    //显示玩家状态摘要
    printf(COLOR_BOLD "?? 玩家状态：" COLOR_RESET);
    printf("等级 %d | 坤币 %d | 能量 %d/%d\n\n", 
           player.level, player.kun_coins, player.energy, player.max_energy);
    
    draw_separator();
    printf(COLOR_BOLD "?? 主菜单\n" COLOR_RESET);
    printf("1.  ?? 查看状态\n");
    printf("2.  ?? 打工赚钱\n");
    printf("3.  ?? 商城购物\n");
    printf("4.  ?? 技能系统\n");
    printf("5.  ?? 背包物品\n");
    printf("6.  ?? 任务列表\n");
    printf("7.  ?? 宠物系统\n");
    printf("8.  ?? 公司管理\n");
    printf("9.  ?? 股票市场\n");
    printf("10. ?? 房产投资\n");
    printf("11. ?? 成就系统\n");
    printf("12. ? 声望系统\n");
    printf("13. ??  游戏设置\n");
    printf("14. ?? 保存游戏\n");
    printf("15. ? 帮助指南\n");
    printf("0.  ?? 退出游戏\n");
    draw_separator();
    
    //显示当前事件
    show_active_events();
}

//================ 其他功能函数 ================
void use_inventory_item(int item_index) {
    if (item_index < 0 || item_index >= player.inventory_count) return;
    
    InventoryItem *item = &player.inventory[item_index];
    
    if (item->quantity <= 0) {
        print_colored(COLOR_RED, "? 物品数量不足！\n");
        return;
    }
    
    item->quantity--;
    
    if (strcmp(item->name, "能量饮料") == 0) {
        player.energy += 20;
        if (player.energy > player.max_energy) player.energy = player.max_energy;
        print_colored(COLOR_GREEN, "? 使用能量饮料，恢复20点能量！\n");
    } else if (strcmp(item->name, "新手装备") == 0) {
        print_colored(COLOR_GREEN, "? 装备新手装备，任务收入增加5%%！\n");
    }
    
    //如果物品用完，从背包移除
    if (item->quantity <= 0) {
        for (int i = item_index; i < player.inventory_count - 1; i++) {
            player.inventory[i] = player.inventory[i + 1];
        }
        player.inventory_count--;
    }
}

//================ 主程序 ================
int main() {
    enable_ansi();
    show_loading_screen(2000);
    
    init_game();
    
    while (1) {
        show_main_menu();
        
        printf("请输入选项: ");
        int choice = get_valid_input(0, 15);
        
        switch (choice) {
            case 1:
                show_status();
                press_any_key();
                break;
                
            case 2: { //打工赚钱
                CLEAR_SCREEN();
                draw_box("任务中心");
                
                printf("可用任务：\n\n");
                for (int i = 0; i < MAX_TASKS; i++) {
                    Task *task = &tasks[i];
                    
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
                
            case 3: { //商城购物
                CLEAR_SCREEN();
                draw_box("商城");
                
                printf("可用物品：\n\n");
                for (int i = 0; i < MAX_MARKET; i++) {
                    MarketItem *item = &market[i];
                    
                    printf("%d. %s\n", i + 1, item->name);
                    
                    //显示稀有度颜色
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
                    
                    if (item->is_limited) {
                        time_t remaining = item->available_until - time(NULL);
                        if (remaining > 0) {
                            int days = remaining / 86400;
                            printf("   ? 限定物品，剩余 %d 天\n", days);
                        }
                    }
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
                
            case 13: { //游戏设置
                CLEAR_SCREEN();
                draw_box("游戏设置");
                
                printf("1. 显示设置\n");
                printf("2. 声音设置\n");
                printf("3. 游戏难度\n");
                printf("4. 重置进度\n");
                printf("5. 关于游戏\n");
                printf("0. 返回\n");
                draw_separator();
                
                int setting_choice = get_valid_input(0, 5);
                if (setting_choice == 4) {
                    printf("确定要重置游戏进度吗？(y/n): ");
                    char confirm = getchar();
                    if (confirm == 'y' || confirm == 'Y') {
                        init_player();
                        print_colored(COLOR_GREEN, "游戏进度已重置！\n");
                    }
                    clear_input_buffer();
                }
                press_any_key();
                break;
            }
                
            case 14:
                save_game();
                press_any_key();
                break;
                
            case 15: { //帮助指南
                CLEAR_SCREEN();
                draw_box("游戏指南");
                
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
                printf("? 能量随时间恢复\n");
                printf("? 使用物品恢复能量\n\n");
                
                printf(COLOR_BOLD "?? 特色功能\n" COLOR_RESET);
                printf("? 季节系统：不同季节有不同加成\n");
                printf("? 宠物系统：宠物提供额外加成\n");
                printf("? 公司经营：升级公司获得被动收入\n");
                printf("? 股票市场：投资股票获得收益\n");
                printf("? 房产投资：购买房产获得稳定收入\n");
                printf("? 声望系统：重置游戏获得永久加成\n\n");
                
                draw_separator();
                printf("按任意键返回...");
                getchar();
                break;
            }
                
            case 0:
                CLEAR_SCREEN();
                printf("感谢游玩打工模拟器 v2.0！\n");
                printf("记得常回来打工哦！\n");
                save_game();
                return 0;
        }
        
        //处理随机事件
        random_event();
        
        //自动恢复能量（每10分钟恢复1点）
        static time_t last_energy_recovery = 0;
        time_t now = time(NULL);
        if (now - last_energy_recovery >= 600) { //10分钟
            if (player.energy < player.max_energy) {
                player.energy++;
                last_energy_recovery = now;
            }
        }
        
        //更新游戏时间
        static time_t last_update = 0;
        if (last_update == 0) last_update = now;
        player.total_play_time += (int)(now - last_update);
        last_update = now;
    }
    
    return 0;
}
