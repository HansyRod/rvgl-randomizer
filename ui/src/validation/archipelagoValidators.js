import { invoke } from "@tauri-apps/api/core";
import { getArchipelagoCategoryIds } from "../import/archipelagoSeed";
import { getArchipelagoPacks } from "../play/archipelagoPacks";
import { validateProfileName } from "./generateValidators";
import { formatValidationList } from "./validationUtils";

const CONTENT_LABELS = {
  stockCars: "Stock cars", stockTracks: "Stock tracks (including Stunt Arena)",
  dcCars: "Dreamcast cars", dcTracks: "Dreamcast track (Rooftops)",
};

export function validateArchipelago(setup, archipelago, isScanning) {
  const errors = [];
  const warnings = [];
  const { importedFilePath, seedMetadata } = archipelago.import;
  if (!importedFilePath) {
    errors.push({ id: "ap_file_required", scope: "import", message: "Import the .rvgl.json generated for your Archipelago player slot." });
  } else if (!seedMetadata || typeof seedMetadata.slotName !== "string" || !seedMetadata.slotName.trim()
    || typeof seedMetadata.goal !== "string" || !seedMetadata.goal.trim() || typeof seedMetadata.profileName !== "string"
    || typeof seedMetadata.includeDcCars !== "boolean" || typeof seedMetadata.includeDcTrack !== "boolean") {
    errors.push({ id: "ap_metadata_missing", scope: "import", message: "The selected Archipelago file has incomplete run information. Import it again." });
  } else {
    const profile = validateProfileName(seedMetadata.profileName);
    errors.push(...profile.errors.map(issue => ({ ...issue, id: `ap_${issue.id}`, scope: "import", message: `${issue.message} Import a corrected Archipelago file.` })));
    warnings.push(...profile.warnings.map(issue => ({ ...issue, id: `ap_${issue.id}`, scope: "import" })));
  }

  if (!setup.scanResult || isScanning) {
    return { errors, warnings };
  }
  const { scanResult, scanError } = archipelago.setup;
  if (scanError || !scanResult) {
    errors.push({ id: "ap_scan_failed", scope: "scan", message: scanError
      ? `Could not scan Archipelago content: ${scanError} Use Refresh in Setup to try again.`
      : "Archipelago content has not been scanned. Use Refresh in Setup." });
  } else {
    for (const id of getArchipelagoCategoryIds(seedMetadata)) {
      const category = scanResult.categories[id];
      if (category?.status !== "complete") {
        const missing = formatValidationList(category?.missingFolders);
        errors.push({ id: `ap_content_${id}`, scope: "scan", message: `Required Archipelago content is incomplete: ${CONTENT_LABELS[id]}.${missing ? ` Missing: ${missing}.` : ""} Install the required content and Refresh Setup.` });
      }
    }
  }
  return { errors, warnings };
}

export async function validateArchipelagoLaunch(setup, archipelago) {
  const errors = [];
  const discovery = archipelago.setup.scanResult;
  const metadata = archipelago.import.seedMetadata;
  if (!setup.installPath || !setup.scanResult || !discovery || !metadata) {
    return { errors };
  }
  try {
    // Check the primary files found by discovery; do not rescan assets or re-read the JSON.
    const files = getArchipelagoCategoryIds(metadata).flatMap(id => (discovery.categories[id].items || []).map(item => {
      const filename = id.endsWith("Cars") ? "parameters.txt" : `${item.folderName}.inf`;
      return `${item.source.path}/${item.folderName}/${filename}`;
    }));
    let directories = [];
    if (setup.scanResult.installType === "launcher") {
      const packsDir = setup.installPath.replace(/\\/g, "/").split("/").slice(0, -2).join("/");
      const { packlist } = getArchipelagoPacks(setup.scanResult, discovery, metadata, archipelago.play.extraPacks);
      directories = packlist.map(name => `${packsDir}/${name}`);
    }
    const missing = await invoke("check_missing_paths", { files, directories });
    if (missing.files.length > 0) {
      const folders = missing.files.map(path => path.replace(/\\/g, "/").split("/").at(-2));
      errors.push({ id: "ap_content_files_missing", scope: "scan", message: `Required Archipelago content is missing: ${formatValidationList(folders)}. Install the content and Refresh Setup.` });
    }
    if (missing.directories.length > 0) {
      const packs = missing.directories.map(path => path.replace(/\\/g, "/").split("/").pop());
      errors.push({ id: "ap_launch_packs_missing", scope: "scan", message: `Packs in the launch list could not be found: ${formatValidationList(packs)}. Check the installation and Refresh Setup.` });
    }
  } catch (error) {
    errors.push({ id: "ap_content_check_failed", scope: "scan", message: `Could not check required Archipelago content: ${error instanceof Error ? error.message : String(error)} Refresh Setup and try again.` });
  }
  return { errors };
}
