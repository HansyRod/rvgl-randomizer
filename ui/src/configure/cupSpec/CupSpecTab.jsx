import { useCallback } from "react";
import "../carOptions/CarOptionsTab.css";
import "./CupSpecTab.css";
import { useAppContext } from "../../AppProvider";
import {
  STAGE_MODES,
  SAME_TRACK_OPTIONS,
  PointsTableEditor,
  getCupCarLimit,
  normalizePointsTable,
  DEFAULT_MAX_RACE_LENGTH,
} from "./CupUtils";
export { makeDefaultCupSpec, makeDefaultCupSpecState } from "./CupSpecDefaults";

// ─── Default state factory ────────────────────────────────────────────────────

export default function CupSpecTab() {

  const { state, updateCategoryCtx } = useAppContext();

  const { configure } = state;
  
  const { cupSpecState, featureOptions } = configure;
  const maxCupCars = getCupCarLimit(featureOptions?.enable30CarMode);

  const set = useCallback((key, val) => {
    const nextCupSpecState = { ...cupSpecState, [key]: val };
    if (key === "numCars") {
      nextCupSpecState.pointsTable = normalizePointsTable(cupSpecState.pointsTable);
      nextCupSpecState.cups = (cupSpecState.cups || []).map(cup => ({
        ...cup,
        pointsTable: normalizePointsTable(cup.pointsTable ?? cupSpecState.pointsTable),
      }));
    }
    updateCategoryCtx("configure", { cupSpecState: nextCupSpecState });
  }, [cupSpecState, updateCategoryCtx]);

  const setIntWithDefault = useCallback((key, val, def) => {
    let intVal = parseInt(val);
    if (isNaN(intVal)) {
      intVal = def;
    }
    set(key, intVal);
  }, [set]);

  const stageMode = cupSpecState.stageMode;

  const globalNumCars = cupSpecState.numCars;
  const globalLapsMin = cupSpecState.numLapsMin;
  const globalLapsMax = cupSpecState.numLapsMax;
  const globalNumStagesMin = cupSpecState.numStagesMin;
  const globalNumStagesMax = cupSpecState.numStagesMax;
  const globalToggleMaxRaceLength = cupSpecState.toggleMaxRaceLength;

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
                <input type="number" min={1} max={maxCupCars} value={globalNumCars}
                  onChange={e => setIntWithDefault("numCars", e.target.value, 8)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Number of Tries</label>
                <input type="number" min={0} max={10} value={cupSpecState.numTries}
                  onChange={e => setIntWithDefault("numTries", e.target.value, 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Per-Race Position</label>
                <input type="number" min={1} max={maxCupCars} value={cupSpecState.perRaceRequiredPlace}
                  onChange={e => setIntWithDefault("perRaceRequiredPlace", e.target.value, 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Overall Position</label>
                <input type="number" min={1} max={maxCupCars} value={cupSpecState.overallRequiredPlace}
                  onChange={e => setIntWithDefault("overallRequiredPlace", e.target.value, 1)}
                  className="co-number-input" />
              </div>
            </div>

            <div style={{ marginTop: "1.25rem" }}>
              <PointsTableEditor
                points={cupSpecState.pointsTable}
                numCars={globalNumCars}
                maxPositions={maxCupCars}
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
                <input type="number" min={1} max={16} value={globalNumStagesMin}
                  onChange={e => setIntWithDefault("numStagesMin", e.target.value, 2)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Stages</label>
                <input type="number" min={1} max={16} value={globalNumStagesMax}
                  onChange={e => setIntWithDefault("numStagesMax", e.target.value, 8)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Number of Laps</label>
                <input type="number" min={1} max={20} value={globalLapsMin}
                  onChange={e => setIntWithDefault("numLapsMin", e.target.value, 2)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Laps</label>
                <input type="number" min={1} max={20} value={globalLapsMax}
                  onChange={e => setIntWithDefault("numLapsMax", e.target.value, 8)}
                  className="co-number-input" />
              </div>
            </div>
            <div style={{ marginTop: "1rem" }}>
              <label className="co-checkbox-row" style={{ marginBottom: "0.5rem" }}>
                <input
                  type="checkbox"
                  checked={globalToggleMaxRaceLength}
                  onChange={e => set(
                    "toggleMaxRaceLength",
                    e.target.checked
                  )}
                />
                <span>Limit maximum race length (meters)</span>
              </label>
              <p className="co-desc" style={{ marginBottom: "0.5rem" }}>
                Caps laps after a track and variant are randomly selected. It does not affect which tracks are assigned to cup stages, and does not apply when you choose a specific track for a cup yourself.
              </p>
              <input
                type="number"
                min={1}
                value={cupSpecState.maxRaceLengthValue}
                onChange={e => setIntWithDefault("maxRaceLengthValue", e.target.value, DEFAULT_MAX_RACE_LENGTH)}
                disabled={!globalToggleMaxRaceLength}
                className="co-number-input"
                aria-label="Maximum race length in meters"
              />
            </div>
          </section>

          {/* Per-cup configuration is available on dedicated sidebar pages (Bronze/Silver/Gold/Platinum). */}
        </>
      )}
    </div>
  );
}
