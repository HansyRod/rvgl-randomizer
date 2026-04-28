import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";

export function useLaunchValidation() {
  const { state } = useAppContext();
  const { generate, setup } = state;
  const { generatedFilePath } = generate;
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

      if (!cancelled) {
        setAsyncErrors(errors);
        setAsyncWarnings(warnings);
      }
    }

    run();
    return () => { cancelled = true; };
  }, [generatedFilePath, installPath, scanResult?.installType]);

  return { errors: asyncErrors, warnings: asyncWarnings };
}