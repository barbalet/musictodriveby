#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float y;
    float z;
} MDTBFloat3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} MDTBFloat4;

typedef struct {
    MDTBFloat3 focus_position;
    MDTBFloat3 position;
    float yaw;
    float pitch;
    float move_speed;
    uint32_t mode;
} MDTBCamera;

typedef struct {
    MDTBFloat3 center;
    MDTBFloat3 half_extents;
    MDTBFloat4 color;
} MDTBBox;

typedef struct {
    MDTBFloat3 origin;
    uint32_t kind;
    uint32_t variant;
    float activation_radius;
    uint32_t district;
    uint32_t tag_mask;
} MDTBBlockDescriptor;

typedef struct {
    MDTBFloat3 position;
    float radius;
    uint32_t kind;
    uint32_t block_index;
} MDTBInterestPoint;

typedef struct {
    uint32_t from_block_index;
    uint32_t to_block_index;
    MDTBFloat3 midpoint;
    float length;
    uint32_t axis;
} MDTBRoadLink;

typedef struct {
    MDTBFloat3 position;
    MDTBFloat3 half_extents;
    MDTBFloat4 color;
    float phase_offset;
    uint32_t kind;
    uint32_t block_index;
} MDTBDynamicProp;

typedef struct {
    MDTBCamera camera;
    MDTBFloat3 actor_position;
    MDTBFloat3 actor_velocity;
    float actor_heading;
    float actor_ground_height;
    uint32_t surface_kind;
    float elapsed_time;
    float target_yaw;
    float target_pitch;
    uint32_t active_block_index;
    uint32_t nearby_block_count;
    uint32_t active_link_index;
    uint32_t active_pedestrian_spawn_count;
    uint32_t active_vehicle_spawn_count;
} MDTBEngineState;

typedef struct {
    float delta_time;
    float look_delta_x;
    float look_delta_y;
    uint32_t buttons;
} MDTBInputFrame;

enum {
    MDTBInputMoveForward = 1u << 0,
    MDTBInputMoveBackward = 1u << 1,
    MDTBInputMoveLeft = 1u << 2,
    MDTBInputMoveRight = 1u << 3,
    MDTBInputTurnLeft = 1u << 4,
    MDTBInputTurnRight = 1u << 5,
    MDTBInputLookUp = 1u << 6,
    MDTBInputLookDown = 1u << 7,
    MDTBInputSprint = 1u << 8,
    MDTBInputToggleCamera = 1u << 9,
};

enum {
    MDTBCameraModeFirstPerson = 0,
    MDTBCameraModeThirdPerson = 1,
};

enum {
    MDTBSurfaceRoad = 0,
    MDTBSurfaceCurb = 1,
    MDTBSurfaceSidewalk = 2,
    MDTBSurfaceLot = 3,
};

enum {
    MDTBBlockKindHub = 0,
    MDTBBlockKindResidential = 1,
    MDTBBlockKindMixedUse = 2,
};

enum {
    MDTBDistrictSouthHub = 0,
    MDTBDistrictMapleHeights = 1,
    MDTBDistrictMarketSpur = 2,
};

enum {
    MDTBBlockTagRetail = 1u << 0,
    MDTBBlockTagTransit = 1u << 1,
    MDTBBlockTagLandmark = 1u << 2,
    MDTBBlockTagCourt = 1u << 3,
    MDTBBlockTagResidential = 1u << 4,
    MDTBBlockTagSpur = 1u << 5,
};

enum {
    MDTBInterestPointPedestrianSpawn = 0,
    MDTBInterestPointVehicleSpawn = 1,
    MDTBInterestPointLandmark = 2,
    MDTBInterestPointStreamingAnchor = 3,
};

enum {
    MDTBIndexNone = 0xffffffffu,
};

enum {
    MDTBRoadAxisNorthSouth = 0,
    MDTBRoadAxisEastWest = 1,
};

enum {
    MDTBDynamicPropSignalLamp = 0,
    MDTBDynamicPropSwingSign = 1,
    MDTBDynamicPropPennant = 2,
    MDTBDynamicPropWindowGlow = 3,
    MDTBDynamicPropTransitGlow = 4,
    MDTBDynamicPropNeon = 5,
};

void mdtb_engine_init(MDTBEngineState *state);
void mdtb_engine_step(MDTBEngineState *state, MDTBInputFrame input);
size_t mdtb_engine_box_count(void);
void mdtb_engine_copy_boxes(MDTBBox *boxes, size_t count);
size_t mdtb_engine_block_count(void);
void mdtb_engine_copy_blocks(MDTBBlockDescriptor *blocks, size_t count);
size_t mdtb_engine_road_link_count(void);
void mdtb_engine_copy_road_links(MDTBRoadLink *links, size_t count);
size_t mdtb_engine_interest_point_count(void);
void mdtb_engine_copy_interest_points(MDTBInterestPoint *points, size_t count);
size_t mdtb_engine_dynamic_prop_count(void);
void mdtb_engine_copy_dynamic_props(MDTBDynamicProp *props, size_t count);

#ifdef __cplusplus
}
#endif

#endif
