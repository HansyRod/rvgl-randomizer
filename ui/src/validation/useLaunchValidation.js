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
            message: `The config file no longer exists at: ${generatedFilePath}. ` +
              `Generate a new seed or load a different file.`
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
          message: `randomizer.dll was not found. Run "npm run build:dll" to build it, ` +
            `then restart the app.`
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
            message: `Could not find cars/misc/carbox1.bmp in any enabled location. ` +
              `The car selection screen may show blank portraits.`
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
            message: `Cannot write the packlist file to: ${packsDir}. ` +
              `Check folder permissions before launching.`
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