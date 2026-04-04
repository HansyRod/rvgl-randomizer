import React, { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';

export default function LaunchTab({ 
  installPath, 
  scanResult, 
  generatedFilePath, 
  extraArgs, 
  setExtraArgs,
  extraPacks 
}) {
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
      
      packlist = [...autoPacks, ...selectedContentPacks, ...extraPacks];
    }

    try {
      const result = await invoke("launch_game", {
        rvglExePath: installPath,
        extraArgs: extraArgs,
        configPath: generatedFilePath,
        packlist: packlist
      });
      
      setLaunchStatus(`Game running! Process ID: ${result.pid}`);
    } catch (error) {
      console.error(error);
      setLaunchStatus(`Launch failed: ${error}`);
    } finally {
      setIsLaunching(false);
    }
  };

  return (
    <div className="tab-container">
      <div className="control-panel">
        <div className="control-group">
          <label style={{ fontWeight: "bold" }}>Command Line Arguments:</label>
          <input 
            type="text" 
            value={extraArgs} 
            onChange={(e) => setExtraArgs(e.target.value)}
            placeholder="e.g. -window -nointro"
            style={{ padding: "0.5rem", width: "100%", boxSizing: "border-box" }}
          />
          <small style={{ color: "var(--text-secondary)" }}>These flags are passed directly to rvgl.exe.</small>
        </div>

        <div className="launch-summary" style={{ marginTop: "1.5rem", padding: "1rem", backgroundColor: "var(--bg-secondary)", borderRadius: "8px", border: "1px solid var(--border-color)" }}>
          <h3 style={{ marginTop: 0, marginBottom: "1rem" }}>Launch Summary</h3>
          
          {!generatedFilePath ? (
            <p style={{ color: "var(--error-color, #c0392b)", margin: 0 }}>
              ⚠ You must Generate or Load an instance file first (see Randomize tab).
            </p>
          ) : (
            <ul style={{ listStyle: "none", padding: 0, margin: 0, display: "flex", flexDirection: "column", gap: "0.5rem" }}>
              <li><strong>Target Executable:</strong> {installPath.split(/[\\/]/).pop()}</li>
              <li><strong>Configuration:</strong> {generatedFilePath.split(/[\\/]/).pop()}</li>
              {scanResult.installType === 'launcher' && (
                <li><strong>Packlist:</strong> 2 Auto + {extraPacks.length} Extra + Selected Content</li>
              )}
            </ul>
          )}
        </div>

        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", marginTop: "2rem" }}>
          <button 
            className="primary" 
            onClick={handleLaunch} 
            disabled={!generatedFilePath || isLaunching}
            style={{ padding: "0.75rem 3rem", fontSize: "1.2rem", fontWeight: "bold" }}
          >
            {isLaunching ? "Launching..." : "▶ Launch Game"}
          </button>
          
          {launchStatus && (
            <p style={{ marginTop: "1rem", color: launchStatus.includes("failed") ? "var(--error-color, #c0392b)" : "var(--text-primary)" }}>
              {launchStatus}
            </p>
          )}
        </div>
      </div>
    </div>
  );
}