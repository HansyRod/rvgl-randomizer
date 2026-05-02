use crate::scanner::ScanResult;
use std::fs;
use tauri::Manager;

use super::models::*;
use super::cars::*;
use super::tracks::*;
use super::cups::*;
use super::rng::Rng;

// ============================================================================
// Tauri command entry point
// ============================================================================

#[tauri::command]
pub fn generate_result(
    app_handle: tauri::AppHandle,
    install_path: String,
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
        include_cheat_only: false,
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

    // 4. Assemble UiContext
    let required_packs: Vec<String> = match &scan_result.install_type {
        crate::scanner::InstallType::Launcher => scan_result.content_packs
            .as_deref()
            .unwrap_or(&[])
            .iter()
            .filter(|p| p.use_cars || p.use_tracks)
            .map(|p| p.name.clone())
            .collect(),
        crate::scanner::InstallType::Classic => vec![],
    };

    let configure = serde_json::json!({
        "carOptions": opts,
        "trackOptions": track_opts,
        "carsSpecState": cars_spec_state,
        "trackSpecState": track_spec_state,
        "cupSpecState": cup_state,
    });

    let mut generated_car_folders: Vec<String> = stock_cars.iter().map(|c| c.folder.clone()).collect();
    generated_car_folders.extend(dc_cars.iter().map(|c| c.folder.clone()));

    let generated_track_folders: Vec<String> = tracks.iter().map(|t| t.folder.clone()).collect();

    let ui_context = Some(UiContext {
        generated_at: chrono::Utc::now().to_rfc3339(),
        setup: UiInstallContext {
            install_type: format!("{:?}", scan_result.install_type).to_lowercase(),
            install_path,
            required_packs,
        },
        configure,
        generated_car_folders,
        generated_track_folders,
    });

    // 5. Build the output structure
    let config = ConfigData {
        metadata: ConfigMetadata {
            seed: "alpha-test-88".to_string(), // In the future, we can hook this up to the RNG
            version: "1.0.0".to_string(),
            profile_name: Some(profile_name),
            ui_context,
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
