import { useMemo, useCallback } from "react";
import "../carOptions/CarOptionsTab.css";
import "./CupSpecTab.css";
import { useAppContext } from "../../AppProvider";
import { STOCK_TRACKS } from "../../utils/constants";
import { isEffectiveStockTracksMode } from "../../validation/stockMode";
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

// ─── OverrideRow ──────────────────────────────────────────────────────────────
// A single row in the override table.
function OverrideRow({ label, globalValue, overriding, onToggle, children }) {
  return (
    <tr className={`cup-override-row${overriding ? " is-overriding" : ""}`}>
      <td className="cup-ov-label">{label}</td>
      <td className="cup-ov-global">
        <span className="cup-ov-global-value">{globalValue}</span>
      </td>
      <td className="cup-ov-check">
        <input type="checkbox" checked={overriding} onChange={e => onToggle(e.target.checked)} />
      </td>
      <td className="cup-ov-input">
        <div className={overriding ? "" : "cup-ov-disabled"}>
          {children}
        </div>
      </td>
    </tr>
  );
}

// ─── CupConfigPage ────────────────────────────────────────────────────────────
// Renders the per-cup override configuration page for one cup (cupIndex 0–3).
// Each setting has its own override checkbox; when unchecked the control is
// visible but disabled, showing the current global value as a hint.

