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
    MDTBBox box;
    uint32_t block_index;
    uint32_t layer;
} MDTBSceneBox;

typedef struct {
    MDTBFloat3 origin;
    uint32_t kind;
    uint32_t variant;
    float activation_radius;
    uint32_t district;
    uint32_t tag_mask;
    uint32_t frontage_template;
    uint32_t chunk_index;
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
    float yaw;
    uint32_t block_index;
    uint32_t kind;
    uint32_t parking_state;
    uint32_t lane_axis;
    float lane_offset;
} MDTBVehicleAnchor;

typedef struct {
    MDTBFloat3 position;
    MDTBFloat3 half_extents;
    MDTBFloat4 color;
    float phase_offset;
    uint32_t kind;
    uint32_t block_index;
} MDTBDynamicProp;

typedef struct {
    uint32_t block_index;
    float pedestrian_density;
    float vehicle_density;
    float ambient_energy;
    float travel_bias;
    uint32_t style_flags;
} MDTBPopulationProfile;

typedef struct {
    MDTBFloat3 position;
    float radius;
    uint32_t block_index;
    uint32_t axis;
    uint32_t reason;
    float strength;
} MDTBTrafficOccupancy;

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
    uint32_t traversal_mode;
    uint32_t active_vehicle_anchor_index;
    uint32_t nearby_vehicle_anchor_index;
    uint32_t secondary_vehicle_anchor_index;
    uint32_t tertiary_vehicle_anchor_index;
    uint32_t locked_vehicle_anchor_index;
    uint32_t vehicle_selection_locked;
    uint32_t active_vehicle_kind;
    MDTBFloat3 active_vehicle_position;
    float active_vehicle_heading;
    float active_vehicle_speed;
    float active_vehicle_surface_grip;
    float active_vehicle_lane_error;
    float active_vehicle_collision_pulse;
    float active_vehicle_recovery;
    float active_vehicle_steer_visual;
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
    MDTBInputUse = 1u << 10,
    MDTBInputCycleHandoff = 1u << 11,
    MDTBInputToggleHandoffLock = 1u << 12,
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
    MDTBInterestPointHotspot = 4,
};

enum {
    MDTBIndexNone = 0xffffffffu,
};

enum {
    MDTBSceneLayerShared = 0,
    MDTBSceneLayerBlockOwned = 1,
};

enum {
    MDTBTraversalModeOnFoot = 0,
    MDTBTraversalModeVehicle = 1,
};

enum {
    MDTBWorldChunkWestGrid = 0,
    MDTBWorldChunkEastGrid = 1,
};

enum {
    MDTBRoadAxisNorthSouth = 0,
    MDTBRoadAxisEastWest = 1,
};

enum {
    MDTBFrontageTemplateCivicRetail = 0,
    MDTBFrontageTemplateResidentialCourt = 1,
    MDTBFrontageTemplateTransitMarket = 2,
    MDTBFrontageTemplateServiceSpur = 3,
};

enum {
    MDTBDynamicPropSignalLamp = 0,
    MDTBDynamicPropSwingSign = 1,
    MDTBDynamicPropPennant = 2,
    MDTBDynamicPropWindowGlow = 3,
    MDTBDynamicPropTransitGlow = 4,
    MDTBDynamicPropNeon = 5,
};

enum {
    MDTBPopulationStyleTransitHeavy = 1u << 0,
    MDTBPopulationStyleResidentialCalm = 1u << 1,
    MDTBPopulationStyleRetailClustered = 1u << 2,
    MDTBPopulationStyleThroughTraffic = 1u << 3,
};

enum {
    MDTBVehicleKindSedan = 0,
    MDTBVehicleKindCoupe = 1,
    MDTBVehicleKindMoped = 2,
    MDTBVehicleKindBicycle = 3,
    MDTBVehicleKindMotorcycle = 4,
};

enum {
    MDTBVehicleParkingStateCurbside = 0,
    MDTBVehicleParkingStateService = 1,
};

enum {
    MDTBTrafficOccupancyReasonStagedVehicle = 0,
    MDTBTrafficOccupancyReasonPlayerVehicle = 1,
    MDTBTrafficOccupancyReasonPedestrian = 2,
    MDTBTrafficOccupancyReasonStopZone = 3,
};

void mdtb_engine_init(MDTBEngineState *state);
void mdtb_engine_step(MDTBEngineState *state, MDTBInputFrame input);
size_t mdtb_engine_box_count(void);
void mdtb_engine_copy_boxes(MDTBBox *boxes, size_t count);
size_t mdtb_engine_scene_box_count(void);
void mdtb_engine_copy_scene_boxes(MDTBSceneBox *boxes, size_t count);
size_t mdtb_engine_block_count(void);
void mdtb_engine_copy_blocks(MDTBBlockDescriptor *blocks, size_t count);
size_t mdtb_engine_road_link_count(void);
void mdtb_engine_copy_road_links(MDTBRoadLink *links, size_t count);
size_t mdtb_engine_vehicle_anchor_count(void);
void mdtb_engine_copy_vehicle_anchors(MDTBVehicleAnchor *anchors, size_t count);
size_t mdtb_engine_interest_point_count(void);
void mdtb_engine_copy_interest_points(MDTBInterestPoint *points, size_t count);
size_t mdtb_engine_dynamic_prop_count(void);
void mdtb_engine_copy_dynamic_props(MDTBDynamicProp *props, size_t count);
size_t mdtb_engine_population_profile_count(void);
void mdtb_engine_copy_population_profiles(MDTBPopulationProfile *profiles, size_t count);
size_t mdtb_engine_traffic_occupancy_count(void);
void mdtb_engine_copy_traffic_occupancies(MDTBTrafficOccupancy *occupancies, size_t count);

#ifdef __cplusplus
}
#endif

#endif
