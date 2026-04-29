import { useMemo, useCallback } from "react";
import "../carOptions/CarOptionsTab.css";
import "./CupSpecTab.css";
import { useAppContext } from "../../AppProvider";
import {
  CUP_NAMES,
  DEFAULT_CARS_PER_CLASS,
  DEFAULT_POINTS,
  STAGE_MODES,
  makeDefaultStage,
  StageRow,
  CarsPerClassEditor,
  PointsTableEditor,
} from "./CupUtils";

// ─── CupConfigPage ────────────────────────────────────────────────────────────
// Renders the per-cup override configuration page for one cup (cupIndex 0–3).
// Each setting has its own override checkbox; when unchecked the control is
// visible but disabled, showing the current global value as a hint.

export default function CupConfigPage({ cupIndex }) {
  const { state, updateCategoryCtx } = useAppContext();
  const { setup, configure } = state;
  const { scanResult } = setup;
  const { trackSpecState, cupSpecState } = configure;

  const cupSpec = cupSpecState.cups[cupIndex];
  const globalState = cupSpecState;
  const name = CUP_NAMES[cupIndex];

  // Persist a single field on this cup's spec object
  const setCup = useCallback((key, val) => {
    const cups = cupSpecState.cups.map(c =>
      c.index === cupIndex ? { ...c, [key]: val } : c
    );
    updateCategoryCtx("configure", { cupSpecState: { ...cupSpecState, cups } });
  }, [cupSpecState, cupIndex, updateCategoryCtx]);

  // Tracks available for "Specific Track" selection in user-defined stages.
  // • When track randomization is disabled, the 14 stock tracks are the resolved
  //   list, so all of them are valid choices.
  // • When track randomization is enabled, only tracks explicitly pinned by
  //   folder name in the Track Spec are offered — these are the only tracks
  //   guaranteed to appear in the resolved 14-slot list. Showing all installed
  //   tracks would let users pick folders that fail validation and generation.
  const resolvedTracks = useMemo(() => {
    if (!scanResult || !trackSpecState) return [];

    const allTracks = scanResult.installType === "classic"
      ? (scanResult.tracks || [])
      : (scanResult.contentPacks || []).filter(p => p.useTracks).flatMap(p => p.tracks);

    // Track randomization disabled → stock tracks are the valid set
    if (trackSpecState.includeTracks === false) {
      const stockFolders = new Set([
        "nhood1","market2","muse2","garden1","roof","toylite","wild_west1",
        "toy2","nhood2","ship1","muse1","market1","wild_west2","ship2",
      ]);
      return allTracks.filter(t =>
        t.hasValidFile &&
        t.trackType === 0 &&
        stockFolders.has(t.folderName?.toLowerCase())
      );
    }

    // Track randomization enabled → only tracks pinned by folder name in the Track Spec
    const genericPools = new Set(["full random", "stock", "custom"]);
    const pinnedFolders = new Set(
      (trackSpecState.tracks || [])
        .map(t => t.sourcePool)
        .filter(p => p && !genericPools.has(p.toLowerCase()) && !p.toLowerCase().startsWith("pack:"))
        .map(p => p.toLowerCase())
    );

    if (pinnedFolders.size === 0) return [];

    return allTracks.filter(t =>
      t.hasValidFile &&
      t.trackType === 0 &&
      pinnedFolders.has(t.folderName?.toLowerCase())
    );
  }, [scanResult, trackSpecState]);



  // Stage list handlers
  const addStage    = () => setCup("stages", [...(cupSpec.stages || []), makeDefaultStage()]);
  const removeStage = (i) => { const next = [...(cupSpec.stages || [])]; next.splice(i, 1); setCup("stages", next); };
  const updateStage = (i, updates) => { const next = [...(cupSpec.stages || [])]; next[i] = { ...next[i], ...updates }; setCup("stages", next); };

  // Resolved numCars — cup override if active, otherwise global
  const effectiveNumCars = cupSpec.overrideNumCars
    ? (cupSpec.numCars ?? globalState.numCars)
    : globalState.numCars;

  const stageCount = (cupSpec.stages || []).length;

  // Effective stage mode: per-cup override if active, otherwise fall through to global.
  // Stages are always per-cup data, so the builder must be accessible even when the mode
  // is inherited from the global setting rather than overridden here.
  const effectiveStageMode = cupSpec.overrideStageMode ? cupSpec.stageMode : globalState.stageMode;


  return (
    <div className="car-options-tab cup-spec-tab">

      {/* ── Page header ── */}
      <div className="cup-page-header">
        <h1 className="cup-page-title">{name}</h1>
        <p className="co-desc">
          Override global cup settings for this cup. Each setting can be overridden independently —
          leave any unchecked to inherit the global value.
        </p>
      </div>

      {/* ── Section 1 · Stage Mode ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overrideStageMode}
              onChange={e => setCup("overrideStageMode", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Stage Mode</h2>
          </label>
        </div>
        {!cupSpec.overrideStageMode && (
          <p className="co-desc cup-override-hint">
            Inheriting global stage mode: <strong>{STAGE_MODES.find(m => m.id === globalState.stageMode)?.label ?? globalState.stageMode}</strong>.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overrideStageMode ? "" : " disabled"}`}>
          <p className="co-desc">How stages are assigned to this cup only.</p>
          <div className="co-mode-grid">
            {STAGE_MODES.map(mode => (
              <button
                key={mode.id}
                className={`co-mode-card${(cupSpec.stageMode ?? "default") === mode.id ? " selected" : ""}`}
                onClick={() => cupSpec.overrideStageMode && setCup("stageMode", mode.id)}
                disabled={!cupSpec.overrideStageMode}
              >
                <span className="co-mode-label">{mode.label}</span>
                <span className="co-mode-desc">{mode.desc}</span>
              </button>
            ))}
          </div>

          {/* Random Stages: stage count + laps range */}
          {cupSpec.overrideStageMode && cupSpec.stageMode === "random" && (
            <div className="cup-override-grid" style={{ marginTop: "1rem" }}>
              <div className="cup-field-pair">
                <label>Minimum Number of Stages</label>
                <input type="number" min={1} max={16} value={cupSpec.numStagesMin}
                  onChange={e => setCup("numStagesMin", Math.max(1, parseInt(e.target.value) || 1))}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Stages</label>
                <input type="number" min={1} max={16} value={cupSpec.numStagesMax}
                  onChange={e => setCup("numStagesMax", Math.max(cupSpec.numStagesMin, parseInt(e.target.value) || 1))}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Minimum Number of Laps</label>
                <input type="number" min={1} max={30} value={cupSpec.numLapsMin}
                  onChange={e => setCup("numLapsMin", parseInt(e.target.value) || 2)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Maximum Number of Laps</label>
                <input type="number" min={1} max={30} value={cupSpec.numLapsMax}
                  onChange={e => setCup("numLapsMax", Math.max(cupSpec.numLapsMin, parseInt(e.target.value) || 2))}
                  className="co-number-input" />
              </div>
            </div>
          )}
        </div>

        {/* Random Stages sub-options when mode is inherited from global (numStagesMin/Max are per-cup) */}
        {!cupSpec.overrideStageMode && effectiveStageMode === "random" && (
          <div className="cup-override-grid" style={{ marginTop: "1rem" }}>
            <div className="cup-field-pair">
              <label>Minimum Number of Stages</label>
              <input type="number" min={1} max={16} value={cupSpec.numStagesMin}
                onChange={e => setCup("numStagesMin", Math.max(1, parseInt(e.target.value) || 1))}
                className="co-number-input" />
            </div>
            <div className="cup-field-pair">
              <label>Maximum Number of Stages</label>
              <input type="number" min={1} max={16} value={cupSpec.numStagesMax}
                onChange={e => setCup("numStagesMax", Math.max(cupSpec.numStagesMin, parseInt(e.target.value) || 1))}
                className="co-number-input" />
            </div>
          </div>
        )}

        {/* Stage builder — stages are always per-cup data.
            Rendered whenever the effective mode is User-Defined, whether inherited
            from the global setting or set as a per-cup override. */}
        {effectiveStageMode === "userDefined" && (
          <div className="cup-stages-section" style={{ marginTop: "1rem" }}>
            <div className="cup-stages-header-row">
              <span className="cup-cpc-title">Stages</span>
              <span className="cup-stages-count">{stageCount}/16</span>
            </div>
            {stageCount === 0 && (
              <p className="cup-stages-empty">No stages defined. Add at least one stage.</p>
            )}
            {stageCount > 0 && (
              <div className="cup-stages-col-header">
                <span>#</span>
                <span>Track Pool</span>
                <span>Laps</span>
                <span>Reverse</span>
                <span>Mirror</span>
                <span></span>
              </div>
            )}

            {(cupSpec.stages || []).map((stage, i) => (
              <StageRow
                key={i}
                stage={stage}
                index={i}
                onUpdate={updateStage}
                onRemove={removeStage}
                resolvedTracks={resolvedTracks}
                scanResult={scanResult}
              />
            ))}
            {stageCount < 16 && (
              <button className="cup-add-stage-btn" onClick={addStage}>
                + Add Stage
              </button>
            )}
          </div>
        )}
      </section>


      {/* ── Section 2 · Number of Cars ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overrideNumCars}
              onChange={e => setCup("overrideNumCars", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Number of Cars</h2>
          </label>
        </div>
        {!cupSpec.overrideNumCars && (
          <p className="co-desc cup-override-hint">
            Inheriting global value: <strong>{globalState.numCars} cars</strong>.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overrideNumCars ? "" : " disabled"}`}>
          <div className="cup-override-grid">
            <div className="cup-field-pair">
              <label>Number of Cars</label>
              <input
                type="number" min={1} max={16}
                value={cupSpec.overrideNumCars ? (cupSpec.numCars ?? globalState.numCars) : globalState.numCars}
                onChange={e => setCup("numCars", parseInt(e.target.value) || 8)}
                disabled={!cupSpec.overrideNumCars}
                className="co-number-input"
              />
            </div>
          </div>
        </div>
      </section>

      {/* ── Section 3 · Cars Per Class ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overrideCarsPerClass}
              onChange={e => setCup("overrideCarsPerClass", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Cars Per Class</h2>
          </label>
        </div>
        {!cupSpec.overrideCarsPerClass && (
          <p className="co-desc cup-override-hint">
            Using the existing Cars Per Class settings for this cup from Cup Specification.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overrideCarsPerClass ? "" : " disabled"}`}>
          <CarsPerClassEditor
            carsPerClass={cupSpec.carsPerClass ?? DEFAULT_CARS_PER_CLASS[cupIndex]}
            numCars={effectiveNumCars}
            onChange={v => cupSpec.overrideCarsPerClass && setCup("carsPerClass", v)}
          />
        </div>
      </section>

      {/* ── Section 4 · Number of Tries ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overrideNumTries}
              onChange={e => setCup("overrideNumTries", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Number of Tries</h2>
          </label>
        </div>
        {!cupSpec.overrideNumTries && (
          <p className="co-desc cup-override-hint">
            Inheriting global value: <strong>{globalState.numTries} tries</strong>.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overrideNumTries ? "" : " disabled"}`}>
          <div className="cup-override-grid">
            <div className="cup-field-pair">
              <label>Number of Tries</label>
              <input
                type="number" min={1} max={10}
                value={cupSpec.overrideNumTries ? (cupSpec.numTries ?? globalState.numTries) : globalState.numTries}
                onChange={e => setCup("numTries", parseInt(e.target.value) || 3)}
                disabled={!cupSpec.overrideNumTries}
                className="co-number-input"
              />
            </div>
          </div>
        </div>
      </section>

      {/* ── Section 5 · Minimum Position Per Race ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overridePerRacePlace}
              onChange={e => setCup("overridePerRacePlace", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Minimum Position Per Race</h2>
          </label>
        </div>
        {!cupSpec.overridePerRacePlace && (
          <p className="co-desc cup-override-hint">
            Inheriting global value: finish <strong>position {globalState.perRaceRequiredPlace}</strong> or better each race.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overridePerRacePlace ? "" : " disabled"}`}>
          <div className="cup-override-grid">
            <div className="cup-field-pair">
              <label>Minimum Per-Race Position</label>
              <input
                type="number" min={1} max={16}
                value={cupSpec.overridePerRacePlace ? (cupSpec.perRaceRequiredPlace ?? globalState.perRaceRequiredPlace) : globalState.perRaceRequiredPlace}
                onChange={e => setCup("perRaceRequiredPlace", parseInt(e.target.value) || 3)}
                disabled={!cupSpec.overridePerRacePlace}
                className="co-number-input"
              />
            </div>
          </div>
        </div>
      </section>

      {/* ── Section 6 · Minimum Overall Position ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overrideOverallPlace}
              onChange={e => setCup("overrideOverallPlace", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Minimum Overall Position</h2>
          </label>
        </div>
        {!cupSpec.overrideOverallPlace && (
          <p className="co-desc cup-override-hint">
            Inheriting global value: finish <strong>position {globalState.overallRequiredPlace}</strong> overall to beat the cup.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overrideOverallPlace ? "" : " disabled"}`}>
          <div className="cup-override-grid">
            <div className="cup-field-pair">
              <label>Minimum Overall Position</label>
              <input
                type="number" min={1} max={16}
                value={cupSpec.overrideOverallPlace ? (cupSpec.overallRequiredPlace ?? globalState.overallRequiredPlace) : globalState.overallRequiredPlace}
                onChange={e => setCup("overallRequiredPlace", parseInt(e.target.value) || 1)}
                disabled={!cupSpec.overrideOverallPlace}
                className="co-number-input"
              />
            </div>
          </div>
        </div>
      </section>

      {/* ── Section 7 · Points Table ── */}
      <section className="co-section">
        <div className="cup-override-section-header">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={cupSpec.overridePointsTable}
              onChange={e => setCup("overridePointsTable", e.target.checked)}
            />
            <h2 className="co-section-title" style={{ margin: 0 }}>Points Table</h2>
          </label>
        </div>
        {!cupSpec.overridePointsTable && (
          <p className="co-desc cup-override-hint">
            Inheriting global points table.
          </p>
        )}
        <div className={`cup-override-controls${cupSpec.overridePointsTable ? "" : " disabled"}`}>
          <PointsTableEditor
            points={cupSpec.pointsTable ?? [...DEFAULT_POINTS]}
            numCars={effectiveNumCars}
            onChange={v => cupSpec.overridePointsTable && setCup("pointsTable", v)}
          />
        </div>
      </section>

    </div>
  );
}
