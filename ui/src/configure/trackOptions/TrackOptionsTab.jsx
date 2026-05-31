import "../carOptions/CarOptionsTab.css";
import { STOCK_TRACKS } from "../../utils/constants";
import { useAppContext } from "../../AppProvider";
import CustomUnlockMethodsTable from "../carOptions/CustomUnlockMethodsTable";
import { isEffectiveStockTracksMode } from "../../validation/stockMode";

function makeDefaultTrackSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceDifficulty: "Random",
    attrDifficulty: "Random",
    attrObtain: "Random",
  }));
}

function getBaseGameTrackDifficulty(slotIndex, slotCount) {
  if (slotCount === 13) {
    if (slotIndex < 4) return "1";
    if (slotIndex < 7) return "2";
    if (slotIndex < 10) return "3";
    return "4";
  }

  if (slotIndex < 4) return "1";
  if (slotIndex < 8) return "2";
  if (slotIndex < 11) return "3";
  return "4";
}

export default function TrackOptionsTab() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure } = state;
  
  // Destructure individual variables
  const { scanResult } = setup || {};
  const { trackOptions, trackSpecState, preset } = configure;

  const set = (key, value) => updateCategoryCtx("configure", { trackOptions: { ...trackOptions, [key]: value } });
  
  const {
    unlockMode,
    includeStuntArena,
    includeDefault,
    includeTimeTrial,
    includePractice,
    includeSingleRace,
  } = trackOptions;

  const isStockMode = isEffectiveStockTracksMode(scanResult, preset);
  const includeTracks = trackSpecState?.includeTracks !== false;

  const handleModeSelect = (modeId) => {

    const base = trackSpecState ?? {
      tracks: makeDefaultTrackSpec(STOCK_TRACKS),
    };
    const slotCount = base.tracks.length;

    const nextTracks = base.tracks.map((track, i) => {
      const out = { ...track };

      // Set obtain based on mode
      switch (modeId) {
        case "random":
        case "randomUnlock":
          out.attrObtain = "Random";
          break;
        case "unchanged":
        case "randomDifficulty":
        case "baseGame":
        default:
          out.attrObtain = "0";
          break;
      }

      // Set difficulty based on mode
      switch (modeId) {
        case "random":
        case "randomDifficulty":
          out.attrDifficulty = "Random";
          break;
        case "unchanged":
        case "randomUnlock":
          out.attrDifficulty = "Unchanged";
          break;
        case "baseGame":
          out.attrDifficulty = getBaseGameTrackDifficulty(i, slotCount);
          break;
        default:
          break;
      }

      return out;
    });

    updateCategoryCtx("configure", {
      trackSpecState: {
        ...trackSpecState, 
        tracks: nextTracks
      },
      trackOptions: {
        ...trackOptions,
        unlockMode: modeId
      }
    });
  };

  const cards = [
    { id: "random", label: "Full Random", desc: "Randomize both track difficulty and unlock method." },
    { id: "randomUnlock", label: "Random Unlock", desc: "Keep difficulty unchanged, randomize unlock method." },
    { id: "randomDifficulty", label: "Random Difficulty", desc: "Randomize difficulty, set unlock method to Default." },
    { id: "unchanged", label: "All Unchanged", desc: "Keep difficulty unchanged, set unlock method to Default." },
    { id: "baseGame", label: "Base Game Distribution", desc: `Apply 4 Easy, ${isStockMode ? 3 : 4} Medium, 3 Hard, 3 Extreme, use default unlocks.` },
  ];

  const showObtainControls = unlockMode === "random" || unlockMode === "randomUnlock";

  return (
    <div className="car-options-tab">
      <section className="co-section">
        <h2 className="co-section-title">Randomization Scope</h2>
        <p className="co-desc">
          Choose if tracks are included in the randomization.
        </p>
        <div className="co-checkbox-group">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={includeTracks}
              onChange={e => updateCategoryCtx("configure", {
                trackSpecState: {
                  ...trackSpecState,
                  includeTracks: e.target.checked
                }
              })}
            />
            <span>Randomize <strong>Tracks</strong></span>
          </label>
        </div>
      </section>

      <section className="co-section">
        <h2 className="co-section-title">Track Randomization</h2>
        <p className="co-desc">Controls how track difficulty and unlock methods are assigned.</p>
        <div className="co-mode-grid">
          {cards.map(mode => (
            <button
              key={mode.id}
              className={`co-mode-card${unlockMode === mode.id ? " selected" : ""}`}
              onClick={() => handleModeSelect(mode.id)}
            >
              <span className="co-mode-label">{mode.label}</span>
              <span className="co-mode-desc">{mode.desc}</span>
            </button>
          ))}
        </div>
      </section>

      {showObtainControls && (
        <section className="co-section">
          <h2 className="co-section-title">Allowed Unlock Methods</h2>
          <p className="co-desc">
            Choose which unlock methods are included in the random pool.
          </p>
          <div className="co-checkbox-group">
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeDefault}
                onChange={e => set("includeDefault", e.target.checked)}
              />
              <span>
                <strong>Default</strong> — track is unlocked by winning a championship cup.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeTimeTrial}
                onChange={e => set("includeTimeTrial", e.target.checked)}
              />
              <span>
                <strong>Time Trial</strong> — track is unlocked by beating a time trial.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includePractice}
                onChange={e => set("includePractice", e.target.checked)}
              />
              <span>
                <strong>Practice Stars</strong> — track is unlocked by collecting stars in practice mode.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeSingleRace}
                onChange={e => set("includeSingleRace", e.target.checked)}
              />
              <span>
                <strong>Single Race</strong> — track is unlocked by winning a single race.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeStuntArena}
                onChange={e => set("includeStuntArena", e.target.checked)}
              />
              <span>
                <strong>Stunt Arena</strong> — track is unlocked by completing the Stunt Arena.
              </span>
            </label>

            <CustomUnlockMethodsTable
              options={trackOptions}
              onChange={set}
            />
          </div>
        </section>
      )}
    </div>
  );
}
