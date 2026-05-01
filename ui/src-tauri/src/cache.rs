use std::fs;
use std::path::PathBuf;
use tauri::Manager;

/// Helper function to resolve the application's local data directory
fn get_cache_path(app_handle: &tauri::AppHandle) -> Result<PathBuf, String> {
    let mut path = app_handle
        .path()
        .app_local_data_dir()
        .map_err(|e| format!("Could not determine local data dir: {}", e))?;
    
    // Ensure the parent directory exists
    if !path.exists() {
        fs::create_dir_all(&path).map_err(|e| format!("Could not create data dir: {}", e))?;
    }
    
    path.push("cache.json");
    Ok(path)
}

#[tauri::command]
pub fn load_cache(app_handle: tauri::AppHandle) -> Result<serde_json::Value, String> {
    let path = get_cache_path(&app_handle)?;
    if !path.exists() {
        // Return an empty JSON object if no cache exists yet
        return Ok(serde_json::json!({}));
    }
    
    let contents = fs::read_to_string(&path).map_err(|e| format!("Failed to read cache: {}", e))?;
    let data: serde_json::Value = serde_json::from_str(&contents)
        .map_err(|e| format!("Failed to parse cache JSON: {}", e))?;
        
    Ok(data)
}

#[tauri::command]
pub fn save_cache(app_handle: tauri::AppHandle, data: serde_json::Value) -> Result<(), String> {
    let path = get_cache_path(&app_handle)?;
    let contents = serde_json::to_string_pretty(&data)
        .map_err(|e| format!("Failed to serialize cache: {}", e))?;
        
    fs::write(&path, contents).map_err(|e| format!("Failed to write cache: {}", e))?;
    Ok(())
}

#[tauri::command]
pub fn clear_cache(app_handle: tauri::AppHandle) -> Result<(), String> {
    let path = get_cache_path(&app_handle)?;
    if path.exists() {
        fs::remove_file(&path).map_err(|e| format!("Failed to delete cache file: {}", e))?;
    }
    Ok(())
}

#[tauri::command]
pub fn read_config_file(file_path: String) -> Result<serde_json::Value, String> {
    let contents = fs::read_to_string(&file_path)
        .map_err(|e| format!("Failed to read config file: {}", e))?;
        
    let data: serde_json::Value = serde_json::from_str(&contents)
        .map_err(|e| format!("Failed to parse config JSON: {}", e))?;
        
    Ok(data)
}

#[tauri::command]
pub fn write_config_file(file_path: String, data: serde_json::Value) -> Result<(), String> {
    let contents = serde_json::to_string_pretty(&data)
        .map_err(|e| format!("Failed to serialize config: {}", e))?;
        
    fs::write(&file_path, contents)
        .map_err(|e| format!("Failed to write config file: {}", e))?;
        
    Ok(())
}

#[tauri::command]
pub fn read_seed_context(file_path: String) -> Result<serde_json::Value, String> {
    let contents = fs::read_to_string(&file_path)
        .map_err(|e| format!("Cannot read file: {}", e))?;
    let json: serde_json::Value = serde_json::from_str(&contents)
        .map_err(|e| format!("Invalid JSON: {}", e))?;
    Ok(json["metadata"].clone())
}