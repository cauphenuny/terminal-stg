#ifndef CONSTANTS_H
#define CONSTANTS_H

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
#define MSG_SIZE 96
#define USER_CNT   14

#define PASSWORD_SIZE USERNAME_SIZE

#define INIT_BULLETS 12
#define MAX_BULLETS 480
#define BULLETS_PER_MAGAZINE 12

#define INIT_LIFE 10
#define MAX_LIFE 20
#define LIFE_PER_VIAL 5

#define INIT_GRASS 5

#define MAGMA_INIT_TIMES 3
#define MAX_OTHER 30

#define BULLETS_LASTS_TIME 300
#define OTHER_ITEM_LASTS_TIME 1000
#define GLOBAL_SPEED 20

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
    MAP_ITEM_MY_BULLET,
    MAP_ITEM_OTHER_BULLET,
    MAP_ITEM_MAGAZINE,
    MAP_ITEM_BLOOD_VIAL,
    MAP_ITEM_MAGMA,
    MAP_ITEM_GRASS,
    MAP_ITEM_USER,
    MAP_ITEM_END,
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
    map_s[MAP_ITEM_MY_BULLET] = (char*)".";
    map_s[MAP_ITEM_OTHER_BULLET] = (char*)".";
    map_s[MAP_ITEM_USER] = (char*)"A";
    map_s[MAP_ITEM_END] = (char*)" ";

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
    color_s[1] = (char*)L_GREEN;
    color_s[2] = (char*)L_RED;
    color_s[3] = (char*)YELLOW;
    color_s[4] = (char*)L_BLUE;
    color_s[5] = (char*)L_PURPLE;
    color_s[6] = (char*)L_CYAN;
    color_s[7] = (char*)(RED UNDERLINE);
    color_s[8] = (char*)(GREEN UNDERLINE);
    color_s[9] = (char*)(BROWN UNDERLINE);
    color_s[10] = (char*)(BLUE UNDERLINE);
    color_s[11] = (char*)(PURPLE UNDERLINE);
    color_s[12] = (char*)(CYAN UNDERLINE);
    color_s_size = 12;
};

#endif
