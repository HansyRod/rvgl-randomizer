import { useMemo } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import { useAppContext } from "../../AppProvider";
import "./SetupView.css";

// ── Install sub-tab ──────────────────────────────────────────────────────────

export default function InstallPanel({ onContinue }) {
  const { state, updateCategoryCtx } = useAppContext();
  const { app, install } = state;
  const { isScanning } = app;
  const { installPath, scanResult } = install;

  async function handleBrowse() {
    const selected = await open({
      multiple: false,
      directory: false,
      filters: [{ name: "Executable", extensions: ["exe", ""] }],
    });
    if (!selected) return;
    const path = selected.path || selected;
    updateCategoryCtx("install", { installPath: path, scanResult: null });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: path });
      updateCategoryCtx("install", { scanResult: result });
    } catch (err) {
      console.error("Error scanning:", err);
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  async function handleRefresh() {
    if (!installPath) return;
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: installPath });
      updateCategoryCtx("install", { scanResult: result });
    } catch (err) {
      console.error("Error refreshing:", err);
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  const isLauncher = scanResult?.installType === "launcher";
  const isClassic  = scanResult?.installType === "classic";

  const totalCars   = useMemo(() => {
    if (!scanResult) return 0;
    if (isClassic) return (scanResult.cars || []).filter(c => !c.isSystemCar && c.hasValidFile).length;
    return (scanResult.contentPacks || []).flatMap(p => p.cars).filter(c => !c.isSystemCar && c.hasValidFile).length;
  }, [scanResult, isClassic]);

  const totalTracks = useMemo(() => {
    if (!scanResult) return 0;
    if (isClassic) return (scanResult.tracks || []).filter(t => t.hasValidFile && t.trackType === 0).length;
    return (scanResult.contentPacks || []).flatMap(p => p.tracks).filter(t => t.hasValidFile && t.trackType === 0).length;
  }, [scanResult, isClassic]);

  return (
    <div className="setup-panel">
      <div className="setup-section">
        <h3 className="setup-section-title">RVGL executable</h3>
        <div className="install-path-row">
          <span className="install-path-text">{installPath || "No installation selected"}</span>
          <button onClick={handleBrowse} className="btn-primary" disabled={isScanning}>
            {isScanning ? "Scanning…" : installPath ? "Change" : "Browse"}
          </button>
          {installPath && (
            <button onClick={handleRefresh} className="btn-secondary" disabled={isScanning}>↺</button>
          )}
        </div>
      </div>

      {scanResult && (
        <>
          <div className="install-badges">
            {isLauncher && <span className="install-badge badge-launcher">Launcher install</span>}
            {isClassic  && <span className="install-badge badge-classic">Classic install</span>}
            <span className="install-stat">{totalCars} cars found</span>
            <span className="install-stat">{totalTracks} tracks found</span>
          </div>

          {isLauncher && (
            <div className="setup-section">
              <h3 className="setup-section-title">Content packs detected</h3>
              <p className="setup-hint">
                Visit the <strong>Cars</strong> and <strong>Tracks</strong> tabs to choose which packs contribute to your randomization pool.
              </p>
              <div className="pack-summary-grid">
                {(scanResult.contentPacks || []).map(pack => (
                  <div key={pack.name} className="pack-summary-card">
                    <span className="pack-summary-name">{pack.name}</span>
                    <span className="pack-summary-counts">
                      {pack.hasCars && <span>{pack.cars.length || "?"} cars</span>}
                      {pack.hasTracks && <span>{pack.tracks.length || "?"} tracks</span>}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          )}

          <div className="setup-continue-row">
            <button className="btn-primary" onClick={onContinue}>
              Configure content →
            </button>
          </div>
        </>
      )}
    </div>
  );
}