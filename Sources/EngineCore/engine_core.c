#include "engine_core.h"

#include <math.h>
#include <string.h>

enum {
    MDTBMaxSceneBoxes = 4096,
    MDTBMaxCollisionBoxes = 1536,
    MDTBMaxBlocks = 16,
    MDTBMaxRoadLinks = 32,
    MDTBMaxVehicleAnchors = 16,
    MDTBMaxInterestPoints = 128,
    MDTBMaxDynamicProps = 192,
    MDTBMaxPopulationProfiles = MDTBMaxBlocks,
    MDTBMaxTrafficOccupancies = 32,
};

typedef struct {
    float height;
    uint32_t surface_kind;
} MDTBGroundInfo;

typedef struct {
    float max_forward_speed;
    float max_reverse_speed;
    float acceleration_rate;
    float brake_rate;
    float coast_drag_rate;
    float steer_rate;
    float collision_radius;
    float travel_lane_offset;
    float seat_height;
    float seat_forward;
    float seat_side;
    float exit_side_distance;
    float exit_rear_distance;
    float first_person_yaw_limit;
    float first_person_pitch_min;
    float first_person_pitch_max;
    float third_person_distance;
    float third_person_height;
    float third_person_side;
} MDTBVehicleTuning;

static MDTBSceneBox g_scene_boxes[MDTBMaxSceneBoxes];
static size_t g_scene_box_count = 0u;
static MDTBBox g_collision_boxes[MDTBMaxCollisionBoxes];
static size_t g_collision_box_count = 0u;
static MDTBBlockDescriptor g_blocks[MDTBMaxBlocks];
static size_t g_block_count = 0u;
static MDTBRoadLink g_road_links[MDTBMaxRoadLinks];
static size_t g_road_link_count = 0u;
static MDTBVehicleAnchor g_vehicle_anchors[MDTBMaxVehicleAnchors];
static size_t g_vehicle_anchor_count = 0u;
static MDTBInterestPoint g_interest_points[MDTBMaxInterestPoints];
static size_t g_interest_point_count = 0u;
static MDTBDynamicProp g_dynamic_props[MDTBMaxDynamicProps];
static size_t g_dynamic_prop_count = 0u;
static MDTBPopulationProfile g_population_profiles[MDTBMaxPopulationProfiles];
static size_t g_population_profile_count = 0u;
static MDTBTrafficOccupancy g_traffic_occupancies[MDTBMaxTrafficOccupancies];
static size_t g_traffic_occupancy_count = 0u;
static int g_scene_initialized = 0;
static uint32_t g_scene_scope_block_index = MDTBIndexNone;
static uint32_t g_scene_scope_layer = MDTBSceneLayerShared;

static void build_frontage_for_block(const MDTBBlockDescriptor *block);
static void build_hotspot_hooks(const MDTBBlockDescriptor *block, uint32_t block_index);
static void build_vehicle_handoff_hooks(const MDTBBlockDescriptor *block, uint32_t block_index);
static void build_combat_sandbox_props(void);
static MDTBFloat3 view_forward(float yaw, float pitch);
static MDTBFloat3 normalize_flat(MDTBFloat3 value);
static MDTBFloat3 vehicle_forward_flat(float heading);
static MDTBFloat3 vehicle_right_flat(float heading);
static MDTBFloat3 actor_focus_position(const MDTBEngineState *state);
static void refresh_combat_proximity(MDTBEngineState *state);
static void step_combat_state(MDTBEngineState *state, float dt, int wants_attack, int wants_reload);

static const float kPi = 3.1415926535f;
static const float kRoadHalfWidth = 5.8f;
static const float kCurbOuter = 6.35f;
static const float kSidewalkOuter = 12.0f;
static const float kPlayableHalfWidth = 168.0f;
static const float kPlayableHalfLength = 132.0f;
static const float kRoadHeight = 0.02f;
static const float kSidewalkHeight = 0.22f;
static const float kPlayerRadius = 0.34f;
static const float kEyeHeight = 1.68f;
static const float kIntersectionClear = 9.75f;
static const float kCollisionGap = 0.05f;
static const float kCrosswalkOffset = 7.25f;
static const float kLinkActivationHalfWidth = 14.0f;
static const float kVehicleMountRadius = 3.6f;
static const float kVehiclePreviewRadius = 10.5f;
static const float kVehicleLaneAssistStrength = 5.2f;
static const float kVehicleLanePullStrength = 4.0f;
static const MDTBFloat3 kLeadPipePickupPosition = {-13.4f, kSidewalkHeight, 43.8f};
static const MDTBFloat3 kPistolPickupPosition = {-4.2f, kSidewalkHeight, 44.6f};
static const MDTBFloat3 kPracticeDummyPosition = {-12.6f, kSidewalkHeight, 57.4f};
static const MDTBFloat3 kLookoutBasePosition = {4.6f, kSidewalkHeight, 56.8f};
static const float kMeleePickupRadius = 1.45f;
static const float kMeleeTargetRange = 2.45f;
static const float kMeleeTargetProximityRadius = 3.1f;
static const float kMeleeArcDot = 0.38f;
static const float kMeleeWindupDuration = 0.12f;
static const float kMeleeStrikeDuration = 0.10f;
static const float kMeleeRecoveryDuration = 0.24f;
static const float kPracticeDummyMaxHealth = 100.0f;
static const float kPracticeDummyRespawnDelay = 1.6f;
static const float kLookoutMaxHealth = 84.0f;
static const float kLookoutRespawnDelay = 2.2f;
static const float kLookoutAlertDistance = 8.2f;
static const float kLookoutAimBias = 0.06f;
static const uint32_t kPistolClipCapacity = 6u;
static const uint32_t kPistolInitialReserveAmmo = 18u;
static const float kPistolPickupRadius = 1.45f;
static const float kPistolRange = 20.0f;
static const float kPistolAimDot = 0.90f;
static const float kPistolShotCooldown = 0.24f;
static const float kPistolReloadDuration = 1.15f;
static const float kPistolShotFlashDuration = 0.10f;

static const MDTBBlockDescriptor kBlockLayout[] = {
    {{0.0f, 0.0f, 0.0f}, MDTBBlockKindHub, 0u, 58.0f, MDTBDistrictSouthHub, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateCivicRetail, MDTBWorldChunkWestGrid},
    {{0.0f, 0.0f, 72.0f}, MDTBBlockKindResidential, 1u, 56.0f, MDTBDistrictMapleHeights, MDTBBlockTagResidential | MDTBBlockTagTransit, MDTBFrontageTemplateResidentialCourt, MDTBWorldChunkWestGrid},
    {{96.0f, 0.0f, 0.0f}, MDTBBlockKindMixedUse, 2u, 58.0f, MDTBDistrictMarketSpur, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagSpur, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkEastGrid},
    {{96.0f, 0.0f, 72.0f}, MDTBBlockKindMixedUse, 3u, 56.0f, MDTBDistrictMarketSpur, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagSpur | MDTBBlockTagCourt, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkEastGrid},
};

