#include <Server/EntityAllocation.h>
#include <Server/Waves.h>

#include <math.h>
#include <stdlib.h>

#include <Shared/Utilities.h>

static struct rr_maze_grid DEFAULT_GRID = {0};

static void set_respawn_zone(struct rr_spawn_zone *zone, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    zone->x = x * RR_BURRON_GRID_SIZE;
    zone->y = y * RR_BURRON_GRID_SIZE;
    zone->w = w * RR_BURRON_GRID_SIZE;
    zone->h = h * RR_BURRON_GRID_SIZE;
}

EntityIdx rr_simulation_alloc_player(struct rr_simulation *this, EntityIdx arena, EntityIdx entity)
{
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(this, entity);
    rr_component_player_info_set_arena(player_info, arena);
    EntityIdx flower_id = rr_simulation_alloc_entity(this);
    struct rr_component_physical *physical =
        rr_simulation_add_physical(this, flower_id);
    struct rr_component_health *health =
        rr_simulation_add_health(this, flower_id);
    struct rr_component_relations *relations =
        rr_simulation_add_relations(this, flower_id);
    struct rr_spawn_zone *respawn_zone = &rr_simulation_get_arena(this, arena)->respawn_zone;
    rr_component_physical_set_x(physical, respawn_zone->x + respawn_zone->w * rr_frand());
    rr_component_physical_set_y(physical, respawn_zone->y + respawn_zone->h * rr_frand());
    rr_component_physical_set_radius(physical, 32.0f);
    physical->arena = arena;
    physical->friction = 0.75;
    if (rand() < RAND_MAX / 1000)
        rr_component_physical_set_angle(physical, rr_frand() * M_PI * 2);

    rr_simulation_add_flower(this, flower_id);
    rr_component_health_set_max_health(health, 100 * pow(1.02169107605, player_info->level));
    rr_component_health_set_health(health, health->max_health);
    health->damage = 10;
    health->damage_paused = 25;
    rr_component_relations_set_team(relations, rr_simulation_team_id_players);
    rr_component_relations_set_owner(relations, entity);
    rr_component_player_info_set_camera_x(player_info, physical->x);
    rr_component_player_info_set_camera_y(player_info, physical->y);
    rr_component_player_info_set_flower_id(player_info, flower_id);
    return flower_id;
}

EntityIdx rr_simulation_alloc_petal(struct rr_simulation *this, EntityIdx arena, float x, float y, uint8_t id,
                                    uint8_t rarity, EntityIdx owner)
{
    struct rr_petal_data const *data = &RR_PETAL_DATA[id];
    EntityIdx petal_id = rr_simulation_alloc_entity(this);
    struct rr_component_physical *physical =
        rr_simulation_add_physical(this, petal_id);
    struct rr_component_petal *petal = rr_simulation_add_petal(this, petal_id);
    struct rr_component_relations *relations =
        rr_simulation_add_relations(this, petal_id);
    struct rr_component_health *health =
        rr_simulation_add_health(this, petal_id);
    rr_component_physical_set_radius(physical, 10);
    rr_component_physical_set_angle(physical, rr_frand() * M_PI * 2);
    rr_component_physical_set_x(physical, x);
    rr_component_physical_set_y(physical, y);
    physical->arena = arena;
    physical->mass = 5;
    physical->friction = 0.75;
    if (id == rr_petal_id_club)
    {
        rr_component_physical_set_radius(physical, 15);
        physical->mass = 5 * powf(RR_PETAL_RARITY_SCALE[id].damage, 2);
        physical->knockback_scale = 4.5;
    }

    rr_component_petal_set_id(petal, id);
    rr_component_petal_set_rarity(petal, rarity);

    rr_component_relations_set_owner(relations,
                                     owner); // flower owns petal, not player
    rr_component_relations_set_team(
        relations, rr_simulation_get_relations(this, owner)->team);

    float scale_h = RR_PETAL_RARITY_SCALE[rarity].health;
    float scale_d = RR_PETAL_RARITY_SCALE[rarity].damage;

    rr_component_health_set_max_health(health, scale_h * data->health);
    rr_component_health_set_health(health, health->max_health);
    rr_component_health_set_flags(health, health->flags | 1);
    health->damage = scale_d * data->damage / RR_PETAL_DATA[id].count[rarity];
    if (data->secondary_cooldown > 0)
    {
        petal->effect_delay = data->secondary_cooldown;
        petal->no_rotation = 1;
    }
    return petal_id;
}

