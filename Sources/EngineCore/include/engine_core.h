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
    uint32_t road_class;
    uint32_t corridor;
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
    uint32_t territory_faction;
    uint32_t territory_phase;
    float territory_presence;
    float territory_heat;
    float territory_reentry_timer;
    uint32_t territory_entry_mode;
    float territory_watch_timer;
    float territory_front_watch;
    float territory_deep_watch;
    MDTBFloat3 territory_patrol_position;
    float territory_patrol_heading;
    uint32_t territory_patrol_state;
    float territory_patrol_alert;
    MDTBFloat3 territory_inner_position;
    float territory_inner_heading;
    uint32_t territory_inner_state;
    float territory_inner_alert;
    uint32_t territory_commit_state;
    float territory_commit_timer;
    float territory_commit_progress;
    uint32_t territory_resolve_state;
    float territory_resolve_timer;
    float territory_resolve_progress;
    uint32_t territory_reapproach_mode;
    float territory_reapproach_timer;
    float territory_preferred_side;
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
    uint32_t melee_weapon_owned;
    uint32_t melee_weapon_pickup_in_range;
    MDTBFloat3 melee_weapon_pickup_position;
    uint32_t melee_attack_phase;
    uint32_t melee_attack_connected;
    float melee_attack_timer;
    uint32_t firearm_owned;
    uint32_t firearm_pickup_in_range;
    MDTBFloat3 firearm_pickup_position;
    uint32_t equipped_weapon_kind;
    uint32_t firearm_clip_ammo;
    uint32_t firearm_reserve_ammo;
    uint32_t firearm_reloading;
    float firearm_reload_timer;
    float firearm_cooldown_timer;
    MDTBFloat3 firearm_last_shot_from;
    MDTBFloat3 firearm_last_shot_to;
    float firearm_last_shot_timer;
    uint32_t firearm_last_shot_hit;
    MDTBFloat3 combat_target_position;
    uint32_t combat_target_in_range;
    float combat_target_health;
    float combat_target_reaction;
    float combat_target_reset_timer;
    MDTBFloat3 combat_hostile_position;
    float combat_hostile_heading;
    uint32_t combat_hostile_in_range;
    float combat_hostile_health;
    float combat_hostile_reaction;
    float combat_hostile_reset_timer;
    float combat_hostile_alert;
    uint32_t combat_hostile_anchor_index;
    float combat_hostile_reposition_timer;
    float combat_hostile_reacquire_timer;
    MDTBFloat3 combat_hostile_search_position;
    float combat_hostile_search_timer;
    uint32_t combat_focus_target_kind;
    float combat_focus_distance;
    float combat_focus_alignment;
    uint32_t combat_last_hit_target_kind;
    uint32_t combat_focus_occluded;
    uint32_t combat_player_in_cover;
    float player_health;
    float player_recovery_delay;
    float player_damage_pulse;
    float player_reset_timer;
    MDTBFloat3 witness_position;
    float witness_heading;
    uint32_t witness_state;
    float witness_alert;
    float witness_state_timer;
    MDTBFloat3 bystander_position;
    float bystander_heading;
    uint32_t bystander_state;
    float bystander_alert;
    float bystander_state_timer;
    MDTBFloat3 street_incident_position;
    float street_incident_level;
    float street_incident_timer;
    MDTBFloat3 street_recovery_position;
    float street_recovery_level;
    float street_recovery_timer;
    float combat_hostile_attack_cooldown;
    float combat_hostile_attack_windup;
    MDTBFloat3 combat_hostile_last_shot_from;
    MDTBFloat3 combat_hostile_last_shot_to;
    float combat_hostile_last_shot_timer;
    uint32_t combat_hostile_last_shot_hit;
    uint32_t firearm_last_shot_blocked;
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
    MDTBInputPickupWeapon = 1u << 13,
    MDTBInputAttack = 1u << 14,
    MDTBInputEquipMeleeWeapon = 1u << 15,
    MDTBInputEquipFirearm = 1u << 16,
    MDTBInputReloadWeapon = 1u << 17,
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
    MDTBDistrictWestAdams = 0,
    MDTBDistrictJeffersonPark = 1,
    MDTBDistrictExpositionPark = 2,
    MDTBDistrictLeimertPark = 3,
    MDTBDistrictCrenshawCorridor = 4,
    MDTBDistrictHistoricSouthCentral = 5,
    MDTBDistrictVermontSquare = 6,
    MDTBDistrictFlorenceFirestone = 7,
    MDTBDistrictKoreatown = 8,
    MDTBDistrictPicoUnion = 9,
    MDTBDistrictUniversityPark = 10,
    MDTBDistrictSouthPark = 11,
    MDTBDistrictInglewood = 12,
    MDTBDistrictHuntingtonPark = 13,
    MDTBDistrictWatts = 14,
    MDTBDistrictWillowbrook = 15,
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
    MDTBWorldChunkKoreatownUniversity = 0,
    MDTBWorldChunkUniversityParkDowntown = 1,
    MDTBWorldChunkSouthParkIndustrial = 2,
    MDTBWorldChunkMidCityWest = 3,
    MDTBWorldChunkExpoCrenshaw = 4,
    MDTBWorldChunkCentralSouth = 5,
    MDTBWorldChunkLeimertBaldwin = 6,
    MDTBWorldChunkFlorenceVermont = 7,
    MDTBWorldChunkWattsWillowbrook = 8,
};