static const MDTBRoadLink kRoadLayout[] = {
    {0u, 1u, {0.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth},
    {0u, 2u, {48.0f, 0.22f, 0.0f}, 96.0f, MDTBRoadAxisEastWest},
    {1u, 3u, {48.0f, 0.22f, 72.0f}, 96.0f, MDTBRoadAxisEastWest},
    {2u, 3u, {96.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth},
};

static const MDTBPopulationProfile kPopulationProfileLayout[] = {
    {0u, 0.76f, 0.60f, 0.72f, 0.58f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {1u, 0.58f, 0.38f, 0.46f, 0.42f, MDTBPopulationStyleResidentialCalm},
    {2u, 0.84f, 0.70f, 0.90f, 0.82f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
    {3u, 0.72f, 0.82f, 0.86f, 0.94f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
};

static size_t scene_layout_count(void) {
    return sizeof(kBlockLayout) / sizeof(kBlockLayout[0]);
}

static int layout_has_prior_z(size_t layout_index) {
    for (size_t index = 0u; index < layout_index; ++index) {
        if (fabsf(kBlockLayout[index].origin.z - kBlockLayout[layout_index].origin.z) <= 0.01f) {
            return 1;
        }
    }

    return 0;
}

static int layout_has_prior_x(size_t layout_index) {
    for (size_t index = 0u; index < layout_index; ++index) {
        if (fabsf(kBlockLayout[index].origin.x - kBlockLayout[layout_index].origin.x) <= 0.01f) {
            return 1;
        }
    }

    return 0;
}

static MDTBFloat3 make_float3(float x, float y, float z) {
    MDTBFloat3 value;
    value.x = x;
    value.y = y;
    value.z = z;
    return value;
}

static MDTBFloat4 make_float4(float x, float y, float z, float w) {
    MDTBFloat4 value;
    value.x = x;
    value.y = y;
    value.z = z;
    value.w = w;
    return value;
}

static MDTBBox make_box(MDTBFloat3 center, MDTBFloat3 half_extents, MDTBFloat4 color) {
    MDTBBox value;
    value.center = center;
    value.half_extents = half_extents;
    value.color = color;
    return value;
}

static MDTBFloat3 offset_point(MDTBFloat3 origin, float x, float y, float z) {
    return make_float3(origin.x + x, origin.y + y, origin.z + z);
}

static float clampf(float value, float minimum, float maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static float wrap_angle(float value) {
    while (value > kPi) {
        value -= 2.0f * kPi;
    }

    while (value < -kPi) {
        value += 2.0f * kPi;
    }

    return value;
}

static float distance_squared_xz(MDTBFloat3 a, MDTBFloat3 b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return (dx * dx) + (dz * dz);
}

static float dot_flat(MDTBFloat3 a, MDTBFloat3 b) {
    return (a.x * b.x) + (a.z * b.z);
}

static float point_to_segment_distance_squared_xz(MDTBFloat3 point, MDTBFloat3 a, MDTBFloat3 b) {
    const float ab_x = b.x - a.x;
    const float ab_z = b.z - a.z;
    const float length_squared = (ab_x * ab_x) + (ab_z * ab_z);

    if (length_squared <= 0.0001f) {
        return distance_squared_xz(point, a);
    }

    const float ap_x = point.x - a.x;
    const float ap_z = point.z - a.z;
    const float t = clampf(((ap_x * ab_x) + (ap_z * ab_z)) / length_squared, 0.0f, 1.0f);
    const MDTBFloat3 closest = make_float3(a.x + (ab_x * t), point.y, a.z + (ab_z * t));
    return distance_squared_xz(point, closest);
}

static float approachf(float current, float target, float speed, float dt) {
    const float amount = 1.0f - expf(-speed * dt);
    return current + ((target - current) * amount);
}

static float approach_angle(float current, float target, float speed, float dt) {
    const float delta = wrap_angle(target - current);
    return wrap_angle(current + (delta * (1.0f - expf(-speed * dt))));
}

static MDTBFloat3 approach_float3(MDTBFloat3 current, MDTBFloat3 target, float speed, float dt) {
    return make_float3(
        approachf(current.x, target.x, speed, dt),
        approachf(current.y, target.y, speed, dt),
        approachf(current.z, target.z, speed, dt)
    );
}

static MDTBFloat3 lerp_float3(MDTBFloat3 a, MDTBFloat3 b, float t) {
    return make_float3(
        a.x + ((b.x - a.x) * t),
        a.y + ((b.y - a.y) * t),
        a.z + ((b.z - a.z) * t)
    );
}

static float traffic_occupancy_radius_for_vehicle_kind(uint32_t kind) {
    switch (kind) {
        case MDTBVehicleKindBicycle:
            return 1.8f;
        case MDTBVehicleKindMoped:
            return 2.1f;
        case MDTBVehicleKindMotorcycle:
            return 2.5f;
        case MDTBVehicleKindCoupe:
            return 2.8f;
        case MDTBVehicleKindSedan:
        default:
            return 3.0f;
    }
}

static void set_scene_scope(uint32_t block_index, uint32_t layer) {
    g_scene_scope_block_index = block_index;
    g_scene_scope_layer = layer;
}

static void clear_scene_scope(void) {
    g_scene_scope_block_index = MDTBIndexNone;
    g_scene_scope_layer = MDTBSceneLayerShared;
}

static void push_scene_box(MDTBBox box) {
    if (g_scene_box_count >= MDTBMaxSceneBoxes) {
        return;
    }

    g_scene_boxes[g_scene_box_count].box = box;
    g_scene_boxes[g_scene_box_count].block_index = g_scene_scope_block_index;
    g_scene_boxes[g_scene_box_count].layer = g_scene_scope_layer;
    g_scene_box_count += 1u;
}

static void push_collision_box(MDTBBox box) {
    if (g_collision_box_count >= MDTBMaxCollisionBoxes) {
        return;
    }

    g_collision_boxes[g_collision_box_count++] = box;
}

static void push_block_descriptor(MDTBBlockDescriptor block) {
    if (g_block_count >= MDTBMaxBlocks) {
        return;
    }

    g_blocks[g_block_count++] = block;
}

static void push_road_link(MDTBRoadLink link) {
    if (g_road_link_count >= MDTBMaxRoadLinks) {
        return;
    }

    g_road_links[g_road_link_count++] = link;
}

static void push_vehicle_anchor(MDTBFloat3 position, float yaw, uint32_t block_index, uint32_t kind, uint32_t parking_state, uint32_t lane_axis, float lane_offset) {
    if (g_vehicle_anchor_count >= MDTBMaxVehicleAnchors) {
        return;
    }

    g_vehicle_anchors[g_vehicle_anchor_count].position = position;
    g_vehicle_anchors[g_vehicle_anchor_count].yaw = yaw;
    g_vehicle_anchors[g_vehicle_anchor_count].block_index = block_index;
    g_vehicle_anchors[g_vehicle_anchor_count].kind = kind;
    g_vehicle_anchors[g_vehicle_anchor_count].parking_state = parking_state;
    g_vehicle_anchors[g_vehicle_anchor_count].lane_axis = lane_axis;
    g_vehicle_anchors[g_vehicle_anchor_count].lane_offset = lane_offset;
    g_vehicle_anchor_count += 1u;
}

static void push_interest_point(MDTBFloat3 position, float radius, uint32_t kind, uint32_t block_index) {
    if (g_interest_point_count >= MDTBMaxInterestPoints) {
        return;
    }

    g_interest_points[g_interest_point_count].position = position;
    g_interest_points[g_interest_point_count].radius = radius;
    g_interest_points[g_interest_point_count].kind = kind;
    g_interest_points[g_interest_point_count].block_index = block_index;
    g_interest_point_count += 1u;
}

static void push_dynamic_prop(MDTBFloat3 position, MDTBFloat3 half_extents, MDTBFloat4 color, float phase_offset, uint32_t kind, uint32_t block_index) {
    if (g_dynamic_prop_count >= MDTBMaxDynamicProps) {
        return;
    }

    g_dynamic_props[g_dynamic_prop_count].position = position;
    g_dynamic_props[g_dynamic_prop_count].half_extents = half_extents;
    g_dynamic_props[g_dynamic_prop_count].color = color;
    g_dynamic_props[g_dynamic_prop_count].phase_offset = phase_offset;
    g_dynamic_props[g_dynamic_prop_count].kind = kind;
    g_dynamic_props[g_dynamic_prop_count].block_index = block_index;
    g_dynamic_prop_count += 1u;
}

static void push_population_profile(MDTBPopulationProfile profile) {
    if (g_population_profile_count >= MDTBMaxPopulationProfiles) {
        return;
    }

    g_population_profiles[g_population_profile_count++] = profile;
}

static void push_traffic_occupancy(MDTBFloat3 position, float radius, uint32_t block_index, uint32_t axis, uint32_t reason, float strength) {
    if (g_traffic_occupancy_count >= MDTBMaxTrafficOccupancies) {
        return;
    }

    g_traffic_occupancies[g_traffic_occupancy_count].position = position;
    g_traffic_occupancies[g_traffic_occupancy_count].radius = radius;
    g_traffic_occupancies[g_traffic_occupancy_count].block_index = block_index;
    g_traffic_occupancies[g_traffic_occupancy_count].axis = axis;
    g_traffic_occupancies[g_traffic_occupancy_count].reason = reason;
    g_traffic_occupancies[g_traffic_occupancy_count].strength = strength;
    g_traffic_occupancy_count += 1u;
}

static void clear_traffic_occupancies(void) {
    g_traffic_occupancy_count = 0u;
}

static int block_is_active_at_position(uint32_t block_index, MDTBFloat3 position) {
    if (block_index >= g_block_count) {
        return 0;
    }

    const MDTBBlockDescriptor *block = &g_blocks[block_index];
    const float activation_distance = block->activation_radius;
    return distance_squared_xz(position, block->origin) <= (activation_distance * activation_distance);
}

static uint32_t nearest_block_index_for_position(MDTBFloat3 position) {
    uint32_t best_index = MDTBIndexNone;
    float best_distance_squared = 1000000.0f;

    for (size_t index = 0u; index < g_block_count; ++index) {
        const float block_distance_squared = distance_squared_xz(position, g_blocks[index].origin);
        if (block_distance_squared < best_distance_squared) {
            best_distance_squared = block_distance_squared;
            best_index = (uint32_t)index;
        }
    }

    return best_index;
}

static const MDTBVehicleTuning *vehicle_tuning_for_kind(uint32_t kind) {
    static const MDTBVehicleTuning kSedanTuning = {
        16.5f, 5.4f, 3.1f, 7.2f, 3.4f, 1.80f, 1.18f, 1.72f,
        1.12f, 0.42f, -0.16f, 1.95f, 2.65f, 0.60f, -0.40f, 0.30f,
        7.8f, 1.36f, 1.42f
    };
    static const MDTBVehicleTuning kCoupeTuning = {
        18.6f, 5.8f, 3.8f, 7.8f, 3.7f, 2.20f, 1.10f, 1.64f,
        1.04f, 0.36f, -0.14f, 1.80f, 2.45f, 0.64f, -0.42f, 0.32f,
        7.2f, 1.26f, 1.34f
    };
    static const MDTBVehicleTuning kMopedTuning = {
        12.4f, 3.6f, 4.4f, 6.6f, 4.2f, 2.90f, 0.68f, 1.38f,
        0.96f, 0.18f, 0.0f, 1.30f, 1.82f, 0.78f, -0.46f, 0.36f,
        5.8f, 1.04f, 0.72f
    };
    static const MDTBVehicleTuning kBicycleTuning = {
        9.2f, 2.4f, 3.2f, 5.4f, 3.9f, 3.30f, 0.46f, 1.24f,
        1.04f, 0.05f, 0.0f, 0.96f, 1.36f, 0.86f, -0.48f, 0.38f,
        5.2f, 1.08f, 0.48f
    };
    static const MDTBVehicleTuning kMotorcycleTuning = {
        21.4f, 6.0f, 4.8f, 8.1f, 3.9f, 2.55f, 0.76f, 1.48f,
        1.02f, 0.24f, 0.0f, 1.42f, 1.96f, 0.82f, -0.44f, 0.34f,
        6.4f, 1.12f, 0.78f
    };

    switch (kind) {
        case MDTBVehicleKindMotorcycle:
            return &kMotorcycleTuning;
        case MDTBVehicleKindBicycle:
            return &kBicycleTuning;
        case MDTBVehicleKindCoupe:
            return &kCoupeTuning;
        case MDTBVehicleKindMoped:
            return &kMopedTuning;
        case MDTBVehicleKindSedan:
        default:
            return &kSedanTuning;
    }
}

static float vehicle_surface_grip_multiplier(uint32_t surface_kind) {
    switch (surface_kind) {
        case MDTBSurfaceRoad:
            return 1.0f;
        case MDTBSurfaceCurb:
            return 0.58f;
        case MDTBSurfaceLot:
            return 0.72f;
        case MDTBSurfaceSidewalk:
        default:
            return 0.48f;
    }
}

static int vehicle_is_two_wheel(uint32_t kind) {
    return kind == MDTBVehicleKindBicycle ||
        kind == MDTBVehicleKindMoped ||
        kind == MDTBVehicleKindMotorcycle;
}

static float vehicle_lane_offset_for_heading(uint32_t axis, float heading, float lane_magnitude) {
    if (axis == MDTBRoadAxisNorthSouth) {
        return cosf(heading) >= 0.0f ? lane_magnitude : -lane_magnitude;
    }

    return sinf(heading) >= 0.0f ? -lane_magnitude : lane_magnitude;
}

static float vehicle_lane_heading_for_axis(uint32_t axis, float current_heading) {
    if (axis == MDTBRoadAxisNorthSouth) {
        return cosf(current_heading) >= 0.0f ? 0.0f : kPi;
    }

    return sinf(current_heading) >= 0.0f ? (kPi * 0.5f) : -(kPi * 0.5f);
}

static MDTBFloat3 current_player_position(const MDTBEngineState *state) {
    if (state != NULL && state->traversal_mode == MDTBTraversalModeVehicle) {
        return state->active_vehicle_position;
    }

    if (state != NULL) {
        return state->actor_position;
    }

    return make_float3(0.0f, 0.0f, 0.0f);
}

static int weapon_is_owned(const MDTBEngineState *state, uint32_t weapon_kind) {
    if (state == NULL) {
        return 0;
    }

    switch (weapon_kind) {
        case MDTBEquippedWeaponLeadPipe:
            return state->melee_weapon_owned != 0u;
        case MDTBEquippedWeaponPistol:
            return state->firearm_owned != 0u;
        default:
            return 0;
    }
}

static int combat_target_kind_is_active(const MDTBEngineState *state, uint32_t target_kind) {
    if (state == NULL) {
        return 0;
    }

    switch (target_kind) {
        case MDTBCombatTargetDummy:
            return state->combat_target_health > 0.0f &&
                state->combat_target_reset_timer <= 0.0f;
        case MDTBCombatTargetLookout:
            return state->combat_hostile_health > 0.0f &&
                state->combat_hostile_reset_timer <= 0.0f;
        default:
            return 0;
    }
}

static MDTBFloat3 combat_target_position_for_kind(const MDTBEngineState *state, uint32_t target_kind) {
    if (state == NULL) {
        return make_float3(0.0f, 0.0f, 0.0f);
    }

    switch (target_kind) {
        case MDTBCombatTargetDummy:
            return state->combat_target_position;
        case MDTBCombatTargetLookout:
            return state->combat_hostile_position;
        default:
            return make_float3(0.0f, 0.0f, 0.0f);
    }
}

static MDTBFloat3 combat_target_aim_position_for_kind(const MDTBEngineState *state, uint32_t target_kind) {
    MDTBFloat3 position = combat_target_position_for_kind(state, target_kind);

    switch (target_kind) {
        case MDTBCombatTargetDummy:
            position.y += 1.35f;
            break;
        case MDTBCombatTargetLookout:
            position.y += 1.42f;
            break;
        default:
            break;
    }

    return position;
}

static float combat_target_distance_xz(const MDTBEngineState *state, uint32_t target_kind) {
    return distance_squared_xz(current_player_position(state), combat_target_position_for_kind(state, target_kind));
}

static uint32_t preferred_focus_target(const MDTBEngineState *state) {
    if (state == NULL) {
        return MDTBCombatTargetNone;
    }

    if (combat_target_kind_is_active(state, state->combat_focus_target_kind)) {
        return state->combat_focus_target_kind;
    }

    if (state->combat_target_in_range != 0u && combat_target_kind_is_active(state, MDTBCombatTargetDummy)) {
        return MDTBCombatTargetDummy;
    }

    if (state->combat_hostile_in_range != 0u && combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
        return MDTBCombatTargetLookout;
    }

    return MDTBCombatTargetNone;
}

static void update_combat_focus(MDTBEngineState *state) {
    MDTBFloat3 origin;
    MDTBFloat3 forward;
    MDTBFloat3 forward_flat;
    uint32_t target_kinds[2] = {MDTBCombatTargetDummy, MDTBCombatTargetLookout};
    uint32_t best_kind = MDTBCombatTargetNone;
    float best_score = -1000000.0f;
    float best_distance = 0.0f;
    float best_alignment = 0.0f;

    if (state == NULL) {
        return;
    }

    state->combat_focus_target_kind = MDTBCombatTargetNone;
    state->combat_focus_distance = 0.0f;
    state->combat_focus_alignment = 0.0f;

    if (state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    origin = actor_focus_position(state);
    forward = view_forward(state->camera.yaw, state->camera.pitch);
    forward_flat = normalize_flat(make_float3(forward.x, 0.0f, forward.z));

    for (size_t index = 0u; index < 2u; ++index) {
        const uint32_t target_kind = target_kinds[index];
        const MDTBFloat3 target = combat_target_aim_position_for_kind(state, target_kind);
        const MDTBFloat3 to_target = make_float3(target.x - origin.x, target.y - origin.y, target.z - origin.z);
        const float distance = sqrtf((to_target.x * to_target.x) + (to_target.y * to_target.y) + (to_target.z * to_target.z));
        const float flat_distance = sqrtf(combat_target_distance_xz(state, target_kind));
        const float direction_length = sqrtf((to_target.x * to_target.x) + (to_target.y * to_target.y) + (to_target.z * to_target.z));
        const float alignment = direction_length <= 0.0001f
            ? 0.0f
            : ((forward.x * to_target.x) + (forward.y * to_target.y) + (forward.z * to_target.z)) / direction_length;
        const MDTBFloat3 to_target_flat = normalize_flat(make_float3(target.x - origin.x, 0.0f, target.z - origin.z));
        const float flat_alignment = dot_flat(forward_flat, to_target_flat);
        float score;

        if (!combat_target_kind_is_active(state, target_kind)) {
            continue;
        }

        if (distance > (kPistolRange + 2.0f)) {
            continue;
        }

        if (alignment < 0.52f && flat_alignment < 0.56f) {
            continue;
        }

        score = alignment * 5.0f;
        score += flat_alignment * 1.6f;
        score -= distance * 0.10f;
        if (target_kind == preferred_focus_target(state)) {
            score += 0.28f;
        }

        if (score > best_score) {
            best_score = score;
            best_kind = target_kind;
            best_distance = flat_distance;
            best_alignment = fmaxf(alignment, flat_alignment);
        }
    }

    state->combat_focus_target_kind = best_kind;
    state->combat_focus_distance = best_kind == MDTBCombatTargetNone ? 0.0f : best_distance;
    state->combat_focus_alignment = best_kind == MDTBCombatTargetNone ? 0.0f : clampf(best_alignment, 0.0f, 1.0f);
}

static uint32_t select_melee_target(const MDTBEngineState *state) {
    const MDTBFloat3 origin = state->actor_position;
    const MDTBFloat3 forward = normalize_flat(view_forward(state->camera.yaw, 0.0f));
    uint32_t target_kinds[2] = {MDTBCombatTargetDummy, MDTBCombatTargetLookout};
    uint32_t best_kind = MDTBCombatTargetNone;
    float best_score = 1000000.0f;

    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return MDTBCombatTargetNone;
    }

    for (size_t index = 0u; index < 2u; ++index) {
        const uint32_t target_kind = target_kinds[index];
        const MDTBFloat3 target = combat_target_position_for_kind(state, target_kind);
        const MDTBFloat3 to_target = make_float3(target.x - origin.x, 0.0f, target.z - origin.z);
        const float distance_squared = distance_squared_xz(origin, target);
        const MDTBFloat3 direction = normalize_flat(to_target);
        const float alignment = dot_flat(forward, direction);
        float score;

        if (!combat_target_kind_is_active(state, target_kind)) {
            continue;
        }

        if (distance_squared > (kMeleeTargetRange * kMeleeTargetRange) || alignment < kMeleeArcDot) {
            continue;
        }

        score = distance_squared - (alignment * 4.0f);
        if (target_kind == state->combat_focus_target_kind) {
            score -= 0.30f;
        }

        if (score < best_score) {
            best_score = score;
            best_kind = target_kind;
        }
    }

    return best_kind;
}

static uint32_t select_firearm_target(const MDTBEngineState *state, MDTBFloat3 origin, MDTBFloat3 *shot_end_out) {
    MDTBFloat3 forward = view_forward(state->camera.yaw, state->camera.pitch);
    const float forward_length = sqrtf((forward.x * forward.x) + (forward.y * forward.y) + (forward.z * forward.z));
    MDTBFloat3 shot_end;
    MDTBFloat3 forward_flat;
    uint32_t target_kinds[2] = {MDTBCombatTargetDummy, MDTBCombatTargetLookout};
    uint32_t best_kind = MDTBCombatTargetNone;
    float best_score = 1000000.0f;

    if (shot_end_out != NULL) {
        *shot_end_out = origin;
    }

    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return MDTBCombatTargetNone;
    }

    if (forward_length <= 0.0001f) {
        forward = make_float3(0.0f, 0.0f, -1.0f);
    } else {
        forward = make_float3(
            forward.x / forward_length,
            forward.y / forward_length,
            forward.z / forward_length
        );
    }

    shot_end = make_float3(
        origin.x + (forward.x * kPistolRange),
        origin.y + (forward.y * kPistolRange),
        origin.z + (forward.z * kPistolRange)
    );
    forward_flat = normalize_flat(make_float3(forward.x, 0.0f, forward.z));

    if (shot_end_out != NULL) {
        *shot_end_out = shot_end;
    }

    for (size_t index = 0u; index < 2u; ++index) {
        const uint32_t target_kind = target_kinds[index];
        const MDTBFloat3 target = combat_target_aim_position_for_kind(state, target_kind);
        const MDTBFloat3 to_target_flat = normalize_flat(make_float3(target.x - origin.x, 0.0f, target.z - origin.z));
        const float flat_alignment = dot_flat(forward_flat, to_target_flat);
        const float impact_radius = target_kind == MDTBCombatTargetLookout ? 0.78f : 0.85f;
        const float aim_dot_threshold = target_kind == MDTBCombatTargetLookout ? (kPistolAimDot - kLookoutAimBias) : kPistolAimDot;
        const float distance_squared = point_to_segment_distance_squared_xz(target, origin, shot_end);
        float score;

        if (!combat_target_kind_is_active(state, target_kind)) {
            continue;
        }

        if (flat_alignment < aim_dot_threshold || distance_squared > (impact_radius * impact_radius)) {
            continue;
        }

        score = distance_squared;
        score += (1.0f - flat_alignment) * 4.0f;
        score += combat_target_distance_xz(state, target_kind) * 0.0015f;
        if (target_kind == state->combat_focus_target_kind) {
            score -= 0.14f;
        }

        if (score < best_score) {
            best_score = score;
            best_kind = target_kind;
            if (shot_end_out != NULL) {
                *shot_end_out = target;
            }
        }
    }

    return best_kind;
}

static void refresh_combat_proximity(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    state->melee_weapon_pickup_position = kLeadPipePickupPosition;
    state->firearm_pickup_position = kPistolPickupPosition;
    state->combat_target_position = kPracticeDummyPosition;
    state->melee_weapon_pickup_in_range = 0u;
    state->firearm_pickup_in_range = 0u;
    state->combat_target_in_range = 0u;
    state->combat_hostile_in_range = 0u;
    state->combat_focus_target_kind = MDTBCombatTargetNone;
    state->combat_focus_distance = 0.0f;
    state->combat_focus_alignment = 0.0f;

    if (state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    if (!state->melee_weapon_owned &&
        distance_squared_xz(state->actor_position, kLeadPipePickupPosition) <= (kMeleePickupRadius * kMeleePickupRadius)) {
        state->melee_weapon_pickup_in_range = 1u;
    }

    if (!state->firearm_owned &&
        distance_squared_xz(state->actor_position, kPistolPickupPosition) <= (kPistolPickupRadius * kPistolPickupRadius)) {
        state->firearm_pickup_in_range = 1u;
    }

    if (combat_target_kind_is_active(state, MDTBCombatTargetDummy) &&
        distance_squared_xz(state->actor_position, state->combat_target_position) <= (kMeleeTargetProximityRadius * kMeleeTargetProximityRadius)) {
        state->combat_target_in_range = 1u;
    }

    if (combat_target_kind_is_active(state, MDTBCombatTargetLookout) &&
        distance_squared_xz(state->actor_position, state->combat_hostile_position) <= (kMeleeTargetProximityRadius * kMeleeTargetProximityRadius)) {
        state->combat_hostile_in_range = 1u;
    }

    update_combat_focus(state);
}

static void clear_melee_attack(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    state->melee_attack_phase = MDTBMeleeAttackIdle;
    state->melee_attack_timer = 0.0f;
    state->melee_attack_connected = 0u;
}

static void clear_firearm_last_shot(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    state->firearm_last_shot_from = make_float3(0.0f, 0.0f, 0.0f);
    state->firearm_last_shot_to = make_float3(0.0f, 0.0f, 0.0f);
    state->firearm_last_shot_timer = 0.0f;
    state->firearm_last_shot_hit = 0u;
}

static void cancel_firearm_reload(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    state->firearm_reloading = 0u;
    state->firearm_reload_timer = 0.0f;
}

static void equip_weapon(MDTBEngineState *state, uint32_t weapon_kind) {
    if (state == NULL) {
        return;
    }

    if (weapon_kind != MDTBEquippedWeaponNone && !weapon_is_owned(state, weapon_kind)) {
        return;
    }

    if (weapon_kind != MDTBEquippedWeaponLeadPipe) {
        clear_melee_attack(state);
    }

    if (weapon_kind != MDTBEquippedWeaponPistol) {
        cancel_firearm_reload(state);
    }

    state->equipped_weapon_kind = weapon_kind;
}

static void start_melee_attack(MDTBEngineState *state) {
    if (state == NULL ||
        state->traversal_mode != MDTBTraversalModeOnFoot ||
        state->equipped_weapon_kind != MDTBEquippedWeaponLeadPipe ||
        !state->melee_weapon_owned ||
        state->melee_attack_phase != MDTBMeleeAttackIdle) {
        return;
    }

    state->melee_attack_phase = MDTBMeleeAttackWindup;
    state->melee_attack_timer = kMeleeWindupDuration;
    state->melee_attack_connected = 0u;
    state->combat_last_hit_target_kind = MDTBCombatTargetNone;
}

static void apply_damage_to_target(MDTBEngineState *state, uint32_t target_kind, float damage, float live_reaction, float down_reaction) {
    if (state == NULL || !combat_target_kind_is_active(state, target_kind)) {
        return;
    }

    switch (target_kind) {
        case MDTBCombatTargetDummy:
            state->combat_target_health = fmaxf(0.0f, state->combat_target_health - damage);
            state->combat_target_reaction = fmaxf(
                state->combat_target_reaction,
                state->combat_target_health > 0.0f ? live_reaction : down_reaction
            );
            if (state->combat_target_health <= 0.0f) {
                state->combat_target_reset_timer = kPracticeDummyRespawnDelay;
            }
            break;
        case MDTBCombatTargetLookout:
            state->combat_hostile_health = fmaxf(0.0f, state->combat_hostile_health - damage);
            state->combat_hostile_reaction = fmaxf(
                state->combat_hostile_reaction,
                state->combat_hostile_health > 0.0f ? live_reaction : down_reaction
            );
            state->combat_hostile_alert = state->combat_hostile_health > 0.0f ? 1.0f : 0.0f;
            if (state->combat_hostile_health <= 0.0f) {
                state->combat_hostile_reset_timer = kLookoutRespawnDelay;
            }
            break;
        default:
            return;
    }

    state->combat_last_hit_target_kind = target_kind;
}

static void pickup_nearest_weapon(MDTBEngineState *state) {
    float best_distance_squared = 1000000.0f;
    uint32_t selected_weapon = MDTBEquippedWeaponNone;

    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    if (!state->melee_weapon_owned && state->melee_weapon_pickup_in_range != 0u) {
        const float distance_squared = distance_squared_xz(state->actor_position, kLeadPipePickupPosition);
        if (distance_squared < best_distance_squared) {
            best_distance_squared = distance_squared;
            selected_weapon = MDTBEquippedWeaponLeadPipe;
        }
    }

    if (!state->firearm_owned && state->firearm_pickup_in_range != 0u) {
        const float distance_squared = distance_squared_xz(state->actor_position, kPistolPickupPosition);
        if (distance_squared < best_distance_squared) {
            best_distance_squared = distance_squared;
            selected_weapon = MDTBEquippedWeaponPistol;
        }
    }

    switch (selected_weapon) {
        case MDTBEquippedWeaponLeadPipe:
            state->melee_weapon_owned = 1u;
            equip_weapon(state, MDTBEquippedWeaponLeadPipe);
            break;
        case MDTBEquippedWeaponPistol:
            state->firearm_owned = 1u;
            if (state->firearm_clip_ammo == 0u && state->firearm_reserve_ammo == 0u) {
                state->firearm_clip_ammo = kPistolClipCapacity;
                state->firearm_reserve_ammo = kPistolInitialReserveAmmo;
            }
            equip_weapon(state, MDTBEquippedWeaponPistol);
            break;
        default:
            break;
    }
}

static void start_firearm_reload(MDTBEngineState *state) {
    if (state == NULL ||
        state->traversal_mode != MDTBTraversalModeOnFoot ||
        state->equipped_weapon_kind != MDTBEquippedWeaponPistol ||
        !state->firearm_owned ||
        state->firearm_reloading != 0u ||
        state->firearm_clip_ammo >= kPistolClipCapacity ||
        state->firearm_reserve_ammo == 0u) {
        return;
    }

    clear_melee_attack(state);
    state->firearm_reloading = 1u;
    state->firearm_reload_timer = kPistolReloadDuration;
}

static void complete_firearm_reload(MDTBEngineState *state) {
    uint32_t missing_rounds;
    uint32_t loaded_rounds;

    if (state == NULL) {
        return;
    }

    missing_rounds = kPistolClipCapacity - state->firearm_clip_ammo;
    loaded_rounds = missing_rounds < state->firearm_reserve_ammo ? missing_rounds : state->firearm_reserve_ammo;

    state->firearm_clip_ammo += loaded_rounds;
    state->firearm_reserve_ammo -= loaded_rounds;
    state->firearm_reloading = 0u;
    state->firearm_reload_timer = 0.0f;
}

static void fire_pistol(MDTBEngineState *state) {
    MDTBFloat3 shot_origin;
    MDTBFloat3 shot_end;
    MDTBFloat3 muzzle_right;
    uint32_t hit_target_kind;

    if (state == NULL ||
        state->traversal_mode != MDTBTraversalModeOnFoot ||
        state->equipped_weapon_kind != MDTBEquippedWeaponPistol ||
        !state->firearm_owned ||
        state->firearm_reloading != 0u ||
        state->firearm_cooldown_timer > 0.0f) {
        return;
    }

    if (state->firearm_clip_ammo == 0u) {
        if (state->firearm_reserve_ammo > 0u) {
            start_firearm_reload(state);
        }
        return;
    }

    state->firearm_clip_ammo -= 1u;
    state->firearm_cooldown_timer = kPistolShotCooldown;
    state->combat_last_hit_target_kind = MDTBCombatTargetNone;

    shot_origin = actor_focus_position(state);
    muzzle_right = make_float3(cosf(state->camera.yaw), 0.0f, sinf(state->camera.yaw));
    shot_origin.x += muzzle_right.x * 0.14f;
    shot_origin.z += muzzle_right.z * 0.14f;
    shot_origin.y -= 0.04f;

    hit_target_kind = select_firearm_target(state, shot_origin, &shot_end);
    state->firearm_last_shot_from = shot_origin;
    state->firearm_last_shot_to = shot_end;
    state->firearm_last_shot_timer = kPistolShotFlashDuration;
    state->firearm_last_shot_hit = hit_target_kind != MDTBCombatTargetNone ? 1u : 0u;

    if (hit_target_kind != MDTBCombatTargetNone) {
        apply_damage_to_target(
            state,
            hit_target_kind,
            hit_target_kind == MDTBCombatTargetLookout ? 28.0f : 22.0f,
            hit_target_kind == MDTBCombatTargetLookout ? 0.98f : 0.82f,
            hit_target_kind == MDTBCombatTargetLookout ? 1.42f : 1.30f
        );
    }
}

static void step_combat_state(MDTBEngineState *state, float dt, int wants_attack, int wants_reload) {
    if (state == NULL) {
        return;
    }

    state->combat_target_reaction = approachf(state->combat_target_reaction, 0.0f, 7.0f, dt);
    state->combat_hostile_reaction = approachf(state->combat_hostile_reaction, 0.0f, 7.6f, dt);
    state->firearm_cooldown_timer = fmaxf(state->firearm_cooldown_timer - dt, 0.0f);

    if (state->firearm_last_shot_timer > 0.0f) {
        state->firearm_last_shot_timer = fmaxf(state->firearm_last_shot_timer - dt, 0.0f);
        if (state->firearm_last_shot_timer <= 0.0f) {
            state->firearm_last_shot_hit = 0u;
        }
    }

    if (state->combat_target_reset_timer > 0.0f) {
        state->combat_target_reset_timer = fmaxf(state->combat_target_reset_timer - dt, 0.0f);
        if (state->combat_target_reset_timer <= 0.0f) {
            state->combat_target_health = kPracticeDummyMaxHealth;
            state->combat_target_reaction = 0.0f;
        }
    }

    if (state->combat_hostile_reset_timer > 0.0f) {
        state->combat_hostile_reset_timer = fmaxf(state->combat_hostile_reset_timer - dt, 0.0f);
        if (state->combat_hostile_reset_timer <= 0.0f) {
            state->combat_hostile_health = kLookoutMaxHealth;
            state->combat_hostile_reaction = 0.0f;
            state->combat_hostile_alert = 0.0f;
            state->combat_hostile_position = kLookoutBasePosition;
            state->combat_hostile_heading = kPi;
        }
    }

    if (state->firearm_reloading != 0u) {
        state->firearm_reload_timer = fmaxf(state->firearm_reload_timer - dt, 0.0f);
        if (state->firearm_reload_timer <= 0.0f) {
            complete_firearm_reload(state);
        }
    }

    if (state->traversal_mode != MDTBTraversalModeOnFoot) {
        clear_melee_attack(state);
        cancel_firearm_reload(state);
        refresh_combat_proximity(state);
        return;
    }

    if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
        const MDTBFloat3 actor_position = current_player_position(state);
        const float alert_distance_squared = distance_squared_xz(actor_position, state->combat_hostile_position);
        const float patrol_phase = state->elapsed_time * 0.92f;
        const float patrol_depth_phase = state->elapsed_time * 1.56f;
        const float alert_target =
            (alert_distance_squared <= (kLookoutAlertDistance * kLookoutAlertDistance) ||
             state->combat_focus_target_kind == MDTBCombatTargetLookout) ? 1.0f : 0.18f;
        const float patrol_scale = 1.0f - (state->combat_hostile_alert * 0.58f);
        const float step_toward_player = clampf(state->combat_hostile_alert, 0.0f, 0.9f);
        MDTBFloat3 desired_position = make_float3(
            kLookoutBasePosition.x + sinf(patrol_phase) * 3.1f * patrol_scale,
            kLookoutBasePosition.y,
            kLookoutBasePosition.z + cosf(patrol_depth_phase) * 0.9f
        );
        const MDTBFloat3 to_actor = make_float3(
            actor_position.x - state->combat_hostile_position.x,
            0.0f,
            actor_position.z - state->combat_hostile_position.z
        );
        float desired_heading = state->combat_hostile_heading;

        state->combat_hostile_alert = approachf(state->combat_hostile_alert, alert_target, 3.8f, dt);
        desired_position.x += normalize_flat(to_actor).x * step_toward_player * 0.9f;
        desired_position.z += normalize_flat(to_actor).z * step_toward_player * 0.9f;
        state->combat_hostile_position = approach_float3(state->combat_hostile_position, desired_position, 5.0f, dt);

        if (fabsf(to_actor.x) > 0.001f || fabsf(to_actor.z) > 0.001f) {
            desired_heading = atan2f(to_actor.x, -to_actor.z);
        } else {
            desired_heading = sinf(patrol_phase) * 0.16f;
        }

        state->combat_hostile_heading = approach_angle(state->combat_hostile_heading, desired_heading, 8.4f, dt);
    } else {
        state->combat_hostile_alert = approachf(state->combat_hostile_alert, 0.0f, 6.0f, dt);
    }

    refresh_combat_proximity(state);

    if (wants_reload) {
        start_firearm_reload(state);
    }

    if (wants_attack) {
        switch (state->equipped_weapon_kind) {
            case MDTBEquippedWeaponLeadPipe:
                start_melee_attack(state);
                break;
            case MDTBEquippedWeaponPistol:
                fire_pistol(state);
                break;
            default:
                break;
        }
    }

    if (state->melee_attack_phase != MDTBMeleeAttackIdle) {
        state->melee_attack_timer = fmaxf(state->melee_attack_timer - dt, 0.0f);
    }

    switch (state->melee_attack_phase) {
        case MDTBMeleeAttackWindup:
            if (state->melee_attack_timer <= 0.0f) {
                const uint32_t melee_target_kind = select_melee_target(state);
                state->melee_attack_phase = MDTBMeleeAttackStrike;
                state->melee_attack_timer = kMeleeStrikeDuration;
                state->melee_attack_connected = 0u;
                if (melee_target_kind != MDTBCombatTargetNone) {
                    apply_damage_to_target(
                        state,
                        melee_target_kind,
                        melee_target_kind == MDTBCombatTargetLookout ? 30.0f : 34.0f,
                        melee_target_kind == MDTBCombatTargetLookout ? 1.08f : 1.0f,
                        melee_target_kind == MDTBCombatTargetLookout ? 1.46f : 1.35f
                    );
                    state->melee_attack_connected = 1u;
                }
            }
            break;
        case MDTBMeleeAttackStrike:
            if (state->melee_attack_timer <= 0.0f) {
                state->melee_attack_phase = MDTBMeleeAttackRecovery;
                state->melee_attack_timer = kMeleeRecoveryDuration;
            }
            break;
        case MDTBMeleeAttackRecovery:
            if (state->melee_attack_timer <= 0.0f) {
                clear_melee_attack(state);
            }
            break;
        case MDTBMeleeAttackIdle:
        default:
            break;
    }

    refresh_combat_proximity(state);
}

static int position_overlaps_collision(MDTBFloat3 position, float radius) {
    for (size_t index = 0u; index < g_collision_box_count; ++index) {
        const MDTBBox *box = &g_collision_boxes[index];
        if (fabsf(position.x - box->center.x) <= box->half_extents.x + radius &&
            fabsf(position.y - box->center.y) <= box->half_extents.y + radius &&
            fabsf(position.z - box->center.z) <= box->half_extents.z + radius) {
            return 1;
        }
    }

    return 0;
}

static float resolve_collision_axis_with_radius(float fixed_value, float current, float proposed, int is_x_axis, float radius) {
    float resolved = proposed;

    for (size_t index = 0u; index < g_collision_box_count; ++index) {
        const MDTBBox *box = &g_collision_boxes[index];
        const float min_x = box->center.x - box->half_extents.x - radius - kCollisionGap;
        const float max_x = box->center.x + box->half_extents.x + radius + kCollisionGap;
        const float min_z = box->center.z - box->half_extents.z - radius - kCollisionGap;
        const float max_z = box->center.z + box->half_extents.z + radius + kCollisionGap;

        if (is_x_axis) {
            if (fixed_value <= min_z || fixed_value >= max_z) {
                continue;
            }

            if (resolved > current && current <= min_x && resolved > min_x && resolved < max_x) {
                resolved = min_x;
            } else if (resolved < current && current >= max_x && resolved < max_x && resolved > min_x) {
                resolved = max_x;
            }
        } else {
            if (fixed_value <= min_x || fixed_value >= max_x) {
                continue;
            }

            if (resolved > current && current <= min_z && resolved > min_z && resolved < max_z) {
                resolved = min_z;
            } else if (resolved < current && current >= max_z && resolved < max_z && resolved > min_z) {
                resolved = max_z;
            }
        }
    }

    return resolved;
}

static float handoff_candidate_score(const MDTBEngineState *state, MDTBFloat3 actor_position, const MDTBVehicleAnchor *anchor, float *distance_squared_out) {
    const MDTBFloat3 forward = normalize_flat(view_forward(state->camera.yaw, 0.0f));
    const MDTBFloat3 to_anchor = make_float3(anchor->position.x - actor_position.x, 0.0f, anchor->position.z - actor_position.z);
    const MDTBFloat3 anchor_direction = normalize_flat(to_anchor);
    const float distance_squared = distance_squared_xz(actor_position, anchor->position);
    float score = distance_squared;

    if (distance_squared_out != NULL) {
        *distance_squared_out = distance_squared;
    }

    score -= fmaxf(dot_flat(forward, anchor_direction), -0.5f) * 6.0f;

    if (state->active_link_index < g_road_link_count &&
        g_road_links[state->active_link_index].axis == anchor->lane_axis) {
        score -= 0.85f;
    }

    if (anchor->parking_state == MDTBVehicleParkingStateCurbside) {
        score -= 0.20f;
    }

    return score;
}

static void insert_vehicle_candidate(uint32_t anchor_index, float score, uint32_t ranked_indices[3], float ranked_scores[3]) {
    for (size_t slot = 0u; slot < 3u; ++slot) {
        if (score >= ranked_scores[slot]) {
            continue;
        }

        for (size_t move = 2u; move > slot; --move) {
            ranked_scores[move] = ranked_scores[move - 1u];
            ranked_indices[move] = ranked_indices[move - 1u];
        }

        ranked_scores[slot] = score;
        ranked_indices[slot] = anchor_index;
        return;
    }
}

static int ranked_vehicle_contains(uint32_t anchor_index, const uint32_t ranked_indices[3]) {
    for (size_t index = 0u; index < 3u; ++index) {
        if (ranked_indices[index] == anchor_index) {
            return 1;
        }
    }

    return 0;
}

static void assign_vehicle_selection_from_ranked(
    MDTBEngineState *state,
    uint32_t preferred_anchor_index,
    uint32_t ranked_indices[3]
) {
    uint32_t ordered_indices[3] = {MDTBIndexNone, MDTBIndexNone, MDTBIndexNone};
    size_t write_index = 0u;

    if (preferred_anchor_index < g_vehicle_anchor_count && ranked_vehicle_contains(preferred_anchor_index, ranked_indices)) {
        ordered_indices[write_index++] = preferred_anchor_index;
    }

    for (size_t index = 0u; index < 3u && write_index < 3u; ++index) {
        const uint32_t candidate = ranked_indices[index];
        if (candidate >= g_vehicle_anchor_count) {
            continue;
        }

        if (write_index > 0u && ordered_indices[0] == candidate) {
            continue;
        }

        ordered_indices[write_index++] = candidate;
    }

    state->nearby_vehicle_anchor_index = ordered_indices[0];
    state->secondary_vehicle_anchor_index = ordered_indices[1];
    state->tertiary_vehicle_anchor_index = ordered_indices[2];
}

static void cycle_vehicle_selection(MDTBEngineState *state) {
    uint32_t ordered_indices[3];

    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    ordered_indices[0] = state->nearby_vehicle_anchor_index;
    ordered_indices[1] = state->secondary_vehicle_anchor_index;
    ordered_indices[2] = state->tertiary_vehicle_anchor_index;

    if (ordered_indices[1] >= g_vehicle_anchor_count) {
        return;
    }

    state->nearby_vehicle_anchor_index = ordered_indices[1];
    state->secondary_vehicle_anchor_index = ordered_indices[2];
    state->tertiary_vehicle_anchor_index = ordered_indices[0];

    if (state->tertiary_vehicle_anchor_index >= g_vehicle_anchor_count) {
        state->tertiary_vehicle_anchor_index = MDTBIndexNone;
    }

    if (state->vehicle_selection_locked) {
        state->locked_vehicle_anchor_index = state->nearby_vehicle_anchor_index;
    }
}

static void toggle_vehicle_selection_lock(MDTBEngineState *state) {
    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    if (state->nearby_vehicle_anchor_index >= g_vehicle_anchor_count) {
        state->vehicle_selection_locked = 0u;
        state->locked_vehicle_anchor_index = MDTBIndexNone;
        return;
    }

    if (state->vehicle_selection_locked &&
        state->locked_vehicle_anchor_index == state->nearby_vehicle_anchor_index) {
        state->vehicle_selection_locked = 0u;
        state->locked_vehicle_anchor_index = MDTBIndexNone;
        return;
    }

    state->vehicle_selection_locked = 1u;
    state->locked_vehicle_anchor_index = state->nearby_vehicle_anchor_index;
}

static void update_runtime_activity(MDTBEngineState *state) {
    MDTBFloat3 actor_position;
    uint32_t active_block_index = MDTBIndexNone;
    uint32_t previous_selected_vehicle_index;
    uint32_t previous_locked_vehicle_index;
    uint32_t previous_selection_locked;
    float best_block_distance_squared = 1000000.0f;
    uint32_t nearby_block_count = 0u;
    float best_link_distance_squared = 1000000.0f;
    const float preview_radius_squared = kVehiclePreviewRadius * kVehiclePreviewRadius;
    uint32_t ranked_vehicle_indices[3] = {MDTBIndexNone, MDTBIndexNone, MDTBIndexNone};
    float ranked_vehicle_scores[3] = {1000000.0f, 1000000.0f, 1000000.0f};

    if (state == NULL) {
        return;
    }

    actor_position = current_player_position(state);
    previous_selected_vehicle_index = state->nearby_vehicle_anchor_index;
    previous_locked_vehicle_index = state->locked_vehicle_anchor_index;
    previous_selection_locked = state->vehicle_selection_locked;
    state->active_block_index = MDTBIndexNone;
    state->nearby_block_count = 0u;
    state->active_link_index = MDTBIndexNone;
    state->active_pedestrian_spawn_count = 0u;
    state->active_vehicle_spawn_count = 0u;
    state->nearby_vehicle_anchor_index = MDTBIndexNone;
    state->secondary_vehicle_anchor_index = MDTBIndexNone;
    state->tertiary_vehicle_anchor_index = MDTBIndexNone;

    for (size_t index = 0u; index < g_block_count; ++index) {
        const float block_distance_squared = distance_squared_xz(actor_position, g_blocks[index].origin);

        if (block_distance_squared < best_block_distance_squared) {
            best_block_distance_squared = block_distance_squared;
            active_block_index = (uint32_t)index;
        }

        if (block_is_active_at_position((uint32_t)index, actor_position)) {
            nearby_block_count += 1u;
        }
    }

    if (active_block_index == MDTBIndexNone) {
        return;
    }

    state->active_block_index = active_block_index;
    state->nearby_block_count = nearby_block_count > 0u ? nearby_block_count : 1u;

    for (size_t index = 0u; index < g_road_link_count; ++index) {
        const MDTBRoadLink *link = &g_road_links[index];
        float link_distance_squared;

        if (link->from_block_index >= g_block_count || link->to_block_index >= g_block_count) {
            continue;
        }

        if (link->from_block_index != active_block_index && link->to_block_index != active_block_index) {
            continue;
        }

        link_distance_squared = point_to_segment_distance_squared_xz(
            actor_position,
            g_blocks[link->from_block_index].origin,
            g_blocks[link->to_block_index].origin
        );

        if (link_distance_squared < best_link_distance_squared &&
            link_distance_squared <= (kLinkActivationHalfWidth * kLinkActivationHalfWidth)) {
            best_link_distance_squared = link_distance_squared;
            state->active_link_index = (uint32_t)index;
        }
    }

    for (size_t index = 0u; index < g_interest_point_count; ++index) {
        const MDTBInterestPoint *point = &g_interest_points[index];

        if (!block_is_active_at_position(point->block_index, actor_position) && point->block_index != active_block_index) {
            continue;
        }

        if (point->kind == MDTBInterestPointPedestrianSpawn) {
            state->active_pedestrian_spawn_count += 1u;
        } else if (point->kind == MDTBInterestPointVehicleSpawn) {
            state->active_vehicle_spawn_count += 1u;
        }
    }

    for (size_t index = 0u; index < g_vehicle_anchor_count; ++index) {
        const MDTBVehicleAnchor *anchor = &g_vehicle_anchors[index];
        float vehicle_distance_squared;
        int same_chunk = 0;

        if (anchor->block_index >= g_block_count) {
            continue;
        }

        if (active_block_index < g_block_count &&
            g_blocks[anchor->block_index].chunk_index == g_blocks[active_block_index].chunk_index) {
            same_chunk = 1;
        }

        if (!block_is_active_at_position(anchor->block_index, actor_position) && !same_chunk) {
            continue;
        }

        vehicle_distance_squared = distance_squared_xz(actor_position, anchor->position);
        if (vehicle_distance_squared > preview_radius_squared) {
            continue;
        }

        insert_vehicle_candidate(
            (uint32_t)index,
            handoff_candidate_score(state, actor_position, anchor, NULL),
            ranked_vehicle_indices,
            ranked_vehicle_scores
        );
    }

    if (state->traversal_mode == MDTBTraversalModeVehicle) {
        state->nearby_vehicle_anchor_index = state->active_vehicle_anchor_index;
        state->secondary_vehicle_anchor_index = MDTBIndexNone;
        state->tertiary_vehicle_anchor_index = MDTBIndexNone;
        return;
    }

    if (previous_selection_locked &&
        previous_locked_vehicle_index < g_vehicle_anchor_count &&
        ranked_vehicle_contains(previous_locked_vehicle_index, ranked_vehicle_indices)) {
        state->vehicle_selection_locked = 1u;
        state->locked_vehicle_anchor_index = previous_locked_vehicle_index;
        assign_vehicle_selection_from_ranked(state, previous_locked_vehicle_index, ranked_vehicle_indices);
        return;
    }

    state->vehicle_selection_locked = 0u;
    state->locked_vehicle_anchor_index = MDTBIndexNone;

    if (previous_selected_vehicle_index < g_vehicle_anchor_count &&
        ranked_vehicle_contains(previous_selected_vehicle_index, ranked_vehicle_indices)) {
        assign_vehicle_selection_from_ranked(state, previous_selected_vehicle_index, ranked_vehicle_indices);
        return;
    }

    assign_vehicle_selection_from_ranked(state, MDTBIndexNone, ranked_vehicle_indices);
}

static void refresh_traffic_occupancies(const MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    clear_traffic_occupancies();

    for (size_t index = 0u; index < g_vehicle_anchor_count; ++index) {
        const MDTBVehicleAnchor *anchor = &g_vehicle_anchors[index];
        float radius;

        if (anchor->block_index >= g_block_count) {
            continue;
        }

        if (state->traversal_mode == MDTBTraversalModeVehicle &&
            state->active_vehicle_anchor_index == (uint32_t)index) {
            continue;
        }

        radius = traffic_occupancy_radius_for_vehicle_kind(anchor->kind);
        push_traffic_occupancy(
            anchor->position,
            radius,
            anchor->block_index,
            anchor->lane_axis,
            MDTBTrafficOccupancyReasonStagedVehicle,
            anchor->parking_state == MDTBVehicleParkingStateService ? 0.72f : 0.58f
        );
    }

    if (state->traversal_mode == MDTBTraversalModeVehicle) {
        const MDTBFloat3 forward = vehicle_forward_flat(state->active_vehicle_heading);
        const float radius = traffic_occupancy_radius_for_vehicle_kind(state->active_vehicle_kind);
        const MDTBFloat3 hold_position = make_float3(
            state->active_vehicle_position.x + (forward.x * (radius + 1.8f)),
            state->active_vehicle_position.y,
            state->active_vehicle_position.z + (forward.z * (radius + 1.8f))
        );
        const uint32_t axis = state->active_link_index < g_road_link_count ? g_road_links[state->active_link_index].axis : MDTBRoadAxisNorthSouth;

        push_traffic_occupancy(
            state->active_vehicle_position,
            radius + 0.6f,
            state->active_block_index,
            axis,
            MDTBTrafficOccupancyReasonPlayerVehicle,
            1.0f
        );
        push_traffic_occupancy(
            hold_position,
            radius + 1.0f,
            state->active_block_index,
            axis,
            MDTBTrafficOccupancyReasonPlayerVehicle,
            0.86f
        );
    } else if (state->active_link_index < g_road_link_count &&
               (state->surface_kind == MDTBSurfaceRoad || state->surface_kind == MDTBSurfaceCurb)) {
        push_traffic_occupancy(
            state->actor_position,
            2.4f,
            state->active_block_index,
            g_road_links[state->active_link_index].axis,
            MDTBTrafficOccupancyReasonPedestrian,
            0.82f
        );
    }

    if (state->active_link_index < g_road_link_count) {
        const MDTBRoadLink *link = &g_road_links[state->active_link_index];
        const float stop_strength =
            state->traversal_mode == MDTBTraversalModeVehicle ?
            0.54f + fminf(state->camera.move_speed / 18.0f, 0.24f) :
            ((state->surface_kind == MDTBSurfaceRoad || state->surface_kind == MDTBSurfaceCurb) ? 0.76f : 0.48f);
        push_traffic_occupancy(
            link->midpoint,
            2.2f + stop_strength * 1.2f,
            state->active_block_index,
            link->axis,
            MDTBTrafficOccupancyReasonStopZone,
            stop_strength
        );
    }
}

static void push_building(MDTBFloat3 center, MDTBFloat3 half_extents, MDTBFloat4 color) {
    const MDTBBox box = make_box(center, half_extents, color);
    push_scene_box(box);
    push_collision_box(box);
}

static void push_prop(MDTBFloat3 center, MDTBFloat3 half_extents, MDTBFloat4 color, int is_solid) {
    const MDTBBox box = make_box(center, half_extents, color);
    push_scene_box(box);
    if (is_solid) {
        push_collision_box(box);
    }
}

static void push_tree(float x, float z, float canopy_scale) {
    push_prop(
        make_float3(x, 1.0f, z),
        make_float3(0.12f, 1.0f, 0.12f),
        make_float4(0.33f, 0.26f, 0.18f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 2.65f, z),
        make_float3(0.85f * canopy_scale, 0.85f, 0.85f * canopy_scale),
        make_float4(0.29f, 0.46f, 0.28f, 1.0f),
        0
    );
}

static void push_bench(float x, float z) {
    const MDTBFloat4 wood_color = make_float4(0.43f, 0.28f, 0.18f, 1.0f);
    const MDTBFloat4 metal_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);

    push_prop(make_float3(x, 0.42f, z), make_float3(0.62f, 0.08f, 0.24f), wood_color, 1);
    push_prop(make_float3(x, 0.74f, z - 0.20f), make_float3(0.62f, 0.24f, 0.07f), wood_color, 1);

    push_prop(make_float3(x - 0.42f, 0.24f, z), make_float3(0.06f, 0.24f, 0.06f), metal_color, 1);
    push_prop(make_float3(x + 0.42f, 0.24f, z), make_float3(0.06f, 0.24f, 0.06f), metal_color, 1);
    push_prop(make_float3(x - 0.40f, 0.52f, z - 0.18f), make_float3(0.05f, 0.26f, 0.05f), metal_color, 1);
    push_prop(make_float3(x + 0.40f, 0.52f, z - 0.18f), make_float3(0.05f, 0.26f, 0.05f), metal_color, 1);
}

static void push_planter(float x, float z, float size) {
    push_prop(
        make_float3(x, 0.28f, z),
        make_float3(size, 0.28f, size),
        make_float4(0.49f, 0.44f, 0.39f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 0.72f, z),
        make_float3(size * 0.7f, 0.30f, size * 0.7f),
        make_float4(0.31f, 0.49f, 0.28f, 1.0f),
        0
    );
}

static void push_signal_pole(float x, float z) {
    push_prop(
        make_float3(x, 1.15f, z),
        make_float3(0.08f, 1.15f, 0.08f),
        make_float4(0.27f, 0.29f, 0.34f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 2.55f, z + ((z > 0.0f) ? -0.5f : 0.5f)),
        make_float3(0.10f, 0.10f, 0.65f),
        make_float4(0.34f, 0.35f, 0.39f, 1.0f),
        0
    );
}

static void push_parked_car(float x, float z, float half_x, float half_z, MDTBFloat4 color) {
    push_prop(
        make_float3(x, 0.52f, z),
        make_float3(half_x, 0.52f, half_z),
        color,
        1
    );

    push_prop(
        make_float3(x, 0.98f, z),
        make_float3(half_x * 0.55f, 0.34f, half_z * 0.72f),
        make_float4(color.x * 0.88f, color.y * 0.88f, color.z * 0.88f, 1.0f),
        0
    );
}

static void push_bollard(float x, float z, float height) {
    push_prop(
        make_float3(x, height * 0.5f, z),
        make_float3(0.11f, height * 0.5f, 0.11f),
        make_float4(0.42f, 0.40f, 0.36f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, height + 0.07f, z),
        make_float3(0.15f, 0.07f, 0.15f),
        make_float4(0.85f, 0.68f, 0.19f, 1.0f),
        0
    );
}

static void push_trash_bin(float x, float z) {
    push_prop(
        make_float3(x, 0.46f, z),
        make_float3(0.28f, 0.46f, 0.28f),
        make_float4(0.24f, 0.30f, 0.33f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 0.98f, z),
        make_float3(0.33f, 0.06f, 0.33f),
        make_float4(0.30f, 0.37f, 0.40f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, 0.56f, z + 0.18f),
        make_float3(0.16f, 0.18f, 0.02f),
        make_float4(0.17f, 0.19f, 0.20f, 1.0f),
        0
    );
}

static void push_newsstand(float x, float z, int spans_on_x_axis) {
    const MDTBFloat4 body_color = make_float4(0.56f, 0.22f, 0.18f, 1.0f);
    const MDTBFloat4 canopy_color = make_float4(0.90f, 0.78f, 0.26f, 1.0f);
    const MDTBFloat4 paper_color = make_float4(0.88f, 0.88f, 0.82f, 1.0f);

    if (spans_on_x_axis) {
        push_prop(make_float3(x, 0.76f, z), make_float3(0.70f, 0.76f, 0.36f), body_color, 1);
        push_prop(make_float3(x, 1.56f, z + 0.08f), make_float3(0.84f, 0.08f, 0.48f), canopy_color, 0);
        push_prop(make_float3(x, 0.82f, z + 0.32f), make_float3(0.54f, 0.42f, 0.04f), paper_color, 0);
    } else {
        push_prop(make_float3(x, 0.76f, z), make_float3(0.36f, 0.76f, 0.70f), body_color, 1);
        push_prop(make_float3(x + 0.08f, 1.56f, z), make_float3(0.48f, 0.08f, 0.84f), canopy_color, 0);
        push_prop(make_float3(x + 0.32f, 0.82f, z), make_float3(0.04f, 0.42f, 0.54f), paper_color, 0);
    }
}

static void push_store_awning_x(float x, float z, float half_x, float facing_sign) {
    const float canopy_z = z + (facing_sign * 0.88f);

    push_prop(
        make_float3(x - (half_x * 0.82f), 1.18f, z + (facing_sign * 0.76f)),
        make_float3(0.07f, 1.18f, 0.07f),
        make_float4(0.49f, 0.45f, 0.37f, 1.0f),
        1
    );
    push_prop(
        make_float3(x + (half_x * 0.82f), 1.18f, z + (facing_sign * 0.76f)),
        make_float3(0.07f, 1.18f, 0.07f),
        make_float4(0.49f, 0.45f, 0.37f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 2.38f, canopy_z),
        make_float3(half_x, 0.12f, 0.96f),
        make_float4(0.90f, 0.55f, 0.21f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, 2.05f, z + (facing_sign * 0.10f)),
        make_float3(half_x, 0.08f, 0.18f),
        make_float4(0.67f, 0.31f, 0.18f, 1.0f),
        0
    );
}

static void push_low_fence_run_x(float z, float x_start, float x_end) {
    const MDTBFloat4 fence_color = make_float4(0.44f, 0.46f, 0.44f, 1.0f);
    float current = x_start;

    while (current <= x_end + 0.01f) {
        push_prop(make_float3(current, 0.74f, z), make_float3(0.06f, 0.74f, 0.06f), fence_color, 1);
        current += 2.8f;
    }

    current = x_start + 1.4f;
    while (current < x_end) {
        const float half_x = fminf(1.12f, (x_end - current) * 0.5f);
        if (half_x <= 0.08f) {
            break;
        }

        push_prop(make_float3(current, 0.54f, z), make_float3(half_x, 0.04f, 0.04f), fence_color, 1);
        push_prop(make_float3(current, 0.98f, z), make_float3(half_x, 0.04f, 0.04f), fence_color, 1);
        current += 2.8f;
    }
}

static void push_low_fence_run_z(float x, float z_start, float z_end) {
    const MDTBFloat4 fence_color = make_float4(0.44f, 0.46f, 0.44f, 1.0f);
    float current = z_start;

    while (current <= z_end + 0.01f) {
        push_prop(make_float3(x, 0.74f, current), make_float3(0.06f, 0.74f, 0.06f), fence_color, 1);
        current += 2.8f;
    }

    current = z_start + 1.4f;
    while (current < z_end) {
        const float half_z = fminf(1.12f, (z_end - current) * 0.5f);
        if (half_z <= 0.08f) {
            break;
        }

        push_prop(make_float3(x, 0.54f, current), make_float3(0.04f, 0.04f, half_z), fence_color, 1);
        push_prop(make_float3(x, 0.98f, current), make_float3(0.04f, 0.04f, half_z), fence_color, 1);
        current += 2.8f;
    }
}

static void push_lane_arrow_z(float x_origin, float z_center, float direction_sign) {
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.86f, 0.68f, 1.0f);

    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.01f, z_center - (direction_sign * 0.85f)),
        make_float3(0.20f, 0.01f, 1.05f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - 0.42f, kRoadHeight + 0.01f, z_center + (direction_sign * 0.34f)),
        make_float3(0.18f, 0.01f, 0.46f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 0.42f, kRoadHeight + 0.01f, z_center + (direction_sign * 0.34f)),
        make_float3(0.18f, 0.01f, 0.46f),
        stripe_color
    ));
}

