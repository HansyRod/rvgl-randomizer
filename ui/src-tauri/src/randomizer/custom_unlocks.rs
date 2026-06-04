use super::models::{
    CarOptionsInput, CarSpec, CustomUnlockCondition, CustomUnlockSpec, CustomUnlockTrackMode,
    RandomizedCar, RandomizedTrack, TrackOptionsInput, TrackSpec,
};
use super::rng::Rng;

const SPECIFIC_CUSTOM_UNLOCK_METHODS: [i32; 3] = [6, 7, 8];
const COUNT_CUSTOM_UNLOCK_METHODS: [i32; 4] = [9, 10, 11, 12];

struct SpecificUnlockTrackCountRanges {
    race_win_min: i32,
    race_win_max: i32,
    practice_star_min: i32,
    practice_star_max: i32,
    time_trial_min: i32,
    time_trial_max: i32,
}

impl SpecificUnlockTrackCountRanges {
    fn from_car_options(options: &CarOptionsInput) -> Self {
        Self {
            race_win_min: options.specific_race_win_track_count_min,
            race_win_max: options.specific_race_win_track_count_max,
            practice_star_min: options.specific_practice_star_track_count_min,
            practice_star_max: options.specific_practice_star_track_count_max,
            time_trial_min: options.specific_time_trial_track_count_min,
            time_trial_max: options.specific_time_trial_track_count_max,
        }
    }

    fn from_track_options(options: &TrackOptionsInput) -> Self {
        Self {
            race_win_min: options.specific_race_win_track_count_min,
            race_win_max: options.specific_race_win_track_count_max,
            practice_star_min: options.specific_practice_star_track_count_min,
            practice_star_max: options.specific_practice_star_track_count_max,
            time_trial_min: options.specific_time_trial_track_count_min,
            time_trial_max: options.specific_time_trial_track_count_max,
        }
    }

    fn range_for_method(&self, method: i32) -> (i32, i32) {
        match method {
            6 => (self.race_win_min, self.race_win_max),
            7 => (self.practice_star_min, self.practice_star_max),
            8 => (self.time_trial_min, self.time_trial_max),
            _ => (1, 1),
        }
    }
}

pub fn apply_car_custom_unlocks(
    cars: &mut [RandomizedCar],
    specs: &[CarSpec],
    tracks: &[RandomizedTrack],
    options: &CarOptionsInput,
    row_label_prefix: &str,
    rng: &mut Rng,
) -> Result<(), String> {
    let ranges = SpecificUnlockTrackCountRanges::from_car_options(options);
    for (index, (car, spec)) in cars.iter_mut().zip(specs.iter()).enumerate() {
        car.custom_unlock = build_custom_unlock_condition(
            car.obtain,
            spec.custom_unlock.as_ref(),
            tracks,
            None,
            &make_row_label(row_label_prefix, index, &spec.id),
            &ranges,
            rng,
        )?;
    }
    Ok(())
}

pub fn apply_track_custom_unlocks(
    tracks: &mut [RandomizedTrack],
    specs: &[TrackSpec],
    options: &TrackOptionsInput,
    rng: &mut Rng,
) -> Result<(), String> {
    let ranges = SpecificUnlockTrackCountRanges::from_track_options(options);
    let track_pool = tracks.to_vec();
    for (index, (track, spec)) in tracks.iter_mut().zip(specs.iter()).enumerate() {
        track.custom_unlock = build_custom_unlock_condition(
            track.obtain,
            spec.custom_unlock.as_ref(),
            &track_pool,
            Some(&track.folder),
            &make_row_label("Track", index, &spec.id),
            &ranges,
            rng,
        )?;
    }
    Ok(())
}