enum {
    MDTBRoadAxisNorthSouth = 0,
    MDTBRoadAxisEastWest = 1,
};

enum {
    MDTBRoadClassResidentialStreet = 0,
    MDTBRoadClassAvenue = 1,
    MDTBRoadClassBoulevard = 2,
    MDTBRoadClassConnector = 3,
};

enum {
    MDTBCorridorFairfaxAve = 0,
    MDTBCorridorLaBreaAve = 1,
    MDTBCorridorCrenshawBlvd = 2,
    MDTBCorridorArlingtonAve = 3,
    MDTBCorridorWesternAve = 4,
    MDTBCorridorVermontAve = 5,
    MDTBCorridorFigueroaSt = 6,
    MDTBCorridorCentralAve = 7,
    MDTBCorridorPicoBlvd = 8,
    MDTBCorridorWashingtonBlvd = 9,
    MDTBCorridorAdamsBlvd = 10,
    MDTBCorridorJeffersonBlvd = 11,
    MDTBCorridorExpositionBlvd = 12,
    MDTBCorridorMartinLutherKingBlvd = 13,
    MDTBCorridorSlausonAve = 14,
    MDTBCorridorFlorenceAve = 15,
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
    MDTBTrafficOccupancyReasonIncident = 4,
};

enum {
    MDTBMeleeAttackIdle = 0,
    MDTBMeleeAttackWindup = 1,
    MDTBMeleeAttackStrike = 2,
    MDTBMeleeAttackRecovery = 3,
};

enum {
    MDTBEquippedWeaponNone = 0,
    MDTBEquippedWeaponLeadPipe = 1,
    MDTBEquippedWeaponPistol = 2,
};

enum {
    MDTBCombatTargetNone = 0,
    MDTBCombatTargetDummy = 1,
    MDTBCombatTargetLookout = 2,
};

enum {
    MDTBWitnessStateIdle = 0,
    MDTBWitnessStateInvestigate = 1,
    MDTBWitnessStateFlee = 2,
    MDTBWitnessStateCooldown = 3,
};

enum {
    MDTBTerritoryFactionNone = 0,
    MDTBTerritoryFactionCourtSet = 1,
};

enum {
    MDTBTerritoryPhaseNone = 0,
    MDTBTerritoryPhaseBoundary = 1,
    MDTBTerritoryPhaseClaimed = 2,
    MDTBTerritoryPhaseHot = 3,
};

enum {
    MDTBTerritoryEntryNone = 0,
    MDTBTerritoryEntryOnFoot = 1,
    MDTBTerritoryEntryVehicle = 2,
};

enum {
    MDTBTerritoryPatrolIdle = 0,
    MDTBTerritoryPatrolWatch = 1,
    MDTBTerritoryPatrolHandoff = 2,
    MDTBTerritoryPatrolCooldown = 3,
    MDTBTerritoryPatrolScreen = 4,
    MDTBTerritoryPatrolClear = 5,
    MDTBTerritoryPatrolReform = 6,
    MDTBTerritoryPatrolBrace = 7,
};

enum {
    MDTBTerritoryCommitNone = 0,
    MDTBTerritoryCommitWindow = 1,
    MDTBTerritoryCommitActive = 2,
    MDTBTerritoryCommitComplete = 3,
};

enum {
    MDTBTerritoryResolveNone = 0,
    MDTBTerritoryResolveWindow = 1,
    MDTBTerritoryResolveHold = 2,
    MDTBTerritoryResolvePullout = 3,
};

enum {
    MDTBTerritoryReapproachNone = 0,
    MDTBTerritoryReapproachReclaim = 1,
    MDTBTerritoryReapproachRetake = 2,
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
