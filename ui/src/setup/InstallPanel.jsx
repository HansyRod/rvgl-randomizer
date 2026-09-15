import { useMemo, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import { useAppContext } from "../AppProvider";
import HistoryPanel from "../components/HistoryPanel";
import { handleTogglePack } from "./packHelpers";
import { formatInstallError } from "./installValidation";
import "./SetupView.css";

// ── Install sub-tab ──────────────────────────────────────────────────────────

export default function InstallPanel() {
  const { state, updateCategoryCtx } = useAppContext();
  const { app, setup } = state;
  const { isScanning, isFetchingPack } = app;
  const { installPath, scanResult, installError } = setup;
  const installHistory = setup.installHistory || [];
  const [failedPath, setFailedPath] = useState("");

  function updateHistory(path, installType) {
    const history = Array.isArray(setup.installHistory) ? setup.installHistory : [];
    const newHistory = history.filter(h => h.path !== path);
    newHistory.unshift({ path, installType });
    return newHistory;
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
    updateCategoryCtx("setup", { installError: "" });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: path });
      const history = updateHistory(path, result.installType);
      setFailedPath("");
      updateCategoryCtx("setup", {
        installPath: path,
        scanResult: result,
        installError: "",
        installHistory: history,
      });
    } catch (err) {
      console.error("Error scanning:", err);
      setFailedPath(path);
      updateCategoryCtx("setup", {
        installError: formatInstallError(err, !!scanResult),
      });
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  function handleRemoveHistory(pathToRemove) {
    const newHistory = installHistory.filter(h => h.path !== pathToRemove);
    updateCategoryCtx("setup", { installHistory: newHistory });
  }

  async function handleSwitch(historyPath) {
    if (isScanning || historyPath === installPath) return;
    updateCategoryCtx("setup", { installError: "" });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: historyPath });
      const history = updateHistory(historyPath, result.installType);
      setFailedPath("");
      updateCategoryCtx("setup", {
        installPath: historyPath,
        scanResult: result,
        installError: "",
        installHistory: history,
      });
    } catch (err) {
      console.error("Error switching:", err);
      setFailedPath(historyPath);
      updateCategoryCtx("setup", {
        installError: formatInstallError(err, !!scanResult),
        installHistory: installHistory.filter((entry) => entry.path !== historyPath),
      });
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  async function handleRefresh() {
    if (!installPath) return;
    updateCategoryCtx("setup", { installError: "" });
    updateCategoryCtx("app", { isScanning: true });
    try {
      const result = await invoke("scan_install", { executablePath: installPath });
      updateCategoryCtx("setup", { scanResult: result, installError: "" });
    } catch (err) {
      console.error("Error refreshing:", err);
      updateCategoryCtx("setup", {
        scanResult: null,
        installError: formatInstallError(err, false),
        installHistory: installHistory.filter((entry) => entry.path !== installPath),
      });
    } finally {
      updateCategoryCtx("app", { isScanning: false });
    }
  }

  const isLauncher = scanResult?.installType === "launcher";
  const isClassic  = scanResult?.installType === "classic";
  const previousInstallations = installHistory.filter((entry) => entry.path !== installPath);

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
          <span className="install-path-text">{installPath || failedPath || "No installation selected"}</span>
          <button onClick={handleBrowse} className="btn-primary btn-action-wide" disabled={isScanning}>
            {isScanning ? "Scanning…" : installPath ? "Change" : "Browse"}
          </button>
          {installPath && (
            <button onClick={handleRefresh} className="btn-secondary btn-icon" disabled={isScanning} title="Refresh">
              <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2.5" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
              </svg>
            </button>
          )}
        </div>
        {installError && (
          <p className="install-error" role="alert">{installError}</p>
        )}
      </div>

      {scanResult && (
        <div className="install-badges">
          {isLauncher && <span className="install-badge badge-launcher">Launcher install</span>}
          {isClassic  && <span className="install-badge badge-classic">Classic install</span>}
          <span className="install-stat">{totalCars} cars found</span>
          <span className="install-stat">{totalTracks} tracks found</span>
        </div>
      )}

      {(previousInstallations.length > 0 || (scanResult && isLauncher)) && (
        <div className="setup-columns">
          {previousInstallations.length > 0 && (
            <HistoryPanel
              title="Previous installations"
              items={installHistory}
              activeKey={installPath}
              getKey={(entry) => entry.path}
              getPrimaryText={(entry) => entry.path}
              getBadgeLabel={(entry) => `${entry.installType} install`}
              getBadgeClassName={(entry) => `install-badge badge-${entry.installType}`}
              actionLabel="Switch"
              onAction={(entry) => handleSwitch(entry.path)}
              onRemove={handleRemoveHistory}
              disabled={isScanning}
              summaryLabel="install locations"
            />
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
    </div>
  );
}