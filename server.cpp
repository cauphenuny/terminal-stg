#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <ctime>
#include <cmath>
#include <set>
#include <vector>
#include <list>

#include "common.h"
#include "func.h"

#define REGISTERED_USER_LIST_SIZE 100

#define REGISTERED_USER_FILE "userlists.log"

using std::multiset;
using std::vector;

pthread_mutex_t userlist_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessions_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t battles_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t default_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t items_lock[USER_CNT];

int server_fd = 0, port = 50000, port_range = 100;

void wrap_recv(int conn, client_message_t* pcm);
void wrap_send(int conn, server_message_t* psm);

void send_to_client(int uid, int message);
void say_to_client(int uid, char* message);
void send_to_client_with_username(int uid, int message, char* user_name);
void close_session(int conn, int message);

void check_user_status(int uid);

void terminate_process(int recved_signal);

static int user_list_size = 0;
static uint64_t sum_delay_time = 0, prev_time;

struct {
    char user_name[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
} registered_user_list[REGISTERED_USER_LIST_SIZE];

struct session_t {
    char user_name[USERNAME_SIZE];
    char ip_addr[IPADDR_SIZE];
    int conn;
    int state;
    int is_admin;
    uint32_t bid;
    uint32_t inviter_id;
    client_message_t cm;
} sessions[USER_CNT];

struct session_args_t {
    int conn;
    char ip_addr[IPADDR_SIZE];
};

typedef struct session_args_t session_args_t;

class item_t { public:
    int id;
    int dir;
    int owner;
    uint64_t time;
    int count;
    int kind;
    pos_t pos;
    item_t(const item_t &it) : id(it.id),
                               dir(it.dir), 
                               owner(it.owner),
                               time(it.time),
                               count(it.count),
                               kind(it.kind),
                               pos(it.pos)
                               {}
    item_t() {
        id = 0;
        dir = owner = time = count = kind = 0;
        pos.x = pos.y = 0;
    }
    friend const bool operator < (const item_t it1, const item_t it2) {
        return it1.time < it2.time;
    }
};

class battle_t { public:
    int is_alloced;
    size_t nr_users;
    class user_t { public:
        int battle_state;
        int nr_bullets;
        int dir;
        int life;
        int kill;
        int death;
        int killby;
        pos_t pos;
        pos_t last_pos;
    } users[USER_CNT];

    int num_of_other;  // number of other alloced item except for bullet
    int item_count;
    uint64_t global_time;

    std::list<item_t> items;

