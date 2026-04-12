use crate::scanner::{Car, Pool, ScanResult, Track, InstallType};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::fs;
use tauri::Manager; // Add this to your imports at the top if not present

// ============================================================================
// Input types — mirror the JS carsSpecState shape
// ============================================================================

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CarSpec {
    pub id: String,
    pub source_pool: String,   // "Full Random", "Stock", "DC", "Custom", "Pack:X", or a folder name
    pub source_rating: String, // "Random" or "0".."5"
    pub source_obtain: String, // "Random" or "-1".."4"
    pub attr_rating: String,   // "Random", "Unchanged", or "0".."5"
    pub attr_obtain: String,   // "Random", "Unchanged", or "-1".."4"
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CarsSpecState {
    pub stock_cars: Vec<CarSpec>,
    pub dc_cars: Vec<CarSpec>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TrackSpec {
    pub id: String,
    pub source_pool: String,       // "Full Random", "Stock", "Custom", "Pack:X", or folder
    pub source_difficulty: String, // "Random" or "1".."4"
    pub attr_difficulty: String,   // "Random", "Unchanged", or "1".."4"
    pub attr_obtain: String,       // "Random" or "-1".."5"
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TrackSpecState {
    #[serde(default = "default_true")]
    pub include_tracks: bool,
    #[serde(default)]
    pub tracks: Vec<TrackSpec>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct RatingDist {
    pub enabled: bool,
    pub min: usize,
    pub max: usize,
}

/// High-level options from the Car Options tab.
/// All fields are optional so older JSON payloads remain compatible.
#[derive(Deserialize, Debug, Clone)]
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

#[derive(Deserialize, Debug, Clone)]
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

fn default_unlock_mode() -> String { "random".to_string() }
fn default_track_unlock_mode() -> String { "random".to_string() }
fn default_pool()        -> String { "Full Random".to_string() }
fn default_random()      -> String { "Random".to_string() }
fn default_false()       -> bool   { false }
fn default_true()        -> bool   { true }

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
pub struct ConfigMetadata {
    pub seed: String,
    pub version: String,
    #[serde(rename = "profileName", skip_serializing_if = "Option::is_none")]
    pub profile_name: Option<String>,
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

#[derive(Deserialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum CupStageMode {
    Default,
    Random,
    UserDefined,
}

impl Default for CupStageMode {
    fn default() -> Self { CupStageMode::Default }
}

#[derive(Deserialize, Debug, Clone, PartialEq)]
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
#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct UserStageSpec {
    /// "Random", "1".."4" for difficulty pool, or a specific folder name
    pub source_pool: String,
    /// None = random, Some(n) = fixed
    pub num_laps: Option<u32>,
    /// None = random, Some(true/false) = forced
    pub is_reverse: Option<bool>,
    /// None = random, Some(true/false) = forced
    pub is_mirror: Option<bool>,
}

/// Per-cup configuration.
#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CupSpec {
    /// One of the 4 base cups: 0=Bronze, 1=Silver, 2=Gold, 3=Platinum
    pub index: usize,

    // ── override global shared settings ──────────────────────────────
    #[serde(default)]
    pub override_global: bool,
    pub num_cars: Option<u32>,
    pub num_tries: Option<u32>,
    pub per_race_required_place: Option<u32>,
    pub overall_required_place: Option<u32>,
    pub points_table: Option<Vec<i32>>,
    /// cars_per_class[0..5] = Rookie..SuperPro; sum must == num_cars - 1
    pub cars_per_class: Option<Vec<u32>>,
    /// Laps range override for this cup (overrides global when override_global=true)
    pub num_laps_min: Option<u32>,
    pub num_laps_max: Option<u32>,

    // ── stage configuration ───────────────────────────────────────────
    #[serde(default)]
    pub num_stages_min: u32,
    #[serde(default)]
    pub num_stages_max: u32,

    /// User-defined stages (only used when global stage_mode == UserDefined)
    #[serde(default)]
    pub stages: Vec<UserStageSpec>,
}

/// Top-level cup options input, mirroring the UI's cupSpecState.
#[derive(Deserialize, Debug, Clone)]
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
fn default_points_table()    -> Vec<i32>  {
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

// ============================================================================
// Cup generation helpers
// ============================================================================

/// Returns a track's has_reversed value by looking it up from the scan result.
/// Searches only packs that have use_tracks=true for launcher installs.
fn track_has_reversed(folder: &str, scan: &ScanResult) -> bool {
    match &scan.install_type {
        InstallType::Classic => {
            scan.tracks
                .as_deref()
                .unwrap_or(&[])
                .iter()
                .find(|t| t.folder_name.eq_ignore_ascii_case(folder))
                .map(|t| t.has_reversed)
                .unwrap_or(false)
        }
        InstallType::Launcher => {
            scan.content_packs
                .as_deref()
                .unwrap_or(&[])
                .iter()
                .filter(|p| p.use_tracks)
                .flat_map(|p| p.tracks.iter())
                .find(|t| t.folder_name.eq_ignore_ascii_case(folder))
                .map(|t| t.has_reversed)
                .unwrap_or(false)
        }
    }
}

/// Variant = (is_reverse, is_mirror)
type Variant = (bool, bool);

fn allowed_variants(folder: &str, scan: &ScanResult, opts: &CupSpecState) -> Vec<Variant> {
    let has_rev = track_has_reversed(folder, scan);
    let mut out = vec![(false, false)]; // Normal always available
    if opts.allow_mirror {
        out.push((false, true));
    }
    if has_rev && opts.allow_reverse {
        out.push((true, false));
    }
    if has_rev && opts.allow_reverse_mirror {
        out.push((true, true));
    }
    out
}

/// Roll a random number of laps within [min, max].
fn roll_laps(min: u32, max: u32, rng: &mut Rng) -> u32 {
    if min >= max { return min; }
    min + rng.next_usize((max - min + 1) as usize) as u32
}

/// Build a padding points table that is exactly 16 entries long.
fn pad_points(src: &[i32]) -> Vec<i32> {
    let mut v = src.to_vec();
    v.truncate(16);
    while v.len() < 16 { v.push(0); }
    v
}

/// Static defaults per cup index.
fn default_cars_per_class(cup_index: usize) -> Vec<u32> {
    match cup_index {
        0 => vec![7, 0, 0, 0, 0, 0],
        1 => vec![0, 4, 3, 0, 0, 0],
        2 => vec![0, 0, 4, 3, 0, 0],
        3 => vec![0, 0, 1, 3, 3, 0],
        _ => vec![7, 0, 0, 0, 0, 0],
    }
}

fn cup_name(cup_index: usize) -> &'static str {
    match cup_index {
        0 => "Bronze Cup",
        1 => "Silver Cup",
        2 => "Gold Cup",
        3 => "Platinum Cup",
        _ => "Unknown Cup",
    }
}

/// obtain_condition: Bronze = 0 (always unlocked), others = 1 (Championship)
fn cup_obtain(cup_index: usize) -> i32 {
    if cup_index == 0 { 0 } else { 1 }
}

// ============================================================================
// Default-stages builder
//
// Follows the base game layout:
//   Bronze  : tracks[0..3]  — Normal
//   Silver  : tracks[4..7]  — Normal
//   Gold    : tracks[8..10] Normal  +  tracks[4] (Silver[0]) Mirror
//   Platinum: tracks[11..13] Normal + tracks[5] (Silver[1]) RevMirror-if-has_rev else Mirror
//                                   + tracks[8] (Gold[0]) Mirror
// ============================================================================
fn build_default_stages(
    cup_index: usize,
    resolved: &[RandomizedTrack],
    scan: &ScanResult,
    laps_min: u32,
    laps_max: u32,
    rng: &mut Rng,
) -> Vec<RandomizedCupStage> {
    let make_stage = |folder: &str, is_reverse: bool, is_mirror: bool, rng: &mut Rng| {
        RandomizedCupStage {
            track_folder: folder.to_string(),
            num_laps: roll_laps(laps_min, laps_max, rng),
            is_reverse,
            is_mirror,
        }
    };

    match cup_index {
        // Bronze: tracks 0–3, all Normal
        0 => (0..4.min(resolved.len()))
            .map(|i| make_stage(&resolved[i].folder, false, false, rng))
            .collect(),

        // Silver: tracks 4–7, all Normal
        1 => (4..8.min(resolved.len()))
            .map(|i| make_stage(&resolved[i].folder, false, false, rng))
            .collect(),

        // Gold: tracks 8–10 Normal + Silver[0] (track 4) Mirror
        2 => {
            let mut stages: Vec<RandomizedCupStage> = (8..11.min(resolved.len()))
                .map(|i| make_stage(&resolved[i].folder, false, false, rng))
                .collect();
            if resolved.len() > 4 {
                stages.push(make_stage(&resolved[4].folder, false, true, rng));
            }
            stages
        }

        // Platinum: tracks 11–13 Normal
        //   + Silver[1] (track 5): RevMirror if has_rev, else Mirror
        //   + Gold[0]   (track 8): Mirror
        3 => {
            let mut stages: Vec<RandomizedCupStage> = (11..14.min(resolved.len()))
                .map(|i| make_stage(&resolved[i].folder, false, false, rng))
                .collect();

            // Silver[1] replacement
            if resolved.len() > 5 {
                let folder = &resolved[5].folder;
                let has_rev = track_has_reversed(folder, scan);
                if has_rev {
                    stages.push(make_stage(folder, true, true, rng));
                } else {
                    // Fallback: find any track from positions 0–10 with has_rev, prefer non-5/8
                    let fallback = (0..11.min(resolved.len()))
                        .filter(|&i| i != 5 && i != 8)
                        .find(|&i| track_has_reversed(&resolved[i].folder, scan));
                    if let Some(i) = fallback {
                        stages.push(make_stage(&resolved[i].folder, true, true, rng));
                    } else {
                        // Last resort: just play it in Mirror only
                        stages.push(make_stage(folder, false, true, rng));
                    }
                }
            }

            // Gold[0] replacement (track 8) Mirror
            if resolved.len() > 8 {
                stages.push(make_stage(&resolved[8].folder, false, true, rng));
            }

            stages
        }

        _ => vec![],
    }
}

// ============================================================================
// Random-stages builder
// ============================================================================

/// Tracks used so far within a single cup, along with which variants were used.
struct CupUsage {
    /// folder -> set of variants used
    variants: std::collections::HashMap<String, Vec<Variant>>,
}

impl CupUsage {
    fn new() -> Self { CupUsage { variants: std::collections::HashMap::new() } }

    fn has_used(&self, folder: &str) -> bool {
        self.variants.contains_key(folder)
    }

    fn used_variants(&self, folder: &str) -> &[Variant] {
        self.variants.get(folder).map(|v| v.as_slice()).unwrap_or(&[])
    }

    fn record(&mut self, folder: &str, variant: Variant) {
        self.variants.entry(folder.to_string()).or_default().push(variant);
    }
}

fn build_random_stages(
    resolved: &[RandomizedTrack],
    scan: &ScanResult,
    opts: &CupSpecState,
    cup_usage: &mut CupUsage, // cross-cup first-appearance tracking
    per_cup_usage: &mut CupUsage,
    num_stages: u32,
    laps_min: u32,
    laps_max: u32,
    rng: &mut Rng,
) -> Vec<RandomizedCupStage> {
    let mut stages = Vec::new();

    for _ in 0..num_stages {
        // 1. Determine candidate tracks respecting same-track handling
        let candidates: Vec<&RandomizedTrack> = resolved.iter().filter(|t| {
            match opts.same_track_handling {
                SameTrackHandling::Forbid => !per_cup_usage.has_used(&t.folder),
                SameTrackHandling::AllowAny => true,
                SameTrackHandling::AllowVariants => {
                    // Track is usable if there's at least one variant not yet used
                    let used = per_cup_usage.used_variants(&t.folder);
                    let available = allowed_variants(&t.folder, scan, opts);
                    available.iter().any(|v| !used.contains(v))
                }
            }
        }).collect();

        // Fallback: if no candidates (all tracks used/exhausted), allow any
        let candidates = if candidates.is_empty() {
            resolved.iter().collect::<Vec<_>>()
        } else {
            candidates
        };

        if candidates.is_empty() { break; }

        let track = candidates[rng.next_usize(candidates.len())];

        // 2. Determine variant (is_reverse, is_mirror)
        let variant = pick_variant(track, scan, opts, cup_usage, per_cup_usage, rng);

        per_cup_usage.record(&track.folder, variant);
        cup_usage.record(&track.folder, variant);

        stages.push(RandomizedCupStage {
            track_folder: track.folder.clone(),
            num_laps: roll_laps(laps_min, laps_max, rng),
            is_reverse: variant.0,
            is_mirror: variant.1,
        });
    }

    stages
}

/// Pick which variant to play for a track in a stage.
fn pick_variant(
    track: &RandomizedTrack,
    scan: &ScanResult,
    opts: &CupSpecState,
    cross_cup_usage: &CupUsage,
    per_cup_usage: &CupUsage,
    rng: &mut Rng,
) -> Variant {
    let all_variants = allowed_variants(&track.folder, scan, opts);

    // guarantee_first_normal: if this track has never appeared before, force Normal
    if opts.guarantee_first_normal && !cross_cup_usage.has_used(&track.folder) {
        return (false, false);
    }

    // In AllowVariants mode, exclude already-used variants within this cup
    let available: Vec<Variant> = if opts.same_track_handling == SameTrackHandling::AllowVariants {
        let used = per_cup_usage.used_variants(&track.folder);
        all_variants.into_iter().filter(|v| !used.contains(v)).collect()
    } else {
        all_variants
    };

    if available.is_empty() {
        return (false, false); // fallback
    }

    available[rng.next_usize(available.len())]
}

// ============================================================================
// User-defined stages builder
// ============================================================================

fn build_user_defined_stages(
    stage_specs: &[UserStageSpec],
    resolved: &[RandomizedTrack],
    scan: &ScanResult,
    opts: &CupSpecState,
    cross_cup_usage: &mut CupUsage,
    laps_min: u32,
    laps_max: u32,
    rng: &mut Rng,
) -> Vec<RandomizedCupStage> {
    let mut stages = Vec::new();

    for spec in stage_specs.iter().take(16) {
        // 1. Select track
        let track_folder = select_track_for_stage(spec, resolved, scan, rng);
        let folder = match &track_folder {
            Some(f) => f.clone(),
            None => continue,
        };

        // 2. Determine variant
        let variant = resolve_user_variant(spec, &folder, scan, opts, cross_cup_usage, rng);

        cross_cup_usage.record(&folder, variant);

        stages.push(RandomizedCupStage {
            track_folder: folder,
            num_laps: spec.num_laps.unwrap_or_else(|| roll_laps(laps_min, laps_max, rng)),
            is_reverse: variant.0,
            is_mirror: variant.1,
        });
    }

    stages
}

fn select_track_for_stage(
    spec: &UserStageSpec,
    resolved: &[RandomizedTrack],
    scan: &ScanResult,
    rng: &mut Rng,
) -> Option<String> {
    // Specific folder given
    if spec.source_pool != "Random" && !spec.source_pool.starts_with(|c: char| c.is_ascii_digit()) {
        // It's a specific folder name — check it's in the resolved list
        if resolved.iter().any(|t| t.folder.eq_ignore_ascii_case(&spec.source_pool)) {
            return Some(spec.source_pool.clone());
        }
        // Not in resolved list — fall through to random
    }

    // Difficulty pool filter ("1".."4")
    let candidates: Vec<&RandomizedTrack> = if let Ok(diff) = spec.source_pool.parse::<i32>() {
        resolved.iter().filter(|t| t.difficulty == diff).collect()
    } else {
        resolved.iter().collect()
    };

    // If reverse is forced true, prefer tracks with has_reversed=true
    let prefer_rev = spec.is_reverse == Some(true);
    if prefer_rev {
        let rev_candidates: Vec<&&RandomizedTrack> = candidates.iter()
            .filter(|t| track_has_reversed(&t.folder, scan))
            .collect();
        if !rev_candidates.is_empty() {
            let i = rng.next_usize(rev_candidates.len());
            return Some(rev_candidates[i].folder.clone());
        }
    }

    // Fallback: any candidate
    if candidates.is_empty() {
        // Relax pool constraint entirely
        if resolved.is_empty() { return None; }
        let i = rng.next_usize(resolved.len());
        return Some(resolved[i].folder.clone());
    }
    let i = rng.next_usize(candidates.len());
    Some(candidates[i].folder.clone())
}

fn resolve_user_variant(
    spec: &UserStageSpec,
    folder: &str,
    scan: &ScanResult,
    opts: &CupSpecState,
    _cross_cup_usage: &CupUsage,
    rng: &mut Rng,
) -> Variant {
    let has_rev = track_has_reversed(folder, scan);

    // If both are explicitly set by the user, bypass global restrictions
    if let (Some(rev), Some(mir)) = (spec.is_reverse, spec.is_mirror) {
        let actual_rev = if rev && !has_rev { false } else { rev };
        return (actual_rev, mir);
    }

    let all_allowed = allowed_variants(folder, scan, opts);

    // Reverse forced, mirror random
    if let Some(rev) = spec.is_reverse {
        let actual_rev = if rev && !has_rev { false } else { rev };
        let candidates: Vec<Variant> = all_allowed.into_iter().filter(|v| v.0 == actual_rev).collect();
        if candidates.is_empty() {
            return (actual_rev, false);
        }
        return candidates[rng.next_usize(candidates.len())];
    }

    // Mirror forced, reverse random
    if let Some(mir) = spec.is_mirror {
        let candidates: Vec<Variant> = all_allowed.into_iter().filter(|v| v.1 == mir).collect();
        if candidates.is_empty() {
            return (false, mir);
        }
        return candidates[rng.next_usize(candidates.len())];
    }

    // Both random (user-defined mode disables guarantee_first_normal)
    if all_allowed.is_empty() {
        return (false, false);
    }
    all_allowed[rng.next_usize(all_allowed.len())]
}

// ============================================================================
// Top-level cup generation — called from generate_result
// ============================================================================

fn generate_cups(
    cup_state: &CupSpecState,
    resolved_tracks: &[RandomizedTrack],
    scan: &ScanResult,
    rng: &mut Rng,
) -> Vec<RandomizedCup> {
    if !cup_state.enabled || resolved_tracks.is_empty() {
        return default_cups(cup_state, resolved_tracks, scan, rng);
    }

    // cross-cup first-appearance tracker (only used in Default/Random modes
    // with guarantee_first_normal; user-defined mode always disables it)
    let mut cross_cup_usage = CupUsage::new();

    let static_diffs = [1i32, 2, 3, 4];
    let static_obtain = [0i32, 1, 1, 1];
    let static_names = ["Bronze Cup", "Silver Cup", "Gold Cup", "Platinum Cup"];

    let mut cups = Vec::new();

    for cup_index in 0..4 {
        // ── resolve per-cup settings ──────────────────────────────────
        let cup_spec = cup_state.cups.iter().find(|c| c.index == cup_index);
        let override_global = cup_spec.map(|c| c.override_global).unwrap_or(false);

        let num_cars = if override_global { cup_spec.and_then(|c| c.num_cars).unwrap_or(cup_state.num_cars) }
                       else { cup_state.num_cars };
        let num_tries = if override_global { cup_spec.and_then(|c| c.num_tries).unwrap_or(cup_state.num_tries) }
                        else { cup_state.num_tries };
        let per_race = if override_global { cup_spec.and_then(|c| c.per_race_required_place).unwrap_or(cup_state.per_race_required_place) }
                       else { cup_state.per_race_required_place };
        let overall = if override_global { cup_spec.and_then(|c| c.overall_required_place).unwrap_or(cup_state.overall_required_place) }
                      else { cup_state.overall_required_place };
        let points = if override_global { cup_spec.and_then(|c| c.points_table.clone()).unwrap_or_else(|| cup_state.points_table.clone()) }
                     else { cup_state.points_table.clone() };
        let cars_per_class = cup_spec.and_then(|c| c.cars_per_class.clone())
            .unwrap_or_else(|| default_cars_per_class(cup_index));
        let laps_min = if override_global { cup_spec.and_then(|c| c.num_laps_min).unwrap_or(cup_state.num_laps_min) }
                       else { cup_state.num_laps_min };
        let laps_max = if override_global { cup_spec.and_then(|c| c.num_laps_max).unwrap_or(cup_state.num_laps_max) }
                       else { cup_state.num_laps_max };

        // ── build stages ──────────────────────────────────────────────
        let mut per_cup_usage = CupUsage::new();

        let stages = match cup_state.stage_mode {
            CupStageMode::Default => {
                let s = build_default_stages(cup_index, resolved_tracks, scan, laps_min, laps_max, rng);
                // Register default stages into cross_cup_usage so later cups see them
                for stage in &s {
                    let v = (stage.is_reverse, stage.is_mirror);
                    if cup_state.guarantee_first_normal {
                        cross_cup_usage.record(&stage.track_folder, v);
                    }
                }
                s
            }

            CupStageMode::Random => {
                let num_stages_min = cup_spec.map(|c| c.num_stages_min).unwrap_or(4).max(1);
                let num_stages_max = cup_spec.map(|c| c.num_stages_max).unwrap_or(4).max(num_stages_min);
                let num_stages = if num_stages_min == num_stages_max {
                    num_stages_min
                } else {
                    num_stages_min + rng.next_usize((num_stages_max - num_stages_min + 1) as usize) as u32
                };
                build_random_stages(
                    resolved_tracks, scan, cup_state,
                    &mut cross_cup_usage, &mut per_cup_usage,
                    num_stages, laps_min, laps_max, rng,
                )
            }

            CupStageMode::UserDefined => {
                let stage_specs = cup_spec.map(|c| c.stages.as_slice()).unwrap_or(&[]);
                // UserDefined mode: guarantee_first_normal is auto-disabled per spec
                build_user_defined_stages(
                    stage_specs, resolved_tracks, scan, cup_state,
                    &mut cross_cup_usage, laps_min, laps_max, rng,
                )
            }
        };

        cups.push(RandomizedCup {
            name: static_names[cup_index].to_string(),
            difficulty: static_diffs[cup_index],
            obtain_condition: static_obtain[cup_index],
            num_cars,
            num_tries,
            per_race_required_place: per_race,
            overall_required_place: overall,
            cars_per_class,
            points_table: pad_points(&points),
            stages,
        });
    }

    cups
}

/// Fallback when cups are disabled or no tracks exist — emit minimal valid cups
/// so the DLL still processes them correctly.
fn default_cups(
    cup_state: &CupSpecState,
    resolved_tracks: &[RandomizedTrack],
    scan: &ScanResult,
    rng: &mut Rng,
) -> Vec<RandomizedCup> {
    (0..4).map(|i| RandomizedCup {
        name: cup_name(i).to_string(),
        difficulty: (i as i32) + 1,
        obtain_condition: cup_obtain(i),
        num_cars: cup_state.num_cars,
        num_tries: cup_state.num_tries,
        per_race_required_place: cup_state.per_race_required_place,
        overall_required_place: cup_state.overall_required_place,
        cars_per_class: default_cars_per_class(i),
        points_table: pad_points(&cup_state.points_table),
        stages: build_default_stages(i, resolved_tracks, scan,
                                     cup_state.num_laps_min, cup_state.num_laps_max, rng),
    }).collect()
}

fn make_default_cup_spec_rust(index: usize) -> CupSpec {
    CupSpec {
        index,
        override_global: false,
        num_cars: None,
        num_tries: None,
        per_race_required_place: None,
        overall_required_place: None,
        points_table: None,
        cars_per_class: None,
        num_laps_min: None,
        num_laps_max: None,
        num_stages_min: 4,
        num_stages_max: 4,
        stages: vec![],
    }
}

// ============================================================================
// Simple LCG-based RNG — no external crate needed
// Uses current system time as seed.
// ============================================================================
struct Rng {
    state: u64,
}

impl Rng {
    fn new() -> Self {
        // Seed from system time (nanoseconds)
        use std::time::{SystemTime, UNIX_EPOCH};
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.subsec_nanos() as u64 ^ d.as_secs().wrapping_mul(6364136223846793005))
            .unwrap_or(12345);
        Rng { state: nanos | 1 }
    }

    /// Returns a random usize in [0, n)
    fn next_usize(&mut self, n: usize) -> usize {
        // LCG constants from Knuth
        self.state = self.state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        ((self.state >> 33) as usize) % n
    }

    /// Returns a random i32 in [lo, hi] (inclusive)
    fn next_i32(&mut self, lo: i32, hi: i32) -> i32 {
        let range = (hi - lo + 1) as usize;
        lo + self.next_usize(range) as i32
    }

    /// Shuffles a vector in place
    fn shuffle<T>(&mut self, data: &mut [T]) {
        for i in (1..data.len()).rev() {
            let j = self.next_usize(i + 1);
            data.swap(i, j);
        }
    }
}

// ============================================================================
// Candidate set builder
// ============================================================================

/// Build a flat list of valid (non-system, valid-file) cars from the scan.
fn collect_available_cars(scan: &ScanResult) -> Vec<Car> {
    let mut out = Vec::new();
    match &scan.install_type {
        crate::scanner::InstallType::Classic => {
            if let Some(cars) = &scan.cars {
                for c in cars {
                    if !c.is_system_car && c.has_valid_file {
                        out.push(c.clone());
                    }
                }
            }
        }
        crate::scanner::InstallType::Launcher => {
            if let Some(packs) = &scan.content_packs {
                for pack in packs {
                    if pack.use_cars {
                        for c in &pack.cars {
                            if !c.is_system_car && c.has_valid_file {
                                out.push(c.clone());
                            }
                        }
                    }
                }
            }
        }
    }
    out
}

fn is_stock_track_folder(folder: &str) -> bool {
    matches!(
        folder.to_ascii_lowercase().as_str(),
        "nhood1" | "market2" | "muse2" | "garden1" | "roof" | "toylite" | "wild_west1" |
        "toy2" | "nhood2" | "ship1" | "muse1" | "market1" | "wild_west2" | "ship2"
    )
}

fn collect_available_tracks(scan: &ScanResult) -> Vec<Track> {
    let mut out = Vec::new();
    match &scan.install_type {
        crate::scanner::InstallType::Classic => {
            if let Some(tracks) = &scan.tracks {
                for t in tracks {
                    if t.has_valid_file && t.track_type == 0 {
                        out.push(t.clone());
                    }
                }
            }
        }
        crate::scanner::InstallType::Launcher => {
            if let Some(packs) = &scan.content_packs {
                for pack in packs {
                    if pack.use_tracks {
                        for t in &pack.tracks {
                            if t.has_valid_file && t.track_type == 0 {
                                out.push(t.clone());
                            }
                        }
                    }
                }
            }
        }
    }
    out
}

fn track_candidate_set<'a>(spec: &TrackSpec, all_tracks: &'a [Track], scan: &ScanResult) -> Vec<&'a Track> {
    let pool_filtered: Vec<&Track> = if spec.source_pool == "Full Random" {
        all_tracks.iter().collect()
    } else if spec.source_pool == "Stock" {
        all_tracks.iter().filter(|t| is_stock_track_folder(&t.folder_name)).collect()
    } else if spec.source_pool == "Custom" {
        all_tracks.iter().filter(|t| !is_stock_track_folder(&t.folder_name)).collect()
    } else if spec.source_pool.starts_with("Pack:") {
        let pack_name = &spec.source_pool["Pack:".len()..];
        if let Some(packs) = &scan.content_packs {
            if let Some(pack) = packs.iter().find(|p| p.name == pack_name) {
                let folders: HashSet<&str> = pack.tracks.iter().map(|t| t.folder_name.as_str()).collect();
                all_tracks.iter().filter(|t| folders.contains(t.folder_name.as_str())).collect()
            } else {
                vec![]
            }
        } else {
            vec![]
        }
    } else {
        all_tracks
            .iter()
            .filter(|t| t.folder_name.eq_ignore_ascii_case(&spec.source_pool))
            .collect()
    };

    let is_specific =
        spec.source_pool != "Full Random" &&
        spec.source_pool != "Stock" &&
        spec.source_pool != "Custom" &&
        !spec.source_pool.starts_with("Pack:");

    if is_specific || spec.source_difficulty == "Random" {
        pool_filtered
    } else if let Ok(d) = spec.source_difficulty.parse::<i32>() {
        pool_filtered.into_iter().filter(|t| t.difficulty == d).collect()
    } else {
        pool_filtered
    }
}

