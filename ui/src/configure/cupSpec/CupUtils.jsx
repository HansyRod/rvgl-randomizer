import { useMemo } from "react";

// ─── Constants ────────────────────────────────────────────────────────────────

export const CUP_NAMES = ["Bronze Cup", "Silver Cup", "Gold Cup", "Platinum Cup"];
export const CUP_DIFFICULTIES = [1, 2, 3, 4]; // fixed, never shown to user
export const DEFAULT_CARS_PER_CLASS = [
  [7, 0, 0, 0, 0, 0],
  [0, 4, 3, 0, 0, 0],
  [0, 0, 4, 3, 0, 0],
  [0, 0, 1, 3, 3, 0],
];
export const RATING_LABELS = ["Rookie", "Amateur", "Advanced", "Semi-Pro", "Pro", "Super Pro"];
export const DEFAULT_POINTS = [10, 6, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
export const STAGE_MODES = [
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
export const SAME_TRACK_OPTIONS = [
  { id: "forbid",        label: "Forbid",         desc: "Each track may only appear once per cup." },
  { id: "allowAny",     label: "Allow Any",       desc: "A track can repeat any number of times." },
  { id: "allowVariants",label: "Allow Variants",  desc: "A track may repeat only as a different mode (Normal/Reverse/Mirror/…)." },
];

// ─── Helpers ──────────────────────────────────────────────────────────────────

export function makeDefaultStage() {
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

// ─── Shared sub-components ────────────────────────────────────────────────────

export function StageRow({ stage, index, onUpdate, onRemove, resolvedTracks, scanResult }) {
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

export function CarsPerClassEditor({ carsPerClass, numCars, onChange }) {
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

export function PointsTableEditor({ points, numCars, onChange }) {
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
