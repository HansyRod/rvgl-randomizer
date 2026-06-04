use crate::scanner::{Car, Pool, ScanResult};
use std::collections::HashSet;
use super::rng::Rng;
use super::models::*;

pub const STOCK_CAR_FOLDERS: [&str; 28] = [
    "rc", "mite", "phat", "moss", "mud", "beatall", "volken",
    "tc6", "dino", "candy", "gencar", "tc4", "mouse", "flag",
    "tc2", "r5", "tc5", "sgt", "tc3", "adeon", "fone",
    "tc1", "rotor", "cougar", "sugo", "toyeca", "amw", "panga",
];

// ============================================================================
// Candidate set builder
// ============================================================================

pub fn is_stock_car_folder(folder: &str) -> bool {
    STOCK_CAR_FOLDERS
        .iter()
        .any(|stock| stock.eq_ignore_ascii_case(folder))
}

pub fn count_available_stock_cars(cars: &[Car]) -> usize {
    STOCK_CAR_FOLDERS
        .iter()
        .filter(|stock_folder| {
            cars.iter().any(|car| car.folder_name.eq_ignore_ascii_case(stock_folder))
        })
        .count()
}

/// Build a flat list of valid (non-system, valid-file) cars from the scan.
pub fn collect_available_cars(scan: &ScanResult) -> Vec<Car> {
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

pub fn has_all_stock_cars(cars: &[Car]) -> bool {
    count_available_stock_cars(cars) == STOCK_CAR_FOLDERS.len()
}

pub fn has_only_stock_cars_loaded(cars: &[Car]) -> bool {
    cars.len() == STOCK_CAR_FOLDERS.len()
        && cars.iter().all(|car| is_stock_car_folder(&car.folder_name))
}

pub fn is_specific_car_pool(pool: &str) -> bool {
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

pub fn resolve_car_list(
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
            // Build the pool from individual per-method flags
            let mut allowed: Vec<i32> = Vec::new();
            if car_options.include_starting_car  { allowed.push(0); }
            if car_options.include_championship   { allowed.push(1); }
            if car_options.include_time_trial     { allowed.push(2); }
            if car_options.include_practice_stars { allowed.push(3); }
            if car_options.include_single_race    { allowed.push(4); }
            if car_options.include_cheat_only     { allowed.push(-1); }
            if car_options.include_stunt_arena    { allowed.push(5); }
            if car_options.include_specific_race_win       { allowed.push(6); }
            if car_options.include_specific_practice_star  { allowed.push(7); }
            if car_options.include_specific_time_trial     { allowed.push(8); }
            if car_options.include_race_win_count          { allowed.push(9); }
            if car_options.include_practice_star_count     { allowed.push(10); }
            if car_options.include_time_trial_count        { allowed.push(11); }
            if car_options.include_stunt_arena_star_count  { allowed.push(12); }
            // Fallback: if all methods are disabled, use the full standard set
            if allowed.is_empty() {
                allowed = vec![0, 1, 2, 3, 4];
            }
            let idx = rng.next_usize(allowed.len());
            allowed[idx]
        }
        other => other.parse::<i32>().unwrap_or(scanned),
    }
}

pub fn build_randomized_car(car: &Car, spec: &CarSpec, rng: &mut Rng, car_options: &CarOptionsInput) -> RandomizedCar {
    RandomizedCar {
        folder: car.folder_name.clone(),
        rating: resolve_rating(&spec.attr_rating, car.rating, rng, car_options.include_super_pro),
        obtain: resolve_obtain(&spec.attr_obtain, car.obtain_method, rng, car_options),
        selectable_player: true,
        selectable_cpu: true,
        custom_unlock: None,
    }
}

// ============================================================================
// Global Distribution Allocator
// ============================================================================

pub fn allocate_ratings(
    count: usize,
    distributions: &std::collections::HashMap<String, RatingDist>,
    include_super_pro: bool,
    rng: &mut Rng,
) -> Vec<i32> {
    let mut result = Vec::with_capacity(count);
    if count == 0 { return result; }

    // Build the set of valid rating indices.
    // A rating is excluded only if:
    //   - It is Super Pro (index 5) and include_super_pro is false, OR
    //   - It has an explicit distribution entry with max == 0.
    // Ratings absent from the map are unrestricted (no forced zero).
    let all_indices: Vec<usize> = (0..=5).collect();
    let allowed_indices: Vec<usize> = all_indices
        .into_iter()
        .filter(|&i| {
            // Remove Super Pro entirely when disabled
            if i == 5 && !include_super_pro {
                return false;
            }
            // If there is an explicit entry with max == 0, exclude this rating
            if let Some(dist) = distributions.get(&i.to_string()) {
                return dist.max > 0;
            }
            // Not in the map → unrestricted, always allowed
            true
        })
        .collect();

    if allowed_indices.is_empty() {
        return result;
    }

    let mut remaining = count;
    let mut counts = vec![0usize; 6]; // index == rating value

    // 1. Assign minimums for ratings that have an explicit distribution entry.
    //    Cap each minimum against that rating's max and the remaining budget.
    for &i in &allowed_indices {
        if let Some(dist) = distributions.get(&i.to_string()) {
            let m = dist.min.min(dist.max).min(remaining);
            counts[i] = m;
            remaining -= m;
        }
    }

    // 2. Distribute the remaining slots randomly, respecting per-rating maximums.
    //    Ratings without a map entry are considered unbounded (no upper limit).
    while remaining > 0 {
        let candidates: Vec<usize> = allowed_indices
            .iter()
            .cloned()
            .filter(|&i| {
                if let Some(dist) = distributions.get(&i.to_string()) {
                    // Respect the explicit maximum
                    counts[i] < dist.max
                } else {
                    // No restriction on this rating — always a valid candidate
                    true
                }
            })
            .collect();

        if candidates.is_empty() { break; } // All explicit maxes are exhausted

        let pick = candidates[rng.next_usize(candidates.len())];
        counts[pick] += 1;
        remaining -= 1;
    }

    // 3. Build the flat list (ordered lowest rating → highest)
    for (i, &c) in counts.iter().enumerate() {
        for _ in 0..c {
            result.push(i as i32);
        }
    }

    // 4. Safety: if explicit maxes were too tight to fill the count, fill the
    //    remainder from the allowed set (ignoring maximums as a last resort).
    while result.len() < count {
        let ridx = rng.next_usize(allowed_indices.len());
        result.push(allowed_indices[ridx] as i32);
    }

    rng.shuffle(&mut result);
    result
}