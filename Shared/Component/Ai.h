#pragma once

#include <Shared/Component/Common.h>
#include <Shared/Entity.h>
#include <Shared/Utilities.h>

struct rr_simulation;
struct proto_bug;
RR_CLIENT_ONLY(struct rr_renderer;)

enum rr_ai_state : uint8_t
{
    rr_ai_state_idle,
    rr_ai_state_idle_moving,
    rr_ai_state_attacking
};

enum rr_ai_type : uint8_t
{
    // does nothing, even if attacked
    rr_ai_type_passive,
    // runs towards attackers
    rr_ai_type_neutral,
    rr_ai_type_aggressive
};

struct rr_component_ai
{
    RR_SERVER_ONLY(uint64_t protocol_state;)
    RR_SERVER_ONLY(uint32_t ticks_until_next_action;)
                   EntityIdx parent_id;
                   // only used if the ai_type == rr_ai_type == rr_ai_type_neutral
                   // and if rr_ai_state == rr_ai_state_attacking
    RR_SERVER_ONLY(EntityIdx target_entity;)
    RR_SERVER_ONLY(enum rr_ai_state ai_state;)
    RR_SERVER_ONLY(enum rr_ai_type ai_type;)
};

void rr_component_ai_init(struct rr_component_ai *);
void rr_component_ai_free(struct rr_component_ai *, struct rr_simulation *);

RR_SERVER_ONLY(void rr_component_ai_write(struct rr_component_ai *, struct proto_bug *, int);)
RR_CLIENT_ONLY(void rr_component_ai_read(struct rr_component_ai *, struct proto_bug *);)