fn build_custom_unlock_condition(
    obtain: i32,
    custom_unlock: Option<&CustomUnlockSpec>,
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
    row_label: &str,
    ranges: &SpecificUnlockTrackCountRanges,
    rng: &mut Rng,
) -> Result<Option<CustomUnlockCondition>, String> {
    if !is_custom_unlock_method(obtain) {
        return Ok(None);
    }

    if is_specific_custom_unlock_method(obtain) {
        if let Some(custom_unlock) = custom_unlock {
            if custom_unlock.method != obtain.to_string() {
                return Err(format!(
                    "{row_label}: custom unlock condition method {} does not match obtain method {}.",
                    custom_unlock.method,
                    obtain
                ));
            }
            return build_explicit_specific_track_condition(
                custom_unlock,
                tracks,
                excluded_track_folder,
                row_label,
                rng,
            )
            .map(Some);
        }

        return build_random_specific_track_condition(
            obtain,
            tracks,
            excluded_track_folder,
            row_label,
            ranges,
            rng,
        )
        .map(Some);
    }

    if is_count_custom_unlock_method(obtain) {
        let custom_unlock = custom_unlock.ok_or_else(|| {
            format!("{row_label}: custom unlock condition is missing for obtain method {obtain}.")
        })?;
        if custom_unlock.method != obtain.to_string() {
            return Err(format!(
                "{row_label}: custom unlock condition method {} does not match obtain method {}.",
                custom_unlock.method,
                obtain
            ));
        }

        let required_count = custom_unlock.required_count.unwrap_or(0);
        if required_count < 1 {
            return Err(format!("{row_label}: custom unlock required count must be greater than 0."));
        }

        return Ok(Some(CustomUnlockCondition {
            track_folders: Vec::new(),
            required_count,
            archipelago_item: String::new(),
        }));
    }

    Ok(None)
}

fn build_explicit_specific_track_condition(
    custom_unlock: &CustomUnlockSpec,
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
    row_label: &str,
    rng: &mut Rng,
) -> Result<CustomUnlockCondition, String> {
    let track_folders = match custom_unlock.mode {
        Some(CustomUnlockTrackMode::SpecificTracks) => {
            validate_specific_track_folders(
                &custom_unlock.track_folders,
                tracks,
                excluded_track_folder,
                row_label,
            )?;
            custom_unlock.track_folders.clone()
        }
        Some(CustomUnlockTrackMode::RandomTracks) => {
            let count = custom_unlock.random_track_count.unwrap_or(0);
            if count < 1 {
                return Err(format!(
                    "{row_label}: custom unlock random track count must be greater than 0."
                ));
            }
            choose_random_track_folders(tracks, count as usize, excluded_track_folder, row_label, rng)?
        }
        None => {
            return Err(format!(
                "{row_label}: custom unlock must use either specific tracks or a random track count."
            ));
        }
    };

    Ok(CustomUnlockCondition {
        track_folders,
        required_count: 0,
        archipelago_item: String::new(),
    })
}

fn build_random_specific_track_condition(
    method: i32,
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
    row_label: &str,
    ranges: &SpecificUnlockTrackCountRanges,
    rng: &mut Rng,
) -> Result<CustomUnlockCondition, String> {
    let eligible_count = count_eligible_tracks(tracks, excluded_track_folder);
    if eligible_count == 0 {
        return Err(format!(
            "{row_label}: custom unlock has no eligible prerequisite tracks."
        ));
    }

    let (min, max) = ranges.range_for_method(method);
    let min = min.max(1);
    let max = max.max(min);
    let eligible_count = eligible_count as i32;
    let clamped_max = max.min(eligible_count);
    let clamped_min = min.min(clamped_max);
    let count = rng.next_i32(clamped_min, clamped_max) as usize;

    Ok(CustomUnlockCondition {
        track_folders: choose_random_track_folders(tracks, count, excluded_track_folder, row_label, rng)?,
        required_count: 0,
        archipelago_item: String::new(),
    })
}

fn validate_specific_track_folders(
    track_folders: &[String],
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
    row_label: &str,
) -> Result<(), String> {
    if track_folders.is_empty() {
        return Err(format!("{row_label}: custom unlock requires at least one prerequisite track."));
    }

    for folder in track_folders {
        if !track_folder_exists(folder, tracks) {
            return Err(format!(
                "{row_label}: prerequisite track \"{}\" is not in the final generated track list.",
                folder
            ));
        }

        if excluded_track_folder
            .map(|excluded| folder.eq_ignore_ascii_case(excluded))
            .unwrap_or(false)
        {
            return Err(format!(
                "{row_label}: custom unlock cannot require the target track itself ({}).",
                folder
            ));
        }
    }

    Ok(())
}

