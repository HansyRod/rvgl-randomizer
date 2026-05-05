use crate::scanner::{ScanResult, Track};
use super::models::*;
use super::rng::Rng;
use std::collections::HashSet;

fn is_stock_track_folder(folder: &str) -> bool {
    matches!(
        folder.to_ascii_lowercase().as_str(),
        "nhood1" | "market2" | "muse2" | "garden1" | "roof" | "toylite" | "wild_west1" |
        "toy2" | "nhood2" | "ship1" | "muse1" | "market1" | "wild_west2" | "ship2"
    )
}

pub fn collect_available_tracks(scan: &ScanResult) -> Vec<Track> {
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

pub fn resolve_track_list(
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

pub fn resolve_track_difficulty(attr: &str, scanned: i32, slot_index: usize, mode: &str, rng: &mut Rng) -> i32 {
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

pub fn resolve_track_obtain(attr: &str, mode: &str, opts: &TrackOptionsInput, rng: &mut Rng) -> i32 {
    if mode == "randomDifficulty" || mode == "unchanged" || mode == "baseGame" {
        0
    } else if attr == "Random" {
        let mut allowed = vec![0, 1, 2, 3, 4];
        if opts.include_stunt_arena { allowed.push(5); }
        let i = rng.next_usize(allowed.len());
        allowed[i]
    } else {
        attr.parse::<i32>().unwrap_or(0)
    }
}

fn difficulty_index(difficulty: i32) -> Option<usize> {
    if (1..=4).contains(&difficulty) {
        Some((difficulty - 1) as usize)
    } else {
        None
    }
}

fn count_generated_difficulties(tracks: &[RandomizedTrack]) -> [usize; 4] {
    let mut counts = [0usize; 4];
    for track in tracks {
        if let Some(idx) = difficulty_index(track.difficulty) {
            counts[idx] += 1;
        }
    }
    counts
}

fn missing_difficulties(counts: &[usize; 4]) -> Vec<i32> {
    (1..=4)
        .filter(|difficulty| counts[(difficulty - 1) as usize] == 0)
        .collect()
}

fn slot_has_randomized_difficulty(spec: &TrackSpec, mode: &str) -> bool {
    if mode == "baseGame" || mode == "randomUnlock" || mode == "unchanged" {
        return false;
    }
    spec.attr_difficulty == "Random"
}

fn slot_uses_scanned_difficulty(spec: &TrackSpec, mode: &str) -> bool {
    if mode == "randomUnlock" || mode == "unchanged" {
        return true;
    }
    if mode == "baseGame" {
        return false;
    }
    spec.attr_difficulty == "Unchanged"
}

pub fn ensure_track_difficulty_coverage(
    tracks: &mut [RandomizedTrack],
    specs: &[TrackSpec],
    all_tracks: &[Track],
    scan: &ScanResult,
    mode: &str,
    rng: &mut Rng,
) {
    if tracks.len() < 4 || specs.is_empty() {
        return;
    }

    let mut counts = count_generated_difficulties(tracks);

    for missing in missing_difficulties(&counts) {
        let eligible: Vec<usize> = tracks.iter().enumerate()
            .filter_map(|(idx, track)| {
                if !slot_has_randomized_difficulty(&specs[idx], mode) {
                    return None;
                }
                let current_idx = difficulty_index(track.difficulty)?;
                (counts[current_idx] > 1).then_some(idx)
            })
            .collect();

        if eligible.is_empty() {
            continue;
        }

        let max_count = eligible.iter()
            .filter_map(|idx| difficulty_index(tracks[*idx].difficulty).map(|diff_idx| counts[diff_idx]))
            .max()
            .unwrap_or(0);

        let best: Vec<usize> = eligible.into_iter()
            .filter(|idx| {
                difficulty_index(tracks[*idx].difficulty)
                    .map(|diff_idx| counts[diff_idx] == max_count)
                    .unwrap_or(false)
            })
            .collect();

        let chosen_idx = best[rng.next_usize(best.len())];
        if let Some(old_idx) = difficulty_index(tracks[chosen_idx].difficulty) {
            counts[old_idx] -= 1;
        }
        tracks[chosen_idx].difficulty = missing;
        if let Some(new_idx) = difficulty_index(missing) {
            counts[new_idx] += 1;
        }
    }

    for missing in missing_difficulties(&counts) {
        let used_folders: HashSet<String> = tracks.iter().map(|track| track.folder.clone()).collect();
        let mut replacement_slots = Vec::new();

        for (idx, track) in tracks.iter().enumerate() {
            if !slot_uses_scanned_difficulty(&specs[idx], mode) {
                continue;
            }

            let Some(current_idx) = difficulty_index(track.difficulty) else {
                continue;
            };
            if counts[current_idx] <= 1 {
                continue;
            }

            let preferred: Vec<Track> = track_candidate_set(&specs[idx], all_tracks, scan)
                .into_iter()
                .filter(|candidate| {
                    candidate.difficulty == missing &&
                    (!used_folders.contains(&candidate.folder_name) || candidate.folder_name.eq_ignore_ascii_case(&track.folder))
                })
                .cloned()
                .collect();

            let fallback: Vec<Track> = if preferred.is_empty() {
                track_candidate_set(&specs[idx], all_tracks, scan)
                    .into_iter()
                    .filter(|candidate| candidate.difficulty == missing)
                    .cloned()
                    .collect()
            } else {
                Vec::new()
            };

            if !preferred.is_empty() || !fallback.is_empty() {
                replacement_slots.push((idx, preferred, fallback, counts[current_idx]));
            }
        }

        if replacement_slots.is_empty() {
            continue;
        }

        replacement_slots.sort_by(|a, b| b.3.cmp(&a.3));
        let (slot_idx, preferred, fallback, _) = &replacement_slots[0];
        let choices = if !preferred.is_empty() { preferred } else { fallback };
        let chosen_track = choices[rng.next_usize(choices.len())].clone();

        if let Some(old_idx) = difficulty_index(tracks[*slot_idx].difficulty) {
            counts[old_idx] -= 1;
        }
        tracks[*slot_idx].folder = chosen_track.folder_name;
        tracks[*slot_idx].difficulty = chosen_track.difficulty;
        if let Some(new_idx) = difficulty_index(chosen_track.difficulty) {
            counts[new_idx] += 1;
        }
    }
}