    void reset() {
        is_alloced = nr_users = num_of_other = item_count = 0;
        global_time = 0;
        items.clear();
    }
    battle_t() {
        reset();
    }

} battles[USER_CNT];

void load_user_list() {
    FILE* userlist = fopen(REGISTERED_USER_FILE, "r");
    if (userlist == NULL) {
        log("can not find " REGISTERED_USER_FILE "\n");
        return;
    }
#define LOAD_FAIL                                                                          \
    log("failed to load users, try to delete " REGISTERED_USER_FILE ".\n"),                \
        user_list_size = 0, memset(registered_user_list, 0, sizeof(registered_user_list)), \
        fclose(userlist);
    for (int i = 0; i < REGISTERED_USER_LIST_SIZE; i++) {
        if (fgets(registered_user_list[i].user_name, USERNAME_SIZE, userlist) != NULL) {
            registered_user_list[i].user_name[strlen(registered_user_list[i].user_name) - 1] = 0;
            for (int j = 0; j < i; j++) {
                if (strncmp(registered_user_list[i].user_name, registered_user_list[j].user_name, USERNAME_SIZE - 1) != 0)
                    continue;
                LOAD_FAIL;
                return;
            }
            user_list_size++;
        } else {
            break;
        }
        if (fgets(registered_user_list[i].password, PASSWORD_SIZE, userlist) == NULL) {
            LOAD_FAIL;
            return;
        }
        registered_user_list[i].password[strlen(registered_user_list[i].password) - 1] = 0;
    }
#undef LOAD_FAIL
    //for (int i = 0; i < user_list_size; i++) {
    //    log("loaded user %s\n", registered_user_list[i].user_name);
    //}
    log("loaded %d users from " REGISTERED_USER_FILE ".\n", user_list_size);
    fclose(userlist);
}
void save_user_list() {
    FILE* userlist = fopen(REGISTERED_USER_FILE, "w");
    for (int i = 0; i < user_list_size; i++) {
        fprintf(userlist, "%s\n", registered_user_list[i].user_name);
        fprintf(userlist, "%s\n", registered_user_list[i].password);
    }
    log("saved %d users to " REGISTERED_USER_FILE ".\n", user_list_size);
}

void save_user(int i) {
    FILE* userlist = fopen(REGISTERED_USER_FILE, "a");
    fprintf(userlist, "%s\n", registered_user_list[i].user_name);
    fprintf(userlist, "%s\n", registered_user_list[i].password);
    log("saved users %s to " REGISTERED_USER_FILE ".\n", registered_user_list[i].user_name);
}

int query_session_built(uint32_t uid) {
    assert(uid < USER_CNT);

    if (sessions[uid].state == USER_STATE_UNUSED
        || sessions[uid].state == USER_STATE_NOT_LOGIN) {
        return false;
    } else {
        return true;
    }
}

void inform_all_user_battle_player(int bid);

void user_quit_battle(uint32_t bid, uint32_t uid) {
    assert(bid < USER_CNT && uid < USER_CNT);

    log("user %s@[%s] quit from battle %d(%ld users)\n", sessions[uid].user_name, sessions[uid].ip_addr, bid, battles[bid].nr_users);
    battles[bid].nr_users--;
    battles[bid].users[uid].battle_state = BATTLE_STATE_UNJOINED;
    sessions[uid].state = USER_STATE_LOGIN;

    if (battles[bid].nr_users == 0) {
        // disband battle
        log("disband battle %d\n", bid);
        battles[bid].is_alloced = false;
    } else {
        server_message_t sm;
        sm.message = SERVER_MESSAGE_USER_QUIT_BATTLE;
        strncpy(sm.friend_name, sessions[uid].user_name, USERNAME_SIZE - 1);

        for (int i = 0; i < USER_CNT; i++) {
            if (battles[bid].users[i].battle_state != BATTLE_STATE_UNJOINED) {
                wrap_send(sessions[i].conn, &sm);
            }
        }
    }
}

void user_join_battle_common_part(uint32_t bid, uint32_t uid, uint32_t joined_state) {
    assert(bid < USER_CNT && uid < USER_CNT);

    log("user %s@[%s] join in battle %d(%ld users)\n", sessions[uid].user_name, sessions[uid].ip_addr, bid, battles[bid].nr_users);

    if (joined_state == USER_STATE_BATTLE) {
        battles[bid].nr_users++;
        battles[bid].users[uid].battle_state = BATTLE_STATE_LIVE;
    } else if (joined_state == USER_STATE_WAIT_TO_BATTLE) {
        battles[bid].users[uid].battle_state = BATTLE_STATE_UNJOINED;
    } else {
        loge("check here, other joined_state:%d\n", joined_state);
    }

    battles[bid].users[uid].life = INIT_LIFE;
    battles[bid].users[uid].kill = 0;
    battles[bid].users[uid].death = 0;
    battles[bid].users[uid].nr_bullets = INIT_BULLETS;

    sessions[uid].state = joined_state;
    sessions[uid].bid = bid;
}

void user_join_battle(uint32_t bid, uint32_t uid) {
    int ux = (rand() & 0x7FFF) % BATTLE_W;
    int uy = (rand() & 0x7FFF) % BATTLE_H;
    battles[bid].users[uid].pos.x = ux;
    battles[bid].users[uid].pos.y = uy;
    log("alloc position (%hhu, %hhu) for launcher ##%d %s@[%s]\n",
        ux, uy, uid, sessions[uid].user_name, sessions[uid].ip_addr);

    sessions[uid].state = USER_STATE_BATTLE;

    if (battles[bid].users[uid].battle_state == BATTLE_STATE_UNJOINED) {
        user_join_battle_common_part(bid, uid, USER_STATE_BATTLE);
    }
}

void user_invited_to_join_battle(uint32_t bid, uint32_t uid) {
    if (sessions[uid].state == USER_STATE_WAIT_TO_BATTLE
        && bid != sessions[uid].bid) {
        log("user #%d %s@[%s] rejects old battle #%d since he was invited to a new battle\n",
            uid, sessions[uid].user_name, sessions[uid].ip_addr, sessions[uid].bid);

        send_to_client_with_username(sessions[uid].inviter_id, SERVER_MESSAGE_FRIEND_REJECT_BATTLE, sessions[uid].user_name);
    }

    user_join_battle_common_part(bid, uid, USER_STATE_WAIT_TO_BATTLE);
}

int find_uid_by_user_name(const char* user_name) {
    int ret_uid = -1;
    log("find user %s\n", user_name);
    for (int i = 0; i < USER_CNT; i++) {
        if (query_session_built(i)) {
            if (strncmp(user_name, sessions[i].user_name, USERNAME_SIZE - 1) == 0) {
                ret_uid = i;
                break;
            }
        }
    }

    if (ret_uid == -1) {
        logi("fail\n");
    } else {
        logi("found: #%d %s@[%s]\n", ret_uid, sessions[ret_uid].user_name, sessions[ret_uid].ip_addr);
    }

    return ret_uid;
}

int get_unalloced_battle() {
    int ret_bid = -1;
    pthread_mutex_lock(&battles_lock);
    for (int i = 1; i < USER_CNT; i++) {
        if (battles[i].is_alloced == false) {
            battles[i].reset();
            battles[i].is_alloced = true;
            ret_bid = i;
            break;
        }
    }
    pthread_mutex_unlock(&battles_lock);
    if (ret_bid == -1) {
        loge("check here, returned battle id should not be -1\n");
    } else {
        log("alloc unalloced battle id #%d\n", ret_bid);
    }
    return ret_bid;
}

int get_unused_session() {
    int ret_uid = -1;
    pthread_mutex_lock(&sessions_lock);
    for (int i = 0; i < USER_CNT; i++) {
        if (sessions[i].state == USER_STATE_UNUSED) {
            memset(&sessions[i], 0, sizeof(struct session_t));
            sessions[i].conn = -1;
            sessions[i].state = USER_STATE_NOT_LOGIN;
            ret_uid = i;
            break;
        }
    }
    pthread_mutex_unlock(&sessions_lock);
    if (ret_uid == -1) {
        log("fail to alloc session id\n");
    } else {
        log("alloc unused session id #%d\n", ret_uid);
    }
    return ret_uid;
}

void inform_friends(int uid, int message) {
    server_message_t sm;
    char* user_name = sessions[uid].user_name;
    memset(&sm, 0, sizeof(server_message_t));
    sm.message = message;
    for (int i = 0; i < USER_CNT; i++) {
        if (i == uid || !query_session_built(i))
            continue;
        strncpy(sm.friend_name, user_name, USERNAME_SIZE - 1);
        wrap_send(sessions[i].conn, &sm);
    }
}

void forced_generate_items(int bid, int x, int y, int kind, int count) {
    //if (battles[bid].num_of_other >= MAX_OTHER) return;
    battles[bid].item_count++;
    item_t new_item;
    new_item.id = battles[bid].item_count;
    new_item.kind = kind;
    new_item.pos.x = x;
    new_item.pos.y = y;
    new_item.time = battles[bid].global_time + count;
    battles[bid].items.push_back(new_item);
    log("new item: #%dk%d(%d,%d)\n", new_item.id,
        new_item.kind,
        new_item.pos.x,
        new_item.pos.y);
}

void random_generate_items(int bid) {
    int random_kind;
    if (!probability(1, 100)) return;
    if (battles[bid].num_of_other >= MAX_OTHER) return;
    random_kind = rand() % (ITEM_END - 1) + 1;
    if (random_kind == ITEM_BLOOD_VIAL && probability(1, 2))
        random_kind = ITEM_MAGAZINE;
    battles[bid].item_count++;
    item_t new_item;
    new_item.id = battles[bid].item_count;
    new_item.kind = random_kind;
    new_item.pos.x = (rand() & 0x7FFF) % BATTLE_W;
    new_item.pos.y = (rand() & 0x7FFF) % BATTLE_H;
    new_item.time = battles[bid].global_time + OTHER_ITEM_LASTS_TIME;
    battles[bid].num_of_other++;
    log("new item: #%dk%d(%d,%d)\n", new_item.id,
        new_item.kind,
        new_item.pos.x,
        new_item.pos.y);
    if (random_kind == ITEM_MAGMA) {
        new_item.count = MAGMA_INIT_TIMES;
    }
    battles[bid].items.push_back(new_item);
    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state != BATTLE_STATE_LIVE)
            continue;
        check_user_status(i);
    }
}

void move_bullets(int bid) {
    for (auto& cur : battles[bid].items) {
    //for (int i = 0; i < MAX_ITEM; i++) {
        if (cur.kind != ITEM_BULLET)
            continue;
        // log("try to move bullet %d with dir %d\n", i, cur.dir);
        switch (cur.dir) {
            case DIR_UP: {
                if (cur.pos.y > 0) { (cur.pos.y)--; break; }
                else { cur.dir = DIR_DOWN; break;}
            }
            case DIR_DOWN: {
                if (cur.pos.y < BATTLE_H - 1) { (cur.pos.y)++; break; }
                else { cur.dir = DIR_UP; break;}
            }
            case DIR_LEFT: {
                if (cur.pos.x > 0) { (cur.pos.x)--; break; }
                else { cur.dir = DIR_RIGHT; break;}
            }
            case DIR_RIGHT: {
                if (cur.pos.x < BATTLE_W - 1) { (cur.pos.x)++; break; }
                else { cur.dir = DIR_LEFT; break; }
            }
            case DIR_UP_LEFT: {
                if (cur.pos.y > 0) { (cur.pos.y)--; }
                else { cur.dir = DIR_DOWN_LEFT; break; }
                if (cur.pos.x > 1) { (cur.pos.x) -= 2; }
                else { cur.dir = DIR_UP_RIGHT; break; }
                break;
            }
            case DIR_UP_RIGHT: {
                if (cur.pos.y > 0) { (cur.pos.y)--; }
                else { cur.dir = DIR_DOWN_RIGHT; break;}
                if (cur.pos.x < BATTLE_W - 2) { (cur.pos.x) += 2; }
                else { cur.dir = DIR_UP_LEFT; break; }
                break;
            }
            case DIR_DOWN_LEFT: {
                if (cur.pos.y < BATTLE_H - 2) { (cur.pos.y)++; }
                else { cur.dir = DIR_UP_LEFT; break; }
                if (cur.pos.x > 1) { (cur.pos.x) -= 2; }
                else { cur.dir = DIR_DOWN_RIGHT; break; }
                break;
            }
            case DIR_DOWN_RIGHT: {
                if (cur.pos.y < BATTLE_H - 2) { (cur.pos.y)++; }
                else { cur.dir = DIR_UP_RIGHT; break;}
                if (cur.pos.x < BATTLE_W - 2) { (cur.pos.x) += 2; }
                else { cur.dir = DIR_DOWN_LEFT; break; }
                break;
            }
        }
    }
}