fn resolve_track_list(
    specs: &[TrackSpec],
    all_tracks: &[Track],
    scan: &ScanResult,
    rng: &mut Rng,
) -> Vec<Option<Track>> {
    let mut indexed: Vec<(usize, usize)> = specs.iter().enumerate()
        .map(|(i, s)| (i, track_candidate_set(s, all_tracks, scan).len()))
        .collect();
    indexed.sort_by_key(|(_, count)| *count);

    let mut used: HashSet<String> = HashSet::new();
    let mut out = vec![None; specs.len()];

    for (idx, _) in indexed {
        let candidates = track_candidate_set(&specs[idx], all_tracks, scan);
        let unused: Vec<&&Track> = candidates.iter().filter(|t| !used.contains(&t.folder_name)).collect();
        let chosen = if !unused.is_empty() {
            let i = rng.next_usize(unused.len());
            Some((*unused[i]).clone())
        } else if !candidates.is_empty() {
            let i = rng.next_usize(candidates.len());
            Some((*candidates[i]).clone())
        } else if !all_tracks.is_empty() {
            let i = rng.next_usize(all_tracks.len());
            Some(all_tracks[i].clone())
        } else {
            None
        };
        if let Some(track) = &chosen {
            used.insert(track.folder_name.clone());
        }
        out[idx] = chosen;
    }
    out
}

