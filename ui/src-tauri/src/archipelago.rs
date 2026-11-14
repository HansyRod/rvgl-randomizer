use std::fs::File;
use std::io;
use std::path::{Path, PathBuf};

use serde_json::{json, Value};
use zip::ZipArchive;

#[tauri::command]
pub fn extract_archipelago_file(
    file_path: String,
    output_path: Option<String>,
) -> Result<String, Value> {
    let file = File::open(&file_path).map_err(|e| e.to_string())?;
    let mut archive = ZipArchive::new(file).map_err(|e| e.to_string())?;
    for index in 0..archive.len() {
        let mut entry = archive.by_index(index).map_err(|e| e.to_string())?;
        if !entry.is_file()
            || !entry
                .name()
                .map_err(|e| e.to_string())?
                .to_ascii_lowercase()
                .ends_with(".rvgl.json")
        {
            continue;
        }
        // Never use archive paths as extraction paths; preserve only the enclosed filename.
        let filename = entry
            .enclosed_name()
            .and_then(|path| path.file_name().map(|name| name.to_owned()))
            .ok_or("Invalid .rvgl.json filename in the archive.")?;
        let destination = output_path
            .as_deref()
            .map(PathBuf::from)
            .unwrap_or_else(|| Path::new(&file_path).with_file_name(&filename));
        let mut output = match File::create(&destination) {
            Ok(file) => file,
            Err(error)
                if matches!(
                    error.kind(),
                    io::ErrorKind::PermissionDenied | io::ErrorKind::ReadOnlyFilesystem
                ) =>
            {
                return Err(
                    json!({ "message": error.to_string(), "fileName": filename.to_string_lossy() }),
                );
            }
            Err(error) => return Err(error.to_string().into()),
        };
        io::copy(&mut entry, &mut output).map_err(|e| e.to_string())?;
        return Ok(destination.to_string_lossy().into_owned());
    }
    Err("The archive does not contain a .rvgl.json file.".into())
}

