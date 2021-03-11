#ifndef COMMON_H
#define COMMON_H

#define VERSION "v2.0.0"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif
/* some special characters(terminal graph):
 *
 *  ▁ ▂ ▃ ▄ ▅ ▆ ▇ █ ▊ ▌ ▎ ▖ ▗ ▘ ▙ ▚ ▛ ▜ ▝ ▞ ▟ ━ ┃
 *
 *  ┏ ┓ ┗ ┛ ┣ ┫ ┳ ┻ ╋ ╸ ╹ ╺
 *
 *  ╻ ╏ ─ ─ │ │ ╴ ╴ ╵ ╵ ╶ ╶ ╵ ╵ ⎢ ⎥ ⎺ ⎻ ⎼ ⎽ ◾ ▪ ╱ ╲ ╳ 
 *
 *  ■ □ ★ ☆ ◆ ◇ ◣ ◢  ◤ ◥ ▲ △ ▼ ▽ ⊿ 
 *
 *  ┌─┬─┐
 *  │ │ │
 *  ├─┼─┤
 *  │ │ │
 *  └─┴─┘
 *
 * */

/* output colored text
 *   for example:
 *     printf("\033[{style};{color}mHello World!\n\033[0m");
 *
 *     where {style} is interger range from 0 to 4, {color} is a
 *     interger range from 30 to 38
 *
 *
 * 256 color support:
 *   control code: "\033[48;5;{color}m" or "\033[38;5;{color}m"
 *
 *   where {color} is a interger range from 0 to 255, see more
 *   details in 256-colors.py
 *
 *
 * cursor move:
 *   control code: "\033"  (\033 is ascii code of <esc>)
 *
 *   cursor up:             "\033[{count}A"
 *        moves the cursor up by count rows;
 *        the default count is 1.
 *
 *     cursor down:           "\033[{count}B"
 *        moves the cursor down by count rows;
 *        the default count is 1.
 *
 *     cursor forward:        "\033[{count}C"
 *        moves the cursor forward by count columns;
 *        the default count is 1.
 *
 *     cursor backward:       "\033[{count}D"
 *        moves the cursor backward by count columns;
 *        the default count is 1.
 *
 *     set cursor position:   "\033[{row};{column}f"
 */

/* output style: 16 color mode */
#define VT100_STYLE_NORMAL     "0"
#define VT100_STYLE_BOLD       "1"
#define VT100_STYLE_DARK       "2"
#define VT100_STYLE_BACKGROUND "3"
#define VT100_STYLE_UNDERLINE  "4"

/* output color: 16 color mode */
#define VT100_COLOR_BLACK    "30"
#define VT100_COLOR_RED      "31"
#define VT100_COLOR_GREEN    "32"
#define VT100_COLOR_YELLOW   "33"
#define VT100_COLOR_BLUE     "34"
#define VT100_COLOR_PURPLE   "35"
#define VT100_COLOR_SKYBLUE  "36"
#define VT100_COLOR_WHITE    "37"
#define VT100_COLOR_NORMAL   "38"


