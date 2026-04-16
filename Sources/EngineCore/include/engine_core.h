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
    MDTBCamera camera;
    MDTBFloat3 actor_position;
    MDTBFloat3 actor_velocity;
    float actor_heading;
    float actor_ground_height;
    uint32_t surface_kind;
    float elapsed_time;
    float target_yaw;
    float target_pitch;
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

void mdtb_engine_init(MDTBEngineState *state);
void mdtb_engine_step(MDTBEngineState *state, MDTBInputFrame input);
size_t mdtb_engine_box_count(void);
void mdtb_engine_copy_boxes(MDTBBox *boxes, size_t count);

#ifdef __cplusplus
}
#endif

#endif
