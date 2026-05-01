import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";

export function useLaunchValidation() {
  const { state } = useAppContext();
  const { generate, setup } = state;
  const { generatedFilePath, seedContext } = generate;
  const { installPath, scanResult } = setup;

  const [asyncErrors, setAsyncErrors] = useState([]);
  const [asyncWarnings, setAsyncWarnings] = useState([]);

  useEffect(() => {
    let cancelled = false;

    async function run() {
      const errors = [];
      const warnings = [];

      // CONFIG_FILE_MISSING
      if (generatedFilePath) {
        const exists = await invoke("check_file_exists", {
          filePath: generatedFilePath
        }).catch(() => false);

        if (!exists) {
          errors.push({
            id: "launch_config_missing",
            scope: "launch",
            message: "The selected seed file can no longer be found. Generate it again or load a different file."
          });
        }
      }

      // DLL_NOT_FOUND — resolve the expected path the same way launcher.rs does
      // We ask Rust to check the resource path since we can't resolve it from JS
      const dllExists = await invoke("check_dll_exists").catch(() => false);
      if (!dllExists) {
        errors.push({
          id: "launch_dll_missing",
          scope: "launch",
          message: "The randomizer component is missing, so the game cannot be launched from the app. Rebuild the app support files and restart the app."
        });
      }

      // MISSING_CARBOX_ASSETS
      if (installPath) {
        const carboxExists = await invoke("check_carbox_assets_exist", {
          executablePath: installPath
        }).catch(() => false);

        if (!carboxExists) {
          warnings.push({
            id: "launch_carbox_missing",
            scope: "launch",
            message: "A required car portrait image could not be found. Car portraits may appear blank in the selection screen."
          });
        }
      }

      // PACKLIST_NOT_WRITABLE
      if (installPath && scanResult?.installType === "launcher") {
        const exePath = installPath.replace(/\\/g, "/");
        const parts = exePath.split("/");
        // packs dir is 2 levels up from the exe: .../packs/<platform>/rvgl.exe
        const packsDir = parts.slice(0, -2).join("\\");

        const writable = await invoke("check_path_writable", {
          dirPath: packsDir
        }).catch(() => false);

        if (!writable) {
          errors.push({
            id: "launch_packlist_not_writable",
            scope: "launch",
            message: "The app cannot update the enabled content list for this RVGL install. Check folder permissions before launching."
          });
        }
      }

      // 5A: INSTALL TYPE MISMATCH
      if (seedContext?.setup?.installType && scanResult?.installType) {
        if (seedContext.setup.installType !== scanResult.installType) {
          warnings.push({
            id: "launch_install_mismatch",
            scope: "launch",
            message: `This seed was generated for a ${seedContext.setup.installType} install but your current install is ${scanResult.installType}. Launch may fail or produce unexpected results.`
          });
        }
      }

      // 5C: REQUIRED PACKS MISSING
      if (installPath && scanResult?.installType === "launcher" && seedContext?.setup?.requiredPacks) {
        const requiredPacks = seedContext.setup.requiredPacks;
        
        const allAvailablePacks = (scanResult.contentPacks || []).map(p => p.name);
        const currentlyEnabledPacks = (scanResult.contentPacks || [])
          .filter(p => p.useCars || p.useTracks)
          .map(p => p.name);
        
        const completelyMissingPacks = requiredPacks.filter(p => !allAvailablePacks.includes(p));
        const presentButDisabledPacks = requiredPacks.filter(p => allAvailablePacks.includes(p) && !currentlyEnabledPacks.includes(p));
        
        if (completelyMissingPacks.length > 0) {
          warnings.push({
            id: "launch_packs_not_found",
            scope: "launch",
            message: `The following packs are required by this seed but are completely missing from your RVGL installation: ${completelyMissingPacks.join(', ')}. Install them to ensure the seed works correctly.`
          });
        }
        
        if (presentButDisabledPacks.length > 0) {
          warnings.push({
            id: "launch_missing_packs",
            scope: "launch",
            message: `The following packs are required by this seed but are not enabled: ${presentButDisabledPacks.join(', ')}. Enable them in Setup before launching.`
          });
        }
      }

      // 5C: MISSING CONTENT FOLDERS
      if (seedContext?.generatedCarFolders || seedContext?.generatedTrackFolders) {
        const seedCarFolders = seedContext.generatedCarFolders || [];
        const seedTrackFolders = seedContext.generatedTrackFolders || [];
        
        let availableCarFolders = [];
        let availableTrackFolders = [];
        
        if (scanResult?.installType === "launcher") {
          const activePacks = scanResult.contentPacks || [];
          availableCarFolders = activePacks.filter(p => p.useCars).flatMap(p => (p.cars || []).map(c => c.folderName));
          availableTrackFolders = activePacks.filter(p => p.useTracks).flatMap(p => (p.tracks || []).map(t => t.folderName));
        } else {
          availableCarFolders = (scanResult?.cars || []).map(c => c.folderName);
          availableTrackFolders = (scanResult?.tracks || []).map(t => t.folderName);
        }
        
        const missingCars = seedCarFolders.some(f => !availableCarFolders.includes(f));
        const missingTracks = seedTrackFolders.some(f => !availableTrackFolders.includes(f));
        
        if (missingCars || missingTracks) {
          errors.push({
            id: "launch_missing_content",
            scope: "launch",
            message: "Some content referenced by this seed is missing from your current active install or loaded packs. Verify your RVGL installation is complete and the correct packs are enabled."
          });
        }
      }

      if (!cancelled) {
        setAsyncErrors(errors);
        setAsyncWarnings(warnings);
      }
    }

    run();
    return () => { cancelled = true; };
  }, [generatedFilePath, installPath, scanResult, seedContext]);

  return { errors: asyncErrors, warnings: asyncWarnings };
}