#define log(fmt, ...) \
    fprintf(stderr, "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_BLUE "m[LOG] \033[0m" "%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

// inner log
#define logi(fmt, ...) \
    fprintf(stderr, "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_BLUE "m[LOG] \033[0m" "%s:%d: ==> " fmt, __func__, __LINE__, ## __VA_ARGS__)

#define loge(fmt, ...) \
    fprintf(stderr, "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_RED "m[ERROR] \033[0m" "%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

#define eprintf(...) do { \
    loge(__VA_ARGS__); \
    exit(0); \
} while(0)

/* detects the width and height of local screen firstly by `ioctl`
 *
 * usage of ioctl:
 *     struct winsize ws;
 *     ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
 *     ws.ws_col;
 *     ws.ws_row;
 */

#define SCR_W 79
#define SCR_H 23

//#define BATTLE_W (SCR_W)
//#define BATTLE_H (SCR_H - 2)
#define BATTLE_W 60
#define BATTLE_H (SCR_H - 2)

#define FFA_MAP_W BATTLE_W
#define FFA_MAP_H BATTLE_H

#define IPADDR_SIZE 24
#define USERNAME_SIZE  12
#define MSG_SIZE 40
#define USER_CNT   10

#define PASSWORD_SIZE USERNAME_SIZE

#define INIT_BULLETS 12
#define MAX_BULLETS 240
#define BULLETS_PER_MAGAZINE 12

#define INIT_LIFE 10
#define MAX_LIFE 20
#define LIFE_PER_VIAL 5

#define INIT_GRASS 5

#define MAGMA_INIT_TIMES 3
#define MAX_OTHER 30

#define BULLETS_LASTS_TIME 500
#define OTHER_ITEM_LASTS_TIME 1000
#define GLOBAL_SPEED 30000

#define MAX_ITEM (USER_CNT * (MAX_BULLETS) + MAX_OTHER)

enum {
    CLIENT_COMMAND_USER_QUIT,
    CLIENT_COMMAND_USER_REGISTER,
    CLIENT_COMMAND_USER_LOGIN,
    CLIENT_COMMAND_USER_LOGOUT,
    CLIENT_MESSAGE_FATAL,
    CLIENT_COMMAND_FETCH_ALL_USERS,
    CLIENT_COMMAND_FETCH_ALL_FRIENDS,
    CLIENT_COMMAND_LAUNCH_BATTLE,
    CLIENT_COMMAND_ENTER_FFA,
    CLIENT_COMMAND_QUIT_BATTLE,
    CLIENT_COMMAND_ACCEPT_BATTLE,
    CLIENT_COMMAND_REJECT_BATTLE,
    CLIENT_COMMAND_INVITE_USER,
    CLIENT_COMMAND_SEND_MESSAGE,
    CLIENT_COMMAND_MOVE_UP,
    CLIENT_COMMAND_MOVE_DOWN,
    CLIENT_COMMAND_MOVE_LEFT,
    CLIENT_COMMAND_MOVE_RIGHT,
    CLIENT_COMMAND_FIRE_UP,
    CLIENT_COMMAND_FIRE_DOWN,
    CLIENT_COMMAND_FIRE_LEFT,
    CLIENT_COMMAND_FIRE_RIGHT,
    CLIENT_COMMAND_FIRE_UP_LEFT,
    CLIENT_COMMAND_FIRE_UP_RIGHT,
    CLIENT_COMMAND_FIRE_DOWN_LEFT,
    CLIENT_COMMAND_FIRE_DOWN_RIGHT,
    CLIENT_COMMAND_FIRE_AOE_UP,
    CLIENT_COMMAND_FIRE_AOE_DOWN,
    CLIENT_COMMAND_FIRE_AOE_LEFT,
    CLIENT_COMMAND_FIRE_AOE_RIGHT,
    CLIENT_COMMAND_ADMIN_CONTROL,
    CLIENT_COMMAND_END,
};

enum {
    SERVER_SAY_NOTHING,
    SERVER_RESPONSE_REGISTER_SUCCESS,
    SERVER_RESPONSE_REGISTER_FAIL,
    SERVER_RESPONSE_YOU_HAVE_REGISTERED,
    SERVER_RESPONSE_LOGIN_SUCCESS,
    SERVER_RESPONSE_YOU_HAVE_LOGINED,
    SERVER_RESPONSE_YOU_HAVE_NOT_LOGIN,
    SERVER_RESPONSE_LOGIN_FAIL_UNREGISTERED_USERID,
    SERVER_RESPONSE_LOGIN_FAIL_ERROR_PASSWORD,
    SERVER_RESPONSE_LOGIN_FAIL_DUP_USERID,    // user id has been registered by other users
    SERVER_RESPONSE_LOGIN_FAIL_SERVER_LIMITS, // server unable to handle more users
    SERVER_RESPONSE_ALL_USERS_INFO,
    SERVER_RESPONSE_ALL_FRIENDS_INFO,
    SERVER_RESPONSE_LAUNCH_BATTLE_FAIL,
    SERVER_RESPONSE_LAUNCH_BATTLE_SUCCESS,
    SERVER_RESPONSE_YOURE_NOT_IN_BATTLE,
    SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE,
    SERVER_RESPONSE_INVITATION_SENT,
    SERVER_RESPONSE_NOBODY_INVITE_YOU,
    /* ----------------------------------------------- */
    SERVER_MESSAGE,
    SERVER_MESSAGE_DELIM,
    SERVER_MESSAGE_FRIEND_LOGIN,
    SERVER_MESSAGE_FRIEND_LOGOUT,
    SERVER_MESSAGE_FRIEND_ACCEPT_BATTLE,
    SERVER_MESSAGE_FRIEND_REJECT_BATTLE,
    SERVER_MESSAGE_FRIEND_NOT_LOGIN,
    SERVER_MESSAGE_FRIEND_ALREADY_IN_BATTLE,
    SERVER_MESSAGE_INVITE_TO_BATTLE,
    SERVER_MESSAGE_FRIEND_MESSAGE,
    SERVER_MESSAGE_USER_QUIT_BATTLE,
    SERVER_MESSAGE_BATTLE_DISBANDED,          // since no other users in this battle
    SERVER_MESSAGE_BATTLE_INFORMATION,
    SERVER_MESSAGE_YOU_ARE_DEAD,
    SERVER_MESSAGE_YOU_ARE_SHOOTED,
    SERVER_MESSAGE_YOU_ARE_TRAPPED_IN_MAGMA,
    SERVER_MESSAGE_YOU_GOT_BLOOD_VIAL,
    SERVER_MESSAGE_YOU_GOT_MAGAZINE,
    SERVER_MESSAGE_YOUR_MAGAZINE_IS_EMPTY,

    SERVER_STATUS_QUIT,
    SERVER_STATUS_FATAL,
};

enum {
    ITEM_NONE,
    ITEM_MAGAZINE,
    ITEM_MAGMA,
    ITEM_GRASS,
    ITEM_BLOOD_VIAL,
    ITEM_END,
    ITEM_BULLET,
};

enum {
    MAP_ITEM_NONE,
    MAP_ITEM_MAGAZINE,
    MAP_ITEM_MAGMA,
    MAP_ITEM_GRASS,
    MAP_ITEM_BLOOD_VIAL,
    MAP_ITEM_END,
    MAP_ITEM_MY_BULLET,
    MAP_ITEM_OTHER_BULLET,
    MAP_ITEM_USER,
};

enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP_LEFT,
    DIR_UP_RIGHT,
    DIR_DOWN_LEFT,
    DIR_DOWN_RIGHT,
};

enum {
    BATTLE_STATE_UNJOINED,
    BATTLE_STATE_LIVE,
    BATTLE_STATE_WITNESS,
    BATTLE_STATE_DEAD,
};

/* unused  -->  not login  -->  login  <-...->  battle
 *
 * unused  -->  not login  -->  unused  // login fail
 *
 * login  <-->  invited to battle  <-->  battle
 * */
#define USER_STATE_UNUSED          0
#define USER_STATE_NOT_LOGIN       1
#define USER_STATE_LOGIN           2
#define USER_STATE_BATTLE          3
#define USER_STATE_WAIT_TO_BATTLE  4

typedef struct pos_t {
    uint8_t x;
    uint8_t y; 
} pos_t;

// format of messages sended from client to server
typedef struct client_message_t {
    uint8_t command;
    char user_name[USERNAME_SIZE]; // last byte must be zero
    union
    {
        char message[MSG_SIZE];
        char password[PASSWORD_SIZE];
    };
} client_message_t;

// format of messages sended from server to client
typedef struct server_message_t {
    union {
        uint8_t response;
        uint8_t message;
    };

    union {
        // support at most five users
        char friend_name[USERNAME_SIZE];

        struct {
            char user_name[USERNAME_SIZE];
            uint8_t user_state;
        } all_users[USER_CNT];

        struct {
            uint8_t life, index, bullets_num, color;
            pos_t user_pos[USER_CNT];
            uint8_t user_color[USER_CNT];
            uint8_t map[BATTLE_H][BATTLE_W / 2 + 1];
            //pos_t item_pos[MAX_ITEM];
            //uint8_t item_kind[MAX_ITEM];
        };

        struct {
            char from_user[USERNAME_SIZE];
            char msg[MSG_SIZE];
        }; // for message
    };
} server_message_t;

#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"
#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"
#define NONE                 "\e[0m"

char* signal_name_s[128];
char* color_s[128];
char*  map_s[128];
int item_to_map[128];
int color_s_size;

void init_constants() {
    map_s[MAP_ITEM_NONE] = (char*)" ";
    map_s[MAP_ITEM_MAGAZINE] = (char*)"+";
    map_s[MAP_ITEM_MAGMA] = (char*)"X";
    map_s[MAP_ITEM_GRASS] = (char*)"█";
    map_s[MAP_ITEM_BLOOD_VIAL] = (char*)"*";
    map_s[MAP_ITEM_END] = (char*)" ";
    map_s[MAP_ITEM_MY_BULLET] = (char*)".";
    map_s[MAP_ITEM_OTHER_BULLET] = (char*)".";

    item_to_map[ITEM_NONE] = MAP_ITEM_NONE;
    item_to_map[ITEM_MAGAZINE] = MAP_ITEM_MAGAZINE;
    item_to_map[ITEM_MAGMA] = MAP_ITEM_MAGMA;
    item_to_map[ITEM_GRASS] = MAP_ITEM_GRASS;
    item_to_map[ITEM_BLOOD_VIAL] = MAP_ITEM_BLOOD_VIAL;
    item_to_map[ITEM_END] = MAP_ITEM_END;

    signal_name_s[SIGHUP   ] = (char*)"SIGHUP";
    signal_name_s[SIGINT   ] = (char*)"SIGINT";
    signal_name_s[SIGQUIT  ] = (char*)"SIGQUIT";
    signal_name_s[SIGILL   ] = (char*)"SIGILL" ;
    signal_name_s[SIGABRT  ] = (char*)"SIGABRT";
    signal_name_s[SIGFPE   ] = (char*)"SIGFPE" ;
    signal_name_s[SIGKILL  ] = (char*)"SIGKILL";
    signal_name_s[SIGSEGV  ] = (char*)"SIGSEGV";
    signal_name_s[SIGPIPE  ] = (char*)"SIGPIPE";
    signal_name_s[SIGALRM  ] = (char*)"SIGALRM";
    signal_name_s[SIGTERM  ] = (char*)"SIGTERM";
    signal_name_s[SIGUSR1  ] = (char*)"SIGUSR1";
    signal_name_s[SIGUSR2  ] = (char*)"SIGUSR2";
    signal_name_s[SIGCHLD  ] = (char*)"SIGCHLD";
    signal_name_s[SIGCONT  ] = (char*)"SIGCONT";
    signal_name_s[SIGSTOP  ] = (char*)"SIGSTOP";
    signal_name_s[SIGTSTP  ] = (char*)"SIGTSTP";
    signal_name_s[SIGTTIN  ] = (char*)"SIGTTIN";
    signal_name_s[SIGTTOU  ] = (char*)"SIGTTOU";
    signal_name_s[SIGBUS   ] = (char*)"SIGBUS" ;
    signal_name_s[SIGPOLL  ] = (char*)"SIGPOLL";
    signal_name_s[SIGPROF  ] = (char*)"SIGPROF";
    signal_name_s[SIGSYS   ] = (char*)"SIGSYS" ;
    signal_name_s[SIGTRAP  ] = (char*)"SIGTRAP";
    signal_name_s[SIGURG   ] = (char*)"SIGURG" ;
    signal_name_s[SIGVTALRM] = (char*)"SIGVTALRM";
    signal_name_s[SIGXCPU  ] = (char*)"SIGXCPU";
    signal_name_s[SIGXFSZ  ] = (char*)"SIGXFSZ";
    color_s[0] = (char*)NONE;
    color_s[1] = (char*)RED;
    color_s[2] = (char*)GREEN;
    color_s[3] = (char*)BROWN;
    color_s[4] = (char*)BLUE;
    color_s[5] = (char*)PURPLE;
    color_s[6] = (char*)CYAN;
    color_s[7] = (char*)L_RED;
    color_s[8] = (char*)L_GREEN;
    color_s[9] = (char*)YELLOW;
    color_s[10] = (char*)L_BLUE;
    color_s[11] = (char*)L_PURPLE;
    color_s[12] = (char*)L_CYAN;
    color_s_size = 12;
};


#endif
