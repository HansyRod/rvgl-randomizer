import { useMemo } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import { useAppContext } from "../AppProvider";
import { handleTogglePack } from "./packHelpers";
import "./SetupView.css";

// ── Install sub-tab ──────────────────────────────────────────────────────────

export default function InstallPanel({ onContinue }) {
  const { state, updateCategoryCtx } = useAppContext();
  const { app, setup } = state;
  const { isScanning, isFetchingPack } = app;
  const { installPath, scanResult } = setup;
  const installHistory = setup.installHistory || [];

  function updateHistory(path, installType) {
    const history = Array.isArray(setup.installHistory) ? setup.installHistory : [];
    const newHistory = history.filter(h => h.path !== path);
    newHistory.unshift({ path, installType });
    return newHistory.slice(0, 5);
  }

  const togglePack = (packIndex, type) => {
    handleTogglePack(packIndex, type, scanResult, updateCategoryCtx);
  };

  async function handleBrowse() {
    const selected = await open({
      multiple: false,
      directory: false,
      filters: [{ name: "Executable", extensions: ["exe", ""] }],
    });
    if (!selected) return;
    const path = selected.path || selected;
    updateCategoryCtx("setup", { installPath: path, scanResult: null });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: path });
      const history = updateHistory(path, result.installType);
      updateCategoryCtx("setup", { scanResult: result, installHistory: history });
    } catch (err) {
      console.error("Error scanning:", err);
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  async function handleSwitch(historyPath) {
    if (isScanning || historyPath === installPath) return;
    updateCategoryCtx("setup", { installPath: historyPath, scanResult: null });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: historyPath });
      const history = updateHistory(historyPath, result.installType);
      updateCategoryCtx("setup", { scanResult: result, installHistory: history });
    } catch (err) {
      console.error("Error switching:", err);
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  async function handleRefresh() {
    if (!installPath) return;
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: installPath });
      updateCategoryCtx("setup", { scanResult: result });
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
        <div className="install-badges">
          {isLauncher && <span className="install-badge badge-launcher">Launcher install</span>}
          {isClassic  && <span className="install-badge badge-classic">Classic install</span>}
          <span className="install-stat">{totalCars} cars found</span>
          <span className="install-stat">{totalTracks} tracks found</span>
        </div>
      )}

      {(installHistory.filter(h => h.path !== installPath).length > 0 || (scanResult && isLauncher)) && (
        <div className="setup-columns">
          {installHistory.filter(h => h.path !== installPath).length > 0 && (
            <div className="setup-section">
              <h3 className="setup-section-title">Previous installations</h3>
              <div className="install-history-list">
                {installHistory.filter(h => h.path !== installPath).map((h, i) => (
                  <div key={i} className="install-history-item">
                    <div className="install-history-details">
                      <span className="install-path-text" title={h.path}>{h.path}</span>
                      <span className={`install-badge badge-${h.installType}`}>{h.installType} install</span>
                    </div>
                    <button 
                      className="btn-secondary" 
                      onClick={() => handleSwitch(h.path)}
                      disabled={isScanning}
                    >
                      Switch
                    </button>
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>
      )}

      {scanResult && isLauncher && (
        <div className="setup-columns pack-columns">
          <div className="setup-section pack-list-box">
            <h3 className="setup-section-title" style={{textTransform: "capitalize", fontSize: "0.95rem", color: "var(--text-primary)"}}>Cars</h3>
            <p className="setup-hint">
              Choose which packs contribute cars to the randomization pool.
            </p>
            <div className="pack-list">
              {(scanResult.contentPacks || []).map((pack, i) => {
                if (!pack.hasCars && !(pack.cars && pack.cars.length > 0)) return null;
                return (
                  <div key={`car-${pack.name}`} className="pack-list-item">
                    <label className="pack-label">
                      <input 
                        type="checkbox" 
                        checked={!!pack.useCars} 
                        onChange={() => togglePack(i, "cars")}
                        disabled={isScanning || isFetchingPack}
                      />
                      <span>{pack.name}</span>
                    </label>
                    {(pack.cars !== undefined && pack.cars.length > 0) && (
                      <span className={`pack-chip ${pack.useCars ? "chip-active" : "chip-inactive"}`}>{pack.cars.length} cars</span>
                    )}
                  </div>
                );
              })}
            </div>
          </div>

          <div className="setup-section pack-list-box">
            <h3 className="setup-section-title" style={{textTransform: "capitalize", fontSize: "0.95rem", color: "var(--text-primary)"}}>Tracks</h3>
            <p className="setup-hint">
              Choose which packs contribute tracks to the randomization pool.
            </p>
            <div className="pack-list">
              {(scanResult.contentPacks || []).map((pack, i) => {
                if (!pack.hasTracks && !(pack.tracks && pack.tracks.length > 0)) return null;
                return (
                  <div key={`track-${pack.name}`} className="pack-list-item">
                    <label className="pack-label">
                      <input 
                        type="checkbox" 
                        checked={!!pack.useTracks} 
                        onChange={() => togglePack(i, "tracks")}
                        disabled={isScanning || isFetchingPack}
                      />
                      <span>{pack.name}</span>
                    </label>
                    {(pack.tracks !== undefined && pack.tracks.length > 0) && (
                      <span className={`pack-chip ${pack.useTracks ? "chip-active" : "chip-inactive"}`}>{pack.tracks.length} tracks</span>
                    )}
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      )}

      {scanResult && (
        <div className="setup-continue-row">
          <button className="btn-primary" onClick={onContinue}>
            Configure content →
          </button>
        </div>
      )}
    </div>
  );
}