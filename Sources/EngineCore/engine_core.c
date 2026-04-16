#include "engine_core.h"

#include <math.h>
#include <string.h>

enum {
    MDTBMaxSceneBoxes = 2304,
    MDTBMaxCollisionBoxes = 768,
};

typedef struct {
    float height;
    uint32_t surface_kind;
} MDTBGroundInfo;

static MDTBBox g_scene_boxes[MDTBMaxSceneBoxes];
static size_t g_scene_box_count = 0u;
static MDTBBox g_collision_boxes[MDTBMaxCollisionBoxes];
static size_t g_collision_box_count = 0u;
static int g_scene_initialized = 0;

static const float kPi = 3.1415926535f;
static const float kSegmentLength = 24.0f;
static const float kRoadHalfWidth = 5.8f;
static const float kCurbOuter = 6.35f;
static const float kSidewalkOuter = 12.0f;
static const float kPlayableHalfWidth = 58.0f;
static const float kPlayableHalfLength = 58.0f;
static const float kRoadHeight = 0.02f;
static const float kSidewalkHeight = 0.22f;
static const float kPlayerRadius = 0.34f;
static const float kEyeHeight = 1.68f;
static const float kIntersectionClear = 9.75f;
static const float kCollisionGap = 0.05f;

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

static void push_scene_box(MDTBBox box) {
    if (g_scene_box_count >= MDTBMaxSceneBoxes) {
        return;
    }

    g_scene_boxes[g_scene_box_count++] = box;
}

