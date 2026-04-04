use crate::scanner::{Car, Pool, ScanResult};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::fs;
use std::path::Path;

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
#[serde(rename_all = "camelCase")]
pub struct ConfigData {
    pub metadata: ConfigMetadata,
    pub global_options: ConfigGlobalOptions,
    pub stock_cars: Vec<RandomizedCar>,
    pub dc_cars: Vec<RandomizedCar>,
    pub tracks: Vec<serde_json::Value>,
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

    // --- Step 2: Rating filter (only when pool is not a specific car) ---
    let rating_filtered: Vec<&Car> =
        if spec.source_pool.is_empty()
            || spec.source_pool == "Full Random"
            || spec.source_pool == "Stock"
            || spec.source_pool == "DC"
            || spec.source_pool == "Custom"
            || spec.source_pool.starts_with("Pack:")
        {
            if spec.source_rating == "Random" {
                pool_filtered
            } else if let Ok(r) = spec.source_rating.parse::<i32>() {
                pool_filtered.into_iter().filter(|c| c.rating == r).collect()
            } else {
                pool_filtered
            }
        } else {
            pool_filtered // specific car — skip further filters
        };

    // --- Step 3: Obtain filter ---
    if spec.source_pool.is_empty()
        || spec.source_pool == "Full Random"
        || spec.source_pool == "Stock"
        || spec.source_pool == "DC"
        || spec.source_pool == "Custom"
        || spec.source_pool.starts_with("Pack:")
    {
        if spec.source_obtain == "Random" {
            rating_filtered
        } else if let Ok(o) = spec.source_obtain.parse::<i32>() {
            rating_filtered
                .into_iter()
                .filter(|c| c.obtain_method == o)
                .collect()
        } else {
            rating_filtered
        }
    } else {
        rating_filtered
    }
}

// ============================================================================
// Core randomizer
// ============================================================================

struct SlotWork<'a> {
    category: &'static str, // "stock" | "dc"
    index: usize,
    spec: &'a CarSpec,
}

fn resolve_car_list<'a>(
    specs_stock: &'a [CarSpec],
    specs_dc: &'a [CarSpec],
    all_cars: &'a [Car],
    scan: &ScanResult,
    rng: &mut Rng,
) -> (Vec<Option<&'a Car>>, Vec<Option<&'a Car>>) {
    // Build work items with candidate counts
    let mut work: Vec<(SlotWork<'a>, usize)> = Vec::new();

    for (i, spec) in specs_stock.iter().enumerate() {
        let count = candidate_set(spec, all_cars, scan).len();
        work.push((SlotWork { category: "stock", index: i, spec }, count));
    }
    for (i, spec) in specs_dc.iter().enumerate() {
        let count = candidate_set(spec, all_cars, scan).len();
        work.push((SlotWork { category: "dc", index: i, spec }, count));
    }

    // Sort: fewest candidates first, stable (keeps original order within same count)
    work.sort_by_key(|(_, cnt)| *cnt);

    // Resolve slots
    let mut used_folders: HashSet<String> = HashSet::new();
    let mut stock_results: Vec<Option<&Car>> = vec![None; specs_stock.len()];
    let mut dc_results: Vec<Option<&Car>> = vec![None; specs_dc.len()];

    for (slot, _) in &work {
        let candidates = candidate_set(slot.spec, all_cars, scan);

        // Prefer unused cars
        let unused: Vec<&&Car> = candidates.iter().filter(|c| !used_folders.contains(&c.folder_name)).collect();

        let chosen: Option<&Car> = if !unused.is_empty() {
            let idx = rng.next_usize(unused.len());
            Some(unused[idx])
        } else if !candidates.is_empty() {
            // Fallback: allow duplicate
            let idx = rng.next_usize(candidates.len());
            Some(candidates[idx])
        } else {
            // No candidates at all — pick from entire pool as last resort
            if !all_cars.is_empty() {
                let idx = rng.next_usize(all_cars.len());
                Some(&all_cars[idx])
            } else {
                None
            }
        };

        if let Some(car) = chosen {
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

fn resolve_rating(attr: &str, scanned: i32, rng: &mut Rng) -> i32 {
    match attr {
        "Unchanged" => scanned,
        "Random" => rng.next_i32(0, 5),
        other => other.parse::<i32>().unwrap_or(scanned),
    }
}

fn resolve_obtain(attr: &str, scanned: i32, rng: &mut Rng) -> i32 {
    match attr {
        "Unchanged" => scanned,
        "Random" => rng.next_i32(0, 4),
        other => other.parse::<i32>().unwrap_or(scanned),
    }
}

fn build_randomized_car(car: &Car, spec: &CarSpec, rng: &mut Rng) -> RandomizedCar {
    RandomizedCar {
        folder: car.folder_name.clone(),
        rating: resolve_rating(&spec.attr_rating, car.rating, rng),
        obtain: resolve_obtain(&spec.attr_obtain, car.obtain_method, rng),
        selectable_player: true,
        selectable_cpu: true,
    }
}

// ============================================================================
// Tauri command entry point
// ============================================================================

#[tauri::command]
pub fn generate_result(
    scan_result: ScanResult,
    spec_state: CarsSpecState,
    output_path: String,
) -> Result<String, String> {
    let mut rng = Rng::new();

    // 1. Collect valid cars
    let all_cars = collect_available_cars(&scan_result);

    if all_cars.is_empty() {
        return Err("No valid cars found in scan result. Cannot generate.".to_string());
    }

    // 2. Resolve car list (constrained slots first)
    let (stock_resolved, dc_resolved) = resolve_car_list(
        &spec_state.stock_cars,
        &spec_state.dc_cars,
        &all_cars,
        &scan_result,
        &mut rng,
    );

    // 3. Populate attributes for each resolved car
    let stock_cars: Vec<RandomizedCar> = spec_state
        .stock_cars
        .iter()
        .enumerate()
        .filter_map(|(i, spec)| {
            stock_resolved[i].map(|car| build_randomized_car(car, spec, &mut rng))
        })
        .collect();

    let dc_cars: Vec<RandomizedCar> = spec_state
        .dc_cars
        .iter()
        .enumerate()
        .filter_map(|(i, spec)| {
            dc_resolved[i].map(|car| build_randomized_car(car, spec, &mut rng))
        })
        .collect();

    // 4. Build the output structure
    let config = ConfigData {
        metadata: ConfigMetadata {
            seed: "alpha-test-88".to_string(),
            version: "1.0.0".to_string(),
        },
        global_options: ConfigGlobalOptions {
            load_extra_cars: false,
            load_extra_tracks: false,
            load_extra_cups: true,
        },
        stock_cars,
        dc_cars,
        tracks: vec![],
        cups: vec![],
    };

    // 5. Serialize to JSON
    let json = serde_json::to_string_pretty(&config)
        .map_err(|e| format!("JSON serialization error: {}", e))?;

    // 6. Write to the specified output path
    let out_path = Path::new(&output_path);
    if let Some(parent) = out_path.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent)
                .map_err(|e| format!("Could not create output directory: {}", e))?;
        }
    }

    fs::write(out_path, &json)
        .map_err(|e| format!("Could not write result.json: {}", e))?;

    Ok(format!("Generated successfully → {}", output_path))
}
