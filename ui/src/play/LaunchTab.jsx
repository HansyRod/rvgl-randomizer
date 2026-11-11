import { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { useAppContext } from '../AppProvider';
import SeedSummaryPanel from "../components/SeedSummaryPanel";
import { getAutoPacks } from "../setup/packHelpers";
import { getArchipelagoPacks } from "./archipelagoPacks";

export default function LaunchTab({errors}) {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { app, setup, generate, play, archipelago } = state;
  const isArchipelago = app.mode === "archipelago";
  
  // Destructure individual variables
  const { installPath, scanResult } = setup;
  const { generatedFilePath, profileName } = generate;
  const { extraArgs, extraPacks, runningPid } = play;
  const { importedFilePath, seedMetadata } = archipelago.import;
  const configPath = isArchipelago ? importedFilePath : generatedFilePath;
  const launchProfileName = isArchipelago ? seedMetadata?.profileName : profileName;

  const [launchStatus, setLaunchStatus] = useState("");
  const [isLaunching, setIsLaunching] = useState(false);

  const handleLaunch = async () => {
    setIsLaunching(true);
    setLaunchStatus("Injecting randomizer and launching...");
    
    let packlist = null;
    if (scanResult.installType === 'launcher') {
      if (isArchipelago) {
        packlist = getArchipelagoPacks(scanResult, archipelago.setup.scanResult, seedMetadata, archipelago.play.extraPacks).packlist;
      } else {
        const autoPacks = getAutoPacks(scanResult);
        const selectedContentPacks = scanResult.contentPacks
          .filter(p => p.useCars || p.useTracks)
          .map(p => p.name);

        packlist = [...selectedContentPacks, ...extraPacks, ...autoPacks];
      }
    }

    try {
      const result = await invoke("launch_game", {
        rvglExePath: installPath,
        extraArgs: extraArgs,
        configPath: configPath,
        packlist: packlist,
        profileName: launchProfileName
      });
      const { pid } = result;
      updateCategoryCtx("play", { runningPid: pid });
      setLaunchStatus("");
    } catch (error) {
      console.error(error);
      setLaunchStatus(`Launch failed: ${error}`);
    } finally {
      setIsLaunching(false);
    }
  };

  const isLaunchBtnDisabled = !configPath || !launchProfileName || isLaunching || errors.length > 0
    || (isArchipelago && (app.isScanning || !archipelago.setup.scanResult));

  return (
    <div className="tab-container launch-tab">
      <div className="control-panel">
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", marginBottom: "2rem" }}>
          <button 
            className="primary launch-btn core-button"
            onClick={handleLaunch} 
            disabled={isLaunchBtnDisabled}
          >
            {isLaunching ? "Launching..." : "▶ Launch Game"}
          </button>
          
          {runningPid > 0 ? (
            <p style={{ marginTop: "1rem", marginBottom: 0, color: "var(--text-primary)" }}>
              Game Running!
            </p>
          ) : launchStatus && (
            <p style={{ marginTop: "1rem", marginBottom: 0, color: launchStatus.includes("failed") ? "var(--error-color, #c0392b)" : "var(--text-primary)" }}>
              {launchStatus}
            </p>
          )}
        </div>

        <div className="control-group">
          <label style={{ fontWeight: "bold" }}>Command Line Arguments:</label>
          <input 
            type="text" 
            value={extraArgs} 
            onChange={(e) => updateCategoryCtx("play", { extraArgs: e.target.value })}
            placeholder="e.g. -window -nointro"
            style={{ padding: "0.5rem", width: "100%", boxSizing: "border-box" }}
          />
          <small style={{ color: "var(--text-secondary)" }}>These flags are passed directly to rvgl.exe.</small>
        </div>

        <div style={{ marginTop: "1.5rem" }}>
          {!configPath ? (
            <div style={{ padding: "1rem", backgroundColor: "var(--bg-secondary)", borderRadius: "8px", border: "1px solid var(--border-color)" }}>
              <p style={{ color: "var(--error-color, #c0392b)", margin: 0 }}>
                {isArchipelago ? "⚠ Import an Archipelago file first (see Import)." : "⚠ You must Generate or Load an instance file first (see Randomize tab)."}
              </p>
            </div>
          ) : (
            <>
              <h3 style={{ marginTop: 0, marginBottom: "0.75rem" }}>Launch Summary</h3>
              <SeedSummaryPanel configPath={configPath} seedMetadata={isArchipelago ? seedMetadata : undefined} mode={app.mode} compact={!isArchipelago} />
            </>
          )}
        </div>
      </div>
    </div>
  );
}