EntityIdx rr_simulation_alloc_mob(struct rr_simulation *this, EntityIdx arena_id,
                                  float x, float y,
                                  enum rr_mob_id mob_id,
                                  enum rr_rarity_id rarity_id,
                                  enum rr_simulation_team_id team_id)
{
    ++rr_simulation_get_arena(this, arena_id)->mob_count;
    EntityIdx entity = rr_simulation_alloc_entity(this);

    struct rr_component_mob *mob = rr_simulation_add_mob(this, entity);
    struct rr_component_physical *physical =
        rr_simulation_add_physical(this, entity);
    struct rr_component_relations *relations =
        rr_simulation_add_relations(this, entity);
    struct rr_component_ai *ai = rr_simulation_add_ai(this, entity);
    // init team elsewhere
    mob->zone = &DEFAULT_GRID;
    rr_component_mob_set_id(mob, mob_id);
    rr_component_mob_set_rarity(mob, rarity_id);
    struct rr_mob_rarity_scale const *rarity_scale =
        RR_MOB_RARITY_SCALING + rarity_id;
    struct rr_mob_data const *mob_data = RR_MOB_DATA + mob_id;
    rr_component_physical_set_radius(physical,
                                     mob_data->radius * rarity_scale->radius);
    rr_component_physical_set_angle(physical, rr_frand() * 2 * M_PI);
    rr_component_physical_set_x(physical, x);
    rr_component_physical_set_y(physical, y);
    physical->arena = arena_id;
    physical->friction = 0.75;
    physical->mass = 100.0f * powf(3, RR_MOB_RARITY_SCALING[rarity_id].damage);
    if (mob_id == rr_mob_id_meteor)
    {
        physical->mass *= 100;
        team_id = rr_simulation_team_id_players;
    }
    rr_component_relations_set_team(relations, team_id);
    if (mob_id == rr_mob_id_beehive)
    {
        struct rr_component_arena *arena =
        rr_simulation_add_arena(this, entity);
        arena->grid = &RR_MAZE_BURROW[0][0];
        arena->maze_dim = RR_BURROW_MAZE_DIM;
        arena->grid_size = RR_BURRON_GRID_SIZE;
        rr_component_arena_spatial_hash_init(arena, this);
        set_respawn_zone(&arena->respawn_zone, 1, 1, 1, 1);
        for (uint32_t X = 0; X < arena->maze_dim; ++X)
        {
            for (uint32_t Y = 0; Y < arena->maze_dim; ++Y)
            {
                uint8_t v = arena->grid[Y * arena->maze_dim + X].value;
                if (v == 0 || (v & 8))
                    continue;
                rr_simulation_alloc_mob(this, entity, (X+rr_frand())*arena->grid_size, (Y+rr_frand())*arena->grid_size, rr_mob_id_honeybee, rarity_id, team_id);
                break;            
            }
        }
    }
    else
    {
        struct rr_component_health *health = rr_simulation_add_health(this, entity);
        rr_component_health_set_max_health(health,
                                        mob_data->health * rarity_scale->health);
        rr_component_health_set_health(health,
                                    mob_data->health * rarity_scale->health);
        health->damage = mob_data->damage * rarity_scale->damage;
    }
    /*
    if (mob_id == 255 && 0)
    {
        // ai->ai_type = rr_ai_type_neutral;
        EntityIdx prev_node = entity;
        struct rr_component_centipede *centipede =
            rr_simulation_add_centipede(this, entity);
        struct rr_vector extension;
        rr_vector_from_polar(&extension, physical->radius * 2, physical->angle);
        EntityIdx new_entity = RR_NULL_ENTITY;
        for (uint64_t i = 0; i < 5; ++i)
        {
            new_entity = rr_simulation_alloc_mob(this, 255, rarity_id, team_id);
            centipede->child_node = new_entity;
            centipede = rr_simulation_add_centipede(this, new_entity);
            struct rr_component_physical *new_phys =
                rr_simulation_get_physical(this, new_entity);
            rr_component_physical_set_x(new_phys,
                                        physical->x + extension.x * (i + 1));
            rr_component_physical_set_y(new_phys,
                                        physical->y + extension.y * (i + 1));
            centipede->parent_node = entity;
            entity = new_entity;
        }
    }
    */

    return entity;
}

EntityIdx rr_simulation_alloc_entity(struct rr_simulation *this)
{
    for (EntityIdx i = 1; i < RR_MAX_ENTITY_COUNT; i++)
    {
        if (!rr_simulation_has_entity(this, i))
        {
            if (rr_bitset_get_bit(this->deleted_last_tick, i))
                continue;
            this->entity_tracker[i] = 1;
#ifndef NDEBUG
            //printf("created with id %d\n", i);
#endif
            return i;
        }
    }

    RR_UNREACHABLE("ran out of entity ids");
}
