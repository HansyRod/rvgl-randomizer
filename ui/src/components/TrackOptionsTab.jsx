import { useEffect } from "react";
import "./CarOptionsTab.css";
import { STOCK_TRACKS } from "../utils/constants";

function makeDefaultTrackSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceDifficulty: "Random",
    attrDifficulty: "Random",
    attrObtain: "Random",
  }));
}

export default function TrackOptionsTab({
  specState,
  setSpecState,
  trackOptions,
  setTrackOptions,
}) {
  const set = (key, value) => setTrackOptions(prev => ({ ...prev, [key]: value }));
  const unlockMode = trackOptions.unlockMode;

  useEffect(() => {
    setSpecState(prev => {
      const base = prev ?? {
        includeTracks: true,
        tracks: makeDefaultTrackSpec(STOCK_TRACKS),
      };
      const nextTracks = (base.tracks || makeDefaultTrackSpec(STOCK_TRACKS)).map((track, i) => {
      const out = { ...track };
      if (unlockMode === "baseGame") {
        out.attrObtain = "1";
        if (i < 4) out.attrDifficulty = "1";
        else if (i < 8) out.attrDifficulty = "2";
        else if (i < 11) out.attrDifficulty = "3";
        else out.attrDifficulty = "4";
      } else if (unlockMode === "unchanged") {
        out.attrDifficulty = "Unchanged";
        out.attrObtain = "1";
      } else if (unlockMode === "randomUnlock") {
        out.attrDifficulty = "Unchanged";
        out.attrObtain = "Random";
      } else if (unlockMode === "randomDifficulty") {
        out.attrDifficulty = "Random";
        out.attrObtain = "1";
      } else {
        out.attrDifficulty = "Random";
        out.attrObtain = "Random";
      }
      return out;
    });
      return { ...base, tracks: nextTracks };
    });
  }, [unlockMode, setSpecState]);

  const cards = [
    { id: "random", label: "Full Random", desc: "Randomize both track difficulty and unlock method." },
    { id: "randomUnlock", label: "Random Unlock", desc: "Keep difficulty unchanged, randomize unlock method." },
    { id: "randomDifficulty", label: "Random Difficulty", desc: "Randomize difficulty, set unlock method to Championship." },
    { id: "unchanged", label: "All Unchanged", desc: "Keep difficulty unchanged, set unlock method to Championship." },
    { id: "baseGame", label: "Base Game Distribution", desc: "Apply 4 Easy, 4 Medium, 3 Hard, 3 Extreme, unlock by Championship." },
  ];

  const showObtainControls = unlockMode === "random" || unlockMode === "randomUnlock";

  return (
    <div className="car-options-tab">
      <section className="co-section">
        <h2 className="co-section-title">Track Randomization</h2>
        <p className="co-desc">Controls how track difficulty and unlock methods are assigned.</p>
        <div className="co-mode-grid">
          {cards.map(mode => (
            <button
              key={mode.id}
              className={`co-mode-card${unlockMode === mode.id ? " selected" : ""}`}
              onClick={() => set("unlockMode", mode.id)}
            >
              <span className="co-mode-label">{mode.label}</span>
              <span className="co-mode-desc">{mode.desc}</span>
            </button>
          ))}
        </div>
      </section>

      {showObtainControls && (
        <section className="co-section">
          <h2 className="co-section-title">Random Obtain Methods</h2>
          <p className="co-desc">
            If disabled, random obtain is replaced with Championship (1).
          </p>
          <div className="co-checkbox-group">
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={trackOptions.enableRandomObtainMethods}
                onChange={e => set("enableRandomObtainMethods", e.target.checked)}
              />
              <span>Enable random obtain methods</span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={trackOptions.includeCheatOnly}
                onChange={e => set("includeCheatOnly", e.target.checked)}
              />
              <span>Include <strong>Cheat Only</strong> <span className="co-tag">-1</span></span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={trackOptions.includeUnlockedByDefault}
                onChange={e => set("includeUnlockedByDefault", e.target.checked)}
              />
              <span>Include <strong>Unlocked by Default</strong> <span className="co-tag">0</span></span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={trackOptions.includeStuntArena}
                onChange={e => set("includeStuntArena", e.target.checked)}
              />
              <span>Include <strong>Stunt Arena</strong> <span className="co-tag">5</span></span>
            </label>
          </div>
        </section>
      )}
    </div>
  );
}
