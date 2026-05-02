import "../carOptions/CarOptionsTab.css";
import { STOCK_TRACKS } from "../../utils/constants";
import { useAppContext } from "../../AppProvider";

function makeDefaultTrackSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceDifficulty: "Random",
    attrDifficulty: "Random",
    attrObtain: "Random",
  }));
}

export default function TrackOptionsTab() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { configure } = state;
  
  // Destructure individual variables
  const { trackOptions, trackSpecState } = configure;

  const set = (key, value) => updateCategoryCtx("configure", { trackOptions: { ...trackOptions, [key]: value } });
  
  const { unlockMode, includeCheatOnly, includeStuntArena } = trackOptions;
  const includeTracks = trackSpecState?.includeTracks !== false;

  const handleModeSelect = (modeId) => {

    const base = trackSpecState ?? {
      tracks: makeDefaultTrackSpec(STOCK_TRACKS),
    };

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
          if (i < 4) {
            out.attrDifficulty = "1"; // Easy first 4 tracks
          }
          else if (i < 8) {
            out.attrDifficulty = "2"; // Medium next 4 tracks
          }
          else if (i < 11) {
            out.attrDifficulty = "3"; // Hard next 3 tracks
          }
          else {
            out.attrDifficulty = "4"; // Extreme last 3 tracks
          }
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
    { id: "baseGame", label: "Base Game Distribution", desc: "Apply 4 Easy, 4 Medium, 3 Hard, 3 Extreme, use default unlocks." },
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
            Standard methods (Default, Championship, Time Trial, Practice, Single Race) are always in the pool.
            Enable the options below to include additional unlock types.
          </p>
          <div className="co-checkbox-group">
            <label className="co-checkbox-row">
              <input type="checkbox" checked={includeCheatOnly} onChange={e => set("includeCheatOnly", e.target.checked)} />
              <span>Include <strong>Cheat Only</strong> — tracks are permanently locked without cheat codes.</span>
            </label>
            <label className="co-checkbox-row">
              <input type="checkbox" checked={includeStuntArena} onChange={e => set("includeStuntArena", e.target.checked)} />
              <span>Include <strong>Stunt Arena</strong> — tracks unlocked via Stunt Arena completion.</span>
            </label>
          </div>
        </section>
      )}
    </div>
  );
}