fn resolve_track_difficulty(attr: &str, scanned: i32, slot_index: usize, mode: &str, rng: &mut Rng) -> i32 {
    if mode == "baseGame" {
        if slot_index < 4 { 1 } else if slot_index < 8 { 2 } else if slot_index < 11 { 3 } else { 4 }
    } else if mode == "randomUnlock" || mode == "unchanged" {
        scanned
    } else {
        match attr {
            "Unchanged" => scanned,
            "Random" => rng.next_i32(1, 4),
            other => other.parse::<i32>().unwrap_or(scanned),
        }
    }
}

fn resolve_track_obtain(attr: &str, mode: &str, opts: &TrackOptionsInput, rng: &mut Rng) -> i32 {
    if mode == "randomDifficulty" || mode == "unchanged" || mode == "baseGame" {
        1
    } else if attr == "Random" {
        if !opts.enable_random_obtain_methods {
            return 1;
        }
        let mut allowed = vec![1, 2, 3, 4];
        if opts.include_cheat_only { allowed.push(-1); }
        if opts.include_unlocked_by_default { allowed.push(0); }
        if opts.include_stunt_arena { allowed.push(5); }
        let i = rng.next_usize(allowed.len());
        allowed[i]
    } else {
        attr.parse::<i32>().unwrap_or(1)
    }
}

