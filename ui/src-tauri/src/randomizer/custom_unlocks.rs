use super::models::{
    CarSpec, CustomUnlockCondition, CustomUnlockSpec, CustomUnlockTrackMode, RandomizedCar,
    RandomizedTrack, TrackSpec,
};
use super::rng::Rng;

const SPECIFIC_CUSTOM_UNLOCK_METHODS: [i32; 3] = [6, 7, 8];
const COUNT_CUSTOM_UNLOCK_METHODS: [i32; 4] = [9, 10, 11, 12];

pub fn apply_car_custom_unlocks(
    cars: &mut [RandomizedCar],
    specs: &[CarSpec],
    tracks: &[RandomizedTrack],
    row_label_prefix: &str,
    rng: &mut Rng,
) -> Result<(), String> {
    for (index, (car, spec)) in cars.iter_mut().zip(specs.iter()).enumerate() {
        car.custom_unlock = build_custom_unlock_condition(
            &spec.attr_obtain,
            spec.custom_unlock.as_ref(),
            tracks,
            None,
            &make_row_label(row_label_prefix, index, &spec.id),
            rng,
        )?;
    }
    Ok(())
}

pub fn apply_track_custom_unlocks(
    tracks: &mut [RandomizedTrack],
    specs: &[TrackSpec],
    rng: &mut Rng,
) -> Result<(), String> {
    let track_pool = tracks.to_vec();
    for (index, (track, spec)) in tracks.iter_mut().zip(specs.iter()).enumerate() {
        track.custom_unlock = build_custom_unlock_condition(
            &spec.attr_obtain,
            spec.custom_unlock.as_ref(),
            &track_pool,
            Some(&track.folder),
            &make_row_label("Track", index, &spec.id),
            rng,
        )?;
    }
    Ok(())
}

fn build_custom_unlock_condition(
    attr_obtain: &str,
    custom_unlock: Option<&CustomUnlockSpec>,
    tracks: &[RandomizedTrack],
    excluded_track_folder: Option<&str>,
    row_label: &str,
    rng: &mut Rng,
) -> Result<Option<CustomUnlockCondition>, String> {
    let method = match attr_obtain.parse::<i32>() {
        Ok(method) => method,
        Err(_) => return Ok(None),
    };
    if !is_custom_unlock_method(method) {
        return Ok(None);
    }

    let custom_unlock = custom_unlock.ok_or_else(|| {
        format!("{row_label}: custom unlock condition is missing for obtain method {method}.")
    })?;
    if custom_unlock.method != attr_obtain {
        return Err(format!(
            "{row_label}: custom unlock condition method {} does not match obtain method {}.",
            custom_unlock.method,
            attr_obtain
        ));
    }

    if is_specific_custom_unlock_method(method) {
        return build_specific_track_condition(custom_unlock, tracks, excluded_track_folder, row_label, rng)
            .map(Some);
    }

    if is_count_custom_unlock_method(method) {
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

fn build_specific_track_condition(
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