static void push_lane_arrow_x(float x_center, float z_origin, float direction_sign) {
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.86f, 0.68f, 1.0f);

    push_scene_box(make_box(
        make_float3(x_center - (direction_sign * 0.85f), kRoadHeight + 0.01f, z_origin),
        make_float3(1.05f, 0.01f, 0.20f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_center + (direction_sign * 0.34f), kRoadHeight + 0.01f, z_origin - 0.42f),
        make_float3(0.46f, 0.01f, 0.18f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_center + (direction_sign * 0.34f), kRoadHeight + 0.01f, z_origin + 0.42f),
        make_float3(0.46f, 0.01f, 0.18f),
        stripe_color
    ));
}

static void push_billboard(float x, float z, float width, float height, int align_on_x_axis, MDTBFloat4 face_color) {
    const float pole_offset = width * 0.32f;

    if (align_on_x_axis) {
        push_prop(make_float3(x, 2.1f, z - pole_offset), make_float3(0.16f, 2.1f, 0.16f), make_float4(0.36f, 0.35f, 0.33f, 1.0f), 1);
        push_prop(make_float3(x, 2.1f, z + pole_offset), make_float3(0.16f, 2.1f, 0.16f), make_float4(0.36f, 0.35f, 0.33f, 1.0f), 1);
        push_prop(make_float3(x, 4.55f, z), make_float3(0.24f, 0.18f, width * 0.62f), make_float4(0.33f, 0.33f, 0.35f, 1.0f), 0);
        push_prop(make_float3(x, 4.15f, z), make_float3(0.18f, height, width * 0.5f), face_color, 0);
    } else {
        push_prop(make_float3(x - pole_offset, 2.1f, z), make_float3(0.16f, 2.1f, 0.16f), make_float4(0.36f, 0.35f, 0.33f, 1.0f), 1);
        push_prop(make_float3(x + pole_offset, 2.1f, z), make_float3(0.16f, 2.1f, 0.16f), make_float4(0.36f, 0.35f, 0.33f, 1.0f), 1);
        push_prop(make_float3(x, 4.55f, z), make_float3(width * 0.62f, 0.18f, 0.24f), make_float4(0.33f, 0.33f, 0.35f, 1.0f), 0);
        push_prop(make_float3(x, 4.15f, z), make_float3(width * 0.5f, height, 0.18f), face_color, 0);
    }
}