fn is_specific_car_pool(pool: &str) -> bool {
    !pool.is_empty()
        && pool != "Full Random"
        && pool != "Stock"
        && pool != "DC"
        && pool != "Custom"
        && !pool.starts_with("Pack:")
}

/// For a given CarSpec, return the subset of `all_cars` that satisfies the
/// source pool and filter constraints.
fn candidate_set<'a>(
    spec: &CarSpec,
    all_cars: &'a [Car],
    scan: &ScanResult,
) -> Vec<&'a Car> {
    // --- Step 1: Pool filter ---
    let pool_filtered: Vec<&Car> = if spec.source_pool == "Full Random" {
        all_cars.iter().collect()
    } else if spec.source_pool == "Stock" {
        all_cars.iter().filter(|c| c.pool == Pool::Stock).collect()
    } else if spec.source_pool == "DC" {
        all_cars.iter().filter(|c| c.pool == Pool::Dc).collect()
    } else if spec.source_pool == "Custom" {
        all_cars.iter().filter(|c| c.pool == Pool::Custom).collect()
    } else if spec.source_pool.starts_with("Pack:") {
        let pack_name = &spec.source_pool["Pack:".len()..];
        if let Some(packs) = &scan.content_packs {
            if let Some(pack) = packs.iter().find(|p| p.name == pack_name) {
                // Return refs into all_cars whose folder is in this pack
                let pack_folders: HashSet<&str> =
                    pack.cars.iter().map(|c| c.folder_name.as_str()).collect();
                all_cars
                    .iter()
                    .filter(|c| pack_folders.contains(c.folder_name.as_str()))
                    .collect()
            } else {
                vec![]
            }
        } else {
            vec![]
        }
    } else {
        // Specific car folder — exactly one candidate
        all_cars
            .iter()
            .filter(|c| c.folder_name.eq_ignore_ascii_case(&spec.source_pool))
            .collect()
    };

    // specific car — skip further filters
    let is_specific = is_specific_car_pool(&spec.source_pool);

    // --- Step 2: Rating filter (only when pool is not a specific car) ---
    let rating_filtered: Vec<&Car> = if is_specific {
        pool_filtered
    } else if spec.source_rating == "Random" {
        pool_filtered
    } else if let Ok(r) = spec.source_rating.parse::<i32>() {
        pool_filtered.into_iter().filter(|c| c.rating == r).collect()
    } else {
        pool_filtered
    };

    // --- Step 3: Obtain filter ---
    if is_specific {
        rating_filtered
    } else if spec.source_obtain == "Random" {
        rating_filtered
    } else if let Ok(o) = spec.source_obtain.parse::<i32>() {
        rating_filtered.into_iter().filter(|c| c.obtain_method == o).collect()
    } else {
        rating_filtered
    }
}

