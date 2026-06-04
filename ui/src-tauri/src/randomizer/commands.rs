use crate::scanner::ScanResult;
use std::fs;
use tauri::Manager;

use super::models::*;
use super::cars::*;
use super::tracks::*;
use super::custom_unlocks::*;
use super::cups::*;
use super::rng::Rng;

fn is_effective_stock_mode(has_all_stock: bool, has_only_stock: bool, preset_stock_mode: bool) -> bool {
    has_all_stock && (has_only_stock || preset_stock_mode)
}

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
    preset_id: String,
    preset_stock_mode: Option<PresetStockModeInput>,
    file_name: String,
    profile_name: String,
) -> Result<String, String> {
    let mut rng = Rng::new();

    // Use provided options or fall back to safe defaults
    let opts = car_options.unwrap_or_else(|| CarOptionsInput {
        unlock_mode:             default_unlock_mode(),
        num_starting_cars:       0,
        enable_starting_cars_pool: default_false(),
        starting_cars_pool:      default_pool(),
        enable_starting_cars_rating: default_false(),
        starting_cars_rating:    default_random(),
        include_cheat_only:      false,
        include_stunt_arena:     false,
        include_starting_car:    default_true(),
        include_championship:    default_true(),
        include_time_trial:      default_true(),
        include_practice_stars:  default_true(),
        include_single_race:     default_true(),
        include_specific_race_win: default_false(),
        include_specific_practice_star: default_false(),
        include_specific_time_trial: default_false(),
        include_race_win_count: default_false(),
        include_practice_star_count: default_false(),
        include_time_trial_count: default_false(),
        include_stunt_arena_star_count: default_false(),
        specific_race_win_track_count_min: default_one(),
        specific_race_win_track_count_max: default_one(),
        specific_practice_star_track_count_min: default_one(),
        specific_practice_star_track_count_max: default_one(),
        specific_time_trial_track_count_min: default_one(),
        specific_time_trial_track_count_max: default_one(),
        race_win_count_min: default_one(),
        race_win_count_max: default_track_count_max(),
        practice_star_count_min: default_one(),
        practice_star_count_max: default_track_count_max(),
        time_trial_count_min: default_one(),
        time_trial_count_max: default_track_count_max(),
        stunt_arena_star_count_min: default_one(),
        stunt_arena_star_count_max: default_stunt_arena_star_count_max(),
        include_super_pro:       default_true(),
        pool_rating_distributions: std::collections::HashMap::new(),
        attr_rating_distributions: std::collections::HashMap::new(),
    });
    let track_opts = track_options.unwrap_or_else(|| TrackOptionsInput {
        unlock_mode: default_track_unlock_mode(),
        include_stunt_arena: false,
        include_default: default_true(),
        include_time_trial: default_true(),
        include_practice: default_true(),
        include_single_race: default_true(),
        include_specific_race_win: default_false(),
        include_specific_practice_star: default_false(),
        include_specific_time_trial: default_false(),
        include_race_win_count: default_false(),
        include_practice_star_count: default_false(),
        include_time_trial_count: default_false(),
        include_stunt_arena_star_count: default_false(),
        specific_race_win_track_count_min: default_one(),
        specific_race_win_track_count_max: default_one(),
        specific_practice_star_track_count_min: default_one(),
        specific_practice_star_track_count_max: default_one(),
        specific_time_trial_track_count_min: default_one(),
        specific_time_trial_track_count_max: default_one(),
        race_win_count_min: default_one(),
        race_win_count_max: default_track_count_max(),
        practice_star_count_min: default_one(),
        practice_star_count_max: default_track_count_max(),
        time_trial_count_min: default_one(),
        time_trial_count_max: default_track_count_max(),
        stunt_arena_star_count_min: default_one(),
        stunt_arena_star_count_max: default_stunt_arena_star_count_max(),
    });

    let all_cars = collect_available_cars(&scan_result);
    let all_tracks = collect_available_tracks(&scan_result);
    let preset_stock_mode = preset_stock_mode.unwrap_or_default();
    let has_all_stock_cars = has_all_stock_cars(&all_cars);
    let has_only_stock_cars = has_only_stock_cars_loaded(&all_cars);
    let has_all_stock_tracks = has_all_stock_tracks(&all_tracks);
    let has_only_stock_tracks = has_only_stock_tracks_loaded(&all_tracks);
    let is_stock_cars = is_effective_stock_mode(
        has_all_stock_cars,
        has_only_stock_cars,
        preset_stock_mode.cars,
    );
    let is_stock_tracks = is_effective_stock_mode(
        has_all_stock_tracks,
        has_only_stock_tracks,
        preset_stock_mode.tracks,
    );

    let mut content_errors = Vec::new();
    if preset_stock_mode.cars && !has_all_stock_cars {
        content_errors.push(format!(
            "The selected stock preset requires all {} stock cars, but only {} are currently available.",
            STOCK_CAR_FOLDERS.len(),
            count_available_stock_cars(&all_cars)
        ));
    }
    if preset_stock_mode.tracks && !has_all_stock_tracks {
        content_errors.push(format!(
            "The selected stock preset requires all {} stock tracks, but only {} are currently available.",
            STOCK_TRACK_FOLDERS.len() - 1,
            count_available_stock_tracks(&all_tracks)
        ));
    }

    let min_cars = if is_stock_cars { 28 } else { 42 };
    let min_tracks = if is_stock_tracks { 13 } else { 14 };

    if all_cars.len() < min_cars {
        content_errors.push(format!(
            "Only {} eligible cars are available, but generation requires at least {}.",
            all_cars.len(),
            min_cars
        ));
    }
    if all_tracks.len() < min_tracks {
        content_errors.push(format!(
            "Only {} eligible tracks are available, but generation requires at least {}.",
            all_tracks.len(),
            min_tracks
        ));
    }
    if !content_errors.is_empty() {
        return Err(content_errors.join(" "));
    }

    // 1. Prepare specs with resolved pool distributions
    let mut final_specs_stock = if cars_spec_state.include_stock_cars {
        cars_spec_state.stock_cars.clone()
    } else {
        Vec::new()
    };
    let mut final_specs_dc = if cars_spec_state.include_dc_cars {
        cars_spec_state.dc_cars.clone()
    } else {
        Vec::new()
    };

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
    let mut stock_car_specs = Vec::new();
    for i in 0..final_specs_stock.len() {
        if let Some(car) = &stock_resolved[i] {
            let mut spec = final_specs_stock[i].clone();
            if let Some(&r) = attr_stock_map.get(&i) {
                spec.attr_rating = r.to_string();
            }
            stock_cars.push(build_randomized_car(car, &spec, &mut rng, &opts));
            stock_car_specs.push(spec);
        }
    }

    let mut dc_cars = Vec::new();
    let mut dc_car_specs = Vec::new();
    for i in 0..final_specs_dc.len() {
        if let Some(car) = &dc_resolved[i] {
            let mut spec = final_specs_dc[i].clone();
            if let Some(&r) = attr_dc_map.get(&i) {
                spec.attr_rating = r.to_string();
            }
            dc_cars.push(build_randomized_car(car, &spec, &mut rng, &opts));
            dc_car_specs.push(spec);
        }
    }

    let mut tracks = Vec::new();
    let mut track_specs = Vec::new();
    if track_spec_state.include_tracks && !track_spec_state.tracks.is_empty() {
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
                        track_spec_state.tracks.len(),
                        &track_opts.unlock_mode,
                        &mut rng,
                    ),
                    obtain: resolve_track_obtain(
                        &spec.attr_obtain,
                        &track_opts.unlock_mode,
                        &track_opts,
                        &mut rng,
                    ),
                    custom_unlock: None,
                });
                track_specs.push(spec.clone());
            }
        }
        ensure_track_difficulty_coverage(
            &mut tracks,
            &track_spec_state.tracks,
            &all_tracks,
            &scan_result,
            &track_opts.unlock_mode,
            &mut rng,
        );
        apply_track_custom_unlocks(&mut tracks, &track_specs, &track_opts, &mut rng)?;
        tracks.sort_by_key(|track| track.difficulty);
    }

    let cup_tracks = if track_spec_state.include_tracks {
        tracks.clone()
    } else {
        collect_stock_tracks_in_order(&all_tracks, !is_stock_tracks)
            .into_iter()
            .map(|track| RandomizedTrack {
                folder: track.folder_name,
                difficulty: track.difficulty,
                obtain: 0,
                custom_unlock: None,
            })
            .collect()
    };

    apply_car_custom_unlocks(&mut stock_cars, &stock_car_specs, &cup_tracks, &opts, "Stock car", &mut rng)?;
    apply_car_custom_unlocks(&mut dc_cars, &dc_car_specs, &cup_tracks, &opts, "DC car", &mut rng)?;

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
        num_laps_min: 2,
        num_laps_max: 8,
        num_stages_min: 3,
        num_stages_max: 6,
        cups: vec![
            make_default_cup_spec_rust(0),
            make_default_cup_spec_rust(1),
            make_default_cup_spec_rust(2),
            make_default_cup_spec_rust(3),
        ],
    });
    let cups = generate_cups(&cup_state, &cup_tracks, &scan_result, &mut rng);

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
        "preset": preset_id,
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
            seed: rng.seed().to_string(),
            version: "0.1.0".to_string(),
            profile_name: Some(profile_name),
            ui_context,
        },
        global_options: ConfigGlobalOptions {
            load_extra_cars: false,
            load_extra_tracks: false,
            load_extra_cups: false,
            is_stock_cars,
            is_stock_tracks,
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

#[cfg(test)]
mod tests {
    use super::is_effective_stock_mode;
    use crate::randomizer::cars::{
        count_available_stock_cars,
        has_all_stock_cars,
        has_only_stock_cars_loaded,
        STOCK_CAR_FOLDERS,
    };
    use crate::randomizer::tracks::{
        count_available_stock_tracks,
        has_all_stock_tracks,
        has_only_stock_tracks_loaded,
        STOCK_TRACK_FOLDERS,
    };
    use crate::scanner::{Car, Pool, Track};

    fn make_car(folder_name: &str) -> Car {
        Car {
            folder_name: folder_name.to_string(),
            name: folder_name.to_string(),
            rating: 0,
            obtain_method: 0,
            is_system_car: false,
            has_valid_file: true,
            carbox_filename: None,
            pool: if STOCK_CAR_FOLDERS.iter().any(|stock| stock.eq_ignore_ascii_case(folder_name)) {
                Pool::Stock
            } else {
                Pool::Custom
            },
        }
    }

    fn make_track(folder_name: &str) -> Track {
        Track {
            folder_name: folder_name.to_string(),
            name: folder_name.to_string(),
            has_reversed: false,
            track_type: 0,
            difficulty: 1,
            has_valid_file: true,
        }
    }

    #[test]
    fn pure_stock_content_activates_stock_mode_without_preset() {
        let cars: Vec<Car> = STOCK_CAR_FOLDERS.iter().map(|folder| make_car(folder)).collect();
        let tracks: Vec<Track> = STOCK_TRACK_FOLDERS
            .iter()
            .filter(|folder| !folder.eq_ignore_ascii_case("roof"))
            .map(|folder| make_track(folder))
            .collect();

        assert!(has_all_stock_cars(&cars));
        assert!(has_only_stock_cars_loaded(&cars));
        assert!(has_all_stock_tracks(&tracks));
        assert!(has_only_stock_tracks_loaded(&tracks));
        assert!(is_effective_stock_mode(true, true, false));
    }

    #[test]
    fn mixed_content_plus_stock_preset_activates_stock_mode() {
        let mut cars: Vec<Car> = STOCK_CAR_FOLDERS.iter().map(|folder| make_car(folder)).collect();
        cars.push(make_car("custom_car"));

        let mut tracks: Vec<Track> = STOCK_TRACK_FOLDERS
            .iter()
            .filter(|folder| !folder.eq_ignore_ascii_case("roof"))
            .map(|folder| make_track(folder))
            .collect();
        tracks.push(make_track("custom_track"));

        assert!(has_all_stock_cars(&cars));
        assert!(!has_only_stock_cars_loaded(&cars));
        assert!(has_all_stock_tracks(&tracks));
        assert!(!has_only_stock_tracks_loaded(&tracks));
        assert!(is_effective_stock_mode(true, false, true));
    }

    #[test]
    fn mixed_content_plus_non_stock_preset_does_not_activate_stock_mode() {
        let mut cars: Vec<Car> = STOCK_CAR_FOLDERS.iter().map(|folder| make_car(folder)).collect();
        cars.push(make_car("custom_car"));

        let mut tracks: Vec<Track> = STOCK_TRACK_FOLDERS
            .iter()
            .filter(|folder| !folder.eq_ignore_ascii_case("roof"))
            .map(|folder| make_track(folder))
            .collect();
        tracks.push(make_track("custom_track"));

        assert!(has_all_stock_cars(&cars));
        assert!(!has_only_stock_cars_loaded(&cars));
        assert!(has_all_stock_tracks(&tracks));
        assert!(!has_only_stock_tracks_loaded(&tracks));
        assert!(!is_effective_stock_mode(true, false, false));
    }

    #[test]
    fn stock_preset_without_complete_stock_roster_does_not_activate_stock_mode() {
        let cars: Vec<Car> = STOCK_CAR_FOLDERS
            .iter()
            .take(STOCK_CAR_FOLDERS.len() - 1)
            .map(|folder| make_car(folder))
            .collect();
        let tracks: Vec<Track> = STOCK_TRACK_FOLDERS
            .iter()
            .filter(|folder| !folder.eq_ignore_ascii_case("roof"))
            .take(STOCK_TRACK_FOLDERS.len() - 2)
            .map(|folder| make_track(folder))
            .collect();

        assert_eq!(count_available_stock_cars(&cars), STOCK_CAR_FOLDERS.len() - 1);
        assert!(!has_all_stock_cars(&cars));
        assert_eq!(count_available_stock_tracks(&tracks), STOCK_TRACK_FOLDERS.len() - 2);
        assert!(!has_all_stock_tracks(&tracks));
        assert!(!is_effective_stock_mode(false, false, true));
    }
}