void check_user_status(int uid) {
    //log("checking...\n");
    //auto start_time = myclock();
    int bid = sessions[uid].bid;
    int ux = battles[bid].users[uid].pos.x;
    int uy = battles[bid].users[uid].pos.y;
    //for (int i = 0; i < MAX_ITEM; i++) {
    auto& items = battles[bid].items;
    bool if_erased = 0;
    for (auto it = items.begin(); it != items.end();) {

        int ix = it->pos.x;
        int iy = it->pos.y;
        bool is_erased = 0;

        if (battles[bid].users[uid].battle_state != BATTLE_STATE_LIVE) {
            it++;
            continue;
        }

        if (ix == ux && iy == uy) {
            switch (it->kind) {
                case ITEM_MAGAZINE: {
                    battles[bid].users[uid].nr_bullets += BULLETS_PER_MAGAZINE;
                    log("user #%d %s@[%s] is got magazine\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                    if (battles[bid].users[uid].nr_bullets > MAX_BULLETS) {
                        log("user #%d %s@[%s] 's bullets exceeds max value\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                        battles[bid].users[uid].nr_bullets = MAX_BULLETS;
                    }
                    send_to_client(uid, SERVER_MESSAGE_YOU_GOT_MAGAZINE);
                    it = items.erase(it), is_erased = 1, if_erased = 1;
                    break;
                }
                case ITEM_MAGMA: {
                    battles[bid].users[uid].life = max(battles[bid].users[uid].life - 1, 0);
                    battles[bid].users[uid].killby = -1;
                    it->count--;
                    log("user #%d %s@[%s] is trapped in magma\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                    send_to_client(uid, SERVER_MESSAGE_YOU_ARE_TRAPPED_IN_MAGMA);
                    if (it->count <= 0) {
                        log("magma %d is exhausted\n", it->id);
                        battles[bid].num_of_other--;
                        it = items.erase(it), is_erased = 1, if_erased = 1;
                    }
                    break;
                }
                case ITEM_BLOOD_VIAL: {
                    battles[bid].users[uid].life += LIFE_PER_VIAL;
                    log("user #%d %s@[%s] got blood vial\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                    if (battles[bid].users[uid].life > MAX_LIFE) {
                        log("user #%d %s@[%s] life exceeds max value\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                        battles[bid].users[uid].life = MAX_LIFE;
                    }
                    battles[bid].num_of_other--;
                    send_to_client(uid, SERVER_MESSAGE_YOU_GOT_BLOOD_VIAL);
                    it = items.erase(it), is_erased = 1, if_erased = 1;
                    break;
                }
                case ITEM_BULLET: {
                    if (it->owner != uid) {
                        battles[bid].users[uid].life = max(battles[bid].users[uid].life - 1, 0);
                        battles[bid].users[uid].killby = it->owner;
                        log("user #%d %s@[%s] is shooted\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
                        send_to_client(uid, SERVER_MESSAGE_YOU_ARE_SHOOTED);
                        it = items.erase(it), is_erased = 1, if_erased = 1;
                        break;
                    }
                    break;
                }
            }
        }
        if (!is_erased) it++;
    }
    if (if_erased) log("current item size: %ld\n", items.size());
    //auto end_time = myclock();
    //log("completed.\n");
}

void check_who_is_shooted(int bid) {
    //for (int i = 0; i < MAX_ITEM; i++) {
    //log("checking...\n");
    auto& items = battles[bid].items;
    for (auto cur = items.begin(); cur != items.end();) {
        if (cur->kind != ITEM_BULLET) {
            cur++;
            continue;
        }

        int ix = cur->pos.x;
        int iy = cur->pos.y;
        int is_erased = 0;

        for (int j = 0; j < USER_CNT; j++) {
            if (battles[bid].users[j].battle_state != BATTLE_STATE_LIVE)
                continue;
            int ux = battles[bid].users[j].pos.x;
            int uy = battles[bid].users[j].pos.y;

            if (ix == ux && iy == uy && cur->owner != j) {
                battles[bid].users[j].life = max(battles[bid].users[j].life - 1, 0);
                battles[bid].users[j].killby = cur->owner;
                log("user #%d %s@[%s] shooted by #%d %s\n",
                    j, sessions[j].user_name, sessions[j].ip_addr, 
                    cur->owner, sessions[cur->owner].user_name);
                send_to_client(j, SERVER_MESSAGE_YOU_ARE_SHOOTED);
                cur = items.erase(cur), is_erased = 1;
                break;
            }
        }
        if (!is_erased) cur++;
    }
    //log("completed.\n");
}

void check_who_is_dead(int bid) {
    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state == BATTLE_STATE_LIVE
            && battles[bid].users[i].life <= 0) {
            log("user #%d %s@[%s] is dead\n", i, sessions[i].user_name, sessions[i].ip_addr);
            battles[bid].users[i].battle_state = BATTLE_STATE_DEAD;
            log("send dead info to user #%d %s@[%s]\n", i, sessions[i].user_name, sessions[i].ip_addr);
            send_to_client(i, SERVER_MESSAGE_YOU_ARE_DEAD);
            battles[bid].users[i].death++;
            log("death of user #%d %s@[%s]: %d\n", i, sessions[i].user_name, sessions[i].ip_addr, battles[bid].users[i].death);
            if (battles[bid].users[i].killby != -1) {
                int by = battles[bid].users[i].killby;
                battles[bid].users[by].kill++;
                log("kill of user #%d %s@[%s]: %d\n", i, sessions[by].user_name, sessions[by].ip_addr, battles[bid].users[by].kill);
            }
        } else if (battles[bid].users[i].battle_state == BATTLE_STATE_DEAD) {
            battles[bid].users[i].battle_state = BATTLE_STATE_WITNESS;
            battles[bid].users[i].nr_bullets = 0;
        }
    }
}

void check_item_count(int bid) {
    //log("call func %s\n", __func__);
    //for (int i = 0; i < MAX_ITEM; i++) {
    //    if (battles[bid].items[i].times) {
    //        battles[bid].items[i].times--;
    //        if (!battles[bid].items[i].times) {
    //            log("free item #%d\n", i);
    //            battles[bid].items[i].is_used = false;
    //            if (battles[bid].items[i].kind < ITEM_END) {
    //                battles[bid].num_of_other--;
    //            }
    //            //battles[bid].items[i].kind = ITEM_BLANK;
    //        }
    //    }
    //}
    //log("check completed...\n");
    auto& items = battles[bid].items;
    bool if_erased = 0;
    for (auto cur = items.begin(); cur != items.end();) {
        if (cur->time < battles[bid].global_time) {
            if (cur->kind < ITEM_END) {
                battles[bid].num_of_other--;
            }
            log("free item #%d\n", cur->id);
            cur = battles[bid].items.erase(cur), if_erased = 1;
        } else {
            cur++;
        }
    }
    if (if_erased) log("current item size: %ld\n", items.size());
}

void render_map_for_user(int uid, server_message_t* psm) {
    int bid = sessions[uid].bid;
    int map[BATTLE_H][BATTLE_W] = {0};
    int cur, x, y;
    //for (int i = 0, x, y; i < MAX_ITEM; i++) {
    for (auto it : battles[bid].items) {
        x = it.pos.x;
        y = it.pos.y;
        switch (it.kind) {
            case ITEM_BULLET: {
                if (it.owner == uid) {
                    map[y][x] = max(map[y][x], MAP_ITEM_MY_BULLET);
                } else {
                    map[y][x] = max(map[y][x], MAP_ITEM_OTHER_BULLET);
                }
                break;
            }
            default: {
                cur = item_to_map[it.kind];
                map[y][x] = max(map[y][x], cur);
            }
        }
        //sm.item_kind[i] = it.kind;
        //sm.item_pos[i].x = it.pos.x;
        //sm.item_pos[i].y = it.pos.y;
    }
    for (int i = 0; i < BATTLE_H; i++) {
        for (int j = 0; j < BATTLE_W; j += 2) {
            //psm->map[i][j] = map[i][j];
            //if (psm->map[i][j] != 0) log("set item #%d\n", psm->map[i][j]);
            psm->map[i][j >> 1] = (map[i][j]) | (map[i][j + 1] << 4);
        }
    }
}

void inform_all_user_battle_player(int bid) {
    server_message_t sm;
    sm.message = SERVER_MESSAGE_BATTLE_PLAYER;
    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state == BATTLE_STATE_LIVE && 
            battles[bid].users[i].life > 0) {
            strncpy(sm.user_name[i], sessions[i].user_name, USERNAME_SIZE - 1);
            sm.user_namecolor[i] = i % color_s_size + 1;
            sm.user_life[i] = battles[bid].users[i].life;
            sm.user_death[i] = battles[bid].users[i].death;
            sm.user_kill[i] = battles[bid].users[i].kill;
        } else {
            strcpy(sm.user_name[i], (char*)"");
            sm.user_namecolor[i] = 0;
            sm.user_life[i] = 0;
            sm.user_death[i] = 0;
            sm.user_kill[i] = 0;
        }
    }
    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state != BATTLE_STATE_UNJOINED) {
            wrap_send(sessions[i].conn, &sm);
            //log("inform user #%d %s@[%s]\n", i, sessions[i].user_name, sessions[i].ip_addr);
        }
    }
}