/// Build candidates using only source_pool (ignores rating/obtain filters).
fn candidate_set_pool_only<'a>(
    spec: &CarSpec,
    all_cars: &'a [Car],
    scan: &ScanResult,
) -> Vec<&'a Car> {
    let mut pool_only = spec.clone();
    pool_only.source_rating = "Random".to_string();
    pool_only.source_obtain = "Random".to_string();
    candidate_set(&pool_only, all_cars, scan)
}

// ============================================================================
// Core slot resolver (fewest-candidates-first)
// ============================================================================

struct SlotWork {
    category: &'static str, // "stock" | "dc"
    index: usize,
    spec: CarSpec, // We'll use a modified spec with resolved ratings
}

fn resolve_car_list(
    specs_stock: &[CarSpec],
    specs_dc: &[CarSpec],
    all_cars: &[Car],
    scan: &ScanResult,
    rng: &mut Rng,
) -> (Vec<Option<Car>>, Vec<Option<Car>>) {
    // Build work items with current candidate counts
    let mut work: Vec<(SlotWork, usize)> = Vec::new();

    for (i, spec) in specs_stock.iter().enumerate() {
        let count = candidate_set(spec, all_cars, scan).len();
        work.push((SlotWork { category: "stock", index: i, spec: spec.clone() }, count));
    }
    for (i, spec) in specs_dc.iter().enumerate() {
        let count = candidate_set(spec, all_cars, scan).len();
        work.push((SlotWork { category: "dc", index: i, spec: spec.clone() }, count));
    }

    // Sort: fewest candidates first, stable (keeps original order within same count)
    work.sort_by_key(|(_, cnt)| *cnt);

    // Resolve slots
    let mut used_folders: HashSet<String> = HashSet::new();
    let mut stock_results: Vec<Option<Car>> = vec![None; specs_stock.len()];
    let mut dc_results: Vec<Option<Car>> = vec![None; specs_dc.len()];

    for (slot, _) in &work {
        let candidates = candidate_set(&slot.spec, all_cars, scan);
        // Prefer unused cars
        let unused: Vec<&&Car> = candidates.iter().filter(|c| !used_folders.contains(&c.folder_name)).collect();

        let chosen: Option<Car> = if !unused.is_empty() {
            let idx = rng.next_usize(unused.len());
            Some((*unused[idx]).clone())
        } else if !candidates.is_empty() {
            // Fallback: allow duplicate
            let idx = rng.next_usize(candidates.len());
            Some((*candidates[idx]).clone())
        } else {
            // No candidates after full filter set:
            // 1) relax rating/obtain but keep source pool constraints.
            // 2) if still empty, pick from all cars as last resort.
            let relaxed = candidate_set_pool_only(&slot.spec, all_cars, scan);
            if !relaxed.is_empty() {
                let idx = rng.next_usize(relaxed.len());
                Some((*relaxed[idx]).clone())
            } else if !all_cars.is_empty() {
                let idx = rng.next_usize(all_cars.len());
                Some(all_cars[idx].clone())
            } else {
                None
            }
        };

        if let Some(car) = &chosen {
            used_folders.insert(car.folder_name.clone());
        }

        match slot.category {
            "stock" => stock_results[slot.index] = chosen,
            "dc" => dc_results[slot.index] = chosen,
            _ => {}
        }
    }

    (stock_results, dc_results)
}