static void push_half_court(float x, float z) {
    const MDTBFloat4 fence_color = make_float4(0.41f, 0.47f, 0.43f, 1.0f);
    const MDTBFloat4 stripe_color = make_float4(0.93f, 0.88f, 0.74f, 1.0f);

    push_scene_box(make_box(
        make_float3(x, kSidewalkHeight + 0.02f, z),
        make_float3(6.1f, 0.02f, 9.4f),
        make_float4(0.30f, 0.35f, 0.39f, 1.0f)
    ));

    push_scene_box(make_box(
        make_float3(x, kSidewalkHeight + 0.03f, z + 2.8f),
        make_float3(2.1f, 0.01f, 0.08f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x, kSidewalkHeight + 0.03f, z - 0.2f),
        make_float3(0.08f, 0.01f, 3.0f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x, kSidewalkHeight + 0.03f, z + 5.6f),
        make_float3(0.95f, 0.01f, 0.08f),
        stripe_color
    ));

    push_prop(make_float3(x, 2.05f, z + 7.2f), make_float3(0.10f, 2.05f, 0.10f), fence_color, 1);
    push_prop(make_float3(x, 3.35f, z + 6.45f), make_float3(0.82f, 0.52f, 0.08f), make_float4(0.78f, 0.84f, 0.88f, 1.0f), 0);
    push_prop(make_float3(x, 2.85f, z + 5.82f), make_float3(0.35f, 0.04f, 0.35f), make_float4(0.91f, 0.47f, 0.23f, 1.0f), 0);

    for (int side = -1; side <= 1; side += 2) {
        push_prop(make_float3(x + ((float)side * 6.0f), 1.45f, z), make_float3(0.10f, 1.45f, 9.1f), fence_color, 1);
        push_prop(make_float3(x, 1.45f, z + ((float)side * 9.1f)), make_float3(6.0f, 1.45f, 0.10f), fence_color, 1);
    }
}

static void push_carport_landmark(float x, float z) {
    const MDTBFloat4 beam_color = make_float4(0.55f, 0.52f, 0.44f, 1.0f);

    push_prop(make_float3(x - 2.9f, 1.35f, z - 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x + 2.9f, 1.35f, z - 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x - 2.9f, 1.35f, z + 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x + 2.9f, 1.35f, z + 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x, 2.95f, z), make_float3(3.25f, 0.18f, 2.15f), make_float4(0.72f, 0.69f, 0.58f, 1.0f), 0);
    push_parked_car(x, z, 1.28f, 2.45f, make_float4(0.42f, 0.37f, 0.58f, 1.0f));
}

static void push_bus_shelter(float x, float z) {
    const MDTBFloat4 frame_color = make_float4(0.31f, 0.33f, 0.36f, 1.0f);
    const MDTBFloat4 roof_color = make_float4(0.67f, 0.76f, 0.80f, 1.0f);

    push_prop(make_float3(x - 0.92f, 1.22f, z), make_float3(0.08f, 1.22f, 0.08f), frame_color, 1);
    push_prop(make_float3(x + 0.92f, 1.22f, z), make_float3(0.08f, 1.22f, 0.08f), frame_color, 1);
    push_prop(make_float3(x, 2.46f, z), make_float3(1.14f, 0.10f, 1.34f), roof_color, 0);
    push_prop(make_float3(x - 0.78f, 1.18f, z), make_float3(0.05f, 1.02f, 1.08f), make_float4(0.69f, 0.79f, 0.84f, 1.0f), 0);
    push_bench(x, z - 0.52f);
}

static void push_apartment_entry(float x, float z) {
    push_prop(make_float3(x, kSidewalkHeight + 1.55f, z), make_float3(0.28f, 1.55f, 2.85f), make_float4(0.62f, 0.58f, 0.46f, 1.0f), 1);
    push_prop(make_float3(x - 1.05f, kSidewalkHeight + 1.24f, z), make_float3(0.10f, 1.24f, 2.42f), make_float4(0.43f, 0.46f, 0.48f, 1.0f), 1);
    push_prop(make_float3(x + 1.05f, kSidewalkHeight + 1.24f, z), make_float3(0.10f, 1.24f, 2.42f), make_float4(0.43f, 0.46f, 0.48f, 1.0f), 1);
    push_prop(make_float3(x, kSidewalkHeight + 3.05f, z), make_float3(1.48f, 0.14f, 2.85f), make_float4(0.86f, 0.76f, 0.34f, 1.0f), 0);
}

static void push_corner_store_landmark(float x, float z) {
    push_building(
        make_float3(x, kSidewalkHeight + 2.55f, z),
        make_float3(4.2f, 2.55f, 3.4f),
        make_float4(0.59f, 0.47f, 0.34f, 1.0f)
    );

    push_prop(
        make_float3(x, kSidewalkHeight + 1.55f, z + 3.72f),
        make_float3(4.7f, 0.18f, 0.46f),
        make_float4(0.88f, 0.39f, 0.17f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, kSidewalkHeight + 3.35f, z + 1.95f),
        make_float3(2.25f, 0.34f, 0.22f),
        make_float4(0.92f, 0.85f, 0.54f, 1.0f),
        0
    );

    push_prop(
        make_float3(x - 2.9f, kSidewalkHeight + 1.0f, z + 3.15f),
        make_float3(0.22f, 1.0f, 0.22f),
        make_float4(0.68f, 0.31f, 0.17f, 1.0f),
        1
    );
}

static void push_crosswalk_x(float x_center, float z_origin) {
    for (int stripe_index = -3; stripe_index <= 3; ++stripe_index) {
        push_scene_box(make_box(
            make_float3(x_center, kRoadHeight + 0.01f, z_origin + ((float)stripe_index * 1.45f)),
            make_float3(0.85f, 0.01f, 0.42f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
    }
}

static void push_crosswalk_z(float x_origin, float z_center) {
    for (int stripe_index = -3; stripe_index <= 3; ++stripe_index) {
        push_scene_box(make_box(
            make_float3(x_origin + ((float)stripe_index * 1.45f), kRoadHeight + 0.01f, z_center),
            make_float3(0.42f, 0.01f, 0.85f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
    }
}

static void build_block_lot_surfaces(const MDTBBlockDescriptor *block) {
    const MDTBFloat4 lot_color = make_float4(0.54f, 0.49f, 0.42f, 1.0f);

    push_scene_box(make_box(
        make_float3(block->origin.x - 35.0f, kSidewalkHeight * 0.5f, block->origin.z),
        make_float3(23.0f, kSidewalkHeight * 0.5f, 29.0f),
        lot_color
    ));

    push_scene_box(make_box(
        make_float3(block->origin.x + 35.0f, kSidewalkHeight * 0.5f, block->origin.z),
        make_float3(23.0f, kSidewalkHeight * 0.5f, 29.0f),
        lot_color
    ));
}

static void build_world_surfaces(void) {
    const MDTBFloat4 soil_color = make_float4(0.45f, 0.48f, 0.42f, 1.0f);
    const MDTBFloat4 road_color = make_float4(0.17f, 0.17f, 0.19f, 1.0f);
    const MDTBFloat4 curb_color = make_float4(0.74f, 0.74f, 0.73f, 1.0f);
    const MDTBFloat4 sidewalk_color = make_float4(0.62f, 0.62f, 0.59f, 1.0f);

    push_scene_box(make_box(
        make_float3(0.0f, 0.0f, 0.0f),
        make_float3(kPlayableHalfWidth + 4.0f, 0.03f, kPlayableHalfLength + 4.0f),
        soil_color
    ));

    for (size_t index = 0u; index < scene_layout_count(); ++index) {
        const float x_origin = kBlockLayout[index].origin.x;

        if (layout_has_prior_x(index)) {
            continue;
        }

        push_scene_box(make_box(
            make_float3(x_origin, kRoadHeight * 0.5f, 0.0f),
            make_float3(kRoadHalfWidth, kRoadHeight * 0.5f, kPlayableHalfLength + 4.0f),
            road_color
        ));

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(x_origin + ((float)side * 6.08f), 0.09f, 0.0f),
                make_float3(0.20f, 0.09f, kPlayableHalfLength + 4.0f),
                curb_color
            ));

            push_scene_box(make_box(
                make_float3(x_origin + ((float)side * 9.18f), kSidewalkHeight * 0.5f, 0.0f),
                make_float3(2.82f, kSidewalkHeight * 0.5f, kPlayableHalfLength + 4.0f),
                sidewalk_color
            ));
        }
    }

    for (size_t index = 0u; index < scene_layout_count(); ++index) {
        const float z_origin = kBlockLayout[index].origin.z;

        if (layout_has_prior_z(index)) {
            continue;
        }

        push_scene_box(make_box(
            make_float3(0.0f, kRoadHeight * 0.5f, z_origin),
            make_float3(kPlayableHalfWidth + 4.0f, kRoadHeight * 0.5f, kRoadHalfWidth),
            road_color
        ));

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(0.0f, 0.09f, z_origin + ((float)side * 6.08f)),
                make_float3(kPlayableHalfWidth + 4.0f, 0.09f, 0.20f),
                curb_color
            ));

            push_scene_box(make_box(
                make_float3(0.0f, kSidewalkHeight * 0.5f, z_origin + ((float)side * 9.18f)),
                make_float3(kPlayableHalfWidth + 4.0f, kSidewalkHeight * 0.5f, 2.82f),
                sidewalk_color
            ));
        }
    }
}

static void build_road_markings(void) {
    const MDTBFloat4 stripe_color = make_float4(0.89f, 0.82f, 0.20f, 1.0f);

    for (size_t layout_index = 0u; layout_index < scene_layout_count(); ++layout_index) {
        const float x_origin = kBlockLayout[layout_index].origin.x;

        if (layout_has_prior_x(layout_index)) {
            continue;
        }

        for (int segment = -6; segment <= 6; ++segment) {
            const float center = (float)segment * 20.0f;
            int inside_intersection = 0;

            for (size_t block_index = 0u; block_index < scene_layout_count(); ++block_index) {
                if (fabsf(x_origin - kBlockLayout[block_index].origin.x) > 0.01f) {
                    continue;
                }

                if (fabsf(center - kBlockLayout[block_index].origin.z) <= kIntersectionClear) {
                    inside_intersection = 1;
                    break;
                }
            }

            if (!inside_intersection) {
                for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                    push_scene_box(make_box(
                        make_float3(x_origin, kRoadHeight + 0.01f, center + ((float)stripe_index * 4.4f)),
                        make_float3(0.13f, 0.01f, 1.2f),
                        stripe_color
                    ));
                }
            }
        }
    }

    for (size_t layout_index = 0u; layout_index < scene_layout_count(); ++layout_index) {
        const float z_origin = kBlockLayout[layout_index].origin.z;

        if (layout_has_prior_z(layout_index)) {
            continue;
        }

        for (int segment = -6; segment <= 6; ++segment) {
            const float center_x = (float)segment * 24.0f;
            int inside_intersection = 0;

            for (size_t block_index = 0u; block_index < scene_layout_count(); ++block_index) {
                if (fabsf(z_origin - kBlockLayout[block_index].origin.z) > 0.01f) {
                    continue;
                }

                if (fabsf(center_x - kBlockLayout[block_index].origin.x) <= kIntersectionClear) {
                    inside_intersection = 1;
                    break;
                }
            }

            if (inside_intersection) {
                continue;
            }

            for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                push_scene_box(make_box(
                    make_float3(center_x + ((float)stripe_index * 5.0f), kRoadHeight + 0.01f, z_origin),
                    make_float3(1.4f, 0.01f, 0.13f),
                    stripe_color
                ));
            }
        }
    }

    for (size_t block_index = 0u; block_index < scene_layout_count(); ++block_index) {
        const float x_origin = kBlockLayout[block_index].origin.x;
        const float z_origin = kBlockLayout[block_index].origin.z;

        push_crosswalk_x(x_origin - kCrosswalkOffset, z_origin);
        push_crosswalk_x(x_origin + kCrosswalkOffset, z_origin);
        push_crosswalk_z(x_origin, z_origin - kCrosswalkOffset);
        push_crosswalk_z(x_origin, z_origin + kCrosswalkOffset);

        push_scene_box(make_box(
            make_float3(x_origin, kRoadHeight + 0.01f, z_origin - 6.65f),
            make_float3(4.65f, 0.01f, 0.12f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin, kRoadHeight + 0.01f, z_origin + 6.65f),
            make_float3(4.65f, 0.01f, 0.12f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin - 6.65f, kRoadHeight + 0.01f, z_origin),
            make_float3(0.12f, 0.01f, 4.65f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin + 6.65f, kRoadHeight + 0.01f, z_origin),
            make_float3(0.12f, 0.01f, 4.65f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));

        push_lane_arrow_z(x_origin, z_origin - 18.5f, -1.0f);
        push_lane_arrow_z(x_origin, z_origin + 18.5f, 1.0f);
        push_lane_arrow_x(x_origin - 18.5f, z_origin, -1.0f);
        push_lane_arrow_x(x_origin + 18.5f, z_origin, 1.0f);
    }
}