void inform_all_user_battle_state(int bid) {
    server_message_t sm;
    sm.message = SERVER_MESSAGE_BATTLE_INFORMATION;
    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state == BATTLE_STATE_LIVE) {
            sm.user_pos[i].x = battles[bid].users[i].pos.x;
            sm.user_pos[i].y = battles[bid].users[i].pos.y;
            sm.user_color[i] = i % color_s_size + 1;
        } else {
            sm.user_pos[i].x = -1;
            sm.user_pos[i].y = -1;
            sm.user_color[i] = 0;
        }
    }

    for (int i = 0; i < USER_CNT; i++) {
        if (battles[bid].users[i].battle_state != BATTLE_STATE_UNJOINED) {
            render_map_for_user(i, &sm);
            sm.index = i;
            sm.life = battles[bid].users[i].life;
            sm.bullets_num = battles[bid].users[i].nr_bullets;
            sm.color = i % color_s_size + 1;
            wrap_send(sessions[i].conn, &sm);
        }
    }
}

void* battle_ruler(void* args) {
    int bid = (int)(uintptr_t)args;
    log("battle ruler for battle #%d\n", bid);
    // FIXME: battle re-alloced before exiting loop
    for (int i = 0; i < INIT_GRASS; i++) {
        forced_generate_items(bid,
                              (rand() & 0x7FFF) % BATTLE_W,
                              (rand() & 0x7FFF) % BATTLE_H,
                              ITEM_GRASS,
                              10000);
    }
    inform_all_user_battle_player(bid);
    uint64_t  t[2];
    while (battles[bid].is_alloced) {
        battles[bid].global_time++;
        t[0] = myclock();
        move_bullets(bid);
        check_who_is_shooted(bid);
        check_who_is_dead(bid);
        inform_all_user_battle_state(bid);
        inform_all_user_battle_player(bid);
        check_item_count(bid);
        random_generate_items(bid);
        t[1] = myclock();
        if (t[1] - t[0] >= 10) logw("current delay %lums, size = %ld\n",
                             t[1] - t[0], battles[bid].items.size());
        sum_delay_time += t[1] - t[0];
        while (myclock() < t[0] + GLOBAL_SPEED) usleep(100);
        if (myclock() > prev_time + 10000) {
            prev_time = myclock();
            log("total calulating time: %lums / 10s\n", sum_delay_time);
            sum_delay_time = 0;
        }
    }
    return NULL;
}

int check_user_registered(char* user_name, char* password) {
    for (int i = 0; i < REGISTERED_USER_LIST_SIZE; i++) {
        if (strncmp(user_name, registered_user_list[i].user_name, USERNAME_SIZE - 1) != 0)
            continue;

        if (strncmp(password, registered_user_list[i].password, PASSWORD_SIZE - 1) != 0) {
            logi("user name %s sent error password\n", user_name);
            return SERVER_RESPONSE_LOGIN_FAIL_ERROR_PASSWORD;
        } else {
            return SERVER_RESPONSE_LOGIN_SUCCESS;
        }
    }

    logi("user name %s hasn't been registered\n", user_name);
    return SERVER_RESPONSE_LOGIN_FAIL_UNREGISTERED_USERID;
}

void launch_battle(int bid) {
    pthread_t thread;

    log("try to create battle_ruler thread\n");
    if (pthread_create(&thread, NULL, battle_ruler, (void*)(uintptr_t)bid) == -1) {
        eprintf("fail to launch battle\n");
    }
}

int client_command_user_register(int uid) {
    int ul_index = -1;
    char* user_name = sessions[uid].cm.user_name;
    char* password = sessions[uid].cm.password;
    log("user %s tries to register with password %s\n", user_name, password);

    for (int i = 0; i < REGISTERED_USER_LIST_SIZE; i++) {
        if (strncmp(user_name, registered_user_list[i].user_name, USERNAME_SIZE - 1) != 0)
            continue;

        log("user %s&%s has been registered\n", user_name, password);
        send_to_client(uid, SERVER_RESPONSE_YOU_HAVE_REGISTERED);
        return 0;
    }

    pthread_mutex_lock(&userlist_lock);
    if (user_list_size < REGISTERED_USER_LIST_SIZE)
        ul_index = user_list_size++;
    pthread_mutex_unlock(&userlist_lock);

    log("fetch empty user list index #%d\n", ul_index);
    if (ul_index == -1) {
        log("user %s registers fail\n", user_name);
        send_to_client(uid, SERVER_RESPONSE_REGISTER_FAIL);
    } else {
        log("user %s registers success\n", user_name);
        strncpy(registered_user_list[ul_index].user_name,
                user_name, USERNAME_SIZE - 1);
        strncpy(registered_user_list[ul_index].password,
                password, PASSWORD_SIZE - 1);
        send_to_client(uid, SERVER_RESPONSE_REGISTER_SUCCESS);
        save_user(ul_index);
    }
    return 0;
}