// ============================================================================
// Attribute resolution
// ============================================================================

fn resolve_rating(attr: &str, scanned: i32, rng: &mut Rng, include_super_pro: bool) -> i32 {
    match attr {
        "Unchanged" => scanned,
        "Random" => {
            let max_r = if include_super_pro { 5 } else { 4 };
            rng.next_i32(0, max_r)
        }
        other => other.parse::<i32>().unwrap_or(scanned),
    }
}

/// Resolve the obtain (unlock) method for a slot.
/// When attr == "Random", the pool of allowed values is built from `car_options`.
fn resolve_obtain(attr: &str, scanned: i32, rng: &mut Rng, car_options: &CarOptionsInput) -> i32 {
    match attr {
        "Unchanged" => scanned,
        "Random" => {
            // Standard methods are always available
            let mut allowed: Vec<i32> = vec![0, 1, 2, 3, 4];
            if car_options.include_cheat_only {
                allowed.push(-1);
            }
            if car_options.include_stunt_arena {
                allowed.push(5);
            }
            let idx = rng.next_usize(allowed.len());
            allowed[idx]
        }
        other => other.parse::<i32>().unwrap_or(scanned),
    }
}

fn build_randomized_car(car: &Car, spec: &CarSpec, rng: &mut Rng, car_options: &CarOptionsInput) -> RandomizedCar {
    RandomizedCar {
        folder: car.folder_name.clone(),
        rating: resolve_rating(&spec.attr_rating, car.rating, rng, car_options.include_super_pro),
        obtain: resolve_obtain(&spec.attr_obtain, car.obtain_method, rng, car_options),
        selectable_player: true,
        selectable_cpu: true,
    }
}

