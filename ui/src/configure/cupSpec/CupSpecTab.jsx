import { useCallback } from "react";
import "../carOptions/CarOptionsTab.css";
import "./CupSpecTab.css";
import { useAppContext } from "../../AppProvider";
import {
  DEFAULT_POINTS,
  DEFAULT_CARS_PER_CLASS,
  STAGE_MODES,
  SAME_TRACK_OPTIONS,
  PointsTableEditor,
} from "./CupUtils";

// ─── Default state factory ────────────────────────────────────────────────────

export function makeDefaultCupSpec(index) {
  return {
    index,
    // Per-field override flags (each independent)
    overrideStageMode: false,
    overrideNumCars: false,
    overrideCarsPerClass: false,
    overrideNumTries: false,
    overridePerRacePlace: false,
    overrideOverallPlace: false,
    overridePointsTable: false,
    // Per-cup values (used when the corresponding override flag is true)
    stageMode: "default",
    numStagesMin: 3,
    numStagesMax: 6,
    numLapsMin: 2,
    numLapsMax: 8,
    stages: [],
    numCars: 8,
    numTries: 3,
    perRaceRequiredPlace: 3,
    overallRequiredPlace: 1,
    pointsTable: [...DEFAULT_POINTS],
    carsPerClass: [...DEFAULT_CARS_PER_CLASS[index]],
  };
}

export function makeDefaultCupSpecState() {
  return {
    enabled: true,
    stageMode: "default",
    guaranteeFirstNormal: true,
    sameTrackHandling: "forbid",
    allowReverse: true,
    allowMirror: false,
    allowReverseMirror: false,
    numCars: 8,
    numTries: 3,
    perRaceRequiredPlace: 3,
    overallRequiredPlace: 1,
    pointsTable: [...DEFAULT_POINTS],
    numLapsMin: 2,
    numLapsMax: 8,
    numStagesMin: 3,
    numStagesMax: 6,
    cups: [0, 1, 2, 3].map(makeDefaultCupSpec),
  };
}



