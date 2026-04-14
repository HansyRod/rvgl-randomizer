import { useState, useMemo, useCallback } from "react";
import "../carOptions/CarOptionsTab.css";
import "./CupSpecTab.css";
import { useAppContext } from "../../AppProvider";

// ─── Constants ────────────────────────────────────────────────────────────────

const CUP_NAMES = ["Bronze Cup", "Silver Cup", "Gold Cup", "Platinum Cup"];
const CUP_DIFFICULTIES = [1, 2, 3, 4]; // fixed, never shown to user
const DEFAULT_CARS_PER_CLASS = [
  [7, 0, 0, 0, 0, 0],
  [0, 4, 3, 0, 0, 0],
  [0, 0, 4, 3, 0, 0],
  [0, 0, 1, 3, 3, 0],
];
const RATING_LABELS = ["Rookie", "Amateur", "Advanced", "Semi-Pro", "Pro", "Super Pro"];
const DEFAULT_POINTS = [10, 6, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const STAGE_MODES = [
  {
    id: "default",
    label: "Default Stages",
    desc: "Follow the base game's stage layout, using the randomized track list.",
  },
  {
    id: "random",
    label: "Random Stages",
    desc: "Randomly generate stages from the resolved track pool.",
  },
  {
    id: "userDefined",
    label: "User-Defined Stages",
    desc: "Manually configure each stage. Guarantee First Normal is auto-disabled.",
  },
];
const SAME_TRACK_OPTIONS = [
  { id: "forbid",        label: "Forbid",         desc: "Each track may only appear once per cup." },
  { id: "allowAny",     label: "Allow Any",       desc: "A track can repeat any number of times." },
  { id: "allowVariants",label: "Allow Variants",  desc: "A track may repeat only as a different mode (Normal/Reverse/Mirror/…)." },
];

// ─── Default state factory ────────────────────────────────────────────────────

export function makeDefaultCupSpec(index) {
  return {
    index,
    overrideGlobal: false,
    numCars: 8,
    numTries: 3,
    perRaceRequiredPlace: 3,
    overallRequiredPlace: 1,
    pointsTable: [...DEFAULT_POINTS],
    carsPerClass: [...DEFAULT_CARS_PER_CLASS[index]],
    numLapsMin: 6,
    numLapsMax: 6,
    numStagesMin: 4,
    numStagesMax: 4,
    stages: [],
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
    numLapsMin: 6,
    numLapsMax: 6,
    cups: [0, 1, 2, 3].map(makeDefaultCupSpec),
  };
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

function makeDefaultStage() {
  return {
    sourcePool: "Random",
    numLaps: null,
    isReverse: null,
    isMirror: null,
  };
}

function carsPerClassSum(arr) {
  return (arr || []).reduce((s, v) => s + (Number(v) || 0), 0);
}

// ─── Sub-components ───────────────────────────────────────────────────────────

function StageRow({ stage, index, onUpdate, onRemove, resolvedTracks, scanResult }) {
  const trackOptions = useMemo(() => {
    if (!resolvedTracks || resolvedTracks.length === 0) return [];
    return resolvedTracks;
  }, [resolvedTracks]);

  const poolValue = stage.sourcePool ?? "Random";

  return (
    <div className="cup-stage-row">
      <span className="cup-stage-num">{index + 1}</span>

      {/* Track pool */}
      <select
        value={poolValue}
        onChange={e => onUpdate(index, { sourcePool: e.target.value })}
        className="cup-stage-select"
      >
        <option value="Random">Full Random</option>
        <optgroup label="By Difficulty">
          <option value="1">Easy tracks</option>
          <option value="2">Medium tracks</option>
          <option value="3">Hard tracks</option>
          <option value="4">Extreme tracks</option>
        </optgroup>
        {trackOptions.length > 0 && (
          <optgroup label="Specific Track">
            {trackOptions.map(t => (
              <option key={t.folderName ?? t.folder} value={t.folderName ?? t.folder}>
                {t.name} ({t.folderName ?? t.folder})
              </option>
            ))}
          </optgroup>
        )}
      </select>

      {/* Laps */}
      <div className="cup-stage-laps">
        <label className="cup-stage-inline-label">Laps</label>
        <select
          value={stage.numLaps === null ? "inherit" : String(stage.numLaps)}
          onChange={e => onUpdate(index, { numLaps: e.target.value === "inherit" ? null : parseInt(e.target.value) })}
          className="cup-stage-select-sm"
        >
          <option value="inherit">Inherit</option>
          {[1,2,3,4,5,6,7,8,9,10].map(n => (
            <option key={n} value={n}>{n}</option>
          ))}
        </select>
      </div>

      {/* Reverse flag */}
      <div className="cup-stage-flag">
        <label className="cup-stage-inline-label">Rev</label>
        <select
          value={stage.isReverse === null ? "random" : String(stage.isReverse)}
          onChange={e => onUpdate(index, { isReverse: e.target.value === "random" ? null : e.target.value === "true" })}
          className="cup-stage-select-sm"
        >
          <option value="random">Rnd</option>
          <option value="false">No</option>
          <option value="true">Yes</option>
        </select>
      </div>

      {/* Mirror flag */}
      <div className="cup-stage-flag">
        <label className="cup-stage-inline-label">Mir</label>
        <select
          value={stage.isMirror === null ? "random" : String(stage.isMirror)}
          onChange={e => onUpdate(index, { isMirror: e.target.value === "random" ? null : e.target.value === "true" })}
          className="cup-stage-select-sm"
        >
          <option value="random">Rnd</option>
          <option value="false">No</option>
          <option value="true">Yes</option>
        </select>
      </div>

      <button
        onClick={() => onRemove(index)}
        title="Remove stage"
        className="cup-stage-remove"
      >✕</button>
    </div>
  );
}

function CarsPerClassEditor({ carsPerClass, numCars, onChange }) {
  const sum = carsPerClassSum(carsPerClass);
  const expected = (numCars || 8) - 1;
  const isValid = sum === expected;

  return (
    <div className="cup-cars-per-class">
      <div className="cup-cpc-header">
        <span className="cup-cpc-title">Cars Per Class</span>
        <span className={`cup-cpc-sum ${isValid ? "valid" : "invalid"}`}>
          Sum: {sum} / {expected} {!isValid && "⚠ Must equal numCars − 1"}
        </span>
      </div>
      <div className="cup-cpc-grid">
        {RATING_LABELS.map((label, i) => (
          <div key={label} className="cup-cpc-cell">
            <label className="cup-cpc-label">{label}</label>
            <input
              type="number"
              min={0}
              max={expected}
              value={carsPerClass[i] ?? 0}
              onChange={e => {
                const next = [...carsPerClass];
                next[i] = Math.max(0, parseInt(e.target.value) || 0);
                onChange(next);
              }}
              className="cup-cpc-input"
            />
          </div>
        ))}
      </div>
    </div>
  );
}

function PointsTableEditor({ points, numCars, onChange }) {
  const maxActive = Math.min(numCars || 8, 16);
  return (
    <div className="cup-points-table">
      <div className="cup-cpc-title" style={{ marginBottom: "0.5rem" }}>Points Table (1st → 16th)</div>
      <div className="cup-points-grid">
        {Array.from({ length: 16 }, (_, i) => (
          <div key={i} className="cup-points-cell">
            <label className="cup-points-label">{i + 1}{i === 0 ? "st" : i === 1 ? "nd" : i === 2 ? "rd" : "th"}</label>
            <input
              type="number"
              min={0}
              max={99}
              value={points[i] ?? 0}
              disabled={i >= maxActive}
              onChange={e => {
                const next = [...points];
                next[i] = Math.max(0, parseInt(e.target.value) || 0);
                onChange(next);
              }}
              className="cup-points-input"
            />
          </div>
        ))}
      </div>
    </div>
  );
}

function CupCard({
  cupIndex,
  cupSpec,
  globalState,
  stageMode,
  resolvedTracks,
  scanResult,
  onUpdateCup,
}) {
  const [expanded, setExpanded] = useState(false);
  const name = CUP_NAMES[cupIndex];

  const set = useCallback((key, val) => {
    onUpdateCup(cupIndex, { [key]: val });
  }, [cupIndex, onUpdateCup]);

  const useOverride = cupSpec.overrideGlobal;

  // Active values — cup override or global
  const numCars = useOverride ? (cupSpec.numCars ?? globalState.numCars) : globalState.numCars;
  const lapsMin = useOverride ? (cupSpec.numLapsMin ?? globalState.numLapsMin) : globalState.numLapsMin;
  const lapsMax = useOverride ? (cupSpec.numLapsMax ?? globalState.numLapsMax) : globalState.numLapsMax;

  const addStage = () => set("stages", [...(cupSpec.stages || []), makeDefaultStage()]);
  const removeStage = (i) => {
    const next = [...(cupSpec.stages || [])];
    next.splice(i, 1);
    set("stages", next);
  };
  const updateStage = (i, updates) => {
    const next = [...(cupSpec.stages || [])];
    next[i] = { ...next[i], ...updates };
    set("stages", next);
  };

  const stageCount = (cupSpec.stages || []).length;

  return (
    <div className="cup-card">
      <button
        className="cup-card-header"
        onClick={() => setExpanded(e => !e)}
      >
        <span className="cup-card-name">{name}</span>
        <span className="cup-card-summary">
          {stageMode === "userDefined"
            ? `${stageCount} stage${stageCount !== 1 ? "s" : ""}`
            : stageMode === "random"
            ? `${cupSpec.numStagesMin}–${cupSpec.numStagesMax} stages`
            : "Default stages"}
        </span>
        <span className="cup-card-chevron">{expanded ? "▲" : "▼"}</span>
      </button>

      {expanded && (
        <div className="cup-card-body">

          {/* Override global settings toggle */}
          <label className="co-checkbox-row" style={{ marginBottom: "1rem" }}>
            <input
              type="checkbox"
              checked={useOverride}
              onChange={e => set("overrideGlobal", e.target.checked)}
            />
            <span>Override global settings for this cup</span>
          </label>

          {useOverride && (
            <div className="cup-override-grid">
              <div className="cup-field-pair">
                <label>Num Cars</label>
                <input type="number" min={1} max={16} value={cupSpec.numCars ?? globalState.numCars}
                  onChange={e => set("numCars", parseInt(e.target.value) || 8)} className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Num Tries</label>
                <input type="number" min={1} max={10} value={cupSpec.numTries ?? globalState.numTries}
                  onChange={e => set("numTries", parseInt(e.target.value) || 3)} className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Per-Race Place</label>
                <input type="number" min={1} max={16} value={cupSpec.perRaceRequiredPlace ?? globalState.perRaceRequiredPlace}
                  onChange={e => set("perRaceRequiredPlace", parseInt(e.target.value) || 3)} className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Overall Place</label>
                <input type="number" min={1} max={16} value={cupSpec.overallRequiredPlace ?? globalState.overallRequiredPlace}
                  onChange={e => set("overallRequiredPlace", parseInt(e.target.value) || 1)} className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Laps Min</label>
                <input type="number" min={1} max={30} value={cupSpec.numLapsMin ?? globalState.numLapsMin}
                  onChange={e => set("numLapsMin", parseInt(e.target.value) || 6)} className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Laps Max</label>
                <input type="number" min={1} max={30} value={cupSpec.numLapsMax ?? globalState.numLapsMax}
                  onChange={e => set("numLapsMax", parseInt(e.target.value) || 6)} className="co-number-input" />
              </div>

              <div style={{ gridColumn: "1 / -1" }}>
                <CarsPerClassEditor
                  carsPerClass={cupSpec.carsPerClass ?? DEFAULT_CARS_PER_CLASS[cupIndex]}
                  numCars={numCars}
                  onChange={v => set("carsPerClass", v)}
                />
              </div>

              <div style={{ gridColumn: "1 / -1" }}>
                <PointsTableEditor
                  points={cupSpec.pointsTable ?? [...DEFAULT_POINTS]}
                  numCars={numCars}
                  onChange={v => set("pointsTable", v)}
                />
              </div>
            </div>
          )}

          {!useOverride && (
            <div style={{ marginBottom: "1rem" }}>
              <CarsPerClassEditor
                carsPerClass={cupSpec.carsPerClass ?? DEFAULT_CARS_PER_CLASS[cupIndex]}
                numCars={numCars}
                onChange={v => set("carsPerClass", v)}
              />
            </div>
          )}

          {/* Stage count range (Random mode) */}
          {stageMode === "random" && (
            <div className="cup-override-grid" style={{ marginBottom: "1rem" }}>
              <div className="cup-field-pair">
                <label>Min Stages</label>
                <input type="number" min={1} max={16} value={cupSpec.numStagesMin}
                  onChange={e => set("numStagesMin", Math.max(1, parseInt(e.target.value) || 1))}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Max Stages</label>
                <input type="number" min={1} max={16} value={cupSpec.numStagesMax}
                  onChange={e => set("numStagesMax", Math.max(cupSpec.numStagesMin, parseInt(e.target.value) || 1))}
                  className="co-number-input" />
              </div>
            </div>
          )}

          {/* User-defined stages */}
          {stageMode === "userDefined" && (
            <div className="cup-stages-section">
              <div className="cup-stages-header-row">
                <span className="cup-cpc-title">Stages</span>
                <span className="cup-stages-count">{stageCount}/16</span>
              </div>
              {stageCount === 0 && (
                <p className="cup-stages-empty">No stages defined. Add at least one stage.</p>
              )}
              {stageCount > 0 && (
                <div className="cup-stages-col-header">
                  <span style={{ width: 24 }}>#</span>
                  <span style={{ flex: 2 }}>Track Pool</span>
                  <span style={{ flex: 1 }}>Laps</span>
                  <span>Rev</span>
                  <span>Mir</span>
                  <span style={{ width: 28 }}></span>
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
        </div>
      )}
    </div>
  );
}

// ─── Main tab ─────────────────────────────────────────────────────────────────

export default function CupSpecTab() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure } = state;
  
  // Destructure individual variables
  const { scanResult } = setup;
  const { trackSpecState, cupSpecState } = configure;

  const set = useCallback((key, val) => {
    updateCategoryCtx("configure", { cupSpecState: { ...cupSpecState, [key]: val } });
  }, [cupSpecState, updateCategoryCtx]);

  const updateCup = useCallback((cupIndex, updates) => {

    const cups = cupSpecState.cups.map(c =>
      c.index === cupIndex ? { ...c, ...updates } : c
    );

    updateCategoryCtx("configure", { cupSpecState: { ...cupSpecState, cups: cups } });

  }, [cupSpecState, updateCategoryCtx]);

  // Resolved track list — what tracks the track spec will produce.
  // We surface this as a hint for the stage pool selector.
  // This is the intersection of "what the scan has" and "what track spec selected".
  const resolvedTracks = useMemo(() => {
    if (!scanResult || !trackSpecState) return [];

    const includeTracks = trackSpecState.includeTracks !== false;
    if (!includeTracks) {
      // Stock tracks only (folders hardcoded as they are always available)
      const stockFolders = [
        "nhood1","market2","muse2","garden1","roof","toylite","wild_west1",
        "toy2","nhood2","ship1","muse1","market1","wild_west2","ship2"
      ];
      const allTracks = scanResult.installType === "classic"
        ? (scanResult.tracks || [])
        : (scanResult.contentPacks || []).filter(p => p.useTracks).flatMap(p => p.tracks);
      return allTracks.filter(t => stockFolders.includes(t.folderName?.toLowerCase()));
    }

    // When tracks are randomized, the spec array drives what pool is available.
    // We show tracks whose sourcePool is a specific folder name (guaranteed),
    // plus all tracks from pools like "Full Random" as advisory options.
    const allTracks = scanResult.installType === "classic"
      ? (scanResult.tracks || [])
      : (scanResult.contentPacks || []).filter(p => p.useTracks).flatMap(p => p.tracks);

    return allTracks.filter(t => t.hasValidFile && t.trackType === 0);
  }, [scanResult, trackSpecState]);

  const stageMode = cupSpecState.stageMode;
  const userDefinedMode = stageMode === "userDefined";

  const globalNumCars = cupSpecState.numCars;
  const globalLapsMin = cupSpecState.numLapsMin;
  const globalLapsMax = cupSpecState.numLapsMax;

  return (
    <div className="car-options-tab cup-spec-tab">

      {/* ── Enable / disable cups ── */}
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
            Cups are disabled. The DLL will use default cup data.
          </p>
        )}
      </section>

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

          {/* ── Variant flags (hidden in userDefined mode) ── */}
          {!userDefinedMode && (
            <section className="co-section">
              <h2 className="co-section-title">Track Variant Flags</h2>
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
            </section>
          )}

          {/* ── Random-mode constraints (hidden in userDefined) ── */}
          {stageMode === "random" && (
            <section className="co-section">
              <h2 className="co-section-title">Random Stage Constraints</h2>
              <div className="co-checkbox-group">
                <label className="co-checkbox-row">
                  <input type="checkbox" checked={cupSpecState.guaranteeFirstNormal}
                    onChange={e => set("guaranteeFirstNormal", e.target.checked)} />
                  <span>
                    <strong>Guarantee first appearance in Normal mode</strong>
                    {" "}— a track is always played normally the first time it appears across all cups.
                  </span>
                </label>
              </div>
              <div style={{ marginTop: "1rem" }}>
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
            </section>
          )}

          {/* ── Global shared settings ── */}
          <section className="co-section">
            <h2 className="co-section-title">Global Cup Settings</h2>
            <p className="co-desc">
              Applies to all cups unless a cup overrides these values.
            </p>
            <div className="cup-override-grid">
              <div className="cup-field-pair">
                <label>Num Cars</label>
                <input type="number" min={1} max={16} value={globalNumCars}
                  onChange={e => set("numCars", parseInt(e.target.value) || 8)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Num Tries</label>
                <input type="number" min={1} max={10} value={cupSpecState.numTries}
                  onChange={e => set("numTries", parseInt(e.target.value) || 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Per-Race Place</label>
                <input type="number" min={1} max={16} value={cupSpecState.perRaceRequiredPlace}
                  onChange={e => set("perRaceRequiredPlace", parseInt(e.target.value) || 3)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Overall Place</label>
                <input type="number" min={1} max={16} value={cupSpecState.overallRequiredPlace}
                  onChange={e => set("overallRequiredPlace", parseInt(e.target.value) || 1)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Laps Min</label>
                <input type="number" min={1} max={30} value={globalLapsMin}
                  onChange={e => set("numLapsMin", parseInt(e.target.value) || 6)}
                  className="co-number-input" />
              </div>
              <div className="cup-field-pair">
                <label>Laps Max</label>
                <input type="number" min={1} max={30} value={globalLapsMax}
                  onChange={e => set("numLapsMax", parseInt(e.target.value) || 6)}
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

          {/* ── Per-cup cards ── */}
          <section className="co-section">
            <h2 className="co-section-title">Cups</h2>
            <div className="cup-cards-list">
              {cupSpecState.cups.map(cupSpec => (
                <CupCard
                  key={cupSpec.index}
                  cupIndex={cupSpec.index}
                  cupSpec={cupSpec}
                  globalState={cupSpecState}
                  stageMode={stageMode}
                  resolvedTracks={resolvedTracks}
                  scanResult={scanResult}
                  onUpdateCup={updateCup}
                />
              ))}
            </div>
          </section>
        </>
      )}
    </div>
  );
}