// ============================================================================
// Global Distribution Allocator
// ============================================================================

fn allocate_ratings(
    count: usize,
    distributions: &std::collections::HashMap<String, RatingDist>,
    include_super_pro: bool,
    rng: &mut Rng,
) -> Vec<i32> {
    let mut result = Vec::with_capacity(count);
    if count == 0 { return result; }

    // Enabled rows are treated as the explicit allowed set.
    // If nothing is enabled, fall back to the default full range (0..4 or 0..5).
    let mut enabled_indices: Vec<usize> = (0..=5)
        .filter(|&i| distributions.get(&i.to_string()).map(|d| d.enabled).unwrap_or(false))
        .collect();
    if !include_super_pro {
        enabled_indices.retain(|&i| i != 5);
    }

    let has_explicit_enabled = !enabled_indices.is_empty();
    let allowed_indices: Vec<usize> = if has_explicit_enabled {
        enabled_indices
    } else {
        let mut base: Vec<usize> = (0..=5).collect();
        if !include_super_pro {
            base.retain(|&i| i != 5);
        }
        base
    };

    if allowed_indices.is_empty() {
        return result;
    }

    let mut remaining = count;
    let mut counts = vec![0; 6]; // 0..5

    // 1. Assign minimums
    for &i in &allowed_indices {
        if let Some(dist) = distributions.get(&i.to_string()) {
            let max_cap = dist.max;
            let m = dist.min.min(max_cap).min(remaining);
            counts[i] = m;
            remaining -= m;
        }
    }

    // 2. Distribute remaining
    if remaining > 0 {
        // Simple random distribution respecting max
        while remaining > 0 {
            // Filter to only those that can still accept more
            let candidates: Vec<usize> = allowed_indices
                .iter()
                .cloned()
                .filter(|&i| {
                    if has_explicit_enabled {
                        let dist = distributions.get(&i.to_string());
                        if let Some(d) = dist {
                            return counts[i] < d.max;
                        }
                        return false;
                    }
                    if let Some(d) = distributions.get(&i.to_string()) {
                        if d.enabled {
                            return counts[i] < d.max;
                        }
                    }
                    true
                })
                .collect();

            if candidates.is_empty() { break; } // Safety break

            let idx = candidates[rng.next_usize(candidates.len())];
            counts[idx] += 1;
            remaining -= 1;
        }
    }

    // 3. Build the flat list
    for (i, &c) in counts.iter().enumerate() {
        for _ in 0..c {
            result.push(i as i32);
        }
    }
    
    // 4. If we still have slots left (e.g. explicit maxes too low), keep choices
    // inside the already-allowed rating set.
    while result.len() < count {
        let ridx = rng.next_usize(allowed_indices.len());
        let r = allowed_indices[ridx] as i32;
        result.push(r);
    }

    rng.shuffle(&mut result);
    result
}

// ============================================================================
// Tauri command entry point
// ============================================================================