export default function CupSpecTab() {

  const { state, updateCategoryCtx } = useAppContext();

  const { configure } = state;
  
  const { cupSpecState } = configure;

  const set = useCallback((key, val) => {
    updateCategoryCtx("configure", { cupSpecState: { ...cupSpecState, [key]: val } });
  }, [cupSpecState, updateCategoryCtx]);

  const stageMode = cupSpecState.stageMode;
  const userDefinedMode = stageMode === "userDefined";

  const globalNumCars = cupSpecState.numCars;
  const globalLapsMin = cupSpecState.numLapsMin;
  const globalLapsMax = cupSpecState.numLapsMax;
  const globalNumStagesMin = cupSpecState.numStagesMin;
  const globalNumStagesMax = cupSpecState.numStagesMax;

  return (
    <div className="car-options-tab cup-spec-tab">

      {/* ── Enable / disable cups ── */}
      { /* Forcing cup randomization to be always enabled since the default
        // cups will fail if the player changes the track specification.
      
      <section className="co-section">
        <h2 className="co-section-title">Cup Randomization</h2>
        <label className="co-checkbox-row">
          <input
            type="checkbox"
            checked={cupSpecState.enabled}
            onChange={e => set("enabled", e.target.checked)}
          />
          <span>Include cups in randomization</span>
        </label>
        {!cupSpecState.enabled && (
          <p className="co-desc" style={{ marginTop: "0.5rem" }}>
            Cup randomization is disabled. 
          </p>
        )}
      </section>

      */ }

      {cupSpecState.enabled && (
        <>
          {/* ── Track dependency info ── */}
          <div className="cup-info-banner">
            <strong>ℹ Track dependency:</strong> Cup stages can only use the 14 tracks
            allocated by the Track Spec. If &quot;Include Tracks in Randomization&quot; is off,
            the original 14 stock tracks are the pool. Configure tracks in the{" "}
            <strong>Track Spec</strong> tab first to control which tracks are available here.
          </div>

          {/* ── Stage mode ── */}
          <section className="co-section">
            <h2 className="co-section-title">Stage Mode</h2>
            <p className="co-desc">How stages are assigned to each cup.</p>
            <div className="co-mode-grid">
              {STAGE_MODES.map(mode => (
                <button
                  key={mode.id}
                  className={`co-mode-card${stageMode === mode.id ? " selected" : ""}`}
                  onClick={() => set("stageMode", mode.id)}
                >
                  <span className="co-mode-label">{mode.label}</span>
                  <span className="co-mode-desc">{mode.desc}</span>
                </button>
              ))}
            </div>
          </section>

          {/* ── Global shared settings ── */}
          <section className="co-section">
            <h2 className="co-section-title">Global Cup Settings</h2>
            <p className="co-desc">
              Applies to all cups unless a cup overrides these values.
            </p>
            <div className="cup-override-grid">
              <div className="cup-field-pair">
                <label>Number of Cars</label>
                <input type="number" min={1} max={16} value={globalNumCars}
                  onChange={e => set("numCars", parseInt(e.target.value) || 8)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Number of Tries</label>
                <input type="number" min={1} max={10} value={cupSpecState.numTries}
                  onChange={e => set("numTries", parseInt(e.target.value) || 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Per-Race Position</label>
                <input type="number" min={1} max={16} value={cupSpecState.perRaceRequiredPlace}
                  onChange={e => set("perRaceRequiredPlace", parseInt(e.target.value) || 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Overall Position</label>
                <input type="number" min={1} max={16} value={cupSpecState.overallRequiredPlace}
                  onChange={e => set("overallRequiredPlace", parseInt(e.target.value) || 1)}
                  className="co-number-input" />
              </div>
            </div>

            <div style={{ marginTop: "1.25rem" }}>
              <PointsTableEditor
                points={cupSpecState.pointsTable}
                numCars={globalNumCars}
                onChange={v => set("pointsTable", v)}
              />
            </div>
          </section>

          {/* ── Random-mode constraints ── */}
          <section className="co-section">
            <h2 className="co-section-title">Random Stage Constraints</h2>
            <p className="co-desc">
              Settings applied when "Stage Mode" is set to "Random".
            </p>
            <h4 style={{marginTop: "1rem", marginBottom: 0}}>Track Variant Flags</h4>
            <p className="co-desc">
              Which modes may be assigned to stages. Normal is always available.
              Reverse requires a track to have a reversed version.
            </p>
            <div className="co-checkbox-group">
              <label className="co-checkbox-row">
                <input type="checkbox" checked={cupSpecState.allowReverse}
                  onChange={e => set("allowReverse", e.target.checked)} />
                <span>Allow Reverse stages</span>
              </label>
              <label className="co-checkbox-row">
                <input type="checkbox" checked={cupSpecState.allowMirror}
                  onChange={e => set("allowMirror", e.target.checked)} />
                <span>Allow Mirror stages</span>
              </label>
              <label className="co-checkbox-row">
                <input type="checkbox" checked={cupSpecState.allowReverseMirror}
                  onChange={e => set("allowReverseMirror", e.target.checked)} />
                <span>Allow Reverse Mirror stages</span>
              </label>
            </div>
            <div className="co-checkbox-group" style={{ marginTop: "1rem" }}>
              <label className="co-checkbox-row">
                <input type="checkbox" checked={cupSpecState.guaranteeFirstNormal}
                  onChange={e => set("guaranteeFirstNormal", e.target.checked)} />
                <span>
                  <strong>Guarantee first appearance in Normal mode</strong>
                  {" "}— a track is always played normally the first time it appears across all cups.
                </span>
              </label>
            </div>
            <h4 style={{marginTop: "1rem", marginBottom: 0}}>Duplicate Track Handling</h4>
            <div>
              <p className="co-desc" style={{ marginBottom: "0.5rem" }}>
                When a track appears more than once in the same cup:
              </p>
              <div className="co-mode-grid">
                {SAME_TRACK_OPTIONS.map(opt => (
                  <button
                    key={opt.id}
                    className={`co-mode-card${cupSpecState.sameTrackHandling === opt.id ? " selected" : ""}`}
                    onClick={() => set("sameTrackHandling", opt.id)}
                  >
                    <span className="co-mode-label">{opt.label}</span>
                    <span className="co-mode-desc">{opt.desc}</span>
                  </button>
                ))}
              </div>
            </div>
            <h4 style={{marginTop: "1rem", marginBottom: 0}}>Number of Stages & Laps</h4>
            <div className="cup-override-grid">
              <div className="cup-field-pair">
                <label>Minimum Number of Stages</label>
                <input type="number" min={1} max={30} value={globalNumStagesMin}
                  onChange={e => set("numStagesMin", parseInt(e.target.value) || 2)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Stages</label>
                <input type="number" min={1} max={30} value={globalNumStagesMax}
                  onChange={e => set("numStagesMax", parseInt(e.target.value) || 8)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Number of Laps</label>
                <input type="number" min={1} max={30} value={globalLapsMin}
                  onChange={e => set("numLapsMin", parseInt(e.target.value) || 2)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Laps</label>
                <input type="number" min={1} max={30} value={globalLapsMax}
                  onChange={e => set("numLapsMax", parseInt(e.target.value) || 8)}
                  className="co-number-input" />
              </div>
            </div>
          </section>

          {/* Per-cup configuration is available on dedicated sidebar pages (Bronze/Silver/Gold/Platinum). */}
        </>
      )}
    </div>
  );
}
