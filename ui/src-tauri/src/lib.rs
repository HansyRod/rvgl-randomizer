pub mod scanner;
pub mod randomizer;
pub mod launcher;
pub mod cache; // Added cache module

use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            #[cfg(debug_assertions)]
            {
                if let Some(window) = app.get_webview_window("main") {
                    window.open_devtools();
                }
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            scanner::scan_install,
            scanner::scan_cars_folder,
            scanner::scan_levels_folder,
            scanner::check_profile_exists,
            randomizer::generate_result,
            launcher::launch_game,
            cache::load_cache,
            cache::save_cache,
            cache::clear_cache,
            cache::read_config_file,
            cache::write_config_file
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}