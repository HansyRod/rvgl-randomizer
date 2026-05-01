import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";
import "./SeedSummaryPanel.css";

function formatDate(isoString) {
  if (!isoString) return "Unknown date";
  const d = new Date(isoString);
  return d.toLocaleDateString(undefined, { 
    day: 'numeric', month: 'short', year: 'numeric',
    hour: '2-digit', minute: '2-digit'
  });
}

function getUnlockModeLabel(modeId) {
  switch (modeId) {
    case "random": return "Full Random";
    case "randomRatings": return "Random Ratings";
    case "randomUnlock": return "Random Unlock";
    case "baseGame": return "Base Game Distribution";
    case "unchanged": return "Unchanged";
    default: return modeId || "Unknown";
  }
}

function getStageModeLabel(modeId) {
  switch (modeId) {
    case "default": return "Default Stages";
    case "randomLaps": return "Random Laps";
    case "randomTracks": return "Random Tracks";
    case "fullRandom": return "Full Random";
    default: return modeId || "Unknown";
  }
}

export default function SeedSummaryPanel({ configPath, seedMetadata, compact }) {
  const { state } = useAppContext();
  const { scanResult } = state.setup;
  
  const [metadata, setMetadata] = useState(seedMetadata || null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  useEffect(() => {
    let cancelled = false;
    
    if (seedMetadata) {
      setMetadata(seedMetadata);
      return;
    }
    
    if (!configPath) {
      setMetadata(null);
      return;
    }

    async function loadData() {
      setLoading(true);
      setError(null);
      try {
        const data = await invoke("read_seed_context", { filePath: configPath });
        if (!cancelled) {
          setMetadata(data);
        }
      } catch (err) {
        if (!cancelled) {
          console.error("Failed to load seed summary:", err);
          setError("Could not read seed file.");
        }
      } finally {
        if (!cancelled) setLoading(false);
      }
    }

    loadData();
    return () => { cancelled = true; };
  }, [configPath, seedMetadata]);

  if (!configPath && !seedMetadata) return null;
  if (loading) return <div className="seed-summary-panel loading">Loading summary...</div>;
  if (error || !metadata || !metadata.uiContext) {
    return (
      <div className="seed-summary-panel error">
        <p>No preview available for this file.</p>
        {configPath && <span className="file-path">{configPath.split(/[\\/]/).pop()}</span>}
      </div>
    );
  }

  const { uiContext, profileName } = metadata;
  const { configure, setup, generatedAt } = uiContext;
  
  const carUnlockMode = configure?.carOptions?.unlockMode;
  const carMode = getUnlockModeLabel(carUnlockMode);
  const isCarRandomized = carUnlockMode !== "unchanged";
  
  const trackUnlockMode = configure?.trackOptions?.unlockMode;
  const trackMode = getUnlockModeLabel(trackUnlockMode);
  const isTrackRandomized = trackUnlockMode !== "unchanged";
  
  const cupsEnabled = configure?.cupSpecState?.enabled;
  const cupStageMode = getStageModeLabel(configure?.cupSpecState?.stageMode);
  
  const lapsMin = configure?.cupSpecState?.numLapsMin ?? "?";
  const lapsMax = configure?.cupSpecState?.numLapsMax ?? "?";
  const cupLaps = lapsMin === lapsMax ? `${lapsMin}` : `${lapsMin}-${lapsMax}`;
  
  const requiredPacks = setup?.requiredPacks || [];
  
  // Check packs against current scanResult
  const currentlyEnabledPacks = (scanResult?.contentPacks || [])
    .filter(p => p.useCars || p.useTracks)
    .map(p => p.name);

  const filename = configPath ? configPath.split(/[\\/]/).pop() : "Preview";

  return (
    <div className={`seed-summary-panel ${compact ? 'compact' : ''}`}>
      <div className="summary-header">
        <div className="summary-title">
          <span className="icon">📋</span>
          <span className="filename">{filename}</span>
        </div>
        <div className="summary-meta">
          Generated: {formatDate(generatedAt)} &middot; Profile: {profileName || "player1"}
        </div>
      </div>

      <div className="summary-body">
        <div className="summary-section">
          <div className="section-label">CARS</div>
          <div className="section-content">
            {isCarRandomized ? `Randomized (${carMode})` : "Unchanged"}
          </div>
        </div>

        <div className="summary-section">
          <div className="section-label">TRACKS</div>
          <div className="section-content">
            {isTrackRandomized ? `Randomized (${trackMode})` : "Unchanged"}
          </div>
        </div>

        {!compact && (
          <div className="summary-section">
            <div className="section-label">CUPS</div>
            <div className="section-content">
              {cupsEnabled ? "Randomized" : "Unchanged"}
              {cupsEnabled && <span className="separator">|</span>}
              {cupsEnabled && <span className="mode">Mode: {cupStageMode}</span>}
              {cupsEnabled && <div className="sub-content">{cupLaps} laps each</div>}
            </div>
          </div>
        )}
      </div>

      {requiredPacks.length > 0 && (
        <div className="summary-footer">
          <div className="section-label">REQUIRES</div>
          <div className="pack-list">
            {requiredPacks.map(pack => {
              const hasPack = currentlyEnabledPacks.includes(pack);
              return (
                <span key={pack} className={`pack-badge ${hasPack ? 'valid' : 'invalid'}`}>
                  {pack} {hasPack ? '✓' : '✗'}
                </span>
              );
            })}
          </div>
        </div>
      )}
    </div>
  );
}
