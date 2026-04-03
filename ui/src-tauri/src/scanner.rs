use regex::Regex;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum InstallType {
    Classic,
    Launcher,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct ScanResult {
    pub install_type: InstallType,
    pub cars: Option<Vec<Car>>,
    pub tracks: Option<Vec<Track>>,
    pub content_packs: Option<Vec<ContentPack>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct ContentPack {
    pub name: String,
    pub absolute_path: String,
    pub has_cars: bool,
    pub has_tracks: bool,
    pub use_cars: bool,
    pub use_tracks: bool,
    pub cars: Vec<Car>,
    pub tracks: Vec<Track>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Car {
    pub folder_name: String,
    pub name: String,
    pub rating: i32,
    pub obtain_method: i32,
    pub is_system_car: bool,
    pub has_valid_file: bool,
    pub carbox_filename: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Track {
    pub folder_name: String,
    pub name: String,
    pub has_reversed: bool,
    pub track_type: i32,
    pub difficulty: i32,
    pub has_valid_file: bool,
}

#[tauri::command]
pub fn scan_install(executable_path: String) -> Option<ScanResult> {
    let exe_path = Path::new(&executable_path);
    if !exe_path.exists() {
        return None;
    }
    // Path could be rvgl.exe or rvgl_win64 etc.
    let rvgl_root = exe_path.parent()?;

    // Check for Launcher pattern: rvgl_root grandparent is "packs" ?
    // Wait, if it's "packs/rvgl_win64/rvgl.exe", then parent is "rvgl_win64", grandparent is "packs".
    // Or if it's in a pack folder directly: it could be any pack. Let's check grandparent.
    let mut is_launcher = false;
    let mut packs_dir = None;

    if let Some(grandparent) = rvgl_root.parent() {
        if let Some(grandparent_name) = grandparent.file_name() {
            if grandparent_name.to_ascii_lowercase() == "packs" {
                is_launcher = true;
                packs_dir = Some(grandparent);
            }
        }
    }

    if is_launcher {
        if let Some(packs_path) = packs_dir {
            let mut content_packs = Vec::new();
            if let Ok(entries) = fs::read_dir(packs_path) {
                for entry in entries.flatten() {
                    let path = entry.path();
                    if path.is_dir() {
                        if let Some(name) = path.file_name().and_then(|n| n.to_str()) {
                            let cars_path = path.join("cars");
                            let levels_path = path.join("levels");

                            let has_cars = cars_path.is_dir();
                            let has_tracks = levels_path.is_dir();

                            let is_game_files = name.to_ascii_lowercase() == "game_files";
                            let mut use_cars = false;
                            let mut use_tracks = false;
                            let mut cars = Vec::new();
                            let mut tracks = Vec::new();

                            if is_game_files {
                                if has_cars {
                                    use_cars = true;
                                    cars = scan_cars_folder_sync(&cars_path);
                                }
                                if has_tracks {
                                    use_tracks = true;
                                    tracks = scan_levels_folder_sync(&levels_path);
                                }
                            }

                            content_packs.push(ContentPack {
                                name: name.to_string(),
                                absolute_path: path.to_string_lossy().to_string(),
                                has_cars,
                                has_tracks,
                                use_cars,
                                use_tracks,
                                cars,
                                tracks,
                            });
                        }
                    }
                }
            }

            return Some(ScanResult {
                install_type: InstallType::Launcher,
                cars: None,
                tracks: None,
                content_packs: Some(content_packs),
            });
        }
    }

    // Classic pattern
    let cars_path = rvgl_root.join("cars");
    let levels_path = rvgl_root.join("levels");

    let mut cars = Vec::new();
    let mut tracks = Vec::new();

    if cars_path.is_dir() {
        cars = scan_cars_folder_sync(&cars_path);
    }
    if levels_path.is_dir() {
        tracks = scan_levels_folder_sync(&levels_path);
    }

    Some(ScanResult {
        install_type: InstallType::Classic,
        cars: Some(cars),
        tracks: Some(tracks),
        content_packs: None,
    })
}

#[tauri::command]
pub fn scan_cars_folder(folder_path: String) -> Vec<Car> {
    scan_cars_folder_sync(Path::new(&folder_path))
}

#[tauri::command]
pub fn scan_levels_folder(folder_path: String) -> Vec<Track> {
    scan_levels_folder_sync(Path::new(&folder_path))
}

fn scan_cars_folder_sync(folder_path: &Path) -> Vec<Car> {
    let mut cars = Vec::new();
    let system_cars = [
        "wincar", "wincar2", "wincar3", "wincar4", "trolley", "ufo", "q", "misc",
    ];

    let dir = match fs::read_dir(folder_path) {
        Ok(d) => d,
        Err(_) => return cars,
    };

    for entry in dir {
        if let Ok(entry) = entry {
            let path = entry.path();
            if path.is_dir() {
                if let Some(folder_name_str) = path.file_name().and_then(|n| n.to_str()) {
                    let folder_name = folder_name_str.to_string();
                    let is_system_car = system_cars
                        .iter()
                        .any(|c| c.eq_ignore_ascii_case(&folder_name));
                    let params_path = path.join("parameters.txt");

                    let mut name = folder_name.clone();
                    // -2 represents 'Unknown' according to the enum values discussed
                    let mut rating = -2;
                    let mut obtain_method = -2;
                    let has_valid_file = params_path.is_file();

                    let mut carbox_filename = if path.join("carbox.bmp").is_file() {
                        Some("carbox.bmp".to_string())
                    } else if path.join("box.bmp").is_file() {
                        Some("box.bmp".to_string())
                    } else {
                        None
                    };

                    if has_valid_file {
                        if let Ok(contents) = fs::read_to_string(&params_path) {
                            for raw_line in contents.lines() {
                                let trimmed_full = raw_line.trim();

                                // Check for commented ;)TCARBOX config
                                if carbox_filename.is_none() && trimmed_full.starts_with(";)") {
                                    let stripped = trimmed_full.trim_start_matches(|c| c == ';' || c == ')').trim();
                                    if let Some(val) = parse_param_value(stripped, "TCARBOX") {
                                        carbox_filename = Some(val.trim_matches(|c| c == '"' || c == '\'').to_string());
                                    }
                                }

                                let line = trimmed_full.split(';').next().unwrap_or("").trim();
                                if line.is_empty() {
                                    continue;
                                }

                                if let Some(n) = parse_param_value(line, "Name") {
                                    name = n.trim_matches(|c| c == '"' || c == '\'').to_string();
                                } else if let Some(r) = parse_param_value(line, "Rating") {
                                    if let Ok(val) = r.parse::<i32>() {
                                        rating = val;
                                    }
                                } else if let Some(o) = parse_param_value(line, "Obtain") {
                                    if let Ok(val) = o.parse::<i32>() {
                                        obtain_method = val;
                                    }
                                }
                            }
                        }
                    }

                    cars.push(Car {
                        folder_name,
                        name,
                        rating,
                        obtain_method,
                        is_system_car,
                        has_valid_file,
                        carbox_filename,
                    });
                }
            }
        }
    }

    cars.sort_by(|a, b| a.rating.cmp(&b.rating).then(a.name.cmp(&b.name)));
    cars
}

fn scan_levels_folder_sync(folder_path: &Path) -> Vec<Track> {
    let mut tracks = Vec::new();
    let name_re = Regex::new(r#"(?i)\bNAME\b\s*(?:=|\s+)\s*(?:"([^"]+)"|'([^']+)'|(.+))"#).unwrap();

    let dir = match fs::read_dir(folder_path) {
        Ok(d) => d,
        Err(_) => return tracks,
    };

    for entry in dir {
        if let Ok(entry) = entry {
            let path = entry.path();
            if path.is_dir() {
                if let Some(folder_name_str) = path.file_name().and_then(|n| n.to_str()) {
                    let folder_name = folder_name_str.to_string();
                    let has_reversed = path.join("reversed").is_dir();
                    let inf_path = path.join(format!("{}.inf", folder_name));
                    let has_valid_file = inf_path.is_file();

                    let mut name = folder_name.clone();
                    let mut track_type = None;
                    let mut difficulty = 0;

                    if has_valid_file {
                        if let Ok(contents) = fs::read_to_string(&inf_path) {
                            for raw_line in contents.lines() {
                                let line = raw_line.split(';').next().unwrap_or("").trim();
                                if line.is_empty() {
                                    continue;
                                }

                                if line.to_uppercase().starts_with("NAME") {
                                    if let Some(caps) = name_re.captures(line) {
                                        if let Some(m) = caps.get(1) {
                                            name = m.as_str().trim().to_string();
                                        } else if let Some(m) = caps.get(2) {
                                            name = m.as_str().trim().to_string();
                                        } else if let Some(m) = caps.get(3) {
                                            name = m.as_str().trim().to_string();
                                        }
                                    }
                                } else if let Some(val) = parse_param_value(line, "DIFFICULTY") {
                                    if let Ok(d) = val.parse::<i32>() {
                                        difficulty = d;
                                    }
                                } else if let Some(val) = parse_param_value(line, "GAMETYPE") {
                                    if let Ok(t) = val.parse::<i32>() {
                                        track_type = Some(t);
                                    }
                                }
                            }
                        }
                    }

                    // Compute final track type with fallbacks
                    let final_track_type = match track_type {
                        Some(t) => t,
                        None => {
                            let lower_folder = folder_name.to_lowercase();
                            if ["bot_bat", "muse_bat", "nhood1_battle", "markar"]
                                .contains(&lower_folder.as_str())
                            {
                                1
                            } else if lower_folder == "stunts" {
                                2
                            } else if lower_folder == "frontend" {
                                3
                            } else {
                                0 // default race
                            }
                        }
                    };

                    tracks.push(Track {
                        folder_name,
                        name,
                        has_reversed,
                        track_type: final_track_type,
                        difficulty,
                        has_valid_file,
                    });
                }
            }
        }
    }

    tracks.sort_by(|a, b| a.name.cmp(&b.name));
    tracks
}

fn parse_param_value<'a>(line: &'a str, key: &str) -> Option<&'a str> {
    // Splits by ':' or '='
    let parts: Vec<&str> = line.splitn(2, |c| c == ':' || c == '=').collect();
    if parts.len() == 2 {
        if parts[0].trim().eq_ignore_ascii_case(key) {
            return Some(parts[1].trim());
        }
    } else {
        // Space separated
        let parts: Vec<&str> = line.split_whitespace().collect();
        if parts.len() >= 2 && parts[0].eq_ignore_ascii_case(key) {
            // Find the start index of the second token to preserve all trailing content
            let val_idx = line.find(parts[1]).unwrap();
            return Some(line[val_idx..].trim());
        }
    }
    None
}