fn choose_random_track_folders(
    tracks: &[RandomizedTrack],
    count: usize,
    excluded_track_folder: Option<&str>,
    row_label: &str,
    rng: &mut Rng,
) -> Result<Vec<String>, String> {
    let mut folders: Vec<String> = tracks
        .iter()
        .filter(|track| {
            excluded_track_folder
                .map(|excluded| !track.folder.eq_ignore_ascii_case(excluded))
                .unwrap_or(true)
        })
        .map(|track| track.folder.clone())
        .collect();
    if folders.is_empty() {
        return Err(format!(
            "{row_label}: custom unlock has no eligible prerequisite tracks."
        ));
    }
    if count > folders.len() {
        return Err(format!(
            "{row_label}: custom unlock requires {count} random prerequisite tracks, but only {} are eligible.",
            folders.len()
        ));
    }

    rng.shuffle(&mut folders);
    folders.truncate(count);
    Ok(folders)
}

fn count_eligible_tracks(
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
) -> usize {
    tracks
        .iter()
        .filter(|track| {
            excluded_track_folder
                .map(|excluded| !track.folder.eq_ignore_ascii_case(excluded))
                .unwrap_or(true)
        })
        .count()
}

fn track_folder_exists(folder: &str, tracks: &[RandomizedTrack]) -> bool {
    tracks
        .iter()
        .any(|track| track.folder.eq_ignore_ascii_case(folder))
}

fn is_custom_unlock_method(method: i32) -> bool {
    is_specific_custom_unlock_method(method) || is_count_custom_unlock_method(method)
}

fn is_specific_custom_unlock_method(method: i32) -> bool {
    SPECIFIC_CUSTOM_UNLOCK_METHODS.contains(&method)
}

fn is_count_custom_unlock_method(method: i32) -> bool {
    COUNT_CUSTOM_UNLOCK_METHODS.contains(&method)
}

