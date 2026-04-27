use tauri::Manager;

#[tauri::command]
pub fn check_file_exists(file_path: String) -> bool {
    std::path::Path::new(&file_path).is_file()
}

#[tauri::command]
pub fn check_path_writable(dir_path: String) -> bool {
    use std::fs;
    let path = std::path::Path::new(&dir_path);
    // Try to create a temp file in the directory to verify write access.
    let test_path = path.join(".rvgl_write_test");
    match fs::write(&test_path, b"") {
        Ok(_) => {
            let _ = fs::remove_file(&test_path);
            true
        }
        Err(_) => false,
    }
}

#[tauri::command]
pub fn check_carbox_assets_exist(executable_path: String) -> bool {
    let exe_path = std::path::Path::new(&executable_path);
    let rvgl_root = match exe_path.parent() {
        Some(p) => p,
        None => return false,
    };

    // Classic install: check directly under root
    if rvgl_root.join("cars").join("misc").join("carbox1.bmp").is_file() {
        return true;
    }

    // Launcher install: check in all sibling pack directories
    if let Some(packs_dir) = rvgl_root.parent() {
        if packs_dir.file_name().map(|n| n.to_ascii_lowercase()) ==
            Some(std::ffi::OsString::from("packs")) {
            if let Ok(entries) = std::fs::read_dir(packs_dir) {
                for entry in entries.flatten() {
                    let candidate = entry.path()
                        .join("cars").join("misc").join("carbox1.bmp");
                    if candidate.is_file() {
                        return true;
                    }
                }
            }
        }
    }

    false
}

#[tauri::command]
pub fn check_dll_exists(app_handle: tauri::AppHandle) -> bool {
    let mut dll_path: std::path::PathBuf = match app_handle
        .path()
        .resolve("resources/randomizer.dll", tauri::path::BaseDirectory::Resource)
    {
        Ok(path) => path,
        Err(_) => return false,
    };

    if !dll_path.exists() && cfg!(debug_assertions) {
        let source_path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("resources")
            .join("randomizer.dll");
        if source_path.exists() {
            dll_path = source_path;
        }
    }

    dll_path.is_file()
}