export default function CupConfigPage({ cupIndex }) {
  const { state, updateCategoryCtx } = useAppContext();
  const { setup, configure } = state;
  const { scanResult } = setup;
  const { trackSpecState, cupSpecState, preset } = configure;
  const isStockTracksMode = scanResult ? isEffectiveStockTracksMode(scanResult, preset) : false;
  const slotCount = isStockTracksMode ? 13 : 14;

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

  const setCupIntWithDefault = useCallback((key, val, defaultVal) => {
    const intVal = parseInt(val);
    setCup(key, isNaN(intVal) ? defaultVal : intVal);
  }, [setCup]);

  // Effective values (cup override if active, else global)
  const eff = {
    stageMode:    cupSpec.overrideStageMode    ? (cupSpec.stageMode ?? globalState.stageMode)                       : globalState.stageMode,
    numCars:      cupSpec.overrideNumCars      ? (cupSpec.numCars ?? globalState.numCars)                           : globalState.numCars,
    numTries:     cupSpec.overrideNumTries     ? (cupSpec.numTries ?? globalState.numTries)                         : globalState.numTries,
    perRacePlace: cupSpec.overridePerRacePlace ? (cupSpec.perRaceRequiredPlace ?? globalState.perRaceRequiredPlace) : globalState.perRaceRequiredPlace,
    overallPlace: cupSpec.overrideOverallPlace ? (cupSpec.overallRequiredPlace ?? globalState.overallRequiredPlace) : globalState.overallRequiredPlace,
    numStagesMin: cupSpec.overrideStageMode    ? (cupSpec.numStagesMin ?? globalState.numStagesMin)                 : globalState.numStagesMin,
    numStagesMax: cupSpec.overrideStageMode    ? (cupSpec.numStagesMax ?? globalState.numStagesMax)                 : globalState.numStagesMax,
    numLapsMin:   cupSpec.overrideStageMode    ? (cupSpec.numLapsMin ?? globalState.numLapsMin)                     : globalState.numLapsMin,
    numLapsMax:   cupSpec.overrideStageMode    ? (cupSpec.numLapsMax ?? globalState.numLapsMax)                     : globalState.numLapsMax,
  };

  const isRandomMode = eff.stageMode === "random";

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

    // Track randomization disabled → the normalized track spec is the valid set
    if (trackSpecState.includeTracks === false) {
      const eligibleTracks = new Map(
        allTracks
          .filter(track => track.hasValidFile && track.trackType === 0 && track.folderName)
          .map(track => [track.folderName.toLowerCase(), track])
      );
      const stockFolders = isStockTracksMode
        ? STOCK_TRACKS.filter(folder => folder.toLowerCase() !== "roof")
        : STOCK_TRACKS;

      return stockFolders
        .map(folder => eligibleTracks.get(folder.toLowerCase()))
        .filter(Boolean);
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
  }, [scanResult, trackSpecState, isStockTracksMode]);



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
        {cupSpec.overrideStageMode
          ?
          <div className={`cup-override-controls${cupSpec.overrideStageMode ? "" : " disabled"}`}>
            <p className="co-desc">How stages are assigned to this cup.</p>
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
          </div>
          :
          <p className="co-desc cup-override-hint">
            Inheriting global stage mode: <strong>{STAGE_MODES.find(m => m.id === globalState.stageMode)?.label ?? globalState.stageMode}</strong>.
          </p>
        }

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
                slotCount={slotCount}
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
        { cupSpec.overrideCarsPerClass
          ?
          <div className={`cup-override-controls${cupSpec.overrideCarsPerClass ? "" : " disabled"}`}>
            <CarsPerClassEditor
              carsPerClass={cupSpec.carsPerClass ?? DEFAULT_CARS_PER_CLASS[cupIndex]}
              numCars={effectiveNumCars}
              onChange={v => cupSpec.overrideCarsPerClass && setCup("carsPerClass", v)}
            />
          </div>
          :
          <p className="co-desc cup-override-hint">
            Using the default Cars Per Class settings for this cup.
          </p>
        }
      </section>

      {/* ── Override table ── */}
      <section className="cup-ov-table-section">
        <h3>Override Summary</h3>
        <p>Choose which settings to override and compare the cup value with the global value.</p>
        <table className="cup-ov-table">
          <thead>
            <tr>
              <th className="cup-ov-th-label">Setting</th>
              <th className="cup-ov-th-global">Global value</th>
              <th className="cup-ov-th-check">Override?</th>
              <th className="cup-ov-th-input">Cup value</th>
            </tr>
          </thead>
          <tbody>

            {/* Number of Cars */}
            <OverrideRow
              label="Number of Cars"
              globalValue={globalState.numCars}
              overriding={cupSpec.overrideNumCars}
              onToggle={v => setCup("overrideNumCars", v)}
            >
              <input
                type="number" min={1} max={16}
                value={cupSpec.numCars ?? globalState.numCars}
                onChange={e => setCupIntWithDefault("numCars", e.target.value, 8)}
                disabled={!cupSpec.overrideNumCars}
                className="co-number-input"
              />
            </OverrideRow>

            {/* Number of Tries */}
            <OverrideRow
              label="Number of Tries"
              globalValue={globalState.numTries}
              overriding={cupSpec.overrideNumTries}
              onToggle={v => setCup("overrideNumTries", v)}
            >
              <input
                type="number" min={0} max={10}
                value={cupSpec.numTries ?? globalState.numTries}
                onChange={e => setCupIntWithDefault("numTries", e.target.value, 3)}
                disabled={!cupSpec.overrideNumTries}
                className="co-number-input"
              />
            </OverrideRow>

            {/* Per-Race Position */}
            <OverrideRow
              label="Minimum Position Per Race"
              globalValue={`${globalState.perRaceRequiredPlace}${ordinal(globalState.perRaceRequiredPlace)}`}
              overriding={cupSpec.overridePerRacePlace}
              onToggle={v => setCup("overridePerRacePlace", v)}
            >
              <input
                type="number" min={1} max={16}
                value={cupSpec.perRaceRequiredPlace ?? globalState.perRaceRequiredPlace}
                onChange={e => setCupIntWithDefault("perRaceRequiredPlace", e.target.value, 3)}
                disabled={!cupSpec.overridePerRacePlace}
                className="co-number-input"
              />
            </OverrideRow>

            {/* Overall Position */}
            <OverrideRow
              label="Minimum Overall Position"
              globalValue={`${globalState.overallRequiredPlace}${ordinal(globalState.overallRequiredPlace)}`}
              overriding={cupSpec.overrideOverallPlace}
              onToggle={v => setCup("overrideOverallPlace", v)}
            >
              <input
                type="number" min={1} max={16}
                value={cupSpec.overallRequiredPlace ?? globalState.overallRequiredPlace}
                onChange={e => setCupIntWithDefault("overallRequiredPlace", e.target.value, 1)}
                disabled={!cupSpec.overrideOverallPlace}
                className="co-number-input"
              />
            </OverrideRow>

            {/* Random-mode rows — only shown when the effective mode is Random
                (either from global or from an already-active stage-mode override) */}
            {isRandomMode && (<>

              <OverrideRow
                label="Minimum Number of Stages"
                globalValue={globalState.numStagesMin}
                overriding={cupSpec.overrideNumStagesMin}
                onToggle={v => setCup("overrideNumStagesMin", v)}
              >
                <input
                  type="number" min={1} max={16}
                  value={cupSpec.numStagesMin ?? globalState.numStagesMin}
                  onChange={e => setCupIntWithDefault("numStagesMin", e.target.value, 2)}
                  disabled={!cupSpec.overrideNumStagesMin}
                  className="co-number-input"
                />
              </OverrideRow>

              <OverrideRow
                label="Maximum Number of Stages"
                globalValue={globalState.numStagesMax}
                overriding={cupSpec.overrideNumStagesMax}
                onToggle={v => setCup("overrideNumStagesMax", v)}
              >
                <input
                  type="number" min={1} max={16}
                  value={cupSpec.numStagesMax ?? globalState.numStagesMax}
                  onChange={e => setCupIntWithDefault("numStagesMax", e.target.value, 8)}
                  disabled={!cupSpec.overrideNumStagesMax}
                  className="co-number-input"
                />
              </OverrideRow>

              <OverrideRow
                label="Minimum Number of Laps"
                globalValue={globalState.numLapsMin}
                overriding={cupSpec.overrideNumLapsMin}
                onToggle={v => setCup("overrideNumLapsMin", v)}
              >
                <input
                  type="number" min={1} max={20}
                  value={cupSpec.numLapsMin ?? globalState.numLapsMin}
                  onChange={e => setCupIntWithDefault("numLapsMin", e.target.value, 2)}
                  disabled={!cupSpec.overrideNumLapsMin}
                  className="co-number-input"
                />
              </OverrideRow>

              <OverrideRow
                label="Maximum Number of Laps"
                globalValue={globalState.numLapsMax}
                overriding={cupSpec.overrideNumLapsMax}
                onToggle={v => setCup("overrideNumLapsMax", v)}
              >
                <input
                  type="number" min={1} max={20}
                  value={cupSpec.numLapsMax ?? globalState.numLapsMax}
                  onChange={e => setCupIntWithDefault("numLapsMax", e.target.value, 8)}
                  disabled={!cupSpec.overrideNumLapsMax}
                  className="co-number-input"
                />
              </OverrideRow>
            </>)}

          </tbody>
        </table>
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
        {cupSpec.overridePointsTable
          ?
          <div className={`cup-override-controls${cupSpec.overridePointsTable ? "" : " disabled"}`}>
            <PointsTableEditor
              points={cupSpec.pointsTable ?? [...DEFAULT_POINTS]}
              numCars={effectiveNumCars}
              onChange={v => cupSpec.overridePointsTable && setCup("pointsTable", v)}
            />
          </div>
          :
          <p className="co-desc cup-override-hint">
            Inheriting global points table.
          </p>
        }
      </section>

    </div>
  );
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
function ordinal(n) {
  if (n === 1) return "st";
  if (n === 2) return "nd";
  if (n === 3) return "rd";
  return "th";
}