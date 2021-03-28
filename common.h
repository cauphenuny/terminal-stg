#ifndef COMMON_H
#define COMMON_H

#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wunused-result"

const char* version = (char*)"v2.5.9";

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>


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
    fprintf(stderr, \
    "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_BLUE "m[LOG] \033[0m" \
    "\033[" VT100_STYLE_DARK ";" VT100_COLOR_NORMAL "m%s:%d: \033[0m" \
    fmt "\n", __func__, __LINE__, ## __VA_ARGS__)

#define logw(fmt, ...) \
    fprintf(stderr, \
    "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_YELLOW "m[WARN] \033[0m" \
    "\033[" VT100_STYLE_DARK ";" VT100_COLOR_NORMAL "m%s:%d: \033[0m" \
    fmt "\n", __func__, __LINE__, ## __VA_ARGS__)

#define loge(fmt, ...) \
    fprintf(stderr, \
    "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_RED "m[ERROR] \033[0m" \
    "\033[" VT100_STYLE_DARK ";" VT100_COLOR_NORMAL "m%s:%d: \033[0m" \
    fmt "\n", __func__, __LINE__, ## __VA_ARGS__)

// inner log
#define logi(fmt, ...) \
    fprintf(stderr, \
    "\033[" VT100_STYLE_NORMAL ";" VT100_COLOR_BLUE "m[LOG] \033[0m" \
    "\033[" VT100_STYLE_DARK ";" VT100_COLOR_NORMAL "m%s:%d: \033[0m" \
    "==> " fmt "\n", __func__, __LINE__, ## __VA_ARGS__)

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


struct pos_t {
    uint8_t x;
    uint8_t y; 
};

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
            uint16_t life, index, bullets_num, color;
            pos_t user_pos[USER_CNT];
            uint8_t user_color[USER_CNT];
            uint8_t map[BATTLE_H][BATTLE_W / 2 + 1];
            //pos_t item_pos[MAX_ITEM];
            //uint8_t item_kind[MAX_ITEM];
        };

        struct {
            char name[USERNAME_SIZE];
            uint8_t namecolor;
            uint8_t kill;
            uint8_t death;
            uint8_t score;
            uint16_t life;
        } users[USER_CNT];

        struct {
            char from_user[USERNAME_SIZE];
            char msg[MSG_SIZE];
        }; // for message
    };
} server_message_t;

enum {
    CLIENT_COMMAND_USER_QUIT,
    CLIENT_COMMAND_USER_REGISTER,
    CLIENT_COMMAND_USER_LOGIN,
    CLIENT_COMMAND_USER_LOGOUT,
    CLIENT_MESSAGE_FATAL,
    CLIENT_COMMAND_FETCH_ALL_USERS,
    CLIENT_COMMAND_FETCH_ALL_FRIENDS,
    CLIENT_COMMAND_LAUNCH_BATTLE,
    CLIENT_COMMAND_LAUNCH_FFA,
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
    SERVER_MESSAGE_BATTLE_PLAYER,
    SERVER_MESSAGE_YOU_ARE_DEAD,
    SERVER_MESSAGE_YOU_ARE_SHOOTED,
    SERVER_MESSAGE_YOU_ARE_TRAPPED_IN_MAGMA,
    SERVER_MESSAGE_YOU_GOT_BLOOD_VIAL,
    SERVER_MESSAGE_YOU_GOT_MAGAZINE,
    SERVER_MESSAGE_YOUR_MAGAZINE_IS_EMPTY,

    SERVER_STATUS_QUIT,
    SERVER_STATUS_FATAL,
};

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

// output colored text
//   for example:
//     printf("\033[{style};{color}mHello World!\n\033[0m");
//
//     where {style} is interger range from 0 to 4, {color} is a
//     interger range from 30 to 38
//
//
// 256 color support:
//   control code: "\033[48;5;{color}m" or "\033[38;5;{color}m"
//
//   where {color} is a interger range from 0 to 255, see more
//   details in 256-colors.py
//
//
// cursor move:
//   control code: "\033"  (\033 is ascii code of <esc>)
//
//   cursor up:             "\033[{count}A"
//        moves the cursor up by count rows;
//        the default count is 1.
//
//     cursor down:           "\033[{count}B"
//        moves the cursor down by count rows;
//        the default count is 1.
//
//     cursor forward:        "\033[{count}C"
//        moves the cursor forward by count columns;
//        the default count is 1.
//
//     cursor backward:       "\033[{count}D"
//        moves the cursor backward by count columns;
//        the default count is 1.
//
//     set cursor position:   "\033[{row};{column}f"
//

#endif
