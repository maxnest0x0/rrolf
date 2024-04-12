// Copyright (C) 2024  Paul Johnson

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <Server/Squad.h>

#include <stdlib.h>
#include <string.h>

#include <Server/Server.h>

void rr_squad_init(struct rr_squad *this, struct rr_server *server, uint8_t pos)
{
    memset(this, 0, sizeof *this);
    for (uint32_t i = 0; i < 6; ++i)
        this->squad_code[i] = (char)(97 + rand() % 26);
    this->squad_code[6] = 0;
    for (uint32_t i = 0; i < RR_MAX_CLIENT_COUNT; ++i)
        rr_bitset_unset(server->clients[i].joined_squad_before, pos);
}

uint8_t rr_squad_has_space(struct rr_squad *this)
{
    return this->member_count < RR_SQUAD_MEMBER_COUNT;
}

void rr_squad_add_client(struct rr_squad *this, struct rr_server_client *client)
{
    for (uint32_t i = 0; i < RR_SQUAD_MEMBER_COUNT; ++i)
    {
        if (this->members[i].in_use)
            continue;
        memset(&this->members[i], 0, sizeof this->members[i]);
        client->squad_pos = i;
        this->members[i].client = client;
        this->member_count += 1;
        this->members[i].in_use = 1;
        this->members[i].is_dev = client->dev;
        return;
    }
}

void rr_squad_remove_client(struct rr_squad *this,
                            struct rr_server_client *client)
{
    this->member_count -= 1;
    memset(&this->members[client->squad_pos], 0,
           sizeof(struct rr_squad_member));
    if (this->member_count == 0)
        rr_squad_init(this, client->server, client->squad);
    client->squad_pos = 0;
    client->in_squad = 0;
}

void rr_client_can_rejoin_squads(struct rr_server *this,
                                 struct rr_server_client *member,
                                 uint8_t choosen)
{
    // check if there's non-empty public squad at choosen pos or below
    for (uint8_t i = choosen; i < RR_SQUAD_COUNT; ++i)
    {
        if (this->squads[i].private)
            continue;
        for (uint8_t j = 0; j < RR_SQUAD_MEMBER_COUNT; ++j)
            if (this->squads[i].members[j].in_use)
                return;
    }

    // chosen empty squad once, next time search from top
    for (uint8_t i = 0; i < RR_SQUAD_COUNT; ++i)
        if (!this->squads[i].private)
            rr_bitset_unset(member->joined_squad_before, i);
}

uint8_t rr_client_find_squad(struct rr_server *this,
                             struct rr_server_client *member)
{
    for (uint8_t i = 0; i < RR_SQUAD_COUNT; ++i)
        if (rr_squad_has_space(&this->squads[i]) && !this->squads[i].private &&
            !rr_bitset_get(member->joined_squad_before, i))
        {
            rr_client_can_rejoin_squads(this, member, i);
            return i;
        }
    return RR_ERROR_CODE_INVALID_SQUAD;
}

uint8_t rr_client_create_squad(struct rr_server *this,
                               struct rr_server_client *member)
{
    for (uint8_t i = 0; i < RR_SQUAD_COUNT; ++i)
        if (this->squads[i].member_count == 0)
        {
            this->squads[i].private = 1;
            return i;
        }
    return RR_ERROR_CODE_INVALID_SQUAD;
}

uint8_t rr_client_join_squad_with_code(struct rr_server *this, char *code)
{
    for (uint8_t i = 0; i < RR_SQUAD_COUNT; ++i)
        if (memcmp(this->squads[i].squad_code, code, 6) == 0)
            return rr_squad_has_space(&this->squads[i])
                       ? i
                       : RR_ERROR_CODE_FULL_SQUAD;
    return RR_ERROR_CODE_INVALID_SQUAD;
}

uint8_t rr_client_join_squad(struct rr_server *this,
                             struct rr_server_client *member, uint8_t pos)
{
    if (!rr_squad_has_space(&this->squads[pos]))
        return 0;
    rr_squad_add_client(&this->squads[pos], member);
    member->squad = pos;
    member->in_squad = 1;
    rr_bitset_set(member->joined_squad_before, pos);
    return 1;
}

void rr_client_leave_squad(struct rr_server *this,
                           struct rr_server_client *member)
{
    if (!member->in_squad)
        return;
    rr_squad_remove_client(&this->squads[member->squad], member);
    member->squad = 0;
    member->in_squad = 0;
}

struct rr_squad_member *
rr_squad_get_client_slot(struct rr_server *this,
                         struct rr_server_client *member)
{
    if (!member->in_squad)
        return NULL;
    return &this->squads[member->squad].members[member->squad_pos];
}

struct rr_squad *rr_client_get_squad(struct rr_server *this,
                                     struct rr_server_client *member)
{
    if (!member->in_squad)
        return NULL;
    return &this->squads[member->squad];
}