fn make_row_label(prefix: &str, index: usize, id: &str) -> String {
    if id.is_empty() {
        format!("{prefix} slot {}", index + 1)
    } else {
        format!("{prefix} slot {} ({id})", index + 1)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::{HashMap, HashSet};

    fn randomized_car_with_obtain(folder: &str, obtain: i32) -> RandomizedCar {
        RandomizedCar {
            folder: folder.to_string(),
            rating: 0,
            obtain,
            selectable_player: true,
            selectable_cpu: true,
            custom_unlock: None,
        }
    }

    fn randomized_track(folder: &str) -> RandomizedTrack {
        randomized_track_with_obtain(folder, 0)
    }

    fn randomized_track_with_obtain(folder: &str, obtain: i32) -> RandomizedTrack {
        RandomizedTrack {
            folder: folder.to_string(),
            difficulty: 1,
            obtain,
            custom_unlock: None,
        }
    }

    fn car_spec(attr_obtain: &str, custom_unlock: Option<CustomUnlockSpec>) -> CarSpec {
        CarSpec {
            id: "slot-id".to_string(),
            source_pool: "Full Random".to_string(),
            source_rating: "Random".to_string(),
            source_obtain: "Random".to_string(),
            attr_rating: "Unchanged".to_string(),
            attr_obtain: attr_obtain.to_string(),
            custom_unlock,
        }
    }

    fn track_spec(attr_obtain: &str, custom_unlock: Option<CustomUnlockSpec>) -> TrackSpec {
        TrackSpec {
            id: "slot-id".to_string(),
            source_pool: "Full Random".to_string(),
            source_difficulty: "Random".to_string(),
            attr_difficulty: "Unchanged".to_string(),
            attr_obtain: attr_obtain.to_string(),
            custom_unlock,
        }
    }

    fn specific_unlock(method: &str, folders: &[&str]) -> CustomUnlockSpec {
        CustomUnlockSpec {
            method: method.to_string(),
            mode: Some(CustomUnlockTrackMode::SpecificTracks),
            track_folders: folders.iter().map(|folder| folder.to_string()).collect(),
            random_track_count: None,
            required_count: None,
        }
    }

    fn random_track_unlock(method: &str, count: i32) -> CustomUnlockSpec {
        CustomUnlockSpec {
            method: method.to_string(),
            mode: Some(CustomUnlockTrackMode::RandomTracks),
            track_folders: Vec::new(),
            random_track_count: Some(count),
            required_count: None,
        }
    }

    fn count_unlock(method: &str, count: i32) -> CustomUnlockSpec {
        CustomUnlockSpec {
            method: method.to_string(),
            mode: None,
            track_folders: Vec::new(),
            random_track_count: None,
            required_count: Some(count),
        }
    }

    fn car_options() -> CarOptionsInput {
        CarOptionsInput {
            unlock_mode: "random".to_string(),
            num_starting_cars: 0,
            enable_starting_cars_pool: false,
            starting_cars_pool: "Full Random".to_string(),
            enable_starting_cars_rating: false,
            starting_cars_rating: "Random".to_string(),
            include_cheat_only: false,
            include_stunt_arena: false,
            include_starting_car: true,
            include_championship: true,
            include_time_trial: true,
            include_practice_stars: true,
            include_single_race: true,
            include_specific_race_win: false,
            include_specific_practice_star: false,
            include_specific_time_trial: false,
            include_race_win_count: false,
            include_practice_star_count: false,
            include_time_trial_count: false,
            include_stunt_arena_star_count: false,
            specific_race_win_track_count_min: 1,
            specific_race_win_track_count_max: 1,
            specific_practice_star_track_count_min: 1,
            specific_practice_star_track_count_max: 1,
            specific_time_trial_track_count_min: 1,
            specific_time_trial_track_count_max: 1,
            race_win_count_min: 1,
            race_win_count_max: 14,
            practice_star_count_min: 1,
            practice_star_count_max: 14,
            time_trial_count_min: 1,
            time_trial_count_max: 14,
            stunt_arena_star_count_min: 1,
            stunt_arena_star_count_max: 20,
            include_super_pro: true,
            pool_rating_distributions: HashMap::new(),
            attr_rating_distributions: HashMap::new(),
        }
    }

    fn track_options() -> TrackOptionsInput {
        TrackOptionsInput {
            unlock_mode: "random".to_string(),
            include_stunt_arena: false,
            include_default: true,
            include_time_trial: true,
            include_practice: true,
            include_single_race: true,
            include_specific_race_win: false,
            include_specific_practice_star: false,
            include_specific_time_trial: false,
            include_race_win_count: false,
            include_practice_star_count: false,
            include_time_trial_count: false,
            include_stunt_arena_star_count: false,
            specific_race_win_track_count_min: 1,
            specific_race_win_track_count_max: 1,
            specific_practice_star_track_count_min: 1,
            specific_practice_star_track_count_max: 1,
            specific_time_trial_track_count_min: 1,
            specific_time_trial_track_count_max: 1,
            race_win_count_min: 1,
            race_win_count_max: 14,
            practice_star_count_min: 1,
            practice_star_count_max: 14,
            time_trial_count_min: 1,
            time_trial_count_max: 14,
            stunt_arena_star_count_min: 1,
            stunt_arena_star_count_max: 20,
        }
    }

    #[test]
    fn car_specific_custom_unlock_outputs_track_folder_array() {
        let mut cars = vec![randomized_car_with_obtain("car1", 6)];
        let specs = vec![car_spec("6", Some(specific_unlock("6", &["nhood1", "market1"])))];
        let tracks = vec![randomized_track("nhood1"), randomized_track("market1")];
        let options = car_options();
        let mut rng = Rng::new();

        apply_car_custom_unlocks(&mut cars, &specs, &tracks, &options, "Stock car", &mut rng).unwrap();

        let condition = cars[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders, vec!["nhood1", "market1"]);
        assert_eq!(condition.required_count, 0);
        assert!(condition.archipelago_item.is_empty());
    }

    #[test]
    fn track_specific_custom_unlock_outputs_track_folder_array() {
        let mut tracks = vec![
            randomized_track_with_obtain("market1", 7),
            randomized_track("nhood1"),
        ];
        let specs = vec![
            track_spec("7", Some(specific_unlock("7", &["nhood1"]))),
            track_spec("0", None),
        ];
        let options = track_options();
        let mut rng = Rng::new();

        apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap();

        let condition = tracks[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders, vec!["nhood1"]);
        assert_eq!(condition.required_count, 0);
    }

    #[test]
    fn count_custom_unlock_outputs_required_count() {
        let mut tracks = vec![randomized_track_with_obtain("market1", 10)];
        let specs = vec![track_spec("10", Some(count_unlock("10", 3)))];
        let options = track_options();
        let mut rng = Rng::new();

        apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap();

        let condition = tracks[0].custom_unlock.as_ref().unwrap();
        assert!(condition.track_folders.is_empty());
        assert_eq!(condition.required_count, 3);
    }

    #[test]
    fn random_track_count_resolves_prerequisite_tracks() {
        let mut cars = vec![randomized_car_with_obtain("car1", 8)];
        let specs = vec![car_spec("8", Some(random_track_unlock("8", 2)))];
        let tracks = vec![
            randomized_track("nhood1"),
            randomized_track("market1"),
            randomized_track("muse1"),
        ];
        let valid_folders: HashSet<String> = tracks.iter().map(|track| track.folder.clone()).collect();
        let options = car_options();
        let mut rng = Rng::new();

        apply_car_custom_unlocks(&mut cars, &specs, &tracks, &options, "Stock car", &mut rng).unwrap();

        let condition = cars[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders.len(), 2);
        assert!(condition.track_folders.iter().all(|folder| valid_folders.contains(folder)));
    }

    #[test]
    fn custom_unlock_obtain_rejects_missing_condition() {
        let mut cars = vec![randomized_car_with_obtain("car1", 9)];
        let specs = vec![car_spec("9", None)];
        let tracks = vec![randomized_track("nhood1")];
        let options = car_options();
        let mut rng = Rng::new();

        let error = apply_car_custom_unlocks(&mut cars, &specs, &tracks, &options, "Stock car", &mut rng)
            .unwrap_err();

        assert!(error.contains("custom unlock condition is missing"));
    }

    #[test]
    fn track_specific_custom_unlock_rejects_self_dependency() {
        let mut tracks = vec![
            randomized_track_with_obtain("market1", 6),
            randomized_track("nhood1"),
        ];
        let specs = vec![
            track_spec("6", Some(specific_unlock("6", &["market1"]))),
            track_spec("0", None),
        ];
        let options = track_options();
        let mut rng = Rng::new();

        let error = apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap_err();

        assert!(error.contains("cannot require the target track itself"));
    }

    #[test]
    fn random_track_count_rejects_impossible_prerequisite_count() {
        let mut tracks = vec![randomized_track_with_obtain("market1", 8)];
        let specs = vec![track_spec("8", Some(random_track_unlock("8", 1)))];
        let options = track_options();
        let mut rng = Rng::new();

        let error = apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap_err();

        assert!(error.contains("no eligible prerequisite tracks"));
    }

    #[test]
    fn random_resolved_car_specific_unlock_uses_option_track_count_range() {
        let mut cars = vec![randomized_car_with_obtain("car1", 6)];
        let specs = vec![car_spec("Random", None)];
        let tracks = vec![
            randomized_track("nhood1"),
            randomized_track("market1"),
            randomized_track("muse1"),
        ];
        let mut options = car_options();
        options.specific_race_win_track_count_min = 2;
        options.specific_race_win_track_count_max = 2;
        let valid_folders: HashSet<String> = tracks.iter().map(|track| track.folder.clone()).collect();
        let mut rng = Rng::new();

        apply_car_custom_unlocks(&mut cars, &specs, &tracks, &options, "Stock car", &mut rng).unwrap();

        let condition = cars[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders.len(), 2);
        assert!(condition.track_folders.iter().all(|folder| valid_folders.contains(folder)));
    }

    #[test]
    fn random_resolved_track_specific_unlock_excludes_target_track() {
        let mut tracks = vec![
            randomized_track_with_obtain("market1", 6),
            randomized_track("nhood1"),
            randomized_track("muse1"),
        ];
        let specs = vec![
            track_spec("Random", None),
            track_spec("0", None),
            track_spec("0", None),
        ];
        let mut options = track_options();
        options.specific_race_win_track_count_min = 2;
        options.specific_race_win_track_count_max = 2;
        let mut rng = Rng::new();

        apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap();

        let condition = tracks[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders.len(), 2);
        assert!(!condition.track_folders.iter().any(|folder| folder == "market1"));
    }

    #[test]
    fn random_resolved_track_specific_unlock_clamps_count_to_available_tracks() {
        let mut tracks = vec![
            randomized_track_with_obtain("market1", 6),
            randomized_track("nhood1"),
        ];
        let specs = vec![track_spec("Random", None), track_spec("0", None)];
        let mut options = track_options();
        options.specific_race_win_track_count_min = 3;
        options.specific_race_win_track_count_max = 3;
        let mut rng = Rng::new();

        apply_track_custom_unlocks(&mut tracks, &specs, &options, &mut rng).unwrap();

        let condition = tracks[0].custom_unlock.as_ref().unwrap();
        assert_eq!(condition.track_folders, vec!["nhood1"]);
    }
}
