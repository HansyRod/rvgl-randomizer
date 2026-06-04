use super::models::{
    CarSpec, CustomUnlockCondition, CustomUnlockSpec, CustomUnlockTrackMode, RandomizedCar,
    RandomizedTrack,
};
use super::rng::Rng;

const SPECIFIC_CUSTOM_UNLOCK_METHODS: [i32; 3] = [6, 7, 8];
const COUNT_CUSTOM_UNLOCK_METHODS: [i32; 4] = [9, 10, 11, 12];

pub fn apply_car_custom_unlocks(
    cars: &mut [RandomizedCar],
    specs: &[CarSpec],
    tracks: &[RandomizedTrack],
    rng: &mut Rng,
) {
    for (car, spec) in cars.iter_mut().zip(specs.iter()) {
        car.custom_unlock = build_custom_unlock_condition(
            &spec.attr_obtain,
            spec.custom_unlock.as_ref(),
            tracks,
            rng,
        );
    }
}

fn build_custom_unlock_condition(
    attr_obtain: &str,
    custom_unlock: Option<&CustomUnlockSpec>,
    tracks: &[RandomizedTrack],
    rng: &mut Rng,
) -> Option<CustomUnlockCondition> {
    let method = attr_obtain.parse::<i32>().ok()?;
    if !is_custom_unlock_method(method) {
        return None;
    }

    let custom_unlock = custom_unlock?;
    if custom_unlock.method != attr_obtain {
        return None;
    }

    if is_specific_custom_unlock_method(method) {
        return build_specific_track_condition(custom_unlock, tracks, rng);
    }

    if is_count_custom_unlock_method(method) {
        return Some(CustomUnlockCondition {
            track_folders: Vec::new(),
            required_count: custom_unlock.required_count.unwrap_or(0),
            archipelago_item: String::new(),
        });
    }

    None
}

fn build_specific_track_condition(
    custom_unlock: &CustomUnlockSpec,
    tracks: &[RandomizedTrack],
    rng: &mut Rng,
) -> Option<CustomUnlockCondition> {
    let track_folders = match custom_unlock.mode {
        Some(CustomUnlockTrackMode::SpecificTracks) => custom_unlock.track_folders.clone(),
        Some(CustomUnlockTrackMode::RandomTracks) => {
            let count = custom_unlock.random_track_count.unwrap_or(0).max(0) as usize;
            choose_random_track_folders(tracks, count, rng)
        }
        None if !custom_unlock.track_folders.is_empty() => custom_unlock.track_folders.clone(),
        None => Vec::new(),
    };

    Some(CustomUnlockCondition {
        track_folders,
        required_count: 0,
        archipelago_item: String::new(),
    })
}

fn choose_random_track_folders(
    tracks: &[RandomizedTrack],
    count: usize,
    rng: &mut Rng,
) -> Vec<String> {
    if count == 0 || tracks.is_empty() {
        return Vec::new();
    }

    let mut folders: Vec<String> = tracks.iter().map(|track| track.folder.clone()).collect();
    rng.shuffle(&mut folders);
    folders.truncate(count.min(folders.len()));
    folders
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
