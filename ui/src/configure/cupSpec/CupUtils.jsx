// No React hook imports needed for this module

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
    numLaps: null,    // null = random between numLapsMin/numLapsMax; number = fixed
    numLapsMin: 2,
    numLapsMax: 8,
    isReverse: null,
    isMirror: null,
  };
}

function carsPerClassSum(arr) {
  return (arr || []).reduce((s, v) => s + (Number(v) || 0), 0);
}

// ─── Shared sub-components ────────────────────────────────────────────────────

const SLOT_COUNT = 14; // Fixed number of stock track slots (slots 0-13)

export function StageRow({ stage, index, onUpdate, onRemove, resolvedTracks }) {
  const poolValue = stage.sourcePool ?? "Random";

  // numLaps: null → random mode; number → fixed mode
  const isFixedLaps = typeof stage.numLaps === "number";

  return (
    <div className="cup-stage-row">
      <span className="cup-stage-num">{index + 1}</span>

      {/* Track — Random | Specific Slot | Specific Track from resolved list */}
      <select
        value={poolValue}
        onChange={e => onUpdate(index, { sourcePool: e.target.value })}
        className="cup-stage-select"
      >
        <option value="Random">Random</option>
        <optgroup label="Specific Slot">
          {Array.from({ length: SLOT_COUNT }, (_, i) => (
            <option key={`slot:${i}`} value={`slot:${i}`}>Slot {i + 1}</option>
          ))}
        </optgroup>
        {resolvedTracks && resolvedTracks.length > 0 && (
          <optgroup label="Specific Track">
            {resolvedTracks.map(t => (
              <option key={t.folderName ?? t.folder} value={t.folderName ?? t.folder}>
                {t.name ?? (t.folderName ?? t.folder)}
              </option>
            ))}
          </optgroup>
        )}
        { poolValue !== "Random" && 
         !poolValue.startsWith("slot:") && 
         !resolvedTracks.some((t) => t.folderName === poolValue) &&
          (
            <option value={poolValue}>Unknown Track: {poolValue}</option>
          )
        }
      </select>

      {/* Laps — Fixed (single value) or Random (explicit min–max range) */}
      <div className="cup-stage-laps">
        <select
          value={isFixedLaps ? "fixed" : "random"}
          onChange={e => onUpdate(index,
            e.target.value === "fixed"
              ? { numLaps: stage.numLapsMin ?? 6 }
              : { numLaps: null }
          )}
          className="cup-stage-select-sm"
        >
          <option value="fixed">Fixed</option>
          <option value="random">Random</option>
        </select>
        {isFixedLaps ? (
          <input
            type="number" min={1} max={30}
            value={stage.numLaps}
            onChange={e => onUpdate(index, { numLaps: Math.max(1, parseInt(e.target.value) || 1) })}
            className="cup-stage-laps-input"
          />
        ) : (
          <>
            <input
              type="number" min={1} max={30}
              value={stage.numLapsMin ?? 2}
              onChange={e => onUpdate(index, { numLapsMin: Math.max(1, parseInt(e.target.value) || 1) })}
              className="cup-stage-laps-input"
              title="Min laps"
            />
            <span className="cup-stage-laps-sep">–</span>
            <input
              type="number" min={1} max={30}
              value={stage.numLapsMax ?? 8}
              onChange={e => onUpdate(index, { numLapsMax: Math.max(stage.numLapsMin ?? 1, parseInt(e.target.value) || 1) })}
              className="cup-stage-laps-input"
              title="Max laps"
            />
          </>
        )}
      </div>

      {/* Reverse flag */}
      <div className="cup-stage-flag">
        <select
          value={stage.isReverse === null ? "random" : String(stage.isReverse)}
          onChange={e => onUpdate(index, { isReverse: e.target.value === "random" ? null : e.target.value === "true" })}
          className="cup-stage-select-sm"
        >
          <option value="random">Random</option>
          <option value="false">No</option>
          <option value="true">Yes</option>
        </select>
      </div>

      {/* Mirror flag */}
      <div className="cup-stage-flag">
        <select
          value={stage.isMirror === null ? "random" : String(stage.isMirror)}
          onChange={e => onUpdate(index, { isMirror: e.target.value === "random" ? null : e.target.value === "true" })}
          className="cup-stage-select-sm"
        >
          <option value="random">Random</option>
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