static void build_side_buildings(const MDTBBlockDescriptor *block, int x_sign) {
    static const MDTBFloat4 hub_palette[4] = {
        {0.58f, 0.44f, 0.35f, 1.0f},
        {0.39f, 0.50f, 0.62f, 1.0f},
        {0.62f, 0.54f, 0.34f, 1.0f},
        {0.51f, 0.40f, 0.47f, 1.0f},
    };
    static const MDTBFloat4 residential_palette[4] = {
        {0.53f, 0.45f, 0.39f, 1.0f},
        {0.46f, 0.55f, 0.44f, 1.0f},
        {0.56f, 0.53f, 0.40f, 1.0f},
        {0.49f, 0.46f, 0.56f, 1.0f},
    };
    static const MDTBFloat4 mixed_use_palette[4] = {
        {0.64f, 0.50f, 0.31f, 1.0f},
        {0.40f, 0.47f, 0.62f, 1.0f},
        {0.52f, 0.34f, 0.42f, 1.0f},
        {0.45f, 0.58f, 0.53f, 1.0f},
    };
    const MDTBFloat4 *palette = residential_palette;

    if (block->kind == MDTBBlockKindHub) {
        palette = hub_palette;
    } else if (block->kind == MDTBBlockKindMixedUse) {
        palette = mixed_use_palette;
    }

    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            const int palette_index = (int)((block->variant + (uint32_t)row + (uint32_t)column) % 4u);
            const float center_x = block->origin.x + ((float)x_sign * (18.0f + ((float)column * 15.0f)));
            const float center_z = block->origin.z + (-18.0f + ((float)row * 15.0f));
            const float half_x = 2.6f + ((column == 0) ? 0.5f : 0.0f);
            const float half_y = block->kind == MDTBBlockKindHub
                ? (3.0f + ((row == 0) ? 0.9f : 0.25f) + ((column == 1) ? 0.35f : 0.0f))
                : (block->kind == MDTBBlockKindMixedUse
                    ? (2.55f + ((row == 0) ? 0.45f : 0.18f) + ((column == 1) ? 0.22f : 0.0f))
                    : (2.1f + ((row == 1) ? 0.35f : 0.0f)));
            const float half_z = block->kind == MDTBBlockKindHub
                ? (3.2f + ((column == 0) ? 0.8f : 0.3f))
                : (block->kind == MDTBBlockKindMixedUse
                    ? (3.1f + ((column == 0) ? 0.7f : 0.35f))
                    : (3.6f + ((column == 0) ? 0.6f : 0.2f)));

            push_building(
                make_float3(center_x, kSidewalkHeight + half_y, center_z),
                make_float3(half_x, half_y, half_z),
                palette[palette_index]
            );
        }
    }

    push_tree(block->origin.x + ((float)x_sign * 13.4f), block->origin.z - 24.0f, 1.0f);
    push_tree(block->origin.x + ((float)x_sign * 13.0f), block->origin.z + 22.8f, 1.0f);
    push_planter(block->origin.x + ((float)x_sign * 14.8f), block->origin.z - 8.6f, 0.48f);
    push_planter(block->origin.x + ((float)x_sign * 9.4f), block->origin.z + 14.2f, 0.58f);
}

static void build_intersection_props(const MDTBBlockDescriptor *block) {
    push_signal_pole(block->origin.x - 8.9f, block->origin.z - 8.9f);
    push_signal_pole(block->origin.x + 8.9f, block->origin.z - 8.9f);
    push_signal_pole(block->origin.x - 8.9f, block->origin.z + 8.9f);
    push_signal_pole(block->origin.x + 8.9f, block->origin.z + 8.9f);

    push_planter(block->origin.x - 10.2f, block->origin.z - 10.2f, 0.56f);
    push_planter(block->origin.x + 10.2f, block->origin.z - 10.2f, 0.56f);
    push_planter(block->origin.x - 10.2f, block->origin.z + 10.2f, 0.56f);
    push_planter(block->origin.x + 10.2f, block->origin.z + 10.2f, 0.56f);

    push_prop(
        offset_point(block->origin, 11.4f, 1.15f, -16.0f),
        make_float3(0.12f, 1.15f, 1.35f),
        make_float4(0.28f, 0.29f, 0.32f, 1.0f),
        1
    );
    push_prop(
        offset_point(block->origin, 10.8f, 2.15f, -16.0f),
        make_float3(0.82f, 0.16f, 1.55f),
        make_float4(0.56f, 0.61f, 0.68f, 1.0f),
        0
    );
    push_prop(
        offset_point(block->origin, 10.8f, 1.05f, -16.0f),
        make_float3(0.65f, 0.55f, 1.15f),
        make_float4(0.70f, 0.72f, 0.75f, 1.0f),
        1
    );

    push_prop(
        offset_point(block->origin, -16.0f, 0.55f, 11.1f),
        make_float3(1.55f, 0.55f, 0.34f),
        make_float4(0.79f, 0.25f, 0.19f, 1.0f),
        1
    );
    push_prop(
        offset_point(block->origin, 16.0f, 0.55f, -11.1f),
        make_float3(1.55f, 0.55f, 0.34f),
        make_float4(0.23f, 0.49f, 0.67f, 1.0f),
        1
    );

    push_bollard(block->origin.x - 11.9f, block->origin.z - 7.4f, 0.92f);
    push_bollard(block->origin.x - 11.9f, block->origin.z + 7.4f, 0.92f);
    push_bollard(block->origin.x + 11.9f, block->origin.z - 7.4f, 0.92f);
    push_bollard(block->origin.x + 11.9f, block->origin.z + 7.4f, 0.92f);
    push_bollard(block->origin.x - 7.4f, block->origin.z - 11.9f, 0.92f);
    push_bollard(block->origin.x + 7.4f, block->origin.z - 11.9f, 0.92f);
    push_bollard(block->origin.x - 7.4f, block->origin.z + 11.9f, 0.92f);
    push_bollard(block->origin.x + 7.4f, block->origin.z + 11.9f, 0.92f);
}

static void build_intersection_dynamic_props(const MDTBBlockDescriptor *block, uint32_t block_index) {
    for (int x_sign = -1; x_sign <= 1; x_sign += 2) {
        for (int z_sign = -1; z_sign <= 1; z_sign += 2) {
            push_dynamic_prop(
                offset_point(block->origin, 8.9f * (float)x_sign, 2.42f, 9.35f * (float)z_sign),
                make_float3(0.14f, 0.14f, 0.14f),
                make_float4(0.24f, 0.82f, 0.34f, 1.0f),
                ((float)(x_sign + 2 + (z_sign > 0 ? 1 : 0)) * 0.45f),
                MDTBDynamicPropSignalLamp,
                block_index
            );
        }
    }
}

static void build_hub_dynamic_props(const MDTBBlockDescriptor *block, uint32_t block_index) {
    build_intersection_dynamic_props(block, block_index);

    push_dynamic_prop(
        offset_point(block->origin, -37.15f, 2.06f, -13.35f),
        make_float3(0.84f, 0.42f, 0.08f),
        make_float4(0.90f, 0.74f, 0.22f, 1.0f),
        0.0f,
        MDTBDynamicPropSwingSign,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -44.9f, 2.62f, -11.8f),
        make_float3(0.28f, 0.03f, 0.12f),
        make_float4(0.86f, 0.36f, 0.19f, 1.0f),
        0.2f,
        MDTBDynamicPropPennant,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -42.8f, 2.60f, -11.85f),
        make_float3(0.28f, 0.03f, 0.12f),
        make_float4(0.90f, 0.80f, 0.28f, 1.0f),
        0.8f,
        MDTBDynamicPropPennant,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -40.7f, 2.63f, -11.8f),
        make_float3(0.28f, 0.03f, 0.12f),
        make_float4(0.27f, 0.54f, 0.71f, 1.0f),
        1.4f,
        MDTBDynamicPropPennant,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 10.1f, 2.18f, 34.0f),
        make_float3(0.58f, 0.18f, 0.06f),
        make_float4(0.88f, 0.91f, 0.95f, 1.0f),
        1.1f,
        MDTBDynamicPropWindowGlow,
        block_index
    );
}

static void build_residential_dynamic_props(const MDTBBlockDescriptor *block, uint32_t block_index) {
    build_intersection_dynamic_props(block, block_index);

    push_dynamic_prop(
        offset_point(block->origin, 11.2f, 2.08f, -16.2f),
        make_float3(0.30f, 0.32f, 0.05f),
        make_float4(0.82f, 0.82f, 0.92f, 1.0f),
        0.4f,
        MDTBDynamicPropTransitGlow,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -41.6f, 2.46f, 15.0f),
        make_float3(0.88f, 0.10f, 0.10f),
        make_float4(0.92f, 0.80f, 0.34f, 1.0f),
        0.9f,
        MDTBDynamicPropWindowGlow,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 38.4f, 2.16f, 33.8f),
        make_float3(0.92f, 0.10f, 0.08f),
        make_float4(0.76f, 0.76f, 0.86f, 1.0f),
        1.7f,
        MDTBDynamicPropWindowGlow,
        block_index
    );
}

static void build_hub_frontage(const MDTBBlockDescriptor *block) {
    const float z_origin = block->origin.z;

    push_store_awning_x(-42.0f, z_origin - 13.45f, 4.65f, 1.0f);
    push_newsstand(-45.5f, z_origin - 11.25f, 1);
    push_trash_bin(-38.6f, z_origin - 11.0f);
    push_prop(make_float3(-40.9f, 0.38f, z_origin - 11.1f), make_float3(0.42f, 0.38f, 0.42f), make_float4(0.55f, 0.43f, 0.30f, 1.0f), 1);
    push_prop(make_float3(-39.8f, 0.62f, z_origin - 11.0f), make_float3(0.34f, 0.62f, 0.28f), make_float4(0.19f, 0.35f, 0.56f, 1.0f), 1);

    for (int bollard = 0; bollard < 4; ++bollard) {
        push_bollard(-46.8f + ((float)bollard * 2.1f), z_origin - 10.1f, 0.86f);
    }

    push_store_awning_x(-18.0f, z_origin - 13.65f, 3.35f, 1.0f);
    push_store_awning_x(18.0f, z_origin - 13.25f, 3.75f, 1.0f);
    push_store_awning_x(33.0f, z_origin - 13.65f, 2.95f, 1.0f);
    push_newsstand(12.6f, z_origin - 10.75f, 1);
    push_newsstand(28.9f, z_origin - 10.65f, 1);
    push_trash_bin(-22.1f, z_origin - 10.7f);
    push_trash_bin(36.5f, z_origin - 10.8f);
    push_bench(23.9f, z_origin - 10.3f);

    push_store_awning_x(-18.0f, z_origin + 13.65f, 3.35f, -1.0f);
    push_store_awning_x(18.0f, z_origin + 13.25f, 3.75f, -1.0f);
    push_store_awning_x(33.0f, z_origin + 13.65f, 2.95f, -1.0f);
    push_newsstand(-13.1f, z_origin + 10.7f, 1);
    push_newsstand(23.1f, z_origin + 10.7f, 1);
    push_trash_bin(-22.3f, z_origin + 10.85f);
    push_trash_bin(36.4f, z_origin + 10.9f);
    push_bench(27.4f, z_origin + 10.4f);

    push_bench(-10.7f, z_origin - 31.5f);
    push_trash_bin(-13.0f, z_origin - 32.4f);
    push_prop(make_float3(-11.2f, 2.05f, z_origin - 34.0f), make_float3(0.12f, 2.05f, 0.12f), make_float4(0.36f, 0.37f, 0.39f, 1.0f), 1);
    push_prop(make_float3(-11.2f, 3.42f, z_origin - 34.0f), make_float3(1.08f, 0.12f, 0.12f), make_float4(0.83f, 0.75f, 0.49f, 1.0f), 0);

    push_bench(9.8f, z_origin + 31.7f);
    push_trash_bin(12.2f, z_origin + 32.4f);
    push_prop(make_float3(10.8f, 1.28f, z_origin + 34.0f), make_float3(0.12f, 1.28f, 1.05f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(10.1f, 2.34f, z_origin + 34.0f), make_float3(0.96f, 0.10f, 1.28f), make_float4(0.67f, 0.78f, 0.82f, 1.0f), 0);
}

static void build_mixed_use_frontage(const MDTBBlockDescriptor *block) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;

    push_store_awning_x(x_origin - 19.5f, z_origin - 13.4f, 3.8f, 1.0f);
    push_store_awning_x(x_origin + 11.5f, z_origin - 13.2f, 4.8f, 1.0f);
    push_store_awning_x(x_origin + 31.0f, z_origin + 13.4f, 3.4f, -1.0f);
    push_bus_shelter(x_origin - 21.8f, z_origin - 16.0f);
    push_newsstand(x_origin - 6.2f, z_origin - 10.7f, 1);
    push_newsstand(x_origin + 25.6f, z_origin + 10.8f, 1);
    push_trash_bin(x_origin - 25.2f, z_origin + 10.8f);
    push_trash_bin(x_origin + 8.6f, z_origin - 10.8f);
    push_bench(x_origin + 19.6f, z_origin - 10.3f);
    push_bench(x_origin - 11.8f, z_origin + 10.3f);
    push_planter(x_origin - 32.0f, z_origin - 12.2f, 0.60f);
    push_planter(x_origin + 36.2f, z_origin + 12.0f, 0.58f);
    push_prop(make_float3(x_origin + 6.2f, kSidewalkHeight + 2.15f, z_origin - 13.0f), make_float3(2.2f, 0.16f, 0.20f), make_float4(0.92f, 0.63f, 0.22f, 1.0f), 0);
    push_prop(make_float3(x_origin + 33.8f, kSidewalkHeight + 2.2f, z_origin + 13.2f), make_float3(1.9f, 0.16f, 0.20f), make_float4(0.28f, 0.66f, 0.84f, 1.0f), 0);
    push_prop(make_float3(x_origin - 33.6f, 1.28f, z_origin + 13.0f), make_float3(0.12f, 1.28f, 1.08f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(x_origin - 34.3f, 2.34f, z_origin + 13.0f), make_float3(1.06f, 0.12f, 1.24f), make_float4(0.74f, 0.79f, 0.82f, 1.0f), 0);

    push_low_fence_run_x(z_origin - 46.2f, x_origin + 25.0f, x_origin + 48.0f);
    push_low_fence_run_z(x_origin + 48.0f, z_origin - 46.2f, z_origin - 27.8f);
    push_low_fence_run_x(z_origin + 45.8f, x_origin - 49.0f, x_origin - 28.0f);
    push_low_fence_run_z(x_origin - 49.0f, z_origin + 28.4f, z_origin + 45.8f);
}

static void build_service_court_annex(const MDTBBlockDescriptor *block, uint32_t block_index) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;
    const MDTBFloat4 service_lane_color = make_float4(0.24f, 0.26f, 0.28f, 1.0f);
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.85f, 0.63f, 1.0f);

    push_scene_box(make_box(
        make_float3(x_origin - 29.4f, kSidewalkHeight + 0.02f, z_origin - 28.8f),
        make_float3(10.4f, 0.02f, 5.2f),
        service_lane_color
    ));

    for (int stripe_index = 0; stripe_index < 3; ++stripe_index) {
        push_scene_box(make_box(
            make_float3(x_origin - 35.6f + ((float)stripe_index * 4.2f), kSidewalkHeight + 0.03f, z_origin - 28.8f),
            make_float3(1.25f, 0.01f, 0.08f),
            stripe_color
        ));
    }

    push_building(
        make_float3(x_origin - 40.2f, kSidewalkHeight + 2.35f, z_origin - 31.0f),
        make_float3(4.8f, 2.35f, 6.4f),
        make_float4(0.48f, 0.41f, 0.34f, 1.0f)
    );
    push_prop(
        make_float3(x_origin - 33.8f, kSidewalkHeight + 2.18f, z_origin - 28.8f),
        make_float3(2.1f, 0.14f, 3.1f),
        make_float4(0.74f, 0.46f, 0.22f, 1.0f),
        0
    );
    push_prop(
        make_float3(x_origin - 34.1f, kSidewalkHeight + 1.1f, z_origin - 23.6f),
        make_float3(0.14f, 1.1f, 1.4f),
        make_float4(0.31f, 0.33f, 0.35f, 1.0f),
        1
    );
    push_prop(
        make_float3(x_origin - 26.6f, 0.72f, z_origin - 31.6f),
        make_float3(0.68f, 0.72f, 1.2f),
        make_float4(0.26f, 0.39f, 0.46f, 1.0f),
        1
    );
    push_prop(
        make_float3(x_origin - 26.6f, 1.52f, z_origin - 31.6f),
        make_float3(0.76f, 0.08f, 1.28f),
        make_float4(0.44f, 0.56f, 0.62f, 1.0f),
        0
    );
    push_trash_bin(x_origin - 28.8f, z_origin - 24.7f);
    push_bench(x_origin - 22.4f, z_origin - 24.1f);

    for (int bollard = 0; bollard < 4; ++bollard) {
        push_bollard(x_origin - 35.8f + ((float)bollard * 2.6f), z_origin - 22.8f, 0.92f);
    }

    push_interest_point(make_float3(x_origin - 30.2f, kSidewalkHeight, z_origin - 27.9f), 5.5f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(x_origin - 23.8f, kSidewalkHeight, z_origin - 28.2f), 5.8f, MDTBInterestPointVehicleSpawn, block_index);
    push_interest_point(make_float3(x_origin - 38.4f, kSidewalkHeight, z_origin - 30.8f), 7.5f, MDTBInterestPointLandmark, block_index);

    push_dynamic_prop(
        offset_point(block->origin, -33.8f, 2.18f, -28.8f),
        make_float3(0.94f, 0.16f, 0.08f),
        make_float4(0.92f, 0.55f, 0.23f, 1.0f),
        0.5f,
        MDTBDynamicPropNeon,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -26.6f, 1.92f, -31.6f),
        make_float3(0.36f, 0.26f, 0.08f),
        make_float4(0.76f, 0.84f, 0.90f, 1.0f),
        1.0f,
        MDTBDynamicPropWindowGlow,
        block_index
    );
}

static void build_mixed_use_block(const MDTBBlockDescriptor *block, uint32_t block_index) {
    build_block_lot_surfaces(block);
    build_side_buildings(block, -1);
    build_side_buildings(block, 1);
    build_intersection_props(block);
    build_frontage_for_block(block);

    push_corner_store_landmark(block->origin.x + 39.0f, block->origin.z - 34.8f);
    push_billboard(block->origin.x + 46.0f, block->origin.z + 16.0f, 4.9f, 1.25f, 1, make_float4(0.22f, 0.54f, 0.72f, 1.0f));
    push_billboard(block->origin.x - 28.0f, block->origin.z + 34.0f, 5.3f, 1.35f, 0, make_float4(0.88f, 0.63f, 0.24f, 1.0f));
    push_parked_car(block->origin.x - 38.0f, block->origin.z + 34.4f, 1.15f, 2.15f, make_float4(0.27f, 0.43f, 0.61f, 1.0f));
    push_parked_car(block->origin.x + 35.4f, block->origin.z - 35.6f, 1.18f, 2.25f, make_float4(0.63f, 0.37f, 0.22f, 1.0f));

    if (block->frontage_template == MDTBFrontageTemplateServiceSpur) {
        build_service_court_annex(block, block_index);
    }

    push_interest_point(make_float3(block->origin.x, kSidewalkHeight, block->origin.z), 18.0f, MDTBInterestPointStreamingAnchor, block_index);
    push_interest_point(make_float3(block->origin.x - 10.0f, kSidewalkHeight, block->origin.z - 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(block->origin.x + 14.0f, kSidewalkHeight, block->origin.z + 10.0f), 5.5f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(block->origin.x + 31.0f, kSidewalkHeight, block->origin.z + 13.4f), 7.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(block->origin.x + 39.0f, kSidewalkHeight, block->origin.z - 34.8f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(block->origin.x - 12.0f, kSidewalkHeight, block->origin.z + 2.8f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    push_interest_point(make_float3(block->origin.x + 18.0f, kSidewalkHeight, block->origin.z - 2.6f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    build_hotspot_hooks(block, block_index);
    build_vehicle_handoff_hooks(block, block_index);

    push_dynamic_prop(
        offset_point(block->origin, -21.8f, 2.08f, -16.0f),
        make_float3(0.32f, 0.30f, 0.05f),
        make_float4(0.84f, 0.84f, 0.94f, 1.0f),
        0.6f,
        MDTBDynamicPropTransitGlow,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 6.2f, 2.15f, -13.0f),
        make_float3(1.08f, 0.18f, 0.08f),
        make_float4(0.92f, 0.63f, 0.22f, 1.0f),
        0.3f,
        MDTBDynamicPropNeon,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 33.8f, 2.20f, 13.2f),
        make_float3(0.98f, 0.18f, 0.08f),
        make_float4(0.28f, 0.66f, 0.84f, 1.0f),
        1.2f,
        MDTBDynamicPropNeon,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, -33.6f, 2.34f, 13.0f),
        make_float3(0.76f, 0.12f, 0.08f),
        make_float4(0.74f, 0.79f, 0.82f, 1.0f),
        1.8f,
        MDTBDynamicPropWindowGlow,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 39.9f, 2.04f, -30.9f),
        make_float3(0.88f, 0.34f, 0.08f),
        make_float4(0.91f, 0.72f, 0.24f, 1.0f),
        0.0f,
        MDTBDynamicPropSwingSign,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 27.0f, 2.58f, 11.8f),
        make_float3(0.26f, 0.03f, 0.12f),
        make_float4(0.86f, 0.38f, 0.21f, 1.0f),
        0.2f,
        MDTBDynamicPropPennant,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 29.2f, 2.62f, 11.8f),
        make_float3(0.26f, 0.03f, 0.12f),
        make_float4(0.92f, 0.82f, 0.26f, 1.0f),
        0.8f,
        MDTBDynamicPropPennant,
        block_index
    );
    push_dynamic_prop(
        offset_point(block->origin, 31.4f, 2.60f, 11.8f),
        make_float3(0.26f, 0.03f, 0.12f),
        make_float4(0.27f, 0.56f, 0.74f, 1.0f),
        1.4f,
        MDTBDynamicPropPennant,
        block_index
    );
}

