use serde::{Deserialize, Serialize};

// ============================================================================
// Input types — mirror the JS carsSpecState shape
// ============================================================================

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CarSpec {
    pub id: String,
    pub source_pool: String,   // "Full Random", "Stock", "DC", "Custom", "Pack:X", or a folder name
    pub source_rating: String, // "Random" or "0".."5"
    pub source_obtain: String, // "Random" or "-1".."4"
    pub attr_rating: String,   // "Random", "Unchanged", or "0".."5"
    pub attr_obtain: String,   // "Random", "Unchanged", or "-1".."4"
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CarsSpecState {
    pub stock_cars: Vec<CarSpec>,
    pub dc_cars: Vec<CarSpec>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TrackSpec {
    pub id: String,
    pub source_pool: String,       // "Full Random", "Stock", "Custom", "Pack:X", or folder
    pub source_difficulty: String, // "Random" or "1".."4"
    pub attr_difficulty: String,   // "Random", "Unchanged", or "1".."4"
    pub attr_obtain: String,       // "Random" or "-1".."5"
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TrackSpecState {
    #[serde(default = "default_true")]
    pub include_tracks: bool,
    #[serde(default)]
    pub tracks: Vec<TrackSpec>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct RatingDist {
    pub enabled: bool,
    pub min: usize,
    pub max: usize,
}

/// High-level options from the Car Options tab.
/// All fields are optional so older JSON payloads remain compatible.
#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CarOptionsInput {
    #[serde(default = "default_unlock_mode")]
    pub unlock_mode: String,
    #[serde(default)]
    pub num_starting_cars: usize,
    #[serde(default = "default_false")]
    pub enable_starting_cars_pool: bool,
    #[serde(default = "default_pool")]
    pub starting_cars_pool: String,
    #[serde(default = "default_false")]
    pub enable_starting_cars_rating: bool,
    #[serde(default = "default_random")]
    pub starting_cars_rating: String,
    #[serde(default)]
    pub include_cheat_only: bool,
    #[serde(default)]
    pub include_stunt_arena: bool,
    #[serde(default = "default_true")]
    pub include_super_pro: bool,
    #[serde(default)]
    pub pool_rating_distributions: std::collections::HashMap<String, RatingDist>,
    #[serde(default)]
    pub attr_rating_distributions: std::collections::HashMap<String, RatingDist>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TrackOptionsInput {
    #[serde(default = "default_track_unlock_mode")]
    pub unlock_mode: String, // random | randomUnlock | randomDifficulty | unchanged | baseGame
    #[serde(default = "default_true")]
    pub enable_random_obtain_methods: bool,
    #[serde(default)]
    pub include_cheat_only: bool,
    #[serde(default)]
    pub include_unlocked_by_default: bool,
    #[serde(default)]
    pub include_stunt_arena: bool,
}

pub fn default_unlock_mode() -> String { "random".to_string() }
pub fn default_track_unlock_mode() -> String { "random".to_string() }
pub fn default_pool()        -> String { "Full Random".to_string() }
pub fn default_random()      -> String { "Random".to_string() }
pub fn default_false()       -> bool   { false }
pub fn default_true()        -> bool   { true }

// ============================================================================
// Output types — match ConfigData / ConfigManager.cpp field names exactly
// ============================================================================

#[derive(Serialize, Debug, Clone)]
pub struct RandomizedCar {
    pub folder: String,
    pub rating: i32,
    pub obtain: i32,
    pub selectable_player: bool,
    pub selectable_cpu: bool,
}

#[derive(Serialize, Debug, Clone)]
pub struct RandomizedTrack {
    pub folder: String,
    pub difficulty: i32,
    pub obtain: i32,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct UiContext {
    pub generated_at: String,
    pub setup: UiInstallContext,
    pub configure: serde_json::Value,
    pub generated_car_folders: Vec<String>,
    pub generated_track_folders: Vec<String>,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct UiInstallContext {
    pub install_type: String,
    pub install_path: String,
    pub required_packs: Vec<String>,
}

#[derive(Serialize, Debug, Clone)]
pub struct ConfigMetadata {
    pub seed: String,
    pub version: String,
    #[serde(rename = "profileName", skip_serializing_if = "Option::is_none")]
    pub profile_name: Option<String>,
    #[serde(rename = "uiContext", skip_serializing_if = "Option::is_none")]
    pub ui_context: Option<UiContext>,
}

#[derive(Serialize, Debug, Clone)]
pub struct ConfigGlobalOptions {
    pub load_extra_cars: bool,
    pub load_extra_tracks: bool,
    pub load_extra_cups: bool,
}

#[derive(Serialize, Debug, Clone)]
pub struct ConfigData {
    pub metadata: ConfigMetadata,
    pub global_options: ConfigGlobalOptions,
    #[serde(rename = "stockCars")]
    pub stock_cars: Vec<RandomizedCar>,
    #[serde(rename = "dcCars")]
    pub dc_cars: Vec<RandomizedCar>,
    pub tracks: Vec<RandomizedTrack>,
    pub cups: Vec<RandomizedCup>,
}

// ============================================================================
// CUP SPEC — input types from the UI
// ============================================================================

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum CupStageMode {
    Default,
    Random,
    UserDefined,
}

impl Default for CupStageMode {
    fn default() -> Self { CupStageMode::Default }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum SameTrackHandling {
    Forbid,
    AllowAny,
    AllowVariants,
}

impl Default for SameTrackHandling {
    fn default() -> Self { SameTrackHandling::Forbid }
}

/// A single user-defined stage spec inside a cup.
#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct UserStageSpec {
    /// "Random", "slot:N" for slot index, or a specific track folder name
    pub source_pool: String,
    /// None = random between num_laps_min..num_laps_max; Some(n) = fixed
    pub num_laps: Option<u32>,
    /// Per-stage random laps min (used when num_laps is None; falls back to cup-level range)
    pub num_laps_min: Option<u32>,
    /// Per-stage random laps max (used when num_laps is None; falls back to cup-level range)
    pub num_laps_max: Option<u32>,
    /// None = random, Some(true/false) = forced
    pub is_reverse: Option<bool>,
    /// None = random, Some(true/false) = forced
    pub is_mirror: Option<bool>,
}

/// Per-cup configuration.
#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CupSpec {
    /// One of the 4 base cups: 0=Bronze, 1=Silver, 2=Gold, 3=Platinum
    pub index: usize,

    // ── per-field override flags ───────────────────────────────────────
    #[serde(default)] pub override_stage_mode: bool,
    #[serde(default)] pub override_num_cars: bool,
    #[serde(default)] pub override_cars_per_class: bool,
    #[serde(default)] pub override_num_tries: bool,
    #[serde(default)] pub override_per_race_place: bool,
    #[serde(default)] pub override_overall_place: bool,
    #[serde(default)] pub override_points_table: bool,

    // ── per-cup stage mode (only meaningful when override_stage_mode = true) ──
    #[serde(default)]
    pub stage_mode: CupStageMode,

    // ── per-cup values (used when the corresponding override flag is true) ──
    pub num_cars: Option<u32>,
    pub num_tries: Option<u32>,
    pub per_race_required_place: Option<u32>,
    pub overall_required_place: Option<u32>,
    pub points_table: Option<Vec<i32>>,
    /// cars_per_class[0..5] = Rookie..SuperPro; sum must == num_cars - 1
    pub cars_per_class: Option<Vec<u32>>,
    /// Per-cup laps range (used when override_stage_mode=true and effective mode is Random)
    pub num_laps_min: Option<u32>,
    pub num_laps_max: Option<u32>,

    // ── stage configuration ───────────────────────────────────────────
    /// Stage count range for Random mode (always per-cup)
    #[serde(default)]
    pub num_stages_min: u32,
    #[serde(default)]
    pub num_stages_max: u32,

    /// User-defined stages (used when effective stage_mode == UserDefined)
    #[serde(default)]
    pub stages: Vec<UserStageSpec>,
}

/// Top-level cup options input, mirroring the UI's cupSpecState.
#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CupSpecState {
    #[serde(default = "default_true")]
    pub enabled: bool,

    // ── global stage mode ─────────────────────────────────────────────
    #[serde(default)]
    pub stage_mode: CupStageMode,

    // ── global constraints ────────────────────────────────────────────
    #[serde(default = "default_true")]
    pub guarantee_first_normal: bool,

    #[serde(default)]
    pub same_track_handling: SameTrackHandling,

    #[serde(default = "default_true")]
    pub allow_reverse: bool,
    #[serde(default)]
    pub allow_mirror: bool,
    #[serde(default)]
    pub allow_reverse_mirror: bool,

    // ── global shared settings (per-cup can override) ─────────────────
    #[serde(default = "default_num_cars")]
    pub num_cars: u32,
    #[serde(default = "default_num_tries")]
    pub num_tries: u32,
    #[serde(default = "default_per_race_place")]
    pub per_race_required_place: u32,
    #[serde(default = "default_overall_place")]
    pub overall_required_place: u32,
    #[serde(default = "default_points_table")]
    pub points_table: Vec<i32>,

    // ── global laps range ─────────────────────────────────────────────
    #[serde(default = "default_laps_min")]
    pub num_laps_min: u32,
    #[serde(default = "default_laps_max")]
    pub num_laps_max: u32,

    // ── per-cup configuration ─────────────────────────────────────────
    pub cups: Vec<CupSpec>,
}

fn default_num_cars()        -> u32       { 8 }
fn default_num_tries()       -> u32       { 3 }
fn default_per_race_place()  -> u32       { 3 }
fn default_overall_place()   -> u32       { 1 }
fn default_laps_min()        -> u32       { 6 }
fn default_laps_max()        -> u32       { 6 }
pub fn default_points_table()    -> Vec<i32>  {
    vec![10, 6, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
}

// ============================================================================
// Cup generation output types (already in ConfigData.h schema)
// ============================================================================

#[derive(Serialize, Debug, Clone)]
pub struct RandomizedCupStage {
    #[serde(rename = "trackFolder")]
    pub track_folder: String,
    #[serde(rename = "numLaps")]
    pub num_laps: u32,
    #[serde(rename = "isReverse")]
    pub is_reverse: bool,
    #[serde(rename = "isMirror")]
    pub is_mirror: bool,
}

#[derive(Serialize, Debug, Clone)]
pub struct RandomizedCup {
    pub name: String,
    pub difficulty: i32,
    #[serde(rename = "obtainCondition")]
    pub obtain_condition: i32,
    #[serde(rename = "numCars")]
    pub num_cars: u32,
    #[serde(rename = "numTries")]
    pub num_tries: u32,
    #[serde(rename = "perRaceRequiredPlace")]
    pub per_race_required_place: u32,
    #[serde(rename = "overallRequiredPlace")]
    pub overall_required_place: u32,
    #[serde(rename = "carsPerClass")]
    pub cars_per_class: Vec<u32>,
    #[serde(rename = "pointsTable")]
    pub points_table: Vec<i32>,
    pub stages: Vec<RandomizedCupStage>,
}