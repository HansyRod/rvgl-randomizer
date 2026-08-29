import { CAR_RATINGS, OBTAIN_METHODS, RATINGS_LIST, ATTR_RATINGS_LIST, OBTAINS_LIST, ATTR_OBTAINS_LIST } from "../../utils/constants";
import { getCustomUnlockSelectionLabel, isCustomUnlockMethod } from "../../utils/customUnlockState";

const CURRENT_CUSTOM_UNLOCK = "Current Custom Unlock";

export default function CarSpecRow({index, rowState, updateRow, carByFolder, sourcePoolOptionsJSX, poolValidOptions, carOptions, onOpenSearch,
  onOpenCustomUnlock, trackByFolder, lockStartingPool, lockStartingRating, lockStartingObtain, onRemove}) {
  if (!rowState) return null;

  const id = rowState.id;
  const isGeneralPool = rowState.sourcePool === "Full Random" || 
                        rowState.sourcePool === "Stock" || 
                        rowState.sourcePool === "DC" || 
                        rowState.sourcePool === "Custom" || 
                        rowState.sourcePool.startsWith("Pack:");

  const isSpecificCar = !isGeneralPool && carByFolder[rowState.sourcePool] !== undefined;
  const specificCar = isSpecificCar ? carByFolder[rowState.sourcePool] : null;

  const validOptions = isGeneralPool ? poolValidOptions[rowState.sourcePool] : null;

  // Disabled checks for each input
  const disableSourcePool = lockStartingPool;
  const disableSourceRating = isSpecificCar || lockStartingRating;
  const disableSourceObtain = isSpecificCar || (lockStartingObtain && (carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomRatings"));
  const disableAttrRating = carOptions?.unlockMode === "baseGame" || carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomUnlock";
  const disableAttrObtain = lockStartingObtain || carOptions?.unlockMode === "baseGame" || carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomRatings";
  const hasCustomUnlock = isCustomUnlockMethod(rowState.attrObtain);
  const customUnlockSelectionLabel = getCustomUnlockSelectionLabel(rowState.customUnlock, trackByFolder);
  const defaultAttrObtainOptions = ATTR_OBTAINS_LIST.filter(opt => opt.val === "Random" || opt.val === "Unchanged");
  const baseGameAttrObtainOptions = ATTR_OBTAINS_LIST.filter(opt =>
    opt.val !== "Random" &&
    opt.val !== "Unchanged" &&
    !isCustomUnlockMethod(opt.val)
  );
  const customUnlockAttrObtainOptions = ATTR_OBTAINS_LIST.filter(opt => isCustomUnlockMethod(opt.val));

  return (
    <div className={`spec-grid-row${onRemove ? " has-remove" : ""}`}>
      <div className="car-id">{id}</div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select 
            value={rowState.sourcePool} 
            onChange={e => {
              const val = e.target.value;
              if (val === "Specific Car") {
                onOpenSearch(index);
              } else {
                updateRow(index, { sourcePool: val });
              }
            }}
            disabled={disableSourcePool}
          >
            {sourcePoolOptionsJSX}
            {!isGeneralPool && (
              <optgroup label="Current Selection">
                <option value={rowState.sourcePool}>
                  {specificCar ? specificCar.name : rowState.sourcePool}
                </option>
              </optgroup>
            )}
            <option value="Specific Car">Specific Car...</option>
          </select>
        </div>
        <div className="field-group">
          <select 
            value={isSpecificCar ? specificCar.rating.toString() : rowState.sourceRating} 
            onChange={e => updateRow(index, { sourceRating: e.target.value })} 
            disabled={disableSourceRating}
          >
            {isSpecificCar ? (
              <option value={specificCar.rating.toString()}>{CAR_RATINGS[specificCar.rating] || "Unknown"}</option>
            ) : (
              RATINGS_LIST.map(opt => (
                <option key={opt.val} value={opt.val} disabled={validOptions && !validOptions.ratings.has(opt.val)}>
                  {opt.label}
                </option>
              ))
            )}
          </select>
        </div>
        <div className="field-group">
          <select 
            value={isSpecificCar ? specificCar.obtainMethod.toString() : rowState.sourceObtain} 
            onChange={e => updateRow(index, { sourceObtain: e.target.value })} 
            disabled={disableSourceObtain}
          >
            {isSpecificCar ? (
              <option value={specificCar.obtainMethod.toString()}>{OBTAIN_METHODS[specificCar.obtainMethod] || "Unknown"}</option>
            ) : (
              OBTAINS_LIST.map(opt => (
                <option key={opt.val} value={opt.val} disabled={validOptions && !validOptions.obtains.has(opt.val)}>
                  {opt.label}
                </option>
              ))
            )}
          </select>
        </div>
      </div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select value={rowState.attrRating} onChange={e => updateRow(index, { attrRating: e.target.value })}
            disabled={disableAttrRating}>
            {ATTR_RATINGS_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
          </select>
        </div>
        <div className="field-group">
          <select
            value={hasCustomUnlock ? CURRENT_CUSTOM_UNLOCK : rowState.attrObtain}
            onChange={e => {
              const val = e.target.value;
              if (val === CURRENT_CUSTOM_UNLOCK) return;

              updateRow(index, { attrObtain: val });
              if (isCustomUnlockMethod(val)) {
                onOpenCustomUnlock(index);
              }
            }}
            disabled={disableAttrObtain}
          >
            {defaultAttrObtainOptions.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
            <optgroup label="Base Game">
              {baseGameAttrObtainOptions.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))}
            </optgroup>
            <optgroup label="Custom Unlocks">
              {customUnlockAttrObtainOptions.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))}
            </optgroup>
            {hasCustomUnlock && (
              <optgroup label="Current Selection">
                <option value={CURRENT_CUSTOM_UNLOCK}>{customUnlockSelectionLabel}</option>
              </optgroup>
            )}
          </select>
        </div>
      </div>
      {onRemove && (
        <button
          type="button"
          className="car-remove-button"
          onClick={() => onRemove(index)}
          aria-label={`Remove ${id}`}
          title="Remove car slot"
        >
          ✕
        </button>
      )}
    </div>
  );
};