int client_command_user_login(int uid) {
    int is_dup = 0;
    client_message_t* pcm = &sessions[uid].cm;
    char* user_name = pcm->user_name;
    char* password = pcm->password;
    log("user %s try to login\n", user_name);
    int message = check_user_registered(user_name, password);

    if (query_session_built(uid)) {
        log("user %s  [%s] has logined\n", sessions[uid].user_name, sessions[uid].ip_addr);
        send_to_client(uid, SERVER_RESPONSE_YOU_HAVE_LOGINED);
        return 0;
    }

    for (int i = 0; i < USER_CNT; i++) {
        if (query_session_built(i)) {
            logi("check dup user id: %s vs. %s\n", user_name, sessions[i].user_name);
            if (strncmp(user_name, sessions[i].user_name, USERNAME_SIZE - 1) == 0) {
                log("user #%d %s duplicate with %dth user %s@[%s]\n", uid, user_name, i, sessions[i].user_name, sessions[i].ip_addr);
                is_dup = 1;
                break;
            }
        }
    }

    // no duplicate user ids found
    if (is_dup) {
        log("send fail dup id message to client.\n");
        send_to_client(uid, SERVER_RESPONSE_LOGIN_FAIL_DUP_USERID);
        sessions[uid].state = USER_STATE_NOT_LOGIN;
    } else if (message == SERVER_RESPONSE_LOGIN_SUCCESS) {
        log("user %s login success\n", user_name);
        sessions[uid].state = USER_STATE_LOGIN;
        send_to_client(uid, SERVER_RESPONSE_LOGIN_SUCCESS);
        strncpy(sessions[uid].user_name, user_name, USERNAME_SIZE - 1);
        inform_friends(uid, SERVER_MESSAGE_FRIEND_LOGIN);
    } else {
        send_to_client(uid, message);
    }

    return 0;
}