static void push_collision_box(MDTBBox box) {
    if (g_collision_box_count >= MDTBMaxCollisionBoxes) {
        return;
    }

    g_collision_boxes[g_collision_box_count++] = box;
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

static void push_lane_arrow_z(float z_center, float direction_sign) {
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.86f, 0.68f, 1.0f);

    push_scene_box(make_box(
        make_float3(0.0f, kRoadHeight + 0.01f, z_center - (direction_sign * 0.85f)),
        make_float3(0.20f, 0.01f, 1.05f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(-0.42f, kRoadHeight + 0.01f, z_center + (direction_sign * 0.34f)),
        make_float3(0.18f, 0.01f, 0.46f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(0.42f, kRoadHeight + 0.01f, z_center + (direction_sign * 0.34f)),
        make_float3(0.18f, 0.01f, 0.46f),
        stripe_color
    ));
}

static void push_lane_arrow_x(float x_center, float direction_sign) {
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.86f, 0.68f, 1.0f);

    push_scene_box(make_box(
        make_float3(x_center - (direction_sign * 0.85f), kRoadHeight + 0.01f, 0.0f),
        make_float3(1.05f, 0.01f, 0.20f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_center + (direction_sign * 0.34f), kRoadHeight + 0.01f, -0.42f),
        make_float3(0.46f, 0.01f, 0.18f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(x_center + (direction_sign * 0.34f), kRoadHeight + 0.01f, 0.42f),
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

static void push_carport_landmark(float x, float z) {
    const MDTBFloat4 beam_color = make_float4(0.55f, 0.52f, 0.44f, 1.0f);

    push_prop(make_float3(x - 2.9f, 1.35f, z - 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x + 2.9f, 1.35f, z - 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x - 2.9f, 1.35f, z + 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x + 2.9f, 1.35f, z + 1.8f), make_float3(0.14f, 1.35f, 0.14f), beam_color, 1);
    push_prop(make_float3(x, 2.95f, z), make_float3(3.25f, 0.18f, 2.15f), make_float4(0.72f, 0.69f, 0.58f, 1.0f), 0);
    push_parked_car(x, z, 1.28f, 2.45f, make_float4(0.42f, 0.37f, 0.58f, 1.0f));
}

static void push_crosswalk_x(float x_center) {
    for (int stripe_index = -3; stripe_index <= 3; ++stripe_index) {
        push_scene_box(make_box(
            make_float3(x_center, kRoadHeight + 0.01f, (float)stripe_index * 1.45f),
            make_float3(0.85f, 0.01f, 0.42f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
    }
}

static void push_crosswalk_z(float z_center) {
    for (int stripe_index = -3; stripe_index <= 3; ++stripe_index) {
        push_scene_box(make_box(
            make_float3((float)stripe_index * 1.45f, kRoadHeight + 0.01f, z_center),
            make_float3(0.42f, 0.01f, 0.85f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
    }
}

static void build_world_surfaces(void) {
    const MDTBFloat4 soil_color = make_float4(0.45f, 0.48f, 0.42f, 1.0f);
    const MDTBFloat4 road_color = make_float4(0.17f, 0.17f, 0.19f, 1.0f);
    const MDTBFloat4 curb_color = make_float4(0.74f, 0.74f, 0.73f, 1.0f);
    const MDTBFloat4 sidewalk_color = make_float4(0.62f, 0.62f, 0.59f, 1.0f);
    const MDTBFloat4 lot_color = make_float4(0.54f, 0.49f, 0.42f, 1.0f);

    push_scene_box(make_box(
        make_float3(0.0f, 0.0f, 0.0f),
        make_float3(kPlayableHalfWidth + 4.0f, 0.03f, kPlayableHalfLength + 4.0f),
        soil_color
    ));

    push_scene_box(make_box(
        make_float3(0.0f, kRoadHeight * 0.5f, 0.0f),
        make_float3(kRoadHalfWidth, kRoadHeight * 0.5f, kPlayableHalfLength + 4.0f),
        road_color
    ));

    push_scene_box(make_box(
        make_float3(0.0f, kRoadHeight * 0.5f, 0.0f),
        make_float3(kPlayableHalfWidth + 4.0f, kRoadHeight * 0.5f, kRoadHalfWidth),
        road_color
    ));

    for (int side = -1; side <= 1; side += 2) {
        push_scene_box(make_box(
            make_float3((float)side * 6.08f, 0.09f, 0.0f),
            make_float3(0.20f, 0.09f, kPlayableHalfLength + 4.0f),
            curb_color
        ));

        push_scene_box(make_box(
            make_float3(0.0f, 0.09f, (float)side * 6.08f),
            make_float3(kPlayableHalfWidth + 4.0f, 0.09f, 0.20f),
            curb_color
        ));
    }

    for (int x_sign = -1; x_sign <= 1; x_sign += 2) {
        for (int z_sign = -1; z_sign <= 1; z_sign += 2) {
            push_scene_box(make_box(
                make_float3((float)x_sign * 9.18f, kSidewalkHeight * 0.5f, (float)z_sign * 31.9f),
                make_float3(2.82f, kSidewalkHeight * 0.5f, 26.1f),
                sidewalk_color
            ));

            push_scene_box(make_box(
                make_float3((float)x_sign * 31.9f, kSidewalkHeight * 0.5f, (float)z_sign * 9.18f),
                make_float3(26.1f, kSidewalkHeight * 0.5f, 2.82f),
                sidewalk_color
            ));

            push_scene_box(make_box(
                make_float3((float)x_sign * 35.0f, kSidewalkHeight * 0.5f, (float)z_sign * 35.0f),
                make_float3(23.0f, kSidewalkHeight * 0.5f, 23.0f),
                lot_color
            ));
        }
    }
}

static void build_road_markings(void) {
    const MDTBFloat4 stripe_color = make_float4(0.89f, 0.82f, 0.20f, 1.0f);

    for (int segment = -2; segment <= 2; ++segment) {
        const float center = (float)segment * kSegmentLength;
        if (fabsf(center) > kIntersectionClear) {
            for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                push_scene_box(make_box(
                    make_float3(0.0f, kRoadHeight + 0.01f, center + ((float)stripe_index * 5.0f)),
                    make_float3(0.13f, 0.01f, 1.4f),
                    stripe_color
                ));
            }
        }

        if (fabsf(center) > kIntersectionClear) {
            for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                push_scene_box(make_box(
                    make_float3(center + ((float)stripe_index * 5.0f), kRoadHeight + 0.01f, 0.0f),
                    make_float3(1.4f, 0.01f, 0.13f),
                    stripe_color
                ));
            }
        }
    }

    push_crosswalk_x(-7.25f);
    push_crosswalk_x(7.25f);
    push_crosswalk_z(-7.25f);
    push_crosswalk_z(7.25f);

    push_scene_box(make_box(
        make_float3(0.0f, kRoadHeight + 0.01f, -6.65f),
        make_float3(4.65f, 0.01f, 0.12f),
        make_float4(0.92f, 0.91f, 0.87f, 1.0f)
    ));

    push_scene_box(make_box(
        make_float3(0.0f, kRoadHeight + 0.01f, 6.65f),
        make_float3(4.65f, 0.01f, 0.12f),
        make_float4(0.92f, 0.91f, 0.87f, 1.0f)
    ));

    push_scene_box(make_box(
        make_float3(-6.65f, kRoadHeight + 0.01f, 0.0f),
        make_float3(0.12f, 0.01f, 4.65f),
        make_float4(0.92f, 0.91f, 0.87f, 1.0f)
    ));

    push_scene_box(make_box(
        make_float3(6.65f, kRoadHeight + 0.01f, 0.0f),
        make_float3(0.12f, 0.01f, 4.65f),
        make_float4(0.92f, 0.91f, 0.87f, 1.0f)
    ));

    push_lane_arrow_z(-18.5f, -1.0f);
    push_lane_arrow_z(18.5f, 1.0f);
    push_lane_arrow_x(-18.5f, -1.0f);
    push_lane_arrow_x(18.5f, 1.0f);
}

static void build_quadrant_blocks(int x_sign, int z_sign, int variant) {
    static const MDTBFloat4 kFacadePalette[5] = {
        {0.58f, 0.44f, 0.35f, 1.0f},
        {0.39f, 0.50f, 0.62f, 1.0f},
        {0.43f, 0.55f, 0.38f, 1.0f},
        {0.62f, 0.54f, 0.34f, 1.0f},
        {0.51f, 0.40f, 0.47f, 1.0f},
    };

    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            const int palette_index = (variant + row + column) % 5;
            const float center_x = (float)x_sign * (18.0f + ((float)column * 15.0f));
            const float center_z = (float)z_sign * (18.0f + ((float)row * 15.0f));
            const float half_x = 2.3f + ((column % 2 == 0) ? 0.6f : 0.0f);
            const float half_y = 2.8f + (((variant + row) % 2 == 0) ? 0.9f : 0.2f);
            const float half_z = 3.0f + (((variant + column) % 2 == 0) ? 0.5f : 1.1f);

            push_building(
                make_float3(center_x, kSidewalkHeight + half_y, center_z),
                make_float3(half_x, half_y, half_z),
                kFacadePalette[palette_index]
            );
        }
    }

    push_planter((float)x_sign * 9.1f, (float)z_sign * 15.6f, 0.58f);
    push_planter((float)x_sign * 14.6f, (float)z_sign * 8.8f, 0.48f);
    push_tree((float)x_sign * 13.2f, (float)z_sign * 20.2f, 1.0f);
}

static void build_vertical_segment_props(float center_z, int variant) {
    const float z_offset = ((variant % 2 == 0) ? 5.0f : -5.0f);
    if (fabsf(center_z) < kIntersectionClear) {
        return;
    }

    push_bench(-8.9f, center_z - 6.4f);
    push_bench(8.9f, center_z + 5.8f);

    push_planter(-8.2f, center_z + 5.8f, 0.44f);
    push_planter(8.2f, center_z - 5.8f, 0.44f);

    push_tree(-10.4f, center_z + z_offset, 0.95f);
    push_tree(10.4f, center_z - z_offset, 0.95f);

    push_parked_car(-14.4f, center_z + 2.8f, 1.1f, 2.1f, make_float4(0.64f, 0.28f, 0.24f, 1.0f));
    push_parked_car(14.4f, center_z - 2.6f, 1.1f, 2.1f, make_float4(0.23f, 0.43f, 0.62f, 1.0f));
}

static void build_horizontal_segment_props(float center_x, int variant) {
    const float x_offset = ((variant % 2 == 0) ? 5.0f : -5.0f);
    if (fabsf(center_x) < kIntersectionClear) {
        return;
    }

    push_bench(center_x - 6.3f, -8.9f);
    push_bench(center_x + 5.7f, 8.9f);

    push_planter(center_x + 5.7f, -8.2f, 0.44f);
    push_planter(center_x - 5.7f, 8.2f, 0.44f);

    push_tree(center_x + x_offset, -10.4f, 0.95f);
    push_tree(center_x - x_offset, 10.4f, 0.95f);

    push_parked_car(center_x + 2.8f, -14.4f, 2.1f, 1.1f, make_float4(0.56f, 0.48f, 0.22f, 1.0f));
    push_parked_car(center_x - 2.6f, 14.4f, 2.1f, 1.1f, make_float4(0.33f, 0.55f, 0.37f, 1.0f));
}

static void build_intersection_props(void) {
    push_signal_pole(-8.9f, -8.9f);
    push_signal_pole(8.9f, -8.9f);
    push_signal_pole(-8.9f, 8.9f);
    push_signal_pole(8.9f, 8.9f);

    push_planter(-10.2f, -10.2f, 0.56f);
    push_planter(10.2f, -10.2f, 0.56f);
    push_planter(-10.2f, 10.2f, 0.56f);
    push_planter(10.2f, 10.2f, 0.56f);

    push_prop(
        make_float3(11.4f, 1.15f, -16.0f),
        make_float3(0.12f, 1.15f, 1.35f),
        make_float4(0.28f, 0.29f, 0.32f, 1.0f),
        1
    );

    push_prop(
        make_float3(10.8f, 2.15f, -16.0f),
        make_float3(0.82f, 0.16f, 1.55f),
        make_float4(0.56f, 0.61f, 0.68f, 1.0f),
        0
    );

    push_prop(
        make_float3(10.8f, 1.05f, -16.0f),
        make_float3(0.65f, 0.55f, 1.15f),
        make_float4(0.70f, 0.72f, 0.75f, 1.0f),
        1
    );

    push_prop(
        make_float3(-16.0f, 0.55f, 11.1f),
        make_float3(1.55f, 0.55f, 0.34f),
        make_float4(0.79f, 0.25f, 0.19f, 1.0f),
        1
    );

    push_prop(
        make_float3(16.0f, 0.55f, -11.1f),
        make_float3(1.55f, 0.55f, 0.34f),
        make_float4(0.23f, 0.49f, 0.67f, 1.0f),
        1
    );

    push_bollard(-11.9f, -7.4f, 0.92f);
    push_bollard(-11.9f, 7.4f, 0.92f);
    push_bollard(11.9f, -7.4f, 0.92f);
    push_bollard(11.9f, 7.4f, 0.92f);
    push_bollard(-7.4f, -11.9f, 0.92f);
    push_bollard(7.4f, -11.9f, 0.92f);
    push_bollard(-7.4f, 11.9f, 0.92f);
    push_bollard(7.4f, 11.9f, 0.92f);
}

static void build_landmarks(void) {
    push_corner_store_landmark(-42.0f, -17.2f);
    push_billboard(-23.5f, -45.5f, 5.4f, 1.45f, 0, make_float4(0.88f, 0.63f, 0.24f, 1.0f));

    push_half_court(35.0f, 35.0f);
    push_billboard(46.0f, 12.5f, 4.8f, 1.35f, 1, make_float4(0.29f, 0.53f, 0.67f, 1.0f));

    push_carport_landmark(39.5f, -37.5f);
    push_prop(
        make_float3(46.2f, kSidewalkHeight + 2.4f, -21.5f),
        make_float3(0.26f, 2.4f, 3.0f),
        make_float4(0.78f, 0.72f, 0.52f, 1.0f),
        1
    );
    push_prop(
        make_float3(44.8f, kSidewalkHeight + 2.2f, -21.5f),
        make_float3(1.1f, 2.2f, 0.18f),
        make_float4(0.92f, 0.46f, 0.22f, 1.0f),
        0
    );

    push_prop(
        make_float3(-40.5f, kSidewalkHeight + 1.6f, 38.0f),
        make_float3(5.6f, 1.6f, 0.28f),
        make_float4(0.36f, 0.44f, 0.56f, 1.0f),
        1
    );
    push_prop(
        make_float3(-44.3f, kSidewalkHeight + 2.8f, 37.4f),
        make_float3(0.20f, 2.8f, 0.20f),
        make_float4(0.38f, 0.37f, 0.34f, 1.0f),
        1
    );
    push_prop(
        make_float3(-44.3f, kSidewalkHeight + 4.8f, 37.4f),
        make_float3(1.45f, 0.16f, 0.16f),
        make_float4(0.82f, 0.78f, 0.58f, 1.0f),
        0
    );
}

static void build_main_route_frontage(void) {
    push_store_awning_x(-42.0f, -13.45f, 4.65f, 1.0f);
    push_newsstand(-45.5f, -11.25f, 1);
    push_trash_bin(-38.6f, -11.0f);
    push_prop(make_float3(-40.9f, 0.38f, -11.1f), make_float3(0.42f, 0.38f, 0.42f), make_float4(0.55f, 0.43f, 0.30f, 1.0f), 1);
    push_prop(make_float3(-39.8f, 0.62f, -11.0f), make_float3(0.34f, 0.62f, 0.28f), make_float4(0.19f, 0.35f, 0.56f, 1.0f), 1);

    for (int bollard = 0; bollard < 4; ++bollard) {
        push_bollard(-46.8f + ((float)bollard * 2.1f), -10.1f, 0.86f);
    }

    push_store_awning_x(-18.0f, -13.65f, 3.35f, 1.0f);
    push_store_awning_x(18.0f, -13.25f, 3.75f, 1.0f);
    push_store_awning_x(33.0f, -13.65f, 2.95f, 1.0f);
    push_newsstand(12.6f, -10.75f, 1);
    push_newsstand(28.9f, -10.65f, 1);
    push_trash_bin(-22.1f, -10.7f);
    push_trash_bin(36.5f, -10.8f);
    push_bench(23.9f, -10.3f);
    push_prop(make_float3(18.8f, 0.42f, -10.6f), make_float3(0.38f, 0.42f, 0.38f), make_float4(0.58f, 0.44f, 0.30f, 1.0f), 1);

    push_store_awning_x(-18.0f, 13.65f, 3.35f, -1.0f);
    push_store_awning_x(18.0f, 13.25f, 3.75f, -1.0f);
    push_store_awning_x(33.0f, 13.65f, 2.95f, -1.0f);
    push_newsstand(-13.1f, 10.7f, 1);
    push_newsstand(23.1f, 10.7f, 1);
    push_trash_bin(-22.3f, 10.85f);
    push_trash_bin(36.4f, 10.9f);
    push_bench(27.4f, 10.4f);
    push_prop(make_float3(17.8f, 0.46f, 10.8f), make_float3(0.34f, 0.46f, 0.34f), make_float4(0.26f, 0.44f, 0.29f, 1.0f), 1);

    push_bench(-10.7f, -31.5f);
    push_trash_bin(-13.0f, -32.4f);
    push_prop(make_float3(-11.2f, 2.05f, -34.0f), make_float3(0.12f, 2.05f, 0.12f), make_float4(0.36f, 0.37f, 0.39f, 1.0f), 1);
    push_prop(make_float3(-11.2f, 3.42f, -34.0f), make_float3(1.08f, 0.12f, 0.12f), make_float4(0.83f, 0.75f, 0.49f, 1.0f), 0);

    push_bench(9.8f, 31.7f);
    push_trash_bin(12.2f, 32.4f);
    push_prop(make_float3(10.8f, 1.28f, 34.0f), make_float3(0.12f, 1.28f, 1.05f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(10.1f, 2.34f, 34.0f), make_float3(0.96f, 0.10f, 1.28f), make_float4(0.67f, 0.78f, 0.82f, 1.0f), 0);
}

static void build_lot_edge_collision_detail(void) {
    push_low_fence_run_x(-47.6f, 28.0f, 50.2f);
    push_low_fence_run_z(28.0f, -47.6f, -28.6f);

    push_low_fence_run_x(46.3f, -50.0f, -29.6f);
    push_low_fence_run_z(-29.6f, 28.8f, 46.3f);

    push_low_fence_run_x(-29.0f, -50.2f, -35.6f);
    push_low_fence_run_z(-50.2f, -47.8f, -32.6f);
    push_low_fence_run_x(29.2f, 35.8f, 50.4f);
    push_low_fence_run_z(50.4f, 29.2f, 44.0f);
}

static void build_scene(void) {
    g_scene_box_count = 0u;
    g_collision_box_count = 0u;

    build_world_surfaces();
    build_road_markings();

    for (int segment_index = -2; segment_index <= 2; ++segment_index) {
        const float center = (float)segment_index * kSegmentLength;
        build_vertical_segment_props(center, segment_index + 2);
        build_horizontal_segment_props(center, segment_index + 3);
    }

    build_quadrant_blocks(-1, -1, 0);
    build_quadrant_blocks(1, -1, 1);
    build_quadrant_blocks(-1, 1, 2);
    build_quadrant_blocks(1, 1, 3);
    build_intersection_props();
    build_landmarks();
    build_main_route_frontage();
    build_lot_edge_collision_detail();

    g_scene_initialized = 1;
}

static void ensure_scene_initialized(void) {
    if (!g_scene_initialized) {
        build_scene();
    }
}

static MDTBGroundInfo ground_info(float x, float z) {
    MDTBGroundInfo info;
    const float abs_x = fabsf(x);
    const float abs_z = fabsf(z);
    const float distance_to_road = fminf(
        fmaxf(abs_x - kRoadHalfWidth, 0.0f),
        fmaxf(abs_z - kRoadHalfWidth, 0.0f)
    );

    if (abs_x <= kRoadHalfWidth || abs_z <= kRoadHalfWidth) {
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

static MDTBFloat3 actor_focus_position(const MDTBEngineState *state) {
    return make_float3(
        state->actor_position.x,
        state->actor_ground_height + kEyeHeight,
        state->actor_position.z
    );
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
    float resolved = proposed;

    for (size_t index = 0u; index < g_collision_box_count; ++index) {
        const MDTBBox *box = &g_collision_boxes[index];
        const float min_x = box->center.x - box->half_extents.x - kPlayerRadius - kCollisionGap;
        const float max_x = box->center.x + box->half_extents.x + kPlayerRadius + kCollisionGap;
        const float min_z = box->center.z - box->half_extents.z - kPlayerRadius - kCollisionGap;
        const float max_z = box->center.z + box->half_extents.z + kPlayerRadius + kCollisionGap;

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
    state->target_yaw = state->camera.yaw;
    state->target_pitch = state->camera.pitch;
    state->actor_heading = state->camera.yaw;
    state->surface_kind = MDTBSurfaceSidewalk;
    state->camera.focus_position = actor_focus_position(state);
    state->camera.position = state->camera.focus_position;
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
    const MDTBGroundInfo current_ground = ground_info(state->actor_position.x, state->actor_position.z);
    const float traversal_scale = surface_speed_multiplier(current_ground.surface_kind);
    const float base_speed = (input.buttons & MDTBInputSprint) != 0u ? 10.5f : 6.45f;
    const float max_speed = base_speed * traversal_scale;
    const float clamped_look_dx = clampf(input.look_delta_x, -80.0f, 80.0f);
    const float clamped_look_dy = clampf(input.look_delta_y, -80.0f, 80.0f);

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
    state->target_pitch = clampf(state->target_pitch, -0.52f, 0.38f);

    state->camera.yaw = approach_angle(state->camera.yaw, state->target_yaw, 12.5f, dt);
    state->camera.pitch = approachf(state->camera.pitch, state->target_pitch, 10.0f, dt);

    const float forward_x = sinf(state->target_yaw);
    const float forward_z = -cosf(state->target_yaw);
    const float right_x = cosf(state->target_yaw);
    const float right_z = sinf(state->target_yaw);

    float desired_velocity_x = 0.0f;
    float desired_velocity_z = 0.0f;

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

    const float move_length = sqrtf((desired_velocity_x * desired_velocity_x) + (desired_velocity_z * desired_velocity_z));
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

    float proposed_x = state->actor_position.x + (state->actor_velocity.x * dt);
    float proposed_z = state->actor_position.z + (state->actor_velocity.z * dt);

    const float resolved_x = resolve_collision_axis(state->actor_position.z, state->actor_position.x, proposed_x, 1);
    const float clamped_x = clampf(resolved_x, -kPlayableHalfWidth, kPlayableHalfWidth);
    if (fabsf(clamped_x - proposed_x) > 0.0001f) {
        state->actor_velocity.x = 0.0f;
    }
    state->actor_position.x = clamped_x;

    const float resolved_z = resolve_collision_axis(state->actor_position.x, state->actor_position.z, proposed_z, 0);
    const float clamped_z = clampf(resolved_z, -kPlayableHalfLength, kPlayableHalfLength);
    if (fabsf(clamped_z - proposed_z) > 0.0001f) {
        state->actor_velocity.z = 0.0f;
    }
    state->actor_position.z = clamped_z;

    const MDTBGroundInfo ground = ground_info(state->actor_position.x, state->actor_position.z);
    const float ground_follow_speed = ground.surface_kind == MDTBSurfaceCurb ? 16.0f : 24.0f;
    state->actor_ground_height = approachf(state->actor_ground_height, ground.height, ground_follow_speed, dt);
    state->actor_position.y = state->actor_ground_height;
    state->surface_kind = ground.surface_kind;
    state->camera.move_speed = sqrtf((state->actor_velocity.x * state->actor_velocity.x) + (state->actor_velocity.z * state->actor_velocity.z));

    const MDTBFloat3 actor_focus = actor_focus_position(state);

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

        const MDTBFloat3 focus_target = make_float3(
            actor_focus.x + (lead_direction.x * lead),
            actor_focus.y + 0.34f + (speed_ratio * 0.18f),
            actor_focus.z + (lead_direction.z * lead)
        );

        state->camera.focus_position = approach_float3(state->camera.focus_position, focus_target, 10.0f, dt);

        const float chase_distance = 6.6f + (speed_ratio * 1.35f);
        const float shoulder_offset = 1.18f;
        const float camera_height = 1.18f + (speed_ratio * 0.22f) + (fmaxf(-state->camera.pitch, 0.0f) * 0.65f);

        MDTBFloat3 desired_camera = make_float3(
            state->camera.focus_position.x - (flat_forward.x * chase_distance) + (shoulder_right.x * shoulder_offset),
            state->camera.focus_position.y + camera_height,
            state->camera.focus_position.z - (flat_forward.z * chase_distance) + (shoulder_right.z * shoulder_offset)
        );

        desired_camera = resolve_third_person_camera(state->camera.focus_position, desired_camera);
        state->camera.position = approach_float3(state->camera.position, desired_camera, 8.5f, dt);
    }

    state->elapsed_time += dt;
}

size_t mdtb_engine_box_count(void) {
    ensure_scene_initialized();
    return g_scene_box_count;
}

void mdtb_engine_copy_boxes(MDTBBox *boxes, size_t count) {
    ensure_scene_initialized();
    const size_t available = mdtb_engine_box_count();
    if (boxes == NULL || count == 0u) {
        return;
    }

    const size_t copy_count = count < available ? count : available;
    memcpy(boxes, g_scene_boxes, copy_count * sizeof(MDTBBox));
}
