import { TRACK_DIFFICULTIES, TRACK_DIFFICULTY_SOURCE_LIST, TRACK_DIFFICULTY_ATTR_LIST, TRACK_OBTAINS_LIST } from "../../utils/constants";

export default function TrackSpecRow({ index, rowState, updateRow, trackByFolder, sourcePoolOptionsJSX, trackOptions, onOpenSearch}) {
  if (!rowState) return null;
  const isGeneralPool =
    rowState.sourcePool === "Full Random" ||
    rowState.sourcePool === "Stock" ||
    rowState.sourcePool === "Custom" ||
    rowState.sourcePool.startsWith("Pack:");
  const isSpecificTrack = !isGeneralPool && trackByFolder[rowState.sourcePool] !== undefined;
  const specificTrack = isSpecificTrack ? trackByFolder[rowState.sourcePool] : null;

  const lockDifficulty =
    trackOptions?.unlockMode === "randomUnlock" ||
    trackOptions?.unlockMode === "unchanged" ||
    trackOptions?.unlockMode === "baseGame";
  const lockObtain =
    trackOptions?.unlockMode === "randomDifficulty" ||
    trackOptions?.unlockMode === "unchanged" ||
    trackOptions?.unlockMode === "baseGame";

  return (
    <div className="spec-grid-row">
      <div className="car-id">{rowState.id}</div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select
            value={rowState.sourcePool}
            onChange={e => {
              const val = e.target.value;
              if (val === "Specific Track") onOpenSearch(index);
              else updateRow(index, { sourcePool: val });
            }}
          >
            {sourcePoolOptionsJSX}
            {!isGeneralPool && (
              <optgroup label="Current Selection">
                <option value={rowState.sourcePool}>
                  {specificTrack ? specificTrack.name : rowState.sourcePool}
                </option>
              </optgroup>
            )}
            <option value="Specific Track">Specific Track...</option>
          </select>
        </div>
        <div className="field-group">
          <select
            value={isSpecificTrack ? String(specificTrack.difficulty) : rowState.sourceDifficulty}
            onChange={e => updateRow(index, { sourceDifficulty: e.target.value })}
            disabled={isSpecificTrack}
          >
            {isSpecificTrack ? (
              <option value={String(specificTrack.difficulty)}>
                {TRACK_DIFFICULTIES[specificTrack.difficulty] || "Unknown"}
              </option>
            ) : (
              TRACK_DIFFICULTY_SOURCE_LIST.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))
            )}
          </select>
        </div>
      </div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select
            value={rowState.attrDifficulty}
            onChange={e => updateRow(index, { attrDifficulty: e.target.value })}
            disabled={lockDifficulty}
          >
            {TRACK_DIFFICULTY_ATTR_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
          </select>
        </div>
        <div className="field-group">
          <select
            value={rowState.attrObtain}
            onChange={e => updateRow(index, { attrObtain: e.target.value })}
            disabled={lockObtain}
          >
            {TRACK_OBTAINS_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>
                {opt.label}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  );
}