int client_command_user_logout(int uid) {
    if (sessions[uid].state == USER_STATE_BATTLE
        || sessions[uid].state == USER_STATE_WAIT_TO_BATTLE) {
        log("user #%d %s@[%s] tries to logout was in battle\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
        user_quit_battle(sessions[uid].bid, uid);
    }

    log("user #%d %s@[%s] logout\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    sessions[uid].state = USER_STATE_NOT_LOGIN;
    inform_friends(uid, SERVER_MESSAGE_FRIEND_LOGOUT);
    return 0;
}

void list_all_users(server_message_t* psm) {
    for (int i = 0; i < USER_CNT; i++) {
        if (query_session_built(i)) {
            log("%s: found %s %s\n", __func__, sessions[i].user_name,
                sessions[i].state == USER_STATE_BATTLE ? "in battle" : "");
            psm->all_users[i].user_state = sessions[i].state;
            strncpy(psm->all_users[i].user_name, sessions[i].user_name, USERNAME_SIZE - 1);
        }
    }
}

int client_command_fetch_all_users(int uid) {
    char* user_name = sessions[uid].user_name;
    log("user #%d %s@[%s] tries to fetch all users's info\n", uid, user_name, sessions[uid].user_name);

    if (!query_session_built(uid)) {
        logi("user #%d %s@[%s] who tries to list users hasn't login\n", uid, user_name, sessions[uid].user_name);
        send_to_client(uid, SERVER_RESPONSE_YOU_HAVE_NOT_LOGIN);
        return 0;
    }

    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    list_all_users(&sm);
    sm.response = SERVER_RESPONSE_ALL_USERS_INFO;

    wrap_send(sessions[uid].conn, &sm);

    return 0;
}

int client_command_fetch_all_friends(int uid) {
    char* user_name = sessions[uid].user_name;
    log("user #%d %s@[%s] tries to fetch all friends' info\n", uid, user_name, sessions[uid].ip_addr);

    if (!query_session_built(uid)) {
        logi("user #%d %s@[%s] who tries to list users hasn't login\n", uid, user_name, sessions[uid].ip_addr);
        send_to_client(uid, SERVER_RESPONSE_YOU_HAVE_NOT_LOGIN);
        return 0;
    }

    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    list_all_users(&sm);
    sm.all_users[uid].user_state = USER_STATE_UNUSED;
    sm.response = SERVER_RESPONSE_ALL_FRIENDS_INFO;

    wrap_send(sessions[uid].conn, &sm);

    return 0;
}

int invite_friend_to_battle(int bid, int uid, char* friend_name) {
    int friend_id = find_uid_by_user_name(friend_name);
    if (friend_id == -1) {
        // fail to find friend
        logi("friend %s hasn't login\n", friend_name);
        send_to_client(uid, SERVER_MESSAGE_FRIEND_NOT_LOGIN);
    } else if (friend_id == uid) {
        logi("launch battle %d for %s\n", bid, sessions[uid].user_name);
        sessions[uid].inviter_id = uid;
        send_to_client(uid, SERVER_RESPONSE_INVITATION_SENT);
    } else if (sessions[friend_id].state == USER_STATE_BATTLE) {
        // friend already in battle
        logi("friend %s already in battle\n", friend_name);
        send_to_client(uid, SERVER_MESSAGE_FRIEND_ALREADY_IN_BATTLE);
    } else {
        // invite friend
        logi("friend #%d %s found\n", friend_id, friend_name);

        user_invited_to_join_battle(bid, friend_id);
        // WARNING: can't move this statement
        sessions[friend_id].inviter_id = uid;

        send_to_client_with_username(friend_id, SERVER_MESSAGE_INVITE_TO_BATTLE, sessions[uid].user_name);
    }

    return 0;
}

int client_command_launch_battle(int uid) {
    if (sessions[uid].state == USER_STATE_BATTLE) {
        log("user %s who tries to launch battle has been in battle\n", sessions[uid].user_name);
        send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
        return 0;
    } else {
        log("user %s tries to launch battle\n", sessions[uid].user_name);
    }

    int bid = get_unalloced_battle();
    client_message_t* pcm = &sessions[uid].cm;

    log("%s launch battle with %s\n", sessions[uid].user_name, pcm->user_name);

    if (bid == -1) {
        loge("fail to create battle for %s and %s\n", sessions[uid].user_name, pcm->user_name);
        send_to_client(uid, SERVER_RESPONSE_LAUNCH_BATTLE_FAIL);
        return 0;
    } else {
        logi("launch battle %d for %s, invite %s\n", bid, sessions[uid].user_name, pcm->user_name);
        user_join_battle(bid, uid);
        invite_friend_to_battle(bid, uid, pcm->user_name);
        launch_battle(bid);
        send_to_client(uid, SERVER_RESPONSE_LAUNCH_BATTLE_SUCCESS);
    }

    return 0;
}

int client_command_quit_battle(int uid) {
    log("user #%d %s@[%s] tries to quit battle\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    if (sessions[uid].state != USER_STATE_BATTLE) {
        logi("but he hasn't join battle\n");
        send_to_client(uid, SERVER_RESPONSE_YOURE_NOT_IN_BATTLE);
    } else {
        logi("call user_quit_battle to quit\n");
        user_quit_battle(sessions[uid].bid, uid);
    }
    return 0;
}

int client_command_invite_user(int uid) {
    client_message_t* pcm = &sessions[uid].cm;
    int bid = sessions[uid].bid;
    int friend_id = find_uid_by_user_name(pcm->user_name);
    log("user #%d %s@[%s] tries to invite friend\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);

    if (sessions[uid].state != USER_STATE_BATTLE) {
        log("user %s@[%s] who invites friend %s wasn't in battle\n", sessions[uid].user_name, sessions[uid].ip_addr, pcm->user_name);
        send_to_client(uid, SERVER_RESPONSE_YOURE_NOT_IN_BATTLE);
    } else {
        logi("invite user %s@[%s] to battle #%d\n", sessions[friend_id].user_name, sessions[uid].ip_addr, bid);
        invite_friend_to_battle(bid, uid, pcm->user_name);
    }
    return 0;
}

int client_command_send_message(int uid) {
    client_message_t* pcm = &sessions[uid].cm;
    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    sm.message = SERVER_MESSAGE_FRIEND_MESSAGE;
    strncpy(sm.from_user, sessions[uid].user_name, USERNAME_SIZE);
    strncpy(sm.msg, pcm->message, MSG_SIZE);
    if (pcm->user_name[0] == '\0') {
        logi("user %d:%s@[%s] yells at all users: %s\n", uid, sessions[uid].user_name, sessions[uid].ip_addr, pcm->message);
        int i;
        for (i = 0; i < USER_CNT; i++) {
            if (uid == i) continue;
            wrap_send(sessions[i].conn, &sm);
        }
    } else {
        int friend_id = find_uid_by_user_name(pcm->user_name);
        if (friend_id == -1 || friend_id == uid) {
            logi("user %d:%s@[%s] fails to speak to %s:\"%s\"\n", uid, sessions[uid].user_name, sessions[uid].ip_addr, pcm->user_name, pcm->message);
        } else {
            logi("uiser %d:%s@[%s] speaks to %d:%s : \"%s\"\n", uid, sessions[uid].user_name, sessions[uid].ip_addr, friend_id, pcm->user_name, pcm->message);
            wrap_send(sessions[friend_id].conn, &sm);
        }
    }
    return 0;
}

int client_command_create_ffa(int uid) {
    if (sessions[uid].state == USER_STATE_BATTLE) {
        log("user %s who tries to launch battle has been in battle\n", sessions[uid].user_name);
        send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
        return 0;
    } else {
        log("user %s tries to create ffa sessions #0\n", sessions[uid].user_name);
    }

    int bid = 0;
    client_message_t* pcm = &sessions[uid].cm;

    log("%s launch battle with %s\n", sessions[uid].user_name, pcm->user_name);

    if (battles[bid].is_alloced) {
        loge("fail to create battle for %s and %s\n", sessions[uid].user_name, pcm->user_name);
        send_to_client(uid, SERVER_RESPONSE_LAUNCH_BATTLE_FAIL);
        return 0;
    } else {
        logi("launch battle #0 for ffa\n");
        battles[bid].is_alloced = true;
        user_join_battle(bid, uid);
        invite_friend_to_battle(bid, uid, pcm->user_name);
        launch_battle(bid);
        send_to_client(uid, SERVER_RESPONSE_LAUNCH_BATTLE_SUCCESS);
    }

    return 0;
}

int client_command_launch_ffa(int uid) {
    log("user %s@[%s] try ffa\n", sessions[uid].user_name, sessions[uid].ip_addr);

    if (sessions[uid].state == USER_STATE_BATTLE) {
        logi("already in battle\n");
        send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
    } else {
        int bid = 0;

        if (battles[bid].is_alloced) {
            user_join_battle(bid, uid);
            logi("accept success\n");
        } else {
            logi("user %s@[%s] created ffa session #0\n", sessions[uid].user_name, sessions[uid].ip_addr);
            client_command_create_ffa(uid);
        }
    }
    return 0;
}

int client_command_accept_battle(int uid) {
    log("user %s@[%s] accept battle #%d\n", sessions[uid].user_name, sessions[uid].ip_addr, sessions[uid].bid);

    if (sessions[uid].state == USER_STATE_BATTLE) {
        logi("already in battle\n");
        send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
    } else if (sessions[uid].state == USER_STATE_WAIT_TO_BATTLE) {
        int inviter_id = sessions[uid].inviter_id;
        int bid = sessions[uid].bid;

        if (battles[bid].is_alloced) {
            send_to_client_with_username(inviter_id, SERVER_MESSAGE_FRIEND_ACCEPT_BATTLE, sessions[inviter_id].user_name);
            user_join_battle(bid, uid);
            logi("accept success\n");
        } else {
            logi("user %s@[%s] accept battle which didn't exist\n", sessions[uid].user_name, sessions[uid].ip_addr);
            send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
        }

    } else {
        logi("hasn't been invited\n");
        send_to_client(uid, SERVER_RESPONSE_NOBODY_INVITE_YOU);
    }

    return 0;
}

int client_command_reject_battle(int uid) {
    log("user %s@[%s] reject battle #%d\n", sessions[uid].user_name, sessions[uid].ip_addr, sessions[uid].bid);
    if (sessions[uid].state == USER_STATE_BATTLE) {
        logi("user already in battle\n");
        send_to_client(uid, SERVER_RESPONSE_YOURE_ALREADY_IN_BATTLE);
    } else if (sessions[uid].state == USER_STATE_WAIT_TO_BATTLE) {
        logi("reject success\n");
        int bid = sessions[uid].bid;
        send_to_client(sessions[uid].inviter_id, SERVER_MESSAGE_FRIEND_REJECT_BATTLE);
        sessions[uid].state = USER_STATE_LOGIN;
        battles[bid].users[uid].battle_state = BATTLE_STATE_UNJOINED;
    } else {
        logi("hasn't been invited\n");
        send_to_client(uid, SERVER_RESPONSE_NOBODY_INVITE_YOU);
    }
    return 0;
}

int client_command_quit(int uid) {
    int conn = sessions[uid].conn;
    if (sessions[uid].state == USER_STATE_BATTLE
        || sessions[uid].state == USER_STATE_WAIT_TO_BATTLE) {
        log("user #%d %s@[%s] tries to quit client was in battle\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
        user_quit_battle(sessions[uid].bid, uid);
    }

    sessions[uid].conn = -1;
    log("user #%d %s@[%s] quit\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    sessions[uid].state = USER_STATE_UNUSED;
    close(conn);
    return -1;
}

int client_command_move_up(int uid) {
    log("user #%d %s@[%s] move up\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    int bid = sessions[uid].bid;
    battles[bid].users[uid].dir = DIR_UP;
    if (battles[bid].users[uid].pos.y > 0) {
        battles[bid].users[uid].pos.y--;
        check_user_status(uid);
    }
    return 0;
}

int client_command_move_down(int uid) {
    log("user #%d %s@[%s] move down\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    int bid = sessions[uid].bid;
    battles[bid].users[uid].dir = DIR_DOWN;
    if (battles[bid].users[uid].pos.y < BATTLE_H - 1) {
        battles[bid].users[uid].pos.y++;
        check_user_status(uid);
    }
    return 0;
}

int client_command_move_left(int uid) {
    log("user #%d %s@[%s] move left\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    int bid = sessions[uid].bid;
    battles[bid].users[uid].dir = DIR_LEFT;
    if (battles[bid].users[uid].pos.x > 0) {
        battles[bid].users[uid].pos.x--;
        check_user_status(uid);
    }
    return 0;
}

int client_command_move_right(int uid) {
    log("user #%d %s@[%s] move right\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    int bid = sessions[uid].bid;
    battles[bid].users[uid].dir = DIR_RIGHT;
    if (battles[bid].users[uid].pos.x < BATTLE_W - 1) {
        battles[bid].users[uid].pos.x++;
        check_user_status(uid);
    }
    return 0;
}
int client_command_fire(int uid, int delta_x, int delta_y, int dir) {
    int bid = sessions[uid].bid;

    if (battles[bid].users[uid].nr_bullets <= 0) {
        send_to_client(uid, SERVER_MESSAGE_YOUR_MAGAZINE_IS_EMPTY);
        return 1;
    }
    int x = battles[bid].users[uid].pos.x + delta_x;
    int y = battles[bid].users[uid].pos.y + delta_y;
    if (x < 0 || x >= BATTLE_W) return 0;
    if (y < 0 || y >= BATTLE_H) return 0;
    log("user #%d %s@[%s] fire %d\n", uid, sessions[uid].user_name, sessions[uid].ip_addr, dir);
    item_t new_item;
    new_item.id = ++battles[bid].item_count;
    new_item.kind = ITEM_BULLET;
    new_item.dir = dir;
    new_item.owner = uid;
    new_item.pos.x = x;
    new_item.pos.y = y;
    new_item.time = battles[bid].global_time + BULLETS_LASTS_TIME;
    battles[bid].users[uid].nr_bullets--;
    battles[bid].items.push_back(new_item);
    log("current item size: %ld\n", battles[bid].items.size());
    return 0;
}

int client_command_fire_up(int uid) {
    client_command_fire(uid, 0, 0, DIR_UP);
    return 0;
}
int client_command_fire_down(int uid) {
    client_command_fire(uid, 0, 0, DIR_DOWN);
    return 0;
}
int client_command_fire_left(int uid) {
    client_command_fire(uid, 0, 0, DIR_LEFT);
    return 0;
}
int client_command_fire_right(int uid) {
    client_command_fire(uid, 0, 0, DIR_RIGHT);
    return 0;
}

int client_command_fire_up_left(int uid) {
    client_command_fire(uid, 0, 0, DIR_UP_LEFT);
    return 0;
}
int client_command_fire_up_right(int uid) {
    client_command_fire(uid, 0, 0, DIR_UP_RIGHT);
    return 0;
}
int client_command_fire_down_left(int uid) {
    client_command_fire(uid, 0, 0, DIR_DOWN_LEFT);
    return 0;
}
int client_command_fire_down_right(int uid) {
    client_command_fire(uid, 0, 0, DIR_DOWN_RIGHT);
    return 0;
}

int client_command_fire_aoe(int uid, int dir) {
    log("user #%d %s@[%s] fire(aoe) %d\n", uid, sessions[uid].user_name, sessions[uid].ip_addr, dir);
    int limit = battles[sessions[uid].bid].users[uid].nr_bullets / 2;
    for (int i = 0; limit; i++) {
        for (int j = -i; j <= i && limit; j++) {
            switch (dir) {
                case DIR_UP:
                    if (client_command_fire(uid, j, -i + abs(j), dir) != 0) return 0;
                    break;
                case DIR_DOWN:
                    if (client_command_fire(uid, j, i - abs(j), dir) != 0) return 0;
                    break;
                case DIR_LEFT:
                    if (client_command_fire(uid, -i + abs(j), j, dir) != 0) return 0;
                    break;
                case DIR_RIGHT:
                    if (client_command_fire(uid, i - abs(j), j, dir) != 0) return 0;
                    break;
            }
            limit--;
        }
    }
    return 0;
}

int client_command_fire_aoe_up(int uid) {
    return client_command_fire_aoe(uid, DIR_UP);
}
int client_command_fire_aoe_down(int uid) {
    return client_command_fire_aoe(uid, DIR_DOWN);
}
int client_command_fire_aoe_left(int uid) {
    return client_command_fire_aoe(uid, DIR_LEFT);
}
int client_command_fire_aoe_right(int uid) {
    return client_command_fire_aoe(uid, DIR_RIGHT);
}

int admin_ban_user(char* args) {
    log("admin ban user '%s'\n", args);
    int uid = atoi(args);
    if (uid < 0 || uid >= USER_CNT) {
        return 0;
    }
    if (sessions[uid].conn >= 0) {
        char* message = (char*)malloc(MSG_SIZE);
        sprintf(message, "admin banned user #%d %s@[%s]\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
        log("admin banned user #%d %s@[%s]\n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
        send_to_client(uid, SERVER_STATUS_QUIT);
        log("close conn:%d\n", sessions[uid].conn);
        close(sessions[uid].conn);
        sessions[uid].conn = -1;
        for (int i = 0; i < USER_CNT; i++) {
            if (sessions[i].conn >= 0) {
                say_to_client(i, message);
            }
        }
    }
    return 0;
}

static struct {
    const char* cmd;
    int (*func)(char* args);
} admin_handler[] = {
    {"ban", admin_ban_user},
    //{"bullets", admin_set_bullets},
};

#define NR_HANDLER ((int)sizeof(admin_handler) / (int)sizeof(admin_handler[0]))

int client_command_admin_control(int uid) {
    if (!sessions[uid].is_admin) {
        say_to_client(uid, (char*)"you are not admin");
        return 0;
    }
    client_message_t* pcm = &sessions[uid].cm;
    char* command = (char*)pcm->message;

    strtok(command, " \t");
    char* args = strtok(NULL, " \t");

    for (int i = 0; i < NR_HANDLER; i++) {
        if (strcmp(command, admin_handler[i].cmd) == 0) {
            admin_handler[i].func(args);
            return 0;
        }
    }
    return 0;
}

int client_message_fatal(int uid) {
    loge("received FATAL from user #%d %s@[%s] \n", uid, sessions[uid].user_name, sessions[uid].ip_addr);
    for (int i = 0; i < USER_CNT; i++) {
        if (sessions[i].conn >= 0) {
            send_to_client(i, SERVER_STATUS_FATAL);
            log("send FATAL to user #%d %s@[%s]\n", i, sessions[i].user_name, sessions[i].ip_addr);
        }
    }
    terminate_process(SIGINT);
    return 0;
}

static int (*handler[256])(int);

void init_handler() {
    handler[CLIENT_MESSAGE_FATAL] = client_message_fatal,

    handler[CLIENT_COMMAND_USER_QUIT] = client_command_quit,
    handler[CLIENT_COMMAND_USER_REGISTER] = client_command_user_register,
    handler[CLIENT_COMMAND_USER_LOGIN] = client_command_user_login,
    handler[CLIENT_COMMAND_USER_LOGOUT] = client_command_user_logout,

    handler[CLIENT_COMMAND_FETCH_ALL_USERS] = client_command_fetch_all_users,
    handler[CLIENT_COMMAND_FETCH_ALL_FRIENDS] = client_command_fetch_all_friends,

    handler[CLIENT_COMMAND_LAUNCH_BATTLE] = client_command_launch_battle,
    handler[CLIENT_COMMAND_QUIT_BATTLE] = client_command_quit_battle,
    handler[CLIENT_COMMAND_ACCEPT_BATTLE] = client_command_accept_battle,
    handler[CLIENT_COMMAND_LAUNCH_FFA] = client_command_launch_ffa,
    handler[CLIENT_COMMAND_REJECT_BATTLE] = client_command_reject_battle,
    handler[CLIENT_COMMAND_INVITE_USER] = client_command_invite_user,

    handler[CLIENT_COMMAND_SEND_MESSAGE] = client_command_send_message,

    handler[CLIENT_COMMAND_MOVE_UP] = client_command_move_up,
    handler[CLIENT_COMMAND_MOVE_DOWN] = client_command_move_down,
    handler[CLIENT_COMMAND_MOVE_LEFT] = client_command_move_left,
    handler[CLIENT_COMMAND_MOVE_RIGHT] = client_command_move_right,

    handler[CLIENT_COMMAND_FIRE_UP] = client_command_fire_up,
    handler[CLIENT_COMMAND_FIRE_DOWN] = client_command_fire_down,
    handler[CLIENT_COMMAND_FIRE_LEFT] = client_command_fire_left,
    handler[CLIENT_COMMAND_FIRE_RIGHT] = client_command_fire_right,

    handler[CLIENT_COMMAND_FIRE_UP_LEFT] = client_command_fire_up_left,
    handler[CLIENT_COMMAND_FIRE_UP_RIGHT] = client_command_fire_up_right,
    handler[CLIENT_COMMAND_FIRE_DOWN_LEFT] = client_command_fire_down_left,
    handler[CLIENT_COMMAND_FIRE_DOWN_RIGHT] = client_command_fire_down_right,

    handler[CLIENT_COMMAND_FIRE_AOE_UP] = client_command_fire_aoe_up,
    handler[CLIENT_COMMAND_FIRE_AOE_DOWN] = client_command_fire_aoe_down,
    handler[CLIENT_COMMAND_FIRE_AOE_LEFT] = client_command_fire_aoe_left,
    handler[CLIENT_COMMAND_FIRE_AOE_RIGHT] = client_command_fire_aoe_right;

    handler[CLIENT_COMMAND_ADMIN_CONTROL] = client_command_admin_control;

}

void wrap_recv(int conn, client_message_t* pcm) {
    size_t total_len = 0;
    while (total_len < sizeof(client_message_t)) {
        size_t len = recv(conn, pcm + total_len, sizeof(client_message_t) - total_len, 0);
        if (len < 0) {
            loge("broken pipe\n");
        }

        total_len += len;
    }
}

void wrap_send(int conn, server_message_t* psm) {
    size_t total_len = 0;
    while (total_len < sizeof(server_message_t)) {
        size_t len = send(conn, psm + total_len, sizeof(server_message_t) - total_len, 0);
        if (len < 0) {
            loge("broken pipe\n");
        }

        total_len += len;
    }
}

void send_to_client(int uid, int message) {
    int conn = sessions[uid].conn;
    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    sm.response = message;
    wrap_send(conn, &sm);
}

void say_to_client(int uid, char *message) {
    int conn = sessions[uid].conn;
    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    sm.message = SERVER_MESSAGE;
    strncpy(sm.msg, message, MSG_SIZE - 1);
    wrap_send(conn, &sm);
}

void send_to_client_with_username(int uid, int message, char* user_name) {
    int conn = sessions[uid].conn;
    server_message_t sm;
    memset(&sm, 0, sizeof(server_message_t));
    sm.response = message;
    strncpy(sm.friend_name, user_name, USERNAME_SIZE - 1);
    wrap_send(conn, &sm);
}

void close_session(int conn, int message) {
    send_to_client(conn, message);
    close(conn);
}

void* session_start(void* args) {
    int uid = -1;
    session_args_t info = *(session_args_t*)(uintptr_t)args;
    client_message_t* pcm = NULL;
    if ((uid = get_unused_session()) < 0) {
        close_session(info.conn, SERVER_RESPONSE_LOGIN_FAIL_SERVER_LIMITS);
        return NULL;
    } else {
        sessions[uid].conn = info.conn;
        strncpy(sessions[uid].user_name, "<unknown>", USERNAME_SIZE - 1);
        strncpy(sessions[uid].ip_addr, info.ip_addr, IPADDR_SIZE - 1);
        if (strncmp(sessions[uid].ip_addr, "", IPADDR_SIZE) == 0) {
            strncpy(sessions[uid].ip_addr, "unknown", IPADDR_SIZE - 1);
        }
        pcm = &sessions[uid].cm;
        memset(pcm, 0, sizeof(client_message_t));
        log("build session #%d\n", uid);
        if (strncmp(info.ip_addr, "127.0.0.1", IPADDR_SIZE) == 0) {
            log("admin login!\n");
            sessions[uid].is_admin = 1;
        }
    }

    while (1) {
        wrap_recv(info.conn, pcm);
        if (pcm->command >= CLIENT_COMMAND_END)
            continue;

        uint64_t t[2];
        t[0] = myclock();
        int ret_code = handler[pcm->command](uid);
        t[1] = myclock();
        sum_delay_time += t[1] - t[0];
        //log("state of user %s: %d\n", sessions[uid].user_name, sessions[uid].state);
        if (ret_code < 0) {
            log("close session #%d\n", uid);
            break;
        }
    }
    return NULL;
}

void* run_battle(void* args) {
    // TODO:
    return NULL;
}

int server_start() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        eprintf("Create Socket Failed!\n");
    }

    struct sockaddr_in servaddr;
    bool binded = false;
    for (int cur_port = port; cur_port <= port + port_range; cur_port++) {
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(cur_port);
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
            logw("Can not bind to port %d!\n", cur_port);
        } else {
            binded = true;
            port = cur_port;
            break;
        }
    }
    if (!binded) {
        eprintf("Can not start server.");
    }

    if (listen(sockfd, USER_CNT) == -1) {
        eprintf("fail to listen on socket.\n");
    } else {
        log("Listen on port %d.\n", port);
    }

    return sockfd;
}

void terminate_process(int recved_signal) {
    for (int i = 0; i < USER_CNT; i++) {
        if (sessions[i].conn >= 0) {
            log("send QUIT to user #%d %s@[%s]\n", i, sessions[i].user_name, sessions[i].ip_addr);
            send_to_client(i, SERVER_STATUS_QUIT);
            log("close conn:%d\n", sessions[i].conn);
            close(sessions[i].conn);
            sessions[i].conn = -1;
        }
    }

    if (server_fd) {
        close(server_fd);
        log("close server fd:%d\n", server_fd);
    }

    pthread_mutex_destroy(&sessions_lock);
    pthread_mutex_destroy(&battles_lock);
    for (int i = 0; i < USER_CNT; i++) {
        pthread_mutex_destroy(&items_lock[i]);
    }

    log("exit(%d)\n", recved_signal);
    exit(recved_signal);
}

void terminate_entrance(int recved_signal) {
    loge("received signal %s, terminate.\n", signal_name_s[recved_signal]);
    terminate_process(recved_signal);
}

void* speed_monitor(void* args) {
    while (1) {

    }
}

int main(int argc, char* argv[]) {
    init_constants();
    init_handler();
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    srand(time(NULL));

    pthread_t thread;

    if (signal(SIGINT, terminate_entrance) == SIG_ERR) {
        eprintf("An error occurred while setting a signal handler.\n");
    }
    if (signal(SIGSEGV, terminate_entrance) == SIG_ERR) {
        eprintf("An error occurred while setting a signal handler.\n");
    }
    if (signal(SIGABRT, terminate_entrance) == SIG_ERR) {
        eprintf("An error occurred while setting a signal handler.\n");
    }
    if (signal(SIGTERM, terminate_entrance) == SIG_ERR) {
        eprintf("An error occurred while setting a signal handler.\n");
    }

    for (int i = 0; i < USER_CNT; i++) {
        pthread_mutex_init(&items_lock[i], NULL);
    }
    log("server " VERSION "\n");
    //log("message_size = %ld\n", sizeof(server_message_t));

    server_fd = server_start();
    load_user_list();

    if (pthread_create(&thread, NULL, speed_monitor, NULL) != 0) {
        loge("failed to create speed_monitor thread.");
    }

    for (int i = 0; i < USER_CNT; i++)
        sessions[i].conn = -1;

    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    while (1) {
        static session_args_t info;
        info.conn = accept(server_fd, (struct sockaddr*)&client_addr, &length);
        strncpy(info.ip_addr, inet_ntoa(client_addr.sin_addr), IPADDR_SIZE - 1);
        log("connected by [%s:%d] , conn:%d\n", info.ip_addr, client_addr.sin_port, info.conn);
        if (info.conn < 0) {
            loge("fail to accept client.\n");
        } else if (pthread_create(&thread, NULL, session_start, (void*)(uintptr_t)&info) != 0) {
            loge("fail to create thread.\n");
        }
        logi("bind thread #%lu\n", thread);
    }

    return 0;
}
