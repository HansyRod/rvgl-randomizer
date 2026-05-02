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
        if opts.include_cheat_only { allowed.push(-1); }
        if opts.include_stunt_arena { allowed.push(5); }
        let i = rng.next_usize(allowed.len());
        allowed[i]
    } else {
        attr.parse::<i32>().unwrap_or(0)
    }
}
