use crate::scanner::{Car, Pool, ScanResult, Track};
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
    #[serde(default = "default_pool")]
    pub starting_cars_pool: String,
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
    pub cups: Vec<serde_json::Value>,
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
    spec_state: CarsSpecState,
    car_options: Option<CarOptionsInput>,
    track_spec_state: TrackSpecState,
    track_options: Option<TrackOptionsInput>,
    file_name: String,
) -> Result<String, String> {
    let mut rng = Rng::new();

    // Use provided options or fall back to safe defaults
    let opts = car_options.unwrap_or_else(|| CarOptionsInput {
        unlock_mode:          default_unlock_mode(),
        num_starting_cars:    0,
        starting_cars_pool:   default_pool(),
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
    let mut final_specs_stock = spec_state.stock_cars.clone();
    let mut final_specs_dc = spec_state.dc_cars.clone();

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

    // 4. Build the output structure
    let config = ConfigData {
        metadata: ConfigMetadata {
            seed: "alpha-test-88".to_string(), // In the future, we can hook this up to the RNG
            version: "1.0.0".to_string(),
        },
        global_options: ConfigGlobalOptions {
            load_extra_cars: false,
            load_extra_tracks: false,
            load_extra_cups: true,
        },
        stock_cars,
        dc_cars,
        tracks,
        cups: vec![],
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