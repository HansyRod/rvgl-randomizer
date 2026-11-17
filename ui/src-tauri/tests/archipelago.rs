use rvgl_randomizer_ui_lib::archipelago::extract_archipelago_file;
use std::fs::{self, File};
use std::io::Write;
use std::path::Path;
use zip::write::SimpleFileOptions;

#[test]
fn extracts_original_bytes_and_filename_without_parsing_manifest_or_json() {
    let root = std::env::temp_dir().join(format!(
        "rvgl-extract-{}-{}",
        std::process::id(),
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos()
    ));
    fs::create_dir(&root).unwrap();
    let path = root.join("RenamedDownload.aprvgl");
    let filename = "AP_123456_P2_Player.rvgl.json";
    let contents = b"Not validated as JSON: \xff\n";
    let mut archive = zip::ZipWriter::new(File::create(&path).unwrap());
    let options =
        SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
    archive.start_file("archipelago.json", options).unwrap();
    archive.write_all(b"Ignored manifest").unwrap();
    archive.start_file(filename, options).unwrap();
    archive.write_all(contents).unwrap();
    archive.finish().unwrap();
    let source = path.to_string_lossy().into_owned();
    let extracted = extract_archipelago_file(source.clone(), None).unwrap();
    assert_eq!(Path::new(&extracted), root.join(filename));
    assert_eq!(fs::read(&extracted).unwrap(), contents);
    assert!(!root.join("archipelago.json").exists());
    let chosen = root.join("chosen");
    fs::create_dir(&chosen).unwrap();
    let destination = chosen.join(filename).to_string_lossy().into_owned();
    assert_eq!(
        extract_archipelago_file(source, Some(destination.clone())).unwrap(),
        destination
    );
    assert_eq!(fs::read(&destination).unwrap(), contents);
    fs::remove_file(destination).unwrap();
    fs::remove_dir(chosen).unwrap();
    fs::remove_file(extracted).unwrap();
    fs::remove_file(path).unwrap();
    fs::remove_dir(root).unwrap();
}
