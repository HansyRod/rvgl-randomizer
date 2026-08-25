use crate::scanner::{ScanResult, InstallType};

use super::models::*;
use super::rng::Rng;

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

/// Build a points table that is exactly wide enough for every supported cup
/// finishing position. Short input tables are padded with zeroes; long input
/// tables are truncated at the contract boundary.
fn pad_points(src: &[i32]) -> Vec<i32> {
    let mut v = src.to_vec();
    v.truncate(CUP_POINTS_TABLE_LENGTH);
    while v.len() < CUP_POINTS_TABLE_LENGTH { v.push(0); }
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
// Follows the base game layout with fixed lap counts per stage:
//   Bronze  : tracks[0,1,2,3] all Normal with laps [3,4,3,5]
//   Silver  : tracks[4,5,6,7] all Normal with laps [4,6,4,4]
//   Gold    : track8(Normal), track4(Mirror), track9(Normal), track10(Normal) with laps [5,5,5,5]
//   Platinum: track11(Normal), track12(Normal), track5(RevMirror or Mirror), track10(Mirror), track13(Normal) with laps [6,6,8,6,6]
// ============================================================================

/// Returns the fixed lap counts for each stage in a default cup.
fn default_laps_per_stage(cup_index: usize) -> &'static [u32] {
    match cup_index {
        0 => &[3, 4, 3, 5],     // Bronze
        1 => &[4, 6, 4, 4],     // Silver
        2 => &[5, 8, 5, 5],     // Gold
        3 => &[6, 6, 10, 6, 6],  // Platinum
        _ => &[],
    }
}

