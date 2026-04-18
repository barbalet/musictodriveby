#include "engine_core.h"

#include <math.h>
#include <string.h>

enum {
    MDTBMaxSceneBoxes = 16384,
    MDTBMaxCollisionBoxes = 6144,
    MDTBMaxBlocks = 32,
    MDTBMaxRoadLinks = 128,
    MDTBMaxRoadSpines = 16,
    MDTBMaxVehicleAnchors = 64,
    MDTBMaxInterestPoints = 320,
    MDTBMaxDynamicProps = 768,
    MDTBMaxPopulationProfiles = MDTBMaxBlocks,
    MDTBMaxTrafficOccupancies = 96,
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

typedef struct {
    float coordinate;
    uint32_t axis;
    uint32_t road_class;
    uint32_t corridor;
} MDTBRoadSpine;

typedef struct {
    float road_half_width;
    float curb_outer;
    float sidewalk_outer;
    float lane_offset;
    float median_half_width;
} MDTBRoadProfile;

typedef struct {
    uint32_t chunk_index;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float corner_plaza_scale;
    float planter_scale;
    float refuge_scale;
} MDTBWorldChunkDescriptor;

typedef struct {
    uint32_t corridor;
    uint32_t axis;
    uint32_t road_class;
    float coordinate;
    float crosswalk_offset;
    float stop_bar_offset;
    float arrow_offset;
    float signal_offset;
    float planter_offset;
    float plaza_scale;
} MDTBCorridorDescriptor;

typedef struct {
    const MDTBCorridorDescriptor *vertical;
    const MDTBCorridorDescriptor *horizontal;
    const MDTBWorldChunkDescriptor *chunk;
} MDTBIntersectionProfile;

typedef struct {
    uint32_t frontage_template;
    float shopfront_z;
    float furniture_z;
    float transit_stop_z;
    float rear_anchor_z;
    float rear_fence_z;
    float planter_size;
    float primary_awning_scale;
    float secondary_awning_scale;
    float loading_zone_z;
    float loading_zone_half_z;
    float loading_zone_half_x_scale;
} MDTBFrontageDescriptor;

typedef struct {
    const MDTBFrontageDescriptor *descriptor;
    MDTBIntersectionProfile intersection;
    float shopfront_z;
    float furniture_z;
    float transit_stop_z;
    float rear_anchor_z;
    float rear_fence_z;
    float planter_size;
    float primary_awning_scale;
    float secondary_awning_scale;
    float loading_zone_z;
    float loading_zone_half_z;
    float loading_zone_half_x_scale;
} MDTBFrontageProfile;

static MDTBSceneBox g_scene_boxes[MDTBMaxSceneBoxes];
static size_t g_scene_box_count = 0u;
static MDTBBox g_collision_boxes[MDTBMaxCollisionBoxes];
static size_t g_collision_box_count = 0u;
static MDTBBlockDescriptor g_blocks[MDTBMaxBlocks];
static size_t g_block_count = 0u;
static MDTBRoadLink g_road_links[MDTBMaxRoadLinks];
static size_t g_road_link_count = 0u;
static MDTBRoadSpine g_road_spines[MDTBMaxRoadSpines];
static size_t g_road_spine_count = 0u;
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
static void rebuild_road_spines(void);
static const MDTBRoadSpine *road_spine_for_axis_coordinate(uint32_t axis, float coordinate);
static int road_spine_segment_hits_intersection(const MDTBRoadSpine *spine, float segment_center);
static const MDTBRoadProfile *road_profile_for_class(uint32_t road_class);
static const MDTBRoadProfile *road_profile_for_spine(const MDTBRoadSpine *spine);
static const MDTBRoadProfile *road_profile_for_link(const MDTBRoadLink *link);
static const MDTBWorldChunkDescriptor *world_chunk_descriptor_for_index(uint32_t chunk_index);
static const MDTBCorridorDescriptor *corridor_descriptor_for_axis_coordinate(uint32_t axis, float coordinate);
static MDTBIntersectionProfile intersection_profile_for_block(const MDTBBlockDescriptor *block);
static const MDTBFrontageDescriptor *frontage_descriptor_for_template(uint32_t frontage_template);
static MDTBFrontageProfile frontage_profile_for_block(const MDTBBlockDescriptor *block);
static MDTBFloat4 road_surface_color_for_class(uint32_t road_class);
static MDTBFloat4 curb_surface_color_for_class(uint32_t road_class);
static MDTBFloat4 sidewalk_surface_color_for_class(uint32_t road_class);
static MDTBFloat3 view_forward(float yaw, float pitch);
static MDTBFloat3 normalize_flat(MDTBFloat3 value);
static MDTBFloat3 vehicle_forward_flat(float heading);
static MDTBFloat3 vehicle_right_flat(float heading);
static MDTBFloat3 actor_focus_position(const MDTBEngineState *state);
static MDTBFloat3 player_cover_position(const MDTBEngineState *state);
static uint32_t choose_lookout_pressure_anchor(const MDTBEngineState *state);
static MDTBFloat3 lookout_anchor_position(uint32_t anchor_index);
static float territory_preferred_side_bias(const MDTBEngineState *state);
static float territory_preferred_side_strength(const MDTBEngineState *state);
static MDTBFloat3 territory_shoulder_focus_position(const MDTBEngineState *state, MDTBFloat3 position, float lateral_offset, float depth_offset);
static int territory_descriptor_for_position(MDTBFloat3 position, uint32_t *faction_out, uint32_t *phase_out, float *presence_out);
static void step_territory_state(MDTBEngineState *state, float dt);
static void start_hostile_search(MDTBEngineState *state, MDTBFloat3 position, float duration);
static void trigger_street_incident(MDTBEngineState *state, MDTBFloat3 position, float level, float duration);
static void trigger_street_recovery(MDTBEngineState *state, MDTBFloat3 position, float level, float duration);
static MDTBFloat3 witness_target_position(uint32_t witness_state);
static MDTBFloat3 bystander_target_position(uint32_t bystander_state);
static int active_civilian_response_count(const MDTBEngineState *state);
static int segment_hits_world_cover(MDTBFloat3 start, MDTBFloat3 end, float padding, MDTBFloat3 *impact_out);
static int position_overlaps_collision(MDTBFloat3 position, float radius);
static void reset_combat_encounter(MDTBEngineState *state);
static void refresh_combat_proximity(MDTBEngineState *state);
static void step_combat_state(MDTBEngineState *state, float dt, int wants_attack, int wants_reload);

static const float kPi = 3.1415926535f;
static const float kRoadHalfWidth = 5.8f;
static const float kCurbOuter = 6.35f;
static const float kSidewalkOuter = 12.0f;
static const float kPlayableHalfWidth = 360.0f;
static const float kPlayableHalfLength = 288.0f;
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
static const float kLookoutAttackRange = 16.0f;
static const float kLookoutAttackCooldown = 1.55f;
static const float kLookoutAttackBlockedCooldown = 1.95f;
static const float kLookoutAttackWindupDuration = 0.42f;
static const float kLookoutShotFlashDuration = 0.14f;
static const float kLookoutShotDamage = 18.0f;
static const float kLookoutRepositionRetargetDelay = 1.15f;
static const float kLookoutReacquireDuration = 0.72f;
static const float kLookoutVehicleReacquireDuration = 1.10f;
static const float kLookoutSearchDuration = 2.2f;
static const uint32_t kPistolClipCapacity = 6u;
static const uint32_t kPistolInitialReserveAmmo = 18u;
static const float kPistolPickupRadius = 1.45f;
static const float kPistolRange = 20.0f;
static const float kPistolAimDot = 0.90f;
static const float kPistolShotCooldown = 0.24f;
static const float kPistolReloadDuration = 1.15f;
static const float kPistolShotFlashDuration = 0.10f;
static const float kPlayerMaxHealth = 100.0f;
static const float kPlayerRecoveryDelay = 1.85f;
static const float kPlayerRecoveryRate = 14.0f;
static const float kPlayerResetGraceDuration = 1.55f;
static const MDTBFloat3 kCombatLaneResetPosition = {-4.0f, kSidewalkHeight, 46.4f};
static const MDTBFloat3 kWitnessBasePosition = {-19.2f, kSidewalkHeight, 44.2f};
static const MDTBFloat3 kWitnessInvestigatePosition = {-15.0f, kSidewalkHeight, 48.5f};
static const MDTBFloat3 kWitnessFleePosition = {-28.4f, kSidewalkHeight, 43.4f};
static const float kWitnessNoticeRadius = 22.0f;
static const float kWitnessPanicRadius = 12.5f;
static const float kWitnessInvestigateDuration = 2.8f;
static const float kWitnessFleeDuration = 4.2f;
static const float kWitnessCooldownDuration = 5.0f;
static const MDTBFloat3 kBystanderBasePosition = {18.8f, kSidewalkHeight, 45.8f};
static const MDTBFloat3 kBystanderInvestigatePosition = {12.6f, kSidewalkHeight, 49.2f};
static const MDTBFloat3 kBystanderFleePosition = {28.6f, kSidewalkHeight, 39.8f};
static const float kBystanderNoticeRadius = 30.0f;
static const float kBystanderPanicRadius = 22.0f;
static const float kBystanderInvestigateDuration = 3.2f;
static const float kBystanderFleeDuration = 4.8f;
static const float kBystanderCooldownDuration = 5.8f;
static const float kStreetIncidentNoticeDuration = 2.4f;
static const float kStreetIncidentSearchDuration = 3.6f;
static const float kStreetIncidentFleeDuration = 5.8f;
static const float kStreetIncidentSettleDuration = 2.1f;
static const float kStreetIncidentNormalizationBoost = 1.55f;
static const float kStreetIncidentVehicleNormalizationBoost = 2.20f;
static const float kStreetRecoveryDuration = 3.4f;
static const float kLookoutSearchSettleDuration = 0.84f;
static const float kCourtSetTerritoryMinX = -32.0f;
static const float kCourtSetTerritoryMaxX = 20.0f;
static const float kCourtSetTerritoryEntryZ = 33.5f;
static const float kCourtSetTerritoryCoreZ = 43.5f;
static const float kCourtSetTerritoryMaxZ = 67.5f;
static const float kTerritoryWatchCarryDuration = 3.6f;
static const float kTerritoryReapproachReclaimDuration = 4.8f;
static const float kTerritoryReapproachRetakeDuration = 4.2f;
static const MDTBFloat3 kTerritoryPatrolBasePosition = {-26.2f, kSidewalkHeight, 30.2f};
static const MDTBFloat3 kTerritoryPatrolLinePosition = {-21.8f, kSidewalkHeight, 35.8f};
static const MDTBFloat3 kTerritoryPatrolHandoffPosition = {-17.4f, kSidewalkHeight, 40.4f};
static const MDTBFloat3 kTerritoryInnerBasePosition = {14.6f, kSidewalkHeight, 58.6f};
static const MDTBFloat3 kTerritoryInnerReceivePosition = {10.6f, kSidewalkHeight, 55.8f};
static const float kTerritoryPatrolSidewalkRadius = 10.5f;
static const MDTBFloat3 kLookoutPressureAnchors[] = {
    {-13.8f, kSidewalkHeight, 52.0f},
    {-1.9f, kSidewalkHeight, 55.0f},
    {9.4f, kSidewalkHeight, 49.7f},
    {5.4f, kSidewalkHeight, 56.6f},
};

static const MDTBBlockDescriptor kBlockLayout[] = {
    {{0.0f, 0.0f, 0.0f}, MDTBBlockKindHub, 0u, 58.0f, MDTBDistrictWestAdams, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateCivicRetail, MDTBWorldChunkMidCityWest},
    {{0.0f, 0.0f, 72.0f}, MDTBBlockKindResidential, 1u, 56.0f, MDTBDistrictWestAdams, MDTBBlockTagResidential | MDTBBlockTagTransit | MDTBBlockTagCourt, MDTBFrontageTemplateResidentialCourt, MDTBWorldChunkMidCityWest},
    {{0.0f, 0.0f, 144.0f}, MDTBBlockKindResidential, 2u, 56.0f, MDTBDistrictLeimertPark, MDTBBlockTagResidential | MDTBBlockTagLandmark, MDTBFrontageTemplateResidentialCourt, MDTBWorldChunkExpoCrenshaw},
    {{0.0f, 0.0f, 216.0f}, MDTBBlockKindMixedUse, 3u, 58.0f, MDTBDistrictLeimertPark, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkExpoCrenshaw},
    {{96.0f, 0.0f, 0.0f}, MDTBBlockKindMixedUse, 4u, 58.0f, MDTBDistrictJeffersonPark, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkMidCityWest},
    {{96.0f, 0.0f, 72.0f}, MDTBBlockKindMixedUse, 5u, 58.0f, MDTBDistrictExpositionPark, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkMidCityWest},
    {{96.0f, 0.0f, 144.0f}, MDTBBlockKindResidential, 6u, 56.0f, MDTBDistrictCrenshawCorridor, MDTBBlockTagResidential | MDTBBlockTagTransit | MDTBBlockTagCourt, MDTBFrontageTemplateResidentialCourt, MDTBWorldChunkExpoCrenshaw},
    {{96.0f, 0.0f, 216.0f}, MDTBBlockKindMixedUse, 7u, 58.0f, MDTBDistrictCrenshawCorridor, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkExpoCrenshaw},
    {{192.0f, 0.0f, 0.0f}, MDTBBlockKindMixedUse, 8u, 58.0f, MDTBDistrictHistoricSouthCentral, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagSpur, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkCentralSouth},
    {{192.0f, 0.0f, 72.0f}, MDTBBlockKindMixedUse, 9u, 58.0f, MDTBDistrictHistoricSouthCentral, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark | MDTBBlockTagCourt, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkCentralSouth},
    {{192.0f, 0.0f, 144.0f}, MDTBBlockKindMixedUse, 10u, 58.0f, MDTBDistrictHistoricSouthCentral, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagSpur, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkFlorenceVermont},
    {{192.0f, 0.0f, 216.0f}, MDTBBlockKindMixedUse, 11u, 58.0f, MDTBDistrictFlorenceFirestone, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagSpur | MDTBBlockTagCourt, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkFlorenceVermont},
    {{288.0f, 0.0f, 0.0f}, MDTBBlockKindMixedUse, 12u, 58.0f, MDTBDistrictVermontSquare, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagLandmark, MDTBFrontageTemplateTransitMarket, MDTBWorldChunkCentralSouth},
    {{288.0f, 0.0f, 72.0f}, MDTBBlockKindResidential, 13u, 56.0f, MDTBDistrictVermontSquare, MDTBBlockTagResidential | MDTBBlockTagTransit, MDTBFrontageTemplateResidentialCourt, MDTBWorldChunkCentralSouth},
    {{288.0f, 0.0f, 144.0f}, MDTBBlockKindMixedUse, 14u, 58.0f, MDTBDistrictFlorenceFirestone, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagSpur | MDTBBlockTagLandmark, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkFlorenceVermont},
    {{288.0f, 0.0f, 216.0f}, MDTBBlockKindMixedUse, 15u, 58.0f, MDTBDistrictFlorenceFirestone, MDTBBlockTagRetail | MDTBBlockTagTransit | MDTBBlockTagSpur | MDTBBlockTagCourt, MDTBFrontageTemplateServiceSpur, MDTBWorldChunkFlorenceVermont},
};

static const MDTBRoadLink kRoadLayout[] = {
    {0u, 1u, {0.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassBoulevard, MDTBCorridorCrenshawBlvd},
    {1u, 2u, {0.0f, 0.22f, 108.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassBoulevard, MDTBCorridorCrenshawBlvd},
    {2u, 3u, {0.0f, 0.22f, 180.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassBoulevard, MDTBCorridorCrenshawBlvd},
    {4u, 5u, {96.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorArlingtonAve},
    {5u, 6u, {96.0f, 0.22f, 108.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorArlingtonAve},
    {6u, 7u, {96.0f, 0.22f, 180.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorArlingtonAve},
    {8u, 9u, {192.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorWesternAve},
    {9u, 10u, {192.0f, 0.22f, 108.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorWesternAve},
    {10u, 11u, {192.0f, 0.22f, 180.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorWesternAve},
    {12u, 13u, {288.0f, 0.22f, 36.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorVermontAve},
    {13u, 14u, {288.0f, 0.22f, 108.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorVermontAve},
    {14u, 15u, {288.0f, 0.22f, 180.0f}, 72.0f, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, MDTBCorridorVermontAve},
    {0u, 4u, {48.0f, 0.22f, 0.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorAdamsBlvd},
    {4u, 8u, {144.0f, 0.22f, 0.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorAdamsBlvd},
    {8u, 12u, {240.0f, 0.22f, 0.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorAdamsBlvd},
    {1u, 5u, {48.0f, 0.22f, 72.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorJeffersonBlvd},
    {5u, 9u, {144.0f, 0.22f, 72.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorJeffersonBlvd},
    {9u, 13u, {240.0f, 0.22f, 72.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorJeffersonBlvd},
    {2u, 6u, {48.0f, 0.22f, 144.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorExpositionBlvd},
    {6u, 10u, {144.0f, 0.22f, 144.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorExpositionBlvd},
    {10u, 14u, {240.0f, 0.22f, 144.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorExpositionBlvd},
    {3u, 7u, {48.0f, 0.22f, 216.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorMartinLutherKingBlvd},
    {7u, 11u, {144.0f, 0.22f, 216.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorMartinLutherKingBlvd},
    {11u, 15u, {240.0f, 0.22f, 216.0f}, 96.0f, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, MDTBCorridorMartinLutherKingBlvd},
};

static const MDTBWorldChunkDescriptor kWorldChunkLayout[] = {
    {MDTBWorldChunkMidCityWest, -48.0f, 144.0f, -36.0f, 108.0f, 1.16f, 1.10f, 1.02f},
    {MDTBWorldChunkCentralSouth, 144.0f, 336.0f, -36.0f, 108.0f, 0.98f, 0.92f, 0.90f},
    {MDTBWorldChunkExpoCrenshaw, -48.0f, 144.0f, 108.0f, 252.0f, 1.12f, 1.16f, 1.08f},
    {MDTBWorldChunkFlorenceVermont, 144.0f, 336.0f, 108.0f, 252.0f, 0.94f, 0.88f, 0.96f},
};

static const MDTBCorridorDescriptor kCorridorLayout[] = {
    {MDTBCorridorCrenshawBlvd, MDTBRoadAxisNorthSouth, MDTBRoadClassBoulevard, 0.0f, 7.60f, 6.90f, 20.9f, 9.55f, 10.95f, 1.22f},
    {MDTBCorridorArlingtonAve, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, 96.0f, 6.95f, 6.45f, 18.8f, 8.90f, 10.05f, 1.00f},
    {MDTBCorridorWesternAve, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, 192.0f, 7.05f, 6.55f, 19.2f, 9.05f, 10.20f, 1.04f},
    {MDTBCorridorVermontAve, MDTBRoadAxisNorthSouth, MDTBRoadClassAvenue, 288.0f, 7.15f, 6.65f, 19.5f, 9.15f, 10.35f, 1.08f},
    {MDTBCorridorAdamsBlvd, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, 0.0f, 7.45f, 6.90f, 20.4f, 9.20f, 10.65f, 1.18f},
    {MDTBCorridorJeffersonBlvd, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, 72.0f, 7.55f, 7.00f, 20.7f, 9.35f, 10.85f, 1.22f},
    {MDTBCorridorExpositionBlvd, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, 144.0f, 7.65f, 7.10f, 21.0f, 9.55f, 11.05f, 1.26f},
    {MDTBCorridorMartinLutherKingBlvd, MDTBRoadAxisEastWest, MDTBRoadClassBoulevard, 216.0f, 7.75f, 7.20f, 21.2f, 9.70f, 11.20f, 1.30f},
};

static const MDTBFrontageDescriptor kFrontageLayout[] = {
    {MDTBFrontageTemplateCivicRetail, 13.45f, 10.75f, 15.9f, 33.8f, 46.5f, 0.58f, 1.08f, 1.02f, 4.65f, 0.54f, 1.00f},
    {MDTBFrontageTemplateResidentialCourt, 13.40f, 10.70f, 16.1f, 33.9f, 46.2f, 0.50f, 0.94f, 0.98f, 4.65f, 0.54f, 1.00f},
    {MDTBFrontageTemplateTransitMarket, 13.30f, 10.65f, 16.0f, 33.5f, 46.0f, 0.60f, 1.02f, 1.00f, 4.65f, 0.54f, 1.00f},
    {MDTBFrontageTemplateServiceSpur, 13.25f, 10.70f, 16.3f, 32.9f, 45.9f, 0.56f, 0.98f, 0.95f, 4.65f, 0.54f, 1.08f},
};

static const MDTBPopulationProfile kPopulationProfileLayout[] = {
    {0u, 0.82f, 0.60f, 0.76f, 0.56f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {1u, 0.58f, 0.40f, 0.48f, 0.40f, MDTBPopulationStyleResidentialCalm},
    {2u, 0.52f, 0.34f, 0.42f, 0.34f, MDTBPopulationStyleResidentialCalm},
    {3u, 0.70f, 0.54f, 0.68f, 0.58f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {4u, 0.80f, 0.64f, 0.78f, 0.62f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {5u, 0.78f, 0.62f, 0.82f, 0.64f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {6u, 0.56f, 0.42f, 0.52f, 0.46f, MDTBPopulationStyleResidentialCalm | MDTBPopulationStyleTransitHeavy},
    {7u, 0.74f, 0.60f, 0.74f, 0.66f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {8u, 0.72f, 0.76f, 0.80f, 0.88f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
    {9u, 0.70f, 0.78f, 0.82f, 0.92f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
    {10u, 0.66f, 0.82f, 0.78f, 0.96f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleThroughTraffic},
    {11u, 0.68f, 0.86f, 0.80f, 0.98f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
    {12u, 0.74f, 0.68f, 0.76f, 0.70f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered},
    {13u, 0.54f, 0.46f, 0.50f, 0.52f, MDTBPopulationStyleResidentialCalm | MDTBPopulationStyleTransitHeavy},
    {14u, 0.64f, 0.86f, 0.78f, 1.0f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleRetailClustered | MDTBPopulationStyleThroughTraffic},
    {15u, 0.62f, 0.88f, 0.76f, 1.0f, MDTBPopulationStyleTransitHeavy | MDTBPopulationStyleThroughTraffic},
};

static size_t scene_layout_count(void) {
    return sizeof(kBlockLayout) / sizeof(kBlockLayout[0]);
}

static void rebuild_road_spines(void) {
    g_road_spine_count = 0u;

    for (size_t index = 0u; index < (sizeof(kRoadLayout) / sizeof(kRoadLayout[0])); ++index) {
        const MDTBRoadLink *link = &kRoadLayout[index];
        const float coordinate =
            link->axis == MDTBRoadAxisNorthSouth
            ? link->midpoint.x
            : link->midpoint.z;
        int already_present = 0;

        for (size_t spine_index = 0u; spine_index < g_road_spine_count; ++spine_index) {
            if (g_road_spines[spine_index].axis == link->axis &&
                fabsf(g_road_spines[spine_index].coordinate - coordinate) <= 0.01f) {
                already_present = 1;
                break;
            }
        }

        if (already_present || g_road_spine_count >= MDTBMaxRoadSpines) {
            continue;
        }

        g_road_spines[g_road_spine_count].coordinate = coordinate;
        g_road_spines[g_road_spine_count].axis = link->axis;
        g_road_spines[g_road_spine_count].road_class = link->road_class;
        g_road_spines[g_road_spine_count].corridor = link->corridor;
        g_road_spine_count += 1u;
    }
}

static const MDTBRoadSpine *road_spine_for_axis_coordinate(uint32_t axis, float coordinate) {
    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        if (g_road_spines[index].axis == axis &&
            fabsf(g_road_spines[index].coordinate - coordinate) <= 0.01f) {
            return &g_road_spines[index];
        }
    }

    return NULL;
}

static int road_spine_segment_hits_intersection(const MDTBRoadSpine *spine, float segment_center) {
    if (spine == NULL) {
        return 0;
    }

    for (size_t block_index = 0u; block_index < scene_layout_count(); ++block_index) {
        const MDTBBlockDescriptor *block = &kBlockLayout[block_index];
        const float corridor_coordinate =
            spine->axis == MDTBRoadAxisNorthSouth
            ? block->origin.x
            : block->origin.z;
        const float intersection_coordinate =
            spine->axis == MDTBRoadAxisNorthSouth
            ? block->origin.z
            : block->origin.x;

        if (fabsf(corridor_coordinate - spine->coordinate) <= 0.01f &&
            fabsf(segment_center - intersection_coordinate) <= kIntersectionClear) {
            return 1;
        }
    }

    return 0;
}

static const MDTBRoadProfile *road_profile_for_class(uint32_t road_class) {
    static const MDTBRoadProfile kBoulevardProfile = {7.2f, 7.9f, 14.2f, 2.25f, 0.52f};
    static const MDTBRoadProfile kAvenueProfile = {6.2f, 6.9f, 11.8f, 1.95f, 0.0f};
    static const MDTBRoadProfile kConnectorProfile = {5.2f, 5.8f, 10.2f, 1.70f, 0.0f};
    static const MDTBRoadProfile kResidentialProfile = {kRoadHalfWidth, kCurbOuter, kSidewalkOuter, 1.58f, 0.0f};

    switch (road_class) {
        case MDTBRoadClassBoulevard:
            return &kBoulevardProfile;
        case MDTBRoadClassAvenue:
            return &kAvenueProfile;
        case MDTBRoadClassConnector:
            return &kConnectorProfile;
        case MDTBRoadClassResidentialStreet:
        default:
            return &kResidentialProfile;
    }
}

static const MDTBRoadProfile *road_profile_for_spine(const MDTBRoadSpine *spine) {
    return spine != NULL ? road_profile_for_class(spine->road_class) : road_profile_for_class(MDTBRoadClassResidentialStreet);
}

static const MDTBRoadProfile *road_profile_for_link(const MDTBRoadLink *link) {
    return link != NULL ? road_profile_for_class(link->road_class) : road_profile_for_class(MDTBRoadClassResidentialStreet);
}

static const MDTBWorldChunkDescriptor *world_chunk_descriptor_for_index(uint32_t chunk_index) {
    for (size_t index = 0u; index < (sizeof(kWorldChunkLayout) / sizeof(kWorldChunkLayout[0])); ++index) {
        if (kWorldChunkLayout[index].chunk_index == chunk_index) {
            return &kWorldChunkLayout[index];
        }
    }

    return NULL;
}

static const MDTBCorridorDescriptor *corridor_descriptor_for_axis_coordinate(uint32_t axis, float coordinate) {
    for (size_t index = 0u; index < (sizeof(kCorridorLayout) / sizeof(kCorridorLayout[0])); ++index) {
        if (kCorridorLayout[index].axis == axis &&
            fabsf(kCorridorLayout[index].coordinate - coordinate) <= 0.01f) {
            return &kCorridorLayout[index];
        }
    }

    return NULL;
}

static MDTBIntersectionProfile intersection_profile_for_block(const MDTBBlockDescriptor *block) {
    MDTBIntersectionProfile profile;

    profile.vertical = NULL;
    profile.horizontal = NULL;
    profile.chunk = NULL;

    if (block == NULL) {
        return profile;
    }

    profile.vertical = corridor_descriptor_for_axis_coordinate(MDTBRoadAxisNorthSouth, block->origin.x);
    profile.horizontal = corridor_descriptor_for_axis_coordinate(MDTBRoadAxisEastWest, block->origin.z);
    profile.chunk = world_chunk_descriptor_for_index(block->chunk_index);
    return profile;
}

static const MDTBFrontageDescriptor *frontage_descriptor_for_template(uint32_t frontage_template) {
    for (size_t index = 0u; index < (sizeof(kFrontageLayout) / sizeof(kFrontageLayout[0])); ++index) {
        if (kFrontageLayout[index].frontage_template == frontage_template) {
            return &kFrontageLayout[index];
        }
    }

    return &kFrontageLayout[0];
}

static MDTBFrontageProfile frontage_profile_for_block(const MDTBBlockDescriptor *block) {
    MDTBFrontageProfile profile;
    const MDTBFrontageDescriptor *descriptor = frontage_descriptor_for_template(
        block != NULL ? block->frontage_template : MDTBFrontageTemplateCivicRetail
    );
    const MDTBIntersectionProfile intersection = intersection_profile_for_block(block);
    const float horizontal_plaza_scale = intersection.horizontal != NULL ? intersection.horizontal->plaza_scale : 1.0f;
    const float vertical_plaza_scale = intersection.vertical != NULL ? intersection.vertical->plaza_scale : 1.0f;
    const float chunk_plaza_scale = intersection.chunk != NULL ? intersection.chunk->corner_plaza_scale : 1.0f;
    const float chunk_planter_scale = intersection.chunk != NULL ? intersection.chunk->planter_scale : 1.0f;
    const float horizontal_boulevard_bias =
        intersection.horizontal != NULL && intersection.horizontal->road_class == MDTBRoadClassBoulevard
        ? 0.22f
        : 0.0f;
    const float vertical_boulevard_bias =
        intersection.vertical != NULL && intersection.vertical->road_class == MDTBRoadClassBoulevard
        ? 0.10f
        : 0.0f;
    const float frontage_depth_bias =
        (horizontal_plaza_scale - 1.0f) * 1.10f +
        (chunk_plaza_scale - 1.0f) * 0.85f +
        horizontal_boulevard_bias;
    const float rear_depth_bias =
        (chunk_plaza_scale - 1.0f) * 1.70f +
        (vertical_plaza_scale - 1.0f) * 0.55f;
    const float awning_bias =
        1.0f +
        ((horizontal_plaza_scale - 1.0f) * 0.36f) +
        ((chunk_plaza_scale - 1.0f) * 0.22f) +
        horizontal_boulevard_bias * 0.18f;
    const float secondary_awning_bias =
        1.0f +
        ((vertical_plaza_scale - 1.0f) * 0.20f) +
        ((chunk_plaza_scale - 1.0f) * 0.14f) +
        vertical_boulevard_bias * 0.14f;

    profile.descriptor = descriptor;
    profile.intersection = intersection;
    profile.shopfront_z = descriptor->shopfront_z + frontage_depth_bias;
    profile.furniture_z = descriptor->furniture_z + frontage_depth_bias * 0.62f;
    profile.transit_stop_z = descriptor->transit_stop_z + frontage_depth_bias * 0.82f;
    profile.rear_anchor_z = descriptor->rear_anchor_z + rear_depth_bias;
    profile.rear_fence_z = descriptor->rear_fence_z + rear_depth_bias * 0.92f;
    profile.planter_size = descriptor->planter_size * chunk_planter_scale;
    profile.primary_awning_scale = descriptor->primary_awning_scale * awning_bias;
    profile.secondary_awning_scale = descriptor->secondary_awning_scale * secondary_awning_bias;
    profile.loading_zone_z = descriptor->loading_zone_z + frontage_depth_bias * 0.36f;
    profile.loading_zone_half_z = descriptor->loading_zone_half_z * (1.0f + frontage_depth_bias * 0.05f);
    profile.loading_zone_half_x_scale =
        descriptor->loading_zone_half_x_scale *
        (1.0f + ((chunk_plaza_scale - 1.0f) * 0.18f) + horizontal_boulevard_bias * 0.12f);
    return profile;
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

static uint32_t nearest_road_axis_for_position(MDTBFloat3 position) {
    uint32_t best_axis = MDTBRoadAxisNorthSouth;
    float best_distance_squared = 1000000.0f;

    for (size_t index = 0u; index < g_road_link_count; ++index) {
        const MDTBRoadLink *link = &g_road_links[index];
        float link_distance_squared;

        if (link->from_block_index >= g_block_count || link->to_block_index >= g_block_count) {
            continue;
        }

        link_distance_squared = point_to_segment_distance_squared_xz(
            position,
            g_blocks[link->from_block_index].origin,
            g_blocks[link->to_block_index].origin
        );

        if (link_distance_squared < best_distance_squared) {
            best_distance_squared = link_distance_squared;
            best_axis = link->axis;
        }
    }

    return best_axis;
}

static uint32_t nearest_road_link_index_for_position(MDTBFloat3 position) {
    uint32_t best_index = MDTBIndexNone;
    float best_distance_squared = 1000000.0f;

    for (size_t index = 0u; index < g_road_link_count; ++index) {
        const MDTBRoadLink *link = &g_road_links[index];
        float link_distance_squared;

        if (link->from_block_index >= g_block_count || link->to_block_index >= g_block_count) {
            continue;
        }

        link_distance_squared = point_to_segment_distance_squared_xz(
            position,
            g_blocks[link->from_block_index].origin,
            g_blocks[link->to_block_index].origin
        );

        if (link_distance_squared < best_distance_squared) {
            best_distance_squared = link_distance_squared;
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

static int territory_descriptor_for_position(MDTBFloat3 position, uint32_t *faction_out, uint32_t *phase_out, float *presence_out) {
    if (position.x < kCourtSetTerritoryMinX || position.x > kCourtSetTerritoryMaxX ||
        position.z < kCourtSetTerritoryEntryZ || position.z > kCourtSetTerritoryMaxZ) {
        return 0;
    }

    const float z_presence = clampf(
        (position.z - kCourtSetTerritoryEntryZ) / fmaxf(kCourtSetTerritoryCoreZ - kCourtSetTerritoryEntryZ, 0.01f),
        0.0f,
        1.0f
    );
    const float territory_center_x = (kCourtSetTerritoryMinX + kCourtSetTerritoryMaxX) * 0.5f;
    const float half_width = fmaxf((kCourtSetTerritoryMaxX - kCourtSetTerritoryMinX) * 0.5f, 0.01f);
    const float lateral = clampf(1.0f - (fabsf(position.x - territory_center_x) / half_width), 0.0f, 1.0f);
    const float presence = clampf((0.36f + z_presence * 0.64f) * (0.48f + lateral * 0.52f), 0.0f, 1.0f);

    if (faction_out != NULL) {
        *faction_out = MDTBTerritoryFactionCourtSet;
    }
    if (phase_out != NULL) {
        *phase_out = position.z < kCourtSetTerritoryCoreZ ? MDTBTerritoryPhaseBoundary : MDTBTerritoryPhaseClaimed;
    }
    if (presence_out != NULL) {
        *presence_out = presence;
    }

    return 1;
}

static void step_territory_state(MDTBEngineState *state, float dt) {
    uint32_t faction = MDTBTerritoryFactionNone;
    uint32_t phase = MDTBTerritoryPhaseNone;
    float presence = 0.0f;
    float previous_heat;
    float previous_front_watch;
    float previous_deep_watch;
    MDTBFloat3 player_position;
    MDTBFloat3 patrol_target_position;
    MDTBFloat3 inner_target_position;
    MDTBFloat3 patrol_heading_focus;
    MDTBFloat3 inner_heading_focus;
    int inside;
    int previously_inside;
    int patrol_target_state = MDTBTerritoryPatrolIdle;
    int inner_target_state = MDTBTerritoryPatrolIdle;
    float patrol_target_alert = 0.0f;
    float inner_target_alert = 0.0f;
    float patrol_speed = 1.6f;
    float inner_speed = 1.4f;

    if (state == NULL) {
        return;
    }

    player_position = current_player_position(state);
    previous_heat = state->territory_heat;
    previous_front_watch = state->territory_front_watch;
    previous_deep_watch = state->territory_deep_watch;
    previously_inside = state->territory_phase != MDTBTerritoryPhaseNone;
    inside = territory_descriptor_for_position(player_position, &faction, &phase, &presence);

    if (previously_inside && !inside && previous_heat > 0.28f) {
        state->territory_reentry_timer = fmaxf(state->territory_reentry_timer, 8.8f + previous_heat * 3.4f);
    }

    state->territory_reentry_timer = fmaxf(state->territory_reentry_timer - dt, 0.0f);
    state->territory_watch_timer = fmaxf(state->territory_watch_timer - dt, 0.0f);
    state->territory_reapproach_timer = fmaxf(state->territory_reapproach_timer - dt, 0.0f);
    if (state->territory_reapproach_timer <= 0.0f) {
        state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
        state->territory_reapproach_timer = 0.0f;
    }
    patrol_target_position = kTerritoryPatrolBasePosition;
    inner_target_position = kTerritoryInnerBasePosition;
    patrol_heading_focus = player_position;
    inner_heading_focus = player_position;

    {
        const int entry_transition = !previously_inside;
        const int current_vehicle_entry = state->traversal_mode == MDTBTraversalModeVehicle;
        const float sidewalk_distance = sqrtf(distance_squared_xz(player_position, kTerritoryPatrolLinePosition));
        const float sidewalk_watch_proximity = clampf(1.0f - (sidewalk_distance / kTerritoryPatrolSidewalkRadius), 0.0f, 1.0f);
        const int near_sidewalk_watch =
            !inside &&
            player_position.z >= (kCourtSetTerritoryEntryZ - 6.8f) &&
            player_position.z < kCourtSetTerritoryEntryZ &&
            sidewalk_watch_proximity > 0.0f;
        const int far_sidewalk_approach =
            !inside &&
            !near_sidewalk_watch &&
            player_position.z >= (kCourtSetTerritoryEntryZ - 10.8f) &&
            player_position.z < (kCourtSetTerritoryEntryZ - 6.8f) &&
            sidewalk_watch_proximity > 0.0f;
        const int street_hot = state->street_incident_timer > 0.0f && state->street_incident_level > 0.14f;
        const int direct_pressure =
            state->firearm_last_shot_timer > 0.0f ||
            state->combat_hostile_last_shot_timer > 0.0f ||
            state->combat_hostile_attack_windup > 0.0f ||
            state->combat_hostile_search_timer > 0.0f ||
            state->player_damage_pulse > 0.12f;
        const int civilian_reaction =
            state->witness_state != MDTBWitnessStateIdle ||
            state->bystander_state != MDTBWitnessStateIdle;
        const int provoked =
            street_hot ||
            direct_pressure ||
            civilian_reaction ||
            state->combat_hostile_alert > 0.58f ||
            state->combat_last_hit_target_kind == MDTBCombatTargetLookout;
        const int street_normalizing_nearby =
            state->street_incident_timer > 0.0f &&
            state->street_incident_level > 0.08f &&
            !direct_pressure;
        const int street_reopening_nearby =
            state->street_recovery_timer > 0.0f &&
            state->street_recovery_level > 0.04f;
        const float street_cooldown_memory =
            street_normalizing_nearby
            ? clampf(
                state->street_incident_level * 0.72f +
                state->street_incident_timer / (kStreetIncidentSettleDuration + 0.8f),
                0.0f,
                1.0f
            )
            : (street_reopening_nearby
                ? clampf(
                    state->street_recovery_level * 1.20f +
                    state->street_recovery_timer / (kStreetRecoveryDuration + 0.8f),
                    0.0f,
                    1.0f
                )
                : 0.0f);
        const int hot_nearby =
            !inside &&
            state->territory_reentry_timer > 0.0f &&
            previous_heat > 0.22f &&
            sidewalk_watch_proximity > 0.0f;
        const float territory_center_x = (kCourtSetTerritoryMinX + kCourtSetTerritoryMaxX) * 0.5f;
        const float territory_half_width = fmaxf((kCourtSetTerritoryMaxX - kCourtSetTerritoryMinX) * 0.5f, 0.01f);
        const float lateral_bias = clampf((player_position.x - territory_center_x) / territory_half_width, -1.0f, 1.0f);
        const float lateral_amount = fabsf(lateral_bias);
        const MDTBFloat3 boundary_focus_position = make_float3(
            clampf(player_position.x, territory_center_x - 8.0f, territory_center_x + 8.0f),
            kSidewalkHeight,
            kCourtSetTerritoryEntryZ + 1.0f
        );
        const int pocket_claiming =
            state->territory_resolve_state == MDTBTerritoryResolveHold &&
            state->territory_resolve_timer > 0.0f &&
            state->territory_resolve_progress >= 0.999f;
        const int edge_retaking =
            state->territory_resolve_state == MDTBTerritoryResolvePullout &&
            state->territory_resolve_timer > 0.0f &&
            state->territory_resolve_progress >= 0.999f;
        const float reclaim_return_intensity =
            state->territory_reapproach_mode == MDTBTerritoryReapproachReclaim &&
            state->territory_reapproach_timer > 0.0f &&
            !pocket_claiming
            ? clampf(state->territory_reapproach_timer / kTerritoryReapproachReclaimDuration, 0.0f, 1.0f)
            : 0.0f;
        const float retake_return_intensity =
            state->territory_reapproach_mode == MDTBTerritoryReapproachRetake &&
            state->territory_reapproach_timer > 0.0f &&
            !edge_retaking
            ? clampf(state->territory_reapproach_timer / kTerritoryReapproachRetakeDuration, 0.0f, 1.0f)
            : 0.0f;
        const int reclaim_returning = reclaim_return_intensity > 0.0f;
        const int retake_returning = retake_return_intensity > 0.0f;
        const int side_memory_context =
            inside ||
            far_sidewalk_approach ||
            near_sidewalk_watch ||
            hot_nearby ||
            street_normalizing_nearby ||
            street_reopening_nearby ||
            reclaim_returning ||
            retake_returning ||
            pocket_claiming ||
            edge_retaking ||
            state->territory_reentry_timer > 0.0f;
        const float side_memory_target =
            side_memory_context && lateral_amount > 0.14f
            ? clampf(
                lateral_bias * (
                    (inside
                        ? (0.30f + presence * 0.28f + (phase == MDTBTerritoryPhaseBoundary ? 0.12f : 0.04f))
                        : (0.14f + sidewalk_watch_proximity * 0.30f +
                            (hot_nearby ? 0.08f : 0.0f) +
                            street_cooldown_memory * 0.08f)) +
                    reclaim_return_intensity * 0.24f +
                    retake_return_intensity * 0.24f +
                    (pocket_claiming ? 0.10f : 0.0f) +
                    (edge_retaking ? 0.08f : 0.0f)
                ) +
                state->territory_preferred_side * 0.18f,
                -1.0f,
                1.0f
            )
            : state->territory_preferred_side * (
                side_memory_context
                ? (street_cooldown_memory > 0.0f ? 0.985f : 0.96f)
                : 0.0f
            );
        const float side_memory_speed =
            side_memory_context
            ? (lateral_amount > 0.14f
                ? (1.4f +
                    reclaim_return_intensity * 0.9f +
                    retake_return_intensity * 0.9f +
                    sidewalk_watch_proximity * 0.3f +
                    street_cooldown_memory * 0.22f)
                : (0.24f + street_cooldown_memory * 0.08f))
            : 0.18f;

        state->territory_preferred_side = approachf(
            state->territory_preferred_side,
            side_memory_target,
            side_memory_speed,
            dt
        );

        const float preferred_side_bias = clampf(state->territory_preferred_side, -1.0f, 1.0f);
        const float preferred_side_amount = fabsf(preferred_side_bias);
        const float remembered_lateral_blend = clampf(
            preferred_side_amount * (inside ? 0.42f : 0.50f) +
            reclaim_return_intensity * 0.18f +
            retake_return_intensity * 0.18f +
            (hot_nearby ? 0.08f : 0.0f),
            0.0f,
            0.82f
        );
        const float remembered_lateral_bias = clampf(
            lateral_bias * (1.0f - remembered_lateral_blend) + preferred_side_bias * remembered_lateral_blend,
            -1.0f,
            1.0f
        );
        const float remembered_lateral_amount = fabsf(remembered_lateral_bias);

        if (inside) {
            int stored_vehicle_entry;
            int clear_line = 0;
            int entry_clamp = 0;
            int reforming_line = 0;
            int hardening_line = 0;
            const int hot_reentry =
                !previously_inside &&
                state->territory_reentry_timer > 0.0f &&
                previous_heat > 0.24f;
            const float screen_presence_threshold = current_vehicle_entry ? 0.18f : 0.24f;
            float front_watch_target;
            float deep_watch_target;
            const float boundary_depth = clampf(
                (player_position.z - (kCourtSetTerritoryEntryZ - 0.18f)) /
                fmaxf(kCourtSetTerritoryCoreZ - kCourtSetTerritoryEntryZ + 0.18f, 0.01f),
                0.0f,
                1.0f
            );
            const float watch_carry = clampf(state->territory_watch_timer / kTerritoryWatchCarryDuration, 0.0f, 1.0f);
            const float base_heat =
                (phase == MDTBTerritoryPhaseBoundary ? 0.12f : 0.18f) +
                presence * (phase == MDTBTerritoryPhaseBoundary ? 0.14f : 0.22f) +
                (pocket_claiming ? 0.06f : 0.0f) +
                (edge_retaking ? 0.02f : 0.0f) +
                reclaim_return_intensity * (phase == MDTBTerritoryPhaseBoundary ? 0.02f : 0.04f) +
                retake_return_intensity * (phase == MDTBTerritoryPhaseBoundary ? 0.04f : 0.02f);
            const float provocation_heat = provoked ? (0.28f + presence * 0.40f) : 0.0f;
            float watch_heat;
            float heat_target;

            if (entry_transition || state->territory_entry_mode == MDTBTerritoryEntryNone) {
                state->territory_entry_mode = current_vehicle_entry ? MDTBTerritoryEntryVehicle : MDTBTerritoryEntryOnFoot;
            }
            stored_vehicle_entry = state->territory_entry_mode == MDTBTerritoryEntryVehicle;
            const float patrol_side_bias =
                remembered_lateral_amount < 0.16f
                ? (stored_vehicle_entry ? 1.0f : -1.0f)
                : remembered_lateral_bias;

            const MDTBFloat3 patrol_watch_position = offset_point(
                kTerritoryPatrolLinePosition,
                remembered_lateral_bias * 1.6f,
                0.0f,
                -0.22f + remembered_lateral_amount * 0.24f
            );
            const MDTBFloat3 patrol_screen_position = offset_point(
                kTerritoryPatrolLinePosition,
                remembered_lateral_bias * 2.2f,
                0.0f,
                0.52f + (stored_vehicle_entry ? -0.34f : 0.22f) - remembered_lateral_amount * 0.10f
            );
            const MDTBFloat3 patrol_clamp_position = offset_point(
                kTerritoryPatrolLinePosition,
                patrol_side_bias * 2.48f,
                0.0f,
                0.94f + (stored_vehicle_entry ? -0.18f : 0.26f) - remembered_lateral_amount * 0.12f
            );
            const MDTBFloat3 patrol_handoff_position = offset_point(
                kTerritoryPatrolHandoffPosition,
                remembered_lateral_bias * 1.1f,
                0.0f,
                (stored_vehicle_entry ? -0.18f : 0.22f) + remembered_lateral_amount * 0.16f
            );
            const MDTBFloat3 patrol_clear_position = offset_point(
                kTerritoryPatrolHandoffPosition,
                patrol_side_bias * 3.10f,
                0.0f,
                -0.96f + (stored_vehicle_entry ? -0.16f : 0.10f)
            );
            const MDTBFloat3 patrol_settle_position = lerp_float3(
                patrol_watch_position,
                patrol_screen_position,
                clampf(
                    0.08f +
                    watch_carry * 0.16f +
                    (stored_vehicle_entry ? 0.10f : 0.04f),
                    0.0f,
                    0.36f
                )
            );
            const MDTBFloat3 patrol_brace_position = lerp_float3(
                patrol_screen_position,
                patrol_clamp_position,
                clampf(
                    0.30f +
                    boundary_depth * 0.26f +
                    watch_carry * 0.18f,
                    0.0f,
                    0.84f
                )
            );
            const float inner_lateral_bias = clampf(
                (-remembered_lateral_bias * 0.72f) +
                (stored_vehicle_entry ? 0.18f : -0.06f) +
                (hot_reentry ? 0.12f : 0.0f),
                -1.0f,
                1.0f
            );
            const float inner_lateral_amount = fabsf(inner_lateral_bias);
            const MDTBFloat3 inner_pocket_position = offset_point(
                kTerritoryInnerBasePosition,
                inner_lateral_bias * 1.60f,
                0.0f,
                -0.76f + inner_lateral_amount * 0.92f
            );
            const MDTBFloat3 inner_screen_support_position = offset_point(
                kTerritoryInnerReceivePosition,
                inner_lateral_bias * 0.88f,
                0.0f,
                0.96f - inner_lateral_amount * 0.52f
            );
            const MDTBFloat3 inner_handoff_position = offset_point(
                kTerritoryInnerReceivePosition,
                inner_lateral_bias * 1.18f,
                0.0f,
                (stored_vehicle_entry ? -0.20f : 0.34f) + inner_lateral_amount * 0.24f
            );
            const float inner_clamp_bias = clampf(
                (-patrol_side_bias * 0.92f) +
                (stored_vehicle_entry ? 0.14f : -0.02f) +
                (hot_reentry ? 0.10f : 0.0f),
                -1.0f,
                1.0f
            );
            const float inner_clamp_amount = fabsf(inner_clamp_bias);
            const MDTBFloat3 inner_clamp_position = offset_point(
                kTerritoryInnerReceivePosition,
                inner_clamp_bias * 1.34f,
                0.0f,
                1.32f - inner_clamp_amount * 0.42f
            );
            const float reclaim_commit_side_blend = clampf(
                preferred_side_amount * (
                    0.26f +
                    (phase == MDTBTerritoryPhaseClaimed ? 0.28f : 0.0f) +
                    reclaim_return_intensity * 0.22f +
                    state->territory_deep_watch * 0.18f +
                    watch_carry * 0.10f
                ),
                0.0f,
                0.92f
            );
            const float retake_commit_side_blend = clampf(
                preferred_side_amount * (
                    0.24f +
                    (phase == MDTBTerritoryPhaseBoundary ? 0.26f : 0.0f) +
                    retake_return_intensity * 0.22f +
                    boundary_depth * 0.20f +
                    state->territory_front_watch * 0.18f +
                    watch_carry * 0.10f
                ),
                0.0f,
                0.90f
            );
            const float reclaim_commit_side_bias = clampf(
                remembered_lateral_bias * (1.0f - reclaim_commit_side_blend) +
                preferred_side_bias * reclaim_commit_side_blend,
                -1.0f,
                1.0f
            );
            const float retake_commit_side_bias = clampf(
                remembered_lateral_bias * (1.0f - retake_commit_side_blend) +
                preferred_side_bias * retake_commit_side_blend,
                -1.0f,
                1.0f
            );
            const float reclaim_commit_side_amount = fabsf(reclaim_commit_side_bias);
            const float retake_commit_side_amount = fabsf(retake_commit_side_bias);
            const MDTBFloat3 inner_settle_position = lerp_float3(
                inner_pocket_position,
                inner_screen_support_position,
                clampf(
                    0.08f +
                    watch_carry * 0.14f +
                    boundary_depth * 0.10f,
                    0.0f,
                    0.40f
                )
            );
            const MDTBFloat3 inner_brace_position = lerp_float3(
                inner_screen_support_position,
                inner_clamp_position,
                clampf(
                    0.28f +
                    boundary_depth * 0.30f +
                    watch_carry * 0.14f,
                    0.0f,
                    0.82f
                )
            );
            const MDTBFloat3 reclaim_commit_patrol_position = offset_point(
                patrol_handoff_position,
                reclaim_commit_side_bias * (0.18f + reclaim_commit_side_amount * 0.42f),
                0.0f,
                0.06f + reclaim_commit_side_amount * 0.20f
            );
            const MDTBFloat3 reclaim_commit_clear_position = offset_point(
                patrol_clear_position,
                reclaim_commit_side_bias * (0.28f + reclaim_commit_side_amount * 0.54f),
                0.0f,
                -0.08f + reclaim_commit_side_amount * 0.18f
            );
            const MDTBFloat3 reclaim_commit_inner_position = offset_point(
                inner_handoff_position,
                reclaim_commit_side_bias * (0.24f + reclaim_commit_side_amount * 0.46f),
                0.0f,
                0.12f + reclaim_commit_side_amount * 0.22f
            );
            const MDTBFloat3 reclaim_commit_patrol_focus = offset_point(
                player_position,
                reclaim_commit_side_bias * (0.44f + reclaim_commit_side_amount * 0.58f),
                0.0f,
                0.38f + reclaim_commit_side_amount * 0.24f
            );
            const MDTBFloat3 reclaim_commit_inner_focus = offset_point(
                player_position,
                reclaim_commit_side_bias * (0.72f + reclaim_commit_side_amount * 0.68f),
                0.0f,
                1.02f + reclaim_commit_side_amount * 0.34f
            );
            const MDTBFloat3 retake_commit_patrol_position = offset_point(
                patrol_brace_position,
                retake_commit_side_bias * (0.16f + retake_commit_side_amount * 0.48f),
                0.0f,
                0.06f + retake_commit_side_amount * 0.18f
            );
            const MDTBFloat3 retake_commit_inner_position = offset_point(
                inner_brace_position,
                -retake_commit_side_bias * (0.18f + retake_commit_side_amount * 0.42f),
                0.0f,
                0.08f + retake_commit_side_amount * 0.16f
            );
            const MDTBFloat3 retake_commit_patrol_focus = offset_point(
                boundary_focus_position,
                retake_commit_side_bias * (0.54f + retake_commit_side_amount * 0.56f),
                0.0f,
                0.10f + retake_commit_side_amount * 0.14f
            );
            const MDTBFloat3 retake_commit_inner_focus = offset_point(
                boundary_focus_position,
                -retake_commit_side_bias * (0.68f + retake_commit_side_amount * 0.48f),
                0.0f,
                0.56f + retake_commit_side_amount * 0.18f
            );
            const int returning_from_clear =
                phase == MDTBTerritoryPhaseBoundary &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (state->territory_patrol_alert > 0.12f || reclaim_returning) &&
                (state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                 state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                 state->territory_patrol_state == MDTBTerritoryPatrolBrace ||
                 state->territory_inner_state == MDTBTerritoryPatrolReform ||
                 state->territory_inner_state == MDTBTerritoryPatrolBrace ||
                 previous_deep_watch > 0.16f ||
                 (reclaim_returning &&
                  (presence > (stored_vehicle_entry ? 0.12f : 0.16f) ||
                   state->territory_deep_watch > 0.18f ||
                   watch_carry > 0.10f)));
            const int hardening_retake =
                phase == MDTBTerritoryPhaseBoundary &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (returning_from_clear ||
                 state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                 state->territory_patrol_state == MDTBTerritoryPatrolBrace ||
                 state->territory_inner_state == MDTBTerritoryPatrolReform ||
                 state->territory_inner_state == MDTBTerritoryPatrolBrace) &&
                (presence > (stored_vehicle_entry ? (retake_returning ? 0.18f : 0.24f) : (retake_returning ? 0.22f : 0.28f)) ||
                 boundary_depth > 0.18f) &&
                (state->territory_front_watch > (retake_returning ? 0.18f : 0.24f) || watch_carry > (retake_returning ? 0.12f : 0.18f)) &&
                state->territory_deep_watch <= (state->territory_front_watch + 0.18f);
            const int reclaim_edge_feint =
                phase == MDTBTerritoryPhaseBoundary &&
                reclaim_returning &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                boundary_depth < 0.26f &&
                presence > (stored_vehicle_entry ? 0.10f : 0.14f) &&
                presence < (stored_vehicle_entry ? 0.36f : 0.42f) &&
                state->territory_deep_watch >= (state->territory_front_watch - 0.08f);
            const int retake_edge_challenge =
                phase == MDTBTerritoryPhaseBoundary &&
                retake_returning &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                boundary_depth < 0.22f &&
                presence > (stored_vehicle_entry ? 0.10f : 0.14f) &&
                presence < (stored_vehicle_entry ? 0.30f : 0.36f) &&
                (state->territory_front_watch > 0.18f || watch_carry > 0.12f);
            const int screen_entry =
                phase == MDTBTerritoryPhaseBoundary &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (presence > screen_presence_threshold ||
                 state->territory_front_watch > 0.18f ||
                 sidewalk_watch_proximity > (stored_vehicle_entry ? 0.46f : 0.32f));
            const int boundary_hesitation =
                phase == MDTBTerritoryPhaseBoundary &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (screen_entry ||
                 returning_from_clear ||
                 state->territory_patrol_state == MDTBTerritoryPatrolScreen ||
                 state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                 state->territory_patrol_state == MDTBTerritoryPatrolBrace) &&
                (state->territory_front_watch > 0.22f || state->territory_watch_timer > 0.52f) &&
                presence > (stored_vehicle_entry ? 0.18f : 0.22f) &&
                state->territory_deep_watch <= (state->territory_front_watch + 0.14f);
            const float reclaim_counter_step =
                reclaim_edge_feint
                ? clampf(
                    0.20f +
                    watch_carry * 0.24f +
                    reclaim_return_intensity * 0.34f +
                    state->territory_deep_watch * 0.20f -
                    boundary_depth * 0.18f,
                    0.0f,
                    1.0f
                )
                : 0.0f;
            const float retake_cross_angle =
                retake_edge_challenge
                ? clampf(
                    0.22f +
                    watch_carry * 0.22f +
                    retake_return_intensity * 0.34f +
                    state->territory_front_watch * 0.22f -
                    boundary_depth * 0.14f,
                    0.0f,
                    1.0f
                )
                : 0.0f;
            const MDTBFloat3 reclaim_counter_patrol_position = offset_point(
                patrol_screen_position,
                -patrol_side_bias * (0.24f + reclaim_counter_step * 0.72f),
                0.0f,
                -0.10f + reclaim_counter_step * 0.12f
            );
            const MDTBFloat3 reclaim_counter_inner_position = offset_point(
                inner_handoff_position,
                patrol_side_bias * (0.24f + reclaim_counter_step * 0.68f),
                0.0f,
                -0.18f + reclaim_counter_step * 0.16f
            );
            const MDTBFloat3 reclaim_counter_patrol_focus = lerp_float3(
                boundary_focus_position,
                inner_handoff_position,
                clampf(0.26f + reclaim_counter_step * 0.38f, 0.0f, 0.74f)
            );
            const MDTBFloat3 reclaim_counter_inner_focus = offset_point(
                boundary_focus_position,
                patrol_side_bias * (0.68f + reclaim_counter_step * 0.74f),
                0.0f,
                0.52f + reclaim_counter_step * 0.34f
            );
            const MDTBFloat3 retake_challenge_screen_position = offset_point(
                patrol_screen_position,
                patrol_side_bias * (0.28f + retake_cross_angle * 0.84f),
                0.0f,
                0.12f + retake_cross_angle * 0.18f
            );
            const MDTBFloat3 retake_challenge_brace_position = offset_point(
                patrol_clamp_position,
                patrol_side_bias * (0.12f + retake_cross_angle * 0.52f),
                0.0f,
                0.04f + retake_cross_angle * 0.16f
            );
            const MDTBFloat3 retake_challenge_inner_watch_position = offset_point(
                inner_screen_support_position,
                -patrol_side_bias * (0.26f + retake_cross_angle * 0.62f),
                0.0f,
                0.10f + retake_cross_angle * 0.20f
            );
            const MDTBFloat3 retake_challenge_inner_brace_position = offset_point(
                inner_clamp_position,
                -patrol_side_bias * (0.18f + retake_cross_angle * 0.54f),
                0.0f,
                0.06f + retake_cross_angle * 0.18f
            );
            const MDTBFloat3 retake_challenge_patrol_focus = offset_point(
                boundary_focus_position,
                patrol_side_bias * (0.52f + retake_cross_angle * 0.56f),
                0.0f,
                0.10f
            );
            const MDTBFloat3 retake_challenge_inner_focus = offset_point(
                boundary_focus_position,
                -patrol_side_bias * (0.72f + retake_cross_angle * 0.48f),
                0.0f,
                0.56f + retake_cross_angle * 0.22f
            );

            if (phase == MDTBTerritoryPhaseBoundary) {
                front_watch_target =
                    (stored_vehicle_entry ? 0.20f : 0.28f) +
                    presence * (stored_vehicle_entry ? 0.16f : 0.22f);
                deep_watch_target =
                    (stored_vehicle_entry ? 0.10f : 0.05f) +
                    presence * (stored_vehicle_entry ? 0.14f : 0.08f);
            } else {
                front_watch_target =
                    0.26f +
                    presence * 0.20f +
                    (stored_vehicle_entry ? 0.04f : 0.10f);
                deep_watch_target =
                    (stored_vehicle_entry ? 0.28f : 0.18f) +
                    presence * (stored_vehicle_entry ? 0.28f : 0.22f);
            }

            if (pocket_claiming) {
                front_watch_target += 0.06f;
                deep_watch_target += 0.16f;
            } else if (edge_retaking && phase == MDTBTerritoryPhaseBoundary) {
                front_watch_target += 0.12f;
                deep_watch_target += 0.04f;
            }

            if (reclaim_returning) {
                front_watch_target += phase == MDTBTerritoryPhaseBoundary ? 0.02f : 0.04f;
                deep_watch_target += phase == MDTBTerritoryPhaseBoundary ? 0.10f : 0.18f;
            }

            if (retake_returning) {
                front_watch_target += phase == MDTBTerritoryPhaseBoundary ? 0.12f : 0.06f;
                deep_watch_target += phase == MDTBTerritoryPhaseBoundary ? 0.02f : 0.04f;
            }

            front_watch_target += retake_cross_angle * 0.05f;
            deep_watch_target += reclaim_counter_step * 0.05f;

            if (pocket_claiming &&
                phase == MDTBTerritoryPhaseClaimed &&
                !hot_reentry) {
                clear_line =
                    state->territory_deep_watch > 0.58f ||
                    presence > 0.76f ||
                    direct_pressure;
                patrol_target_state = clear_line ? MDTBTerritoryPatrolClear : MDTBTerritoryPatrolHandoff;
                patrol_target_position = clear_line
                    ? patrol_clear_position
                    : lerp_float3(patrol_handoff_position, patrol_clear_position, 0.24f);
                patrol_target_alert = clear_line
                    ? (0.52f +
                        presence * 0.16f +
                        watch_carry * 0.10f +
                        (stored_vehicle_entry ? 0.04f : 0.0f))
                    : (0.74f +
                        presence * 0.18f +
                        watch_carry * 0.10f +
                        (stored_vehicle_entry ? 0.06f : 0.0f));
                patrol_speed = clear_line ? 3.3f : 3.8f;
                patrol_heading_focus = clear_line ? boundary_focus_position : player_position;
            } else if (reclaim_edge_feint) {
                patrol_target_state = MDTBTerritoryPatrolScreen;
                patrol_target_position = lerp_float3(
                    patrol_watch_position,
                    reclaim_counter_patrol_position,
                    clampf(
                        0.40f +
                        presence * 0.24f +
                        watch_carry * 0.10f +
                        reclaim_return_intensity * 0.12f +
                        reclaim_counter_step * 0.12f,
                        0.0f,
                        0.86f
                    )
                );
                patrol_target_alert =
                    0.26f +
                    presence * 0.12f +
                    watch_carry * 0.06f +
                    reclaim_return_intensity * 0.08f +
                    reclaim_counter_step * 0.04f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                patrol_speed = 3.0f + reclaim_counter_step * 0.3f;
                patrol_heading_focus = reclaim_counter_patrol_focus;
            } else if (reclaim_returning &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (phase == MDTBTerritoryPhaseClaimed ||
                 state->territory_deep_watch > 0.24f ||
                 presence > 0.42f)) {
                clear_line =
                    phase == MDTBTerritoryPhaseClaimed &&
                    (state->territory_deep_watch > 0.46f ||
                     presence > 0.68f ||
                     direct_pressure);
                patrol_target_state = clear_line ? MDTBTerritoryPatrolClear : MDTBTerritoryPatrolHandoff;
                patrol_target_position = clear_line
                    ? lerp_float3(patrol_clear_position, patrol_handoff_position, 0.16f)
                    : lerp_float3(patrol_settle_position, patrol_handoff_position, 0.58f);
                patrol_target_alert = clear_line
                    ? (0.46f +
                        presence * 0.14f +
                        watch_carry * 0.08f +
                        reclaim_return_intensity * 0.08f)
                    : (0.62f +
                        presence * 0.16f +
                        watch_carry * 0.10f +
                        reclaim_return_intensity * 0.10f +
                        (stored_vehicle_entry ? 0.04f : 0.0f));
                patrol_speed = clear_line ? 3.1f : 3.5f;
                patrol_heading_focus = player_position;
            } else if (retake_edge_challenge) {
                entry_clamp = boundary_hesitation;
                patrol_target_state = entry_clamp ? MDTBTerritoryPatrolBrace : MDTBTerritoryPatrolScreen;
                patrol_target_position = entry_clamp
                    ? lerp_float3(
                        patrol_screen_position,
                        retake_challenge_brace_position,
                        clampf(
                            0.42f +
                            presence * 0.18f +
                            watch_carry * 0.10f +
                            retake_return_intensity * 0.10f +
                            retake_cross_angle * 0.10f,
                            0.0f,
                            0.86f
                        )
                    )
                    : lerp_float3(
                        patrol_watch_position,
                        retake_challenge_screen_position,
                        clampf(
                            0.46f +
                            presence * 0.20f +
                            retake_return_intensity * 0.12f +
                            retake_cross_angle * 0.12f,
                            0.0f,
                            0.88f
                        )
                    );
                patrol_target_alert =
                    (entry_clamp ? 0.38f : 0.30f) +
                    presence * 0.16f +
                    watch_carry * 0.08f +
                    retake_return_intensity * 0.10f +
                    retake_cross_angle * 0.04f +
                    (stored_vehicle_entry ? 0.06f : 0.02f);
                patrol_speed = (entry_clamp ? 3.2f : 3.0f) + retake_cross_angle * 0.24f;
                patrol_heading_focus = retake_challenge_patrol_focus;
            } else if (retake_returning &&
                phase == MDTBTerritoryPhaseBoundary &&
                !hot_reentry &&
                !provoked &&
                state->combat_hostile_search_timer <= 0.0f &&
                (presence > (stored_vehicle_entry ? 0.12f : 0.16f) ||
                 state->territory_front_watch > 0.18f ||
                 watch_carry > 0.10f)) {
                hardening_line = 1;
                entry_clamp = 1;
                patrol_target_state = MDTBTerritoryPatrolBrace;
                patrol_target_position = lerp_float3(
                    patrol_screen_position,
                    patrol_clamp_position,
                    clampf(0.56f + boundary_depth * 0.18f + retake_return_intensity * 0.18f, 0.0f, 0.90f)
                );
                patrol_target_alert =
                    0.36f +
                    presence * 0.16f +
                    boundary_depth * 0.10f +
                    watch_carry * 0.08f +
                    retake_return_intensity * 0.10f +
                    (stored_vehicle_entry ? 0.06f : 0.0f);
                patrol_speed = 3.2f;
                patrol_heading_focus = boundary_focus_position;
            } else if (phase == MDTBTerritoryPhaseClaimed ||
                hot_reentry ||
                provoked ||
                state->combat_hostile_search_timer > 0.0f ||
                state->territory_deep_watch > 0.34f) {
                clear_line =
                    !hot_reentry &&
                    (phase == MDTBTerritoryPhaseClaimed ||
                     provoked ||
                     state->combat_hostile_search_timer > 0.0f ||
                     state->combat_hostile_attack_windup > 0.0f ||
                     state->territory_deep_watch > 0.38f) &&
                    (state->territory_inner_alert > 0.18f ||
                     presence > 0.52f ||
                     direct_pressure ||
                     state->combat_hostile_alert > 0.72f);
                patrol_target_state = clear_line ? MDTBTerritoryPatrolClear : MDTBTerritoryPatrolHandoff;
                patrol_target_position = clear_line ? patrol_clear_position : patrol_handoff_position;
                patrol_target_alert = clear_line
                    ? (0.48f +
                        presence * 0.16f +
                        (stored_vehicle_entry ? 0.04f : 0.0f) +
                        (provoked ? 0.04f : 0.0f))
                    : (0.66f +
                        presence * 0.20f +
                        (stored_vehicle_entry ? 0.06f : 0.0f) +
                        (hot_reentry ? 0.10f : 0.0f) +
                        (provoked ? 0.04f : 0.0f));
                patrol_speed = clear_line ? 3.2f : 3.6f;
                patrol_heading_focus = clear_line ? boundary_focus_position : player_position;
            } else if (hardening_retake) {
                hardening_line = 1;
                entry_clamp = 1;
                patrol_target_state = MDTBTerritoryPatrolBrace;
                patrol_target_position = patrol_brace_position;
                patrol_target_alert =
                    0.34f +
                    presence * 0.14f +
                    boundary_depth * 0.10f +
                    watch_carry * 0.08f +
                    state->territory_front_watch * 0.08f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                patrol_speed = 3.1f;
                patrol_heading_focus = boundary_focus_position;
            } else if (returning_from_clear) {
                const float reform_blend = clampf(
                    0.20f +
                    presence * 0.28f +
                    state->territory_front_watch * 0.16f +
                    watch_carry * 0.10f +
                    state->territory_deep_watch * 0.10f,
                    0.0f,
                    0.74f
                );

                reforming_line = 1;
                entry_clamp = boundary_hesitation;
                patrol_target_state = MDTBTerritoryPatrolReform;
                patrol_target_position = lerp_float3(
                    patrol_clear_position,
                    entry_clamp ? patrol_screen_position : patrol_settle_position,
                    reform_blend
                );
                patrol_target_alert =
                    0.22f +
                    presence * 0.10f +
                    state->territory_front_watch * 0.08f +
                    watch_carry * 0.06f +
                    (entry_clamp ? 0.04f : 0.0f) +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                patrol_speed = 2.8f;
                patrol_heading_focus = boundary_focus_position;
            } else if (screen_entry) {
                entry_clamp = boundary_hesitation;
                patrol_target_state = MDTBTerritoryPatrolScreen;
                patrol_target_position = entry_clamp ? patrol_clamp_position : patrol_screen_position;
                patrol_target_alert =
                    (entry_clamp ? 0.42f : 0.36f) +
                    presence * 0.20f +
                    (stored_vehicle_entry ? 0.08f : 0.04f);
                patrol_speed = entry_clamp ? 3.1f : 3.3f;
                patrol_heading_focus = boundary_focus_position;
            } else {
                const float line_blend = clampf(
                    (player_position.z - (kCourtSetTerritoryEntryZ - 0.8f)) /
                    fmaxf(kCourtSetTerritoryCoreZ - kCourtSetTerritoryEntryZ + 0.8f, 0.01f),
                    0.0f,
                    1.0f
                );
                patrol_target_state = MDTBTerritoryPatrolWatch;
                patrol_target_position = lerp_float3(kTerritoryPatrolBasePosition, patrol_watch_position, clampf(0.36f + line_blend * 0.64f, 0.0f, 1.0f));
                patrol_target_alert =
                    0.28f +
                    presence * 0.18f +
                    (stored_vehicle_entry ? 0.08f : 0.0f) +
                    (phase == MDTBTerritoryPhaseBoundary ? 0.04f : 0.0f);
                patrol_speed = 3.0f;
            }

            {
                const float patrol_front_weight =
                    patrol_target_state == MDTBTerritoryPatrolHandoff ? 0.10f :
                    (patrol_target_state == MDTBTerritoryPatrolScreen ? 0.12f :
                     (patrol_target_state == MDTBTerritoryPatrolBrace ? 0.14f :
                      (patrol_target_state == MDTBTerritoryPatrolClear ? 0.04f :
                       (patrol_target_state == MDTBTerritoryPatrolReform ? 0.08f : 0.06f))));
                const float patrol_deep_weight =
                    patrol_target_state == MDTBTerritoryPatrolHandoff ? 0.06f :
                    (patrol_target_state == MDTBTerritoryPatrolScreen ? 0.03f :
                     (patrol_target_state == MDTBTerritoryPatrolBrace ? 0.04f :
                      (patrol_target_state == MDTBTerritoryPatrolClear ? 0.06f :
                       (patrol_target_state == MDTBTerritoryPatrolReform ? 0.04f : 0.02f))));
                front_watch_target += patrol_target_alert * patrol_front_weight;
                deep_watch_target += patrol_target_alert * patrol_deep_weight;
            }

            if (phase == MDTBTerritoryPhaseClaimed ||
                hot_reentry ||
                provoked ||
                patrol_target_state == MDTBTerritoryPatrolHandoff ||
                state->territory_patrol_alert > 0.46f ||
                state->territory_deep_watch > 0.28f) {
                const float inner_blend = clampf(
                    0.24f +
                    presence * 0.48f +
                    patrol_target_alert * 0.22f +
                    (hot_reentry ? 0.16f : 0.0f),
                    0.0f,
                    1.0f
                );

                inner_target_state =
                    (phase == MDTBTerritoryPhaseClaimed ||
                     patrol_target_state == MDTBTerritoryPatrolHandoff ||
                     patrol_target_state == MDTBTerritoryPatrolClear ||
                     hot_reentry)
                    ? MDTBTerritoryPatrolHandoff
                    : MDTBTerritoryPatrolWatch;
                inner_target_position = lerp_float3(
                    clear_line ? inner_clamp_position : inner_pocket_position,
                    inner_handoff_position,
                    inner_blend
                );
                inner_target_alert =
                    0.22f +
                    presence * 0.18f +
                    patrol_target_alert * (inner_target_state == MDTBTerritoryPatrolHandoff ? 0.34f : 0.22f) +
                    (stored_vehicle_entry ? 0.04f : 0.0f) +
                    (provoked ? 0.06f : 0.0f) +
                    (hot_reentry ? 0.10f : 0.0f);
                inner_speed = inner_target_state == MDTBTerritoryPatrolHandoff ? 3.4f : 2.8f;
                inner_heading_focus = clear_line ? boundary_focus_position : player_position;
            } else if (reclaim_edge_feint) {
                inner_target_state = MDTBTerritoryPatrolHandoff;
                inner_target_position = lerp_float3(
                    inner_pocket_position,
                    reclaim_counter_inner_position,
                    clampf(
                        0.18f +
                        presence * 0.18f +
                        patrol_target_alert * 0.20f +
                        reclaim_return_intensity * 0.18f +
                        state->territory_deep_watch * 0.14f +
                        reclaim_counter_step * 0.12f,
                        0.0f,
                        0.70f
                    )
                );
                inner_target_alert =
                    0.14f +
                    presence * 0.06f +
                    patrol_target_alert * 0.20f +
                    reclaim_return_intensity * 0.08f +
                    reclaim_counter_step * 0.04f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                inner_speed = 2.9f + reclaim_counter_step * 0.3f;
                inner_heading_focus = reclaim_counter_inner_focus;
            } else if (retake_edge_challenge) {
                entry_clamp = boundary_hesitation;
                inner_target_state = entry_clamp ? MDTBTerritoryPatrolBrace : MDTBTerritoryPatrolWatch;
                inner_target_position = lerp_float3(
                    inner_pocket_position,
                    entry_clamp ? retake_challenge_inner_brace_position : retake_challenge_inner_watch_position,
                    clampf(
                        (entry_clamp ? 0.34f : 0.22f) +
                        presence * 0.16f +
                        patrol_target_alert * 0.16f +
                        retake_return_intensity * 0.10f +
                        retake_cross_angle * 0.10f,
                        0.0f,
                        0.74f
                    )
                );
                inner_target_alert =
                    (entry_clamp ? 0.16f : 0.10f) +
                    presence * 0.08f +
                    patrol_target_alert * 0.18f +
                    retake_return_intensity * 0.06f +
                    retake_cross_angle * 0.04f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                inner_speed = (entry_clamp ? 2.8f : 2.5f) + retake_cross_angle * 0.22f;
                inner_heading_focus = retake_challenge_inner_focus;
            } else if (hardening_line) {
                const float brace_blend = clampf(
                    0.26f +
                    presence * 0.20f +
                    boundary_depth * 0.18f +
                    patrol_target_alert * 0.18f +
                    state->territory_front_watch * 0.10f,
                    0.0f,
                    0.86f
                );

                inner_target_state = MDTBTerritoryPatrolBrace;
                inner_target_position = lerp_float3(
                    inner_screen_support_position,
                    inner_brace_position,
                    brace_blend
                );
                inner_target_alert =
                    0.16f +
                    presence * 0.08f +
                    patrol_target_alert * 0.24f +
                    boundary_depth * 0.06f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                inner_speed = 2.8f;
                inner_heading_focus = boundary_focus_position;
            } else if (reforming_line) {
                const float reform_blend = clampf(
                    0.12f +
                    presence * 0.18f +
                    state->territory_front_watch * 0.10f +
                    watch_carry * 0.08f +
                    state->territory_deep_watch * 0.12f +
                    patrol_target_alert * 0.10f,
                    0.0f,
                    0.62f
                );

                inner_target_state = MDTBTerritoryPatrolReform;
                inner_target_position = lerp_float3(
                    inner_pocket_position,
                    entry_clamp ? inner_screen_support_position : inner_settle_position,
                    reform_blend
                );
                inner_target_alert =
                    0.10f +
                    presence * 0.06f +
                    patrol_target_alert * 0.16f +
                    watch_carry * 0.06f +
                    state->territory_deep_watch * 0.04f +
                    (entry_clamp ? 0.02f : 0.0f);
                inner_speed = 2.2f;
                inner_heading_focus = boundary_focus_position;
            } else if (patrol_target_state == MDTBTerritoryPatrolScreen) {
                entry_clamp = boundary_hesitation;
                inner_target_state = MDTBTerritoryPatrolWatch;
                inner_target_position = lerp_float3(
                    inner_pocket_position,
                    entry_clamp ? inner_clamp_position : inner_screen_support_position,
                    clampf((entry_clamp ? 0.36f : 0.28f) + presence * 0.22f + patrol_target_alert * 0.16f, 0.0f, 0.74f)
                );
                inner_target_alert =
                    (entry_clamp ? 0.18f : 0.14f) +
                    presence * 0.10f +
                    patrol_target_alert * 0.20f +
                    (stored_vehicle_entry ? 0.04f : 0.0f);
                inner_speed = entry_clamp ? 2.9f : 2.7f;
                inner_heading_focus = boundary_focus_position;
            } else if (phase == MDTBTerritoryPhaseBoundary &&
                       (patrol_target_state != MDTBTerritoryPatrolIdle || presence > 0.18f)) {
                inner_target_state = MDTBTerritoryPatrolWatch;
                inner_target_position = lerp_float3(
                    inner_pocket_position,
                    inner_screen_support_position,
                    clampf(0.10f + presence * 0.18f + patrol_target_alert * 0.10f, 0.0f, 0.36f)
                );
                inner_target_alert = 0.10f + presence * 0.08f + patrol_target_alert * 0.18f;
                inner_speed = 2.5f;
            }

            {
                const int reclaim_commit_follow_through =
                    reclaim_returning &&
                    !reclaim_edge_feint &&
                    !hot_reentry &&
                    !provoked &&
                    state->combat_hostile_search_timer <= 0.0f &&
                    preferred_side_amount > 0.22f &&
                    (phase == MDTBTerritoryPhaseClaimed ||
                     clear_line ||
                     patrol_target_state == MDTBTerritoryPatrolHandoff ||
                     inner_target_state == MDTBTerritoryPatrolHandoff) &&
                    (boundary_depth > 0.18f ||
                     state->territory_deep_watch > 0.22f ||
                     presence > 0.34f);
                const int retake_commit_follow_through =
                    retake_returning &&
                    !retake_edge_challenge &&
                    !hot_reentry &&
                    !provoked &&
                    state->combat_hostile_search_timer <= 0.0f &&
                    preferred_side_amount > 0.22f &&
                    (hardening_line ||
                     patrol_target_state == MDTBTerritoryPatrolBrace ||
                     patrol_target_state == MDTBTerritoryPatrolHandoff ||
                     inner_target_state == MDTBTerritoryPatrolBrace ||
                     inner_target_state == MDTBTerritoryPatrolHandoff) &&
                    (boundary_depth > 0.12f ||
                     presence > 0.28f ||
                     state->territory_front_watch > 0.18f);

                if (reclaim_commit_follow_through) {
                    const float carry_blend = clampf(
                        0.24f +
                        reclaim_commit_side_amount * 0.36f +
                        state->territory_deep_watch * 0.18f +
                        boundary_depth * 0.10f,
                        0.0f,
                        0.88f
                    );
                    const float inner_carry_blend = clampf(carry_blend + 0.10f, 0.0f, 0.92f);

                    patrol_target_position = lerp_float3(
                        patrol_target_position,
                        clear_line ? reclaim_commit_clear_position : reclaim_commit_patrol_position,
                        carry_blend
                    );
                    patrol_heading_focus = lerp_float3(
                        patrol_heading_focus,
                        reclaim_commit_patrol_focus,
                        carry_blend
                    );
                    patrol_target_alert += carry_blend * 0.06f;
                    patrol_speed += carry_blend * 0.28f;
                    inner_target_position = lerp_float3(
                        inner_target_position,
                        reclaim_commit_inner_position,
                        inner_carry_blend
                    );
                    inner_heading_focus = lerp_float3(
                        inner_heading_focus,
                        reclaim_commit_inner_focus,
                        carry_blend
                    );
                    inner_target_alert += carry_blend * 0.05f;
                    inner_speed += carry_blend * 0.22f;
                    deep_watch_target += carry_blend * 0.08f;
                } else if (retake_commit_follow_through) {
                    const float carry_blend = clampf(
                        0.24f +
                        retake_commit_side_amount * 0.34f +
                        boundary_depth * 0.18f +
                        state->territory_front_watch * 0.14f,
                        0.0f,
                        0.88f
                    );
                    const float inner_carry_blend = clampf(carry_blend + 0.08f, 0.0f, 0.90f);

                    patrol_target_position = lerp_float3(
                        patrol_target_position,
                        retake_commit_patrol_position,
                        carry_blend
                    );
                    patrol_heading_focus = lerp_float3(
                        patrol_heading_focus,
                        retake_commit_patrol_focus,
                        carry_blend
                    );
                    patrol_target_alert += carry_blend * 0.06f;
                    patrol_speed += carry_blend * 0.24f;
                    inner_target_position = lerp_float3(
                        inner_target_position,
                        retake_commit_inner_position,
                        inner_carry_blend
                    );
                    inner_heading_focus = lerp_float3(
                        inner_heading_focus,
                        retake_commit_inner_focus,
                        carry_blend
                    );
                    inner_target_alert += carry_blend * 0.05f;
                    inner_speed += carry_blend * 0.20f;
                    front_watch_target += carry_blend * 0.08f;
                }
            }

            front_watch_target += inner_target_alert * (
                inner_target_state == MDTBTerritoryPatrolHandoff
                ? 0.02f
                : (inner_target_state == MDTBTerritoryPatrolBrace ? 0.06f :
                   (entry_clamp ? 0.04f :
                    (inner_target_state == MDTBTerritoryPatrolReform ? 0.02f : 0.0f)))
            );
            deep_watch_target += inner_target_alert * (
                inner_target_state == MDTBTerritoryPatrolHandoff
                ? 0.16f
                : (inner_target_state == MDTBTerritoryPatrolBrace ? 0.10f :
                   (entry_clamp ? 0.10f :
                    (inner_target_state == MDTBTerritoryPatrolReform ? 0.06f : 0.08f)))
            );

            if (entry_clamp) {
                front_watch_target += 0.04f;
                deep_watch_target += 0.05f;
            }

            if (hot_reentry) {
                front_watch_target += 0.16f;
                deep_watch_target += 0.20f;
            }

            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                phase == MDTBTerritoryPhaseBoundary
                    ? (stored_vehicle_entry ? kTerritoryWatchCarryDuration * 0.82f : kTerritoryWatchCarryDuration * 0.72f)
                    : (stored_vehicle_entry ? kTerritoryWatchCarryDuration * 1.18f : kTerritoryWatchCarryDuration * 0.96f)
            );
            state->territory_front_watch = approachf(state->territory_front_watch, clampf(front_watch_target, 0.0f, 1.0f), 3.8f, dt);
            state->territory_deep_watch = approachf(state->territory_deep_watch, clampf(deep_watch_target, 0.0f, 1.0f), 3.5f, dt);

            watch_heat = fmaxf(state->territory_front_watch, state->territory_deep_watch) * 0.12f;
            heat_target = clampf(
                base_heat +
                provocation_heat +
                watch_heat +
                patrol_target_alert * 0.08f +
                inner_target_alert * 0.06f +
                (pocket_claiming ? 0.10f : 0.0f) +
                (edge_retaking ? 0.04f : 0.0f) +
                reclaim_return_intensity * 0.08f +
                retake_return_intensity * 0.06f +
                (hot_reentry ? (0.18f + previous_heat * 0.20f) : 0.0f),
                0.0f,
                1.0f
            );

            state->territory_faction = faction;
            state->territory_presence = approachf(state->territory_presence, presence, 4.2f, dt);
            state->territory_heat = approachf(state->territory_heat, heat_target, provoked ? 2.8f : 0.95f, dt);

            if (hot_reentry && combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
                state->combat_hostile_anchor_index = choose_lookout_pressure_anchor(state);
                state->combat_hostile_reposition_timer = 0.0f;
                state->combat_hostile_search_position = player_position;
                state->combat_hostile_search_timer = 0.0f;
                state->combat_hostile_reacquire_timer = fminf(state->combat_hostile_reacquire_timer, 0.20f);
                state->combat_hostile_attack_cooldown = fminf(state->combat_hostile_attack_cooldown, 0.18f);
                state->combat_hostile_alert = fmaxf(state->combat_hostile_alert, 0.92f);

                if (state->traversal_mode == MDTBTraversalModeOnFoot &&
                    state->combat_player_in_cover == 0u &&
                    state->player_reset_timer <= 0.0f &&
                    state->combat_hostile_attack_windup <= 0.0f) {
                    state->combat_hostile_attack_windup = kLookoutAttackWindupDuration * 0.78f;
                }

                trigger_street_incident(state, player_position, 0.46f, kStreetIncidentSearchDuration * 0.72f);
                state->territory_reentry_timer = fmaxf(state->territory_reentry_timer, 5.2f);
            }

            if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
                const float territory_alert_floor =
                    0.14f +
                    state->territory_presence * 0.20f +
                    state->territory_heat * 0.28f +
                    state->territory_front_watch * 0.16f +
                    state->territory_deep_watch * 0.20f +
                    patrol_target_alert * 0.08f +
                    inner_target_alert * 0.08f +
                    (pocket_claiming ? 0.06f : 0.0f) +
                    reclaim_return_intensity * 0.06f +
                    retake_return_intensity * 0.04f +
                    ((state->territory_reentry_timer > 0.0f && state->territory_heat > 0.34f) ? 0.10f : 0.0f);
                state->combat_hostile_alert = fmaxf(state->combat_hostile_alert, territory_alert_floor);
            }

            state->territory_phase =
                (state->territory_reentry_timer > 0.0f && state->territory_heat > 0.34f)
                ? MDTBTerritoryPhaseHot
                : phase;
        } else {
            if (far_sidewalk_approach ||
                near_sidewalk_watch ||
                hot_nearby ||
                edge_retaking ||
                pocket_claiming ||
                reclaim_returning ||
                retake_returning ||
                (state->street_recovery_timer > 0.0f &&
                 state->street_recovery_level > 0.04f &&
                 sidewalk_watch_proximity > 0.0f &&
                 preferred_side_amount > 0.18f)) {
                const float watch_carry = clampf(state->territory_watch_timer / kTerritoryWatchCarryDuration, 0.0f, 1.0f);
                const float patrol_side_bias =
                    remembered_lateral_amount < 0.16f
                    ? (current_vehicle_entry ? 1.0f : -1.0f)
                    : remembered_lateral_bias;
                const float reopening_reform_intensity =
                    state->street_recovery_timer > 0.0f &&
                    state->street_recovery_level > 0.04f
                    ? clampf(
                        state->street_recovery_level * 0.92f +
                        state->street_recovery_timer / (kStreetRecoveryDuration + 0.8f) +
                        preferred_side_amount * 0.22f,
                        0.0f,
                        1.0f
                    )
                    : 0.0f;
                const MDTBFloat3 patrol_watch_position = offset_point(
                    kTerritoryPatrolLinePosition,
                    remembered_lateral_bias * 1.4f,
                    0.0f,
                    -0.42f + remembered_lateral_amount * 0.18f
                );
                const MDTBFloat3 patrol_screen_position = offset_point(
                    kTerritoryPatrolLinePosition,
                    remembered_lateral_bias * 2.0f,
                    0.0f,
                    0.18f + (current_vehicle_entry ? -0.30f : 0.16f)
                );
                const MDTBFloat3 patrol_clear_position = offset_point(
                    kTerritoryPatrolHandoffPosition,
                    patrol_side_bias * 3.10f,
                    0.0f,
                    -1.02f + (current_vehicle_entry ? -0.16f : 0.08f)
                );
                const MDTBFloat3 patrol_settle_position = lerp_float3(
                    kTerritoryPatrolBasePosition,
                    patrol_watch_position,
                    clampf(
                        0.18f +
                        sidewalk_watch_proximity * 0.44f +
                        watch_carry * 0.18f,
                        0.0f,
                        0.76f
                    )
                );
                const MDTBFloat3 patrol_brace_position = offset_point(
                    patrol_screen_position,
                    patrol_side_bias * 0.42f,
                    0.0f,
                    0.24f
                );
                const float outer_inner_lateral_bias = clampf(
                    (-remembered_lateral_bias * 0.66f) +
                    (current_vehicle_entry ? 0.14f : -0.04f) +
                    (hot_nearby ? 0.10f : 0.0f),
                    -1.0f,
                    1.0f
                );
                const float outer_inner_lateral_amount = fabsf(outer_inner_lateral_bias);
                const MDTBFloat3 inner_pocket_position = offset_point(
                    kTerritoryInnerBasePosition,
                    outer_inner_lateral_bias * 1.44f,
                    0.0f,
                    -0.70f + outer_inner_lateral_amount * 0.86f
                );
                const MDTBFloat3 inner_screen_support_position = offset_point(
                    kTerritoryInnerReceivePosition,
                    outer_inner_lateral_bias * 0.82f,
                    0.0f,
                    1.08f - outer_inner_lateral_amount * 0.48f
                );
                const MDTBFloat3 inner_reform_position = offset_point(
                    inner_screen_support_position,
                    -patrol_side_bias * 0.34f,
                    0.0f,
                    -0.12f
                );
                const MDTBFloat3 inner_settle_position = lerp_float3(
                    inner_pocket_position,
                    inner_screen_support_position,
                    clampf(
                        0.10f +
                        sidewalk_watch_proximity * 0.18f +
                        watch_carry * 0.10f,
                        0.0f,
                        0.34f
                    )
                );
                const MDTBFloat3 inner_brace_position = lerp_float3(
                    inner_screen_support_position,
                    inner_reform_position,
                    clampf(
                        0.34f +
                        sidewalk_watch_proximity * 0.22f +
                        watch_carry * 0.10f,
                        0.0f,
                        0.78f
                    )
                );
                const int screening_edge =
                    near_sidewalk_watch &&
                    !hot_nearby &&
                    !retake_returning &&
                    sidewalk_watch_proximity > (current_vehicle_entry ? 0.52f : 0.34f);
                const int reclaiming_pocket_outside =
                    (pocket_claiming || reclaim_returning) &&
                    sidewalk_watch_proximity > 0.0f;
                const int retaking_edge =
                    (edge_retaking || retake_returning) &&
                    (sidewalk_watch_proximity > 0.0f ||
                     state->territory_front_watch > 0.12f ||
                     watch_carry > 0.10f);
                const int reforming_edge =
                    (reopening_reform_intensity > 0.18f &&
                     sidewalk_watch_proximity > 0.0f &&
                     !hot_nearby &&
                     !reclaiming_pocket_outside &&
                     !retaking_edge) ||
                    (near_sidewalk_watch &&
                     !hot_nearby &&
                     state->territory_patrol_alert > 0.10f &&
                     (state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                      state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                      state->territory_patrol_state == MDTBTerritoryPatrolBrace ||
                      state->territory_inner_state == MDTBTerritoryPatrolBrace ||
                      previous_deep_watch > 0.18f ||
                      state->territory_inner_alert > 0.10f));
                const int hardening_edge =
                    reopening_reform_intensity <= 0.18f &&
                    reforming_edge &&
                    sidewalk_watch_proximity > (current_vehicle_entry ? (retake_returning ? 0.32f : 0.40f) : (retake_returning ? 0.22f : 0.28f)) &&
                    (state->territory_front_watch > (retake_returning ? 0.16f : 0.22f) || watch_carry > (retake_returning ? 0.12f : 0.18f));
                const int entry_clamp =
                    (screening_edge || hardening_edge) &&
                    (state->territory_front_watch > 0.20f || state->territory_watch_timer > 0.48f);
                const float reopening_side_bias =
                    preferred_side_amount > 0.18f
                    ? preferred_side_bias
                    : patrol_side_bias;
                const float reopening_side_amount = fabsf(reopening_side_bias);
                const float outside_reclaim_counter_step =
                    reclaiming_pocket_outside
                    ? clampf(
                        0.18f +
                        sidewalk_watch_proximity * 0.22f +
                        watch_carry * 0.22f +
                        reclaim_return_intensity * 0.26f +
                        state->territory_deep_watch * 0.18f,
                        0.0f,
                        1.0f
                    )
                    : 0.0f;
                const float outside_retake_cross_angle =
                    retaking_edge
                    ? clampf(
                        0.20f +
                        sidewalk_watch_proximity * 0.24f +
                        watch_carry * 0.20f +
                        retake_return_intensity * 0.28f +
                        state->territory_front_watch * 0.18f,
                        0.0f,
                        1.0f
                    )
                    : 0.0f;
                const MDTBFloat3 outside_reclaim_patrol_position = offset_point(
                    patrol_settle_position,
                    -patrol_side_bias * (0.22f + outside_reclaim_counter_step * 0.58f),
                    0.0f,
                    0.02f + outside_reclaim_counter_step * 0.10f
                );
                const MDTBFloat3 outside_reclaim_inner_position = offset_point(
                    inner_screen_support_position,
                    patrol_side_bias * (0.18f + outside_reclaim_counter_step * 0.54f),
                    0.0f,
                    -0.12f + outside_reclaim_counter_step * 0.10f
                );
                const MDTBFloat3 outside_reclaim_patrol_focus = lerp_float3(
                    boundary_focus_position,
                    inner_screen_support_position,
                    clampf(0.18f + outside_reclaim_counter_step * 0.30f, 0.0f, 0.62f)
                );
                const MDTBFloat3 outside_reclaim_inner_focus = offset_point(
                    boundary_focus_position,
                    patrol_side_bias * (0.54f + outside_reclaim_counter_step * 0.46f),
                    0.0f,
                    0.38f + outside_reclaim_counter_step * 0.20f
                );
                const MDTBFloat3 outside_retake_patrol_position = offset_point(
                    patrol_brace_position,
                    patrol_side_bias * (0.18f + outside_retake_cross_angle * 0.56f),
                    0.0f,
                    0.08f + outside_retake_cross_angle * 0.12f
                );
                const MDTBFloat3 outside_retake_inner_position = offset_point(
                    inner_brace_position,
                    -patrol_side_bias * (0.22f + outside_retake_cross_angle * 0.52f),
                    0.0f,
                    0.10f + outside_retake_cross_angle * 0.12f
                );
                const MDTBFloat3 outside_retake_patrol_focus = offset_point(
                    boundary_focus_position,
                    patrol_side_bias * (0.42f + outside_retake_cross_angle * 0.46f),
                    0.0f,
                    0.10f
                );
                const MDTBFloat3 outside_retake_inner_focus = offset_point(
                    boundary_focus_position,
                    -patrol_side_bias * (0.56f + outside_retake_cross_angle * 0.42f),
                    0.0f,
                    0.40f + outside_retake_cross_angle * 0.18f
                );
                const float outside_retake_side_blend = clampf(
                    preferred_side_amount * (
                        0.30f +
                        sidewalk_watch_proximity * 0.22f +
                        state->territory_front_watch * 0.16f +
                        watch_carry * 0.12f +
                        (edge_retaking ? 0.20f : 0.0f) +
                        retake_return_intensity * 0.18f
                    ),
                    0.0f,
                    0.90f
                );
                const float outside_retake_side_bias = clampf(
                    remembered_lateral_bias * (1.0f - outside_retake_side_blend) +
                    preferred_side_bias * outside_retake_side_blend,
                    -1.0f,
                    1.0f
                );
                const float outside_retake_side_amount = fabsf(outside_retake_side_bias);
                const MDTBFloat3 outside_retake_reseal_patrol_position = offset_point(
                    outside_retake_patrol_position,
                    outside_retake_side_bias * (0.18f + outside_retake_side_amount * 0.46f),
                    0.0f,
                    0.06f + outside_retake_side_amount * 0.14f
                );
                const MDTBFloat3 outside_retake_reseal_inner_position = offset_point(
                    outside_retake_inner_position,
                    -outside_retake_side_bias * (0.18f + outside_retake_side_amount * 0.42f),
                    0.0f,
                    0.08f + outside_retake_side_amount * 0.12f
                );
                const MDTBFloat3 outside_retake_reseal_patrol_focus = offset_point(
                    boundary_focus_position,
                    outside_retake_side_bias * (0.48f + outside_retake_side_amount * 0.48f),
                    0.0f,
                    0.12f
                );
                const MDTBFloat3 outside_retake_reseal_inner_focus = offset_point(
                    boundary_focus_position,
                    -outside_retake_side_bias * (0.58f + outside_retake_side_amount * 0.42f),
                    0.0f,
                    0.42f + outside_retake_side_amount * 0.16f
                );
                const MDTBFloat3 reopening_patrol_position = offset_point(
                    patrol_settle_position,
                    reopening_side_bias * (0.34f + reopening_side_amount * 0.78f),
                    0.0f,
                    0.08f + reopening_reform_intensity * 0.18f
                );
                const MDTBFloat3 reopening_inner_position = offset_point(
                    inner_settle_position,
                    -reopening_side_bias * (0.24f + reopening_side_amount * 0.64f),
                    0.0f,
                    0.12f + reopening_reform_intensity * 0.18f
                );
                const MDTBFloat3 reopening_patrol_focus = offset_point(
                    boundary_focus_position,
                    reopening_side_bias * (0.42f + reopening_side_amount * 0.44f),
                    0.0f,
                    0.08f + reopening_reform_intensity * 0.06f
                );
                const MDTBFloat3 reopening_inner_focus = offset_point(
                    boundary_focus_position,
                    -reopening_side_bias * (0.46f + reopening_side_amount * 0.40f),
                    0.0f,
                    0.26f + reopening_reform_intensity * 0.12f
                );
                const int outer_post_approach_carry =
                    far_sidewalk_approach &&
                    !hot_nearby &&
                    !reclaiming_pocket_outside &&
                    !retaking_edge &&
                    !reforming_edge &&
                    !hardening_edge &&
                    !screening_edge &&
                    reopening_reform_intensity <= 0.12f &&
                    preferred_side_amount > 0.18f &&
                    state->territory_reentry_timer <= 0.0f &&
                    state->territory_reapproach_timer <= 0.05f &&
                    state->territory_resolve_timer <= 0.05f &&
                    state->combat_hostile_search_timer <= 0.0f &&
                    state->street_recovery_timer <= 0.0f &&
                    state->street_recovery_level <= 0.04f &&
                    state->street_incident_timer <= 0.0f &&
                    state->street_incident_level <= 0.06f &&
                    watch_carry > 0.0f &&
                    watch_carry < 0.30f &&
                    state->territory_heat > 0.03f &&
                    state->territory_heat < 0.14f &&
                    (state->territory_patrol_state == MDTBTerritoryPatrolIdle ||
                     state->territory_patrol_state == MDTBTerritoryPatrolCooldown ||
                     state->territory_patrol_state == MDTBTerritoryPatrolWatch ||
                     previous_front_watch > 0.08f ||
                     previous_heat > 0.06f);
                const float outer_post_approach_blend =
                    outer_post_approach_carry
                    ? clampf(
                        preferred_side_amount * 0.30f +
                        sidewalk_watch_proximity * 0.16f +
                        watch_carry * 0.26f +
                        previous_front_watch * 0.10f +
                        previous_heat * 0.12f,
                        0.0f,
                        0.68f
                    )
                    : 0.0f;
                const MDTBFloat3 outer_post_approach_position = lerp_float3(
                    kTerritoryPatrolBasePosition,
                    patrol_watch_position,
                    clampf(
                        0.08f +
                        sidewalk_watch_proximity * 0.18f +
                        outer_post_approach_blend * 0.18f,
                        0.0f,
                        0.28f
                    )
                );
                const MDTBFloat3 outer_post_approach_focus = offset_point(
                    kTerritoryPatrolLinePosition,
                    preferred_side_bias * (0.22f + preferred_side_amount * 0.30f),
                    0.0f,
                    -0.28f + outer_post_approach_blend * 0.06f
                );
                const int cold_watch_shoulder_carry =
                    near_sidewalk_watch &&
                    !hot_nearby &&
                    !reclaiming_pocket_outside &&
                    !retaking_edge &&
                    !reforming_edge &&
                    !hardening_edge &&
                    !screening_edge &&
                    reopening_reform_intensity <= 0.12f &&
                    preferred_side_amount > 0.18f &&
                    state->territory_reentry_timer <= 0.0f &&
                    state->territory_reapproach_timer <= 0.05f &&
                    state->territory_resolve_timer <= 0.05f &&
                    state->combat_hostile_search_timer <= 0.0f &&
                    state->street_recovery_timer <= 0.0f &&
                    state->street_recovery_level <= 0.04f &&
                    state->street_incident_timer <= 0.0f &&
                    state->street_incident_level <= 0.06f &&
                    watch_carry > 0.0f &&
                    watch_carry < 0.36f &&
                    state->territory_heat > 0.03f &&
                    state->territory_heat < 0.18f &&
                    (state->territory_patrol_state == MDTBTerritoryPatrolCooldown ||
                     state->territory_patrol_state == MDTBTerritoryPatrolWatch ||
                     previous_front_watch > 0.10f ||
                     previous_heat > 0.08f);
                const float cold_watch_carry_blend =
                    cold_watch_shoulder_carry
                    ? clampf(
                        preferred_side_amount * 0.34f +
                        (1.0f - sidewalk_watch_proximity) * 0.08f +
                        watch_carry * 0.28f +
                        previous_front_watch * 0.12f +
                        previous_heat * 0.14f,
                        0.0f,
                        0.76f
                    )
                    : 0.0f;
                const float cold_watch_side_bias =
                    preferred_side_amount > 0.18f
                    ? preferred_side_bias
                    : remembered_lateral_bias;
                const float cold_watch_side_amount = fabsf(cold_watch_side_bias);
                const MDTBFloat3 cold_watch_patrol_position = offset_point(
                    patrol_watch_position,
                    cold_watch_side_bias * (0.26f + cold_watch_side_amount * 0.38f),
                    0.0f,
                    -0.14f + cold_watch_carry_blend * 0.08f
                );
                const MDTBFloat3 cold_watch_patrol_focus = offset_point(
                    kTerritoryPatrolLinePosition,
                    cold_watch_side_bias * (0.32f + cold_watch_side_amount * 0.34f),
                    0.0f,
                    -0.12f + cold_watch_carry_blend * 0.06f
                );
                const int cold_start_shoulder_carry =
                    near_sidewalk_watch &&
                    !hot_nearby &&
                    !reclaiming_pocket_outside &&
                    !retaking_edge &&
                    reopening_reform_intensity <= 0.18f &&
                    preferred_side_amount > 0.18f &&
                    state->street_recovery_timer <= 0.0f &&
                    state->street_recovery_level <= 0.04f &&
                    state->street_incident_timer <= 0.0f &&
                    state->street_incident_level <= 0.06f &&
                    watch_carry > 0.0f &&
                    watch_carry < 0.42f &&
                    state->territory_heat > 0.04f &&
                    state->territory_heat < 0.24f &&
                    (state->territory_patrol_state == MDTBTerritoryPatrolCooldown ||
                     state->territory_patrol_state == MDTBTerritoryPatrolWatch ||
                     state->territory_inner_state == MDTBTerritoryPatrolCooldown ||
                     state->territory_inner_state == MDTBTerritoryPatrolWatch ||
                     state->territory_patrol_alert > 0.08f ||
                     state->territory_inner_alert > 0.06f);
                const float cold_start_carry_blend =
                    cold_start_shoulder_carry
                    ? clampf(
                        preferred_side_amount * 0.42f +
                        sidewalk_watch_proximity * 0.22f +
                        watch_carry * 0.28f +
                        fmaxf(state->territory_patrol_alert, state->territory_inner_alert) * 0.18f,
                        0.0f,
                        0.88f
                    )
                    : 0.0f;
                const float cold_start_side_bias =
                    preferred_side_amount > 0.18f
                    ? preferred_side_bias
                    : patrol_side_bias;
                const float cold_start_side_amount = fabsf(cold_start_side_bias);
                const MDTBFloat3 cold_start_patrol_screen_position = offset_point(
                    patrol_screen_position,
                    cold_start_side_bias * (0.34f + cold_start_side_amount * 0.42f),
                    0.0f,
                    0.08f + cold_start_carry_blend * 0.10f
                );
                const MDTBFloat3 cold_start_patrol_brace_position = offset_point(
                    patrol_brace_position,
                    cold_start_side_bias * (0.28f + cold_start_side_amount * 0.36f),
                    0.0f,
                    0.10f + cold_start_carry_blend * 0.10f
                );
                const MDTBFloat3 cold_start_patrol_watch_position = lerp_float3(
                    patrol_watch_position,
                    cold_start_patrol_screen_position,
                    clampf(0.12f + cold_start_carry_blend * 0.24f, 0.0f, 0.46f)
                );
                const MDTBFloat3 cold_start_patrol_focus = offset_point(
                    boundary_focus_position,
                    cold_start_side_bias * (0.42f + cold_start_side_amount * 0.40f),
                    0.0f,
                    0.10f + cold_start_carry_blend * 0.04f
                );
                const MDTBFloat3 cold_start_inner_screen_position = offset_point(
                    inner_screen_support_position,
                    -cold_start_side_bias * (0.24f + cold_start_side_amount * 0.32f),
                    0.0f,
                    0.10f + cold_start_carry_blend * 0.12f
                );
                const MDTBFloat3 cold_start_inner_brace_position = offset_point(
                    inner_brace_position,
                    -cold_start_side_bias * (0.20f + cold_start_side_amount * 0.30f),
                    0.0f,
                    0.08f + cold_start_carry_blend * 0.10f
                );
                const MDTBFloat3 cold_start_inner_focus = offset_point(
                    boundary_focus_position,
                    -cold_start_side_bias * (0.48f + cold_start_side_amount * 0.34f),
                    0.0f,
                    0.34f + cold_start_carry_blend * 0.12f
                );

                if (reclaiming_pocket_outside) {
                    const float reclaim_blend = clampf(
                        0.16f +
                        sidewalk_watch_proximity * 0.20f +
                        state->territory_deep_watch * 0.18f +
                        watch_carry * 0.12f +
                        reclaim_return_intensity * 0.10f +
                        outside_reclaim_counter_step * 0.08f,
                        0.0f,
                        0.52f
                    );

                    patrol_target_state = MDTBTerritoryPatrolReform;
                    patrol_target_position = lerp_float3(
                        patrol_clear_position,
                        outside_reclaim_patrol_position,
                        reclaim_blend
                    );
                    patrol_target_alert =
                        0.22f +
                        sidewalk_watch_proximity * 0.14f +
                        state->territory_deep_watch * 0.10f +
                        watch_carry * 0.08f +
                        reclaim_return_intensity * 0.08f +
                        outside_reclaim_counter_step * 0.04f;
                    patrol_speed = 2.9f + outside_reclaim_counter_step * 0.22f;
                    patrol_heading_focus = outside_reclaim_patrol_focus;
                } else if (retaking_edge) {
                    const float brace_blend = clampf(
                        0.54f +
                        sidewalk_watch_proximity * 0.18f +
                        state->territory_front_watch * 0.12f +
                        watch_carry * 0.12f +
                        retake_return_intensity * 0.12f +
                        outside_retake_cross_angle * 0.10f,
                        0.0f,
                        0.92f
                    );

                    patrol_target_state = MDTBTerritoryPatrolBrace;
                    patrol_target_position = lerp_float3(
                        patrol_screen_position,
                        outside_retake_patrol_position,
                        brace_blend
                    );
                    patrol_target_alert =
                        0.34f +
                        sidewalk_watch_proximity * 0.18f +
                        state->territory_front_watch * 0.10f +
                        watch_carry * 0.10f +
                        retake_return_intensity * 0.10f +
                        outside_retake_cross_angle * 0.04f +
                        (current_vehicle_entry ? 0.04f : 0.0f);
                    patrol_speed = 3.1f + outside_retake_cross_angle * 0.22f;
                    patrol_heading_focus = outside_retake_patrol_focus;
                } else if (hardening_edge) {
                    const float brace_blend = clampf(
                        0.34f +
                        sidewalk_watch_proximity * 0.20f +
                        watch_carry * 0.14f,
                        0.0f,
                        0.82f
                    );

                    patrol_target_state = MDTBTerritoryPatrolBrace;
                    patrol_target_position = lerp_float3(
                        patrol_screen_position,
                        cold_start_shoulder_carry ? cold_start_patrol_brace_position : patrol_brace_position,
                        brace_blend
                    );
                    patrol_target_alert =
                        0.28f +
                        sidewalk_watch_proximity * 0.20f +
                        state->territory_front_watch * 0.08f +
                        (current_vehicle_entry ? 0.04f : 0.0f) +
                        cold_start_carry_blend * 0.04f;
                    patrol_speed = 3.0f;
                    patrol_heading_focus = cold_start_shoulder_carry ? cold_start_patrol_focus : boundary_focus_position;
                } else if (reopening_reform_intensity > 0.18f &&
                           sidewalk_watch_proximity > 0.0f &&
                           !hot_nearby) {
                    const float reform_blend = clampf(
                        0.28f +
                        sidewalk_watch_proximity * 0.24f +
                        watch_carry * 0.12f +
                        reopening_reform_intensity * 0.24f,
                        0.0f,
                        0.86f
                    );

                    patrol_target_state = MDTBTerritoryPatrolReform;
                    patrol_target_position = lerp_float3(
                        patrol_clear_position,
                        reopening_patrol_position,
                        reform_blend
                    );
                    patrol_target_alert =
                        0.18f +
                        sidewalk_watch_proximity * 0.12f +
                        watch_carry * 0.06f +
                        reopening_reform_intensity * 0.10f +
                        (current_vehicle_entry ? 0.04f : 0.0f);
                    patrol_speed = 2.8f + reopening_reform_intensity * 0.14f;
                    patrol_heading_focus = reopening_patrol_focus;
                } else if (reforming_edge) {
                    const float reform_blend = clampf(
                        0.18f +
                        sidewalk_watch_proximity * 0.28f +
                        state->territory_front_watch * 0.10f +
                        watch_carry * 0.10f +
                        state->territory_deep_watch * 0.08f,
                        0.0f,
                        0.70f
                    );

                    patrol_target_state = MDTBTerritoryPatrolReform;
                    patrol_target_position = lerp_float3(
                        patrol_clear_position,
                        patrol_settle_position,
                        reform_blend
                    );
                    patrol_target_alert =
                        0.18f +
                        sidewalk_watch_proximity * 0.14f +
                        state->territory_front_watch * 0.06f +
                        watch_carry * 0.06f +
                        (current_vehicle_entry ? 0.04f : 0.0f);
                    patrol_speed = 2.7f;
                    patrol_heading_focus = boundary_focus_position;
                } else {
                    const MDTBFloat3 cold_watch_target_position =
                        cold_watch_shoulder_carry
                        ? cold_watch_patrol_position
                        : (cold_start_shoulder_carry
                            ? cold_start_patrol_watch_position
                            : patrol_watch_position);
                    const MDTBFloat3 screening_patrol_position = offset_point(
                        patrol_screen_position,
                        patrol_side_bias * (entry_clamp ? 0.42f : 0.0f),
                        0.0f,
                        entry_clamp ? 0.24f : 0.0f
                    );
                    patrol_target_state = screening_edge ? MDTBTerritoryPatrolScreen : MDTBTerritoryPatrolWatch;
                    patrol_target_position = screening_edge
                        ? lerp_float3(
                            screening_patrol_position,
                            cold_start_patrol_screen_position,
                            cold_start_carry_blend
                        )
                        : (outer_post_approach_carry
                            ? lerp_float3(
                                outer_post_approach_position,
                                cold_watch_target_position,
                                clampf(0.06f + outer_post_approach_blend * 0.10f, 0.0f, 0.22f)
                            )
                            : lerp_float3(
                                kTerritoryPatrolBasePosition,
                                cold_watch_target_position,
                                clampf(
                                    0.20f +
                                    sidewalk_watch_proximity * 0.80f +
                                    cold_start_carry_blend * 0.06f +
                                    cold_watch_carry_blend * 0.08f,
                                    0.0f,
                                    1.0f
                                )
                            ));
                    patrol_target_alert = screening_edge
                        ? ((entry_clamp ? 0.38f : 0.32f) +
                            sidewalk_watch_proximity * 0.32f +
                            (current_vehicle_entry ? 0.08f : 0.04f) +
                            cold_start_carry_blend * 0.04f)
                        : (outer_post_approach_carry
                            ? (0.12f +
                                sidewalk_watch_proximity * 0.12f +
                                outer_post_approach_blend * 0.04f +
                                (current_vehicle_entry ? 0.04f : 0.0f))
                            : (0.20f +
                                sidewalk_watch_proximity * 0.28f +
                                (current_vehicle_entry ? 0.06f : 0.0f) +
                                (hot_nearby ? 0.10f : 0.0f) +
                                cold_start_carry_blend * 0.03f +
                                cold_watch_carry_blend * 0.02f));
                    patrol_speed = screening_edge
                        ? (3.2f + cold_start_carry_blend * 0.12f)
                        : (outer_post_approach_carry
                            ? (2.4f + outer_post_approach_blend * 0.08f)
                            : (2.8f + cold_start_carry_blend * 0.10f + cold_watch_carry_blend * 0.08f));
                    patrol_heading_focus = screening_edge
                        ? (cold_start_shoulder_carry ? cold_start_patrol_focus : boundary_focus_position)
                        : (outer_post_approach_carry
                            ? outer_post_approach_focus
                            : (cold_watch_shoulder_carry
                            ? cold_watch_patrol_focus
                            : (cold_start_shoulder_carry ? cold_start_patrol_focus : player_position)));
                }

                if (retaking_edge) {
                    const float brace_blend = clampf(
                        0.30f +
                        sidewalk_watch_proximity * 0.18f +
                        watch_carry * 0.12f +
                        patrol_target_alert * 0.14f +
                        retake_return_intensity * 0.08f +
                        outside_retake_cross_angle * 0.08f,
                        0.0f,
                        0.78f
                    );

                    inner_target_state = MDTBTerritoryPatrolBrace;
                    inner_target_position = lerp_float3(
                        inner_screen_support_position,
                        outside_retake_inner_position,
                        brace_blend
                    );
                    inner_target_alert =
                        0.14f +
                        sidewalk_watch_proximity * 0.10f +
                        patrol_target_alert * 0.18f +
                        watch_carry * 0.06f +
                        retake_return_intensity * 0.06f +
                        outside_retake_cross_angle * 0.04f;
                    inner_speed = 2.6f + outside_retake_cross_angle * 0.18f;
                    inner_heading_focus = outside_retake_inner_focus;
                } else if (reclaiming_pocket_outside) {
                    inner_target_state = MDTBTerritoryPatrolHandoff;
                    inner_target_position = lerp_float3(
                        inner_pocket_position,
                        outside_reclaim_inner_position,
                        clampf(
                            0.22f +
                            sidewalk_watch_proximity * 0.14f +
                            state->territory_deep_watch * 0.18f +
                            patrol_target_alert * 0.10f +
                            reclaim_return_intensity * 0.10f +
                            outside_reclaim_counter_step * 0.08f,
                            0.0f,
                            0.64f
                        )
                    );
                    inner_target_alert =
                        0.14f +
                        sidewalk_watch_proximity * 0.08f +
                        state->territory_deep_watch * 0.08f +
                        patrol_target_alert * 0.12f +
                        reclaim_return_intensity * 0.06f +
                        outside_reclaim_counter_step * 0.04f;
                    inner_speed = 2.5f + outside_reclaim_counter_step * 0.18f;
                    inner_heading_focus = outside_reclaim_inner_focus;
                } else if (hot_nearby) {
                    inner_target_state = MDTBTerritoryPatrolWatch;
                    inner_target_position = lerp_float3(
                        inner_pocket_position,
                        inner_screen_support_position,
                        clampf(0.12f + sidewalk_watch_proximity * 0.22f, 0.0f, 0.34f)
                    );
                    inner_target_alert = 0.08f + sidewalk_watch_proximity * 0.12f;
                    inner_speed = 2.4f;
                } else if (hardening_edge) {
                    const float brace_blend = clampf(
                        0.22f +
                        sidewalk_watch_proximity * 0.18f +
                        watch_carry * 0.10f +
                        patrol_target_alert * 0.12f,
                        0.0f,
                        0.70f
                    );

                    inner_target_state = MDTBTerritoryPatrolBrace;
                    inner_target_position = lerp_float3(
                        inner_screen_support_position,
                        cold_start_shoulder_carry ? cold_start_inner_brace_position : inner_brace_position,
                        brace_blend
                    );
                    inner_target_alert =
                        0.10f +
                        sidewalk_watch_proximity * 0.10f +
                        patrol_target_alert * 0.16f +
                        (entry_clamp ? 0.04f : 0.0f) +
                        cold_start_carry_blend * 0.03f;
                    inner_speed = 2.5f;
                    inner_heading_focus = cold_start_shoulder_carry ? cold_start_inner_focus : boundary_focus_position;
                } else if (reopening_reform_intensity > 0.18f &&
                           sidewalk_watch_proximity > 0.0f &&
                           !hot_nearby) {
                    const float reform_blend = clampf(
                        0.18f +
                        sidewalk_watch_proximity * 0.14f +
                        patrol_target_alert * 0.12f +
                        reopening_reform_intensity * 0.18f,
                        0.0f,
                        0.62f
                    );

                    inner_target_state = MDTBTerritoryPatrolReform;
                    inner_target_position = lerp_float3(
                        inner_pocket_position,
                        reopening_inner_position,
                        reform_blend
                    );
                    inner_target_alert =
                        0.08f +
                        sidewalk_watch_proximity * 0.08f +
                        patrol_target_alert * 0.10f +
                        reopening_reform_intensity * 0.08f;
                    inner_speed = 2.1f + reopening_reform_intensity * 0.10f;
                    inner_heading_focus = reopening_inner_focus;
                } else if (reforming_edge) {
                    const float reform_blend = clampf(
                        0.10f +
                        sidewalk_watch_proximity * 0.16f +
                        state->territory_front_watch * 0.08f +
                        watch_carry * 0.08f +
                        patrol_target_alert * 0.12f,
                        0.0f,
                        0.46f
                    );

                    inner_target_state = MDTBTerritoryPatrolReform;
                    inner_target_position = lerp_float3(
                        inner_pocket_position,
                        inner_settle_position,
                        reform_blend
                    );
                    inner_target_alert =
                        0.06f +
                        sidewalk_watch_proximity * 0.08f +
                        patrol_target_alert * 0.10f +
                        watch_carry * 0.04f;
                    inner_speed = 2.0f;
                    inner_heading_focus = boundary_focus_position;
                } else if (screening_edge) {
                    const MDTBFloat3 screening_inner_position =
                        entry_clamp
                        ? offset_point(
                            inner_screen_support_position,
                            -patrol_side_bias * 0.62f,
                            0.0f,
                            0.30f
                        )
                        : inner_screen_support_position;
                    inner_target_state = MDTBTerritoryPatrolWatch;
                    inner_target_position = lerp_float3(
                        inner_pocket_position,
                        cold_start_shoulder_carry
                            ? lerp_float3(
                                screening_inner_position,
                                cold_start_inner_screen_position,
                                cold_start_carry_blend
                            )
                            : screening_inner_position,
                        clampf(
                            (entry_clamp ? 0.24f : 0.16f) +
                            sidewalk_watch_proximity * 0.12f +
                            patrol_target_alert * 0.10f +
                            cold_start_carry_blend * 0.06f,
                            0.0f,
                            0.40f
                        )
                    );
                    inner_target_alert =
                        (entry_clamp ? 0.10f : 0.06f) +
                        sidewalk_watch_proximity * 0.08f +
                        patrol_target_alert * 0.10f +
                        cold_start_carry_blend * 0.03f;
                    inner_speed = entry_clamp
                        ? (2.4f + cold_start_carry_blend * 0.12f)
                        : (2.0f + cold_start_carry_blend * 0.08f);
                    inner_heading_focus = cold_start_shoulder_carry ? cold_start_inner_focus : boundary_focus_position;
                }

                if (retaking_edge &&
                    preferred_side_amount > 0.22f &&
                    patrol_target_state == MDTBTerritoryPatrolBrace &&
                    inner_target_state == MDTBTerritoryPatrolBrace) {
                    const float reseal_blend = clampf(
                        0.24f +
                        outside_retake_side_amount * 0.34f +
                        sidewalk_watch_proximity * 0.20f +
                        watch_carry * 0.16f +
                        state->territory_front_watch * 0.14f,
                        0.0f,
                        0.90f
                    );
                    const float inner_reseal_blend = clampf(reseal_blend + 0.08f, 0.0f, 0.92f);

                    patrol_target_position = lerp_float3(
                        patrol_target_position,
                        outside_retake_reseal_patrol_position,
                        reseal_blend
                    );
                    patrol_heading_focus = lerp_float3(
                        patrol_heading_focus,
                        outside_retake_reseal_patrol_focus,
                        reseal_blend
                    );
                    patrol_target_alert += reseal_blend * 0.06f;
                    patrol_speed += reseal_blend * 0.18f;
                    inner_target_position = lerp_float3(
                        inner_target_position,
                        outside_retake_reseal_inner_position,
                        inner_reseal_blend
                    );
                    inner_heading_focus = lerp_float3(
                        inner_heading_focus,
                        outside_retake_reseal_inner_focus,
                        reseal_blend
                    );
                    inner_target_alert += reseal_blend * 0.04f;
                    inner_speed += reseal_blend * 0.16f;
                }
            }

            state->territory_phase = MDTBTerritoryPhaseNone;
            state->territory_presence = approachf(state->territory_presence, 0.0f, 3.4f, dt);
            state->territory_heat = approachf(state->territory_heat, 0.0f, state->territory_reentry_timer > 0.0f ? 0.16f : 0.44f, dt);
            state->territory_front_watch = approachf(
                state->territory_front_watch,
                0.0f,
                state->territory_watch_timer > 0.0f ? 0.22f : 0.58f,
                dt
            );
            state->territory_deep_watch = approachf(
                state->territory_deep_watch,
                0.0f,
                state->territory_watch_timer > 0.0f ? 0.20f : 0.54f,
                dt
            );

            if (state->territory_reentry_timer <= 0.0f && state->territory_heat <= 0.04f) {
                state->territory_faction = MDTBTerritoryFactionNone;
                state->territory_presence = 0.0f;
                state->territory_heat = 0.0f;
            }

            const int street_cooldown_active =
                (state->street_incident_timer > 0.0f && state->street_incident_level > 0.06f) ||
                (state->street_recovery_timer > 0.0f && state->street_recovery_level > 0.04f);

            if (state->territory_watch_timer <= 0.0f &&
                state->territory_front_watch <= 0.04f &&
                state->territory_deep_watch <= 0.04f &&
                state->territory_heat <= 0.04f &&
                !street_cooldown_active) {
                state->territory_entry_mode = MDTBTerritoryEntryNone;
                state->territory_preferred_side = 0.0f;
                state->territory_watch_timer = 0.0f;
                state->territory_front_watch = 0.0f;
                state->territory_deep_watch = 0.0f;
                if (state->territory_reapproach_timer <= 0.12f) {
                    state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
                    state->territory_reapproach_timer = 0.0f;
                }
            } else if (state->territory_watch_timer <= 0.0f &&
                       fmaxf(previous_front_watch, previous_deep_watch) > 0.12f) {
                state->territory_watch_timer = kTerritoryWatchCarryDuration * 0.28f;
            }
        }

        if (patrol_target_state == MDTBTerritoryPatrolIdle &&
            (state->territory_patrol_state != MDTBTerritoryPatrolIdle || state->territory_patrol_alert > 0.05f)) {
            const int late_fallback_active =
                !inside &&
                state->street_recovery_timer <= 0.0f &&
                state->street_recovery_level <= 0.04f &&
                state->street_incident_timer <= 0.0f &&
                state->street_incident_level <= 0.06f &&
                preferred_side_amount > 0.18f &&
                state->territory_watch_timer > 0.0f &&
                state->territory_heat > 0.04f &&
                (state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                 state->territory_patrol_state == MDTBTerritoryPatrolCooldown ||
                 state->territory_patrol_state == MDTBTerritoryPatrolWatch ||
                 state->territory_inner_state == MDTBTerritoryPatrolReform ||
                 state->territory_inner_state == MDTBTerritoryPatrolCooldown ||
                 state->territory_patrol_alert > 0.10f ||
                 state->territory_inner_alert > 0.08f ||
                 previous_heat > 0.12f);
            const float late_fallback_blend =
                late_fallback_active
                ? clampf(
                    preferred_side_amount * 0.46f +
                    state->territory_watch_timer / kTerritoryWatchCarryDuration * 0.38f +
                    fmaxf(state->territory_patrol_alert, state->territory_inner_alert) * 0.24f +
                    previous_heat * 0.20f,
                    0.0f,
                    0.92f
                )
                : 0.0f;
            const MDTBFloat3 late_fallback_patrol_position = offset_point(
                kTerritoryPatrolBasePosition,
                preferred_side_bias * (0.48f + preferred_side_amount * 0.72f),
                0.0f,
                -0.10f + late_fallback_blend * 0.12f
            );
            patrol_target_state = MDTBTerritoryPatrolCooldown;
            patrol_target_position = lerp_float3(
                kTerritoryPatrolBasePosition,
                late_fallback_patrol_position,
                late_fallback_blend
            );
            patrol_target_alert =
                0.08f +
                fminf(previous_heat, 0.30f) * 0.12f +
                late_fallback_blend * 0.05f;
            patrol_speed = 2.2f + late_fallback_blend * 0.16f;
            patrol_heading_focus = offset_point(
                kTerritoryPatrolLinePosition,
                preferred_side_bias * (0.72f + preferred_side_amount * 0.44f),
                0.0f,
                0.0f
            );
        }

        state->territory_patrol_state = patrol_target_state;
        state->territory_patrol_alert = approachf(
            state->territory_patrol_alert,
            clampf(patrol_target_alert, 0.0f, 1.0f),
            patrol_target_state == MDTBTerritoryPatrolIdle ? 1.2f : 3.2f,
            dt
        );
        state->territory_patrol_position = approach_float3(
            state->territory_patrol_position,
            patrol_target_position,
            patrol_speed,
            dt
        );

        {
            const MDTBFloat3 heading_target =
                patrol_target_state == MDTBTerritoryPatrolIdle
                ? kTerritoryPatrolLinePosition
                : (patrol_target_state == MDTBTerritoryPatrolCooldown
                    ? patrol_heading_focus
                    : patrol_heading_focus);
            float desired_heading = state->territory_patrol_heading;

            if (distance_squared_xz(state->territory_patrol_position, heading_target) > 0.04f) {
                desired_heading = atan2f(
                    heading_target.x - state->territory_patrol_position.x,
                    heading_target.z - state->territory_patrol_position.z
                );
            }

            state->territory_patrol_heading = approach_angle(state->territory_patrol_heading, desired_heading, 6.2f, dt);
        }

        if (inner_target_state == MDTBTerritoryPatrolIdle &&
            (state->territory_inner_state != MDTBTerritoryPatrolIdle || state->territory_inner_alert > 0.05f)) {
            const int late_fallback_active =
                !inside &&
                state->street_recovery_timer <= 0.0f &&
                state->street_recovery_level <= 0.04f &&
                state->street_incident_timer <= 0.0f &&
                state->street_incident_level <= 0.06f &&
                preferred_side_amount > 0.18f &&
                state->territory_watch_timer > 0.0f &&
                state->territory_heat > 0.04f &&
                (state->territory_patrol_state == MDTBTerritoryPatrolReform ||
                 state->territory_patrol_state == MDTBTerritoryPatrolCooldown ||
                 state->territory_inner_state == MDTBTerritoryPatrolReform ||
                 state->territory_inner_state == MDTBTerritoryPatrolCooldown ||
                 state->territory_patrol_alert > 0.10f ||
                 state->territory_inner_alert > 0.08f ||
                 previous_heat > 0.12f);
            const float late_fallback_blend =
                late_fallback_active
                ? clampf(
                    preferred_side_amount * 0.42f +
                    state->territory_watch_timer / kTerritoryWatchCarryDuration * 0.34f +
                    fmaxf(state->territory_patrol_alert, state->territory_inner_alert) * 0.22f +
                    previous_heat * 0.18f,
                    0.0f,
                    0.90f
                )
                : 0.0f;
            const MDTBFloat3 late_fallback_inner_position = offset_point(
                kTerritoryInnerBasePosition,
                -preferred_side_bias * (0.34f + preferred_side_amount * 0.62f),
                0.0f,
                0.08f + late_fallback_blend * 0.12f
            );
            inner_target_state = MDTBTerritoryPatrolCooldown;
            inner_target_position = lerp_float3(
                kTerritoryInnerBasePosition,
                late_fallback_inner_position,
                late_fallback_blend
            );
            inner_target_alert =
                0.06f +
                fminf(previous_heat, 0.26f) * 0.10f +
                late_fallback_blend * 0.04f;
            inner_speed = 2.2f + late_fallback_blend * 0.14f;
            inner_heading_focus = offset_point(
                patrol_heading_focus,
                -preferred_side_bias * (0.42f + preferred_side_amount * 0.30f),
                0.0f,
                0.34f + late_fallback_blend * 0.10f
            );
        }

        state->territory_inner_state = inner_target_state;
        state->territory_inner_alert = approachf(
            state->territory_inner_alert,
            clampf(inner_target_alert, 0.0f, 1.0f),
            inner_target_state == MDTBTerritoryPatrolIdle ? 1.1f : 3.0f,
            dt
        );
        state->territory_inner_position = approach_float3(
            state->territory_inner_position,
            inner_target_position,
            inner_speed,
            dt
        );

        {
            const MDTBFloat3 heading_target =
                inner_target_state == MDTBTerritoryPatrolIdle
                ? state->territory_patrol_position
                : (inner_target_state == MDTBTerritoryPatrolCooldown
                    ? inner_heading_focus
                    : inner_heading_focus);
            float desired_heading = state->territory_inner_heading;

            if (distance_squared_xz(state->territory_inner_position, heading_target) > 0.04f) {
                desired_heading = atan2f(
                    heading_target.x - state->territory_inner_position.x,
                    heading_target.z - state->territory_inner_position.z
                );
            }

            state->territory_inner_heading = approach_angle(state->territory_inner_heading, desired_heading, 5.8f, dt);
        }

        if (!inside && state->territory_patrol_alert > 0.12f) {
            const float sidewalk_watch_scale =
                near_sidewalk_watch
                ? 0.48f
                : (far_sidewalk_approach ? 0.16f : 0.24f);
            const float sidewalk_watch = state->territory_patrol_alert * sidewalk_watch_scale;
            state->territory_front_watch = fmaxf(state->territory_front_watch, sidewalk_watch);
            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                kTerritoryWatchCarryDuration * (near_sidewalk_watch ? 0.44f : (far_sidewalk_approach ? 0.16f : 0.24f))
            );
        }

        if (!inside && edge_retaking) {
            state->territory_front_watch = fmaxf(
                state->territory_front_watch,
                0.34f + sidewalk_watch_proximity * 0.14f
            );
            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                kTerritoryWatchCarryDuration * 0.56f
            );
        }

        if (!inside && retake_returning) {
            state->territory_front_watch = fmaxf(
                state->territory_front_watch,
                0.24f + sidewalk_watch_proximity * 0.12f + retake_return_intensity * 0.08f
            );
            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                kTerritoryWatchCarryDuration * 0.34f
            );
        }

        if (!inside && pocket_claiming) {
            state->territory_deep_watch = fmaxf(
                state->territory_deep_watch,
                0.28f + state->territory_inner_alert * 0.22f
            );
            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                kTerritoryWatchCarryDuration * 0.42f
            );
        }

        if (!inside && reclaim_returning) {
            state->territory_deep_watch = fmaxf(
                state->territory_deep_watch,
                0.22f + state->territory_inner_alert * 0.16f + reclaim_return_intensity * 0.08f
            );
            state->territory_watch_timer = fmaxf(
                state->territory_watch_timer,
                kTerritoryWatchCarryDuration * 0.30f
            );
        }

        if (!inside && hot_nearby && state->territory_inner_alert > 0.10f) {
            state->territory_deep_watch = fmaxf(state->territory_deep_watch, state->territory_inner_alert * 0.18f);
            state->territory_watch_timer = fmaxf(state->territory_watch_timer, kTerritoryWatchCarryDuration * 0.22f);
        }

        {
            const float commit_hold_duration =
                state->territory_entry_mode == MDTBTerritoryEntryVehicle ? 1.35f : 1.10f;
            const float resolve_hold_duration =
                state->territory_entry_mode == MDTBTerritoryEntryVehicle ? 1.20f : 1.02f;
            const float resolve_pullout_duration =
                state->territory_entry_mode == MDTBTerritoryEntryVehicle ? 1.08f : 0.92f;
            const int commit_window_candidate =
                state->player_reset_timer <= 0.0f &&
                state->combat_hostile_search_timer <= 0.0f &&
                state->territory_phase != MDTBTerritoryPhaseHot &&
                state->territory_patrol_state == MDTBTerritoryPatrolBrace &&
                state->territory_patrol_alert > 0.18f &&
                state->territory_deep_watch <= (state->territory_front_watch + (retake_returning ? 0.14f : 0.18f));
            const int commit_push_candidate =
                inside &&
                state->territory_faction == MDTBTerritoryFactionCourtSet &&
                state->territory_phase != MDTBTerritoryPhaseBoundary &&
                (state->territory_presence > (reclaim_returning ? 0.54f : 0.60f) ||
                 state->territory_deep_watch > (state->territory_front_watch + (reclaim_returning ? 0.02f : 0.06f)) ||
                 state->territory_patrol_state == MDTBTerritoryPatrolHandoff ||
                 state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                 state->territory_inner_state == MDTBTerritoryPatrolHandoff);
            const float commit_depth = inside
                ? clampf(
                    (player_position.z - (kCourtSetTerritoryEntryZ + 0.30f)) /
                    fmaxf(kCourtSetTerritoryCoreZ - kCourtSetTerritoryEntryZ - 0.30f, 0.01f),
                    0.0f,
                    1.0f
                )
                : 0.0f;
            const float commit_rate =
                0.62f +
                commit_depth * 0.42f +
                state->territory_presence * 0.22f +
                fmaxf(0.0f, state->territory_deep_watch - state->territory_front_watch) * 0.30f +
                ((state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                  state->territory_patrol_state == MDTBTerritoryPatrolHandoff) ? 0.18f : 0.0f) +
                (state->territory_inner_state == MDTBTerritoryPatrolHandoff ? 0.10f : 0.0f) +
                reclaim_return_intensity * 0.10f -
                retake_return_intensity * 0.04f;
            const float resolve_exit_depth = clampf(
                ((kCourtSetTerritoryEntryZ + 0.45f) - player_position.z) / 2.8f,
                0.0f,
                1.0f
            );
            const int resolve_hold_candidate =
                inside &&
                state->territory_faction == MDTBTerritoryFactionCourtSet &&
                state->territory_phase == MDTBTerritoryPhaseClaimed &&
                (state->territory_presence > (reclaim_returning ? 0.62f : 0.68f) ||
                 state->territory_deep_watch > (state->territory_front_watch + (reclaim_returning ? 0.06f : 0.10f)) ||
                 state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                 state->territory_patrol_state == MDTBTerritoryPatrolHandoff ||
                 state->territory_inner_state == MDTBTerritoryPatrolHandoff);
            const int resolve_pullout_candidate =
                state->territory_faction == MDTBTerritoryFactionCourtSet &&
                ((!inside && player_position.z <= (kCourtSetTerritoryEntryZ + 0.28f)) ||
                 (state->territory_phase == MDTBTerritoryPhaseBoundary &&
                  state->territory_presence < 0.58f &&
                  player_position.z <= (kCourtSetTerritoryEntryZ + 0.72f)));
            const float resolve_hold_rate =
                0.74f +
                state->territory_presence * 0.18f +
                fmaxf(0.0f, state->territory_deep_watch - state->territory_front_watch) * 0.26f +
                ((state->territory_patrol_state == MDTBTerritoryPatrolClear ||
                  state->territory_inner_state == MDTBTerritoryPatrolHandoff) ? 0.12f : 0.0f) +
                reclaim_return_intensity * 0.08f;
            const float resolve_pullout_rate =
                0.84f +
                resolve_exit_depth * 0.32f +
                (state->territory_entry_mode == MDTBTerritoryEntryVehicle ? 0.08f : 0.0f) +
                (state->combat_hostile_search_timer > 0.0f ? 0.10f : 0.0f) +
                retake_return_intensity * 0.10f;

            if (state->territory_phase == MDTBTerritoryPhaseHot || state->player_reset_timer > 0.0f) {
                state->territory_commit_state = MDTBTerritoryCommitNone;
                state->territory_commit_timer = 0.0f;
                state->territory_commit_progress = 0.0f;
                state->territory_resolve_state = MDTBTerritoryResolveNone;
                state->territory_resolve_timer = 0.0f;
                state->territory_resolve_progress = 0.0f;
                state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
                state->territory_reapproach_timer = 0.0f;
            } else {
                switch (state->territory_commit_state) {
                    case MDTBTerritoryCommitWindow:
                        if (commit_push_candidate) {
                            state->territory_commit_state = MDTBTerritoryCommitActive;
                            state->territory_commit_progress = fmaxf(
                                state->territory_commit_progress,
                                0.16f + commit_depth * 0.14f
                            );
                            state->territory_commit_timer = fmaxf(
                                (1.0f - state->territory_commit_progress) * commit_hold_duration,
                                0.28f
                            );
                        } else if (commit_window_candidate) {
                            state->territory_commit_timer = fmaxf(
                                state->territory_commit_timer,
                                0.95f + state->territory_patrol_alert * 0.90f
                            );
                            state->territory_commit_progress = approachf(
                                state->territory_commit_progress,
                                0.0f,
                                2.8f,
                                dt
                            );
                        } else {
                            state->territory_commit_timer = fmaxf(state->territory_commit_timer - dt, 0.0f);
                            state->territory_commit_progress = approachf(
                                state->territory_commit_progress,
                                0.0f,
                                3.4f,
                                dt
                            );

                            if (state->territory_commit_timer <= 0.0f) {
                                state->territory_commit_state = MDTBTerritoryCommitNone;
                                state->territory_commit_progress = 0.0f;
                            }
                        }
                        break;
                    case MDTBTerritoryCommitActive:
                        if (commit_push_candidate) {
                            state->territory_commit_progress = clampf(
                                state->territory_commit_progress + (dt * commit_rate / commit_hold_duration),
                                0.0f,
                                1.0f
                            );
                            state->territory_commit_timer = fmaxf(
                                (1.0f - state->territory_commit_progress) * commit_hold_duration,
                                0.0f
                            );

                            if (state->territory_commit_progress >= 0.999f) {
                                state->territory_commit_state = MDTBTerritoryCommitComplete;
                                state->territory_commit_timer = 2.4f;
                                state->territory_commit_progress = 1.0f;
                                state->territory_resolve_state = MDTBTerritoryResolveWindow;
                                state->territory_resolve_timer = 1.30f;
                                state->territory_resolve_progress = 0.0f;
                                state->territory_heat = fmaxf(state->territory_heat, 0.48f);
                                state->territory_watch_timer = fmaxf(state->territory_watch_timer, kTerritoryWatchCarryDuration * 0.54f);

                                if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
                                    state->combat_hostile_alert = fmaxf(state->combat_hostile_alert, 0.84f);
                                    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, 0.28f);
                                }

                                trigger_street_incident(state, player_position, 0.28f, kStreetIncidentNoticeDuration * 0.72f);
                            }
                        } else if (commit_window_candidate) {
                            state->territory_commit_state = MDTBTerritoryCommitWindow;
                            state->territory_commit_timer = fmaxf(
                                0.68f,
                                (1.0f - state->territory_commit_progress) * commit_hold_duration
                            );
                            state->territory_commit_progress = fmaxf(
                                state->territory_commit_progress - dt * 0.30f,
                                0.0f
                            );
                        } else {
                            state->territory_commit_progress = fmaxf(
                                state->territory_commit_progress - dt * 1.4f,
                                0.0f
                            );
                            state->territory_commit_timer = fmaxf(
                                (1.0f - state->territory_commit_progress) * commit_hold_duration,
                                0.0f
                            );

                            if (state->territory_commit_progress <= 0.05f) {
                                state->territory_commit_state = MDTBTerritoryCommitNone;
                                state->territory_commit_timer = 0.0f;
                                state->territory_commit_progress = 0.0f;
                            }
                        }
                        break;
                    case MDTBTerritoryCommitComplete:
                        state->territory_commit_timer = fmaxf(state->territory_commit_timer - dt, 0.0f);
                        state->territory_commit_progress = 1.0f;
                        if (state->territory_commit_timer <= 0.0f) {
                            state->territory_commit_state = MDTBTerritoryCommitNone;
                            state->territory_commit_progress = 0.0f;
                        }
                        break;
                    case MDTBTerritoryCommitNone:
                    default:
                        if (commit_window_candidate) {
                            state->territory_commit_state = MDTBTerritoryCommitWindow;
                            state->territory_commit_timer = 1.25f + state->territory_patrol_alert * 1.10f;
                            state->territory_commit_progress = 0.0f;
                        } else {
                            state->territory_commit_state = MDTBTerritoryCommitNone;
                            state->territory_commit_timer = 0.0f;
                            state->territory_commit_progress = 0.0f;
                        }
                        break;
                }

                if (state->territory_resolve_state == MDTBTerritoryResolveNone &&
                    state->territory_commit_state == MDTBTerritoryCommitComplete) {
                    state->territory_resolve_state = MDTBTerritoryResolveWindow;
                    state->territory_resolve_timer = fmaxf(state->territory_commit_timer, 1.10f);
                    state->territory_resolve_progress = 0.0f;
                }

                switch (state->territory_resolve_state) {
                    case MDTBTerritoryResolveWindow:
                        if (resolve_hold_candidate) {
                            state->territory_resolve_state = MDTBTerritoryResolveHold;
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress,
                                0.18f + commit_depth * 0.10f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_hold_duration,
                                0.30f
                            );
                        } else if (resolve_pullout_candidate) {
                            state->territory_resolve_state = MDTBTerritoryResolvePullout;
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress,
                                0.20f + resolve_exit_depth * 0.10f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_pullout_duration,
                                0.26f
                            );
                        } else if (state->territory_commit_state == MDTBTerritoryCommitComplete) {
                            state->territory_resolve_timer = fmaxf(
                                state->territory_resolve_timer,
                                0.88f + state->territory_commit_timer * 0.34f
                            );
                        } else {
                            state->territory_resolve_timer = fmaxf(state->territory_resolve_timer - dt, 0.0f);
                            state->territory_resolve_progress = approachf(
                                state->territory_resolve_progress,
                                0.0f,
                                3.4f,
                                dt
                            );

                            if (state->territory_resolve_timer <= 0.0f) {
                                state->territory_resolve_state = MDTBTerritoryResolveNone;
                                state->territory_resolve_progress = 0.0f;
                            }
                        }
                        break;
                    case MDTBTerritoryResolveHold:
                        if (state->territory_resolve_progress >= 0.999f) {
                            state->territory_resolve_timer = fmaxf(state->territory_resolve_timer - dt, 0.0f);
                            if (state->territory_resolve_timer <= 0.0f) {
                                state->territory_resolve_state = MDTBTerritoryResolveNone;
                                state->territory_resolve_progress = 0.0f;
                            }
                        } else if (resolve_hold_candidate) {
                            state->territory_resolve_progress = clampf(
                                state->territory_resolve_progress + (dt * resolve_hold_rate / resolve_hold_duration),
                                0.0f,
                                1.0f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_hold_duration,
                                0.0f
                            );

                            if (state->territory_resolve_progress >= 0.999f) {
                                const MDTBFloat3 reclaim_focus_position = territory_shoulder_focus_position(
                                    state,
                                    player_position,
                                    1.35f,
                                    0.62f
                                );
                                state->territory_resolve_progress = 1.0f;
                                state->territory_resolve_timer = 1.35f;
                                state->territory_reapproach_mode = MDTBTerritoryReapproachReclaim;
                                state->territory_reapproach_timer =
                                    state->territory_entry_mode == MDTBTerritoryEntryVehicle
                                    ? kTerritoryReapproachReclaimDuration + 0.6f
                                    : kTerritoryReapproachReclaimDuration;
                                state->territory_heat = fmaxf(state->territory_heat, 0.56f);
                                state->territory_watch_timer = fmaxf(state->territory_watch_timer, kTerritoryWatchCarryDuration * 0.76f);
                                state->territory_deep_watch = fmaxf(state->territory_deep_watch, 0.62f);

                                if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
                                    state->combat_hostile_alert = fmaxf(state->combat_hostile_alert, 0.92f);
                                    state->combat_hostile_reposition_timer = 0.0f;
                                    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, 0.46f);
                                }

                                trigger_street_incident(
                                    state,
                                    reclaim_focus_position,
                                    0.34f,
                                    kStreetIncidentNoticeDuration * 0.84f
                                );
                            }
                        } else if (resolve_pullout_candidate && state->territory_resolve_progress < 0.34f) {
                            state->territory_resolve_state = MDTBTerritoryResolvePullout;
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress,
                                0.18f + resolve_exit_depth * 0.08f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_pullout_duration,
                                0.26f
                            );
                        } else {
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress - dt * 1.10f,
                                0.0f
                            );

                            if (state->territory_resolve_progress <= 0.05f) {
                                state->territory_resolve_state = MDTBTerritoryResolveWindow;
                                state->territory_resolve_timer = fmaxf(
                                    state->territory_commit_timer,
                                    0.54f
                                );
                                state->territory_resolve_progress = 0.0f;
                            } else {
                                state->territory_resolve_timer = fmaxf(
                                    (1.0f - state->territory_resolve_progress) * resolve_hold_duration,
                                    0.0f
                                );
                            }
                        }
                        break;
                    case MDTBTerritoryResolvePullout:
                        if (state->territory_resolve_progress >= 0.999f) {
                            state->territory_resolve_timer = fmaxf(state->territory_resolve_timer - dt, 0.0f);
                            if (state->territory_resolve_timer <= 0.0f) {
                                state->territory_resolve_state = MDTBTerritoryResolveNone;
                                state->territory_resolve_progress = 0.0f;
                            }
                        } else if (resolve_pullout_candidate) {
                            state->territory_resolve_progress = clampf(
                                state->territory_resolve_progress + (dt * resolve_pullout_rate / resolve_pullout_duration),
                                0.0f,
                                1.0f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_pullout_duration,
                                0.0f
                            );

                            if (state->territory_resolve_progress >= 0.999f) {
                                const MDTBFloat3 retake_focus_position = territory_shoulder_focus_position(
                                    state,
                                    player_position,
                                    1.20f,
                                    0.12f
                                );
                                state->territory_resolve_progress = 1.0f;
                                state->territory_resolve_timer = 1.20f;
                                state->territory_reapproach_mode = MDTBTerritoryReapproachRetake;
                                state->territory_reapproach_timer =
                                    state->territory_entry_mode == MDTBTerritoryEntryVehicle
                                    ? kTerritoryReapproachRetakeDuration + 0.5f
                                    : kTerritoryReapproachRetakeDuration;
                                state->territory_watch_timer = fmaxf(
                                    state->territory_watch_timer,
                                    kTerritoryWatchCarryDuration * 0.46f
                                );
                                state->territory_front_watch = fmaxf(
                                    state->territory_front_watch,
                                    0.28f + resolve_exit_depth * 0.12f
                                );

                                if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
                                    state->combat_hostile_reposition_timer = 0.0f;
                                    start_hostile_search(
                                        state,
                                        retake_focus_position,
                                        state->territory_entry_mode == MDTBTerritoryEntryVehicle
                                            ? kLookoutVehicleReacquireDuration
                                            : kLookoutSearchSettleDuration
                                    );
                                }

                                trigger_street_recovery(
                                    state,
                                    retake_focus_position,
                                    0.24f + resolve_exit_depth * 0.12f,
                                    kStreetRecoveryDuration * 0.72f
                                );
                            }
                        } else if (resolve_hold_candidate && state->territory_resolve_progress < 0.34f) {
                            state->territory_resolve_state = MDTBTerritoryResolveHold;
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress,
                                0.18f + commit_depth * 0.08f
                            );
                            state->territory_resolve_timer = fmaxf(
                                (1.0f - state->territory_resolve_progress) * resolve_hold_duration,
                                0.30f
                            );
                        } else {
                            state->territory_resolve_progress = fmaxf(
                                state->territory_resolve_progress - dt * 1.10f,
                                0.0f
                            );

                            if (state->territory_resolve_progress <= 0.05f) {
                                state->territory_resolve_state = MDTBTerritoryResolveWindow;
                                state->territory_resolve_timer = fmaxf(
                                    state->territory_commit_timer,
                                    0.48f
                                );
                                state->territory_resolve_progress = 0.0f;
                            } else {
                                state->territory_resolve_timer = fmaxf(
                                    (1.0f - state->territory_resolve_progress) * resolve_pullout_duration,
                                    0.0f
                                );
                            }
                        }
                        break;
                    case MDTBTerritoryResolveNone:
                    default:
                        state->territory_resolve_state = MDTBTerritoryResolveNone;
                        state->territory_resolve_timer = 0.0f;
                        state->territory_resolve_progress = 0.0f;
                        break;
                }
            }
        }
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

static MDTBFloat3 combat_target_shot_position_for_kind(const MDTBEngineState *state, uint32_t target_kind) {
    MDTBFloat3 position = combat_target_position_for_kind(state, target_kind);

    switch (target_kind) {
        case MDTBCombatTargetDummy:
            position.y += 1.02f;
            break;
        case MDTBCombatTargetLookout:
            position.y += 1.10f;
            break;
        default:
            break;
    }

    return position;
}

static MDTBFloat3 hostile_shot_origin(const MDTBEngineState *state) {
    MDTBFloat3 origin = combat_target_shot_position_for_kind(state, MDTBCombatTargetLookout);
    const MDTBFloat3 muzzle_right = make_float3(cosf(state->combat_hostile_heading), 0.0f, sinf(state->combat_hostile_heading));

    origin.x += muzzle_right.x * 0.22f;
    origin.z += muzzle_right.z * 0.22f;
    origin.y += 0.08f;
    return origin;
}

static MDTBFloat3 player_cover_position(const MDTBEngineState *state) {
    MDTBFloat3 position = current_player_position(state);

    if (state != NULL && state->traversal_mode == MDTBTraversalModeOnFoot) {
        position.y = state->actor_ground_height + 1.02f;
    }

    return position;
}

static int segment_intersects_box(MDTBFloat3 start, MDTBFloat3 end, const MDTBBox *box, float padding, float *hit_t_out) {
    const MDTBFloat3 direction = make_float3(end.x - start.x, end.y - start.y, end.z - start.z);
    float t_min = 0.0f;
    float t_max = 1.0f;

    if (box == NULL) {
        return 0;
    }

    const float start_values[3] = {start.x, start.y, start.z};
    const float direction_values[3] = {direction.x, direction.y, direction.z};
    const float center_values[3] = {box->center.x, box->center.y, box->center.z};
    const float extent_values[3] = {box->half_extents.x + padding, box->half_extents.y + padding, box->half_extents.z + padding};

    for (size_t axis = 0u; axis < 3u; ++axis) {
        const float minimum = center_values[axis] - extent_values[axis];
        const float maximum = center_values[axis] + extent_values[axis];

        if (fabsf(direction_values[axis]) <= 0.0001f) {
            if (start_values[axis] < minimum || start_values[axis] > maximum) {
                return 0;
            }
            continue;
        }

        const float inverse = 1.0f / direction_values[axis];
        float axis_t0 = (minimum - start_values[axis]) * inverse;
        float axis_t1 = (maximum - start_values[axis]) * inverse;

        if (axis_t0 > axis_t1) {
            const float swap = axis_t0;
            axis_t0 = axis_t1;
            axis_t1 = swap;
        }

        t_min = fmaxf(t_min, axis_t0);
        t_max = fminf(t_max, axis_t1);
        if (t_min > t_max) {
            return 0;
        }
    }

    if (hit_t_out != NULL) {
        *hit_t_out = t_min;
    }

    return 1;
}

static int segment_hits_world_cover(MDTBFloat3 start, MDTBFloat3 end, float padding, MDTBFloat3 *impact_out) {
    float best_t = 1000.0f;
    int found_hit = 0;

    for (size_t index = 0u; index < g_collision_box_count; ++index) {
        float hit_t = 0.0f;

        if (!segment_intersects_box(start, end, &g_collision_boxes[index], padding, &hit_t)) {
            continue;
        }

        if (hit_t < 0.0f || hit_t > 1.0f) {
            continue;
        }

        if (!found_hit || hit_t < best_t) {
            best_t = hit_t;
            found_hit = 1;
        }
    }

    if (found_hit && impact_out != NULL) {
        impact_out->x = start.x + ((end.x - start.x) * best_t);
        impact_out->y = start.y + ((end.y - start.y) * best_t);
        impact_out->z = start.z + ((end.z - start.z) * best_t);
    }

    return found_hit;
}

static MDTBFloat3 lookout_anchor_position(uint32_t anchor_index) {
    if (anchor_index >= (sizeof(kLookoutPressureAnchors) / sizeof(kLookoutPressureAnchors[0]))) {
        return kLookoutBasePosition;
    }

    return kLookoutPressureAnchors[anchor_index];
}

static MDTBFloat3 witness_target_position(uint32_t witness_state) {
    switch (witness_state) {
        case MDTBWitnessStateInvestigate:
            return kWitnessInvestigatePosition;
        case MDTBWitnessStateFlee:
        case MDTBWitnessStateCooldown:
            return kWitnessFleePosition;
        case MDTBWitnessStateIdle:
        default:
            return kWitnessBasePosition;
    }
}

static MDTBFloat3 bystander_target_position(uint32_t bystander_state) {
    switch (bystander_state) {
        case MDTBWitnessStateInvestigate:
            return kBystanderInvestigatePosition;
        case MDTBWitnessStateFlee:
        case MDTBWitnessStateCooldown:
            return kBystanderFleePosition;
        case MDTBWitnessStateIdle:
        default:
            return kBystanderBasePosition;
    }
}

static int active_civilian_response_count(const MDTBEngineState *state) {
    int count = 0;

    if (state == NULL) {
        return 0;
    }

    if (state->witness_state != MDTBWitnessStateIdle ||
        state->witness_alert > 0.05f ||
        state->witness_state_timer > 0.0f) {
        count += 1;
    }

    if (state->bystander_state != MDTBWitnessStateIdle ||
        state->bystander_alert > 0.05f ||
        state->bystander_state_timer > 0.0f) {
        count += 1;
    }

    return count;
}

static float territory_preferred_side_bias(const MDTBEngineState *state) {
    if (state == NULL) {
        return 0.0f;
    }

    return clampf(state->territory_preferred_side, -1.0f, 1.0f);
}

static float territory_preferred_side_strength(const MDTBEngineState *state) {
    return fabsf(territory_preferred_side_bias(state));
}

static MDTBFloat3 territory_shoulder_focus_position(const MDTBEngineState *state, MDTBFloat3 position, float lateral_offset, float depth_offset) {
    const float preferred_side = territory_preferred_side_bias(state);
    const float preferred_strength = territory_preferred_side_strength(state);
    const float lateral_scale = 0.34f + preferred_strength * 0.66f;
    const float depth_scale = 0.28f + preferred_strength * 0.72f;

    if (preferred_strength <= 0.18f) {
        return position;
    }

    position.x += preferred_side * lateral_offset * lateral_scale;
    position.z += depth_offset * depth_scale;
    position.x = clampf(position.x, kCourtSetTerritoryMinX - 10.0f, kCourtSetTerritoryMaxX + 10.0f);
    position.z = clampf(position.z, kCourtSetTerritoryEntryZ - 4.0f, kCourtSetTerritoryMaxZ + 4.0f);
    return position;
}

static void start_hostile_search(MDTBEngineState *state, MDTBFloat3 position, float duration) {
    if (state == NULL) {
        return;
    }

    state->combat_hostile_search_position = position;
    state->combat_hostile_search_timer = fmaxf(state->combat_hostile_search_timer, duration);
    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, 0.42f);
    state->combat_hostile_attack_windup = 0.0f;
}

static void trigger_street_incident(MDTBEngineState *state, MDTBFloat3 position, float level, float duration) {
    if (state == NULL) {
        return;
    }

    position.y = kSidewalkHeight;
    state->street_incident_position = position;
    state->street_incident_level = fmaxf(state->street_incident_level, clampf(level, 0.0f, 1.0f));
    state->street_incident_timer = fmaxf(state->street_incident_timer, duration);
    state->street_recovery_position = position;
    state->street_recovery_level = 0.0f;
    state->street_recovery_timer = 0.0f;
}

static void trigger_street_recovery(MDTBEngineState *state, MDTBFloat3 position, float level, float duration) {
    if (state == NULL || duration <= 0.0f || level <= 0.0f) {
        return;
    }

    position.y = kSidewalkHeight;
    state->street_recovery_position = position;
    state->street_recovery_level = fmaxf(state->street_recovery_level, clampf(level, 0.0f, 1.0f));
    state->street_recovery_timer = fmaxf(state->street_recovery_timer, duration);
}

static uint32_t choose_lookout_pressure_anchor(const MDTBEngineState *state) {
    const MDTBFloat3 player_position =
        (state != NULL && state->combat_hostile_search_timer > 0.0f)
        ? state->combat_hostile_search_position
        : current_player_position(state);
    const MDTBFloat3 player_cover =
        (state != NULL && state->combat_hostile_search_timer > 0.0f)
        ? make_float3(player_position.x, kSidewalkHeight + 1.02f, player_position.z)
        : player_cover_position(state);
    uint32_t best_anchor = 3u;
    float best_score = -1000000.0f;

    if (state == NULL) {
        return 3u;
    }

    const float preferred_side = territory_preferred_side_bias(state);
    const float preferred_strength = territory_preferred_side_strength(state);
    const float reclaim_shoulder_pressure =
        preferred_strength > 0.18f
        ? clampf(
            ((state->territory_resolve_state == MDTBTerritoryResolveHold && state->territory_resolve_timer > 0.0f)
                ? (0.34f + state->territory_resolve_progress * 0.42f)
                : 0.0f) +
            ((state->territory_reapproach_mode == MDTBTerritoryReapproachReclaim && state->territory_reapproach_timer > 0.0f)
                ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachReclaimDuration + 0.6f), 0.0f, 1.0f) * 0.30f
                : 0.0f) +
            fmaxf(0.0f, state->territory_deep_watch - state->territory_front_watch) * 0.18f,
            0.0f,
            1.0f
        )
        : 0.0f;
    const float retake_shoulder_pressure =
        preferred_strength > 0.18f
        ? clampf(
            ((state->territory_resolve_state == MDTBTerritoryResolvePullout && state->territory_resolve_timer > 0.0f)
                ? (0.36f + state->territory_resolve_progress * 0.40f)
                : 0.0f) +
            ((state->territory_reapproach_mode == MDTBTerritoryReapproachRetake && state->territory_reapproach_timer > 0.0f)
                ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachRetakeDuration + 0.5f), 0.0f, 1.0f) * 0.28f
                : 0.0f) +
            state->territory_front_watch * 0.16f,
            0.0f,
            1.0f
        )
        : 0.0f;
    const float cold_watch_shoulder_pressure =
        preferred_strength > 0.18f &&
        state->territory_phase == MDTBTerritoryPhaseNone &&
        state->territory_reentry_timer <= 0.0f &&
        state->territory_reapproach_timer <= 0.05f &&
        state->territory_resolve_timer <= 0.05f &&
        state->combat_hostile_search_timer <= 0.0f &&
        state->street_recovery_timer <= 0.0f &&
        state->street_recovery_level <= 0.04f &&
        state->street_incident_timer <= 0.0f &&
        state->street_incident_level <= 0.06f &&
        state->territory_watch_timer > 0.0f &&
        state->territory_watch_timer < (kTerritoryWatchCarryDuration * 0.36f) &&
        state->territory_heat > 0.03f &&
        state->territory_heat < 0.18f &&
        state->territory_patrol_state == MDTBTerritoryPatrolWatch &&
        state->territory_patrol_alert > 0.08f &&
        state->territory_inner_state != MDTBTerritoryPatrolBrace &&
        state->territory_inner_state != MDTBTerritoryPatrolHandoff
        ? clampf(
            preferred_strength * 0.32f +
            state->territory_front_watch * 0.16f +
            fmaxf(state->territory_patrol_alert, state->territory_inner_alert) * 0.20f,
            0.0f,
            1.0f
        )
        : 0.0f;
    const float cold_start_shoulder_pressure =
        preferred_strength > 0.18f &&
        (state->territory_phase == MDTBTerritoryPhaseBoundary ||
         state->territory_phase == MDTBTerritoryPhaseNone) &&
        state->territory_reentry_timer <= 0.0f &&
        state->territory_reapproach_timer <= 0.05f &&
        state->territory_resolve_timer <= 0.05f &&
        state->street_recovery_timer <= 0.0f &&
        state->street_recovery_level <= 0.04f &&
        state->street_incident_timer <= 0.0f &&
        state->street_incident_level <= 0.06f &&
        state->territory_watch_timer > 0.0f &&
        state->territory_watch_timer < (kTerritoryWatchCarryDuration * 0.42f) &&
        state->territory_heat > 0.04f &&
        state->territory_heat < 0.24f &&
        (state->territory_patrol_state == MDTBTerritoryPatrolScreen ||
         state->territory_patrol_state == MDTBTerritoryPatrolBrace ||
         state->territory_patrol_state == MDTBTerritoryPatrolWatch ||
         state->territory_inner_state == MDTBTerritoryPatrolBrace ||
         state->territory_inner_state == MDTBTerritoryPatrolWatch)
        ? clampf(
            preferred_strength * 0.36f +
            ((state->territory_patrol_state == MDTBTerritoryPatrolBrace ||
              state->territory_inner_state == MDTBTerritoryPatrolBrace)
                ? 0.20f
                : 0.0f) +
            (state->territory_patrol_state == MDTBTerritoryPatrolScreen ? 0.16f : 0.0f) +
            state->territory_front_watch * 0.18f +
            fmaxf(state->territory_patrol_alert, state->territory_inner_alert) * 0.18f,
            0.0f,
            1.0f
        )
        : 0.0f;

    for (uint32_t index = 0u; index < (sizeof(kLookoutPressureAnchors) / sizeof(kLookoutPressureAnchors[0])); ++index) {
        const MDTBFloat3 anchor = lookout_anchor_position(index);
        const MDTBFloat3 anchor_shot_origin = make_float3(anchor.x, anchor.y + 1.18f, anchor.z);
        const float player_distance = sqrtf(distance_squared_xz(anchor, player_position));
        const float travel_distance = sqrtf(distance_squared_xz(anchor, state->combat_hostile_position));
        const int clear_shot = segment_hits_world_cover(anchor_shot_origin, player_cover, 0.10f, NULL) == 0;
        const int far_side = (player_position.x < -2.0f && anchor.x > 2.0f) || (player_position.x > 2.0f && anchor.x < -2.0f);
        const int left_anchor = anchor.x < -3.0f;
        const int right_anchor = anchor.x > 3.0f;
        const int same_shoulder =
            (preferred_side < -0.18f && left_anchor) ||
            (preferred_side > 0.18f && right_anchor);
        const int opposite_shoulder =
            (preferred_side < -0.18f && right_anchor) ||
            (preferred_side > 0.18f && left_anchor);
        const int edge_anchor = anchor.z < 53.8f;
        const int deep_anchor = anchor.z >= 53.8f;
        float score = clear_shot ? 4.8f : -0.9f;

        if (position_overlaps_collision(anchor, 0.26f)) {
            continue;
        }

        score += fminf(fabsf(anchor.x - player_position.x) * 0.06f, 1.1f);
        score -= fabsf(anchor.z - player_position.z) * 0.04f;
        score -= travel_distance * 0.12f;
        score -= fabsf(player_distance - 9.0f) * 0.10f;
        score += far_side ? 0.85f : 0.0f;
        score += same_shoulder ? (reclaim_shoulder_pressure * 0.78f + retake_shoulder_pressure * 0.92f) : 0.0f;
        score -= opposite_shoulder ? (reclaim_shoulder_pressure * 0.34f + retake_shoulder_pressure * 0.42f) : 0.0f;
        score += (same_shoulder && clear_shot) ? (reclaim_shoulder_pressure * 0.26f + retake_shoulder_pressure * 0.22f) : 0.0f;
        score += (same_shoulder && deep_anchor) ? reclaim_shoulder_pressure * 0.18f : 0.0f;
        score += (same_shoulder && edge_anchor) ? retake_shoulder_pressure * 0.22f : 0.0f;
        score += same_shoulder ? (edge_anchor ? cold_watch_shoulder_pressure * 0.68f : cold_watch_shoulder_pressure * 0.18f) : 0.0f;
        score -= opposite_shoulder ? cold_watch_shoulder_pressure * 0.24f : 0.0f;
        score += (same_shoulder && edge_anchor && clear_shot) ? cold_watch_shoulder_pressure * 0.16f : 0.0f;
        score += same_shoulder ? (edge_anchor ? cold_start_shoulder_pressure * 0.74f : cold_start_shoulder_pressure * 0.24f) : 0.0f;
        score -= opposite_shoulder ? cold_start_shoulder_pressure * 0.28f : 0.0f;
        score += (same_shoulder && edge_anchor && clear_shot) ? cold_start_shoulder_pressure * 0.22f : 0.0f;

        if (state->combat_player_in_cover != 0u && clear_shot) {
            score += 0.8f;
        }

        if (index == state->combat_hostile_anchor_index) {
            score += clear_shot ? 0.35f : -0.30f;
        }

        if (score > best_score) {
            best_score = score;
            best_anchor = index;
        }
    }

    return best_anchor;
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
    uint32_t best_occluded = 0u;

    if (state == NULL) {
        return;
    }

    state->combat_focus_target_kind = MDTBCombatTargetNone;
    state->combat_focus_distance = 0.0f;
    state->combat_focus_alignment = 0.0f;
    state->combat_focus_occluded = 0u;

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
        const uint32_t occluded = segment_hits_world_cover(origin, combat_target_shot_position_for_kind(state, target_kind), 0.10f, NULL) ? 1u : 0u;
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
        score -= occluded != 0u ? 0.80f : 0.0f;
        if (target_kind == preferred_focus_target(state)) {
            score += 0.28f;
        }

        if (score > best_score) {
            best_score = score;
            best_kind = target_kind;
            best_distance = flat_distance;
            best_alignment = fmaxf(alignment, flat_alignment);
            best_occluded = occluded;
        }
    }

    state->combat_focus_target_kind = best_kind;
    state->combat_focus_distance = best_kind == MDTBCombatTargetNone ? 0.0f : best_distance;
    state->combat_focus_alignment = best_kind == MDTBCombatTargetNone ? 0.0f : clampf(best_alignment, 0.0f, 1.0f);
    state->combat_focus_occluded = best_kind == MDTBCombatTargetNone ? 0u : best_occluded;
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

static uint32_t select_firearm_target(const MDTBEngineState *state, MDTBFloat3 origin, MDTBFloat3 *shot_end_out, int *shot_blocked_out) {
    MDTBFloat3 forward = view_forward(state->camera.yaw, state->camera.pitch);
    const float forward_length = sqrtf((forward.x * forward.x) + (forward.y * forward.y) + (forward.z * forward.z));
    MDTBFloat3 shot_end;
    MDTBFloat3 cover_impact;
    MDTBFloat3 forward_flat;
    uint32_t target_kinds[2] = {MDTBCombatTargetDummy, MDTBCombatTargetLookout};
    uint32_t best_kind = MDTBCombatTargetNone;
    float best_score = 1000000.0f;
    int default_path_blocked;

    if (shot_end_out != NULL) {
        *shot_end_out = origin;
    }
    if (shot_blocked_out != NULL) {
        *shot_blocked_out = 0;
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

    default_path_blocked = segment_hits_world_cover(origin, shot_end, 0.10f, &cover_impact);
    if (default_path_blocked && shot_end_out != NULL) {
        *shot_end_out = cover_impact;
    }

    for (size_t index = 0u; index < 2u; ++index) {
        const uint32_t target_kind = target_kinds[index];
        const MDTBFloat3 target = combat_target_shot_position_for_kind(state, target_kind);
        const MDTBFloat3 to_target_flat = normalize_flat(make_float3(target.x - origin.x, 0.0f, target.z - origin.z));
        const float flat_alignment = dot_flat(forward_flat, to_target_flat);
        const float impact_radius = target_kind == MDTBCombatTargetLookout ? 0.78f : 0.85f;
        const float aim_dot_threshold = target_kind == MDTBCombatTargetLookout ? (kPistolAimDot - kLookoutAimBias) : kPistolAimDot;
        const float distance_squared = point_to_segment_distance_squared_xz(target, origin, shot_end);
        float score;

        if (!combat_target_kind_is_active(state, target_kind)) {
            continue;
        }

        if (segment_hits_world_cover(origin, target, 0.10f, NULL)) {
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

    if (best_kind == MDTBCombatTargetNone && default_path_blocked && shot_blocked_out != NULL) {
        *shot_blocked_out = 1;
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
    state->combat_focus_occluded = 0u;
    state->combat_player_in_cover = 0u;

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
    state->combat_player_in_cover =
        combat_target_kind_is_active(state, MDTBCombatTargetLookout) &&
        segment_hits_world_cover(hostile_shot_origin(state), player_cover_position(state), 0.10f, NULL) ? 1u : 0u;
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
    state->firearm_last_shot_blocked = 0u;
}

static void clear_hostile_last_shot(MDTBEngineState *state) {
    if (state == NULL) {
        return;
    }

    state->combat_hostile_last_shot_from = make_float3(0.0f, 0.0f, 0.0f);
    state->combat_hostile_last_shot_to = make_float3(0.0f, 0.0f, 0.0f);
    state->combat_hostile_last_shot_timer = 0.0f;
    state->combat_hostile_last_shot_hit = 0u;
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
            state->combat_hostile_attack_windup = 0.0f;
            state->combat_hostile_attack_cooldown = fmaxf(state->combat_hostile_attack_cooldown, 0.48f);
            state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, 0.32f);
            state->combat_hostile_reposition_timer = 0.0f;
            if (state->combat_hostile_health <= 0.0f) {
                state->combat_hostile_reset_timer = kLookoutRespawnDelay;
            }
            break;
        default:
            return;
    }

    state->combat_last_hit_target_kind = target_kind;
}

static void reset_combat_encounter(MDTBEngineState *state) {
    if (state == NULL || state->traversal_mode != MDTBTraversalModeOnFoot) {
        return;
    }

    state->actor_position = kCombatLaneResetPosition;
    state->actor_ground_height = kCombatLaneResetPosition.y;
    state->actor_velocity = make_float3(0.0f, 0.0f, 0.0f);
    state->player_health = kPlayerMaxHealth;
    state->player_recovery_delay = kPlayerRecoveryDelay;
    state->player_damage_pulse = fmaxf(state->player_damage_pulse, 1.0f);
    state->player_reset_timer = kPlayerResetGraceDuration;
    state->combat_target_health = kPracticeDummyMaxHealth;
    state->combat_target_reaction = 0.0f;
    state->combat_target_reset_timer = 0.0f;
    state->combat_hostile_health = kLookoutMaxHealth;
    state->combat_hostile_reaction = 0.0f;
    state->combat_hostile_reset_timer = 0.0f;
    state->combat_hostile_alert = 0.0f;
    state->combat_hostile_anchor_index = 3u;
    state->combat_hostile_position = lookout_anchor_position(state->combat_hostile_anchor_index);
    state->combat_hostile_heading = kPi;
    state->combat_hostile_reposition_timer = 0.0f;
    state->combat_hostile_reacquire_timer = kLookoutVehicleReacquireDuration;
    state->combat_hostile_search_position = kCombatLaneResetPosition;
    state->combat_hostile_search_timer = 0.0f;
    state->combat_hostile_attack_cooldown = 0.45f;
    state->combat_hostile_attack_windup = 0.0f;
    state->witness_position = kWitnessFleePosition;
    state->witness_heading = -0.30f;
    state->witness_state = MDTBWitnessStateCooldown;
    state->witness_alert = 1.0f;
    state->witness_state_timer = kWitnessCooldownDuration;
    state->bystander_position = kBystanderFleePosition;
    state->bystander_heading = 0.82f;
    state->bystander_state = MDTBWitnessStateCooldown;
    state->bystander_alert = 0.84f;
    state->bystander_state_timer = kBystanderCooldownDuration;
    state->street_incident_position = kWitnessFleePosition;
    state->street_incident_level = 0.48f;
    state->street_incident_timer = kStreetIncidentSettleDuration;
    state->street_recovery_position = kCombatLaneResetPosition;
    state->street_recovery_level = 0.0f;
    state->street_recovery_timer = 0.0f;
    state->territory_faction = MDTBTerritoryFactionCourtSet;
    state->territory_phase = MDTBTerritoryPhaseHot;
    state->territory_presence = 0.84f;
    state->territory_heat = 0.82f;
    state->territory_reentry_timer = 6.8f;
    state->territory_entry_mode = MDTBTerritoryEntryOnFoot;
    state->territory_watch_timer = kTerritoryWatchCarryDuration * 1.12f;
    state->territory_front_watch = 0.86f;
    state->territory_deep_watch = 0.92f;
    state->territory_patrol_position = kTerritoryPatrolHandoffPosition;
    state->territory_patrol_heading = atan2f(
        kCombatLaneResetPosition.x - kTerritoryPatrolHandoffPosition.x,
        kCombatLaneResetPosition.z - kTerritoryPatrolHandoffPosition.z
    );
    state->territory_patrol_state = MDTBTerritoryPatrolHandoff;
    state->territory_patrol_alert = 0.88f;
    state->territory_inner_position = kTerritoryInnerReceivePosition;
    state->territory_inner_heading = atan2f(
        kCombatLaneResetPosition.x - kTerritoryInnerReceivePosition.x,
        kCombatLaneResetPosition.z - kTerritoryInnerReceivePosition.z
    );
    state->territory_inner_state = MDTBTerritoryPatrolHandoff;
    state->territory_inner_alert = 0.84f;
    state->territory_commit_state = MDTBTerritoryCommitNone;
    state->territory_commit_timer = 0.0f;
    state->territory_commit_progress = 0.0f;
    state->territory_resolve_state = MDTBTerritoryResolveNone;
    state->territory_resolve_timer = 0.0f;
    state->territory_resolve_progress = 0.0f;
    state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
    state->territory_reapproach_timer = 0.0f;
    state->territory_preferred_side = 0.0f;
    clear_melee_attack(state);
    cancel_firearm_reload(state);
    clear_hostile_last_shot(state);
}

static void apply_damage_to_player(MDTBEngineState *state, float damage) {
    if (state == NULL ||
        state->traversal_mode != MDTBTraversalModeOnFoot ||
        state->player_reset_timer > 0.0f) {
        return;
    }

    state->player_health = fmaxf(0.0f, state->player_health - damage);
    state->player_recovery_delay = kPlayerRecoveryDelay;
    state->player_damage_pulse = fmaxf(state->player_damage_pulse, 1.0f);

    if (state->player_health <= 0.0f) {
        reset_combat_encounter(state);
    }
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
    int shot_blocked = 0;
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

    hit_target_kind = select_firearm_target(state, shot_origin, &shot_end, &shot_blocked);
    state->firearm_last_shot_from = shot_origin;
    state->firearm_last_shot_to = shot_end;
    state->firearm_last_shot_timer = kPistolShotFlashDuration;
    state->firearm_last_shot_hit = hit_target_kind != MDTBCombatTargetNone ? 1u : 0u;
    state->firearm_last_shot_blocked = shot_blocked != 0 ? 1u : 0u;
    trigger_street_incident(
        state,
        shot_end,
        shot_blocked != 0 ? 0.58f : (hit_target_kind == MDTBCombatTargetLookout ? 0.80f : 0.70f),
        kStreetIncidentNoticeDuration
    );

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

static void fire_lookout_shot(MDTBEngineState *state) {
    MDTBFloat3 shot_origin;
    MDTBFloat3 shot_target;
    MDTBFloat3 shot_end;

    if (state == NULL ||
        state->traversal_mode != MDTBTraversalModeOnFoot ||
        !combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
        return;
    }

    shot_origin = hostile_shot_origin(state);
    shot_target = player_cover_position(state);

    if (segment_hits_world_cover(shot_origin, shot_target, 0.10f, &shot_end)) {
        state->combat_hostile_last_shot_hit = 0u;
        state->combat_hostile_attack_cooldown = kLookoutAttackBlockedCooldown;
        state->combat_hostile_reposition_timer = 0.0f;
        start_hostile_search(state, shot_target, kLookoutSearchDuration * 0.72f);
        trigger_street_incident(state, shot_end, 0.68f, kStreetIncidentSearchDuration);
    } else {
        shot_end = shot_target;
        state->combat_hostile_last_shot_hit = 1u;
        state->combat_hostile_attack_cooldown = kLookoutAttackCooldown;
        state->combat_hostile_reposition_timer = kLookoutRepositionRetargetDelay;
        apply_damage_to_player(state, kLookoutShotDamage);
        trigger_street_incident(state, shot_end, 0.84f, kStreetIncidentSearchDuration);
    }

    state->combat_hostile_last_shot_from = shot_origin;
    state->combat_hostile_last_shot_to = shot_end;
    state->combat_hostile_last_shot_timer = kLookoutShotFlashDuration;
    state->combat_hostile_attack_windup = 0.0f;
    state->combat_hostile_alert = 1.0f;
    state->combat_hostile_reacquire_timer = 0.28f;
}

static void step_combat_state(MDTBEngineState *state, float dt, int wants_attack, int wants_reload) {
    const float previous_search_timer = state != NULL ? state->combat_hostile_search_timer : 0.0f;

    if (state == NULL) {
        return;
    }

    state->combat_target_reaction = approachf(state->combat_target_reaction, 0.0f, 7.0f, dt);
    state->combat_hostile_reaction = approachf(state->combat_hostile_reaction, 0.0f, 7.6f, dt);
    state->player_damage_pulse = approachf(state->player_damage_pulse, 0.0f, 7.8f, dt);
    state->firearm_cooldown_timer = fmaxf(state->firearm_cooldown_timer - dt, 0.0f);
    state->combat_hostile_attack_cooldown = fmaxf(state->combat_hostile_attack_cooldown - dt, 0.0f);
    state->combat_hostile_reposition_timer = fmaxf(state->combat_hostile_reposition_timer - dt, 0.0f);
    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer - dt, 0.0f);
    state->combat_hostile_search_timer = fmaxf(state->combat_hostile_search_timer - dt, 0.0f);
    state->player_recovery_delay = fmaxf(state->player_recovery_delay - dt, 0.0f);
    state->player_reset_timer = fmaxf(state->player_reset_timer - dt, 0.0f);
    state->witness_state_timer = fmaxf(state->witness_state_timer - dt, 0.0f);
    state->witness_alert = approachf(state->witness_alert, 0.0f, 1.6f, dt);
    state->bystander_state_timer = fmaxf(state->bystander_state_timer - dt, 0.0f);
    state->bystander_alert = approachf(state->bystander_alert, 0.0f, 1.7f, dt);
    state->street_incident_timer = fmaxf(state->street_incident_timer - dt, 0.0f);
    state->street_incident_level = approachf(
        state->street_incident_level,
        0.0f,
        state->street_incident_timer > 0.0f ? 0.24f : 1.10f,
        dt
    );
    state->street_recovery_timer = fmaxf(state->street_recovery_timer - dt, 0.0f);
    state->street_recovery_level = approachf(
        state->street_recovery_level,
        0.0f,
        state->street_recovery_timer > 0.0f ? 0.30f : 1.10f,
        dt
    );

    if (state->street_recovery_timer <= 0.0f && state->street_recovery_level <= 0.04f) {
        state->street_recovery_level = 0.0f;
        state->street_recovery_position = kCombatLaneResetPosition;
    }

    if (state->firearm_last_shot_timer > 0.0f) {
        state->firearm_last_shot_timer = fmaxf(state->firearm_last_shot_timer - dt, 0.0f);
        if (state->firearm_last_shot_timer <= 0.0f) {
            state->firearm_last_shot_hit = 0u;
            state->firearm_last_shot_blocked = 0u;
        }
    }

    if (state->combat_hostile_last_shot_timer > 0.0f) {
        state->combat_hostile_last_shot_timer = fmaxf(state->combat_hostile_last_shot_timer - dt, 0.0f);
        if (state->combat_hostile_last_shot_timer <= 0.0f) {
            state->combat_hostile_last_shot_hit = 0u;
        }
    }

    if (state->player_health < kPlayerMaxHealth &&
        state->player_recovery_delay <= 0.0f &&
        state->player_reset_timer <= 0.0f) {
        state->player_health = fminf(state->player_health + (kPlayerRecoveryRate * dt), kPlayerMaxHealth);
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
            state->combat_hostile_anchor_index = 3u;
            state->combat_hostile_position = lookout_anchor_position(state->combat_hostile_anchor_index);
            state->combat_hostile_heading = kPi;
            state->combat_hostile_reposition_timer = 0.0f;
            state->combat_hostile_reacquire_timer = kLookoutVehicleReacquireDuration;
            state->combat_hostile_search_position = kCombatLaneResetPosition;
            state->combat_hostile_search_timer = 0.0f;
            state->combat_hostile_attack_cooldown = 0.0f;
            state->combat_hostile_attack_windup = 0.0f;
            state->street_incident_position = kCombatLaneResetPosition;
            state->street_incident_level = 0.0f;
            state->street_incident_timer = 0.0f;
            state->street_recovery_position = kCombatLaneResetPosition;
            state->street_recovery_level = 0.0f;
            state->street_recovery_timer = 0.0f;
            state->territory_entry_mode = MDTBTerritoryEntryNone;
            state->territory_watch_timer = 0.0f;
            state->territory_front_watch = 0.0f;
            state->territory_deep_watch = 0.0f;
            state->territory_patrol_position = kTerritoryPatrolBasePosition;
            state->territory_patrol_heading = atan2f(
                kTerritoryPatrolLinePosition.x - kTerritoryPatrolBasePosition.x,
                kTerritoryPatrolLinePosition.z - kTerritoryPatrolBasePosition.z
            );
            state->territory_patrol_state = MDTBTerritoryPatrolIdle;
            state->territory_patrol_alert = 0.0f;
            state->territory_inner_position = kTerritoryInnerBasePosition;
            state->territory_inner_heading = atan2f(
                kTerritoryPatrolBasePosition.x - kTerritoryInnerBasePosition.x,
                kTerritoryPatrolBasePosition.z - kTerritoryInnerBasePosition.z
            );
            state->territory_inner_state = MDTBTerritoryPatrolIdle;
            state->territory_inner_alert = 0.0f;
            state->territory_commit_state = MDTBTerritoryCommitNone;
            state->territory_commit_timer = 0.0f;
            state->territory_commit_progress = 0.0f;
            state->territory_resolve_state = MDTBTerritoryResolveNone;
            state->territory_resolve_timer = 0.0f;
            state->territory_resolve_progress = 0.0f;
            state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
            state->territory_reapproach_timer = 0.0f;
            state->territory_preferred_side = 0.0f;
        }
    }

    if (state->firearm_reloading != 0u) {
        state->firearm_reload_timer = fmaxf(state->firearm_reload_timer - dt, 0.0f);
        if (state->firearm_reload_timer <= 0.0f) {
            complete_firearm_reload(state);
        }
    }

    {
        const MDTBFloat3 player_position = current_player_position(state);
        const float player_distance_squared = distance_squared_xz(player_position, state->witness_position);
        const float hostile_distance_squared = distance_squared_xz(state->combat_hostile_position, state->witness_position);
        const float threat_distance_squared = fminf(player_distance_squared, hostile_distance_squared);
        const int firearm_noise = state->firearm_last_shot_timer > 0.0f || state->combat_hostile_last_shot_timer > 0.0f;
        const int melee_noise = state->melee_attack_connected != 0 && state->melee_attack_phase != MDTBMeleeAttackIdle;
        const int notice_noise = (firearm_noise || melee_noise) && threat_distance_squared <= (kWitnessNoticeRadius * kWitnessNoticeRadius);
        const int panic_noise =
            (firearm_noise || state->combat_hostile_attack_windup > 0.0f || state->player_health < kPlayerMaxHealth) &&
            threat_distance_squared <= (kWitnessPanicRadius * kWitnessPanicRadius);
        MDTBFloat3 desired_witness = witness_target_position(state->witness_state);
        MDTBFloat3 desired_heading_target = player_position;
        float witness_speed = 3.6f;

        switch (state->witness_state) {
            case MDTBWitnessStateIdle:
                if (notice_noise) {
                    state->witness_state = MDTBWitnessStateInvestigate;
                    state->witness_state_timer = kWitnessInvestigateDuration;
                    state->witness_alert = fmaxf(state->witness_alert, firearm_noise ? 0.82f : 0.54f);
                    trigger_street_incident(
                        state,
                        state->witness_position,
                        firearm_noise ? 0.42f : 0.28f,
                        kStreetIncidentNoticeDuration
                    );
                }
                desired_witness = kWitnessBasePosition;
                break;
            case MDTBWitnessStateInvestigate:
                desired_witness = kWitnessInvestigatePosition;
                witness_speed = 4.2f;
                if (panic_noise || state->combat_hostile_search_timer > 0.0f) {
                    state->witness_state = MDTBWitnessStateFlee;
                    state->witness_state_timer = kWitnessFleeDuration;
                    state->witness_alert = 1.0f;
                    start_hostile_search(state, player_position, kLookoutSearchDuration);
                    trigger_street_incident(state, player_position, 1.0f, kStreetIncidentFleeDuration);
                    desired_witness = kWitnessFleePosition;
                } else if (state->witness_state_timer <= 0.0f && !notice_noise) {
                    state->witness_state = MDTBWitnessStateCooldown;
                    state->witness_state_timer = kWitnessCooldownDuration * 0.5f;
                    desired_witness = kWitnessFleePosition;
                }
                break;
            case MDTBWitnessStateFlee:
                desired_witness = kWitnessFleePosition;
                witness_speed = 6.2f;
                desired_heading_target = desired_witness;
                if (state->witness_state_timer <= 0.0f &&
                    distance_squared_xz(state->witness_position, kWitnessFleePosition) <= (1.6f * 1.6f)) {
                    state->witness_state = MDTBWitnessStateCooldown;
                    state->witness_state_timer = kWitnessCooldownDuration;
                }
                break;
            case MDTBWitnessStateCooldown:
            default:
                desired_witness = state->witness_state_timer > (kWitnessCooldownDuration * 0.45f)
                    ? kWitnessFleePosition
                    : kWitnessBasePosition;
                witness_speed = 3.2f;
                if (state->witness_state_timer <= 0.0f &&
                    distance_squared_xz(state->witness_position, kWitnessBasePosition) <= (2.0f * 2.0f)) {
                    state->witness_state = MDTBWitnessStateIdle;
                    state->witness_alert = 0.0f;
                    desired_witness = kWitnessBasePosition;
                }
                break;
        }

        state->witness_position = approach_float3(state->witness_position, desired_witness, witness_speed, dt);
        const MDTBFloat3 to_heading_target = make_float3(
            desired_heading_target.x - state->witness_position.x,
            0.0f,
            desired_heading_target.z - state->witness_position.z
        );
        if (fabsf(to_heading_target.x) > 0.001f || fabsf(to_heading_target.z) > 0.001f) {
            state->witness_heading = approach_angle(
                state->witness_heading,
                atan2f(to_heading_target.x, -to_heading_target.z),
                7.2f,
                dt
            );
        }
    }

    {
        const MDTBFloat3 incident_position = state->street_incident_position;
        const float bystander_incident_distance_squared = distance_squared_xz(incident_position, state->bystander_position);
        const int incident_active = state->street_incident_timer > 0.0f && state->street_incident_level > 0.12f;
        const int notice_incident =
            incident_active &&
            bystander_incident_distance_squared <= (kBystanderNoticeRadius * kBystanderNoticeRadius) &&
            (state->street_incident_level >= 0.18f ||
             state->witness_state != MDTBWitnessStateIdle);
        const int panic_incident =
            incident_active &&
            bystander_incident_distance_squared <= (kBystanderPanicRadius * kBystanderPanicRadius) &&
            (state->street_incident_level >= 0.48f ||
             state->witness_state == MDTBWitnessStateFlee ||
             state->combat_hostile_search_timer > 0.0f);
        MDTBFloat3 desired_bystander = bystander_target_position(state->bystander_state);
        MDTBFloat3 desired_heading_target = incident_position;
        float bystander_speed = 3.4f;

        switch (state->bystander_state) {
            case MDTBWitnessStateIdle:
                if (notice_incident) {
                    state->bystander_state = MDTBWitnessStateInvestigate;
                    state->bystander_state_timer = kBystanderInvestigateDuration;
                    state->bystander_alert = fmaxf(state->bystander_alert, 0.34f + state->street_incident_level * 0.28f);
                    trigger_street_incident(state, state->bystander_position, 0.20f, kStreetIncidentNoticeDuration * 0.72f);
                }
                desired_bystander = kBystanderBasePosition;
                break;
            case MDTBWitnessStateInvestigate:
                desired_bystander = kBystanderInvestigatePosition;
                bystander_speed = 4.0f;
                if (panic_incident) {
                    state->bystander_state = MDTBWitnessStateFlee;
                    state->bystander_state_timer = kBystanderFleeDuration;
                    state->bystander_alert = 1.0f;
                    trigger_street_incident(state, state->bystander_position, 0.82f, kStreetIncidentFleeDuration * 0.85f);
                    desired_bystander = kBystanderFleePosition;
                    desired_heading_target = desired_bystander;
                } else if (state->bystander_state_timer <= 0.0f && !notice_incident) {
                    state->bystander_state = MDTBWitnessStateCooldown;
                    state->bystander_state_timer = kBystanderCooldownDuration * 0.45f;
                    desired_bystander = kBystanderFleePosition;
                }
                break;
            case MDTBWitnessStateFlee:
                desired_bystander = kBystanderFleePosition;
                desired_heading_target = desired_bystander;
                bystander_speed = 6.4f;
                if (state->bystander_state_timer <= 0.0f &&
                    distance_squared_xz(state->bystander_position, kBystanderFleePosition) <= (1.9f * 1.9f)) {
                    state->bystander_state = MDTBWitnessStateCooldown;
                    state->bystander_state_timer = kBystanderCooldownDuration;
                }
                break;
            case MDTBWitnessStateCooldown:
            default:
                desired_bystander = state->bystander_state_timer > (kBystanderCooldownDuration * 0.42f)
                    ? kBystanderFleePosition
                    : kBystanderBasePosition;
                bystander_speed = 3.3f;
                desired_heading_target = desired_bystander;
                if (state->bystander_state_timer <= 0.0f &&
                    distance_squared_xz(state->bystander_position, kBystanderBasePosition) <= (2.1f * 2.1f)) {
                    state->bystander_state = MDTBWitnessStateIdle;
                    state->bystander_alert = 0.0f;
                    desired_bystander = kBystanderBasePosition;
                }
                break;
        }

        state->bystander_position = approach_float3(state->bystander_position, desired_bystander, bystander_speed, dt);
        {
            const MDTBFloat3 to_heading_target = make_float3(
                desired_heading_target.x - state->bystander_position.x,
                0.0f,
                desired_heading_target.z - state->bystander_position.z
            );
            if (fabsf(to_heading_target.x) > 0.001f || fabsf(to_heading_target.z) > 0.001f) {
                state->bystander_heading = approach_angle(
                    state->bystander_heading,
                    atan2f(to_heading_target.x, -to_heading_target.z),
                    7.0f,
                    dt
                );
            }
        }
    }

    {
        const int civilians_agitated =
            state->witness_state == MDTBWitnessStateInvestigate ||
            state->witness_state == MDTBWitnessStateFlee ||
            state->bystander_state == MDTBWitnessStateInvestigate ||
            state->bystander_state == MDTBWitnessStateFlee;
        const int street_normalizing =
            state->street_incident_timer > 0.0f &&
            state->street_incident_level > 0.08f &&
            state->combat_hostile_search_timer <= 0.0f &&
            state->combat_hostile_attack_windup <= 0.0f &&
            state->combat_hostile_last_shot_timer <= 0.0f &&
            state->firearm_last_shot_timer <= 0.0f &&
            !civilians_agitated;

        if (street_normalizing) {
            float normalize_boost =
                state->traversal_mode == MDTBTraversalModeVehicle ?
                kStreetIncidentVehicleNormalizationBoost :
                kStreetIncidentNormalizationBoost;

            if (active_civilian_response_count(state) == 0) {
                normalize_boost += 0.55f;
            }

            state->street_incident_timer = fmaxf(state->street_incident_timer - dt * normalize_boost, 0.0f);
            state->street_incident_level = approachf(state->street_incident_level, 0.0f, 0.56f + normalize_boost * 0.16f, dt);

            if (state->player_recovery_delay > 0.0f) {
                state->player_recovery_delay = fmaxf(state->player_recovery_delay - dt * 0.42f, 0.0f);
            }

            if (state->street_incident_timer <= 0.0f && state->street_incident_level <= 0.10f) {
                const MDTBFloat3 recovery_position = state->street_incident_position;
                const float civilian_recovery = (float)active_civilian_response_count(state) * 0.06f;
                const float recovery_level = clampf(
                    0.16f +
                    civilian_recovery +
                    (state->traversal_mode == MDTBTraversalModeVehicle ? 0.06f : 0.0f),
                    0.12f,
                    0.34f
                );
                const float recovery_duration =
                    kStreetRecoveryDuration +
                    (float)active_civilian_response_count(state) * 0.55f +
                    (state->traversal_mode == MDTBTraversalModeVehicle ? 0.45f : 0.0f);

                trigger_street_recovery(state, recovery_position, recovery_level, recovery_duration);
                state->street_incident_level = 0.0f;
                state->street_incident_timer = 0.0f;
                state->street_incident_position = kCombatLaneResetPosition;
            }
        }
    }

    if (state->traversal_mode != MDTBTraversalModeOnFoot) {
        if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
            const MDTBFloat3 retreat_anchor = lookout_anchor_position(3u);
            state->combat_hostile_anchor_index = 3u;
            state->combat_hostile_reposition_timer = 0.0f;
            start_hostile_search(state, state->combat_hostile_search_position, kLookoutVehicleReacquireDuration);
            state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, kLookoutVehicleReacquireDuration);
            state->combat_hostile_position = approach_float3(state->combat_hostile_position, retreat_anchor, 4.8f, dt);
            state->combat_hostile_heading = approach_angle(state->combat_hostile_heading, kPi, 6.2f, dt);
            state->combat_hostile_alert = approachf(state->combat_hostile_alert, 0.20f, 4.5f, dt);
            trigger_street_incident(state, state->combat_hostile_search_position, 0.58f, kStreetIncidentSearchDuration);
        }

        if (state->player_recovery_delay > 0.0f && state->combat_hostile_search_timer > 0.0f) {
            state->player_recovery_delay = fmaxf(state->player_recovery_delay - dt * 0.75f, 0.0f);
        }
        clear_melee_attack(state);
        cancel_firearm_reload(state);
        state->combat_hostile_attack_windup = 0.0f;
        refresh_combat_proximity(state);
        return;
    }

    if (combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
        const MDTBFloat3 actor_position = current_player_position(state);
        const MDTBFloat3 actor_cover_position = player_cover_position(state);
        const uint32_t chosen_anchor = choose_lookout_pressure_anchor(state);
        const int territory_claimed =
            state->territory_faction == MDTBTerritoryFactionCourtSet &&
            state->territory_phase != MDTBTerritoryPhaseNone;
        const int territory_hot = state->territory_phase == MDTBTerritoryPhaseHot;
        const float territory_watch = fmaxf(state->territory_front_watch, state->territory_deep_watch);
        const float territory_patrol_screen =
            state->territory_patrol_state == MDTBTerritoryPatrolScreen
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_brace =
            state->territory_patrol_state == MDTBTerritoryPatrolBrace
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_reform =
            state->territory_patrol_state == MDTBTerritoryPatrolReform
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_handoff =
            state->territory_patrol_state == MDTBTerritoryPatrolHandoff
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_inner_brace =
            state->territory_inner_state == MDTBTerritoryPatrolBrace
            ? state->territory_inner_alert
            : 0.0f;
        const float territory_inner_handoff =
            state->territory_inner_state == MDTBTerritoryPatrolHandoff
            ? state->territory_inner_alert
            : 0.0f;
        const float territory_reclaim_return =
            state->territory_reapproach_mode == MDTBTerritoryReapproachReclaim &&
            state->territory_reapproach_timer > 0.0f
            ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachReclaimDuration + 0.6f), 0.0f, 1.0f)
            : 0.0f;
        const float territory_retake_return =
            state->territory_reapproach_mode == MDTBTerritoryReapproachRetake &&
            state->territory_reapproach_timer > 0.0f
            ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachRetakeDuration + 0.5f), 0.0f, 1.0f)
            : 0.0f;
        const MDTBFloat3 current_shot_origin = hostile_shot_origin(state);
        const int current_angle_blocked = segment_hits_world_cover(current_shot_origin, actor_cover_position, 0.10f, NULL) != 0;
        const int wants_angle_change =
            state->combat_player_in_cover != 0u ||
            current_angle_blocked ||
            state->combat_hostile_reacquire_timer > 0.0f ||
            (state->combat_hostile_attack_cooldown > 0.35f && state->combat_hostile_alert >= 0.78f);
        const float alert_distance_squared = distance_squared_xz(actor_position, state->combat_hostile_position);
        const float alert_target =
            (alert_distance_squared <= (kLookoutAlertDistance * kLookoutAlertDistance) ||
             state->combat_focus_target_kind == MDTBCombatTargetLookout) ? 1.0f : 0.18f;
        MDTBFloat3 desired_position;
        const MDTBFloat3 to_actor = make_float3(
            actor_position.x - state->combat_hostile_position.x,
            0.0f,
            actor_position.z - state->combat_hostile_position.z
        );
        float desired_heading = state->combat_hostile_heading;

        state->combat_hostile_alert = approachf(state->combat_hostile_alert, alert_target, 3.8f, dt);
        if (territory_claimed) {
            const float territory_alert_floor =
                0.16f +
                state->territory_presence * 0.18f +
                state->territory_heat * 0.26f +
                state->territory_front_watch * 0.14f +
                state->territory_deep_watch * 0.18f +
                territory_watch * 0.08f +
                state->territory_patrol_alert * 0.10f +
                territory_patrol_screen * 0.05f +
                territory_patrol_brace * 0.06f +
                territory_patrol_reform * 0.03f +
                territory_patrol_handoff * 0.06f +
                state->territory_inner_alert * 0.10f +
                territory_inner_brace * 0.04f +
                territory_inner_handoff * 0.06f +
                territory_reclaim_return * 0.08f +
                territory_retake_return * 0.06f +
                (territory_hot ? 0.10f : 0.0f);
            state->combat_hostile_alert = fmaxf(state->combat_hostile_alert, territory_alert_floor);
        }

        if (state->combat_player_in_cover != 0u || current_angle_blocked) {
            start_hostile_search(state, actor_position, kLookoutSearchDuration * 0.62f);
            trigger_street_incident(state, actor_position, 0.52f, kStreetIncidentSearchDuration);
        } else {
            state->combat_hostile_search_position = actor_position;
            if (state->combat_hostile_search_timer > 0.0f) {
                state->combat_hostile_search_timer = fmaxf(state->combat_hostile_search_timer - dt * 2.6f, 0.0f);
            }
        }

        if (wants_angle_change &&
            state->combat_hostile_reposition_timer <= 0.0f &&
            chosen_anchor != state->combat_hostile_anchor_index) {
            state->combat_hostile_anchor_index = chosen_anchor;
            state->combat_hostile_reposition_timer = kLookoutRepositionRetargetDelay;
            state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, kLookoutReacquireDuration);
            state->combat_hostile_attack_windup = 0.0f;
        }

        desired_position = lookout_anchor_position(state->combat_hostile_anchor_index);
        desired_position.x += sinf(state->elapsed_time * 1.2f + (float)state->combat_hostile_anchor_index) * 0.14f * state->combat_hostile_alert;
        desired_position.z += cosf(state->elapsed_time * 1.5f + (float)state->combat_hostile_anchor_index) * 0.10f * state->combat_hostile_alert;
        state->combat_hostile_position = approach_float3(
            state->combat_hostile_position,
            desired_position,
            state->combat_hostile_reacquire_timer > 0.0f ? 7.0f : 5.4f,
            dt
        );

        if (fabsf(to_actor.x) > 0.001f || fabsf(to_actor.z) > 0.001f) {
            desired_heading = atan2f(to_actor.x, -to_actor.z);
        }

        state->combat_hostile_heading = approach_angle(state->combat_hostile_heading, desired_heading, 8.4f, dt);

        if (state->combat_hostile_search_timer > 0.0f &&
            state->player_recovery_delay > 0.0f &&
            (state->combat_player_in_cover != 0u || current_angle_blocked)) {
            state->player_recovery_delay = fmaxf(state->player_recovery_delay - dt * 0.65f, 0.0f);
        }
    } else {
        state->combat_hostile_alert = approachf(state->combat_hostile_alert, 0.0f, 6.0f, dt);
        state->combat_hostile_attack_windup = 0.0f;
    }

    if (previous_search_timer > 0.0f &&
        state->combat_hostile_search_timer <= 0.0f &&
        combat_target_kind_is_active(state, MDTBCombatTargetLookout)) {
        state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, kLookoutSearchSettleDuration);
        state->combat_hostile_attack_cooldown = fmaxf(state->combat_hostile_attack_cooldown, 0.32f);
        const MDTBFloat3 settle_position = territory_shoulder_focus_position(
            state,
            state->combat_hostile_search_position,
            3.6f,
            1.2f
        );
        trigger_street_incident(
            state,
            settle_position,
            fmaxf(state->street_incident_level, 0.32f),
            kStreetIncidentSettleDuration
        );
    }

    refresh_combat_proximity(state);

    if (combat_target_kind_is_active(state, MDTBCombatTargetLookout) &&
        state->player_reset_timer <= 0.0f) {
        const MDTBFloat3 player_position = current_player_position(state);
        const int territory_claimed =
            state->territory_faction == MDTBTerritoryFactionCourtSet &&
            state->territory_phase != MDTBTerritoryPhaseNone;
        const int territory_hot = state->territory_phase == MDTBTerritoryPhaseHot;
        const float territory_watch = fmaxf(state->territory_front_watch, state->territory_deep_watch);
        const float territory_patrol_screen =
            state->territory_patrol_state == MDTBTerritoryPatrolScreen
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_brace =
            state->territory_patrol_state == MDTBTerritoryPatrolBrace
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_reform =
            state->territory_patrol_state == MDTBTerritoryPatrolReform
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_patrol_handoff =
            state->territory_patrol_state == MDTBTerritoryPatrolHandoff
            ? state->territory_patrol_alert
            : 0.0f;
        const float territory_inner_brace =
            state->territory_inner_state == MDTBTerritoryPatrolBrace
            ? state->territory_inner_alert
            : 0.0f;
        const float territory_inner_handoff =
            state->territory_inner_state == MDTBTerritoryPatrolHandoff
            ? state->territory_inner_alert
            : 0.0f;
        const float territory_reclaim_return =
            state->territory_reapproach_mode == MDTBTerritoryReapproachReclaim &&
            state->territory_reapproach_timer > 0.0f
            ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachReclaimDuration + 0.6f), 0.0f, 1.0f)
            : 0.0f;
        const float territory_retake_return =
            state->territory_reapproach_mode == MDTBTerritoryReapproachRetake &&
            state->territory_reapproach_timer > 0.0f
            ? clampf(state->territory_reapproach_timer / (kTerritoryReapproachRetakeDuration + 0.5f), 0.0f, 1.0f)
            : 0.0f;
        const float player_distance_squared = distance_squared_xz(player_position, state->combat_hostile_position);
        const float attack_range =
            kLookoutAttackRange +
            (territory_claimed ? (territory_hot ? 2.0f : 1.0f) : 0.0f) +
            territory_watch * 1.6f +
            territory_patrol_screen * 0.6f +
            territory_patrol_brace * 0.8f +
            territory_patrol_reform * 0.3f +
            territory_patrol_handoff * 1.2f +
            territory_inner_brace * 0.6f +
            territory_inner_handoff * 1.0f +
            territory_reclaim_return * 0.8f +
            territory_retake_return * 0.5f;
        const float required_alert =
            territory_hot
            ? fmaxf(0.34f, 0.42f - territory_watch * 0.10f - territory_patrol_screen * 0.04f - territory_patrol_brace * 0.04f - territory_patrol_reform * 0.02f - territory_patrol_handoff * 0.08f - territory_inner_brace * 0.03f - territory_inner_handoff * 0.06f - territory_reclaim_return * 0.04f - territory_retake_return * 0.03f)
            : (territory_claimed
                ? fmaxf(0.38f, 0.50f - territory_watch * 0.12f - territory_patrol_screen * 0.04f - territory_patrol_brace * 0.04f - territory_patrol_reform * 0.02f - territory_patrol_handoff * 0.08f - territory_inner_brace * 0.03f - territory_inner_handoff * 0.06f - territory_reclaim_return * 0.05f - territory_retake_return * 0.03f)
                : 0.55f);
        const float anchor_error = sqrtf(distance_squared_xz(state->combat_hostile_position, lookout_anchor_position(state->combat_hostile_anchor_index)));
        const int shot_blocked = segment_hits_world_cover(hostile_shot_origin(state), player_cover_position(state), 0.10f, NULL) != 0;

        if (player_distance_squared <= (attack_range * attack_range) &&
            state->combat_hostile_alert >= required_alert &&
            anchor_error <= 1.25f &&
            !shot_blocked &&
            state->combat_hostile_search_timer <= 0.0f &&
            state->combat_hostile_reacquire_timer <= 0.0f &&
            state->combat_hostile_attack_windup <= 0.0f &&
            state->combat_hostile_attack_cooldown <= 0.0f) {
            state->combat_hostile_attack_windup = kLookoutAttackWindupDuration;
        }
    }

    if (state->combat_hostile_attack_windup > 0.0f) {
        state->combat_hostile_attack_windup = fmaxf(state->combat_hostile_attack_windup - dt, 0.0f);
        state->combat_hostile_reaction = fmaxf(state->combat_hostile_reaction, 0.34f);
        if (state->combat_hostile_attack_windup <= 0.0f) {
            fire_lookout_shot(state);
        }
    }

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

    if (state->street_incident_timer > 0.0f && state->street_incident_level > 0.08f) {
        const MDTBFloat3 incident_position = state->street_incident_position;
        const uint32_t primary_link_index = nearest_road_link_index_for_position(incident_position);
        const uint32_t incident_axis = nearest_road_axis_for_position(incident_position);
        const uint32_t incident_block_index = nearest_block_index_for_position(incident_position);
        const int civilian_sources = active_civilian_response_count(state);
        const int street_normalizing =
            state->combat_hostile_search_timer <= 0.0f &&
            state->combat_hostile_attack_windup <= 0.0f &&
            state->witness_state != MDTBWitnessStateInvestigate &&
            state->witness_state != MDTBWitnessStateFlee &&
            state->bystander_state != MDTBWitnessStateInvestigate &&
            state->bystander_state != MDTBWitnessStateFlee;
        const float normalization_scale = street_normalizing ? 0.72f : 1.0f;
        const float incident_radius = (2.8f + state->street_incident_level * 3.4f) * (street_normalizing ? 0.84f : 1.0f);
        const float incident_strength = (0.46f + state->street_incident_level * 0.44f) * normalization_scale;

        push_traffic_occupancy(
            incident_position,
            incident_radius,
            incident_block_index,
            incident_axis,
            MDTBTrafficOccupancyReasonIncident,
            incident_strength
        );

        if (state->combat_hostile_search_timer > 0.0f || state->witness_state == MDTBWitnessStateFlee) {
            const MDTBFloat3 spill_position = make_float3(
                (incident_position.x + state->witness_position.x) * 0.5f,
                incident_position.y,
                (incident_position.z + state->witness_position.z) * 0.5f
            );
            push_traffic_occupancy(
                spill_position,
                2.0f + state->street_incident_level * 2.2f,
                nearest_block_index_for_position(spill_position),
                nearest_road_axis_for_position(spill_position),
                MDTBTrafficOccupancyReasonIncident,
                0.34f + state->street_incident_level * 0.34f
            );
        }

        if (state->witness_state != MDTBWitnessStateIdle) {
            push_traffic_occupancy(
                state->witness_position,
                1.6f + state->witness_alert * 1.4f,
                nearest_block_index_for_position(state->witness_position),
                nearest_road_axis_for_position(state->witness_position),
                MDTBTrafficOccupancyReasonIncident,
                0.22f + state->witness_alert * 0.34f
            );
        }

        if (state->bystander_state != MDTBWitnessStateIdle) {
            push_traffic_occupancy(
                state->bystander_position,
                1.8f + state->bystander_alert * 1.8f,
                nearest_block_index_for_position(state->bystander_position),
                nearest_road_axis_for_position(state->bystander_position),
                MDTBTrafficOccupancyReasonIncident,
                0.26f + state->bystander_alert * 0.38f
            );
        }

        if (primary_link_index < g_road_link_count) {
            const MDTBRoadLink *primary_link = &g_road_links[primary_link_index];
            const float link_strength =
                (0.22f + state->street_incident_level * 0.20f + (float)civilian_sources * 0.05f) *
                normalization_scale;
            const float link_radius = (2.1f + state->street_incident_level * 1.8f) * (street_normalizing ? 0.88f : 1.0f);

            push_traffic_occupancy(
                primary_link->midpoint,
                link_radius,
                nearest_block_index_for_position(primary_link->midpoint),
                primary_link->axis,
                MDTBTrafficOccupancyReasonIncident,
                link_strength
            );

            if (!street_normalizing &&
                (civilian_sources >= 2 || state->street_incident_level >= 0.64f)) {
                for (size_t index = 0u; index < g_road_link_count; ++index) {
                    const MDTBRoadLink *link = &g_road_links[index];
                    const int shares_primary_block =
                        link->from_block_index == primary_link->from_block_index ||
                        link->from_block_index == primary_link->to_block_index ||
                        link->to_block_index == primary_link->from_block_index ||
                        link->to_block_index == primary_link->to_block_index;

                    if (index == primary_link_index || !shares_primary_block) {
                        continue;
                    }

                    push_traffic_occupancy(
                        link->midpoint,
                        1.7f + state->street_incident_level * 1.4f,
                        nearest_block_index_for_position(link->midpoint),
                        link->axis,
                        MDTBTrafficOccupancyReasonIncident,
                        0.14f + state->street_incident_level * 0.18f + (float)civilian_sources * 0.03f
                    );
                }
            }
        }
    } else if (state->street_recovery_timer > 0.0f && state->street_recovery_level > 0.04f) {
        const MDTBFloat3 recovery_position = state->street_recovery_position;
        const uint32_t recovery_link_index = nearest_road_link_index_for_position(recovery_position);
        const uint32_t recovery_axis = nearest_road_axis_for_position(recovery_position);
        const uint32_t recovery_block_index = nearest_block_index_for_position(recovery_position);
        const int civilian_sources = active_civilian_response_count(state);
        const float preferred_side_strength = territory_preferred_side_strength(state);
        const float recovery_radius = 1.8f + state->street_recovery_level * 2.2f;
        const float recovery_strength =
            0.16f +
            state->street_recovery_level * 0.18f +
            (float)civilian_sources * 0.03f;

        push_traffic_occupancy(
            recovery_position,
            recovery_radius,
            recovery_block_index,
            recovery_axis,
            MDTBTrafficOccupancyReasonIncident,
            recovery_strength
        );

        if (state->witness_state == MDTBWitnessStateCooldown || state->witness_alert > 0.05f) {
            push_traffic_occupancy(
                state->witness_position,
                1.2f + state->witness_alert * 1.1f,
                nearest_block_index_for_position(state->witness_position),
                nearest_road_axis_for_position(state->witness_position),
                MDTBTrafficOccupancyReasonIncident,
                0.12f + state->witness_alert * 0.18f
            );
        }

        if (state->bystander_state == MDTBWitnessStateCooldown || state->bystander_alert > 0.05f) {
            push_traffic_occupancy(
                state->bystander_position,
                1.4f + state->bystander_alert * 1.2f,
                nearest_block_index_for_position(state->bystander_position),
                nearest_road_axis_for_position(state->bystander_position),
                MDTBTrafficOccupancyReasonIncident,
                0.14f + state->bystander_alert * 0.18f
            );
        }

        if (recovery_link_index < g_road_link_count) {
            const MDTBRoadLink *recovery_link = &g_road_links[recovery_link_index];

            push_traffic_occupancy(
                recovery_link->midpoint,
                1.6f + state->street_recovery_level * 1.4f,
                nearest_block_index_for_position(recovery_link->midpoint),
                recovery_link->axis,
                MDTBTrafficOccupancyReasonIncident,
                0.12f + state->street_recovery_level * 0.14f + (float)civilian_sources * 0.02f
            );
        }

        if (preferred_side_strength > 0.18f) {
            const MDTBFloat3 spill_position = territory_shoulder_focus_position(
                state,
                recovery_position,
                4.0f,
                0.24f
            );
            const uint32_t spill_link_index = nearest_road_link_index_for_position(spill_position);

            push_traffic_occupancy(
                spill_position,
                1.4f + state->street_recovery_level * 1.6f,
                nearest_block_index_for_position(spill_position),
                nearest_road_axis_for_position(spill_position),
                MDTBTrafficOccupancyReasonIncident,
                0.10f + state->street_recovery_level * 0.12f + preferred_side_strength * 0.08f
            );

            if (spill_link_index < g_road_link_count && spill_link_index != recovery_link_index) {
                const MDTBRoadLink *spill_link = &g_road_links[spill_link_index];

                push_traffic_occupancy(
                    spill_link->midpoint,
                    1.3f + state->street_recovery_level * 1.2f + preferred_side_strength * 0.4f,
                    nearest_block_index_for_position(spill_link->midpoint),
                    spill_link->axis,
                    MDTBTrafficOccupancyReasonIncident,
                    0.10f + state->street_recovery_level * 0.10f + preferred_side_strength * 0.06f
                );
            }
        }
    }
}

static void push_prop(MDTBFloat3 center, MDTBFloat3 half_extents, MDTBFloat4 color, int is_solid);

static MDTBFloat4 scaled_color(MDTBFloat4 color, float scale) {
    return make_float4(
        clampf(color.x * scale, 0.0f, 1.0f),
        clampf(color.y * scale, 0.0f, 1.0f),
        clampf(color.z * scale, 0.0f, 1.0f),
        color.w
    );
}

static MDTBFloat4 blended_color(MDTBFloat4 lhs, MDTBFloat4 rhs, float amount) {
    const float t = clampf(amount, 0.0f, 1.0f);
    return make_float4(
        lhs.x + ((rhs.x - lhs.x) * t),
        lhs.y + ((rhs.y - lhs.y) * t),
        lhs.z + ((rhs.z - lhs.z) * t),
        lhs.w + ((rhs.w - lhs.w) * t)
    );
}

static void push_building(MDTBFloat3 center, MDTBFloat3 half_extents, MDTBFloat4 color) {
    const MDTBFloat4 plinth_color = scaled_color(color, 0.72f);
    const MDTBFloat4 parapet_color = scaled_color(color, 1.08f);
    const MDTBFloat4 trim_color = blended_color(color, make_float4(0.74f, 0.76f, 0.78f, 1.0f), 0.24f);
    const MDTBFloat4 glass_color = blended_color(color, make_float4(0.70f, 0.80f, 0.88f, 1.0f), 0.42f);
    const MDTBBox box = make_box(center, half_extents, color);
    const float plinth_half_y = fminf(0.18f, half_extents.y * 0.12f);
    const float parapet_half_y = fminf(0.12f, half_extents.y * 0.08f + 0.04f);
    const float belt_half_y = fminf(0.18f, half_extents.y * 0.10f);
    const float front_window_half_x = fmaxf(half_extents.x * 0.62f, 0.52f);
    const float side_window_half_z = fmaxf(half_extents.z * 0.52f, 0.52f);
    const float trim_half_x = fmaxf(half_extents.x - 0.20f, 0.18f);
    const float trim_half_z = fmaxf(half_extents.z - 0.20f, 0.18f);
    const float corner_x = fmaxf(half_extents.x - 0.16f, 0.0f);
    const float corner_z = fmaxf(half_extents.z - 0.16f, 0.0f);
    const float column_half_y = fmaxf(half_extents.y - 0.22f, 0.30f);

    push_scene_box(box);
    push_collision_box(box);

    push_prop(
        make_float3(center.x, center.y - half_extents.y + plinth_half_y, center.z),
        make_float3(half_extents.x + 0.10f, plinth_half_y, half_extents.z + 0.10f),
        plinth_color,
        0
    );
    push_prop(
        make_float3(center.x, center.y + half_extents.y + parapet_half_y, center.z),
        make_float3(half_extents.x + 0.16f, parapet_half_y, half_extents.z + 0.16f),
        parapet_color,
        0
    );
    push_prop(
        make_float3(center.x, center.y + (half_extents.y * 0.10f), center.z + half_extents.z - 0.10f),
        make_float3(front_window_half_x, belt_half_y, 0.05f),
        glass_color,
        0
    );
    push_prop(
        make_float3(center.x, center.y + (half_extents.y * 0.10f), center.z - half_extents.z + 0.10f),
        make_float3(front_window_half_x, belt_half_y, 0.05f),
        glass_color,
        0
    );
    push_prop(
        make_float3(center.x + half_extents.x - 0.10f, center.y + (half_extents.y * 0.10f), center.z),
        make_float3(0.05f, belt_half_y, side_window_half_z),
        glass_color,
        0
    );
    push_prop(
        make_float3(center.x - half_extents.x + 0.10f, center.y + (half_extents.y * 0.10f), center.z),
        make_float3(0.05f, belt_half_y, side_window_half_z),
        glass_color,
        0
    );
    push_prop(
        make_float3(center.x, center.y + (half_extents.y * 0.58f), center.z + half_extents.z - 0.08f),
        make_float3(trim_half_x, 0.05f, 0.03f),
        trim_color,
        0
    );
    push_prop(
        make_float3(center.x, center.y + (half_extents.y * 0.58f), center.z - half_extents.z + 0.08f),
        make_float3(trim_half_x, 0.05f, 0.03f),
        trim_color,
        0
    );
    push_prop(
        make_float3(center.x + half_extents.x - 0.08f, center.y + (half_extents.y * 0.58f), center.z),
        make_float3(0.03f, 0.05f, trim_half_z),
        trim_color,
        0
    );
    push_prop(
        make_float3(center.x - half_extents.x + 0.08f, center.y + (half_extents.y * 0.58f), center.z),
        make_float3(0.03f, 0.05f, trim_half_z),
        trim_color,
        0
    );

    if (corner_x > 0.0f && corner_z > 0.0f) {
        push_prop(make_float3(center.x - corner_x, center.y, center.z - corner_z), make_float3(0.06f, column_half_y, 0.06f), parapet_color, 0);
        push_prop(make_float3(center.x - corner_x, center.y, center.z + corner_z), make_float3(0.06f, column_half_y, 0.06f), parapet_color, 0);
        push_prop(make_float3(center.x + corner_x, center.y, center.z - corner_z), make_float3(0.06f, column_half_y, 0.06f), parapet_color, 0);
        push_prop(make_float3(center.x + corner_x, center.y, center.z + corner_z), make_float3(0.06f, column_half_y, 0.06f), parapet_color, 0);
    }

    if (half_extents.x > 2.2f && half_extents.z > 2.4f && half_extents.y > 1.8f) {
        push_prop(
            make_float3(center.x, center.y + half_extents.y + 0.34f, center.z - (half_extents.z * 0.12f)),
            make_float3(fmaxf(half_extents.x * 0.18f, 0.32f), 0.18f, fmaxf(half_extents.z * 0.14f, 0.28f)),
            scaled_color(color, 0.56f),
            0
        );
        push_prop(
            make_float3(center.x, center.y + half_extents.y + 0.58f, center.z - (half_extents.z * 0.12f)),
            make_float3(fmaxf(half_extents.x * 0.22f, 0.40f), 0.04f, fmaxf(half_extents.z * 0.18f, 0.34f)),
            trim_color,
            0
        );
    }
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
    const MDTBFloat4 slat_color = scaled_color(wood_color, 1.10f);

    push_prop(make_float3(x, 0.36f, z), make_float3(0.62f, 0.04f, 0.06f), slat_color, 1);
    push_prop(make_float3(x, 0.44f, z), make_float3(0.62f, 0.04f, 0.06f), slat_color, 1);
    push_prop(make_float3(x, 0.52f, z), make_float3(0.62f, 0.04f, 0.06f), slat_color, 1);
    push_prop(make_float3(x, 0.72f, z - 0.20f), make_float3(0.62f, 0.04f, 0.05f), wood_color, 1);
    push_prop(make_float3(x, 0.82f, z - 0.18f), make_float3(0.62f, 0.04f, 0.05f), wood_color, 1);
    push_prop(make_float3(x, 0.92f, z - 0.16f), make_float3(0.62f, 0.04f, 0.05f), wood_color, 1);

    push_prop(make_float3(x - 0.44f, 0.24f, z), make_float3(0.05f, 0.24f, 0.05f), metal_color, 1);
    push_prop(make_float3(x + 0.44f, 0.24f, z), make_float3(0.05f, 0.24f, 0.05f), metal_color, 1);
    push_prop(make_float3(x - 0.42f, 0.54f, z - 0.18f), make_float3(0.04f, 0.30f, 0.04f), metal_color, 1);
    push_prop(make_float3(x + 0.42f, 0.54f, z - 0.18f), make_float3(0.04f, 0.30f, 0.04f), metal_color, 1);
    push_prop(make_float3(x - 0.30f, 0.50f, z - 0.06f), make_float3(0.03f, 0.18f, 0.03f), metal_color, 1);
    push_prop(make_float3(x + 0.30f, 0.50f, z - 0.06f), make_float3(0.03f, 0.18f, 0.03f), metal_color, 1);
    push_prop(make_float3(x, 0.54f, z - 0.02f), make_float3(0.50f, 0.03f, 0.03f), metal_color, 1);
}

static void push_planter(float x, float z, float size) {
    const MDTBFloat4 shell_color = make_float4(0.49f, 0.44f, 0.39f, 1.0f);
    const MDTBFloat4 rim_color = scaled_color(shell_color, 1.10f);
    const MDTBFloat4 soil_color = make_float4(0.22f, 0.16f, 0.11f, 1.0f);
    const MDTBFloat4 leaf_color = make_float4(0.31f, 0.49f, 0.28f, 1.0f);
    const float wall_half = fmaxf(size * 0.22f, 0.12f);
    const float inset_size = fmaxf(size - wall_half - 0.04f, size * 0.38f);

    push_prop(
        make_float3(x, 0.28f, z),
        make_float3(size, 0.14f, size),
        shell_color,
        1
    );
    push_prop(
        make_float3(x - size + wall_half, 0.42f, z),
        make_float3(wall_half, 0.14f, size),
        shell_color,
        1
    );
    push_prop(
        make_float3(x + size - wall_half, 0.42f, z),
        make_float3(wall_half, 0.14f, size),
        shell_color,
        1
    );
    push_prop(
        make_float3(x, 0.42f, z - size + wall_half),
        make_float3(size, 0.14f, wall_half),
        shell_color,
        1
    );
    push_prop(
        make_float3(x, 0.42f, z + size - wall_half),
        make_float3(size, 0.14f, wall_half),
        shell_color,
        1
    );
    push_prop(
        make_float3(x, 0.58f, z),
        make_float3(size + 0.04f, 0.04f, size + 0.04f),
        rim_color,
        0
    );
    push_prop(
        make_float3(x, 0.62f, z),
        make_float3(inset_size, 0.04f, inset_size),
        soil_color,
        0
    );
    push_prop(
        make_float3(x - (inset_size * 0.28f), 0.84f, z - (inset_size * 0.12f)),
        make_float3(inset_size * 0.34f, 0.24f, inset_size * 0.28f),
        leaf_color,
        0
    );
    push_prop(
        make_float3(x + (inset_size * 0.22f), 0.92f, z + (inset_size * 0.18f)),
        make_float3(inset_size * 0.28f, 0.30f, inset_size * 0.26f),
        scaled_color(leaf_color, 1.06f),
        0
    );
    push_prop(
        make_float3(x, 0.76f, z + (inset_size * 0.04f)),
        make_float3(inset_size * 0.26f, 0.18f, inset_size * 0.20f),
        scaled_color(leaf_color, 0.92f),
        0
    );
}

static void push_signal_head_visor_side_attachments(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 strap_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.38f, 0.39f, 0.42f, 1.0f);

    push_prop(
        make_float3(x - 0.074f, y + 0.028f, z + (facing_sign * 0.086f)),
        make_float3(0.006f, 0.018f, 0.012f),
        strap_color,
        0
    );
    push_prop(
        make_float3(x + 0.074f, y + 0.028f, z + (facing_sign * 0.086f)),
        make_float3(0.006f, 0.018f, 0.012f),
        strap_color,
        0
    );
    push_prop(
        make_float3(x - 0.058f, y + 0.058f, z + (facing_sign * 0.080f)),
        make_float3(0.010f, 0.008f, 0.010f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x + 0.058f, y + 0.058f, z + (facing_sign * 0.080f)),
        make_float3(0.010f, 0.008f, 0.010f),
        cap_color,
        0
    );
}

static void push_signal_head_visor_cap_seams(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 seam_color = make_float4(0.23f, 0.24f, 0.27f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.36f, 0.37f, 0.40f, 1.0f);

    push_prop(
        make_float3(x - 0.042f, y + 0.072f, z + (facing_sign * 0.118f)),
        make_float3(0.006f, 0.010f, 0.008f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x + 0.042f, y + 0.072f, z + (facing_sign * 0.118f)),
        make_float3(0.006f, 0.010f, 0.008f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.076f, z + (facing_sign * 0.112f)),
        make_float3(0.018f, 0.006f, 0.010f),
        cap_color,
        0
    );
}

static void push_signal_head_visor_underside_seam(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 seam_color = make_float4(0.13f, 0.14f, 0.16f, 1.0f);
    const MDTBFloat4 tie_color = make_float4(0.22f, 0.23f, 0.26f, 1.0f);

    push_prop(
        make_float3(x, y + 0.048f, z + (facing_sign * 0.086f)),
        make_float3(0.050f, 0.004f, 0.006f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x - 0.032f, y + 0.045f, z + (facing_sign * 0.092f)),
        make_float3(0.006f, 0.006f, 0.005f),
        tie_color,
        0
    );
    push_prop(
        make_float3(x + 0.032f, y + 0.045f, z + (facing_sign * 0.092f)),
        make_float3(0.006f, 0.006f, 0.005f),
        tie_color,
        0
    );
}

static void push_signal_head_visor(float x, float y, float z, float facing_sign, MDTBFloat4 visor_color) {
    push_prop(
        make_float3(x, y + 0.07f, z + (facing_sign * 0.10f)),
        make_float3(0.08f, 0.015f, 0.035f),
        visor_color,
        0
    );
    push_prop(
        make_float3(x - 0.06f, y + 0.03f, z + (facing_sign * 0.10f)),
        make_float3(0.015f, 0.04f, 0.03f),
        scaled_color(visor_color, 0.92f),
        0
    );
    push_prop(
        make_float3(x + 0.06f, y + 0.03f, z + (facing_sign * 0.10f)),
        make_float3(0.015f, 0.04f, 0.03f),
        scaled_color(visor_color, 0.92f),
        0
    );
    push_signal_head_visor_underside_seam(x, y, z, facing_sign);
    push_signal_head_visor_cap_seams(x, y, z, facing_sign);
    push_signal_head_visor_side_attachments(x, y, z, facing_sign);
}

static void push_signal_head_face_weathering(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 sun_fade_color = make_float4(0.29f, 0.30f, 0.33f, 1.0f);
    const MDTBFloat4 grime_color = make_float4(0.15f, 0.16f, 0.18f, 1.0f);

    push_prop(
        make_float3(x, y + 0.12f, z + (facing_sign * 0.05f)),
        make_float3(0.10f, 0.03f, 0.01f),
        sun_fade_color,
        0
    );
    push_prop(
        make_float3(x, y - 0.08f, z + (facing_sign * 0.05f)),
        make_float3(0.09f, 0.02f, 0.01f),
        scaled_color(grime_color, 1.04f),
        0
    );
    push_prop(
        make_float3(x, y - 0.24f, z + (facing_sign * 0.05f)),
        make_float3(0.09f, 0.02f, 0.01f),
        grime_color,
        0
    );
}

static void push_signal_head_rear_detail(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 seam_color = make_float4(0.16f, 0.17f, 0.19f, 1.0f);
    const MDTBFloat4 panel_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);
    const MDTBFloat4 latch_color = make_float4(0.38f, 0.39f, 0.42f, 1.0f);

    push_prop(
        make_float3(x, y + 0.12f, z - (facing_sign * 0.09f)),
        make_float3(0.10f, 0.015f, 0.01f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x, y - 0.12f, z - (facing_sign * 0.09f)),
        make_float3(0.10f, 0.015f, 0.01f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x, y, z - (facing_sign * 0.10f)),
        make_float3(0.07f, 0.12f, 0.01f),
        panel_color,
        0
    );
    push_prop(
        make_float3(x, y, z - (facing_sign * 0.11f)),
        make_float3(0.01f, 0.10f, 0.005f),
        scaled_color(seam_color, 1.08f),
        0
    );
    push_prop(
        make_float3(x, y - 0.15f, z - (facing_sign * 0.11f)),
        make_float3(0.02f, 0.02f, 0.005f),
        latch_color,
        0
    );
}

static void push_signal_head_rear_vent_hint(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 frame_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);
    const MDTBFloat4 slot_color = make_float4(0.13f, 0.14f, 0.16f, 1.0f);
    const MDTBFloat4 lip_color = make_float4(0.36f, 0.37f, 0.40f, 1.0f);

    push_prop(
        make_float3(x, y + 0.18f, z - (facing_sign * 0.105f)),
        make_float3(0.045f, 0.035f, 0.005f),
        frame_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.192f, z - (facing_sign * 0.110f)),
        make_float3(0.032f, 0.005f, 0.003f),
        slot_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.172f, z - (facing_sign * 0.110f)),
        make_float3(0.032f, 0.005f, 0.003f),
        slot_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.205f, z - (facing_sign * 0.101f)),
        make_float3(0.038f, 0.004f, 0.004f),
        lip_color,
        0
    );
}

static void push_signal_head_rear_fastener_cluster(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 washer_color = make_float4(0.22f, 0.23f, 0.26f, 1.0f);
    const MDTBFloat4 fastener_color = make_float4(0.38f, 0.39f, 0.42f, 1.0f);

    push_prop(
        make_float3(x - 0.045f, y + 0.05f, z - (facing_sign * 0.112f)),
        make_float3(0.010f, 0.010f, 0.004f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x + 0.045f, y + 0.05f, z - (facing_sign * 0.112f)),
        make_float3(0.010f, 0.010f, 0.004f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x - 0.045f, y - 0.05f, z - (facing_sign * 0.112f)),
        make_float3(0.010f, 0.010f, 0.004f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x + 0.045f, y - 0.05f, z - (facing_sign * 0.112f)),
        make_float3(0.010f, 0.010f, 0.004f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x - 0.045f, y + 0.05f, z - (facing_sign * 0.116f)),
        make_float3(0.005f, 0.005f, 0.003f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x + 0.045f, y + 0.05f, z - (facing_sign * 0.116f)),
        make_float3(0.005f, 0.005f, 0.003f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x - 0.045f, y - 0.05f, z - (facing_sign * 0.116f)),
        make_float3(0.005f, 0.005f, 0.003f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x + 0.045f, y - 0.05f, z - (facing_sign * 0.116f)),
        make_float3(0.005f, 0.005f, 0.003f),
        fastener_color,
        0
    );
}

static void push_signal_head_side_detail(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 hinge_color = make_float4(0.26f, 0.27f, 0.30f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.38f, 0.39f, 0.42f, 1.0f);
    const float side_sign = facing_sign;

    push_prop(
        make_float3(x + (side_sign * 0.13f), y, z - (facing_sign * 0.03f)),
        make_float3(0.01f, 0.16f, 0.015f),
        hinge_color,
        0
    );
    push_prop(
        make_float3(x + (side_sign * 0.135f), y + 0.12f, z - (facing_sign * 0.03f)),
        make_float3(0.015f, 0.025f, 0.02f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x + (side_sign * 0.135f), y - 0.12f, z - (facing_sign * 0.03f)),
        make_float3(0.015f, 0.025f, 0.02f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x + (side_sign * 0.125f), y, z - (facing_sign * 0.08f)),
        make_float3(0.005f, 0.08f, 0.01f),
        scaled_color(hinge_color, 0.92f),
        0
    );
}

static void push_signal_head_rear_coupling_clamp_breakup(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 clamp_color = make_float4(0.18f, 0.19f, 0.22f, 1.0f);
    const MDTBFloat4 tab_color = make_float4(0.34f, 0.35f, 0.38f, 1.0f);

    push_prop(
        make_float3(x, y + 0.14f, z - (facing_sign * 0.118f)),
        make_float3(0.022f, 0.028f, 0.004f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x - 0.022f, y + 0.14f, z - (facing_sign * 0.112f)),
        make_float3(0.006f, 0.012f, 0.004f),
        tab_color,
        0
    );
    push_prop(
        make_float3(x + 0.022f, y + 0.14f, z - (facing_sign * 0.112f)),
        make_float3(0.006f, 0.012f, 0.004f),
        tab_color,
        0
    );
}

static void push_signal_head_rear_coupling(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 conduit_color = make_float4(0.22f, 0.23f, 0.26f, 1.0f);
    const MDTBFloat4 collar_color = make_float4(0.36f, 0.37f, 0.40f, 1.0f);

    push_prop(
        make_float3(x, y + 0.18f, z - (facing_sign * 0.12f)),
        make_float3(0.025f, 0.045f, 0.01f),
        conduit_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.10f, z - (facing_sign * 0.12f)),
        make_float3(0.02f, 0.03f, 0.01f),
        scaled_color(conduit_color, 0.94f),
        0
    );
    push_prop(
        make_float3(x, y + 0.14f, z - (facing_sign * 0.11f)),
        make_float3(0.035f, 0.02f, 0.008f),
        collar_color,
        0
    );
    push_signal_head_rear_coupling_clamp_breakup(x, y, z, facing_sign);
}

static void push_signal_head_lower_lip_seam_hint(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 seam_color = make_float4(0.12f, 0.13f, 0.15f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.28f, 0.29f, 0.32f, 1.0f);

    push_prop(
        make_float3(x, y - 0.312f, z + (facing_sign * 0.010f)),
        make_float3(0.050f, 0.004f, 0.006f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x - 0.040f, y - 0.311f, z + (facing_sign * 0.018f)),
        make_float3(0.006f, 0.004f, 0.005f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x + 0.040f, y - 0.311f, z + (facing_sign * 0.018f)),
        make_float3(0.006f, 0.004f, 0.005f),
        cap_color,
        0
    );
}

static void push_signal_head_lower_drain_lip(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 lip_color = make_float4(0.18f, 0.19f, 0.22f, 1.0f);
    const MDTBFloat4 outlet_color = make_float4(0.30f, 0.31f, 0.34f, 1.0f);

    push_prop(
        make_float3(x, y - 0.31f, z - (facing_sign * 0.01f)),
        make_float3(0.09f, 0.01f, 0.045f),
        lip_color,
        0
    );
    push_prop(
        make_float3(x, y - 0.305f, z + (facing_sign * 0.055f)),
        make_float3(0.035f, 0.008f, 0.01f),
        scaled_color(lip_color, 1.06f),
        0
    );
    push_prop(
        make_float3(x, y - 0.315f, z - (facing_sign * 0.055f)),
        make_float3(0.02f, 0.006f, 0.01f),
        outlet_color,
        0
    );
    push_signal_head_lower_lip_seam_hint(x, y, z, facing_sign);
}

static void push_signal_head_backplate_shim(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 shim_color = make_float4(0.26f, 0.27f, 0.30f, 1.0f);

    push_prop(
        make_float3(x - 0.11f, y, z - (facing_sign * 0.055f)),
        make_float3(0.012f, 0.26f, 0.012f),
        shim_color,
        0
    );
    push_prop(
        make_float3(x + 0.11f, y, z - (facing_sign * 0.055f)),
        make_float3(0.012f, 0.26f, 0.012f),
        shim_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.29f, z - (facing_sign * 0.055f)),
        make_float3(0.08f, 0.012f, 0.012f),
        scaled_color(shim_color, 1.04f),
        0
    );
}

static void push_signal_head_corner_fasteners(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 fastener_color = make_float4(0.40f, 0.41f, 0.44f, 1.0f);
    const MDTBFloat4 washer_color = make_float4(0.22f, 0.23f, 0.26f, 1.0f);

    push_prop(
        make_float3(x - 0.095f, y + 0.24f, z + (facing_sign * 0.012f)),
        make_float3(0.014f, 0.014f, 0.010f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x + 0.095f, y + 0.24f, z + (facing_sign * 0.012f)),
        make_float3(0.014f, 0.014f, 0.010f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x - 0.095f, y - 0.24f, z + (facing_sign * 0.012f)),
        make_float3(0.014f, 0.014f, 0.010f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x + 0.095f, y - 0.24f, z + (facing_sign * 0.012f)),
        make_float3(0.014f, 0.014f, 0.010f),
        washer_color,
        0
    );
    push_prop(
        make_float3(x - 0.095f, y + 0.24f, z + (facing_sign * 0.020f)),
        make_float3(0.008f, 0.008f, 0.006f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x + 0.095f, y + 0.24f, z + (facing_sign * 0.020f)),
        make_float3(0.008f, 0.008f, 0.006f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x - 0.095f, y - 0.24f, z + (facing_sign * 0.020f)),
        make_float3(0.008f, 0.008f, 0.006f),
        fastener_color,
        0
    );
    push_prop(
        make_float3(x + 0.095f, y - 0.24f, z + (facing_sign * 0.020f)),
        make_float3(0.008f, 0.008f, 0.006f),
        fastener_color,
        0
    );
}

static void push_signal_head_side_service_tab(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 tab_color = make_float4(0.23f, 0.24f, 0.27f, 1.0f);
    const MDTBFloat4 latch_color = make_float4(0.37f, 0.38f, 0.41f, 1.0f);
    const float side_sign = -facing_sign;

    push_prop(
        make_float3(x + (side_sign * 0.126f), y + 0.01f, z - (facing_sign * 0.006f)),
        make_float3(0.008f, 0.10f, 0.014f),
        tab_color,
        0
    );
    push_prop(
        make_float3(x + (side_sign * 0.132f), y + 0.09f, z - (facing_sign * 0.004f)),
        make_float3(0.010f, 0.012f, 0.010f),
        latch_color,
        0
    );
    push_prop(
        make_float3(x + (side_sign * 0.132f), y - 0.07f, z - (facing_sign * 0.004f)),
        make_float3(0.010f, 0.012f, 0.010f),
        latch_color,
        0
    );
}

static void push_signal_head_lens_baffle_spacers(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 spacer_color = make_float4(0.18f, 0.19f, 0.21f, 1.0f);
    const MDTBFloat4 rail_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);

    push_prop(
        make_float3(x, y + 0.11f, z + (facing_sign * 0.066f)),
        make_float3(0.086f, 0.012f, 0.010f),
        spacer_color,
        0
    );
    push_prop(
        make_float3(x, y - 0.11f, z + (facing_sign * 0.066f)),
        make_float3(0.086f, 0.012f, 0.010f),
        spacer_color,
        0
    );
    push_prop(
        make_float3(x - 0.086f, y, z + (facing_sign * 0.058f)),
        make_float3(0.010f, 0.24f, 0.010f),
        rail_color,
        0
    );
    push_prop(
        make_float3(x + 0.086f, y, z + (facing_sign * 0.058f)),
        make_float3(0.010f, 0.24f, 0.010f),
        rail_color,
        0
    );
}

static void push_signal_head(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 backplate_color = make_float4(0.18f, 0.19f, 0.21f, 1.0f);
    const MDTBFloat4 body_color = make_float4(0.21f, 0.22f, 0.24f, 1.0f);
    const MDTBFloat4 visor_color = make_float4(0.16f, 0.17f, 0.19f, 1.0f);

    push_prop(
        make_float3(x, y, z - (facing_sign * 0.02f)),
        make_float3(0.16f, 0.36f, 0.08f),
        backplate_color,
        0
    );
    push_signal_head_backplate_shim(x, y, z, facing_sign);
    push_prop(
        make_float3(x, y, z),
        make_float3(0.12f, 0.30f, 0.06f),
        body_color,
        0
    );
    push_signal_head_rear_detail(x, y, z, facing_sign);
    push_signal_head_rear_vent_hint(x, y, z, facing_sign);
    push_signal_head_rear_fastener_cluster(x, y, z, facing_sign);
    push_signal_head_side_detail(x, y, z, facing_sign);
    push_signal_head_rear_coupling(x, y, z, facing_sign);
    push_signal_head_lower_drain_lip(x, y, z, facing_sign);
    push_signal_head_corner_fasteners(x, y, z, facing_sign);
    push_signal_head_side_service_tab(x, y, z, facing_sign);
    push_signal_head_lens_baffle_spacers(x, y, z, facing_sign);
    push_signal_head_face_weathering(x, y, z, facing_sign);
    push_prop(
        make_float3(x, y + 0.22f, z + (facing_sign * 0.08f)),
        make_float3(0.07f, 0.05f, 0.02f),
        make_float4(0.76f, 0.18f, 0.14f, 1.0f),
        0
    );
    push_prop(
        make_float3(x, y, z + (facing_sign * 0.08f)),
        make_float3(0.07f, 0.05f, 0.02f),
        make_float4(0.82f, 0.58f, 0.16f, 1.0f),
        0
    );
    push_prop(
        make_float3(x, y - 0.22f, z + (facing_sign * 0.08f)),
        make_float3(0.07f, 0.05f, 0.02f),
        make_float4(0.22f, 0.60f, 0.22f, 1.0f),
        0
    );
    push_signal_head_visor(x, y + 0.22f, z, facing_sign, visor_color);
    push_signal_head_visor(x, y, z, facing_sign, visor_color);
    push_signal_head_visor(x, y - 0.22f, z, facing_sign, visor_color);
}

static void push_signal_head_mount_gasket(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 gasket_color = make_float4(0.13f, 0.14f, 0.16f, 1.0f);
    const MDTBFloat4 plate_color = make_float4(0.32f, 0.33f, 0.36f, 1.0f);

    push_prop(
        make_float3(x, y + 0.305f, z - (facing_sign * 0.065f)),
        make_float3(0.07f, 0.012f, 0.018f),
        gasket_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.327f, z - (facing_sign * 0.055f)),
        make_float3(0.05f, 0.010f, 0.015f),
        plate_color,
        0
    );
    push_prop(
        make_float3(x - 0.05f, y + 0.304f, z - (facing_sign * 0.058f)),
        make_float3(0.012f, 0.020f, 0.014f),
        scaled_color(gasket_color, 1.04f),
        0
    );
    push_prop(
        make_float3(x + 0.05f, y + 0.304f, z - (facing_sign * 0.058f)),
        make_float3(0.012f, 0.020f, 0.014f),
        scaled_color(gasket_color, 1.04f),
        0
    );
}

static void push_signal_head_mount_clamp_seams(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 seam_color = make_float4(0.24f, 0.25f, 0.28f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.38f, 0.39f, 0.42f, 1.0f);

    push_prop(
        make_float3(x, y + 0.58f, z - (facing_sign * 0.048f)),
        make_float3(0.012f, 0.020f, 0.010f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x - 0.055f, y + 0.58f, z - (facing_sign * 0.042f)),
        make_float3(0.010f, 0.014f, 0.010f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x + 0.055f, y + 0.58f, z - (facing_sign * 0.042f)),
        make_float3(0.010f, 0.014f, 0.010f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.38f, z - (facing_sign * 0.040f)),
        make_float3(0.012f, 0.020f, 0.010f),
        scaled_color(seam_color, 1.04f),
        0
    );
}

static void push_signal_head_mount_brace_edge_breakup(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 edge_color = make_float4(0.19f, 0.20f, 0.23f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.33f, 0.34f, 0.37f, 1.0f);

    push_prop(
        make_float3(x, y + 0.48f, z - (facing_sign * 0.032f)),
        make_float3(0.010f, 0.08f, 0.004f),
        edge_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.54f, z - (facing_sign * 0.010f)),
        make_float3(0.008f, 0.010f, 0.006f),
        cap_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.18f, z - (facing_sign * 0.112f)),
        make_float3(0.022f, 0.010f, 0.004f),
        edge_color,
        0
    );
}

static void push_signal_head_mount(float x, float y, float z, float facing_sign) {
    const MDTBFloat4 brace_color = make_float4(0.28f, 0.30f, 0.33f, 1.0f);
    const MDTBFloat4 clamp_color = make_float4(0.44f, 0.46f, 0.49f, 1.0f);

    push_prop(
        make_float3(x, y + 0.48f, z - (facing_sign * 0.02f)),
        make_float3(0.02f, 0.12f, 0.02f),
        brace_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.58f, z - (facing_sign * 0.02f)),
        make_float3(0.09f, 0.03f, 0.04f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.38f, z - (facing_sign * 0.03f)),
        make_float3(0.11f, 0.03f, 0.03f),
        scaled_color(clamp_color, 0.92f),
        0
    );
    push_prop(
        make_float3(x, y + 0.16f, z - (facing_sign * 0.10f)),
        make_float3(0.04f, 0.12f, 0.02f),
        scaled_color(brace_color, 0.92f),
        0
    );
    push_signal_head_mount_brace_edge_breakup(x, y, z, facing_sign);
    push_signal_head_mount_clamp_seams(x, y, z, facing_sign);
    push_signal_head_mount_gasket(x, y, z, facing_sign);
}

static void push_signal_arm_reinforcement(float x, float z, float arm_sign) {
    const MDTBFloat4 brace_color = make_float4(0.31f, 0.33f, 0.36f, 1.0f);
    const MDTBFloat4 collar_color = make_float4(0.42f, 0.44f, 0.47f, 1.0f);

    push_prop(
        make_float3(x, 2.66f, z + (arm_sign * 0.14f)),
        make_float3(0.12f, 0.05f, 0.12f),
        collar_color,
        0
    );
    push_prop(
        make_float3(x, 2.54f, z + (arm_sign * 0.22f)),
        make_float3(0.10f, 0.04f, 0.16f),
        scaled_color(collar_color, 0.92f),
        0
    );
    push_prop(
        make_float3(x, 2.36f, z + (arm_sign * 0.28f)),
        make_float3(0.04f, 0.16f, 0.14f),
        brace_color,
        0
    );
    push_prop(
        make_float3(x, 2.20f, z + (arm_sign * 0.46f)),
        make_float3(0.03f, 0.12f, 0.14f),
        scaled_color(brace_color, 0.94f),
        0
    );
    push_prop(
        make_float3(x, 2.74f, z + (arm_sign * 0.36f)),
        make_float3(0.09f, 0.03f, 0.12f),
        scaled_color(collar_color, 1.04f),
        0
    );
}

static void push_signal_head_hanger_seam_hint(float x, float y, float z, float arm_sign) {
    const MDTBFloat4 seam_color = make_float4(0.18f, 0.19f, 0.22f, 1.0f);
    const MDTBFloat4 collar_color = make_float4(0.34f, 0.35f, 0.38f, 1.0f);

    push_prop(
        make_float3(x, y, z - (arm_sign * 0.014f)),
        make_float3(0.008f, 0.10f, 0.004f),
        seam_color,
        0
    );
    push_prop(
        make_float3(x, y + 0.09f, z + (arm_sign * 0.006f)),
        make_float3(0.010f, 0.010f, 0.006f),
        collar_color,
        0
    );
}

static void push_pedestrian_signal_readout(float x, float y, float z, int faces_on_x_axis, float facing_sign) {
    const MDTBFloat4 backplate_color = make_float4(0.18f, 0.19f, 0.22f, 1.0f);
    const MDTBFloat4 body_color = make_float4(0.23f, 0.24f, 0.27f, 1.0f);
    const MDTBFloat4 mount_color = make_float4(0.34f, 0.36f, 0.39f, 1.0f);
    const MDTBFloat4 hand_color = make_float4(0.86f, 0.48f, 0.16f, 1.0f);
    const MDTBFloat4 walk_color = make_float4(0.84f, 0.86f, 0.82f, 1.0f);
    const float backplate_half_x = faces_on_x_axis ? 0.04f : 0.13f;
    const float backplate_half_z = faces_on_x_axis ? 0.13f : 0.04f;
    const float body_half_x = faces_on_x_axis ? 0.03f : 0.11f;
    const float body_half_z = faces_on_x_axis ? 0.11f : 0.03f;
    const float light_half_x = faces_on_x_axis ? 0.02f : 0.08f;
    const float light_half_z = faces_on_x_axis ? 0.08f : 0.02f;
    const float rear_offset_x = faces_on_x_axis ? -facing_sign * 0.07f : 0.0f;
    const float rear_offset_z = faces_on_x_axis ? 0.0f : -facing_sign * 0.07f;
    const float face_offset_x = faces_on_x_axis ? facing_sign * 0.02f : 0.0f;
    const float face_offset_z = faces_on_x_axis ? 0.0f : facing_sign * 0.02f;

    push_prop(
        make_float3(x, y, z),
        make_float3(backplate_half_x, 0.23f, backplate_half_z),
        backplate_color,
        0
    );
    push_prop(
        make_float3(x + face_offset_x, y, z + face_offset_z),
        make_float3(body_half_x, 0.19f, body_half_z),
        body_color,
        0
    );
    push_prop(
        make_float3(x + rear_offset_x, y, z + rear_offset_z),
        faces_on_x_axis ? make_float3(0.07f, 0.04f, 0.02f) : make_float3(0.02f, 0.04f, 0.07f),
        mount_color,
        0
    );
    push_prop(
        make_float3(x + (face_offset_x * 1.5f), y + 0.09f, z + (face_offset_z * 1.5f)),
        make_float3(light_half_x, 0.04f, light_half_z),
        hand_color,
        0
    );
    push_prop(
        make_float3(x + (face_offset_x * 1.5f), y - 0.09f, z + (face_offset_z * 1.5f)),
        make_float3(light_half_x, 0.04f, light_half_z),
        walk_color,
        0
    );
    push_prop(
        make_float3(x + face_offset_x, y + 0.20f, z + face_offset_z),
        faces_on_x_axis ? make_float3(0.03f, 0.02f, 0.10f) : make_float3(0.10f, 0.02f, 0.03f),
        scaled_color(body_color, 0.90f),
        0
    );
}

static void push_signal_mast_wiring(float x, float z, float arm_sign) {
    const MDTBFloat4 cable_color = make_float4(0.17f, 0.18f, 0.20f, 1.0f);
    const MDTBFloat4 clamp_color = make_float4(0.35f, 0.37f, 0.40f, 1.0f);

    push_prop(
        make_float3(x, 2.82f, z + (arm_sign * 0.62f)),
        make_float3(0.02f, 0.02f, 0.70f),
        cable_color,
        0
    );
    push_prop(
        make_float3(x, 2.70f, z + (arm_sign * 0.16f)),
        make_float3(0.02f, 0.10f, 0.02f),
        cable_color,
        0
    );
    push_prop(
        make_float3(x, 2.76f, z + (arm_sign * 0.28f)),
        make_float3(0.06f, 0.02f, 0.04f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x, 2.76f, z + (arm_sign * 0.74f)),
        make_float3(0.06f, 0.02f, 0.04f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x, 2.76f, z + (arm_sign * 1.02f)),
        make_float3(0.40f, 0.02f, 0.03f),
        cable_color,
        0
    );
    push_prop(
        make_float3(x, 2.68f, z + (arm_sign * 0.40f)),
        make_float3(0.04f, 0.02f, 0.03f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x, 2.34f, z + (arm_sign * 0.40f)),
        make_float3(0.02f, 0.14f, 0.02f),
        cable_color,
        0
    );
    push_prop(
        make_float3(x - 0.34f, 2.60f, z + (arm_sign * 1.02f)),
        make_float3(0.04f, 0.02f, 0.03f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x + 0.34f, 2.60f, z + (arm_sign * 1.02f)),
        make_float3(0.04f, 0.02f, 0.03f),
        clamp_color,
        0
    );
    push_prop(
        make_float3(x - 0.34f, 2.46f, z + (arm_sign * 1.02f)),
        make_float3(0.02f, 0.13f, 0.02f),
        cable_color,
        0
    );
    push_prop(
        make_float3(x + 0.34f, 2.46f, z + (arm_sign * 1.02f)),
        make_float3(0.02f, 0.13f, 0.02f),
        cable_color,
        0
    );
}

static void push_signal_pole(float x, float z) {
    const MDTBFloat4 pole_color = make_float4(0.27f, 0.29f, 0.34f, 1.0f);
    const MDTBFloat4 arm_color = make_float4(0.34f, 0.35f, 0.39f, 1.0f);
    const MDTBFloat4 base_color = make_float4(0.48f, 0.48f, 0.46f, 1.0f);
    const float arm_sign = (z > 0.0f) ? -1.0f : 1.0f;

    push_prop(
        make_float3(x, 0.10f, z),
        make_float3(0.20f, 0.10f, 0.20f),
        base_color,
        1
    );
    push_prop(
        make_float3(x, 1.15f, z),
        make_float3(0.08f, 1.15f, 0.08f),
        pole_color,
        1
    );
    push_prop(
        make_float3(x, 0.28f, z),
        make_float3(0.12f, 0.06f, 0.12f),
        scaled_color(base_color, 0.86f),
        0
    );
    push_prop(
        make_float3(x - 0.14f, 1.46f, z - (arm_sign * 0.18f)),
        make_float3(0.05f, 0.24f, 0.12f),
        scaled_color(pole_color, 0.88f),
        0
    );

    push_prop(
        make_float3(x, 2.55f, z + (arm_sign * 0.58f)),
        make_float3(0.10f, 0.10f, 0.72f),
        arm_color,
        0
    );
    push_prop(
        make_float3(x, 2.71f, z + (arm_sign * 0.24f)),
        make_float3(0.09f, 0.05f, 0.38f),
        scaled_color(arm_color, 1.08f),
        0
    );
    push_prop(
        make_float3(x, 2.32f, z + (arm_sign * 0.94f)),
        make_float3(0.03f, 0.25f, 0.03f),
        scaled_color(arm_color, 0.92f),
        0
    );
    push_prop(
        make_float3(x - 0.34f, 2.34f, z + (arm_sign * 1.02f)),
        make_float3(0.03f, 0.26f, 0.03f),
        scaled_color(arm_color, 0.92f),
        0
    );
    push_prop(
        make_float3(x + 0.34f, 2.34f, z + (arm_sign * 1.02f)),
        make_float3(0.03f, 0.26f, 0.03f),
        scaled_color(arm_color, 0.92f),
        0
    );
    push_signal_head_hanger_seam_hint(x, 2.28f, z + (arm_sign * 0.40f), arm_sign);
    push_signal_head_hanger_seam_hint(x - 0.34f, 2.34f, z + (arm_sign * 1.02f), arm_sign);
    push_signal_head_hanger_seam_hint(x + 0.34f, 2.34f, z + (arm_sign * 1.02f), arm_sign);
    push_prop(
        make_float3(x + 0.18f, 1.12f, z - (arm_sign * 0.12f)),
        make_float3(0.06f, 0.16f, 0.10f),
        scaled_color(pole_color, 0.84f),
        0
    );

    push_signal_arm_reinforcement(x, z, arm_sign);
    push_signal_mast_wiring(x, z, arm_sign);
    push_signal_head_mount(x - 0.34f, 2.00f, z + (arm_sign * 1.02f), arm_sign);
    push_signal_head_mount(x + 0.34f, 2.00f, z + (arm_sign * 1.02f), arm_sign);
    push_signal_head_mount(x, 1.94f, z + (arm_sign * 0.40f), arm_sign);
    push_signal_head(x - 0.34f, 2.00f, z + (arm_sign * 1.02f), arm_sign);
    push_signal_head(x + 0.34f, 2.00f, z + (arm_sign * 1.02f), arm_sign);
    push_signal_head(x, 1.94f, z + (arm_sign * 0.40f), arm_sign);
}

static void push_signal_pole_corner_detail(
    MDTBFloat3 origin,
    float x_sign,
    float z_sign,
    float signal_x,
    float signal_z,
    MDTBFloat4 sidewalk_color
) {
    const MDTBFloat4 footing_color = scaled_color(sidewalk_color, 0.80f);
    const MDTBFloat4 seam_color = scaled_color(sidewalk_color, 0.66f);
    const MDTBFloat4 hardware_color = make_float4(0.28f, 0.31f, 0.34f, 1.0f);
    const MDTBFloat4 plate_color = make_float4(0.74f, 0.76f, 0.79f, 1.0f);
    const MDTBFloat4 speaker_color = make_float4(0.22f, 0.24f, 0.26f, 1.0f);
    const MDTBFloat4 button_color = make_float4(0.86f, 0.68f, 0.18f, 1.0f);
    const float pole_x = origin.x + (x_sign * signal_x);
    const float pole_z = origin.z + (z_sign * signal_z);
    const float inward_x = -x_sign;
    const float inward_z = -z_sign;

    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.01f, pole_z),
        make_float3(0.34f, 0.01f, 0.34f),
        footing_color
    ));
    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.02f, pole_z),
        make_float3(0.22f, 0.01f, 0.04f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.02f, pole_z),
        make_float3(0.04f, 0.01f, 0.22f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(pole_x + (inward_x * 0.22f), kSidewalkHeight + 0.01f, pole_z),
        make_float3(0.10f, 0.01f, 0.18f),
        scaled_color(footing_color, 1.04f)
    ));
    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.01f, pole_z + (inward_z * 0.22f)),
        make_float3(0.18f, 0.01f, 0.10f),
        scaled_color(footing_color, 1.04f)
    ));

    push_prop(
        make_float3(pole_x + (inward_x * 0.16f), 1.02f, pole_z + (z_sign * 0.12f)),
        make_float3(0.04f, 0.18f, 0.10f),
        hardware_color,
        0
    );
    push_prop(
        make_float3(pole_x + (inward_x * 0.21f), 1.02f, pole_z + (z_sign * 0.12f)),
        make_float3(0.02f, 0.14f, 0.08f),
        plate_color,
        0
    );
    push_prop(
        make_float3(pole_x + (inward_x * 0.19f), 1.13f, pole_z + (z_sign * 0.12f)),
        make_float3(0.01f, 0.03f, 0.04f),
        speaker_color,
        0
    );
    push_prop(
        make_float3(pole_x + (inward_x * 0.19f), 0.97f, pole_z + (z_sign * 0.12f)),
        make_float3(0.01f, 0.04f, 0.03f),
        button_color,
        0
    );

    push_prop(
        make_float3(pole_x + (x_sign * 0.12f), 1.02f, pole_z + (inward_z * 0.16f)),
        make_float3(0.10f, 0.18f, 0.04f),
        hardware_color,
        0
    );
    push_prop(
        make_float3(pole_x + (x_sign * 0.12f), 1.02f, pole_z + (inward_z * 0.21f)),
        make_float3(0.08f, 0.14f, 0.02f),
        plate_color,
        0
    );
    push_prop(
        make_float3(pole_x + (x_sign * 0.12f), 1.13f, pole_z + (inward_z * 0.19f)),
        make_float3(0.04f, 0.03f, 0.01f),
        speaker_color,
        0
    );
    push_prop(
        make_float3(pole_x + (x_sign * 0.12f), 0.97f, pole_z + (inward_z * 0.19f)),
        make_float3(0.03f, 0.04f, 0.01f),
        button_color,
        0
    );

    push_pedestrian_signal_readout(
        pole_x + (inward_x * 0.29f),
        1.52f,
        pole_z + (z_sign * 0.14f),
        1,
        inward_x
    );
    push_pedestrian_signal_readout(
        pole_x + (x_sign * 0.14f),
        1.52f,
        pole_z + (inward_z * 0.29f),
        0,
        inward_z
    );
}

static void push_utility_pole(float x, float z, int line_on_x_axis, float transformer_side) {
    const MDTBFloat4 wood_color = make_float4(0.46f, 0.36f, 0.24f, 1.0f);
    const MDTBFloat4 brace_color = make_float4(0.31f, 0.30f, 0.28f, 1.0f);
    const MDTBFloat4 hardware_color = make_float4(0.66f, 0.65f, 0.62f, 1.0f);
    const float arm_offset = 0.84f;
    const float transformer_x = line_on_x_axis ? x : x + transformer_side * 0.28f;
    const float transformer_z = line_on_x_axis ? z + transformer_side * 0.28f : z;

    push_prop(
        make_float3(x, 0.12f, z),
        make_float3(0.22f, 0.12f, 0.22f),
        scaled_color(brace_color, 0.84f),
        1
    );
    push_prop(
        make_float3(x, 2.16f, z),
        make_float3(0.11f, 2.16f, 0.11f),
        wood_color,
        1
    );
    push_prop(
        make_float3(x, 0.42f, z),
        make_float3(0.15f, 0.06f, 0.15f),
        scaled_color(wood_color, 0.82f),
        0
    );
    push_prop(
        make_float3(x, 3.92f, z),
        line_on_x_axis ? make_float3(0.08f, 0.08f, 1.02f) : make_float3(1.02f, 0.08f, 0.08f),
        scaled_color(wood_color, 0.94f),
        0
    );
    push_prop(
        make_float3(x, 3.58f, z),
        line_on_x_axis ? make_float3(0.05f, 0.05f, 0.72f) : make_float3(0.72f, 0.05f, 0.05f),
        scaled_color(wood_color, 0.86f),
        0
    );
    push_prop(
        make_float3(x, 4.28f, z),
        make_float3(0.10f, 0.06f, 0.10f),
        scaled_color(wood_color, 1.08f),
        0
    );

    if (line_on_x_axis) {
        push_prop(make_float3(x, 3.78f, z - arm_offset), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x, 3.78f, z), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x, 3.78f, z + arm_offset), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x + (transformer_side * 0.22f), 3.16f, z), make_float3(0.16f, 0.34f, 0.22f), hardware_color, 0);
        push_prop(make_float3(x, 2.38f, z + (transformer_side * 0.18f)), make_float3(0.04f, 0.46f, 0.04f), brace_color, 0);
    } else {
        push_prop(make_float3(x - arm_offset, 3.78f, z), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x, 3.78f, z), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x + arm_offset, 3.78f, z), make_float3(0.06f, 0.06f, 0.06f), hardware_color, 0);
        push_prop(make_float3(x, 3.16f, z + (transformer_side * 0.22f)), make_float3(0.22f, 0.34f, 0.16f), hardware_color, 0);
        push_prop(make_float3(x + (transformer_side * 0.18f), 2.38f, z), make_float3(0.04f, 0.46f, 0.04f), brace_color, 0);
    }

    push_prop(
        make_float3(transformer_x, 2.60f, transformer_z),
        make_float3(0.18f, 0.14f, 0.18f),
        scaled_color(hardware_color, 1.04f),
        0
    );
}

static void push_utility_wire_span_z(float x, float z_start, float z_end, float y, float x_offset) {
    const MDTBFloat4 wire_color = make_float4(0.14f, 0.14f, 0.16f, 1.0f);
    const float center_z = (z_start + z_end) * 0.5f;
    const float half_z = fabsf(z_end - z_start) * 0.5f;
    const float span_sag = fminf(half_z * 0.014f, 0.18f);

    push_prop(
        make_float3(x + x_offset, y + span_sag, center_z - (half_z * 0.5f)),
        make_float3(0.02f, 0.02f, half_z * 0.5f),
        wire_color,
        0
    );
    push_prop(
        make_float3(x + x_offset, y - span_sag, center_z + (half_z * 0.5f)),
        make_float3(0.02f, 0.02f, half_z * 0.5f),
        wire_color,
        0
    );
}

static void push_utility_wire_span_x(float x_start, float x_end, float z, float y, float z_offset) {
    const MDTBFloat4 wire_color = make_float4(0.14f, 0.14f, 0.16f, 1.0f);
    const float center_x = (x_start + x_end) * 0.5f;
    const float half_x = fabsf(x_end - x_start) * 0.5f;
    const float span_sag = fminf(half_x * 0.014f, 0.18f);

    push_prop(
        make_float3(center_x - (half_x * 0.5f), y + span_sag, z + z_offset),
        make_float3(half_x * 0.5f, 0.02f, 0.02f),
        wire_color,
        0
    );
    push_prop(
        make_float3(center_x + (half_x * 0.5f), y - span_sag, z + z_offset),
        make_float3(half_x * 0.5f, 0.02f, 0.02f),
        wire_color,
        0
    );
}

static void push_utility_line_run_z(float x, float z_start, float z_end, float spacing, float transformer_side) {
    float current = z_start;
    float previous = z_start;
    int pole_index = 0;

    while (current <= z_end + 0.01f) {
        const float clamped = fminf(current, z_end);
        push_utility_pole(x, clamped, 0, (pole_index % 2 == 0) ? transformer_side : -transformer_side);

        if (pole_index > 0) {
            push_utility_wire_span_z(x, previous, clamped, 3.86f, -0.32f);
            push_utility_wire_span_z(x, previous, clamped, 3.82f, 0.00f);
            push_utility_wire_span_z(x, previous, clamped, 3.86f, 0.32f);
        }

        previous = clamped;
        current += spacing;
        pole_index += 1;
    }
}

static void push_utility_line_run_x(float x_start, float x_end, float z, float spacing, float transformer_side) {
    float current = x_start;
    float previous = x_start;
    int pole_index = 0;

    while (current <= x_end + 0.01f) {
        const float clamped = fminf(current, x_end);
        push_utility_pole(clamped, z, 1, (pole_index % 2 == 0) ? transformer_side : -transformer_side);

        if (pole_index > 0) {
            push_utility_wire_span_x(previous, clamped, z, 3.86f, -0.32f);
            push_utility_wire_span_x(previous, clamped, z, 3.82f, 0.00f);
            push_utility_wire_span_x(previous, clamped, z, 3.86f, 0.32f);
        }

        previous = clamped;
        current += spacing;
        pole_index += 1;
    }
}

static void push_wheel_stop(float x, float z, float half_x, float half_z, MDTBFloat4 color) {
    push_prop(
        make_float3(x, 0.07f, z),
        make_float3(half_x, 0.07f, half_z),
        color,
        0
    );
    push_prop(
        make_float3(x, 0.13f, z),
        make_float3(half_x * 0.78f, 0.03f, half_z * 0.82f),
        scaled_color(color, 1.08f),
        0
    );
}

static void push_lot_parking_pad(float center_x, float center_z, float half_x, float half_z, float stop_sign, float planter_size, MDTBFloat4 asphalt_color, MDTBFloat4 curb_color) {
    const MDTBFloat4 stripe_color = make_float4(0.88f, 0.86f, 0.72f, 1.0f);
    const MDTBFloat4 stop_color = make_float4(0.78f, 0.79f, 0.76f, 1.0f);
    const MDTBFloat4 tire_track_color = scaled_color(asphalt_color, 0.84f);
    const MDTBFloat4 patch_color = blended_color(asphalt_color, curb_color, 0.12f);
    const MDTBFloat4 oil_color = make_float4(0.17f, 0.17f, 0.18f, 1.0f);
    const float curb_half = 0.14f;
    const float inner_x = center_x - stop_sign * (half_x - 0.18f);
    const float outer_x = center_x + stop_sign * (half_x - 0.18f);
    const float stripe_start = center_z - half_z + 4.4f;
    const float stripe_step = 4.4f;
    const float stop_x = center_x + stop_sign * (half_x - 0.92f);

    push_scene_box(make_box(
        make_float3(center_x, kSidewalkHeight + 0.01f, center_z),
        make_float3(half_x, 0.01f, half_z),
        asphalt_color
    ));
    push_scene_box(make_box(
        make_float3(center_x - (stop_sign * (half_x * 0.18f)), kSidewalkHeight + 0.011f, center_z),
        make_float3(half_x * 0.34f, 0.005f, half_z * 0.84f),
        tire_track_color
    ));
    push_scene_box(make_box(
        make_float3(center_x + (stop_sign * (half_x * 0.14f)), kSidewalkHeight + 0.011f, center_z - (half_z * 0.28f)),
        make_float3(half_x * 0.18f, 0.005f, half_z * 0.14f),
        patch_color
    ));
    push_scene_box(make_box(
        make_float3(center_x - (stop_sign * (half_x * 0.08f)), kSidewalkHeight + 0.011f, center_z + (half_z * 0.26f)),
        make_float3(half_x * 0.22f, 0.005f, half_z * 0.12f),
        patch_color
    ));
    push_scene_box(make_box(
        make_float3(center_x, kSidewalkHeight + 0.02f, center_z - half_z),
        make_float3(half_x, 0.01f, curb_half),
        curb_color
    ));
    push_scene_box(make_box(
        make_float3(center_x, kSidewalkHeight + 0.02f, center_z + half_z),
        make_float3(half_x, 0.01f, curb_half),
        curb_color
    ));
    push_scene_box(make_box(
        make_float3(inner_x, kSidewalkHeight + 0.02f, center_z),
        make_float3(curb_half, 0.01f, half_z),
        curb_color
    ));
    push_scene_box(make_box(
        make_float3(outer_x, kSidewalkHeight + 0.02f, center_z),
        make_float3(curb_half, 0.01f, half_z),
        curb_color
    ));
    push_scene_box(make_box(
        make_float3(center_x, kSidewalkHeight + 0.03f, center_z - half_z + 0.72f),
        make_float3(half_x * 0.92f, 0.01f, 0.08f),
        stripe_color
    ));
    push_scene_box(make_box(
        make_float3(center_x, kSidewalkHeight + 0.03f, center_z + half_z - 0.72f),
        make_float3(half_x * 0.92f, 0.01f, 0.08f),
        stripe_color
    ));

    for (int stripe = 0; stripe < 8; ++stripe) {
        const float stripe_z = stripe_start + ((float)stripe * stripe_step);
        const MDTBFloat4 slot_stripe_color =
            (stripe % 3 == 1)
            ? blended_color(stripe_color, asphalt_color, 0.34f)
            : stripe_color;
        if (stripe_z >= center_z + half_z - 1.1f) {
            break;
        }

        push_scene_box(make_box(
            make_float3(center_x, kSidewalkHeight + 0.03f, stripe_z),
            make_float3(half_x * 0.88f, 0.01f, 0.05f),
            slot_stripe_color
        ));
        push_wheel_stop(stop_x, stripe_z - (stripe_step * 0.5f), 0.54f, 0.14f, stop_color);
    }

    push_scene_box(make_box(
        make_float3(center_x - (stop_sign * (half_x * 0.28f)), kSidewalkHeight + 0.012f, center_z - (half_z * 0.38f)),
        make_float3(0.46f, 0.004f, 0.34f),
        oil_color
    ));
    push_scene_box(make_box(
        make_float3(center_x - (stop_sign * (half_x * 0.22f)), kSidewalkHeight + 0.012f, center_z + (half_z * 0.02f)),
        make_float3(0.52f, 0.004f, 0.28f),
        oil_color
    ));

    push_planter(center_x - (stop_sign * (half_x - 1.55f)), center_z - (half_z - 1.8f), planter_size);
    push_planter(center_x - (stop_sign * (half_x - 1.55f)), center_z + (half_z - 1.8f), planter_size);
}

static void push_parked_car_wheel(float x, float y, float z, float half_x, float radius, float half_z, MDTBFloat4 tire_color, MDTBFloat4 rim_color) {
    const float cap_y_offset = radius * 0.54f;
    const float side_z_offset = half_z * 0.44f;

    push_prop(
        make_float3(x, y, z),
        make_float3(half_x, radius * 0.42f, half_z * 0.76f),
        tire_color,
        0
    );
    push_prop(
        make_float3(x, y + cap_y_offset, z),
        make_float3(half_x * 0.92f, radius * 0.16f, half_z * 0.54f),
        tire_color,
        0
    );
    push_prop(
        make_float3(x, y - cap_y_offset, z),
        make_float3(half_x * 0.92f, radius * 0.16f, half_z * 0.54f),
        tire_color,
        0
    );
    push_prop(
        make_float3(x, y, z + side_z_offset),
        make_float3(half_x * 0.88f, radius * 0.28f, half_z * 0.22f),
        tire_color,
        0
    );
    push_prop(
        make_float3(x, y, z - side_z_offset),
        make_float3(half_x * 0.88f, radius * 0.28f, half_z * 0.22f),
        tire_color,
        0
    );
    push_prop(
        make_float3(x, y, z),
        make_float3(half_x * 0.56f, radius * 0.18f, half_z * 0.34f),
        rim_color,
        0
    );
}

static void push_parked_car(float x, float z, float half_x, float half_z, MDTBFloat4 color) {
    const MDTBFloat4 glass_color = make_float4(0.68f, 0.75f, 0.82f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.13f, 0.15f, 0.17f, 1.0f);
    const MDTBFloat4 tire_color = make_float4(0.09f, 0.10f, 0.11f, 1.0f);
    const MDTBFloat4 rim_color = make_float4(0.55f, 0.59f, 0.64f, 1.0f);
    const float wheel_x = half_x * 0.62f;
    const float wheel_z = half_z * 0.62f;
    const float wheel_radius = 0.32f;
    const float wheel_half_z = 0.16f;

    push_prop(
        make_float3(x, 0.20f, z + 0.04f),
        make_float3(half_x * 0.94f, 0.14f, half_z * 0.86f),
        make_float4(color.x * 0.74f, color.y * 0.74f, color.z * 0.74f, 1.0f),
        1
    );

    push_prop(
        make_float3(x, 0.40f, z + 0.02f),
        make_float3(half_x, 0.20f, half_z * 0.80f),
        color,
        1
    );

    push_prop(
        make_float3(x, 0.58f, z + half_z * 0.48f),
        make_float3(half_x * 0.82f, 0.14f, half_z * 0.30f),
        make_float4(color.x * 0.90f, color.y * 0.90f, color.z * 0.90f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, 0.56f, z - half_z * 0.54f),
        make_float3(half_x * 0.78f, 0.12f, half_z * 0.24f),
        make_float4(color.x * 0.78f, color.y * 0.78f, color.z * 0.78f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, 0.78f, z - half_z * 0.04f),
        make_float3(half_x * 0.58f, 0.18f, half_z * 0.44f),
        make_float4(color.x * 0.88f, color.y * 0.88f, color.z * 0.88f, 1.0f),
        0
    );

    push_prop(
        make_float3(x, 1.00f, z - half_z * 0.10f),
        make_float3(half_x * 0.42f, 0.10f, half_z * 0.20f),
        make_float4(color.x * 0.72f, color.y * 0.72f, color.z * 0.72f, 1.0f),
        0
    );

    push_prop(
        make_float3(x - half_x * 0.56f, 0.84f, z - half_z * 0.02f),
        make_float3(0.08f, 0.12f, half_z * 0.48f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x + half_x * 0.56f, 0.84f, z - half_z * 0.02f),
        make_float3(0.08f, 0.12f, half_z * 0.48f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x, 0.86f, z + half_z * 0.34f),
        make_float3(half_x * 0.40f, 0.10f, half_z * 0.10f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x, 0.84f, z - half_z * 0.46f),
        make_float3(half_x * 0.34f, 0.09f, half_z * 0.10f),
        glass_color,
        0
    );

    push_prop(
        make_float3(x, 0.30f, z + half_z * 0.90f),
        make_float3(half_x * 0.70f, 0.06f, 0.08f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x, 0.30f, z - half_z * 0.88f),
        make_float3(half_x * 0.70f, 0.06f, 0.08f),
        trim_color,
        0
    );

    push_parked_car_wheel(x - wheel_x, wheel_radius + 0.06f, z + wheel_z, 0.12f, wheel_radius, wheel_half_z, tire_color, rim_color);
    push_parked_car_wheel(x + wheel_x, wheel_radius + 0.06f, z + wheel_z, 0.12f, wheel_radius, wheel_half_z, tire_color, rim_color);
    push_parked_car_wheel(x - wheel_x, wheel_radius + 0.06f, z - wheel_z, 0.12f, wheel_radius, wheel_half_z, tire_color, rim_color);
    push_parked_car_wheel(x + wheel_x, wheel_radius + 0.06f, z - wheel_z, 0.12f, wheel_radius, wheel_half_z, tire_color, rim_color);
}

static void push_bollard(float x, float z, float height) {
    const MDTBFloat4 shaft_color = make_float4(0.42f, 0.40f, 0.36f, 1.0f);
    const MDTBFloat4 band_color = make_float4(0.85f, 0.68f, 0.19f, 1.0f);

    push_prop(
        make_float3(x, 0.06f, z),
        make_float3(0.16f, 0.06f, 0.16f),
        scaled_color(shaft_color, 0.84f),
        1
    );
    push_prop(
        make_float3(x, height * 0.5f, z),
        make_float3(0.11f, height * 0.5f, 0.11f),
        shaft_color,
        1
    );
    push_prop(
        make_float3(x, height * 0.58f, z),
        make_float3(0.13f, 0.06f, 0.13f),
        band_color,
        0
    );
    push_prop(
        make_float3(x, height * 0.82f, z),
        make_float3(0.12f, 0.05f, 0.12f),
        scaled_color(band_color, 1.06f),
        0
    );
    push_prop(
        make_float3(x, height + 0.05f, z),
        make_float3(0.13f, 0.05f, 0.13f),
        scaled_color(shaft_color, 1.06f),
        0
    );
}

static void push_corner_plaza_pad(MDTBFloat3 origin, float x_offset, float z_offset, float half_x, float half_z, MDTBFloat4 color) {
    push_prop(
        make_float3(origin.x + x_offset, kSidewalkHeight * 0.5f, origin.z + z_offset),
        make_float3(half_x, kSidewalkHeight * 0.5f, half_z),
        color,
        1
    );
}

static void push_refuge_island(float x, float z, float half_x, float half_z, float planter_size, MDTBFloat4 curb_color) {
    const MDTBFloat4 cap_color = scaled_color(curb_color, 1.08f);
    const MDTBFloat4 seam_color = scaled_color(curb_color, 0.84f);
    const MDTBFloat4 stripe_color = make_float4(0.90f, 0.88f, 0.74f, 1.0f);

    push_prop(
        make_float3(x, kSidewalkHeight * 0.5f, z),
        make_float3(half_x, kSidewalkHeight * 0.5f, half_z),
        curb_color,
        1
    );

    if (half_x >= half_z) {
        push_prop(
            make_float3(x, kSidewalkHeight + 0.01f, z),
            make_float3(fmaxf(half_x * 0.72f, 0.54f), 0.01f, 0.08f),
            seam_color,
            0
        );
        push_prop(
            make_float3(x, kSidewalkHeight * 0.5f, z - (half_z + 0.22f)),
            make_float3(fmaxf(half_x * 0.54f, 0.44f), kSidewalkHeight * 0.5f, 0.16f),
            cap_color,
            1
        );
        push_prop(
            make_float3(x, kSidewalkHeight * 0.5f, z + (half_z + 0.22f)),
            make_float3(fmaxf(half_x * 0.54f, 0.44f), kSidewalkHeight * 0.5f, 0.16f),
            cap_color,
            1
        );

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(x, kRoadHeight + 0.01f, z + ((float)side * (half_z + 0.54f))),
                make_float3(fmaxf(half_x * 0.46f, 0.36f), 0.01f, 0.08f),
                stripe_color
            ));
            push_scene_box(make_box(
                make_float3(x - (half_x * 0.34f), kRoadHeight + 0.01f, z + ((float)side * (half_z + 0.92f))),
                make_float3(0.18f, 0.01f, 0.08f),
                stripe_color
            ));
            push_scene_box(make_box(
                make_float3(x + (half_x * 0.34f), kRoadHeight + 0.01f, z + ((float)side * (half_z + 0.92f))),
                make_float3(0.18f, 0.01f, 0.08f),
                stripe_color
            ));
        }
    } else {
        push_prop(
            make_float3(x, kSidewalkHeight + 0.01f, z),
            make_float3(0.08f, 0.01f, fmaxf(half_z * 0.72f, 0.54f)),
            seam_color,
            0
        );
        push_prop(
            make_float3(x - (half_x + 0.22f), kSidewalkHeight * 0.5f, z),
            make_float3(0.16f, kSidewalkHeight * 0.5f, fmaxf(half_z * 0.54f, 0.44f)),
            cap_color,
            1
        );
        push_prop(
            make_float3(x + (half_x + 0.22f), kSidewalkHeight * 0.5f, z),
            make_float3(0.16f, kSidewalkHeight * 0.5f, fmaxf(half_z * 0.54f, 0.44f)),
            cap_color,
            1
        );

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(x + ((float)side * (half_x + 0.54f)), kRoadHeight + 0.01f, z),
                make_float3(0.08f, 0.01f, fmaxf(half_z * 0.46f, 0.36f)),
                stripe_color
            ));
            push_scene_box(make_box(
                make_float3(x + ((float)side * (half_x + 0.92f)), kRoadHeight + 0.01f, z - (half_z * 0.34f)),
                make_float3(0.08f, 0.01f, 0.18f),
                stripe_color
            ));
            push_scene_box(make_box(
                make_float3(x + ((float)side * (half_x + 0.92f)), kRoadHeight + 0.01f, z + (half_z * 0.34f)),
                make_float3(0.08f, 0.01f, 0.18f),
                stripe_color
            ));
        }
    }

    push_planter(x, z, planter_size);
}

static void push_intersection_lane_transition_patch(
    MDTBFloat3 origin,
    uint32_t axis,
    float along_sign,
    float edge_sign,
    float road_half_width,
    float stop_bar_offset,
    MDTBFloat4 road_color
) {
    const MDTBFloat4 patch_color = scaled_color(road_color, 0.84f);
    const float far_along = stop_bar_offset + 2.35f;
    const float mid_along = stop_bar_offset + 1.35f;
    const float near_along = stop_bar_offset + 0.45f;

    if (axis == MDTBRoadAxisNorthSouth) {
        push_scene_box(make_box(
            make_float3(origin.x + (edge_sign * (road_half_width - 0.44f)), kRoadHeight + 0.01f, origin.z + (along_sign * far_along)),
            make_float3(0.24f, 0.01f, 0.95f),
            patch_color
        ));
        push_scene_box(make_box(
            make_float3(origin.x + (edge_sign * (road_half_width - 0.10f)), kRoadHeight + 0.01f, origin.z + (along_sign * mid_along)),
            make_float3(0.27f, 0.01f, 0.72f),
            patch_color
        ));
        push_scene_box(make_box(
            make_float3(origin.x + (edge_sign * (road_half_width + 0.22f)), kRoadHeight + 0.01f, origin.z + (along_sign * near_along)),
            make_float3(0.30f, 0.01f, 0.42f),
            patch_color
        ));
    } else {
        push_scene_box(make_box(
            make_float3(origin.x + (along_sign * far_along), kRoadHeight + 0.01f, origin.z + (edge_sign * (road_half_width - 0.44f))),
            make_float3(0.95f, 0.01f, 0.24f),
            patch_color
        ));
        push_scene_box(make_box(
            make_float3(origin.x + (along_sign * mid_along), kRoadHeight + 0.01f, origin.z + (edge_sign * (road_half_width - 0.10f))),
            make_float3(0.72f, 0.01f, 0.27f),
            patch_color
        ));
        push_scene_box(make_box(
            make_float3(origin.x + (along_sign * near_along), kRoadHeight + 0.01f, origin.z + (edge_sign * (road_half_width + 0.22f))),
            make_float3(0.42f, 0.01f, 0.30f),
            patch_color
        ));
    }
}

static void push_corner_curb_return(
    MDTBFloat3 origin,
    float x_sign,
    float z_sign,
    float crosswalk_offset_x,
    float crosswalk_offset_z,
    MDTBFloat4 curb_color,
    MDTBFloat4 sidewalk_color
) {
    const MDTBFloat4 lip_color = scaled_color(curb_color, 0.96f);
    const MDTBFloat4 landing_color = scaled_color(sidewalk_color, 1.02f);
    const MDTBFloat4 seam_color = scaled_color(sidewalk_color, 0.88f);

    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 0.20f)), 0.09f, origin.z + (z_sign * (crosswalk_offset_z + 1.64f))),
        make_float3(0.20f, 0.09f, 0.78f),
        lip_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 0.90f)), 0.09f, origin.z + (z_sign * (crosswalk_offset_z + 0.90f))),
        make_float3(0.22f, 0.09f, 0.22f),
        lip_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.64f)), 0.09f, origin.z + (z_sign * (crosswalk_offset_z + 0.20f))),
        make_float3(0.78f, 0.09f, 0.20f),
        lip_color
    ));

    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.14f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 1.84f))),
        make_float3(0.50f, kSidewalkHeight * 0.5f, 0.60f),
        landing_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.84f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 1.14f))),
        make_float3(0.60f, kSidewalkHeight * 0.5f, 0.50f),
        landing_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.18f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 1.18f))),
        make_float3(0.40f, kSidewalkHeight * 0.5f, 0.40f),
        scaled_color(landing_color, 1.04f)
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.14f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 1.68f))),
        make_float3(0.34f, 0.01f, 0.06f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.68f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 1.14f))),
        make_float3(0.06f, 0.01f, 0.34f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.34f)), kSidewalkHeight + 0.02f, origin.z + (z_sign * (crosswalk_offset_z + 1.34f))),
        make_float3(0.12f, 0.01f, 0.12f),
        scaled_color(seam_color, 1.05f)
    ));
}

static void build_intersection_mouth_transitions(
    const MDTBBlockDescriptor *block,
    const MDTBIntersectionProfile *profile
) {
    const MDTBRoadProfile *vertical_road =
        profile->vertical != NULL
        ? road_profile_for_class(profile->vertical->road_class)
        : road_profile_for_class(MDTBRoadClassAvenue);
    const MDTBRoadProfile *horizontal_road =
        profile->horizontal != NULL
        ? road_profile_for_class(profile->horizontal->road_class)
        : road_profile_for_class(MDTBRoadClassAvenue);
    const float vertical_crosswalk_offset = profile->vertical != NULL ? profile->vertical->crosswalk_offset : kCrosswalkOffset;
    const float horizontal_crosswalk_offset = profile->horizontal != NULL ? profile->horizontal->crosswalk_offset : kCrosswalkOffset;
    const float northsouth_stop_bar_offset = profile->horizontal != NULL ? profile->horizontal->stop_bar_offset : 6.65f;
    const float eastwest_stop_bar_offset = profile->vertical != NULL ? profile->vertical->stop_bar_offset : 6.65f;
    const MDTBFloat4 vertical_road_color = road_surface_color_for_class(profile->vertical != NULL ? profile->vertical->road_class : MDTBRoadClassAvenue);
    const MDTBFloat4 horizontal_road_color = road_surface_color_for_class(profile->horizontal != NULL ? profile->horizontal->road_class : MDTBRoadClassAvenue);
    const MDTBFloat4 curb_color =
        curb_surface_color_for_class(
            (profile->vertical != NULL && profile->vertical->road_class == MDTBRoadClassBoulevard) ||
            (profile->horizontal != NULL && profile->horizontal->road_class == MDTBRoadClassBoulevard)
            ? MDTBRoadClassBoulevard
            : (profile->vertical != NULL
                ? profile->vertical->road_class
                : (profile->horizontal != NULL ? profile->horizontal->road_class : MDTBRoadClassAvenue))
        );
    const MDTBFloat4 sidewalk_color =
        sidewalk_surface_color_for_class(
            (profile->vertical != NULL && profile->vertical->road_class == MDTBRoadClassBoulevard) ||
            (profile->horizontal != NULL && profile->horizontal->road_class == MDTBRoadClassBoulevard)
            ? MDTBRoadClassBoulevard
            : (profile->vertical != NULL
                ? profile->vertical->road_class
                : (profile->horizontal != NULL ? profile->horizontal->road_class : MDTBRoadClassAvenue))
        );

    for (int sign = -1; sign <= 1; sign += 2) {
        for (int edge_sign = -1; edge_sign <= 1; edge_sign += 2) {
            push_intersection_lane_transition_patch(
                block->origin,
                MDTBRoadAxisNorthSouth,
                (float)sign,
                (float)edge_sign,
                vertical_road->road_half_width,
                northsouth_stop_bar_offset,
                vertical_road_color
            );
            push_intersection_lane_transition_patch(
                block->origin,
                MDTBRoadAxisEastWest,
                (float)sign,
                (float)edge_sign,
                horizontal_road->road_half_width,
                eastwest_stop_bar_offset,
                horizontal_road_color
            );
        }
    }

    for (int x_sign = -1; x_sign <= 1; x_sign += 2) {
        for (int z_sign = -1; z_sign <= 1; z_sign += 2) {
            push_corner_curb_return(
                block->origin,
                (float)x_sign,
                (float)z_sign,
                vertical_crosswalk_offset,
                horizontal_crosswalk_offset,
                curb_color,
                sidewalk_color
            );
        }
    }
}

static void push_crosswalk_landing_corner(
    MDTBFloat3 origin,
    float x_sign,
    float z_sign,
    float crosswalk_offset_x,
    float crosswalk_offset_z,
    MDTBFloat4 sidewalk_color
) {
    const MDTBFloat4 landing_color = scaled_color(sidewalk_color, 1.07f);
    const MDTBFloat4 strip_color = scaled_color(sidewalk_color, 0.92f);

    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.30f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 2.86f))),
        make_float3(0.64f, kSidewalkHeight * 0.5f, 0.56f),
        landing_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.86f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 2.30f))),
        make_float3(0.56f, kSidewalkHeight * 0.5f, 0.64f),
        landing_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.36f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 2.98f))),
        make_float3(0.30f, 0.01f, 0.09f),
        strip_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.98f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 2.36f))),
        make_float3(0.09f, 0.01f, 0.30f),
        strip_color
    ));
}

static void push_crosswalk_edge_treatment_x(float x_center, float z_origin, MDTBFloat4 edge_color, MDTBFloat4 cap_color) {
    push_scene_box(make_box(
        make_float3(x_center - 1.10f, kRoadHeight + 0.01f, z_origin),
        make_float3(0.12f, 0.01f, 5.02f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(x_center + 1.10f, kRoadHeight + 0.01f, z_origin),
        make_float3(0.12f, 0.01f, 5.02f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.01f, z_origin - 5.04f),
        make_float3(0.46f, 0.01f, 0.08f),
        cap_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.01f, z_origin + 5.04f),
        make_float3(0.46f, 0.01f, 0.08f),
        cap_color
    ));
}

static void push_crosswalk_edge_treatment_z(float x_origin, float z_center, MDTBFloat4 edge_color, MDTBFloat4 cap_color) {
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.01f, z_center - 1.10f),
        make_float3(5.02f, 0.01f, 0.12f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.01f, z_center + 1.10f),
        make_float3(5.02f, 0.01f, 0.12f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - 5.04f, kRoadHeight + 0.01f, z_center),
        make_float3(0.08f, 0.01f, 0.46f),
        cap_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 5.04f, kRoadHeight + 0.01f, z_center),
        make_float3(0.08f, 0.01f, 0.46f),
        cap_color
    ));
}

static void push_tactile_warning_pad(
    float x,
    float z,
    int align_on_x_axis,
    MDTBFloat4 pad_color,
    MDTBFloat4 stud_color,
    MDTBFloat4 edge_color
) {
    const float pad_half_x = align_on_x_axis ? 0.48f : 0.12f;
    const float pad_half_z = align_on_x_axis ? 0.12f : 0.48f;
    const float stud_step = 0.18f;

    push_scene_box(make_box(
        make_float3(x, kSidewalkHeight + 0.01f, z),
        make_float3(pad_half_x, 0.01f, pad_half_z),
        pad_color
    ));

    if (align_on_x_axis) {
        push_scene_box(make_box(
            make_float3(x, kSidewalkHeight + 0.02f, z),
            make_float3(pad_half_x * 0.92f, 0.01f, 0.03f),
            edge_color
        ));

        for (int row = -1; row <= 1; row += 2) {
            for (int column = -1; column <= 1; ++column) {
                push_scene_box(make_box(
                    make_float3(x + ((float)column * stud_step), kSidewalkHeight + 0.03f, z + ((float)row * 0.04f)),
                    make_float3(0.04f, 0.01f, 0.03f),
                    stud_color
                ));
            }
        }
    } else {
        push_scene_box(make_box(
            make_float3(x, kSidewalkHeight + 0.02f, z),
            make_float3(0.03f, 0.01f, pad_half_z * 0.92f),
            edge_color
        ));

        for (int row = -1; row <= 1; row += 2) {
            for (int column = -1; column <= 1; ++column) {
                push_scene_box(make_box(
                    make_float3(x + ((float)row * 0.04f), kSidewalkHeight + 0.03f, z + ((float)column * stud_step)),
                    make_float3(0.03f, 0.01f, 0.04f),
                    stud_color
                ));
            }
        }
    }
}

static void push_signal_wait_corner(
    MDTBFloat3 origin,
    float x_sign,
    float z_sign,
    float signal_x,
    float signal_z,
    float crosswalk_offset_x,
    float crosswalk_offset_z,
    MDTBFloat4 sidewalk_color
) {
    const MDTBFloat4 pad_color = scaled_color(sidewalk_color, 1.05f);
    const MDTBFloat4 seam_color = scaled_color(sidewalk_color, 0.78f);
    const MDTBFloat4 approach_color = scaled_color(sidewalk_color, 0.88f);
    const MDTBFloat4 tactile_color = make_float4(0.82f, 0.68f, 0.22f, 1.0f);
    const MDTBFloat4 tactile_stud_color = scaled_color(tactile_color, 1.10f);
    const MDTBFloat4 tactile_edge_color = scaled_color(tactile_color, 0.82f);

    push_prop(
        make_float3(origin.x + (x_sign * (signal_x - 1.34f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (crosswalk_offset_z + 2.04f))),
        make_float3(0.74f, kSidewalkHeight * 0.5f, 0.34f),
        pad_color,
        1
    );
    push_prop(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.04f)), kSidewalkHeight * 0.5f, origin.z + (z_sign * (signal_z - 1.34f))),
        make_float3(0.34f, kSidewalkHeight * 0.5f, 0.74f),
        pad_color,
        1
    );
    push_prop(
        make_float3(origin.x + (x_sign * (signal_x - 1.34f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 2.04f))),
        make_float3(0.64f, 0.01f, 0.05f),
        seam_color,
        0
    );
    push_prop(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 2.04f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (signal_z - 1.34f))),
        make_float3(0.05f, 0.01f, 0.64f),
        seam_color,
        0
    );
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (signal_x - 1.34f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (crosswalk_offset_z + 1.36f))),
        make_float3(0.54f, 0.01f, 0.08f),
        approach_color
    ));
    push_scene_box(make_box(
        make_float3(origin.x + (x_sign * (crosswalk_offset_x + 1.36f)), kSidewalkHeight + 0.01f, origin.z + (z_sign * (signal_z - 1.34f))),
        make_float3(0.08f, 0.01f, 0.54f),
        approach_color
    ));
    push_tactile_warning_pad(
        origin.x + (x_sign * (signal_x - 1.34f)),
        origin.z + (z_sign * (crosswalk_offset_z + 1.56f)),
        1,
        tactile_color,
        tactile_stud_color,
        tactile_edge_color
    );
    push_tactile_warning_pad(
        origin.x + (x_sign * (crosswalk_offset_x + 1.56f)),
        origin.z + (z_sign * (signal_z - 1.34f)),
        0,
        tactile_color,
        tactile_stud_color,
        tactile_edge_color
    );
}

static void push_refuge_throat_surface(
    float x,
    float z,
    int align_on_x_axis,
    float half_x,
    float half_z,
    MDTBFloat4 throat_color,
    MDTBFloat4 seam_color,
    MDTBFloat4 guide_color
) {
    push_scene_box(make_box(
        make_float3(x, kRoadHeight + 0.01f, z),
        make_float3(half_x, 0.01f, half_z),
        throat_color
    ));

    if (align_on_x_axis) {
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.02f, z),
            make_float3(fmaxf(half_x * 0.78f, 0.54f), 0.01f, 0.07f),
            seam_color
        ));
        push_scene_box(make_box(
            make_float3(x - fmaxf(half_x - 0.20f, 0.30f), kRoadHeight + 0.02f, z),
            make_float3(0.08f, 0.01f, fmaxf(half_z * 0.56f, 0.22f)),
            guide_color
        ));
        push_scene_box(make_box(
            make_float3(x + fmaxf(half_x - 0.20f, 0.30f), kRoadHeight + 0.02f, z),
            make_float3(0.08f, 0.01f, fmaxf(half_z * 0.56f, 0.22f)),
            guide_color
        ));
    } else {
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.02f, z),
            make_float3(0.07f, 0.01f, fmaxf(half_z * 0.78f, 0.54f)),
            seam_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.02f, z - fmaxf(half_z - 0.20f, 0.30f)),
            make_float3(fmaxf(half_x * 0.56f, 0.22f), 0.01f, 0.08f),
            guide_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.02f, z + fmaxf(half_z - 0.20f, 0.30f)),
            make_float3(fmaxf(half_x * 0.56f, 0.22f), 0.01f, 0.08f),
            guide_color
        ));
    }
}

static void build_intersection_surface_details(
    const MDTBBlockDescriptor *block,
    const MDTBIntersectionProfile *profile
) {
    const uint32_t dominant_road_class =
        (profile->vertical != NULL && profile->vertical->road_class == MDTBRoadClassBoulevard) ||
        (profile->horizontal != NULL && profile->horizontal->road_class == MDTBRoadClassBoulevard)
        ? MDTBRoadClassBoulevard
        : (profile->vertical != NULL
            ? profile->vertical->road_class
            : (profile->horizontal != NULL ? profile->horizontal->road_class : MDTBRoadClassAvenue));
    const MDTBRoadProfile *vertical_road =
        profile->vertical != NULL
        ? road_profile_for_class(profile->vertical->road_class)
        : road_profile_for_class(MDTBRoadClassAvenue);
    const MDTBRoadProfile *horizontal_road =
        profile->horizontal != NULL
        ? road_profile_for_class(profile->horizontal->road_class)
        : road_profile_for_class(MDTBRoadClassAvenue);
    const float vertical_crosswalk_offset = profile->vertical != NULL ? profile->vertical->crosswalk_offset : kCrosswalkOffset;
    const float horizontal_crosswalk_offset = profile->horizontal != NULL ? profile->horizontal->crosswalk_offset : kCrosswalkOffset;
    const float northsouth_stop_bar_offset = profile->horizontal != NULL ? profile->horizontal->stop_bar_offset : 6.65f;
    const float eastwest_stop_bar_offset = profile->vertical != NULL ? profile->vertical->stop_bar_offset : 6.65f;
    const float signal_x = profile->vertical != NULL ? profile->vertical->signal_offset : 8.9f;
    const float signal_z = profile->horizontal != NULL ? profile->horizontal->signal_offset : 8.9f;
    const float chunk_refuge_scale = profile->chunk != NULL ? profile->chunk->refuge_scale : 1.0f;
    const float center_half_x = fmaxf(fminf(vertical_road->road_half_width - 1.10f, eastwest_stop_bar_offset - 1.25f), 2.10f);
    const float center_half_z = fmaxf(fminf(horizontal_road->road_half_width - 1.10f, northsouth_stop_bar_offset - 1.25f), 2.10f);
    const float wear_center_x = fmaxf(fminf(vertical_crosswalk_offset - 2.10f, vertical_road->road_half_width - 1.55f), 2.15f);
    const float wear_center_z = fmaxf(fminf(horizontal_crosswalk_offset - 2.10f, horizontal_road->road_half_width - 1.55f), 2.15f);
    const MDTBFloat4 road_color = road_surface_color_for_class(dominant_road_class);
    const MDTBFloat4 sidewalk_color = sidewalk_surface_color_for_class(dominant_road_class);
    const MDTBFloat4 center_color = scaled_color(road_color, 0.80f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.58f);
    const MDTBFloat4 wear_color = scaled_color(road_color, 0.68f);
    const MDTBFloat4 crosswalk_edge_color = scaled_color(road_color, 0.62f);
    const MDTBFloat4 crosswalk_cap_color = scaled_color(road_color, 0.76f);
    const MDTBFloat4 throat_guide_color = make_float4(0.88f, 0.86f, 0.72f, 1.0f);

    push_scene_box(make_box(
        make_float3(block->origin.x, kRoadHeight + 0.01f, block->origin.z),
        make_float3(center_half_x, 0.01f, center_half_z),
        center_color
    ));
    push_scene_box(make_box(
        make_float3(block->origin.x, kRoadHeight + 0.02f, block->origin.z),
        make_float3(center_half_x * 0.84f, 0.01f, 0.09f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(block->origin.x, kRoadHeight + 0.02f, block->origin.z),
        make_float3(0.09f, 0.01f, center_half_z * 0.84f),
        seam_color
    ));

    push_crosswalk_edge_treatment_x(block->origin.x - vertical_crosswalk_offset, block->origin.z, crosswalk_edge_color, crosswalk_cap_color);
    push_crosswalk_edge_treatment_x(block->origin.x + vertical_crosswalk_offset, block->origin.z, crosswalk_edge_color, crosswalk_cap_color);
    push_crosswalk_edge_treatment_z(block->origin.x, block->origin.z - horizontal_crosswalk_offset, crosswalk_edge_color, crosswalk_cap_color);
    push_crosswalk_edge_treatment_z(block->origin.x, block->origin.z + horizontal_crosswalk_offset, crosswalk_edge_color, crosswalk_cap_color);

    for (int x_sign = -1; x_sign <= 1; x_sign += 2) {
        for (int z_sign = -1; z_sign <= 1; z_sign += 2) {
            push_scene_box(make_box(
                make_float3(block->origin.x + ((float)x_sign * wear_center_x), kRoadHeight + 0.01f, block->origin.z + ((float)z_sign * (wear_center_z - 0.38f))),
                make_float3(0.54f, 0.01f, 0.96f),
                wear_color
            ));
            push_scene_box(make_box(
                make_float3(block->origin.x + ((float)x_sign * (wear_center_x - 0.38f)), kRoadHeight + 0.01f, block->origin.z + ((float)z_sign * wear_center_z)),
                make_float3(0.96f, 0.01f, 0.54f),
                scaled_color(wear_color, 1.04f)
            ));
            push_crosswalk_landing_corner(
                block->origin,
                (float)x_sign,
                (float)z_sign,
                vertical_crosswalk_offset,
                horizontal_crosswalk_offset,
                sidewalk_color
            );
            push_signal_wait_corner(
                block->origin,
                (float)x_sign,
                (float)z_sign,
                signal_x,
                signal_z,
                vertical_crosswalk_offset,
                horizontal_crosswalk_offset,
                sidewalk_color
            );
        }
    }

    if (profile->vertical != NULL && profile->vertical->road_class == MDTBRoadClassBoulevard) {
        const float throat_half_x = fmaxf(vertical_road->median_half_width + 0.38f, 0.92f);
        const float throat_half_z = 0.58f + ((chunk_refuge_scale - 1.0f) * 0.10f);
        const float throat_z = horizontal_crosswalk_offset + 2.08f;

        push_refuge_throat_surface(
            block->origin.x,
            block->origin.z - throat_z,
            1,
            throat_half_x,
            throat_half_z,
            wear_color,
            seam_color,
            throat_guide_color
        );
        push_refuge_throat_surface(
            block->origin.x,
            block->origin.z + throat_z,
            1,
            throat_half_x,
            throat_half_z,
            wear_color,
            seam_color,
            throat_guide_color
        );
    }

    if (profile->horizontal != NULL && profile->horizontal->road_class == MDTBRoadClassBoulevard) {
        const float throat_half_x = 0.58f + ((chunk_refuge_scale - 1.0f) * 0.10f);
        const float throat_half_z = fmaxf(horizontal_road->median_half_width + 0.38f, 0.92f);
        const float throat_x = vertical_crosswalk_offset + 2.08f;

        push_refuge_throat_surface(
            block->origin.x - throat_x,
            block->origin.z,
            0,
            throat_half_x,
            throat_half_z,
            wear_color,
            seam_color,
            throat_guide_color
        );
        push_refuge_throat_surface(
            block->origin.x + throat_x,
            block->origin.z,
            0,
            throat_half_x,
            throat_half_z,
            wear_color,
            seam_color,
            throat_guide_color
        );
    }
}

static void push_trash_bin(float x, float z) {
    const MDTBFloat4 body_color = make_float4(0.24f, 0.30f, 0.33f, 1.0f);
    const MDTBFloat4 lid_color = make_float4(0.30f, 0.37f, 0.40f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.17f, 0.19f, 0.20f, 1.0f);

    push_prop(
        make_float3(x, 0.10f, z),
        make_float3(0.30f, 0.10f, 0.30f),
        scaled_color(body_color, 0.78f),
        1
    );
    push_prop(
        make_float3(x, 0.46f, z),
        make_float3(0.28f, 0.46f, 0.28f),
        body_color,
        1
    );
    push_prop(
        make_float3(x - 0.21f, 0.46f, z),
        make_float3(0.03f, 0.36f, 0.22f),
        scaled_color(body_color, 0.84f),
        0
    );
    push_prop(
        make_float3(x + 0.21f, 0.46f, z),
        make_float3(0.03f, 0.36f, 0.22f),
        scaled_color(body_color, 0.84f),
        0
    );
    push_prop(
        make_float3(x, 0.92f, z),
        make_float3(0.31f, 0.05f, 0.31f),
        lid_color,
        0
    );
    push_prop(
        make_float3(x, 1.00f, z - 0.02f),
        make_float3(0.25f, 0.03f, 0.27f),
        scaled_color(lid_color, 1.08f),
        0
    );
    push_prop(
        make_float3(x, 0.56f, z + 0.24f),
        make_float3(0.15f, 0.18f, 0.03f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x - 0.16f, 0.18f, z - 0.20f),
        make_float3(0.06f, 0.12f, 0.05f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x + 0.16f, 0.18f, z - 0.20f),
        make_float3(0.06f, 0.12f, 0.05f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x, 0.74f, z - 0.25f),
        make_float3(0.14f, 0.03f, 0.03f),
        scaled_color(trim_color, 1.08f),
        0
    );
    push_prop(
        make_float3(x, 0.34f, z + 0.25f),
        make_float3(0.18f, 0.10f, 0.02f),
        scaled_color(trim_color, 0.94f),
        0
    );
}

static void push_fire_hydrant(float x, float z) {
    const MDTBFloat4 body_color = make_float4(0.78f, 0.12f, 0.10f, 1.0f);
    const MDTBFloat4 cap_color = make_float4(0.90f, 0.76f, 0.24f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.28f, 0.06f, 0.06f, 1.0f);

    push_prop(make_float3(x, 0.07f, z), make_float3(0.18f, 0.07f, 0.18f), scaled_color(body_color, 0.74f), 0);
    push_prop(make_float3(x, 0.36f, z), make_float3(0.11f, 0.24f, 0.11f), body_color, 0);
    push_prop(make_float3(x, 0.66f, z), make_float3(0.09f, 0.10f, 0.09f), body_color, 0);
    push_prop(make_float3(x, 0.80f, z), make_float3(0.13f, 0.06f, 0.13f), cap_color, 0);
    push_prop(make_float3(x - 0.16f, 0.44f, z), make_float3(0.06f, 0.06f, 0.06f), cap_color, 0);
    push_prop(make_float3(x + 0.16f, 0.44f, z), make_float3(0.06f, 0.06f, 0.06f), cap_color, 0);
    push_prop(make_float3(x, 0.55f, z - 0.14f), make_float3(0.05f, 0.05f, 0.04f), trim_color, 0);
}

static void push_parking_meter(float x, float z, float facing_sign) {
    const MDTBFloat4 pole_color = make_float4(0.34f, 0.36f, 0.39f, 1.0f);
    const MDTBFloat4 head_color = make_float4(0.28f, 0.32f, 0.36f, 1.0f);
    const MDTBFloat4 screen_color = make_float4(0.67f, 0.78f, 0.82f, 1.0f);

    push_prop(make_float3(x, 0.06f, z), make_float3(0.10f, 0.06f, 0.10f), scaled_color(pole_color, 0.84f), 0);
    push_prop(make_float3(x, 0.62f, z), make_float3(0.03f, 0.62f, 0.03f), pole_color, 0);
    push_prop(make_float3(x, 1.12f, z + (facing_sign * 0.03f)), make_float3(0.08f, 0.18f, 0.06f), head_color, 0);
    push_prop(make_float3(x, 1.18f, z + (facing_sign * 0.08f)), make_float3(0.04f, 0.07f, 0.02f), screen_color, 0);
    push_prop(make_float3(x, 0.88f, z), make_float3(0.05f, 0.03f, 0.05f), scaled_color(head_color, 1.10f), 0);
}

static void push_signal_control_box(float x, float z, int align_on_x_axis) {
    const MDTBFloat4 cabinet_color = make_float4(0.56f, 0.58f, 0.56f, 1.0f);
    const MDTBFloat4 base_color = make_float4(0.46f, 0.46f, 0.44f, 1.0f);
    const MDTBFloat4 seam_color = make_float4(0.34f, 0.35f, 0.35f, 1.0f);
    const float half_x = align_on_x_axis ? 0.32f : 0.22f;
    const float half_z = align_on_x_axis ? 0.22f : 0.32f;

    push_prop(make_float3(x, 0.08f, z), make_float3(half_x + 0.06f, 0.08f, half_z + 0.06f), base_color, 0);
    push_prop(make_float3(x, 0.66f, z), make_float3(half_x, 0.58f, half_z), cabinet_color, 0);
    push_prop(make_float3(x, 1.28f, z), make_float3(half_x * 0.94f, 0.04f, half_z * 0.94f), scaled_color(cabinet_color, 1.06f), 0);

    if (align_on_x_axis) {
        push_prop(make_float3(x, 0.66f, z + 0.19f), make_float3(half_x * 0.82f, 0.48f, 0.02f), seam_color, 0);
    } else {
        push_prop(make_float3(x + 0.19f, 0.66f, z), make_float3(0.02f, 0.48f, half_z * 0.82f), seam_color, 0);
    }
}

static void push_signal_service_corner(
    MDTBFloat3 origin,
    float x_sign,
    float z_sign,
    float signal_x,
    float signal_z,
    int align_on_x_axis,
    MDTBFloat4 sidewalk_color
) {
    const MDTBFloat4 pad_color = scaled_color(sidewalk_color, 0.86f);
    const MDTBFloat4 seam_color = scaled_color(sidewalk_color, 0.68f);
    const MDTBFloat4 edge_color = scaled_color(sidewalk_color, 0.74f);
    const MDTBFloat4 scuff_color = scaled_color(sidewalk_color, 0.79f);
    const MDTBFloat4 cover_color = make_float4(0.42f, 0.44f, 0.46f, 1.0f);
    const float cabinet_x = origin.x + (x_sign * (signal_x + 1.95f));
    const float cabinet_z = origin.z + (z_sign * (signal_z - 1.75f));
    const float pole_x = origin.x + (x_sign * signal_x);
    const float pole_z = origin.z + (z_sign * signal_z);
    const float hydrant_x = origin.x + (x_sign * (signal_x - 0.75f));
    const float hydrant_z = origin.z + (z_sign * (signal_z + 2.35f));
    const float cabinet_half_x = align_on_x_axis ? 0.62f : 0.44f;
    const float cabinet_half_z = align_on_x_axis ? 0.44f : 0.62f;
    const float conduit_half_z = fmaxf(fabsf(cabinet_z - pole_z) * 0.5f, 0.22f);
    const float conduit_half_x = fmaxf(fabsf(cabinet_x - pole_x) * 0.5f, 0.22f);

    push_scene_box(make_box(
        make_float3(cabinet_x, kSidewalkHeight + 0.01f, cabinet_z),
        make_float3(cabinet_half_x, 0.01f, cabinet_half_z),
        pad_color
    ));
    push_scene_box(make_box(
        make_float3(cabinet_x, kSidewalkHeight + 0.015f, cabinet_z - (cabinet_half_z + 0.12f)),
        make_float3(cabinet_half_x * 0.90f, 0.01f, 0.05f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(cabinet_x, kSidewalkHeight + 0.015f, cabinet_z + (cabinet_half_z + 0.12f)),
        make_float3(cabinet_half_x * 0.90f, 0.01f, 0.05f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(cabinet_x - (cabinet_half_x + 0.12f), kSidewalkHeight + 0.015f, cabinet_z),
        make_float3(0.05f, 0.01f, cabinet_half_z * 0.90f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(cabinet_x + (cabinet_half_x + 0.12f), kSidewalkHeight + 0.015f, cabinet_z),
        make_float3(0.05f, 0.01f, cabinet_half_z * 0.90f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(cabinet_x, kSidewalkHeight + 0.02f, cabinet_z),
        align_on_x_axis ? make_float3(cabinet_half_x * 0.76f, 0.01f, 0.05f) : make_float3(0.05f, 0.01f, cabinet_half_z * 0.76f),
        seam_color
    ));

    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.01f, (pole_z + cabinet_z) * 0.5f),
        make_float3(0.10f, 0.01f, conduit_half_z + 0.14f),
        scuff_color
    ));
    push_scene_box(make_box(
        make_float3(pole_x, kSidewalkHeight + 0.01f, (pole_z + cabinet_z) * 0.5f),
        make_float3(0.05f, 0.01f, conduit_half_z),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3((pole_x + cabinet_x) * 0.5f, kSidewalkHeight + 0.01f, cabinet_z),
        make_float3(conduit_half_x + 0.18f, 0.01f, 0.10f),
        scuff_color
    ));
    push_scene_box(make_box(
        make_float3((pole_x + cabinet_x) * 0.5f, kSidewalkHeight + 0.01f, cabinet_z),
        make_float3(conduit_half_x, 0.01f, 0.05f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(pole_x + (x_sign * 0.72f), kSidewalkHeight + 0.02f, cabinet_z),
        make_float3(0.18f, 0.01f, 0.14f),
        cover_color
    ));

    push_scene_box(make_box(
        make_float3(hydrant_x, kSidewalkHeight + 0.01f, hydrant_z),
        make_float3(0.42f, 0.01f, 0.42f),
        edge_color
    ));
    push_scene_box(make_box(
        make_float3(hydrant_x, kSidewalkHeight + 0.01f, hydrant_z),
        make_float3(0.30f, 0.01f, 0.30f),
        scaled_color(pad_color, 1.04f)
    ));
}

static void push_utility_cabinet(float x, float z, int align_on_x_axis) {
    const MDTBFloat4 cabinet_color = make_float4(0.44f, 0.50f, 0.46f, 1.0f);
    const MDTBFloat4 base_color = make_float4(0.38f, 0.40f, 0.39f, 1.0f);
    const MDTBFloat4 vent_color = make_float4(0.28f, 0.31f, 0.30f, 1.0f);
    const float half_x = align_on_x_axis ? 0.42f : 0.28f;
    const float half_z = align_on_x_axis ? 0.28f : 0.42f;

    push_prop(make_float3(x, 0.08f, z), make_float3(half_x + 0.06f, 0.08f, half_z + 0.06f), base_color, 0);
    push_prop(make_float3(x, 0.82f, z), make_float3(half_x, 0.74f, half_z), cabinet_color, 0);
    push_prop(make_float3(x, 1.60f, z), make_float3(half_x * 0.94f, 0.04f, half_z * 0.94f), scaled_color(cabinet_color, 1.05f), 0);
    push_prop(make_float3(x, 0.86f, z), align_on_x_axis ? make_float3(half_x * 0.72f, 0.42f, 0.02f) : make_float3(0.02f, 0.42f, half_z * 0.72f), vent_color, 0);
    push_prop(make_float3(x, 0.46f, z), align_on_x_axis ? make_float3(half_x * 0.68f, 0.03f, 0.03f) : make_float3(0.03f, 0.03f, half_z * 0.68f), vent_color, 0);
}

static void push_bus_stop_sign(float x, float z, float facing_sign) {
    const MDTBFloat4 pole_color = make_float4(0.38f, 0.40f, 0.44f, 1.0f);
    const MDTBFloat4 sign_color = make_float4(0.88f, 0.92f, 0.96f, 1.0f);
    const MDTBFloat4 accent_color = make_float4(0.26f, 0.58f, 0.82f, 1.0f);

    push_prop(make_float3(x, 0.08f, z), make_float3(0.10f, 0.08f, 0.10f), scaled_color(pole_color, 0.82f), 0);
    push_prop(make_float3(x, 1.48f, z), make_float3(0.03f, 1.48f, 0.03f), pole_color, 0);
    push_prop(make_float3(x, 2.26f, z + (facing_sign * 0.02f)), make_float3(0.18f, 0.24f, 0.03f), sign_color, 0);
    push_prop(make_float3(x, 2.36f, z + (facing_sign * 0.06f)), make_float3(0.12f, 0.05f, 0.02f), accent_color, 0);
    push_prop(make_float3(x, 1.80f, z + (facing_sign * 0.02f)), make_float3(0.13f, 0.14f, 0.02f), scaled_color(sign_color, 0.96f), 0);
}

static void push_newsstand(float x, float z, int spans_on_x_axis) {
    const MDTBFloat4 body_color = make_float4(0.56f, 0.22f, 0.18f, 1.0f);
    const MDTBFloat4 canopy_color = make_float4(0.90f, 0.78f, 0.26f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.26f, 0.22f, 0.19f, 1.0f);
    const MDTBFloat4 glass_color = make_float4(0.68f, 0.78f, 0.82f, 1.0f);
    const MDTBFloat4 paper_color = make_float4(0.88f, 0.88f, 0.82f, 1.0f);

    if (spans_on_x_axis) {
        push_prop(make_float3(x, 0.12f, z), make_float3(0.78f, 0.12f, 0.42f), scaled_color(trim_color, 0.82f), 1);
        push_prop(make_float3(x, 0.78f, z), make_float3(0.70f, 0.68f, 0.34f), body_color, 1);
        push_prop(make_float3(x, 0.24f, z - 0.36f), make_float3(0.62f, 0.18f, 0.05f), scaled_color(body_color, 0.74f), 0);
        push_prop(make_float3(x, 1.44f, z + 0.05f), make_float3(0.82f, 0.07f, 0.46f), trim_color, 0);
        push_prop(make_float3(x, 1.60f, z + 0.08f), make_float3(0.92f, 0.09f, 0.52f), canopy_color, 0);
        push_prop(make_float3(x, 1.22f, z + 0.34f), make_float3(0.58f, 0.48f, 0.04f), glass_color, 0);
        push_prop(make_float3(x - 0.74f, 0.80f, z), make_float3(0.04f, 0.64f, 0.34f), trim_color, 0);
        push_prop(make_float3(x + 0.74f, 0.80f, z), make_float3(0.04f, 0.64f, 0.34f), trim_color, 0);
        push_prop(make_float3(x, 1.24f, z + 0.52f), make_float3(0.62f, 0.10f, 0.03f), scaled_color(canopy_color, 0.90f), 0);
        push_prop(make_float3(x - 0.34f, 0.58f, z + 0.28f), make_float3(0.14f, 0.10f, 0.06f), paper_color, 0);
        push_prop(make_float3(x + 0.02f, 0.68f, z + 0.28f), make_float3(0.16f, 0.16f, 0.06f), paper_color, 0);
        push_prop(make_float3(x + 0.36f, 0.54f, z + 0.28f), make_float3(0.12f, 0.08f, 0.06f), paper_color, 0);
    } else {
        push_prop(make_float3(x, 0.12f, z), make_float3(0.42f, 0.12f, 0.78f), scaled_color(trim_color, 0.82f), 1);
        push_prop(make_float3(x, 0.78f, z), make_float3(0.34f, 0.68f, 0.70f), body_color, 1);
        push_prop(make_float3(x - 0.36f, 0.24f, z), make_float3(0.05f, 0.18f, 0.62f), scaled_color(body_color, 0.74f), 0);
        push_prop(make_float3(x + 0.05f, 1.44f, z), make_float3(0.46f, 0.07f, 0.82f), trim_color, 0);
        push_prop(make_float3(x + 0.08f, 1.60f, z), make_float3(0.52f, 0.09f, 0.92f), canopy_color, 0);
        push_prop(make_float3(x + 0.34f, 1.22f, z), make_float3(0.04f, 0.48f, 0.58f), glass_color, 0);
        push_prop(make_float3(x, 0.80f, z - 0.74f), make_float3(0.34f, 0.64f, 0.04f), trim_color, 0);
        push_prop(make_float3(x, 0.80f, z + 0.74f), make_float3(0.34f, 0.64f, 0.04f), trim_color, 0);
        push_prop(make_float3(x + 0.52f, 1.24f, z), make_float3(0.03f, 0.10f, 0.62f), scaled_color(canopy_color, 0.90f), 0);
        push_prop(make_float3(x + 0.28f, 0.58f, z - 0.34f), make_float3(0.06f, 0.10f, 0.14f), paper_color, 0);
        push_prop(make_float3(x + 0.28f, 0.68f, z + 0.02f), make_float3(0.06f, 0.16f, 0.16f), paper_color, 0);
        push_prop(make_float3(x + 0.28f, 0.54f, z + 0.36f), make_float3(0.06f, 0.08f, 0.12f), paper_color, 0);
    }
}

static void push_store_awning_x(float x, float z, float half_x, float facing_sign) {
    const MDTBFloat4 post_color = make_float4(0.49f, 0.45f, 0.37f, 1.0f);
    const MDTBFloat4 canopy_color = make_float4(0.90f, 0.55f, 0.21f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.67f, 0.31f, 0.18f, 1.0f);
    const MDTBFloat4 glass_color = make_float4(0.66f, 0.78f, 0.84f, 1.0f);
    const MDTBFloat4 tile_color = make_float4(0.45f, 0.35f, 0.27f, 1.0f);
    const MDTBFloat4 mullion_color = make_float4(0.34f, 0.32f, 0.30f, 1.0f);
    const MDTBFloat4 recess_color = make_float4(0.24f, 0.21f, 0.19f, 1.0f);
    const MDTBFloat4 kickplate_color = scaled_color(trim_color, 0.72f);
    const float canopy_z = z + (facing_sign * 0.88f);
    const float front_z = z + (facing_sign * 1.74f);
    const float brace_z = z + (facing_sign * 1.18f);
    const float storefront_z = z + (facing_sign * 0.14f);
    const float entry_z = z - (facing_sign * 0.06f);
    const float display_half_x = fmaxf(half_x * 0.24f, 0.34f);
    const float display_offset_x = fmaxf(half_x * 0.44f, 0.54f);
    const float entry_half_x = fmaxf(half_x * 0.17f, 0.28f);

    push_prop(
        make_float3(x - (half_x * 0.82f), 0.08f, z + (facing_sign * 0.76f)),
        make_float3(0.12f, 0.08f, 0.12f),
        scaled_color(post_color, 0.82f),
        1
    );
    push_prop(
        make_float3(x + (half_x * 0.82f), 0.08f, z + (facing_sign * 0.76f)),
        make_float3(0.12f, 0.08f, 0.12f),
        scaled_color(post_color, 0.82f),
        1
    );
    push_prop(
        make_float3(x - (half_x * 0.82f), 1.18f, z + (facing_sign * 0.76f)),
        make_float3(0.06f, 1.18f, 0.06f),
        post_color,
        1
    );
    push_prop(
        make_float3(x + (half_x * 0.82f), 1.18f, z + (facing_sign * 0.76f)),
        make_float3(0.06f, 1.18f, 0.06f),
        post_color,
        1
    );
    push_prop(
        make_float3(x, 0.10f, z + (facing_sign * 0.08f)),
        make_float3(half_x * 0.96f, 0.08f, 0.24f),
        scaled_color(tile_color, 0.86f),
        0
    );
    push_prop(
        make_float3(x, 0.32f, storefront_z),
        make_float3(half_x * 0.90f, 0.18f, 0.07f),
        tile_color,
        0
    );
    push_prop(
        make_float3(x - display_offset_x, 0.98f, storefront_z),
        make_float3(display_half_x, 0.44f, 0.04f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x + display_offset_x, 0.98f, storefront_z),
        make_float3(display_half_x, 0.44f, 0.04f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x, 0.92f, entry_z),
        make_float3(entry_half_x, 0.70f, 0.12f),
        recess_color,
        0
    );
    push_prop(
        make_float3(x, 1.02f, storefront_z),
        make_float3(entry_half_x * 0.64f, 0.58f, 0.04f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x, 1.86f, z + (facing_sign * 0.08f)),
        make_float3(half_x * 0.82f, 0.12f, 0.05f),
        scaled_color(canopy_color, 0.82f),
        0
    );
    push_prop(
        make_float3(x - display_offset_x, 0.98f, storefront_z),
        make_float3(0.04f, 0.64f, 0.06f),
        mullion_color,
        0
    );
    push_prop(
        make_float3(x + display_offset_x, 0.98f, storefront_z),
        make_float3(0.04f, 0.64f, 0.06f),
        mullion_color,
        0
    );
    push_prop(
        make_float3(x - (entry_half_x + 0.08f), 0.98f, storefront_z),
        make_float3(0.03f, 0.64f, 0.06f),
        mullion_color,
        0
    );
    push_prop(
        make_float3(x + (entry_half_x + 0.08f), 0.98f, storefront_z),
        make_float3(0.03f, 0.64f, 0.06f),
        mullion_color,
        0
    );
    push_prop(
        make_float3(x, 2.38f, canopy_z),
        make_float3(half_x, 0.12f, 0.96f),
        canopy_color,
        0
    );
    push_prop(
        make_float3(x, 2.22f, z + (facing_sign * 0.52f)),
        make_float3(half_x * 0.92f, 0.05f, 0.76f),
        scaled_color(canopy_color, 0.76f),
        0
    );
    push_prop(
        make_float3(x, 2.14f, front_z),
        make_float3(half_x * 0.92f, 0.18f, 0.08f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x, 2.52f, z + (facing_sign * 0.04f)),
        make_float3(half_x * 1.04f, 0.08f, 0.10f),
        scaled_color(trim_color, 0.82f),
        0
    );
    push_prop(
        make_float3(x - (half_x * 0.90f), 2.22f, canopy_z),
        make_float3(0.04f, 0.18f, 0.76f),
        scaled_color(canopy_color, 0.90f),
        0
    );
    push_prop(
        make_float3(x + (half_x * 0.90f), 2.22f, canopy_z),
        make_float3(0.04f, 0.18f, 0.76f),
        scaled_color(canopy_color, 0.90f),
        0
    );
    push_prop(
        make_float3(x, 1.42f, z + (facing_sign * 0.06f)),
        make_float3(half_x * 0.86f, 0.44f, 0.06f),
        glass_color,
        0
    );
    push_prop(
        make_float3(x, 2.05f, z + (facing_sign * 0.10f)),
        make_float3(half_x, 0.08f, 0.18f),
        trim_color,
        0
    );
    push_prop(
        make_float3(x, 0.20f, storefront_z),
        make_float3(half_x * 0.82f, 0.04f, 0.02f),
        kickplate_color,
        0
    );
    push_prop(
        make_float3(x - (half_x * 0.32f), 1.42f, z + (facing_sign * 0.08f)),
        make_float3(0.04f, 0.38f, 0.08f),
        scaled_color(post_color, 0.90f),
        0
    );
    push_prop(
        make_float3(x + (half_x * 0.32f), 1.42f, z + (facing_sign * 0.08f)),
        make_float3(0.04f, 0.38f, 0.08f),
        scaled_color(post_color, 0.90f),
        0
    );
    push_prop(
        make_float3(x - (half_x * 0.58f), 2.18f, brace_z),
        make_float3(0.03f, 0.42f, 0.03f),
        scaled_color(post_color, 0.94f),
        0
    );
    push_prop(
        make_float3(x + (half_x * 0.58f), 2.18f, brace_z),
        make_float3(0.03f, 0.42f, 0.03f),
        scaled_color(post_color, 0.94f),
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

static void push_turn_pocket_surface_z(
    float x_origin,
    float z_origin,
    float direction_sign,
    float stop_bar_offset,
    float arrow_offset,
    float lane_offset,
    MDTBFloat4 road_color
) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.77f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);
    const float near_along = stop_bar_offset + 1.08f;
    const float far_along = arrow_offset - 2.22f;
    if (far_along <= near_along) {
        return;
    }

    const float pocket_half_x = fmaxf(lane_offset + 0.52f, 1.64f);
    const float center_along = (near_along + far_along) * 0.5f;
    const float half_along = (far_along - near_along) * 0.5f;

    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.005f, z_origin + (direction_sign * (near_along + half_along * 0.40f))),
        make_float3(pocket_half_x * 0.66f, 0.01f, fmaxf(half_along * 0.34f, 0.88f)),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.005f, z_origin + (direction_sign * center_along)),
        make_float3(pocket_half_x * 0.84f, 0.01f, fmaxf(half_along * 0.58f, 1.22f)),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.015f, z_origin + (direction_sign * center_along)),
        make_float3(0.06f, 0.01f, fmaxf(half_along * 0.70f, 1.10f)),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - (pocket_half_x * 0.72f), kRoadHeight + 0.015f, z_origin + (direction_sign * center_along)),
        make_float3(0.07f, 0.01f, fmaxf(half_along * 0.46f, 0.82f)),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + (pocket_half_x * 0.72f), kRoadHeight + 0.015f, z_origin + (direction_sign * center_along)),
        make_float3(0.07f, 0.01f, fmaxf(half_along * 0.46f, 0.82f)),
        seam_color
    ));
}

static void push_turn_pocket_surface_x(
    float x_origin,
    float z_origin,
    float direction_sign,
    float stop_bar_offset,
    float arrow_offset,
    float lane_offset,
    MDTBFloat4 road_color
) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.77f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);
    const float near_along = stop_bar_offset + 1.08f;
    const float far_along = arrow_offset - 2.22f;
    if (far_along <= near_along) {
        return;
    }

    const float pocket_half_z = fmaxf(lane_offset + 0.52f, 1.64f);
    const float center_along = (near_along + far_along) * 0.5f;
    const float half_along = (far_along - near_along) * 0.5f;

    push_scene_box(make_box(
        make_float3(x_origin + (direction_sign * (near_along + half_along * 0.40f)), kRoadHeight + 0.005f, z_origin),
        make_float3(fmaxf(half_along * 0.34f, 0.88f), 0.01f, pocket_half_z * 0.66f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + (direction_sign * center_along), kRoadHeight + 0.005f, z_origin),
        make_float3(fmaxf(half_along * 0.58f, 1.22f), 0.01f, pocket_half_z * 0.84f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + (direction_sign * center_along), kRoadHeight + 0.015f, z_origin),
        make_float3(fmaxf(half_along * 0.70f, 1.10f), 0.01f, 0.06f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + (direction_sign * center_along), kRoadHeight + 0.015f, z_origin - (pocket_half_z * 0.72f)),
        make_float3(fmaxf(half_along * 0.46f, 0.82f), 0.01f, 0.07f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + (direction_sign * center_along), kRoadHeight + 0.015f, z_origin + (pocket_half_z * 0.72f)),
        make_float3(fmaxf(half_along * 0.46f, 0.82f), 0.01f, 0.07f),
        seam_color
    ));
}

static void push_stop_bar_shoulder_z(float x_origin, float z_center, float half_width, MDTBFloat4 road_color) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.74f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.58f);

    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.005f, z_center),
        make_float3(half_width + 0.18f, 0.01f, 0.34f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.015f, z_center),
        make_float3(fmaxf((half_width * 0.82f), 3.85f), 0.01f, 0.05f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - fmaxf(half_width - 0.48f, 0.62f), kRoadHeight + 0.015f, z_center),
        make_float3(0.24f, 0.01f, 0.24f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + fmaxf(half_width - 0.48f, 0.62f), kRoadHeight + 0.015f, z_center),
        make_float3(0.24f, 0.01f, 0.24f),
        seam_color
    ));
}

static void push_stop_bar_shoulder_x(float x_center, float z_origin, float half_width, MDTBFloat4 road_color) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.74f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.58f);

    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.005f, z_origin),
        make_float3(0.34f, 0.01f, half_width + 0.18f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin),
        make_float3(0.05f, 0.01f, fmaxf((half_width * 0.82f), 3.85f)),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin - fmaxf(half_width - 0.48f, 0.62f)),
        make_float3(0.24f, 0.01f, 0.24f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin + fmaxf(half_width - 0.48f, 0.62f)),
        make_float3(0.24f, 0.01f, 0.24f),
        seam_color
    ));
}

static void push_lane_arrow_stand_off_z(float x_origin, float z_center, MDTBFloat4 road_color) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.76f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);

    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.005f, z_center),
        make_float3(1.02f, 0.01f, 1.94f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin, kRoadHeight + 0.015f, z_center),
        make_float3(0.07f, 0.01f, 1.40f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - 0.78f, kRoadHeight + 0.015f, z_center),
        make_float3(0.06f, 0.01f, 0.42f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 0.78f, kRoadHeight + 0.015f, z_center),
        make_float3(0.06f, 0.01f, 0.42f),
        seam_color
    ));
}

static void push_lane_arrow_stand_off_x(float x_center, float z_origin, MDTBFloat4 road_color) {
    const MDTBFloat4 apron_color = scaled_color(road_color, 0.76f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);

    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.005f, z_origin),
        make_float3(1.94f, 0.01f, 1.02f),
        apron_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin),
        make_float3(1.40f, 0.01f, 0.07f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin - 0.78f),
        make_float3(0.42f, 0.01f, 0.06f),
        seam_color
    ));
    push_scene_box(make_box(
        make_float3(x_center, kRoadHeight + 0.015f, z_origin + 0.78f),
        make_float3(0.42f, 0.01f, 0.06f),
        seam_color
    ));
}

static void push_lane_divider_throat_z(
    float x_origin,
    float z_origin,
    float direction_sign,
    float divider_offset,
    float stop_bar_offset,
    float arrow_offset,
    MDTBFloat4 road_color,
    MDTBFloat4 divider_color
) {
    const MDTBFloat4 throat_color = scaled_color(road_color, 0.78f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);
    const float near_along = stop_bar_offset + 0.92f;
    const float far_along = arrow_offset - 1.46f;
    if (far_along <= near_along) {
        return;
    }

    const float center_along = (near_along + far_along) * 0.5f;
    const float half_along = fmaxf((far_along - near_along) * 0.5f, 1.10f);

    for (int side = -1; side <= 1; side += 2) {
        const float x = x_origin + ((float)side * divider_offset);
        const float z = z_origin + (direction_sign * center_along);

        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.005f, z),
            make_float3(0.18f, 0.01f, half_along),
            throat_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.015f, z),
            make_float3(0.05f, 0.01f, fmaxf(half_along * 0.82f, 0.74f)),
            seam_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.015f, z_origin + (direction_sign * near_along)),
            make_float3(0.31f, 0.01f, 0.08f),
            divider_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.015f, z_origin + (direction_sign * far_along)),
            make_float3(0.31f, 0.01f, 0.08f),
            divider_color
        ));
    }
}

static void push_lane_divider_throat_x(
    float x_origin,
    float z_origin,
    float direction_sign,
    float divider_offset,
    float stop_bar_offset,
    float arrow_offset,
    MDTBFloat4 road_color,
    MDTBFloat4 divider_color
) {
    const MDTBFloat4 throat_color = scaled_color(road_color, 0.78f);
    const MDTBFloat4 seam_color = scaled_color(road_color, 0.60f);
    const float near_along = stop_bar_offset + 0.92f;
    const float far_along = arrow_offset - 1.46f;
    if (far_along <= near_along) {
        return;
    }

    const float center_along = (near_along + far_along) * 0.5f;
    const float half_along = fmaxf((far_along - near_along) * 0.5f, 1.10f);

    for (int side = -1; side <= 1; side += 2) {
        const float x = x_origin + (direction_sign * center_along);
        const float z = z_origin + ((float)side * divider_offset);

        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.005f, z),
            make_float3(half_along, 0.01f, 0.18f),
            throat_color
        ));
        push_scene_box(make_box(
            make_float3(x, kRoadHeight + 0.015f, z),
            make_float3(fmaxf(half_along * 0.82f, 0.74f), 0.01f, 0.05f),
            seam_color
        ));
        push_scene_box(make_box(
            make_float3(x_origin + (direction_sign * near_along), kRoadHeight + 0.015f, z),
            make_float3(0.08f, 0.01f, 0.31f),
            divider_color
        ));
        push_scene_box(make_box(
            make_float3(x_origin + (direction_sign * far_along), kRoadHeight + 0.015f, z),
            make_float3(0.08f, 0.01f, 0.31f),
            divider_color
        ));
    }
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
    const MDTBFloat4 glass_color = make_float4(0.69f, 0.79f, 0.84f, 1.0f);
    const MDTBFloat4 panel_color = make_float4(0.22f, 0.28f, 0.34f, 1.0f);
    const MDTBFloat4 ad_color = make_float4(0.88f, 0.74f, 0.26f, 1.0f);

    push_prop(make_float3(x - 0.92f, 1.22f, z - 0.98f), make_float3(0.06f, 1.22f, 0.06f), frame_color, 1);
    push_prop(make_float3(x + 0.92f, 1.22f, z - 0.98f), make_float3(0.06f, 1.22f, 0.06f), frame_color, 1);
    push_prop(make_float3(x - 0.92f, 1.22f, z + 0.98f), make_float3(0.06f, 1.22f, 0.06f), frame_color, 1);
    push_prop(make_float3(x + 0.92f, 1.22f, z + 0.98f), make_float3(0.06f, 1.22f, 0.06f), frame_color, 1);
    push_prop(make_float3(x, 2.46f, z), make_float3(1.18f, 0.10f, 1.36f), roof_color, 0);
    push_prop(make_float3(x, 2.58f, z - 0.12f), make_float3(1.00f, 0.04f, 1.12f), scaled_color(roof_color, 0.86f), 0);
    push_prop(make_float3(x, 2.30f, z + 1.16f), make_float3(1.06f, 0.08f, 0.12f), scaled_color(roof_color, 1.08f), 0);
    push_prop(make_float3(x - 0.78f, 1.18f, z), make_float3(0.05f, 1.02f, 1.08f), glass_color, 0);
    push_prop(make_float3(x + 0.78f, 1.18f, z - 0.42f), make_float3(0.05f, 0.94f, 0.58f), glass_color, 0);
    push_prop(make_float3(x, 1.26f, z - 1.08f), make_float3(0.78f, 0.84f, 0.05f), glass_color, 0);
    push_prop(make_float3(x + 0.74f, 1.18f, z + 0.52f), make_float3(0.09f, 0.92f, 0.38f), panel_color, 0);
    push_prop(make_float3(x + 0.74f, 1.18f, z + 0.52f), make_float3(0.05f, 0.78f, 0.28f), ad_color, 0);
    push_prop(make_float3(x, 0.22f, z + 1.10f), make_float3(0.98f, 0.06f, 0.10f), frame_color, 1);
    push_bench(x, z - 0.52f);
    push_trash_bin(x - 1.38f, z + 1.02f);
    push_bus_stop_sign(x - 1.52f, z + 0.94f, 1.0f);
    push_utility_cabinet(x + 1.34f, z - 1.08f, 0);
}

static void push_apartment_entry(float x, float z) {
    const MDTBFloat4 stucco_color = make_float4(0.62f, 0.58f, 0.46f, 1.0f);
    const MDTBFloat4 trim_color = make_float4(0.43f, 0.46f, 0.48f, 1.0f);
    const MDTBFloat4 canopy_color = make_float4(0.86f, 0.76f, 0.34f, 1.0f);
    const MDTBFloat4 glass_color = make_float4(0.66f, 0.78f, 0.84f, 1.0f);
    const MDTBFloat4 door_color = make_float4(0.25f, 0.20f, 0.16f, 1.0f);

    push_prop(make_float3(x, kSidewalkHeight + 0.08f, z), make_float3(1.56f, 0.08f, 2.92f), scaled_color(trim_color, 0.78f), 1);
    push_prop(make_float3(x, kSidewalkHeight + 0.18f, z), make_float3(1.32f, 0.06f, 1.34f), scaled_color(stucco_color, 0.88f), 1);
    push_prop(make_float3(x, kSidewalkHeight + 1.55f, z), make_float3(0.32f, 1.55f, 2.85f), stucco_color, 1);
    push_prop(make_float3(x - 1.05f, kSidewalkHeight + 1.24f, z), make_float3(0.16f, 1.24f, 2.42f), trim_color, 1);
    push_prop(make_float3(x + 1.05f, kSidewalkHeight + 1.24f, z), make_float3(0.16f, 1.24f, 2.42f), trim_color, 1);
    push_prop(make_float3(x - 0.60f, kSidewalkHeight + 0.96f, z), make_float3(0.26f, 0.96f, 0.16f), door_color, 0);
    push_prop(make_float3(x + 0.60f, kSidewalkHeight + 0.96f, z), make_float3(0.26f, 0.96f, 0.16f), door_color, 0);
    push_prop(make_float3(x - 0.60f, kSidewalkHeight + 1.82f, z), make_float3(0.22f, 0.16f, 0.08f), glass_color, 0);
    push_prop(make_float3(x + 0.60f, kSidewalkHeight + 1.82f, z), make_float3(0.22f, 0.16f, 0.08f), glass_color, 0);
    push_prop(make_float3(x - 1.05f, kSidewalkHeight + 0.38f, z), make_float3(0.20f, 0.18f, 2.18f), scaled_color(trim_color, 0.84f), 0);
    push_prop(make_float3(x + 1.05f, kSidewalkHeight + 0.38f, z), make_float3(0.20f, 0.18f, 2.18f), scaled_color(trim_color, 0.84f), 0);
    push_prop(make_float3(x, kSidewalkHeight + 3.05f, z), make_float3(1.54f, 0.14f, 2.85f), canopy_color, 0);
    push_prop(make_float3(x, kSidewalkHeight + 2.88f, z), make_float3(1.24f, 0.05f, 2.46f), scaled_color(canopy_color, 0.78f), 0);
    push_prop(make_float3(x, kSidewalkHeight + 3.22f, z), make_float3(1.34f, 0.05f, 0.12f), scaled_color(canopy_color, 1.08f), 0);
    push_prop(make_float3(x - 0.82f, kSidewalkHeight + 2.44f, z), make_float3(0.07f, 0.07f, 0.07f), scaled_color(canopy_color, 1.12f), 0);
    push_prop(make_float3(x + 0.82f, kSidewalkHeight + 2.44f, z), make_float3(0.07f, 0.07f, 0.07f), scaled_color(canopy_color, 1.12f), 0);
}

static void push_corner_store_landmark(float x, float z) {
    push_building(
        make_float3(x, kSidewalkHeight + 2.55f, z),
        make_float3(4.2f, 2.55f, 3.4f),
        make_float4(0.59f, 0.47f, 0.34f, 1.0f)
    );

    push_prop(
        make_float3(x, kSidewalkHeight + 0.80f, z + 3.18f),
        make_float3(3.48f, 0.70f, 0.12f),
        make_float4(0.66f, 0.78f, 0.84f, 1.0f),
        0
    );
    push_prop(
        make_float3(x - 3.34f, kSidewalkHeight + 0.80f, z + 0.62f),
        make_float3(0.12f, 0.70f, 2.22f),
        make_float4(0.66f, 0.78f, 0.84f, 1.0f),
        0
    );
    push_prop(
        make_float3(x + 1.10f, kSidewalkHeight + 1.46f, z + 3.62f),
        make_float3(2.24f, 0.12f, 0.42f),
        make_float4(0.90f, 0.48f, 0.19f, 1.0f),
        0
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
        make_float3(x + 3.82f, kSidewalkHeight + 2.54f, z + 1.24f),
        make_float3(0.12f, 0.88f, 0.52f),
        make_float4(0.92f, 0.82f, 0.34f, 1.0f),
        0
    );
    push_planter(x - 3.15f, z + 4.92f, 0.46f);
    push_planter(x + 2.84f, z + 4.86f, 0.42f);

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

static MDTBFloat4 road_surface_color_for_class(uint32_t road_class) {
    switch (road_class) {
        case MDTBRoadClassBoulevard:
            return make_float4(0.15f, 0.15f, 0.17f, 1.0f);
        case MDTBRoadClassAvenue:
            return make_float4(0.18f, 0.18f, 0.20f, 1.0f);
        case MDTBRoadClassConnector:
            return make_float4(0.22f, 0.21f, 0.20f, 1.0f);
        case MDTBRoadClassResidentialStreet:
        default:
            return make_float4(0.19f, 0.18f, 0.20f, 1.0f);
    }
}

static MDTBFloat4 curb_surface_color_for_class(uint32_t road_class) {
    switch (road_class) {
        case MDTBRoadClassBoulevard:
            return make_float4(0.76f, 0.75f, 0.72f, 1.0f);
        case MDTBRoadClassAvenue:
            return make_float4(0.72f, 0.72f, 0.71f, 1.0f);
        default:
            return make_float4(0.70f, 0.69f, 0.68f, 1.0f);
    }
}

static MDTBFloat4 sidewalk_surface_color_for_class(uint32_t road_class) {
    switch (road_class) {
        case MDTBRoadClassBoulevard:
            return make_float4(0.64f, 0.63f, 0.59f, 1.0f);
        case MDTBRoadClassAvenue:
            return make_float4(0.60f, 0.60f, 0.57f, 1.0f);
        default:
            return make_float4(0.58f, 0.57f, 0.55f, 1.0f);
    }
}

static void push_road_feature_segment(const MDTBRoadSpine *spine, float lateral_offset, float segment_center, float half_width, float half_length, float half_height, MDTBFloat4 color) {
    if (spine == NULL) {
        return;
    }

    if (spine->axis == MDTBRoadAxisNorthSouth) {
        push_scene_box(make_box(
            make_float3(spine->coordinate + lateral_offset, kRoadHeight + half_height, segment_center),
            make_float3(half_width, half_height, half_length),
            color
        ));
    } else {
        push_scene_box(make_box(
            make_float3(segment_center, kRoadHeight + half_height, spine->coordinate + lateral_offset),
            make_float3(half_length, half_height, half_width),
            color
        ));
    }
}

static void push_road_feature_run(const MDTBRoadSpine *spine, float lateral_offset, float half_width, float half_height, MDTBFloat4 color) {
    if (spine == NULL) {
        return;
    }

    if (spine->axis == MDTBRoadAxisNorthSouth) {
        push_scene_box(make_box(
            make_float3(spine->coordinate + lateral_offset, kRoadHeight + half_height, 0.0f),
            make_float3(half_width, half_height, kPlayableHalfLength + 4.0f),
            color
        ));
    } else {
        push_scene_box(make_box(
            make_float3(0.0f, kRoadHeight + half_height, spine->coordinate + lateral_offset),
            make_float3(kPlayableHalfWidth + 4.0f, half_height, half_width),
            color
        ));
    }
}

static void push_road_oriented_prop(
    const MDTBRoadSpine *spine,
    float lateral_offset,
    float segment_center,
    float y_center,
    float half_lateral,
    float half_height,
    float half_longitudinal,
    MDTBFloat4 color,
    int grounded
) {
    if (spine == NULL) {
        return;
    }

    if (spine->axis == MDTBRoadAxisNorthSouth) {
        push_prop(
            make_float3(spine->coordinate + lateral_offset, y_center, segment_center),
            make_float3(half_lateral, half_height, half_longitudinal),
            color,
            grounded
        );
    } else {
        push_prop(
            make_float3(segment_center, y_center, spine->coordinate + lateral_offset),
            make_float3(half_longitudinal, half_height, half_lateral),
            color,
            grounded
        );
    }
}

static void push_storm_drain_on_spine(const MDTBRoadSpine *spine, float lateral_offset, float segment_center, float curb_sign) {
    const MDTBFloat4 frame_color = make_float4(0.23f, 0.24f, 0.26f, 1.0f);
    const MDTBFloat4 grate_color = make_float4(0.36f, 0.38f, 0.40f, 1.0f);
    const MDTBFloat4 slot_color = make_float4(0.14f, 0.15f, 0.16f, 1.0f);

    push_road_oriented_prop(spine, lateral_offset, segment_center, 0.10f, 0.18f, 0.02f, 0.54f, frame_color, 0);
    push_road_oriented_prop(spine, lateral_offset + (curb_sign * 0.15f), segment_center, 0.13f, 0.05f, 0.05f, 0.50f, slot_color, 0);

    for (int slat = -1; slat <= 1; ++slat) {
        push_road_oriented_prop(
            spine,
            lateral_offset - (curb_sign * 0.03f),
            segment_center + ((float)slat * 0.22f),
            0.14f,
            0.10f,
            0.02f,
            0.03f,
            grate_color,
            0
        );
    }
}

static void push_access_cover_on_spine(const MDTBRoadSpine *spine, float lateral_offset, float segment_center, float half_lateral, float half_longitudinal) {
    const MDTBFloat4 patch_color = make_float4(0.27f, 0.28f, 0.30f, 1.0f);
    const MDTBFloat4 lid_color = make_float4(0.40f, 0.42f, 0.44f, 1.0f);
    const MDTBFloat4 seam_color = make_float4(0.18f, 0.19f, 0.20f, 1.0f);

    push_road_oriented_prop(
        spine,
        lateral_offset,
        segment_center,
        kRoadHeight + 0.015f,
        half_lateral + 0.10f,
        0.01f,
        half_longitudinal + 0.10f,
        patch_color,
        0
    );
    push_road_oriented_prop(
        spine,
        lateral_offset,
        segment_center,
        kRoadHeight + 0.02f,
        half_lateral,
        0.01f,
        half_longitudinal,
        lid_color,
        0
    );

    if (half_lateral >= half_longitudinal) {
        push_road_oriented_prop(
            spine,
            lateral_offset,
            segment_center,
            kRoadHeight + 0.03f,
            half_lateral * 0.72f,
            0.01f,
            0.02f,
            seam_color,
            0
        );
    } else {
        push_road_oriented_prop(
            spine,
            lateral_offset,
            segment_center,
            kRoadHeight + 0.03f,
            0.02f,
            0.01f,
            half_longitudinal * 0.72f,
            seam_color,
            0
        );
    }
}

static void build_road_corridor_details(void) {
    const MDTBFloat4 gutter_color = make_float4(0.21f, 0.22f, 0.24f, 1.0f);

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const int segment_count =
            spine->axis == MDTBRoadAxisNorthSouth
            ? (int)ceilf((kPlayableHalfLength + 8.0f) / 20.0f)
            : (int)ceilf((kPlayableHalfWidth + 8.0f) / 24.0f);
        const float segment_spacing = spine->axis == MDTBRoadAxisNorthSouth ? 20.0f : 24.0f;
        const float gutter_offset = fmaxf(profile->road_half_width - 0.26f, 4.8f);
        const float storm_drain_offset = profile->road_half_width + 0.12f;

        push_road_feature_run(spine, -gutter_offset, 0.18f, 0.01f, gutter_color);
        push_road_feature_run(spine, gutter_offset, 0.18f, 0.01f, gutter_color);

        for (int segment = -segment_count; segment <= segment_count; ++segment) {
            const float center = (float)segment * segment_spacing;

            if (road_spine_segment_hits_intersection(spine, center)) {
                continue;
            }

            if (((segment + (int)index) % 3) == 0) {
                const float offset_center = center + (((segment & 1) == 0) ? 4.6f : -4.6f);

                push_storm_drain_on_spine(spine, -storm_drain_offset, offset_center, -1.0f);
                push_storm_drain_on_spine(spine, storm_drain_offset, offset_center - 2.1f, 1.0f);
            }

            if (((segment + (int)index) % 4) == 0) {
                switch (spine->road_class) {
                    case MDTBRoadClassBoulevard:
                        push_access_cover_on_spine(spine, -profile->lane_offset, center - 3.0f, 0.34f, 0.26f);
                        push_access_cover_on_spine(spine, profile->lane_offset, center + 2.8f, 0.34f, 0.26f);
                        break;
                    case MDTBRoadClassAvenue:
                        push_access_cover_on_spine(spine, 0.0f, center + 1.8f, 0.30f, 0.24f);
                        break;
                    case MDTBRoadClassConnector:
                    case MDTBRoadClassResidentialStreet:
                    default:
                        break;
                }
            }
        }
    }
}

static void build_road_class_features(void) {
    const MDTBFloat4 boulevard_median_color = make_float4(0.63f, 0.58f, 0.24f, 1.0f);
    const MDTBFloat4 boulevard_divider_color = make_float4(0.90f, 0.88f, 0.74f, 1.0f);
    const MDTBFloat4 avenue_lane_color = make_float4(0.24f, 0.25f, 0.27f, 1.0f);
    const MDTBFloat4 avenue_boundary_color = make_float4(0.86f, 0.86f, 0.84f, 1.0f);

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const int segment_count =
            spine->axis == MDTBRoadAxisNorthSouth
            ? (int)ceilf((kPlayableHalfLength + 8.0f) / 20.0f)
            : (int)ceilf((kPlayableHalfWidth + 8.0f) / 24.0f);
        const float segment_spacing = spine->axis == MDTBRoadAxisNorthSouth ? 20.0f : 24.0f;
        const float avenue_boundary_offset = profile->lane_offset + 1.2f;
        const float avenue_band_offset = fminf(profile->road_half_width - 0.65f, avenue_boundary_offset + 0.9f);
        const float boulevard_divider_offset = profile->lane_offset + 0.10f;

        switch (spine->road_class) {
            case MDTBRoadClassBoulevard:
                for (int segment = -segment_count; segment <= segment_count; ++segment) {
                    const float center = (float)segment * segment_spacing;
                    if (road_spine_segment_hits_intersection(spine, center)) {
                        continue;
                    }

                    push_road_feature_segment(spine, 0.0f, center, profile->median_half_width, 6.4f, 0.01f, boulevard_median_color);
                    push_road_feature_segment(spine, -boulevard_divider_offset, center, 0.08f, 1.8f, 0.01f, boulevard_divider_color);
                    push_road_feature_segment(spine, boulevard_divider_offset, center, 0.08f, 1.8f, 0.01f, boulevard_divider_color);
                }
                break;
            case MDTBRoadClassAvenue:
                push_road_feature_run(spine, -avenue_band_offset, 0.62f, 0.01f, avenue_lane_color);
                push_road_feature_run(spine, avenue_band_offset, 0.62f, 0.01f, avenue_lane_color);
                for (int segment = -segment_count; segment <= segment_count; ++segment) {
                    const float center = (float)segment * segment_spacing;
                    if (road_spine_segment_hits_intersection(spine, center)) {
                        continue;
                    }

                    push_road_feature_segment(spine, -avenue_boundary_offset, center, 0.08f, 1.6f, 0.01f, avenue_boundary_color);
                    push_road_feature_segment(spine, avenue_boundary_offset, center, 0.08f, 1.6f, 0.01f, avenue_boundary_color);
                }
                break;
            case MDTBRoadClassConnector:
            case MDTBRoadClassResidentialStreet:
            default:
                break;
        }
    }
}

static void build_block_lot_surfaces(const MDTBBlockDescriptor *block) {
    const int has_retail = (block->tag_mask & MDTBBlockTagRetail) != 0u;
    const int has_residential = (block->tag_mask & MDTBBlockTagResidential) != 0u;
    const int has_spur = (block->tag_mask & MDTBBlockTagSpur) != 0u;
    const MDTBFloat4 lot_color = has_residential
        ? make_float4(0.50f, 0.48f, 0.41f, 1.0f)
        : make_float4(0.54f, 0.49f, 0.42f, 1.0f);
    const MDTBFloat4 asphalt_color = has_spur
        ? make_float4(0.28f, 0.28f, 0.29f, 1.0f)
        : (has_retail ? make_float4(0.31f, 0.31f, 0.33f, 1.0f) : make_float4(0.34f, 0.34f, 0.35f, 1.0f));
    const MDTBFloat4 curb_color = has_residential
        ? make_float4(0.68f, 0.66f, 0.60f, 1.0f)
        : make_float4(0.64f, 0.62f, 0.58f, 1.0f);
    const float planter_size = has_residential ? 0.56f : (has_spur ? 0.42f : 0.48f);
    const float parking_half_x = has_spur ? 9.8f : (has_retail ? 8.6f : 7.4f);
    const float parking_half_z = has_residential ? 12.8f : (has_spur ? 19.8f : 17.2f);
    const float parking_center_z = has_residential ? 0.0f : (has_spur ? -1.0f : 0.8f);

    push_scene_box(make_box(
        make_float3(block->origin.x - 35.0f, kSidewalkHeight * 0.5f, block->origin.z),
        make_float3(23.0f, kSidewalkHeight * 0.5f, 29.0f),
        scaled_color(lot_color, 0.98f)
    ));

    push_scene_box(make_box(
        make_float3(block->origin.x + 35.0f, kSidewalkHeight * 0.5f, block->origin.z),
        make_float3(23.0f, kSidewalkHeight * 0.5f, 29.0f),
        scaled_color(lot_color, 1.02f)
    ));

    push_lot_parking_pad(
        block->origin.x - 35.8f,
        block->origin.z + parking_center_z,
        parking_half_x,
        parking_half_z,
        -1.0f,
        planter_size,
        asphalt_color,
        curb_color
    );
    push_lot_parking_pad(
        block->origin.x + 35.8f,
        block->origin.z - parking_center_z,
        parking_half_x,
        parking_half_z,
        1.0f,
        planter_size,
        asphalt_color,
        curb_color
    );

    push_scene_box(make_box(
        make_float3(block->origin.x - 52.2f, kSidewalkHeight + 0.02f, block->origin.z),
        make_float3(0.14f, 0.01f, 26.4f),
        curb_color
    ));
    push_scene_box(make_box(
        make_float3(block->origin.x + 52.2f, kSidewalkHeight + 0.02f, block->origin.z),
        make_float3(0.14f, 0.01f, 26.4f),
        curb_color
    ));

    if (has_spur) {
        push_scene_box(make_box(
            make_float3(block->origin.x - 42.2f, kSidewalkHeight + 0.01f, block->origin.z - 23.6f),
            make_float3(7.2f, 0.01f, 3.6f),
            scaled_color(asphalt_color, 0.94f)
        ));
        push_scene_box(make_box(
            make_float3(block->origin.x + 42.2f, kSidewalkHeight + 0.01f, block->origin.z + 23.6f),
            make_float3(7.2f, 0.01f, 3.6f),
            scaled_color(asphalt_color, 0.94f)
        ));
    }

    push_utility_line_run_z(block->origin.x - 53.4f, block->origin.z - 24.0f, block->origin.z + 24.0f, 24.0f, 1.0f);
    push_utility_line_run_z(block->origin.x + 53.4f, block->origin.z - 24.0f, block->origin.z + 24.0f, 24.0f, -1.0f);
    push_utility_line_run_x(block->origin.x - 53.0f, block->origin.x - 18.0f, block->origin.z - 25.8f, 17.5f, 1.0f);
    push_utility_line_run_x(block->origin.x + 18.0f, block->origin.x + 53.0f, block->origin.z + 25.8f, 17.5f, -1.0f);
}

static void build_world_surfaces(void) {
    const MDTBFloat4 soil_color = make_float4(0.45f, 0.48f, 0.42f, 1.0f);

    push_scene_box(make_box(
        make_float3(0.0f, 0.0f, 0.0f),
        make_float3(kPlayableHalfWidth + 4.0f, 0.03f, kPlayableHalfLength + 4.0f),
        soil_color
    ));

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const MDTBFloat4 road_color = road_surface_color_for_class(spine->road_class);
        const MDTBFloat4 curb_color = curb_surface_color_for_class(spine->road_class);
        const MDTBFloat4 sidewalk_color = sidewalk_surface_color_for_class(spine->road_class);
        const float curb_half_width = fmaxf((profile->curb_outer - profile->road_half_width) * 0.5f, 0.05f);
        const float curb_center_offset = profile->road_half_width + curb_half_width;
        const float sidewalk_half_width = fmaxf((profile->sidewalk_outer - profile->curb_outer) * 0.5f, 0.8f);
        const float sidewalk_center_offset = profile->curb_outer + sidewalk_half_width;

        if (spine->axis != MDTBRoadAxisNorthSouth) {
            continue;
        }

        push_scene_box(make_box(
            make_float3(spine->coordinate, kRoadHeight * 0.5f, 0.0f),
            make_float3(profile->road_half_width, kRoadHeight * 0.5f, kPlayableHalfLength + 4.0f),
            road_color
        ));

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(spine->coordinate + ((float)side * curb_center_offset), 0.09f, 0.0f),
                make_float3(curb_half_width, 0.09f, kPlayableHalfLength + 4.0f),
                curb_color
            ));

            push_scene_box(make_box(
                make_float3(spine->coordinate + ((float)side * sidewalk_center_offset), kSidewalkHeight * 0.5f, 0.0f),
                make_float3(sidewalk_half_width, kSidewalkHeight * 0.5f, kPlayableHalfLength + 4.0f),
                sidewalk_color
            ));
        }
    }

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const MDTBFloat4 road_color = road_surface_color_for_class(spine->road_class);
        const MDTBFloat4 curb_color = curb_surface_color_for_class(spine->road_class);
        const MDTBFloat4 sidewalk_color = sidewalk_surface_color_for_class(spine->road_class);
        const float curb_half_width = fmaxf((profile->curb_outer - profile->road_half_width) * 0.5f, 0.05f);
        const float curb_center_offset = profile->road_half_width + curb_half_width;
        const float sidewalk_half_width = fmaxf((profile->sidewalk_outer - profile->curb_outer) * 0.5f, 0.8f);
        const float sidewalk_center_offset = profile->curb_outer + sidewalk_half_width;

        if (spine->axis != MDTBRoadAxisEastWest) {
            continue;
        }

        push_scene_box(make_box(
            make_float3(0.0f, kRoadHeight * 0.5f, spine->coordinate),
            make_float3(kPlayableHalfWidth + 4.0f, kRoadHeight * 0.5f, profile->road_half_width),
            road_color
        ));

        for (int side = -1; side <= 1; side += 2) {
            push_scene_box(make_box(
                make_float3(0.0f, 0.09f, spine->coordinate + ((float)side * curb_center_offset)),
                make_float3(kPlayableHalfWidth + 4.0f, 0.09f, curb_half_width),
                curb_color
            ));

            push_scene_box(make_box(
                make_float3(0.0f, kSidewalkHeight * 0.5f, spine->coordinate + ((float)side * sidewalk_center_offset)),
                make_float3(kPlayableHalfWidth + 4.0f, kSidewalkHeight * 0.5f, sidewalk_half_width),
                sidewalk_color
            ));
        }
    }

    build_road_class_features();
    build_road_corridor_details();
}

static void build_road_markings(void) {
    const MDTBFloat4 stripe_color = make_float4(0.89f, 0.82f, 0.20f, 1.0f);
    const MDTBFloat4 edge_line_color = make_float4(0.90f, 0.90f, 0.88f, 1.0f);
    const int vertical_segment_count = (int)ceilf((kPlayableHalfLength + 8.0f) / 20.0f);
    const int horizontal_segment_count = (int)ceilf((kPlayableHalfWidth + 8.0f) / 24.0f);

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const float avenue_boundary_offset = profile->lane_offset + 1.2f;

        if (spine->axis != MDTBRoadAxisNorthSouth) {
            continue;
        }

        for (int segment = -vertical_segment_count; segment <= vertical_segment_count; ++segment) {
            const float center = (float)segment * 20.0f;
            if (!road_spine_segment_hits_intersection(spine, center)) {
                if (spine->road_class == MDTBRoadClassBoulevard) {
                    for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                        push_scene_box(make_box(
                            make_float3(spine->coordinate - profile->median_half_width, kRoadHeight + 0.01f, center + ((float)stripe_index * 4.4f)),
                            make_float3(0.09f, 0.01f, 1.2f),
                            stripe_color
                        ));
                        push_scene_box(make_box(
                            make_float3(spine->coordinate + profile->median_half_width, kRoadHeight + 0.01f, center + ((float)stripe_index * 4.4f)),
                            make_float3(0.09f, 0.01f, 1.2f),
                            stripe_color
                        ));
                    }
                } else {
                    for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                        push_scene_box(make_box(
                            make_float3(spine->coordinate, kRoadHeight + 0.01f, center + ((float)stripe_index * 4.4f)),
                            make_float3(0.13f, 0.01f, 1.2f),
                            stripe_color
                        ));
                    }

                    if (spine->road_class == MDTBRoadClassAvenue) {
                        push_scene_box(make_box(
                            make_float3(spine->coordinate - avenue_boundary_offset, kRoadHeight + 0.01f, center),
                            make_float3(0.07f, 0.01f, 0.9f),
                            edge_line_color
                        ));
                        push_scene_box(make_box(
                            make_float3(spine->coordinate + avenue_boundary_offset, kRoadHeight + 0.01f, center),
                            make_float3(0.07f, 0.01f, 0.9f),
                            edge_line_color
                        ));
                    }
                }
            }
        }
    }

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const float avenue_boundary_offset = profile->lane_offset + 1.2f;

        if (spine->axis != MDTBRoadAxisEastWest) {
            continue;
        }

        for (int segment = -horizontal_segment_count; segment <= horizontal_segment_count; ++segment) {
            const float center_x = (float)segment * 24.0f;
            if (road_spine_segment_hits_intersection(spine, center_x)) {
                continue;
            }

            if (spine->road_class == MDTBRoadClassBoulevard) {
                for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                    push_scene_box(make_box(
                        make_float3(center_x + ((float)stripe_index * 5.0f), kRoadHeight + 0.01f, spine->coordinate - profile->median_half_width),
                        make_float3(1.4f, 0.01f, 0.09f),
                        stripe_color
                    ));
                    push_scene_box(make_box(
                        make_float3(center_x + ((float)stripe_index * 5.0f), kRoadHeight + 0.01f, spine->coordinate + profile->median_half_width),
                        make_float3(1.4f, 0.01f, 0.09f),
                        stripe_color
                    ));
                }
            } else {
                for (int stripe_index = -1; stripe_index <= 1; ++stripe_index) {
                    push_scene_box(make_box(
                        make_float3(center_x + ((float)stripe_index * 5.0f), kRoadHeight + 0.01f, spine->coordinate),
                        make_float3(1.4f, 0.01f, 0.13f),
                        stripe_color
                    ));
                }

                if (spine->road_class == MDTBRoadClassAvenue) {
                    push_scene_box(make_box(
                        make_float3(center_x, kRoadHeight + 0.01f, spine->coordinate - avenue_boundary_offset),
                        make_float3(0.9f, 0.01f, 0.07f),
                        edge_line_color
                    ));
                    push_scene_box(make_box(
                        make_float3(center_x, kRoadHeight + 0.01f, spine->coordinate + avenue_boundary_offset),
                        make_float3(0.9f, 0.01f, 0.07f),
                        edge_line_color
                    ));
                }
            }
        }
    }

    for (size_t block_index = 0u; block_index < scene_layout_count(); ++block_index) {
        const MDTBBlockDescriptor *block = &kBlockLayout[block_index];
        const MDTBIntersectionProfile profile = intersection_profile_for_block(block);
        const MDTBRoadProfile *vertical_road =
            profile.vertical != NULL
            ? road_profile_for_class(profile.vertical->road_class)
            : road_profile_for_class(MDTBRoadClassAvenue);
        const MDTBRoadProfile *horizontal_road =
            profile.horizontal != NULL
            ? road_profile_for_class(profile.horizontal->road_class)
            : road_profile_for_class(MDTBRoadClassAvenue);
        const float x_origin = block->origin.x;
        const float z_origin = block->origin.z;
        const float vertical_crosswalk_offset = profile.vertical != NULL ? profile.vertical->crosswalk_offset : kCrosswalkOffset;
        const float horizontal_crosswalk_offset = profile.horizontal != NULL ? profile.horizontal->crosswalk_offset : kCrosswalkOffset;
        const float northsouth_stop_bar_offset = profile.horizontal != NULL ? profile.horizontal->stop_bar_offset : 6.65f;
        const float eastwest_stop_bar_offset = profile.vertical != NULL ? profile.vertical->stop_bar_offset : 6.65f;
        const float northsouth_stop_bar_half_width = fmaxf(vertical_road->road_half_width - 0.9f, 4.35f);
        const float eastwest_stop_bar_half_width = fmaxf(horizontal_road->road_half_width - 0.9f, 4.35f);
        const float northsouth_arrow_offset = profile.vertical != NULL ? profile.vertical->arrow_offset : 18.5f;
        const float eastwest_arrow_offset = profile.horizontal != NULL ? profile.horizontal->arrow_offset : 18.5f;
        const MDTBFloat4 northsouth_road_color =
            road_surface_color_for_class(profile.vertical != NULL ? profile.vertical->road_class : MDTBRoadClassAvenue);
        const MDTBFloat4 eastwest_road_color =
            road_surface_color_for_class(profile.horizontal != NULL ? profile.horizontal->road_class : MDTBRoadClassAvenue);
        const MDTBFloat4 boulevard_divider_color = make_float4(0.90f, 0.88f, 0.74f, 1.0f);

        push_crosswalk_x(x_origin - vertical_crosswalk_offset, z_origin);
        push_crosswalk_x(x_origin + vertical_crosswalk_offset, z_origin);
        push_crosswalk_z(x_origin, z_origin - horizontal_crosswalk_offset);
        push_crosswalk_z(x_origin, z_origin + horizontal_crosswalk_offset);

        push_turn_pocket_surface_z(
            x_origin,
            z_origin,
            -1.0f,
            northsouth_stop_bar_offset,
            northsouth_arrow_offset,
            vertical_road->lane_offset,
            northsouth_road_color
        );
        push_turn_pocket_surface_z(
            x_origin,
            z_origin,
            1.0f,
            northsouth_stop_bar_offset,
            northsouth_arrow_offset,
            vertical_road->lane_offset,
            northsouth_road_color
        );
        push_turn_pocket_surface_x(
            x_origin,
            z_origin,
            -1.0f,
            eastwest_stop_bar_offset,
            eastwest_arrow_offset,
            horizontal_road->lane_offset,
            eastwest_road_color
        );
        push_turn_pocket_surface_x(
            x_origin,
            z_origin,
            1.0f,
            eastwest_stop_bar_offset,
            eastwest_arrow_offset,
            horizontal_road->lane_offset,
            eastwest_road_color
        );

        push_stop_bar_shoulder_z(x_origin, z_origin - northsouth_stop_bar_offset, northsouth_stop_bar_half_width, northsouth_road_color);
        push_stop_bar_shoulder_z(x_origin, z_origin + northsouth_stop_bar_offset, northsouth_stop_bar_half_width, northsouth_road_color);
        push_stop_bar_shoulder_x(x_origin - eastwest_stop_bar_offset, z_origin, eastwest_stop_bar_half_width, eastwest_road_color);
        push_stop_bar_shoulder_x(x_origin + eastwest_stop_bar_offset, z_origin, eastwest_stop_bar_half_width, eastwest_road_color);

        if (profile.vertical != NULL && profile.vertical->road_class == MDTBRoadClassBoulevard) {
            const float divider_offset = vertical_road->lane_offset + 0.10f;
            push_lane_divider_throat_z(
                x_origin,
                z_origin,
                -1.0f,
                divider_offset,
                northsouth_stop_bar_offset,
                northsouth_arrow_offset,
                northsouth_road_color,
                boulevard_divider_color
            );
            push_lane_divider_throat_z(
                x_origin,
                z_origin,
                1.0f,
                divider_offset,
                northsouth_stop_bar_offset,
                northsouth_arrow_offset,
                northsouth_road_color,
                boulevard_divider_color
            );
        }

        if (profile.horizontal != NULL && profile.horizontal->road_class == MDTBRoadClassBoulevard) {
            const float divider_offset = horizontal_road->lane_offset + 0.10f;
            push_lane_divider_throat_x(
                x_origin,
                z_origin,
                -1.0f,
                divider_offset,
                eastwest_stop_bar_offset,
                eastwest_arrow_offset,
                eastwest_road_color,
                boulevard_divider_color
            );
            push_lane_divider_throat_x(
                x_origin,
                z_origin,
                1.0f,
                divider_offset,
                eastwest_stop_bar_offset,
                eastwest_arrow_offset,
                eastwest_road_color,
                boulevard_divider_color
            );
        }

        push_scene_box(make_box(
            make_float3(x_origin, kRoadHeight + 0.01f, z_origin - northsouth_stop_bar_offset),
            make_float3(northsouth_stop_bar_half_width, 0.01f, 0.12f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin, kRoadHeight + 0.01f, z_origin + northsouth_stop_bar_offset),
            make_float3(northsouth_stop_bar_half_width, 0.01f, 0.12f),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin - eastwest_stop_bar_offset, kRoadHeight + 0.01f, z_origin),
            make_float3(0.12f, 0.01f, eastwest_stop_bar_half_width),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));
        push_scene_box(make_box(
            make_float3(x_origin + eastwest_stop_bar_offset, kRoadHeight + 0.01f, z_origin),
            make_float3(0.12f, 0.01f, eastwest_stop_bar_half_width),
            make_float4(0.92f, 0.91f, 0.87f, 1.0f)
        ));

        push_lane_arrow_stand_off_z(x_origin, z_origin - northsouth_arrow_offset, northsouth_road_color);
        push_lane_arrow_stand_off_z(x_origin, z_origin + northsouth_arrow_offset, northsouth_road_color);
        push_lane_arrow_stand_off_x(x_origin - eastwest_arrow_offset, z_origin, eastwest_road_color);
        push_lane_arrow_stand_off_x(x_origin + eastwest_arrow_offset, z_origin, eastwest_road_color);

        push_lane_arrow_z(x_origin, z_origin - northsouth_arrow_offset, -1.0f);
        push_lane_arrow_z(x_origin, z_origin + northsouth_arrow_offset, 1.0f);
        push_lane_arrow_x(x_origin - eastwest_arrow_offset, z_origin, -1.0f);
        push_lane_arrow_x(x_origin + eastwest_arrow_offset, z_origin, 1.0f);
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
    const MDTBIntersectionProfile profile = intersection_profile_for_block(block);
    const uint32_t dominant_road_class =
        (profile.vertical != NULL && profile.vertical->road_class == MDTBRoadClassBoulevard) ||
        (profile.horizontal != NULL && profile.horizontal->road_class == MDTBRoadClassBoulevard)
        ? MDTBRoadClassBoulevard
        : (profile.vertical != NULL
            ? profile.vertical->road_class
            : (profile.horizontal != NULL ? profile.horizontal->road_class : MDTBRoadClassAvenue));
    const float chunk_plaza_scale = profile.chunk != NULL ? profile.chunk->corner_plaza_scale : 1.0f;
    const float chunk_planter_scale = profile.chunk != NULL ? profile.chunk->planter_scale : 1.0f;
    const float chunk_refuge_scale = profile.chunk != NULL ? profile.chunk->refuge_scale : 1.0f;
    const float vertical_plaza_scale = profile.vertical != NULL ? profile.vertical->plaza_scale : 1.0f;
    const float horizontal_plaza_scale = profile.horizontal != NULL ? profile.horizontal->plaza_scale : 1.0f;
    const float signal_x = profile.vertical != NULL ? profile.vertical->signal_offset : 8.9f;
    const float signal_z = profile.horizontal != NULL ? profile.horizontal->signal_offset : 8.9f;
    const float planter_x = profile.vertical != NULL ? profile.vertical->planter_offset : 10.2f;
    const float planter_z = profile.horizontal != NULL ? profile.horizontal->planter_offset : 10.2f;
    const float corner_pad_half_x = 1.0f + (vertical_plaza_scale * 0.48f) + ((chunk_plaza_scale - 1.0f) * 0.55f);
    const float corner_pad_half_z = 1.0f + (horizontal_plaza_scale * 0.48f) + ((chunk_plaza_scale - 1.0f) * 0.55f);
    const float corner_pad_offset_x = signal_x - fminf(corner_pad_half_x * 0.35f, 0.70f);
    const float corner_pad_offset_z = signal_z - fminf(corner_pad_half_z * 0.35f, 0.70f);
    const float planter_size = 0.42f + (chunk_planter_scale * 0.12f);
    const float bollard_outer_x = signal_x + 3.0f + ((vertical_plaza_scale - 1.0f) * 0.4f);
    const float bollard_outer_z = signal_z + 3.0f + ((horizontal_plaza_scale - 1.0f) * 0.4f);
    const float bollard_inner_x = fmaxf(signal_x - 1.55f, 6.9f);
    const float bollard_inner_z = fmaxf(signal_z - 1.55f, 6.9f);
    const MDTBFloat4 plaza_color = sidewalk_surface_color_for_class(dominant_road_class);
    const MDTBFloat4 refuge_color = curb_surface_color_for_class(dominant_road_class);

    build_intersection_surface_details(block, &profile);
    build_intersection_mouth_transitions(block, &profile);

    for (int x_sign = -1; x_sign <= 1; x_sign += 2) {
        for (int z_sign = -1; z_sign <= 1; z_sign += 2) {
            push_corner_plaza_pad(
                block->origin,
                (float)x_sign * corner_pad_offset_x,
                (float)z_sign * corner_pad_offset_z,
                corner_pad_half_x,
                corner_pad_half_z,
                plaza_color
            );
            push_signal_pole(
                block->origin.x + ((float)x_sign * signal_x),
                block->origin.z + ((float)z_sign * signal_z)
            );
            push_signal_pole_corner_detail(
                block->origin,
                (float)x_sign,
                (float)z_sign,
                signal_x,
                signal_z,
                plaza_color
            );
            push_signal_control_box(
                block->origin.x + ((float)x_sign * (signal_x + 1.95f)),
                block->origin.z + ((float)z_sign * (signal_z - 1.75f)),
                x_sign != z_sign
            );
            push_signal_service_corner(
                block->origin,
                (float)x_sign,
                (float)z_sign,
                signal_x,
                signal_z,
                x_sign != z_sign,
                plaza_color
            );
            push_planter(
                block->origin.x + ((float)x_sign * planter_x),
                block->origin.z + ((float)z_sign * planter_z),
                planter_size
            );
            push_fire_hydrant(
                block->origin.x + ((float)x_sign * (signal_x - 0.75f)),
                block->origin.z + ((float)z_sign * (signal_z + 2.35f))
            );
        }
    }

    if (profile.vertical != NULL && profile.vertical->road_class == MDTBRoadClassBoulevard) {
        const MDTBRoadProfile *vertical_road = road_profile_for_class(profile.vertical->road_class);
        const float refuge_half_x = fmaxf(vertical_road->median_half_width + 0.22f, 0.65f);
        const float refuge_half_z = 1.08f + (chunk_refuge_scale * 0.28f);
        const float refuge_z = (profile.horizontal != NULL ? profile.horizontal->crosswalk_offset : kCrosswalkOffset) + 3.6f;
        const float refuge_planter_size = 0.28f + (chunk_planter_scale * 0.08f);

        push_refuge_island(block->origin.x, block->origin.z - refuge_z, refuge_half_x, refuge_half_z, refuge_planter_size, refuge_color);
        push_refuge_island(block->origin.x, block->origin.z + refuge_z, refuge_half_x, refuge_half_z, refuge_planter_size, refuge_color);
    }

    if (profile.horizontal != NULL && profile.horizontal->road_class == MDTBRoadClassBoulevard) {
        const MDTBRoadProfile *horizontal_road = road_profile_for_class(profile.horizontal->road_class);
        const float refuge_half_x = 1.08f + (chunk_refuge_scale * 0.28f);
        const float refuge_half_z = fmaxf(horizontal_road->median_half_width + 0.22f, 0.65f);
        const float refuge_x = (profile.vertical != NULL ? profile.vertical->crosswalk_offset : kCrosswalkOffset) + 3.6f;
        const float refuge_planter_size = 0.28f + (chunk_planter_scale * 0.08f);

        push_refuge_island(block->origin.x - refuge_x, block->origin.z, refuge_half_x, refuge_half_z, refuge_planter_size, refuge_color);
        push_refuge_island(block->origin.x + refuge_x, block->origin.z, refuge_half_x, refuge_half_z, refuge_planter_size, refuge_color);
    }

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

    push_bollard(block->origin.x - bollard_outer_x, block->origin.z - bollard_inner_z, 0.92f);
    push_bollard(block->origin.x - bollard_outer_x, block->origin.z + bollard_inner_z, 0.92f);
    push_bollard(block->origin.x + bollard_outer_x, block->origin.z - bollard_inner_z, 0.92f);
    push_bollard(block->origin.x + bollard_outer_x, block->origin.z + bollard_inner_z, 0.92f);
    push_bollard(block->origin.x - bollard_inner_x, block->origin.z - bollard_outer_z, 0.92f);
    push_bollard(block->origin.x + bollard_inner_x, block->origin.z - bollard_outer_z, 0.92f);
    push_bollard(block->origin.x - bollard_inner_x, block->origin.z + bollard_outer_z, 0.92f);
    push_bollard(block->origin.x + bollard_inner_x, block->origin.z + bollard_outer_z, 0.92f);
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
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;
    const MDTBFrontageProfile profile = frontage_profile_for_block(block);

    push_store_awning_x(x_origin - 42.0f, z_origin - profile.shopfront_z, 4.65f * profile.primary_awning_scale, 1.0f);
    push_newsstand(x_origin - 45.5f, z_origin - (profile.furniture_z + 0.15f), 1);
    push_trash_bin(x_origin - 38.6f, z_origin - profile.furniture_z);
    push_prop(make_float3(x_origin - 40.9f, 0.38f, z_origin - (profile.furniture_z + 0.10f)), make_float3(0.42f, 0.38f, 0.42f), make_float4(0.55f, 0.43f, 0.30f, 1.0f), 1);
    push_prop(make_float3(x_origin - 39.8f, 0.62f, z_origin - profile.furniture_z), make_float3(0.34f, 0.62f, 0.28f), make_float4(0.19f, 0.35f, 0.56f, 1.0f), 1);

    for (int bollard = 0; bollard < 4; ++bollard) {
        push_bollard(x_origin - 46.8f + ((float)bollard * 2.1f), z_origin - (profile.furniture_z - 0.60f), 0.86f);
    }

    push_store_awning_x(x_origin - 18.0f, z_origin - (profile.shopfront_z + 0.20f), 3.35f * profile.secondary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 18.0f, z_origin - (profile.shopfront_z - 0.20f), 3.75f * profile.secondary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 33.0f, z_origin - (profile.shopfront_z + 0.20f), 2.95f * profile.secondary_awning_scale, 1.0f);
    push_newsstand(x_origin + 12.6f, z_origin - (profile.furniture_z + 0.05f), 1);
    push_newsstand(x_origin + 28.9f, z_origin - (profile.furniture_z - 0.05f), 1);
    push_trash_bin(x_origin - 22.1f, z_origin - profile.furniture_z);
    push_trash_bin(x_origin + 36.5f, z_origin - (profile.furniture_z + 0.10f));
    push_bench(x_origin + 23.9f, z_origin - (profile.furniture_z - 0.40f));
    push_parking_meter(x_origin - 26.8f, z_origin - (profile.furniture_z - 0.44f), 1.0f);
    push_parking_meter(x_origin + 19.2f, z_origin - (profile.furniture_z - 0.46f), 1.0f);

    push_store_awning_x(x_origin - 18.0f, z_origin + (profile.shopfront_z + 0.20f), 3.35f * profile.secondary_awning_scale, -1.0f);
    push_store_awning_x(x_origin + 18.0f, z_origin + (profile.shopfront_z - 0.20f), 3.75f * profile.secondary_awning_scale, -1.0f);
    push_store_awning_x(x_origin + 33.0f, z_origin + (profile.shopfront_z + 0.20f), 2.95f * profile.secondary_awning_scale, -1.0f);
    push_newsstand(x_origin - 13.1f, z_origin + profile.furniture_z, 1);
    push_newsstand(x_origin + 23.1f, z_origin + profile.furniture_z, 1);
    push_trash_bin(x_origin - 22.3f, z_origin + (profile.furniture_z + 0.15f));
    push_trash_bin(x_origin + 36.4f, z_origin + (profile.furniture_z + 0.20f));
    push_bench(x_origin + 27.4f, z_origin + (profile.furniture_z - 0.30f));
    push_parking_meter(x_origin - 16.4f, z_origin + (profile.furniture_z - 0.46f), -1.0f);
    push_parking_meter(x_origin + 31.0f, z_origin + (profile.furniture_z - 0.44f), -1.0f);

    push_bench(x_origin - 10.7f, z_origin - (profile.rear_anchor_z - 2.30f));
    push_trash_bin(x_origin - 13.0f, z_origin - (profile.rear_anchor_z - 1.40f));
    push_utility_cabinet(x_origin - 15.4f, z_origin - (profile.rear_anchor_z - 1.15f), 0);
    push_prop(make_float3(x_origin - 11.2f, 2.05f, z_origin - profile.rear_anchor_z), make_float3(0.12f, 2.05f, 0.12f), make_float4(0.36f, 0.37f, 0.39f, 1.0f), 1);
    push_prop(make_float3(x_origin - 11.2f, 3.42f, z_origin - profile.rear_anchor_z), make_float3(1.08f, 0.12f, 0.12f), make_float4(0.83f, 0.75f, 0.49f, 1.0f), 0);

    push_bench(x_origin + 9.8f, z_origin + (profile.rear_anchor_z - 2.10f));
    push_trash_bin(x_origin + 12.2f, z_origin + (profile.rear_anchor_z - 1.40f));
    push_utility_cabinet(x_origin + 14.6f, z_origin + (profile.rear_anchor_z - 1.05f), 0);
    push_prop(make_float3(x_origin + 10.8f, 1.28f, z_origin + profile.rear_anchor_z), make_float3(0.12f, 1.28f, 1.05f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(x_origin + 10.1f, 2.34f, z_origin + profile.rear_anchor_z), make_float3(0.96f, 0.10f, 1.28f), make_float4(0.67f, 0.78f, 0.82f, 1.0f), 0);
}

static void build_mixed_use_frontage(const MDTBBlockDescriptor *block) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;
    const MDTBFrontageProfile profile = frontage_profile_for_block(block);

    push_store_awning_x(x_origin - 19.5f, z_origin - (profile.shopfront_z + 0.10f), 3.8f * profile.primary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 11.5f, z_origin - (profile.shopfront_z - 0.10f), 4.8f * profile.primary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 31.0f, z_origin + (profile.shopfront_z + 0.10f), 3.4f * profile.secondary_awning_scale, -1.0f);
    push_bus_shelter(x_origin - 21.8f, z_origin - profile.transit_stop_z);
    push_newsstand(x_origin - 6.2f, z_origin - profile.furniture_z, 1);
    push_newsstand(x_origin + 25.6f, z_origin + (profile.furniture_z + 0.10f), 1);
    push_trash_bin(x_origin - 25.2f, z_origin + (profile.furniture_z + 0.10f));
    push_trash_bin(x_origin + 8.6f, z_origin - (profile.furniture_z + 0.10f));
    push_bench(x_origin + 19.6f, z_origin - (profile.furniture_z - 0.40f));
    push_bench(x_origin - 11.8f, z_origin + (profile.furniture_z - 0.40f));
    push_parking_meter(x_origin - 9.4f, z_origin - (profile.furniture_z - 0.44f), 1.0f);
    push_parking_meter(x_origin + 22.2f, z_origin + (profile.furniture_z - 0.44f), -1.0f);
    push_planter(x_origin - 32.0f, z_origin - (profile.shopfront_z - 1.10f), profile.planter_size + 0.02f);
    push_planter(x_origin + 36.2f, z_origin + (profile.shopfront_z - 1.30f), profile.planter_size);
    push_utility_cabinet(x_origin - 24.8f, z_origin - (profile.transit_stop_z - 1.12f), 0);
    push_fire_hydrant(x_origin - 18.6f, z_origin - (profile.transit_stop_z - 1.34f));
    push_prop(make_float3(x_origin + 6.2f, kSidewalkHeight + 2.15f, z_origin - (profile.shopfront_z - 0.30f)), make_float3(2.2f, 0.16f, 0.20f), make_float4(0.92f, 0.63f, 0.22f, 1.0f), 0);
    push_prop(make_float3(x_origin + 33.8f, kSidewalkHeight + 2.2f, z_origin + (profile.shopfront_z - 0.10f)), make_float3(1.9f, 0.16f, 0.20f), make_float4(0.28f, 0.66f, 0.84f, 1.0f), 0);
    push_prop(make_float3(x_origin - 33.6f, 1.28f, z_origin + (profile.shopfront_z - 0.30f)), make_float3(0.12f, 1.28f, 1.08f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(x_origin - 34.3f, 2.34f, z_origin + (profile.shopfront_z - 0.30f)), make_float3(1.06f, 0.12f, 1.24f), make_float4(0.74f, 0.79f, 0.82f, 1.0f), 0);

    push_low_fence_run_x(z_origin - profile.rear_fence_z, x_origin + 25.0f, x_origin + 48.0f);
    push_low_fence_run_z(x_origin + 48.0f, z_origin - profile.rear_fence_z, z_origin - 27.8f);
    push_low_fence_run_x(z_origin + profile.rear_fence_z, x_origin - 49.0f, x_origin - 28.0f);
    push_low_fence_run_z(x_origin - 49.0f, z_origin + 28.4f, z_origin + profile.rear_fence_z);
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
    push_utility_cabinet(x_origin - 31.6f, z_origin - 23.8f, 1);
    push_fire_hydrant(x_origin - 20.8f, z_origin - 23.0f);

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
    const float x_origin = block->origin.x;
    const MDTBFrontageProfile profile = frontage_profile_for_block(block);
    build_block_lot_surfaces(block);
    build_side_buildings(block, -1);
    build_side_buildings(block, 1);
    build_intersection_props(block);
    build_frontage_for_block(block);
    build_hub_dynamic_props(block, block_index);

    push_corner_store_landmark(x_origin - 42.0f, block->origin.z - 17.2f);
    push_billboard(x_origin - 23.5f, block->origin.z - 45.5f, 5.4f, 1.45f, 0, make_float4(0.88f, 0.63f, 0.24f, 1.0f));
    push_half_court(x_origin + 35.0f, block->origin.z + 35.0f);
    push_billboard(x_origin + 46.0f, block->origin.z + 12.5f, 4.8f, 1.35f, 1, make_float4(0.29f, 0.53f, 0.67f, 1.0f));
    push_carport_landmark(x_origin + 39.5f, block->origin.z - 37.5f);

    push_low_fence_run_x(block->origin.z - (profile.rear_fence_z + 1.1f), x_origin + 28.0f, x_origin + 50.2f);
    push_low_fence_run_z(x_origin + 28.0f, block->origin.z - (profile.rear_fence_z + 1.1f), block->origin.z - 28.6f);
    push_low_fence_run_x(block->origin.z + (profile.rear_fence_z - 0.2f), x_origin - 50.0f, x_origin - 29.6f);
    push_low_fence_run_z(x_origin - 29.6f, block->origin.z + 28.8f, block->origin.z + (profile.rear_fence_z - 0.2f));

    push_interest_point(make_float3(x_origin, kSidewalkHeight, block->origin.z), 18.0f, MDTBInterestPointStreamingAnchor, block_index);
    push_interest_point(make_float3(x_origin - 10.0f, kSidewalkHeight, block->origin.z - 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(x_origin + 10.0f, kSidewalkHeight, block->origin.z + 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(x_origin - 14.4f, kSidewalkHeight, block->origin.z + 2.8f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    push_interest_point(make_float3(x_origin + 42.0f, kSidewalkHeight, block->origin.z - 17.2f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(x_origin + 35.0f, kSidewalkHeight, block->origin.z + 35.0f), 8.0f, MDTBInterestPointLandmark, block_index);
    build_hotspot_hooks(block, block_index);
    build_vehicle_handoff_hooks(block, block_index);
}

static void build_residential_frontage(const MDTBBlockDescriptor *block) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;
    const MDTBFrontageProfile profile = frontage_profile_for_block(block);

    push_store_awning_x(x_origin - 18.0f, z_origin - (profile.shopfront_z + 0.20f), 3.1f * profile.secondary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 19.0f, z_origin - (profile.shopfront_z - 0.30f), 2.8f * profile.secondary_awning_scale, 1.0f);
    push_store_awning_x(x_origin - 18.0f, z_origin + (profile.shopfront_z + 0.05f), 2.6f * profile.secondary_awning_scale, -1.0f);
    push_newsstand(x_origin - 12.6f, z_origin + profile.furniture_z, 1);
    push_newsstand(x_origin + 22.4f, z_origin - profile.furniture_z, 1);
    push_trash_bin(x_origin - 22.0f, z_origin - (profile.furniture_z + 0.20f));
    push_trash_bin(x_origin + 34.8f, z_origin + (profile.furniture_z + 0.10f));
    push_bench(x_origin + 24.0f, z_origin - (profile.furniture_z - 0.30f));
    push_bench(x_origin - 22.8f, z_origin + (profile.furniture_z - 0.40f));
    push_parking_meter(x_origin + 24.6f, z_origin - (profile.furniture_z - 0.44f), 1.0f);
    push_parking_meter(x_origin - 14.4f, z_origin + (profile.furniture_z - 0.42f), -1.0f);

    push_bus_shelter(x_origin + 11.2f, z_origin - profile.transit_stop_z);
    push_apartment_entry(x_origin - 41.6f, z_origin + (profile.transit_stop_z - 1.25f));
    push_fire_hydrant(x_origin + 14.0f, z_origin - (profile.transit_stop_z - 1.28f));
    push_utility_cabinet(x_origin - 46.2f, z_origin + (profile.transit_stop_z - 1.18f), 0);
    push_prop(make_float3(x_origin - 43.0f, kSidewalkHeight + 3.45f, z_origin + (profile.transit_stop_z - 1.25f)), make_float3(1.85f, 0.14f, 3.0f), make_float4(0.86f, 0.78f, 0.42f, 1.0f), 0);
    push_prop(make_float3(x_origin + 39.0f, kSidewalkHeight + 1.2f, z_origin + profile.rear_anchor_z), make_float3(0.14f, 1.2f, 1.3f), make_float4(0.29f, 0.31f, 0.33f, 1.0f), 1);
    push_prop(make_float3(x_origin + 38.4f, kSidewalkHeight + 2.1f, z_origin + profile.rear_anchor_z), make_float3(0.92f, 0.10f, 1.48f), make_float4(0.72f, 0.80f, 0.84f, 1.0f), 0);

    push_low_fence_run_x(z_origin + profile.rear_fence_z, x_origin - 48.0f, x_origin - 28.2f);
    push_low_fence_run_z(x_origin - 48.0f, z_origin + 28.5f, z_origin + profile.rear_fence_z);
    push_low_fence_run_x(z_origin + (profile.rear_fence_z - 0.20f), x_origin + 29.0f, x_origin + 48.5f);
    push_low_fence_run_z(x_origin + 48.5f, z_origin + 29.0f, z_origin + (profile.rear_fence_z - 0.20f));
}

static void build_service_spur_frontage(const MDTBBlockDescriptor *block) {
    const float x_origin = block->origin.x;
    const float z_origin = block->origin.z;
    const MDTBFloat4 loading_zone_color = make_float4(0.86f, 0.74f, 0.26f, 1.0f);
    const MDTBFloat4 loading_fill_color = make_float4(0.31f, 0.28f, 0.20f, 1.0f);
    const MDTBFrontageProfile profile = frontage_profile_for_block(block);

    push_store_awning_x(x_origin - 23.0f, z_origin - (profile.shopfront_z + 0.15f), 3.1f * profile.secondary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 6.5f, z_origin - (profile.shopfront_z - 0.25f), 4.3f * profile.primary_awning_scale, 1.0f);
    push_store_awning_x(x_origin + 29.8f, z_origin + (profile.shopfront_z + 0.05f), 2.8f * profile.secondary_awning_scale, -1.0f);
    push_bus_shelter(x_origin + 18.6f, z_origin + profile.transit_stop_z);
    push_newsstand(x_origin - 8.8f, z_origin - (profile.furniture_z + 0.10f), 1);
    push_newsstand(x_origin + 22.8f, z_origin + (profile.furniture_z + 0.10f), 1);
    push_trash_bin(x_origin - 26.4f, z_origin + profile.furniture_z);
    push_trash_bin(x_origin + 10.8f, z_origin - (profile.furniture_z + 0.20f));
    push_bench(x_origin - 14.6f, z_origin + (profile.furniture_z - 0.40f));
    push_bench(x_origin + 16.6f, z_origin - (profile.furniture_z - 0.30f));
    push_parking_meter(x_origin - 10.6f, z_origin - (profile.furniture_z - 0.46f), 1.0f);
    push_parking_meter(x_origin + 21.0f, z_origin + (profile.furniture_z - 0.44f), -1.0f);
    push_planter(x_origin - 31.4f, z_origin - (profile.shopfront_z - 1.15f), profile.planter_size + 0.08f);
    push_planter(x_origin + 35.0f, z_origin + (profile.shopfront_z - 1.25f), profile.planter_size - 0.02f);
    push_utility_cabinet(x_origin + 21.6f, z_origin + (profile.transit_stop_z - 1.12f), 0);
    push_fire_hydrant(x_origin + 16.2f, z_origin + (profile.transit_stop_z - 1.34f));
    push_prop(make_float3(x_origin + 2.8f, kSidewalkHeight + 2.18f, z_origin - (profile.shopfront_z - 0.25f)), make_float3(2.5f, 0.18f, 0.20f), make_float4(0.92f, 0.71f, 0.24f, 1.0f), 0);
    push_prop(make_float3(x_origin + 29.8f, kSidewalkHeight + 2.18f, z_origin + (profile.shopfront_z - 0.15f)), make_float3(1.68f, 0.18f, 0.20f), make_float4(0.33f, 0.68f, 0.82f, 1.0f), 0);
    push_prop(make_float3(x_origin - 28.2f, 1.18f, z_origin + (profile.shopfront_z - 0.45f)), make_float3(0.14f, 1.18f, 1.18f), make_float4(0.30f, 0.32f, 0.35f, 1.0f), 1);
    push_prop(make_float3(x_origin - 28.9f, 2.18f, z_origin + (profile.shopfront_z - 0.45f)), make_float3(1.22f, 0.12f, 1.36f), make_float4(0.76f, 0.81f, 0.84f, 1.0f), 0);

    push_scene_box(make_box(
        make_float3(x_origin - 21.6f, kRoadHeight + 0.01f, z_origin - profile.loading_zone_z),
        make_float3(8.8f * profile.loading_zone_half_x_scale, 0.01f, profile.loading_zone_half_z),
        loading_fill_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 15.2f, kRoadHeight + 0.01f, z_origin + profile.loading_zone_z),
        make_float3(10.4f * profile.loading_zone_half_x_scale, 0.01f, profile.loading_zone_half_z),
        loading_fill_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - 21.6f, kRoadHeight + 0.02f, z_origin - (profile.loading_zone_z + (profile.loading_zone_half_z + 0.06f))),
        make_float3(8.8f * profile.loading_zone_half_x_scale, 0.01f, 0.08f),
        loading_zone_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin - 21.6f, kRoadHeight + 0.02f, z_origin - (profile.loading_zone_z - (profile.loading_zone_half_z + 0.06f))),
        make_float3(8.8f * profile.loading_zone_half_x_scale, 0.01f, 0.08f),
        loading_zone_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 15.2f, kRoadHeight + 0.02f, z_origin + (profile.loading_zone_z + (profile.loading_zone_half_z + 0.06f))),
        make_float3(10.4f * profile.loading_zone_half_x_scale, 0.01f, 0.08f),
        loading_zone_color
    ));
    push_scene_box(make_box(
        make_float3(x_origin + 15.2f, kRoadHeight + 0.02f, z_origin + (profile.loading_zone_z - (profile.loading_zone_half_z + 0.06f))),
        make_float3(10.4f * profile.loading_zone_half_x_scale, 0.01f, 0.08f),
        loading_zone_color
    ));

    for (int stripe = 0; stripe < 4; ++stripe) {
        push_scene_box(make_box(
            make_float3(x_origin - 27.8f + ((float)stripe * 4.0f), kRoadHeight + 0.02f, z_origin - profile.loading_zone_z),
            make_float3(1.05f, 0.01f, 0.08f),
            loading_zone_color
        ));
        push_scene_box(make_box(
            make_float3(x_origin + 8.6f + ((float)stripe * 4.4f), kRoadHeight + 0.02f, z_origin + profile.loading_zone_z),
            make_float3(1.15f, 0.01f, 0.08f),
            loading_zone_color
        ));
    }

    push_low_fence_run_x(z_origin - (profile.rear_fence_z + 0.30f), x_origin + 26.8f, x_origin + 48.4f);
    push_low_fence_run_z(x_origin + 48.4f, z_origin - (profile.rear_fence_z + 0.30f), z_origin - 26.8f);
    push_low_fence_run_x(z_origin + (profile.rear_fence_z - 0.10f), x_origin - 49.2f, x_origin - 30.0f);
    push_low_fence_run_z(x_origin - 49.2f, z_origin + 29.0f, z_origin + (profile.rear_fence_z - 0.10f));
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
        make_float3(9.8f, 0.54f, 49.7f),
        make_float3(1.55f, 0.54f, 0.34f),
        cover_color,
        1
    );
    push_prop(
        make_float3(-15.0f, 1.02f, 53.4f),
        make_float3(0.12f, 1.02f, 1.84f),
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
    push_planter(-12.6f, 49.4f, 0.50f);

    push_bollard(-15.8f, 45.2f, 0.92f);
    push_bollard(-11.8f, 45.4f, 0.92f);
    push_bollard(-7.8f, 45.2f, 0.92f);
    push_bollard(-3.8f, 45.4f, 0.92f);
    push_bollard(0.2f, 45.2f, 0.92f);
    push_bollard(4.2f, 45.4f, 0.92f);
}

static void build_residential_block(const MDTBBlockDescriptor *block, uint32_t block_index) {
    const float x_origin = block->origin.x;
    build_block_lot_surfaces(block);
    build_side_buildings(block, -1);
    build_side_buildings(block, 1);
    build_intersection_props(block);
    build_frontage_for_block(block);
    build_residential_dynamic_props(block, block_index);

    push_billboard(x_origin - 24.0f, block->origin.z - 42.0f, 4.8f, 1.25f, 0, make_float4(0.36f, 0.63f, 0.29f, 1.0f));
    push_billboard(x_origin + 46.0f, block->origin.z + 18.5f, 4.6f, 1.25f, 1, make_float4(0.86f, 0.55f, 0.21f, 1.0f));
    push_parked_car(x_origin - 38.5f, block->origin.z - 35.2f, 1.15f, 2.2f, make_float4(0.22f, 0.48f, 0.63f, 1.0f));
    push_parked_car(x_origin + 35.8f, block->origin.z + 35.6f, 1.15f, 2.2f, make_float4(0.58f, 0.38f, 0.25f, 1.0f));

    push_interest_point(make_float3(x_origin, kSidewalkHeight, block->origin.z), 18.0f, MDTBInterestPointStreamingAnchor, block_index);
    push_interest_point(make_float3(x_origin - 10.0f, kSidewalkHeight, block->origin.z - 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(x_origin + 10.0f, kSidewalkHeight, block->origin.z + 10.0f), 5.0f, MDTBInterestPointPedestrianSpawn, block_index);
    push_interest_point(make_float3(x_origin + 11.2f, kSidewalkHeight, block->origin.z - 16.2f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(x_origin - 41.6f, kSidewalkHeight, block->origin.z + 15.0f), 8.0f, MDTBInterestPointLandmark, block_index);
    push_interest_point(make_float3(x_origin + 35.8f, kSidewalkHeight, block->origin.z + 35.6f), 5.5f, MDTBInterestPointVehicleSpawn, block_index);
    build_hotspot_hooks(block, block_index);
    build_vehicle_handoff_hooks(block, block_index);
}

static void build_scene(void) {
    g_scene_box_count = 0u;
    g_collision_box_count = 0u;
    g_block_count = 0u;
    g_road_link_count = 0u;
    g_road_spine_count = 0u;
    g_vehicle_anchor_count = 0u;
    g_interest_point_count = 0u;
    g_dynamic_prop_count = 0u;
    g_population_profile_count = 0u;
    g_traffic_occupancy_count = 0u;
    clear_scene_scope();
    rebuild_road_spines();

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
    const MDTBRoadProfile *nearest_profile = road_profile_for_class(MDTBRoadClassResidentialStreet);
    int on_road = 0;

    for (size_t index = 0u; index < g_road_spine_count; ++index) {
        const MDTBRoadSpine *spine = &g_road_spines[index];
        const MDTBRoadProfile *profile = road_profile_for_spine(spine);
        const float lateral_distance =
            spine->axis == MDTBRoadAxisNorthSouth
            ? fabsf(x - spine->coordinate)
            : fabsf(z - spine->coordinate);
        const float road_distance =
            fmaxf(lateral_distance - profile->road_half_width, 0.0f);

        if (road_distance < distance_to_road) {
            distance_to_road = road_distance;
            nearest_profile = profile;
        }

        if (lateral_distance <= profile->road_half_width) {
            on_road = 1;
        }
    }

    if (on_road) {
        info.height = kRoadHeight;
        info.surface_kind = MDTBSurfaceRoad;
        return info;
    }

    if (distance_to_road <= (nearest_profile->curb_outer - nearest_profile->road_half_width)) {
        const float ramp = clampf(
            distance_to_road / (nearest_profile->curb_outer - nearest_profile->road_half_width),
            0.0f,
            1.0f
        );
        info.height = kRoadHeight + ((kSidewalkHeight - kRoadHeight) * ramp);
        info.surface_kind = MDTBSurfaceCurb;
        return info;
    }

    if (distance_to_road <= (nearest_profile->sidewalk_outer - nearest_profile->road_half_width)) {
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
    const MDTBFloat3 last_known_position = state != NULL ? state->actor_position : make_float3(0.0f, 0.0f, 0.0f);

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
    state->combat_hostile_attack_windup = 0.0f;
    start_hostile_search(state, last_known_position, kLookoutSearchDuration);
    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, kLookoutVehicleReacquireDuration);
    trigger_street_incident(state, last_known_position, 0.74f, kStreetIncidentSearchDuration);
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
    state->combat_hostile_search_position = exit_position;
    state->combat_hostile_reacquire_timer = fmaxf(state->combat_hostile_reacquire_timer, kLookoutReacquireDuration);
    state->combat_hostile_reposition_timer = 0.0f;
    trigger_street_incident(state, exit_position, 0.52f, kStreetIncidentSettleDuration);
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
        const MDTBRoadProfile *profile = road_profile_for_link(link);
        const float lane_magnitude = fmaxf(profile->lane_offset, tuning->travel_lane_offset * 0.9f);
        const float desired_lane_offset = vehicle_lane_offset_for_heading(link->axis, state->active_vehicle_heading, lane_magnitude);
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
    state->territory_faction = MDTBTerritoryFactionNone;
    state->territory_phase = MDTBTerritoryPhaseNone;
    state->territory_presence = 0.0f;
    state->territory_heat = 0.0f;
    state->territory_reentry_timer = 0.0f;
    state->territory_entry_mode = MDTBTerritoryEntryNone;
    state->territory_watch_timer = 0.0f;
    state->territory_front_watch = 0.0f;
    state->territory_deep_watch = 0.0f;
    state->territory_patrol_position = kTerritoryPatrolBasePosition;
    state->territory_patrol_heading = atan2f(
        kTerritoryPatrolLinePosition.x - kTerritoryPatrolBasePosition.x,
        kTerritoryPatrolLinePosition.z - kTerritoryPatrolBasePosition.z
    );
    state->territory_patrol_state = MDTBTerritoryPatrolIdle;
    state->territory_patrol_alert = 0.0f;
    state->territory_inner_position = kTerritoryInnerBasePosition;
    state->territory_inner_heading = atan2f(
        kTerritoryPatrolBasePosition.x - kTerritoryInnerBasePosition.x,
        kTerritoryPatrolBasePosition.z - kTerritoryInnerBasePosition.z
    );
    state->territory_inner_state = MDTBTerritoryPatrolIdle;
    state->territory_inner_alert = 0.0f;
    state->territory_commit_state = MDTBTerritoryCommitNone;
    state->territory_commit_timer = 0.0f;
    state->territory_commit_progress = 0.0f;
    state->territory_resolve_state = MDTBTerritoryResolveNone;
    state->territory_resolve_timer = 0.0f;
    state->territory_resolve_progress = 0.0f;
    state->territory_reapproach_mode = MDTBTerritoryReapproachNone;
    state->territory_reapproach_timer = 0.0f;
    state->territory_preferred_side = 0.0f;
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
    state->combat_hostile_anchor_index = 3u;
    state->combat_hostile_position = lookout_anchor_position(state->combat_hostile_anchor_index);
    state->combat_hostile_heading = kPi;
    state->combat_hostile_in_range = 0u;
    state->combat_hostile_health = kLookoutMaxHealth;
    state->combat_hostile_reaction = 0.0f;
    state->combat_hostile_reset_timer = 0.0f;
    state->combat_hostile_alert = 0.0f;
    state->combat_hostile_reposition_timer = 0.0f;
    state->combat_hostile_reacquire_timer = 0.0f;
    state->combat_hostile_search_position = kCombatLaneResetPosition;
    state->combat_hostile_search_timer = 0.0f;
    state->combat_focus_target_kind = MDTBCombatTargetNone;
    state->combat_focus_distance = 0.0f;
    state->combat_focus_alignment = 0.0f;
    state->combat_last_hit_target_kind = MDTBCombatTargetNone;
    state->combat_focus_occluded = 0u;
    state->combat_player_in_cover = 0u;
    state->player_health = kPlayerMaxHealth;
    state->player_recovery_delay = 0.0f;
    state->player_damage_pulse = 0.0f;
    state->player_reset_timer = 0.0f;
    state->witness_position = kWitnessBasePosition;
    state->witness_heading = 0.18f;
    state->witness_state = MDTBWitnessStateIdle;
    state->witness_alert = 0.0f;
    state->witness_state_timer = 0.0f;
    state->bystander_position = kBystanderBasePosition;
    state->bystander_heading = -0.24f;
    state->bystander_state = MDTBWitnessStateIdle;
    state->bystander_alert = 0.0f;
    state->bystander_state_timer = 0.0f;
    state->street_incident_position = kCombatLaneResetPosition;
    state->street_incident_level = 0.0f;
    state->street_incident_timer = 0.0f;
    state->street_recovery_position = kCombatLaneResetPosition;
    state->street_recovery_level = 0.0f;
    state->street_recovery_timer = 0.0f;
    state->combat_hostile_attack_cooldown = 0.0f;
    state->combat_hostile_attack_windup = 0.0f;
    clear_hostile_last_shot(state);
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

    step_territory_state(state, dt);

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
