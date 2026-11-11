import { useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";
import { scanArchipelagoContent } from "./archipelagoContent";

export function useInstallScan() {
  const { state, updateCategoryCtx, updateArchipelagoCategoryCtx } = useAppContext();
  const { app, setup, archipelago } = state;
  const { mode, isScanning } = app;
  const { installPath, scanResult } = setup;
  const { scanResult: archipelagoScanResult, scanError } = archipelago.setup;

  async function scanInstall(path, existingScan = null) {
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = existingScan ?? await invoke("scan_install", { executablePath: path });
      let archipelagoScanResult = null;
      let scanError = "";
      if (mode === "archipelago") {
        try {
          archipelagoScanResult = await scanArchipelagoContent(result, path);
        } catch (error) {
          scanError = error instanceof Error ? error.message : String(error);
        }
      }
      updateArchipelagoCategoryCtx("setup", { scanResult: archipelagoScanResult, scanError });
      return result;
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  useEffect(() => {
    // An existing installation scan can be reused when entering Archipelago mode.
    if (mode === "archipelago" && installPath && scanResult && !archipelagoScanResult && !scanError && !isScanning) {
      scanInstall(installPath, scanResult).catch(console.error);
    }
    // Completed scans are kept in AppProvider even if Setup is left while scanning.
  }, [mode, installPath, scanResult, archipelagoScanResult, scanError, isScanning]);

  return scanInstall;
}