#[tauri::command]
pub fn generate_result(
    app_handle: tauri::AppHandle,
    scan_result: ScanResult,
    cars_spec_state: CarsSpecState,
    car_options: Option<CarOptionsInput>,
    track_spec_state: TrackSpecState,
    track_options: Option<TrackOptionsInput>,
    cup_spec_state: Option<CupSpecState>,
    file_name: String,
    profile_name: String,
) -> Result<String, String> {
    let mut rng = Rng::new();

    // Use provided options or fall back to safe defaults
    let opts = car_options.unwrap_or_else(|| CarOptionsInput {
        unlock_mode:          default_unlock_mode(),
        num_starting_cars:    0,
        enable_starting_cars_pool: default_false(),
        starting_cars_pool:   default_pool(),
        enable_starting_cars_rating: default_false(),
        starting_cars_rating: default_random(),
        include_cheat_only:   false,
        include_stunt_arena:  false,
        include_super_pro:    default_true(),
        pool_rating_distributions: std::collections::HashMap::new(),
        attr_rating_distributions: std::collections::HashMap::new(),
    });
    let track_opts = track_options.unwrap_or_else(|| TrackOptionsInput {
        unlock_mode: default_track_unlock_mode(),
        enable_random_obtain_methods: true,
        include_cheat_only: false,
        include_unlocked_by_default: false,
        include_stunt_arena: false,
    });

    let all_cars = collect_available_cars(&scan_result);
    if all_cars.is_empty() {
        return Err("No valid cars found in scan result. Cannot generate.".to_string());
    }

    // 1. Prepare specs with resolved pool distributions
    let mut final_specs_stock = cars_spec_state.stock_cars.clone();
    let mut final_specs_dc = cars_spec_state.dc_cars.clone();

    // Identify slots that need a random pool rating.
    // Distribution constraints are global across stock + DC.
    let flexible_stock_indices: Vec<usize> = final_specs_stock.iter().enumerate()
        .filter(|(_, s)| s.source_rating == "Random" && !is_specific_car_pool(&s.source_pool))
        .map(|(i, _)| i).collect();
    let flexible_dc_indices: Vec<usize> = final_specs_dc.iter().enumerate()
        .filter(|(_, s)| s.source_rating == "Random" && !is_specific_car_pool(&s.source_pool))
        .map(|(i, _)| i).collect();

    // Allocate source ratings (Pool Distribution) globally.
    let total_flexible = flexible_stock_indices.len() + flexible_dc_indices.len();
    let pool_ratings_all = allocate_ratings(
        total_flexible,
        &opts.pool_rating_distributions,
        opts.include_super_pro,
        &mut rng,
    );

    let mut pr_iter = pool_ratings_all.into_iter();
    for i in &flexible_stock_indices {
        if let Some(r) = pr_iter.next() {
            final_specs_stock[*i].source_rating = r.to_string();
        }
    }
    for i in &flexible_dc_indices {
        if let Some(r) = pr_iter.next() {
            final_specs_dc[*i].source_rating = r.to_string();
        }
    }

    // 2. Resolve car list (constrained slots first)
    let (stock_resolved, dc_resolved) = resolve_car_list(
        &final_specs_stock,
        &final_specs_dc,
        &all_cars,
        &scan_result,
        &mut rng,
    );

    // 3. Resolve attribute distributions

    // We need to resolve attr ratings globally for those marked "Random"
    let random_attr_stock_indices: Vec<usize> = final_specs_stock.iter().enumerate()
        .filter(|(_, s)| s.attr_rating == "Random").map(|(i, _)| i).collect();
    let random_attr_dc_indices: Vec<usize> = final_specs_dc.iter().enumerate()
        .filter(|(_, s)| s.attr_rating == "Random").map(|(i, _)| i).collect();

    // Attribute rating distributions are also global across stock + DC.
    let total_random_attr = random_attr_stock_indices.len() + random_attr_dc_indices.len();
    let allocated_attr_all = allocate_ratings(
        total_random_attr,
        &opts.attr_rating_distributions,
        opts.include_super_pro,
        &mut rng,
    );

    let mut ar_iter = allocated_attr_all.into_iter();
    let mut attr_stock_map: std::collections::HashMap<usize, i32> = std::collections::HashMap::new();
    for i in &random_attr_stock_indices {
        if let Some(r) = ar_iter.next() {
            attr_stock_map.insert(*i, r);
        }
    }
    let mut attr_dc_map: std::collections::HashMap<usize, i32> = std::collections::HashMap::new();
    for i in &random_attr_dc_indices {
        if let Some(r) = ar_iter.next() {
            attr_dc_map.insert(*i, r);
        }
    }

    // Build the final RandomizedCar list
    let mut stock_cars = Vec::new();
    for i in 0..final_specs_stock.len() {
        if let Some(car) = &stock_resolved[i] {
            let mut spec = final_specs_stock[i].clone();
            if let Some(&r) = attr_stock_map.get(&i) {
                spec.attr_rating = r.to_string();
            }
            stock_cars.push(build_randomized_car(car, &spec, &mut rng, &opts));
        }
    }

    let mut dc_cars = Vec::new();
    for i in 0..final_specs_dc.len() {
        if let Some(car) = &dc_resolved[i] {
            let mut spec = final_specs_dc[i].clone();
            if let Some(&r) = attr_dc_map.get(&i) {
                spec.attr_rating = r.to_string();
            }
            dc_cars.push(build_randomized_car(car, &spec, &mut rng, &opts));
        }
    }

    let mut tracks = Vec::new();
    if track_spec_state.include_tracks && !track_spec_state.tracks.is_empty() {
        let all_tracks = collect_available_tracks(&scan_result);
        let resolved_tracks = resolve_track_list(&track_spec_state.tracks, &all_tracks, &scan_result, &mut rng);
        for i in 0..track_spec_state.tracks.len() {
            if let Some(track) = &resolved_tracks[i] {
                let spec = &track_spec_state.tracks[i];
                tracks.push(RandomizedTrack {
                    folder: track.folder_name.clone(),
                    difficulty: resolve_track_difficulty(
                        &spec.attr_difficulty,
                        track.difficulty,
                        i,
                        &track_opts.unlock_mode,
                        &mut rng,
                    ),
                    obtain: resolve_track_obtain(
                        &spec.attr_obtain,
                        &track_opts.unlock_mode,
                        &track_opts,
                        &mut rng,
                    ),
                });
            }
        }
    }

    // Phase 3: Cup generation (depends on resolved track list)
    let cup_state = cup_spec_state.unwrap_or_else(|| CupSpecState {
        enabled: true,
        stage_mode: CupStageMode::Default,
        guarantee_first_normal: true,
        same_track_handling: SameTrackHandling::Forbid,
        allow_reverse: true,
        allow_mirror: false,
        allow_reverse_mirror: false,
        num_cars: 8,
        num_tries: 3,
        per_race_required_place: 3,
        overall_required_place: 1,
        points_table: default_points_table(),
        num_laps_min: 6,
        num_laps_max: 6,
        cups: vec![
            make_default_cup_spec_rust(0),
            make_default_cup_spec_rust(1),
            make_default_cup_spec_rust(2),
            make_default_cup_spec_rust(3),
        ],
    });
    let cups = generate_cups(&cup_state, &tracks, &scan_result, &mut rng);

    // 4. Build the output structure
    let config = ConfigData {
        metadata: ConfigMetadata {
            seed: "alpha-test-88".to_string(), // In the future, we can hook this up to the RNG
            version: "1.0.0".to_string(),
            profile_name: Some(profile_name),
        },
        global_options: ConfigGlobalOptions {
            load_extra_cars: false,
            load_extra_tracks: false,
            load_extra_cups: true,
        },
        stock_cars,
        dc_cars,
        tracks,
        cups,
    };

    // 5. Serialize to JSON
    let json = serde_json::to_string_pretty(&config)
        .map_err(|e| format!("JSON serialization error: {}", e))?;

    // 6. Resolve Output Path
    let mut out_path = app_handle
        .path()
        .app_local_data_dir()
        .map_err(|e| format!("Could not determine local data dir: {}", e))?;
    
    out_path.push("generated");
    
    if !out_path.exists() {
        fs::create_dir_all(&out_path).map_err(|e| format!("Could not create output directory: {}", e))?;
    }

    let safe_name = if file_name.ends_with(".json") { file_name } else { format!("{}.json", file_name) };
    out_path.push(safe_name);

    // 7. Write to file
    fs::write(&out_path, &json)
        .map_err(|e| format!("Could not write result.json: {}", e))?;

    // Return the absolute path so the frontend can store it for launching
    Ok(out_path.to_string_lossy().to_string())
}