fn build_default_stages(
    cup_index: usize,
    resolved: &[RandomizedTrack],
    scan: &ScanResult,
    rng: &mut Rng,
) -> Vec<RandomizedCupStage> {
    let laps = default_laps_per_stage(cup_index);
    
    let make_stage = |folder: &str, is_reverse: bool, is_mirror: bool, num_laps: u32| {
        RandomizedCupStage {
            track_folder: folder.to_string(),
            num_laps,
            is_reverse,
            is_mirror,
        }
    };
    let push_stage_at = |stages: &mut Vec<RandomizedCupStage>, idx: usize, is_reverse: bool, is_mirror: bool, num_laps: u32| {
        if let Some(track) = resolved.get(idx) {
            stages.push(make_stage(&track.folder, is_reverse, is_mirror, num_laps));
        }
    };

    match cup_index {
        // Bronze: tracks 0–3, all Normal
        0 => {
            let mut stages = Vec::new();
            for i in 0..4.min(resolved.len()) {
                if let Some(&lap) = laps.get(i) {
                    push_stage_at(&mut stages, i, false, false, lap);
                }
            }
            stages
        }

        // Silver: tracks 4–7, all Normal (or 13-track alternative)
        1 => {
            let mut stages = Vec::new();
            if resolved.len() == 13 {
                // 13-track mode: toylite (4), nhood1 (0), wild_west1 (5), toy2 (6)
                let layout = [
                    (4, 6, false, false),
                    (0, 4, false, true),
                    (5, 4, false, false),
                    (6, 4, false, false),
                ];
                for &(idx, l, rev, mir) in &layout {
                    push_stage_at(&mut stages, idx, rev, mir, l);
                }
            } else {
                for i in 0..4.min(resolved.len()) {
                    let idx = 4 + i;
                    if let Some(&lap) = laps.get(i) {
                        push_stage_at(&mut stages, idx, false, false, lap);
                    }
                }
            }
            stages
        }

        // Gold: track8, track4(mirror), track9, track10 (or 13-track alternative)
        2 => {
            let mut stages = Vec::new();
            let indices = if resolved.len() == 13 {
                [7, 4, 8, 9]
            } else {
                [8, 5, 9, 10]
            };
            for (i, &track_idx) in indices.iter().enumerate() {
                if let Some(&lap) = laps.get(i) {
                    let is_mirror = i == 1; // second stage is mirrored
                    push_stage_at(&mut stages, track_idx, false, is_mirror, lap);
                }
            }
            stages
        }

        // Platinum: track11, track12, track5(with variant logic), track10(mirror), track13 (or 13-track alternative)
        3 => {
            let mut stages = Vec::new();
            
            // Stage 0: track11 (Normal)
            // Stage 1: track12 (Normal)
            if resolved.len() == 13 {
                if let Some(&lap) = laps.get(0) {
                    push_stage_at(&mut stages, 10, false, false, lap);
                }
                if let Some(&lap) = laps.get(1) {
                    push_stage_at(&mut stages, 11, false, false, lap);
                }
            }
            else {
                if let Some(&lap) = laps.get(0) {
                    push_stage_at(&mut stages, 11, false, false, lap);
                }
                if let Some(&lap) = laps.get(1) {
                    push_stage_at(&mut stages, 12, false, false, lap);
                }
            }
            
            let stage_2_idx = if resolved.len() == 13 { 4 } else { 5 };
            let stage_3_idx = if resolved.len() == 13 { 9 } else { 10 };
            let stage_4_idx = if resolved.len() == 13 { 12 } else { 13 };

            // Stage 2: track5 with variant logic
            if let (Some(track), Some(&lap)) = (resolved.get(stage_2_idx), laps.get(2)) {
                let has_rev = track_has_reversed(&track.folder, scan);
                if has_rev {
                    push_stage_at(&mut stages, stage_2_idx, true, true, lap);
                } else {
                    // Fallback: collect all tracks from positions 0–10 with has_rev, excluding 5/10
                    let candidates: Vec<usize> = (0..11.min(resolved.len()))
                        .filter(|&i| i != stage_2_idx && i != stage_3_idx && track_has_reversed(&resolved[i].folder, scan))
                        .collect();
                    if !candidates.is_empty() {
                        let idx = candidates[rng.next_usize(candidates.len())];
                        push_stage_at(&mut stages, idx, true, true, lap);
                    } else {
                        // Last resort: just play it in Mirror only
                        push_stage_at(&mut stages, stage_2_idx, false, true, lap);
                    }
                }
            }
            
            // Stage 3: track10 (Mirror)
            if let Some(&lap) = laps.get(3) {
                push_stage_at(&mut stages, stage_3_idx, false, true, lap);
            }
            
            // Stage 4: track13 (Normal)
            if let Some(&lap) = laps.get(4) {
                push_stage_at(&mut stages, stage_4_idx, false, false, lap);
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

        // Laps: use per-stage range if specified, else fall back to cup-level range
        let stage_laps_min = spec.num_laps_min.unwrap_or(laps_min);
        let stage_laps_max = spec.num_laps_max.unwrap_or(laps_max);

        stages.push(RandomizedCupStage {
            track_folder: folder,
            num_laps: spec.num_laps.unwrap_or_else(|| roll_laps(stage_laps_min, stage_laps_max, rng)),
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
    // Slot-based selection: "slot:N" maps directly to resolved[N]
    if let Some(slot_str) = spec.source_pool.strip_prefix("slot:") {
        if let Ok(idx) = slot_str.parse::<usize>() {
            if idx < resolved.len() {
                return Some(resolved[idx].folder.clone());
            }
        }
        // Out-of-range or malformed slot — fall through to random
    }
    // Specific folder name (not "Random", not a digit prefix, not "slot:")
    else if spec.source_pool != "Random"
        && !spec.source_pool.starts_with(|c: char| c.is_ascii_digit())
    {
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

pub fn generate_cups(
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

        // Effective stage mode: per-cup override wins, otherwise fall through to global
        let effective_mode = if cup_spec.map(|c| c.override_stage_mode).unwrap_or(false) {
            cup_spec.map(|c| c.stage_mode.clone()).unwrap_or_else(|| cup_state.stage_mode.clone())
        } else {
            cup_state.stage_mode.clone()
        };

        let num_cars = if cup_spec.map(|c| c.override_num_cars).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_cars).unwrap_or(cup_state.num_cars)
        } else { cup_state.num_cars };

        let num_tries = if cup_spec.map(|c| c.override_num_tries).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_tries).unwrap_or(cup_state.num_tries)
        } else { cup_state.num_tries };

        let per_race = if cup_spec.map(|c| c.override_per_race_place).unwrap_or(false) {
            cup_spec.and_then(|c| c.per_race_required_place).unwrap_or(cup_state.per_race_required_place)
        } else { cup_state.per_race_required_place };

        let overall = if cup_spec.map(|c| c.override_overall_place).unwrap_or(false) {
            cup_spec.and_then(|c| c.overall_required_place).unwrap_or(cup_state.overall_required_place)
        } else { cup_state.overall_required_place };

        let points = if cup_spec.map(|c| c.override_points_table).unwrap_or(false) {
            cup_spec.and_then(|c| c.points_table.clone()).unwrap_or_else(|| cup_state.points_table.clone())
        } else { cup_state.points_table.clone() };

        let cars_per_class = if cup_spec.map(|c| c.override_cars_per_class).unwrap_or(false) {
            cup_spec.and_then(|c| c.cars_per_class.clone()).unwrap_or_else(|| default_cars_per_class(cup_index))
        } else { default_cars_per_class(cup_index) };

        // Laps range: use per-cup override when override_stage_mode is active, else global
        let laps_min = if cup_spec.map(|c| c.override_num_laps_min).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_laps_min).unwrap_or(cup_state.num_laps_min)
        } else { cup_state.num_laps_min };
        let laps_max = if cup_spec.map(|c| c.override_num_laps_max).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_laps_max).unwrap_or(cup_state.num_laps_max)
        } else { cup_state.num_laps_max };

        // Num stages: use per-cup override when override_stage_mode is active, else global (only applies to Random mode)
        let num_stages_min = if cup_spec.map(|c| c.override_num_stages_min).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_stages_min).unwrap_or(cup_state.num_stages_min)
        } else { cup_state.num_stages_min };
        let num_stages_max = if cup_spec.map(|c| c.override_num_stages_max).unwrap_or(false) {
            cup_spec.and_then(|c| c.num_stages_max).unwrap_or(cup_state.num_stages_max)
        } else { cup_state.num_stages_max };

        // ── build stages ──────────────────────────────────────────────
        let mut per_cup_usage = CupUsage::new();

        let stages = match effective_mode {
            CupStageMode::Default => {
                let s = build_default_stages(cup_index, resolved_tracks, scan, rng);
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
                let num_stages_min = num_stages_min.max(1);
                let num_stages_max = num_stages_max.max(num_stages_min);
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
            custom_unlock: None,
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
        stages: build_default_stages(i, resolved_tracks, scan, rng),
        custom_unlock: None,
    }).collect()
}

pub fn make_default_cup_spec_rust(index: usize) -> CupSpec {
    CupSpec {
        index,
        override_stage_mode: false,
        override_num_cars: false,
        override_cars_per_class: false,
        override_num_tries: false,
        override_per_race_place: false,
        override_overall_place: false,
        override_points_table: false,
        override_num_stages_min: false,
        override_num_stages_max: false,
        override_num_laps_min: false,
        override_num_laps_max: false,

        stage_mode: CupStageMode::Default,
        num_cars: None,
        num_tries: None,
        per_race_required_place: None,
        overall_required_place: None,
        points_table: None,
        cars_per_class: None,
        num_laps_min: None,
        num_laps_max: None,
        num_stages_min: None,
        num_stages_max: None,
        stages: vec![],
    }
}