static void build_hub_block(const MDTBBlockDescriptor *block, uint32_t block_index) {
    build_block_lot_surfaces(block);
    build_side_buildings(block, -1);
    build_side_buildings(block, 1);
    build_intersection_props(block);
    build_frontage_for_block(block);
    build_hub_dynamic_props(block, block_index);

    push_corner_store_landmark(-42.0f, block->origin.z - 17.2f);
    push_billboard(-23.5f, block->origin.z - 45.5f, 5.4f, 1.45f, 0, make_float4(0.88f, 0.63f, 0.24f, 1.0f));
    push_half_court(35.0f, block->origin.z + 35.0f);
    push_billboard(46.0f, block->origin.z + 12.5f, 4.8f, 1.35f, 1, make_float4(0.29f, 0.53f, 0.67f, 1.0f));
    push_carport_landmark(39.5f, block->origin.z - 37.5f);

    push_low_fence_run_x(block->origin.z - 47.6f, 28.0f, 50.2f);
    push_low_fence_run_z(28.0f, block->origin.z - 47.6f, block->origin.z - 28.6f);
    push_low_fence_run_x(block->origin.z + 46.3f, -50.0f, -29.6f);
    push_low_fence_run_z(-29.6f, block->origin.z + 28.8f, block->origin.z + 46.3f);

    push_interest_point(make_float3(0.0f, kSidewalkHeight, block->origin.z), 18.0f, MDTBInterestPointStreamingAnchor, block_index);
    push_interest_point(make_float3(-10.0f, kSidewalkHeight, block->origin.z - 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(10.0f, kSidewalkHeight, block->origin.z + 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(-14.4f, kSidewalkHeight, block->origin.z + 2.8f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    push_interest_point(make_float3(42.0f, kSidewalkHeight, block->origin.z - 17.2f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(35.0f, kSidewalkHeight, block->origin.z + 35.0f), 8.0f, MDTBInterestPointLandmark, block_index);
    build_hotspot_hooks(block, block_index);
    build_vehicle_handoff_hooks(block, block_index);
}

static void build_residential_frontage(const MDTBBlockDescriptor *block) {
    const float z_origin = block->origin.z;

    push_store_awning_x(-18.0f, z_origin - 13.6f, 3.1f, 1.0f);
    push_store_awning_x(19.0f, z_origin - 13.1f, 2.8f, 1.0f);
    push_store_awning_x(-18.0f, z_origin + 13.4f, 2.6f, -1.0f);
    push_newsstand(-12.6f, z_origin + 10.7f, 1);
    push_newsstand(22.4f, z_origin - 10.7f, 1);
    push_trash_bin(-22.0f, z_origin - 10.9f);
    push_trash_bin(34.8f, z_origin + 10.8f);
    push_bench(24.0f, z_origin - 10.4f);
    push_bench(-22.8f, z_origin + 10.3f);

    push_bus_shelter(11.2f, z_origin - 16.2f);
    push_apartment_entry(-41.6f, z_origin + 15.0f);
    push_prop(make_float3(-43.0f, kSidewalkHeight + 3.45f, z_origin + 15.0f), make_float3(1.85f, 0.14f, 3.0f), make_float4(0.86f, 0.78f, 0.42f, 1.0f), 0);
    push_prop(make_float3(39.0f, kSidewalkHeight + 1.2f, z_origin + 33.8f), make_float3(0.14f, 1.2f, 1.3f), make_float4(0.29f, 0.31f, 0.33f, 1.0f), 1);
    push_prop(make_float3(38.4f, kSidewalkHeight + 2.1f, z_origin + 33.8f), make_float3(0.92f, 0.10f, 1.48f), make_float4(0.72f, 0.80f, 0.84f, 1.0f), 0);

    push_low_fence_run_x(z_origin + 46.2f, -48.0f, -28.2f);
    push_low_fence_run_z(-48.0f, z_origin + 28.5f, z_origin + 46.2f);
    push_low_fence_run_x(z_origin + 46.0f, 29.0f, 48.5f);
    push_low_fence_run_z(48.5f, z_origin + 29.0f, z_origin + 46.0f);
}

static void build_service_spur_frontage(const MDTBBlockDescriptor *block) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;

    push_store_awning_x(x_origin - 23.0f, z_origin - 13.4f, 3.1f, 1.0f);
    push_store_awning_x(x_origin + 6.5f, z_origin - 13.0f, 4.3f, 1.0f);
    push_store_awning_x(x_origin + 29.8f, z_origin + 13.3f, 2.8f, -1.0f);
    push_bus_shelter(x_origin + 18.6f, z_origin + 16.2f);
    push_newsstand(x_origin - 8.8f, z_origin - 10.8f, 1);
    push_newsstand(x_origin + 22.8f, z_origin + 10.8f, 1);
    push_trash_bin(x_origin - 26.4f, z_origin + 10.7f);
    push_trash_bin(x_origin + 10.8f, z_origin - 10.9f);
    push_bench(x_origin - 14.6f, z_origin + 10.3f);
    push_bench(x_origin + 16.6f, z_origin - 10.4f);
    push_planter(x_origin - 31.4f, z_origin - 12.1f, 0.64f);
    push_planter(x_origin + 35.0f, z_origin + 12.0f, 0.54f);
    push_prop(make_float3(x_origin + 2.8f, kSidewalkHeight + 2.18f, z_origin - 13.0f), make_float3(2.5f, 0.18f, 0.20f), make_float4(0.92f, 0.71f, 0.24f, 1.0f), 0);
    push_prop(make_float3(x_origin + 29.8f, kSidewalkHeight + 2.18f, z_origin + 13.1f), make_float3(1.68f, 0.18f, 0.20f), make_float4(0.33f, 0.68f, 0.82f, 1.0f), 0);
    push_prop(make_float3(x_origin - 28.2f, 1.18f, z_origin + 12.8f), make_float3(0.14f, 1.18f, 1.18f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(x_origin - 28.9f, 2.18f, z_origin + 12.8f), make_float3(1.22f, 0.12f, 1.36f), make_float4(0.76f, 0.81f, 0.84f, 1.0f), 0);

    push_low_fence_run_x(z_origin - 46.2f, x_origin + 26.8f, x_origin + 48.4f);
    push_low_fence_run_z(x_origin + 48.4f, z_origin - 46.2f, z_origin - 26.8f);
    push_low_fence_run_x(z_origin + 45.8f, x_origin - 49.2f, x_origin - 30.0f);
    push_low_fence_run_z(x_origin - 49.2f, z_origin + 29.0f, z_origin + 45.8f);
}

static void build_frontage_for_block(const MDTBBlockDescriptor *block) {
    switch (block->frontage_template) {
        case MDTBFrontageTemplateResidentialCourt:
            build_residential_frontage(block);
            break;
        case MDTBFrontageTemplateTransitMarket:
            build_mixed_use_frontage(block);
            break;
        case MDTBFrontageTemplateServiceSpur:
            build_service_spur_frontage(block);
            break;
        case MDTBFrontageTemplateCivicRetail:
        default:
            build_hub_frontage(block);
            break;
    }
}

static void build_hotspot_hooks(const MDTBBlockDescriptor *block, uint32_t block_index) {
    switch (block->frontage_template) {
        case MDTBFrontageTemplateResidentialCourt:
            push_interest_point(offset_point(block->origin, 11.2f, kSidewalkHeight, -16.2f), 8.5f, MDTBInterestPointHotspot, block_index);
            push_interest_point(offset_point(block->origin, -41.6f, kSidewalkHeight, 15.0f), 8.5f, MDTBInterestPointHotspot, block_index);
            break;
        case MDTBFrontageTemplateTransitMarket:
            push_interest_point(offset_point(block->origin, -21.8f, kSidewalkHeight, -16.0f), 8.0f, MDTBInterestPointHotspot, block_index);
            push_interest_point(offset_point(block->origin, 39.0f, kSidewalkHeight, -34.8f), 8.5f, MDTBInterestPointHotspot, block_index);
            break;
        case MDTBFrontageTemplateServiceSpur:
            push_interest_point(offset_point(block->origin, -30.2f, kSidewalkHeight, -27.9f), 8.0f, MDTBInterestPointHotspot, block_index);
            push_interest_point(offset_point(block->origin, 29.8f, kSidewalkHeight, 13.3f), 8.0f, MDTBInterestPointHotspot, block_index);
            break;
        case MDTBFrontageTemplateCivicRetail:
        default:
            push_interest_point(offset_point(block->origin, -42.0f, kSidewalkHeight, -13.2f), 8.5f, MDTBInterestPointHotspot, block_index);
            push_interest_point(offset_point(block->origin, 35.0f, kSidewalkHeight, 35.0f), 8.5f, MDTBInterestPointHotspot, block_index);
            break;
    }
}

static void build_vehicle_handoff_hooks(const MDTBBlockDescriptor *block, uint32_t block_index) {
    switch (block->frontage_template) {
        case MDTBFrontageTemplateResidentialCourt:
            push_vehicle_anchor(offset_point(block->origin, 24.0f, kRoadHeight, -2.2f), kPi * 0.5f, block_index, MDTBVehicleKindCoupe, MDTBVehicleParkingStateCurbside, MDTBRoadAxisEastWest, -2.2f);
            push_vehicle_anchor(offset_point(block->origin, -24.0f, kRoadHeight, 2.2f), -(kPi * 0.5f), block_index, MDTBVehicleKindBicycle, MDTBVehicleParkingStateCurbside, MDTBRoadAxisEastWest, 2.2f);
            break;
        case MDTBFrontageTemplateTransitMarket:
            push_vehicle_anchor(offset_point(block->origin, -24.0f, kRoadHeight, 2.2f), -(kPi * 0.5f), block_index, MDTBVehicleKindMoped, MDTBVehicleParkingStateCurbside, MDTBRoadAxisEastWest, 2.2f);
            break;
        case MDTBFrontageTemplateServiceSpur:
            push_vehicle_anchor(offset_point(block->origin, 2.2f, kRoadHeight, -24.0f), 0.0f, block_index, MDTBVehicleKindCoupe, MDTBVehicleParkingStateService, MDTBRoadAxisNorthSouth, 2.2f);
            push_vehicle_anchor(offset_point(block->origin, -2.2f, kRoadHeight, 24.0f), kPi, block_index, MDTBVehicleKindMotorcycle, MDTBVehicleParkingStateCurbside, MDTBRoadAxisNorthSouth, -2.2f);
            break;
        case MDTBFrontageTemplateCivicRetail:
        default:
            push_vehicle_anchor(offset_point(block->origin, -2.2f, kRoadHeight, 24.0f), kPi, block_index, MDTBVehicleKindSedan, MDTBVehicleParkingStateCurbside, MDTBRoadAxisNorthSouth, -2.2f);
            break;
    }
}

static void build_combat_sandbox_props(void) {
    const MDTBFloat4 lane_color = make_float4(0.23f, 0.25f, 0.28f, 1.0f);
    const MDTBFloat4 stripe_color = make_float4(0.90f, 0.86f, 0.64f, 1.0f);
    const MDTBFloat4 cover_color = make_float4(0.47f, 0.48f, 0.44f, 1.0f);
    const MDTBFloat4 slab_color = make_float4(0.62f, 0.58f, 0.52f, 1.0f);

    push_scene_box(make_box(
        make_float3(-4.0f, kSidewalkHeight + 0.02f, 51.0f),
        make_float3(13.2f, 0.02f, 11.0f),
        lane_color
    ));

    for (int stripe_index = 0; stripe_index < 4; ++stripe_index) {
        push_scene_box(make_box(
            make_float3(-12.0f + ((float)stripe_index * 5.8f), kSidewalkHeight + 0.03f, 46.8f),
            make_float3(1.45f, 0.01f, 0.10f),
            stripe_color
        ));
        push_scene_box(make_box(
            make_float3(-11.4f + ((float)stripe_index * 5.8f), kSidewalkHeight + 0.03f, 55.1f),
            make_float3(1.20f, 0.01f, 0.10f),
            stripe_color
        ));
    }

    push_prop(
        make_float3(-10.6f, 0.55f, 50.2f),
        make_float3(1.75f, 0.55f, 0.36f),
        cover_color,
        1
    );
    push_prop(
        make_float3(-1.5f, 0.52f, 53.0f),
        make_float3(1.32f, 0.52f, 0.36f),
        cover_color,
        1
    );
    push_prop(
        make_float3(6.2f, 1.06f, 52.7f),
        make_float3(0.12f, 1.06f, 2.25f),
        make_float4(0.30f, 0.31f, 0.35f, 1.0f),
        1
    );
    push_prop(
        make_float3(-4.0f, 0.18f, 58.9f),
        make_float3(9.2f, 0.18f, 0.34f),
        slab_color,
        1
    );
    push_prop(
        make_float3(-12.6f, 1.0f, 60.6f),
        make_float3(2.4f, 1.0f, 0.14f),
        make_float4(0.34f, 0.36f, 0.38f, 1.0f),
        1
    );
    push_prop(
        make_float3(4.8f, 1.0f, 60.6f),
        make_float3(2.8f, 1.0f, 0.14f),
        make_float4(0.34f, 0.36f, 0.38f, 1.0f),
        1
    );

    push_planter(-8.1f, 47.8f, 0.54f);
    push_planter(2.4f, 48.7f, 0.58f);
    push_planter(8.5f, 56.2f, 0.50f);

    push_bollard(-15.8f, 45.2f, 0.92f);
    push_bollard(-11.8f, 45.4f, 0.92f);
    push_bollard(-7.8f, 45.2f, 0.92f);
    push_bollard(-3.8f, 45.4f, 0.92f);
    push_bollard(0.2f, 45.2f, 0.92f);
    push_bollard(4.2f, 45.4f, 0.92f);
}

static void build_residential_block(const MDTBBlockDescriptor *block, uint32_t block_index) {
    build_block_lot_surfaces(block);
    build_side_buildings(block, -1);
    build_side_buildings(block, 1);
    build_intersection_props(block);
    build_frontage_for_block(block);
    build_residential_dynamic_props(block, block_index);

    push_billboard(-24.0f, block->origin.z - 42.0f, 4.8f, 1.25f, 0, make_float4(0.36f, 0.63f, 0.29f, 1.0f));
    push_billboard(46.0f, block->origin.z + 18.5f, 4.6f, 1.25f, 1, make_float4(0.86f, 0.55f, 0.21f, 1.0f));
    push_parked_car(-38.5f, block->origin.z - 35.2f, 1.15f, 2.2f, make_float4(0.22f, 0.48f, 0.63f, 1.0f));
    push_parked_car(35.8f, block->origin.z + 35.6f, 1.15f, 2.2f, make_float4(0.58f, 0.38f, 0.25f, 1.0f));

    push_interest_point(make_float3(0.0f, kSidewalkHeight, block->origin.z), 18.0f, MDTBInterestPointStreamingAnchor, block_index);
    push_interest_point(make_float3(-10.0f, kSidewalkHeight, block->origin.z - 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(10.0f, kSidewalkHeight, block->origin.z + 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(11.2f, kSidewalkHeight, block->origin.z - 16.2f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(-41.6f, kSidewalkHeight, block->origin.z + 15.0f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(35.8f, kSidewalkHeight, block->origin.z + 35.6f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    build_hotspot_hooks(block, block_index);
    build_vehicle_handoff_hooks(block, block_index);
}

static void build_scene(void) {
    g_scene_box_count = 0u;
    g_collision_box_count = 0u;
    g_block_count = 0u;
    g_road_link_count = 0u;
    g_vehicle_anchor_count = 0u;
    g_interest_point_count = 0u;
    g_dynamic_prop_count = 0u;
    g_population_profile_count = 0u;
    g_traffic_occupancy_count = 0u;
    clear_scene_scope();

    build_world_surfaces();
    build_road_markings();

    for (size_t index = 0u; index < (sizeof(kRoadLayout) / sizeof(kRoadLayout[0])); ++index) {
        push_road_link(kRoadLayout[index]);
    }

    for (size_t index = 0u; index < scene_layout_count(); ++index) {
        const MDTBBlockDescriptor block = kBlockLayout[index];
        push_block_descriptor(block);
        if (index < (sizeof(kPopulationProfileLayout) / sizeof(kPopulationProfileLayout[0]))) {
            push_population_profile(kPopulationProfileLayout[index]);
        }
        set_scene_scope((uint32_t)index, MDTBSceneLayerBlockOwned);

        switch (block.kind) {
            case MDTBBlockKindResidential:
                build_residential_block(&block, (uint32_t)index);
                break;
            case MDTBBlockKindMixedUse:
                build_mixed_use_block(&block, (uint32_t)index);
                break;
            case MDTBBlockKindHub:
            default:
                build_hub_block(&block, (uint32_t)index);
                break;
        }

        clear_scene_scope();
    }

    set_scene_scope(0u, MDTBSceneLayerBlockOwned);
    build_combat_sandbox_props();
    clear_scene_scope();

    g_scene_initialized = 1;
}

static void ensure_scene_initialized(void) {
    if (!g_scene_initialized) {
        build_scene();
    }
}

static MDTBGroundInfo ground_info(float x, float z) {
    MDTBGroundInfo info;
    float distance_to_road = 1000000.0f;
    int on_road = 0;

    for (size_t index = 0u; index < scene_layout_count(); ++index) {
        const float x_distance = fmaxf(fabsf(x - kBlockLayout[index].origin.x) - kRoadHalfWidth, 0.0f);
        distance_to_road = fminf(distance_to_road, x_distance);

        if (fabsf(x - kBlockLayout[index].origin.x) <= kRoadHalfWidth) {
            on_road = 1;
        }
    }

    for (size_t index = 0u; index < scene_layout_count(); ++index) {
        const float corridor_distance = fmaxf(fabsf(z - kBlockLayout[index].origin.z) - kRoadHalfWidth, 0.0f);
        distance_to_road = fminf(distance_to_road, corridor_distance);

        if (fabsf(z - kBlockLayout[index].origin.z) <= kRoadHalfWidth) {
            on_road = 1;
        }
    }

    if (on_road) {
        info.height = kRoadHeight;
        info.surface_kind = MDTBSurfaceRoad;
        return info;
    }

    if (distance_to_road <= (kCurbOuter - kRoadHalfWidth)) {
        const float ramp = clampf(distance_to_road / (kCurbOuter - kRoadHalfWidth), 0.0f, 1.0f);
        info.height = kRoadHeight + ((kSidewalkHeight - kRoadHeight) * ramp);
        info.surface_kind = MDTBSurfaceCurb;
        return info;
    }

    if (distance_to_road <= (kSidewalkOuter - kRoadHalfWidth)) {
        info.height = kSidewalkHeight;
        info.surface_kind = MDTBSurfaceSidewalk;
        return info;
    }

    info.height = kSidewalkHeight;
    info.surface_kind = MDTBSurfaceLot;
    return info;
}

static MDTBFloat3 view_forward(float yaw, float pitch) {
    return make_float3(
        sinf(yaw) * cosf(pitch),
        sinf(pitch),
        -cosf(yaw) * cosf(pitch)
    );
}

static MDTBFloat3 normalize_flat(MDTBFloat3 value) {
    const float length = sqrtf((value.x * value.x) + (value.z * value.z));
    if (length <= 0.0001f) {
        return make_float3(0.0f, 0.0f, -1.0f);
    }

    return make_float3(value.x / length, 0.0f, value.z / length);
}

static MDTBFloat3 vehicle_forward_flat(float yaw) {
    return normalize_flat(view_forward(yaw, 0.0f));
}

static MDTBFloat3 vehicle_right_flat(float yaw) {
    return make_float3(cosf(yaw), 0.0f, sinf(yaw));
}

static MDTBFloat3 actor_focus_position(const MDTBEngineState *state) {
    return make_float3(
        state->actor_position.x,
        state->actor_ground_height + kEyeHeight,
        state->actor_position.z
    );
}

static MDTBFloat3 vehicle_seat_position(const MDTBEngineState *state) {
    const MDTBVehicleTuning *tuning = vehicle_tuning_for_kind(state->active_vehicle_kind);
    const MDTBFloat3 forward = vehicle_forward_flat(state->active_vehicle_heading);
    const MDTBFloat3 right = vehicle_right_flat(state->active_vehicle_heading);
    return make_float3(
        state->active_vehicle_position.x + (forward.x * tuning->seat_forward) + (right.x * tuning->seat_side),
        state->active_vehicle_position.y + tuning->seat_height,
        state->active_vehicle_position.z + (forward.z * tuning->seat_forward) + (right.z * tuning->seat_side)
    );
}

static MDTBFloat3 player_focus_position(const MDTBEngineState *state) {
    if (state->traversal_mode == MDTBTraversalModeVehicle) {
        return vehicle_seat_position(state);
    }

    return actor_focus_position(state);
}

static float surface_speed_multiplier(uint32_t surface_kind) {
    switch (surface_kind) {
        case MDTBSurfaceRoad:
            return 0.97f;
        case MDTBSurfaceCurb:
            return 0.82f;
        case MDTBSurfaceLot:
            return 0.90f;
        case MDTBSurfaceSidewalk:
        default:
            return 1.0f;
    }
}

static float resolve_collision_axis(float fixed_value, float current, float proposed, int is_x_axis) {
    return resolve_collision_axis_with_radius(fixed_value, current, proposed, is_x_axis, kPlayerRadius);
}

static MDTBFloat3 resolve_third_person_camera(MDTBFloat3 focus, MDTBFloat3 desired) {
    float safe_t = 1.0f;

    for (int step = 1; step <= 14; ++step) {
        const float t = (float)step / 14.0f;
        const MDTBFloat3 sample = lerp_float3(focus, desired, t);
        const MDTBGroundInfo sample_ground = ground_info(sample.x, sample.z);

        if (sample.y < sample_ground.height + 1.05f) {
            safe_t = (float)(step - 1) / 14.0f;
            break;
        }

        for (size_t index = 0u; index < g_collision_box_count; ++index) {
            const MDTBBox *box = &g_collision_boxes[index];
            if (fabsf(sample.x - box->center.x) <= box->half_extents.x + 0.28f &&
                fabsf(sample.y - box->center.y) <= box->half_extents.y + 0.28f &&
                fabsf(sample.z - box->center.z) <= box->half_extents.z + 0.28f) {
                safe_t = (float)(step - 1) / 14.0f;
                step = 15;
                break;
            }
        }
    }

    MDTBFloat3 resolved = lerp_float3(focus, desired, safe_t);
    const MDTBGroundInfo resolved_ground = ground_info(resolved.x, resolved.z);
    if (resolved.y < resolved_ground.height + 1.10f) {
        resolved.y = resolved_ground.height + 1.10f;
    }

    return resolved;
}

static void sync_active_vehicle_anchor(const MDTBEngineState *state) {
    if (state == NULL || state->active_vehicle_anchor_index >= g_vehicle_anchor_count) {
        return;
    }

    g_vehicle_anchors[state->active_vehicle_anchor_index].position = state->active_vehicle_position;
    g_vehicle_anchors[state->active_vehicle_anchor_index].yaw = state->active_vehicle_heading;
    g_vehicle_anchors[state->active_vehicle_anchor_index].block_index = nearest_block_index_for_position(state->active_vehicle_position);
    if (state->active_link_index < g_road_link_count) {
        const MDTBRoadLink *link = &g_road_links[state->active_link_index];
        g_vehicle_anchors[state->active_vehicle_anchor_index].lane_axis = link->axis;
        if (link->axis == MDTBRoadAxisNorthSouth) {
            g_vehicle_anchors[state->active_vehicle_anchor_index].lane_offset = state->active_vehicle_position.x - link->midpoint.x;
        } else {
            g_vehicle_anchors[state->active_vehicle_anchor_index].lane_offset = state->active_vehicle_position.z - link->midpoint.z;
        }
        g_vehicle_anchors[state->active_vehicle_anchor_index].parking_state =
            state->surface_kind == MDTBSurfaceRoad || state->surface_kind == MDTBSurfaceCurb ?
            MDTBVehicleParkingStateCurbside :
            MDTBVehicleParkingStateService;
    } else {
        g_vehicle_anchors[state->active_vehicle_anchor_index].parking_state =
            state->surface_kind == MDTBSurfaceLot ? MDTBVehicleParkingStateService : MDTBVehicleParkingStateCurbside;
    }
}

static void enter_vehicle_from_anchor(MDTBEngineState *state, uint32_t anchor_index) {
    MDTBGroundInfo ground;
    const MDTBVehicleTuning *tuning;

    if (state == NULL || anchor_index >= g_vehicle_anchor_count) {
        return;
    }

    ground = ground_info(g_vehicle_anchors[anchor_index].position.x, g_vehicle_anchors[anchor_index].position.z);

    state->traversal_mode = MDTBTraversalModeVehicle;
    state->active_vehicle_anchor_index = anchor_index;
    state->active_vehicle_kind = g_vehicle_anchors[anchor_index].kind;
    state->active_vehicle_position = g_vehicle_anchors[anchor_index].position;
    state->active_vehicle_position.y = ground.height;
    state->active_vehicle_heading = g_vehicle_anchors[anchor_index].yaw;
    state->active_vehicle_speed = 0.0f;
    state->actor_velocity = make_float3(0.0f, 0.0f, 0.0f);
    state->actor_position = state->active_vehicle_position;
    state->actor_ground_height = ground.height;
    state->surface_kind = ground.surface_kind;
    state->actor_heading = state->active_vehicle_heading;
    state->target_yaw = state->active_vehicle_heading;
    tuning = vehicle_tuning_for_kind(state->active_vehicle_kind);
    state->target_pitch = clampf(state->target_pitch, tuning->first_person_pitch_min, tuning->first_person_pitch_max);
    state->active_vehicle_surface_grip = vehicle_surface_grip_multiplier(ground.surface_kind);
    state->active_vehicle_lane_error = 0.0f;
    state->active_vehicle_collision_pulse = 0.0f;
    state->active_vehicle_recovery = 0.0f;
    state->active_vehicle_steer_visual = 0.0f;
    clear_melee_attack(state);
    sync_active_vehicle_anchor(state);
}

static MDTBFloat3 resolve_vehicle_exit_position(const MDTBEngineState *state) {
    const MDTBVehicleTuning *tuning = vehicle_tuning_for_kind(state->active_vehicle_kind);
    const MDTBFloat3 forward = vehicle_forward_flat(state->active_vehicle_heading);
    const MDTBFloat3 right = vehicle_right_flat(state->active_vehicle_heading);
    const MDTBFloat3 base = state->active_vehicle_position;
    const float diagonal_side = tuning->exit_side_distance * 0.82f;
    const float diagonal_rear = tuning->exit_rear_distance * 0.68f;
    const MDTBFloat3 candidates[] = {
        make_float3(base.x + (right.x * tuning->exit_side_distance), base.y, base.z + (right.z * tuning->exit_side_distance)),
        make_float3(base.x - (right.x * tuning->exit_side_distance), base.y, base.z - (right.z * tuning->exit_side_distance)),
        make_float3(base.x - (forward.x * tuning->exit_rear_distance), base.y, base.z - (forward.z * tuning->exit_rear_distance)),
        make_float3(base.x + (right.x * diagonal_side) - (forward.x * diagonal_rear), base.y, base.z + (right.z * diagonal_side) - (forward.z * diagonal_rear)),
        make_float3(base.x - (right.x * diagonal_side) - (forward.x * diagonal_rear), base.y, base.z - (right.z * diagonal_side) - (forward.z * diagonal_rear)),
    };
    MDTBFloat3 best_candidate = make_float3(base.x - (forward.x * fmaxf(1.4f, tuning->exit_rear_distance - 0.4f)), ground_info(base.x, base.z).height, base.z - (forward.z * fmaxf(1.4f, tuning->exit_rear_distance - 0.4f)));
    float best_score = 1000000.0f;

    for (size_t index = 0u; index < sizeof(candidates) / sizeof(candidates[0]); ++index) {
        MDTBFloat3 candidate = candidates[index];
        const MDTBGroundInfo ground = ground_info(candidate.x, candidate.z);
        float score;
        candidate.y = ground.height;

        if (position_overlaps_collision(candidate, kPlayerRadius)) {
            continue;
        }

        score = (float)index;
        if (vehicle_is_two_wheel(state->active_vehicle_kind)) {
            score -= (ground.surface_kind == MDTBSurfaceSidewalk ? 1.6f : 0.0f);
            score -= (ground.surface_kind == MDTBSurfaceLot ? 1.1f : 0.0f);
            score += (ground.surface_kind == MDTBSurfaceRoad ? 0.9f : 0.0f);
            score += (ground.surface_kind == MDTBSurfaceCurb ? 0.2f : 0.0f);
        }

        if (score < best_score) {
            best_score = score;
            best_candidate = candidate;
        }
    }

    return best_candidate;
}

static void exit_vehicle_to_ground(MDTBEngineState *state) {
    MDTBFloat3 exit_position;
    MDTBGroundInfo ground;

    if (state == NULL) {
        return;
    }

    exit_position = resolve_vehicle_exit_position(state);
    ground = ground_info(exit_position.x, exit_position.z);
    exit_position.y = ground.height;

    state->traversal_mode = MDTBTraversalModeOnFoot;
    state->actor_position = exit_position;
    state->actor_ground_height = ground.height;
    state->actor_velocity = make_float3(0.0f, 0.0f, 0.0f);
    state->actor_heading = state->active_vehicle_heading;
    state->surface_kind = ground.surface_kind;
    state->camera.move_speed = 0.0f;
    state->active_vehicle_speed = 0.0f;
    state->active_vehicle_anchor_index = MDTBIndexNone;
    state->target_yaw = state->actor_heading;
    state->target_pitch = clampf(state->target_pitch, -0.52f, 0.38f);
    state->active_vehicle_surface_grip = 0.0f;
    state->active_vehicle_lane_error = 0.0f;
    state->active_vehicle_collision_pulse = 0.0f;
    state->active_vehicle_recovery = 0.0f;
    state->active_vehicle_steer_visual = 0.0f;
}

static int can_enter_vehicle_anchor(const MDTBEngineState *state, uint32_t anchor_index) {
    if (state == NULL || anchor_index >= g_vehicle_anchor_count) {
        return 0;
    }

    return distance_squared_xz(state->actor_position, g_vehicle_anchors[anchor_index].position) <=
        (kVehicleMountRadius * kVehicleMountRadius);
}

static void step_on_foot_movement(MDTBEngineState *state, MDTBInputFrame input, float dt) {
    const MDTBGroundInfo current_ground = ground_info(state->actor_position.x, state->actor_position.z);
    const float traversal_scale = surface_speed_multiplier(current_ground.surface_kind);
    const float base_speed = (input.buttons & MDTBInputSprint) != 0u ? 10.5f : 6.45f;
    const float max_speed = base_speed * traversal_scale;
    const float forward_x = sinf(state->target_yaw);
    const float forward_z = -cosf(state->target_yaw);
    const float right_x = cosf(state->target_yaw);
    const float right_z = sinf(state->target_yaw);
    float desired_velocity_x = 0.0f;
    float desired_velocity_z = 0.0f;
    float proposed_x;
    float proposed_z;
    float resolved_x;
    float resolved_z;
    float clamped_x;
    float clamped_z;
    float move_length;
    MDTBGroundInfo ground;
    const float ground_follow_speed = current_ground.surface_kind == MDTBSurfaceCurb ? 16.0f : 24.0f;

    if ((input.buttons & MDTBInputMoveForward) != 0u) {
        desired_velocity_x += forward_x;
        desired_velocity_z += forward_z;
    }

    if ((input.buttons & MDTBInputMoveBackward) != 0u) {
        desired_velocity_x -= forward_x;
        desired_velocity_z -= forward_z;
    }

    if ((input.buttons & MDTBInputMoveRight) != 0u) {
        desired_velocity_x += right_x;
        desired_velocity_z += right_z;
    }

    if ((input.buttons & MDTBInputMoveLeft) != 0u) {
        desired_velocity_x -= right_x;
        desired_velocity_z -= right_z;
    }

    move_length = sqrtf((desired_velocity_x * desired_velocity_x) + (desired_velocity_z * desired_velocity_z));
    if (move_length > 0.001f) {
        desired_velocity_x = (desired_velocity_x / move_length) * max_speed;
        desired_velocity_z = (desired_velocity_z / move_length) * max_speed;
    }

    state->actor_velocity.x = approachf(state->actor_velocity.x, desired_velocity_x, 13.0f, dt);
    state->actor_velocity.z = approachf(state->actor_velocity.z, desired_velocity_z, 13.0f, dt);

    if (move_length > 0.15f) {
        const float target_heading = atan2f(desired_velocity_x, -desired_velocity_z);
        state->actor_heading = approach_angle(state->actor_heading, target_heading, 13.0f, dt);
    } else if (state->camera.mode == MDTBCameraModeThirdPerson) {
        state->actor_heading = approach_angle(state->actor_heading, state->camera.yaw, 5.5f, dt);
    }

    proposed_x = state->actor_position.x + (state->actor_velocity.x * dt);
    proposed_z = state->actor_position.z + (state->actor_velocity.z * dt);

    resolved_x = resolve_collision_axis(state->actor_position.z, state->actor_position.x, proposed_x, 1);
    clamped_x = clampf(resolved_x, -kPlayableHalfWidth, kPlayableHalfWidth);
    if (fabsf(clamped_x - proposed_x) > 0.0001f) {
        state->actor_velocity.x = 0.0f;
    }
    state->actor_position.x = clamped_x;

    resolved_z = resolve_collision_axis(state->actor_position.x, state->actor_position.z, proposed_z, 0);
    clamped_z = clampf(resolved_z, -kPlayableHalfLength, kPlayableHalfLength);
    if (fabsf(clamped_z - proposed_z) > 0.0001f) {
        state->actor_velocity.z = 0.0f;
    }
    state->actor_position.z = clamped_z;

    ground = ground_info(state->actor_position.x, state->actor_position.z);
    state->actor_ground_height = approachf(state->actor_ground_height, ground.height, ground_follow_speed, dt);
    state->actor_position.y = state->actor_ground_height;
    state->surface_kind = ground.surface_kind;
    state->camera.move_speed = sqrtf((state->actor_velocity.x * state->actor_velocity.x) + (state->actor_velocity.z * state->actor_velocity.z));
}

static void step_vehicle_movement(MDTBEngineState *state, MDTBInputFrame input, float dt) {
    const MDTBVehicleTuning *tuning = vehicle_tuning_for_kind(state->active_vehicle_kind);
    const MDTBGroundInfo current_ground = ground_info(state->active_vehicle_position.x, state->active_vehicle_position.z);
    const int is_two_wheel = vehicle_is_two_wheel(state->active_vehicle_kind);
    const float grip = vehicle_surface_grip_multiplier(current_ground.surface_kind);
    const float throttle = ((input.buttons & MDTBInputMoveForward) != 0u ? 1.0f : 0.0f) - ((input.buttons & MDTBInputMoveBackward) != 0u ? 1.0f : 0.0f);
    const float steer_input =
        ((input.buttons & MDTBInputMoveRight) != 0u ? 1.0f : 0.0f) -
        ((input.buttons & MDTBInputMoveLeft) != 0u ? 1.0f : 0.0f) +
        ((input.buttons & MDTBInputTurnRight) != 0u ? 0.75f : 0.0f) -
        ((input.buttons & MDTBInputTurnLeft) != 0u ? 0.75f : 0.0f);
    const float recovery = state->active_vehicle_recovery;
    const float recovery_drag = 1.0f - (recovery * 0.24f);
    const float steer_visual_scale =
        state->active_vehicle_kind == MDTBVehicleKindBicycle ? 1.28f :
        (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? 1.10f :
        (state->active_vehicle_kind == MDTBVehicleKindMoped ? 0.96f : 0.62f));
    const float max_forward_speed = tuning->max_forward_speed * grip * recovery_drag;
    const float max_reverse_speed = tuning->max_reverse_speed * (0.72f + (grip * 0.28f)) * recovery_drag;
    const float target_speed = throttle > 0.0f ? max_forward_speed : (throttle < 0.0f ? -max_reverse_speed : 0.0f);
    const float approach_rate = throttle == 0.0f ?
        tuning->coast_drag_rate + ((1.0f - grip) * 2.2f) :
        ((throttle < 0.0f && state->active_vehicle_speed > 0.2f) ? tuning->brake_rate : tuning->acceleration_rate * recovery_drag);
    const float speed_ratio = clampf(fabsf(state->active_vehicle_speed) / fmaxf(tuning->max_forward_speed, 0.01f), 0.0f, 1.0f);
    const float low_speed_turn_bonus = is_two_wheel ? (1.08f + ((1.0f - speed_ratio) * 0.24f)) : 1.0f;
    const float lane_assist_scale = is_two_wheel ? (0.70f + (speed_ratio * 0.30f)) : 1.0f;
    const float steer_rate = tuning->steer_rate * (0.60f + ((1.0f - speed_ratio) * 0.55f)) * (0.62f + (grip * 0.38f)) * (0.78f + ((1.0f - recovery) * 0.22f)) * low_speed_turn_bonus;
    const float steer_direction = state->active_vehicle_speed < -0.2f ? -0.45f : 1.0f;
    MDTBFloat3 forward;
    float proposed_x;
    float proposed_z;
    float resolved_x;
    float resolved_z;
    float clamped_x;
    float clamped_z;
    MDTBGroundInfo ground;
    int collided = 0;

    state->active_vehicle_collision_pulse = approachf(state->active_vehicle_collision_pulse, 0.0f, 6.5f, dt);
    state->active_vehicle_recovery = approachf(state->active_vehicle_recovery, 0.0f, 2.2f, dt);
    state->active_vehicle_steer_visual = approachf(
        state->active_vehicle_steer_visual,
        steer_input * steer_visual_scale * (0.35f + speed_ratio * 0.65f),
        8.5f,
        dt
    );
    state->active_vehicle_surface_grip = grip;
    state->active_vehicle_speed = approachf(state->active_vehicle_speed, target_speed, approach_rate, dt);
    if (throttle == 0.0f && fabsf(state->active_vehicle_speed) < 0.12f) {
        state->active_vehicle_speed = 0.0f;
    }

    if (state->active_link_index < g_road_link_count) {
        const MDTBRoadLink *link = &g_road_links[state->active_link_index];
        const float lane_heading = vehicle_lane_heading_for_axis(link->axis, state->active_vehicle_heading);
        state->active_vehicle_heading = approach_angle(
            state->active_vehicle_heading,
            lane_heading,
            kVehicleLaneAssistStrength * grip * (0.70f + ((1.0f - recovery) * 0.30f)) * lane_assist_scale,
            dt
        );
    }

    if (fabsf(steer_input) > 0.001f) {
        state->active_vehicle_heading = wrap_angle(state->active_vehicle_heading + (steer_input * steer_rate * steer_direction * dt));
    }

    forward = vehicle_forward_flat(state->active_vehicle_heading);
    proposed_x = state->active_vehicle_position.x + (forward.x * state->active_vehicle_speed * dt);
    proposed_z = state->active_vehicle_position.z + (forward.z * state->active_vehicle_speed * dt);

    resolved_x = resolve_collision_axis_with_radius(state->active_vehicle_position.z, state->active_vehicle_position.x, proposed_x, 1, tuning->collision_radius);
    clamped_x = clampf(resolved_x, -kPlayableHalfWidth, kPlayableHalfWidth);
    if (fabsf(clamped_x - proposed_x) > 0.0001f) {
        collided = 1;
    }
    state->active_vehicle_position.x = clamped_x;

    resolved_z = resolve_collision_axis_with_radius(state->active_vehicle_position.x, state->active_vehicle_position.z, proposed_z, 0, tuning->collision_radius);
    clamped_z = clampf(resolved_z, -kPlayableHalfLength, kPlayableHalfLength);
    if (fabsf(clamped_z - proposed_z) > 0.0001f) {
        collided = 1;
    }
    state->active_vehicle_position.z = clamped_z;

    if (collided) {
        state->active_vehicle_speed *= -0.12f;
        state->active_vehicle_collision_pulse = fmaxf(state->active_vehicle_collision_pulse, 1.0f);
        state->active_vehicle_recovery = fmaxf(state->active_vehicle_recovery, 1.0f);
    }

    ground = ground_info(state->active_vehicle_position.x, state->active_vehicle_position.z);
    {
        const float absolute_speed = fabsf(state->active_vehicle_speed);
        const int soft_curb_transition = is_two_wheel &&
            absolute_speed <= 4.4f &&
            ((current_ground.surface_kind == MDTBSurfaceRoad &&
              (ground.surface_kind == MDTBSurfaceCurb || ground.surface_kind == MDTBSurfaceSidewalk)) ||
             (current_ground.surface_kind == MDTBSurfaceCurb &&
              (ground.surface_kind == MDTBSurfaceRoad || ground.surface_kind == MDTBSurfaceSidewalk)) ||
             (current_ground.surface_kind == MDTBSurfaceSidewalk && ground.surface_kind == MDTBSurfaceCurb));
        const int soft_service_transition = is_two_wheel &&
            absolute_speed <= 3.2f &&
            current_ground.surface_kind != MDTBSurfaceLot &&
            ground.surface_kind == MDTBSurfaceLot;

        if (soft_curb_transition || soft_service_transition) {
            const float height_follow_rate = soft_curb_transition ?
                8.0f + (absolute_speed * 2.4f) :
                7.0f + (absolute_speed * 1.8f);
            state->active_vehicle_position.y = approachf(state->active_vehicle_position.y, ground.height, height_follow_rate, dt);
        } else {
            state->active_vehicle_position.y = ground.height;
        }

        state->active_vehicle_surface_grip = vehicle_surface_grip_multiplier(ground.surface_kind);

        if (ground.surface_kind != MDTBSurfaceRoad && absolute_speed > 0.4f) {
            float offroad_recovery = clampf((1.0f - state->active_vehicle_surface_grip) * (0.34f + speed_ratio * 0.44f), 0.0f, 0.72f);
            float collision_pulse = clampf((1.0f - state->active_vehicle_surface_grip) * (0.24f + speed_ratio * 0.36f), 0.0f, 0.65f);

            if (soft_curb_transition) {
                const float low_speed_soften = clampf(1.0f - (absolute_speed / 4.5f), 0.0f, 1.0f);
                offroad_recovery *= 0.46f - (low_speed_soften * 0.14f);
                collision_pulse *= 0.52f - (low_speed_soften * 0.14f);
            } else if (soft_service_transition) {
                offroad_recovery *= 0.68f;
                collision_pulse *= 0.74f;
            }

            state->active_vehicle_collision_pulse = fmaxf(state->active_vehicle_collision_pulse, collision_pulse);
            state->active_vehicle_recovery = fmaxf(state->active_vehicle_recovery, offroad_recovery);
        }
    }

    if (state->active_link_index < g_road_link_count) {
        const MDTBRoadLink *link = &g_road_links[state->active_link_index];
        const float desired_lane_offset = vehicle_lane_offset_for_heading(link->axis, state->active_vehicle_heading, tuning->travel_lane_offset);
        if (link->axis == MDTBRoadAxisNorthSouth) {
            const float desired_x = link->midpoint.x + desired_lane_offset;
            const float corrected_x = resolve_collision_axis_with_radius(
                state->active_vehicle_position.z,
                state->active_vehicle_position.x,
                approachf(
                    state->active_vehicle_position.x,
                    desired_x,
                    kVehicleLanePullStrength * grip * (0.68f + ((1.0f - recovery) * 0.32f)) * lane_assist_scale,
                    dt
                ),
                1,
                tuning->collision_radius
            );
            state->active_vehicle_lane_error = fabsf(corrected_x - desired_x);
            state->active_vehicle_position.x = corrected_x;
        } else {
            const float desired_z = link->midpoint.z + desired_lane_offset;
            const float corrected_z = resolve_collision_axis_with_radius(
                state->active_vehicle_position.x,
                state->active_vehicle_position.z,
                approachf(
                    state->active_vehicle_position.z,
                    desired_z,
                    kVehicleLanePullStrength * grip * (0.68f + ((1.0f - recovery) * 0.32f)) * lane_assist_scale,
                    dt
                ),
                0,
                tuning->collision_radius
            );
            state->active_vehicle_lane_error = fabsf(corrected_z - desired_z);
            state->active_vehicle_position.z = corrected_z;
        }
    } else {
        state->active_vehicle_lane_error = 0.0f;
    }

    state->actor_position = state->active_vehicle_position;
    state->actor_ground_height = state->active_vehicle_position.y;
    state->surface_kind = ground.surface_kind;
    state->actor_heading = state->active_vehicle_heading;
    state->camera.move_speed = fabsf(state->active_vehicle_speed);
    sync_active_vehicle_anchor(state);
}

void mdtb_engine_init(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    ensure_scene_initialized();
    memset(state, 0, sizeof(*state));
    state->actor_position.x = -9.0f;
    state->actor_position.z = 26.0f;
    state->actor_ground_height = kSidewalkHeight;
    state->actor_position.y = state->actor_ground_height;
    state->camera.yaw = 0.18f;
    state->camera.pitch = -0.10f;
    state->camera.mode = MDTBCameraModeFirstPerson;
    state->active_block_index = MDTBIndexNone;
    state->active_link_index = MDTBIndexNone;
    state->target_yaw = state->camera.yaw;
    state->target_pitch = state->camera.pitch;
    state->actor_heading = state->camera.yaw;
    state->surface_kind = MDTBSurfaceSidewalk;
    state->traversal_mode = MDTBTraversalModeOnFoot;
    state->active_vehicle_anchor_index = MDTBIndexNone;
    state->nearby_vehicle_anchor_index = MDTBIndexNone;
    state->secondary_vehicle_anchor_index = MDTBIndexNone;
    state->tertiary_vehicle_anchor_index = MDTBIndexNone;
    state->locked_vehicle_anchor_index = MDTBIndexNone;
    state->vehicle_selection_locked = 0u;
    state->active_vehicle_kind = MDTBVehicleKindSedan;
    state->active_vehicle_position = make_float3(0.0f, kRoadHeight, 0.0f);
    state->active_vehicle_heading = 0.0f;
    state->active_vehicle_speed = 0.0f;
    state->active_vehicle_surface_grip = 0.0f;
    state->active_vehicle_lane_error = 0.0f;
    state->active_vehicle_collision_pulse = 0.0f;
    state->active_vehicle_recovery = 0.0f;
    state->active_vehicle_steer_visual = 0.0f;
    state->melee_weapon_owned = 0u;
    state->melee_weapon_pickup_in_range = 0u;
    state->melee_weapon_pickup_position = kLeadPipePickupPosition;
    state->melee_attack_phase = MDTBMeleeAttackIdle;
    state->melee_attack_connected = 0u;
    state->melee_attack_timer = 0.0f;
    state->firearm_owned = 0u;
    state->firearm_pickup_in_range = 0u;
    state->firearm_pickup_position = kPistolPickupPosition;
    state->equipped_weapon_kind = MDTBEquippedWeaponNone;
    state->firearm_clip_ammo = 0u;
    state->firearm_reserve_ammo = 0u;
    state->firearm_reloading = 0u;
    state->firearm_reload_timer = 0.0f;
    state->firearm_cooldown_timer = 0.0f;
    clear_firearm_last_shot(state);
    state->combat_target_position = kPracticeDummyPosition;
    state->combat_target_in_range = 0u;
    state->combat_target_health = kPracticeDummyMaxHealth;
    state->combat_target_reaction = 0.0f;
    state->combat_target_reset_timer = 0.0f;
    state->combat_hostile_position = kLookoutBasePosition;
    state->combat_hostile_heading = kPi;
    state->combat_hostile_in_range = 0u;
    state->combat_hostile_health = kLookoutMaxHealth;
    state->combat_hostile_reaction = 0.0f;
    state->combat_hostile_reset_timer = 0.0f;
    state->combat_hostile_alert = 0.0f;
    state->combat_focus_target_kind = MDTBCombatTargetNone;
    state->combat_focus_distance = 0.0f;
    state->combat_focus_alignment = 0.0f;
    state->combat_last_hit_target_kind = MDTBCombatTargetNone;
    state->camera.focus_position = player_focus_position(state);
    state->camera.position = state->camera.focus_position;
    update_runtime_activity(state);
    refresh_combat_proximity(state);
    refresh_traffic_occupancies(state);
}

void mdtb_engine_step(MDTBEngineState *state, MDTBInputFrame input) {
    if (state == NULL) {
        return;
    }

    ensure_scene_initialized();
    const float dt = clampf(input.delta_time, 0.0f, 0.1f);
    const float turn_speed = 1.75f;
    const float look_speed = 1.0f;
    const float mouse_turn_speed = 0.0044f;
    const float mouse_look_speed = 0.0032f;
    const MDTBVehicleTuning *active_vehicle_tuning = vehicle_tuning_for_kind(state->active_vehicle_kind);
    const float clamped_look_dx = clampf(input.look_delta_x, -80.0f, 80.0f);
    const float clamped_look_dy = clampf(input.look_delta_y, -80.0f, 80.0f);
    const float min_pitch = state->traversal_mode == MDTBTraversalModeVehicle ? active_vehicle_tuning->first_person_pitch_min : -0.52f;
    const float max_pitch = state->traversal_mode == MDTBTraversalModeVehicle ? active_vehicle_tuning->first_person_pitch_max : 0.38f;

    if ((input.buttons & MDTBInputToggleCamera) != 0u) {
        state->camera.mode = state->camera.mode == MDTBCameraModeFirstPerson ? MDTBCameraModeThirdPerson : MDTBCameraModeFirstPerson;
    }

    state->target_yaw += clamped_look_dx * mouse_turn_speed;
    state->target_yaw += ((input.buttons & MDTBInputTurnLeft) != 0u ? -turn_speed : 0.0f) * dt;
    state->target_yaw += ((input.buttons & MDTBInputTurnRight) != 0u ? turn_speed : 0.0f) * dt;
    state->target_yaw = wrap_angle(state->target_yaw);

    state->target_pitch += -clamped_look_dy * mouse_look_speed;
    state->target_pitch += ((input.buttons & MDTBInputLookUp) != 0u ? look_speed : 0.0f) * dt;
    state->target_pitch += ((input.buttons & MDTBInputLookDown) != 0u ? -look_speed : 0.0f) * dt;
    state->target_pitch = clampf(state->target_pitch, min_pitch, max_pitch);

    if (state->traversal_mode == MDTBTraversalModeVehicle && state->camera.mode == MDTBCameraModeFirstPerson) {
        const float relative_yaw = clampf(wrap_angle(state->target_yaw - state->active_vehicle_heading), -active_vehicle_tuning->first_person_yaw_limit, active_vehicle_tuning->first_person_yaw_limit);
        state->target_yaw = wrap_angle(state->active_vehicle_heading + relative_yaw);
    } else if (state->traversal_mode == MDTBTraversalModeVehicle && state->camera.mode == MDTBCameraModeThirdPerson) {
        state->target_yaw = approach_angle(state->target_yaw, state->active_vehicle_heading, 3.0f, dt);
    }

    state->camera.yaw = approach_angle(state->camera.yaw, state->target_yaw, 12.5f, dt);
    state->camera.pitch = approachf(state->camera.pitch, state->target_pitch, 10.0f, dt);

    if (state->traversal_mode == MDTBTraversalModeVehicle) {
        step_vehicle_movement(state, input, dt);
    } else {
        step_on_foot_movement(state, input, dt);
    }

    update_runtime_activity(state);
    refresh_combat_proximity(state);

    if (state->traversal_mode == MDTBTraversalModeOnFoot) {
        if ((input.buttons & MDTBInputCycleHandoff) != 0u) {
            cycle_vehicle_selection(state);
        }

        if ((input.buttons & MDTBInputToggleHandoffLock) != 0u) {
            toggle_vehicle_selection_lock(state);
        }

        if ((input.buttons & MDTBInputPickupWeapon) != 0u) {
            pickup_nearest_weapon(state);
            refresh_combat_proximity(state);
        }

        if ((input.buttons & MDTBInputEquipMeleeWeapon) != 0u) {
            equip_weapon(state, MDTBEquippedWeaponLeadPipe);
        }

        if ((input.buttons & MDTBInputEquipFirearm) != 0u) {
            equip_weapon(state, MDTBEquippedWeaponPistol);
        }
    }

    if ((input.buttons & MDTBInputUse) != 0u) {
        if (state->traversal_mode == MDTBTraversalModeOnFoot) {
            if (state->nearby_vehicle_anchor_index < g_vehicle_anchor_count &&
                can_enter_vehicle_anchor(state, state->nearby_vehicle_anchor_index)) {
                enter_vehicle_from_anchor(state, state->nearby_vehicle_anchor_index);
                update_runtime_activity(state);
            }
        } else if (fabsf(state->active_vehicle_speed) <= 1.4f) {
            exit_vehicle_to_ground(state);
            update_runtime_activity(state);
        }
    }

    step_combat_state(
        state,
        dt,
        (input.buttons & MDTBInputAttack) != 0u,
        (input.buttons & MDTBInputReloadWeapon) != 0u
    );
    refresh_traffic_occupancies(state);

    if (state->traversal_mode == MDTBTraversalModeVehicle) {
        const MDTBFloat3 vehicle_focus = player_focus_position(state);
        const MDTBFloat3 flat_forward = view_forward(state->camera.yaw, 0.0f);
        const MDTBFloat3 vehicle_forward = vehicle_forward_flat(state->active_vehicle_heading);
        const MDTBFloat3 shoulder_right = make_float3(cosf(state->camera.yaw), 0.0f, sinf(state->camera.yaw));
        const float speed_ratio = fminf(state->camera.move_speed / fmaxf(active_vehicle_tuning->max_forward_speed, 0.01f), 1.0f);
        const float bump = state->active_vehicle_collision_pulse;
        const float recovery = state->active_vehicle_recovery;
        const float recovery_sway = sinf(state->elapsed_time * 19.0f) * recovery * 0.12f;
        const float steer_visual = state->active_vehicle_steer_visual;
        const float class_lean =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ? steer_visual * 0.22f :
            (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? steer_visual * 0.17f :
            (state->active_vehicle_kind == MDTBVehicleKindMoped ? steer_visual * 0.14f : steer_visual * 0.08f));
        const float class_bob =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ?
            sinf(state->elapsed_time * 10.5f) * (0.02f + speed_ratio * 0.05f) :
            ((state->active_vehicle_kind == MDTBVehicleKindMotorcycle || state->active_vehicle_kind == MDTBVehicleKindMoped) ?
            sinf(state->elapsed_time * 13.0f) * (0.01f + speed_ratio * 0.02f) :
            0.0f);
        const float focus_lead =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ? 2.45f :
            (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? 3.15f :
            (state->active_vehicle_kind == MDTBVehicleKindMoped ? 2.65f : 2.8f));
        const float camera_height_bias =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ? 0.14f :
            (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? -0.02f :
            (state->active_vehicle_kind == MDTBVehicleKindMoped ? 0.04f : 0.0f));
        const float camera_distance_bias =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ? -0.8f :
            (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? -0.25f :
            (state->active_vehicle_kind == MDTBVehicleKindMoped ? -0.55f : 0.0f));
        const float camera_side_bias =
            state->active_vehicle_kind == MDTBVehicleKindBicycle ? 0.18f :
            (state->active_vehicle_kind == MDTBVehicleKindMotorcycle ? 0.10f :
            (state->active_vehicle_kind == MDTBVehicleKindMoped ? 0.06f : 0.0f));

        if (state->camera.mode == MDTBCameraModeFirstPerson) {
            state->camera.focus_position = approach_float3(
                state->camera.focus_position,
                make_float3(
                    vehicle_focus.x + (vehicle_forward.x * focus_lead),
                    vehicle_focus.y + 0.06f + class_bob + (bump * 0.12f) + (recovery * 0.06f),
                    vehicle_focus.z + (vehicle_forward.z * focus_lead)
                ),
                12.0f,
                dt
            );
            state->camera.position = approach_float3(
                state->camera.position,
                make_float3(
                    vehicle_focus.x - (shoulder_right.x * ((bump * 0.18f) + recovery_sway + class_lean)),
                    vehicle_focus.y + class_bob + (bump * 0.06f) + (recovery * 0.03f),
                    vehicle_focus.z - (shoulder_right.z * ((bump * 0.18f) + recovery_sway + class_lean))
                ),
                16.0f,
                dt
            );
        } else {
            const MDTBFloat3 lead_direction = normalize_flat(lerp_float3(flat_forward, vehicle_forward, 0.55f));
            const MDTBFloat3 focus_target = make_float3(
                state->active_vehicle_position.x + (lead_direction.x * (1.6f + speed_ratio * 1.1f)),
                state->active_vehicle_position.y + active_vehicle_tuning->seat_height + 0.10f + camera_height_bias + class_bob + (speed_ratio * 0.12f) + (bump * 0.10f),
                state->active_vehicle_position.z + (lead_direction.z * (1.6f + speed_ratio * 1.1f))
            );
            MDTBFloat3 desired_camera;

            state->camera.focus_position = approach_float3(state->camera.focus_position, focus_target, 9.0f, dt);

            desired_camera = make_float3(
                state->camera.focus_position.x - (flat_forward.x * (active_vehicle_tuning->third_person_distance + camera_distance_bias + speed_ratio * 1.2f)) + (shoulder_right.x * (active_vehicle_tuning->third_person_side + camera_side_bias + recovery_sway + class_lean)),
                state->camera.focus_position.y + active_vehicle_tuning->third_person_height + camera_height_bias + class_bob + (fmaxf(-state->camera.pitch, 0.0f) * 0.55f) + (bump * 0.24f) + (recovery * 0.10f),
                state->camera.focus_position.z - (flat_forward.z * (active_vehicle_tuning->third_person_distance + camera_distance_bias + speed_ratio * 1.2f)) + (shoulder_right.z * (active_vehicle_tuning->third_person_side + camera_side_bias + recovery_sway + class_lean))
            );

            desired_camera = resolve_third_person_camera(state->camera.focus_position, desired_camera);
            state->camera.position = approach_float3(state->camera.position, desired_camera, 7.6f, dt);
        }
    } else {
        const MDTBFloat3 actor_focus = player_focus_position(state);

        if (state->camera.mode == MDTBCameraModeFirstPerson) {
            MDTBFloat3 camera_target = actor_focus;
            camera_target.y += sinf(state->elapsed_time * 8.5f) * fminf(state->camera.move_speed / 10.0f, 1.0f) * 0.025f;
            state->camera.focus_position = actor_focus;
            state->camera.position.x = camera_target.x;
            state->camera.position.z = camera_target.z;
            state->camera.position.y = approachf(state->camera.position.y, camera_target.y, 18.0f, dt);
        } else {
            const MDTBFloat3 flat_forward = view_forward(state->camera.yaw, 0.0f);
            const MDTBFloat3 actor_forward = view_forward(state->actor_heading, 0.0f);
            const MDTBFloat3 shoulder_right = make_float3(cosf(state->camera.yaw), 0.0f, sinf(state->camera.yaw));
            const float speed_ratio = fminf(state->camera.move_speed / 9.0f, 1.0f);
            const float movement_bias = clampf(state->camera.move_speed / 3.0f, 0.0f, 1.0f);
            const MDTBFloat3 lead_direction = normalize_flat(lerp_float3(flat_forward, actor_forward, movement_bias));
            const float lead = 0.55f + (speed_ratio * 1.05f);
            MDTBFloat3 desired_camera;

            const MDTBFloat3 focus_target = make_float3(
                actor_focus.x + (lead_direction.x * lead),
                actor_focus.y + 0.34f + (speed_ratio * 0.18f),
                actor_focus.z + (lead_direction.z * lead)
            );

            state->camera.focus_position = approach_float3(state->camera.focus_position, focus_target, 10.0f, dt);

            desired_camera = make_float3(
                state->camera.focus_position.x - (flat_forward.x * (6.6f + speed_ratio * 1.35f)) + (shoulder_right.x * 1.18f),
                state->camera.focus_position.y + 1.18f + (speed_ratio * 0.22f) + (fmaxf(-state->camera.pitch, 0.0f) * 0.65f),
                state->camera.focus_position.z - (flat_forward.z * (6.6f + speed_ratio * 1.35f)) + (shoulder_right.z * 1.18f)
            );

            desired_camera = resolve_third_person_camera(state->camera.focus_position, desired_camera);
            state->camera.position = approach_float3(state->camera.position, desired_camera, 8.5f, dt);
        }
    }

    state->elapsed_time += dt;
}

size_t mdtb_engine_box_count(void) {
    ensure_scene_initialized();
    return g_scene_box_count;
}

void mdtb_engine_copy_boxes(MDTBBox *boxes, size_t count) {
    ensure_scene_initialized();
    if (boxes == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_scene_box_count ? count : g_scene_box_count;
    for (size_t index = 0u; index < copy_count; ++index) {
        boxes[index] = g_scene_boxes[index].box;
    }
}

size_t mdtb_engine_scene_box_count(void) {
    ensure_scene_initialized();
    return g_scene_box_count;
}

void mdtb_engine_copy_scene_boxes(MDTBSceneBox *boxes, size_t count) {
    ensure_scene_initialized();
    if (boxes == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_scene_box_count ? count : g_scene_box_count;
    memcpy(boxes, g_scene_boxes, copy_count * sizeof(MDTBSceneBox));
}

size_t mdtb_engine_block_count(void) {
    ensure_scene_initialized();
    return g_block_count;
}

void mdtb_engine_copy_blocks(MDTBBlockDescriptor *blocks, size_t count) {
    ensure_scene_initialized();
    if (blocks == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_block_count ? count : g_block_count;
    memcpy(blocks, g_blocks, copy_count * sizeof(MDTBBlockDescriptor));
}

size_t mdtb_engine_road_link_count(void) {
    ensure_scene_initialized();
    return g_road_link_count;
}

void mdtb_engine_copy_road_links(MDTBRoadLink *links, size_t count) {
    ensure_scene_initialized();
    if (links == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_road_link_count ? count : g_road_link_count;
    memcpy(links, g_road_links, copy_count * sizeof(MDTBRoadLink));
}

size_t mdtb_engine_vehicle_anchor_count(void) {
    ensure_scene_initialized();
    return g_vehicle_anchor_count;
}

void mdtb_engine_copy_vehicle_anchors(MDTBVehicleAnchor *anchors, size_t count) {
    ensure_scene_initialized();
    if (anchors == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_vehicle_anchor_count ? count : g_vehicle_anchor_count;
    memcpy(anchors, g_vehicle_anchors, copy_count * sizeof(MDTBVehicleAnchor));
}

size_t mdtb_engine_interest_point_count(void) {
    ensure_scene_initialized();
    return g_interest_point_count;
}

void mdtb_engine_copy_interest_points(MDTBInterestPoint *points, size_t count) {
    ensure_scene_initialized();
    if (points == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_interest_point_count ? count : g_interest_point_count;
    memcpy(points, g_interest_points, copy_count * sizeof(MDTBInterestPoint));
}

size_t mdtb_engine_dynamic_prop_count(void) {
    ensure_scene_initialized();
    return g_dynamic_prop_count;
}

void mdtb_engine_copy_dynamic_props(MDTBDynamicProp *props, size_t count) {
    ensure_scene_initialized();
    if (props == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_dynamic_prop_count ? count : g_dynamic_prop_count;
    memcpy(props, g_dynamic_props, copy_count * sizeof(MDTBDynamicProp));
}

size_t mdtb_engine_population_profile_count(void) {
    ensure_scene_initialized();
    return g_population_profile_count;
}

void mdtb_engine_copy_population_profiles(MDTBPopulationProfile *profiles, size_t count) {
    ensure_scene_initialized();
    if (profiles == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_population_profile_count ? count : g_population_profile_count;
    memcpy(profiles, g_population_profiles, copy_count * sizeof(MDTBPopulationProfile));
}

size_t mdtb_engine_traffic_occupancy_count(void) {
    ensure_scene_initialized();
    return g_traffic_occupancy_count;
}

void mdtb_engine_copy_traffic_occupancies(MDTBTrafficOccupancy *occupancies, size_t count) {
    ensure_scene_initialized();
    if (occupancies == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < g_traffic_occupancy_count ? count : g_traffic_occupancy_count;
    memcpy(occupancies, g_traffic_occupancies, copy_count * sizeof(MDTBTrafficOccupancy));
}
