#pragma once

// Forward declarations
namespace rf
{
    struct Player;
}

void server_init();
void server_do_frame();
bool check_server_chat_command(const char* msg, rf::Player* sender);
bool server_is_saving_enabled();
void server_reliable_socket_ready(rf::Player* player);
bool server_weapon_items_give_full_ammo();
