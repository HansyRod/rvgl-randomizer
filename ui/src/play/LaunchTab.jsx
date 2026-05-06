import { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { useAppContext } from '../AppProvider';
import SeedSummaryPanel from "../components/SeedSummaryPanel";

export default function LaunchTab({errors}) {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, generate, play } = state;
  
  // Destructure individual variables
  const { installPath, scanResult } = setup;
  const { generatedFilePath, profileName } = generate;
  const { extraArgs, extraPacks, runningPid } = play;

  const [launchStatus, setLaunchStatus] = useState("");
  const [isLaunching, setIsLaunching] = useState(false);

  const handleLaunch = async () => {
    setIsLaunching(true);
    setLaunchStatus("Injecting randomizer and launching...");
    
    let packlist = null;
    if (scanResult.installType === 'launcher') {
      const autoPacks = ["rvgl_win64", "rvgl_assets"];
      const selectedContentPacks = scanResult.contentPacks
        .filter(p => p.useCars || p.useTracks)
        .map(p => p.name);

      packlist = [...selectedContentPacks, ...extraPacks, ...autoPacks];
    }

    try {
      const result = await invoke("launch_game", {
        rvglExePath: installPath,
        extraArgs: extraArgs,
        configPath: generatedFilePath,
        packlist: packlist,
        profileName: profileName
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

  const isLaunchBtnDisabled = !generatedFilePath || isLaunching || errors.length > 0;

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
          {!generatedFilePath ? (
            <div style={{ padding: "1rem", backgroundColor: "var(--bg-secondary)", borderRadius: "8px", border: "1px solid var(--border-color)" }}>
              <p style={{ color: "var(--error-color, #c0392b)", margin: 0 }}>
                ⚠ You must Generate or Load an instance file first (see Randomize tab).
              </p>
            </div>
          ) : (
            <>
              <h3 style={{ marginTop: 0, marginBottom: "0.75rem" }}>Launch Summary</h3>
              <SeedSummaryPanel configPath={generatedFilePath} compact={true} />
            </>
          )}
        </div>
      </div>
    </div>
  );
}
