import { useState, useMemo } from "react";
import "./CarsFullSpecTab.css";
import {
  CAR_RATINGS, OBTAIN_METHODS, RATINGS_LIST,
  ATTR_RATINGS_LIST, OBTAINS_LIST, ATTR_OBTAINS_LIST
} from "../utils/constants";

export default function CarsSpecSection({
  title,
  categoryKey,
  includeKey,
  defaultCarsList,
  scanResult,
  specState,
  setSpecState,
  carOptions
}) {
  const [presetSelection, setPresetSelection] = useState("Full Random");

  const availableCars = useMemo(() => {
    if (!scanResult) return [];
    let cars = [];
    if (scanResult.installType === "classic") {
      cars = scanResult.cars || [];
    } else {
      cars = (scanResult.contentPacks || []).filter(p => p.useCars).flatMap(p => p.cars);
    }
    return cars.filter(c => !c.isSystemCar && c.hasValidFile);
  }, [scanResult]);

  const carByFolder = useMemo(() => {
    const map = {};
    for (const c of availableCars) map[c.folderName] = c;
    return map;
  }, [availableCars]);

  const availablePools = useMemo(() => new Set(availableCars.map(c => c.pool)), [availableCars]);

  const activePacks = useMemo(() => {
    if (!scanResult || scanResult.installType === "classic") return [];
    return (scanResult.contentPacks || []).filter(p => p.useCars).map(p => p.name);
  }, [scanResult]);

  const sourcePoolOptions = useMemo(() => {
    const options = [{ value: "Full Random", label: "Full Random", group: "General" }];

    if (availablePools.has("stock")) options.push({ value: "Stock", label: "Stock Pool", group: "General" });
    if (availablePools.has("dc")) options.push({ value: "DC", label: "DC Pool", group: "General" });
    if (availablePools.has("custom")) options.push({ value: "Custom", label: "Custom Pool", group: "General" });

    for (const pack of activePacks) {
      options.push({ value: `Pack:${pack}`, label: `Pack: ${pack}`, group: "Content Packs" });
    }

    const sortedCars = [...availableCars].sort((a, b) => a.name.localeCompare(b.name));
    for (const car of sortedCars) {
      options.push({ value: car.folderName, label: car.name, group: "Specific Cars" });
    }
    return options;
  }, [availablePools, activePacks, availableCars]);

  const isEnabled = specState[includeKey] !== false;

  const applyPreset = (preset) => {
    setSpecState(prev => {
      const currentList = prev[categoryKey] || [];

      return {
        ...prev,
        [categoryKey]: defaultCarsList.map((id, index) => {
          const currentCar = currentList[index] || {};
          const isBaseGame = carOptions?.unlockMode === "baseGame";
          const isUnchanged = carOptions?.unlockMode === "unchanged";

          return {
            id,
            sourcePool: preset === "Original Content" ? (carByFolder[id] ? id : "Full Random") : "Full Random",
            sourceRating: "Random",
            sourceObtain: "Random",
            attrRating: isBaseGame 
              ? (currentCar.attrRating ?? "Random")
              : (preset === "Original Content" ? "Unchanged" : "Random"),
            attrObtain: (isBaseGame || isUnchanged)
              ? (currentCar.attrObtain ?? "Random")
              : (preset === "Original Content" ? "Unchanged" : "Random")
          };
        })
      };
    });
  };

  const updateRow = (index, updates) => {
    setSpecState(prev => {
      const newCategory = [...prev[categoryKey]];
      newCategory[index] = { ...newCategory[index], ...updates };
      return { ...prev, [categoryKey]: newCategory };
    });
  };

  const getValidOptionsForPool = (poolVal, field) => {
    if (poolVal === "Full Random") {
      return field === "rating"
        ? new Set(RATINGS_LIST.map(r => r.val))
        : new Set(OBTAINS_LIST.map(r => r.val));
    }

    let matchingCars = [];
    if (poolVal === "Stock") matchingCars = availableCars.filter(c => c.pool === "stock");
    else if (poolVal === "DC") matchingCars = availableCars.filter(c => c.pool === "dc");
    else if (poolVal === "Custom") matchingCars = availableCars.filter(c => c.pool === "custom");
    else if (poolVal.startsWith("Pack:")) {
      const packName = poolVal.split(":")[1];
      const pack = (scanResult.contentPacks || []).find(p => p.name === packName);
      if (pack) matchingCars = pack.cars;
    }

    const validSet = new Set(matchingCars.map(c =>
      field === "rating" ? c.rating.toString() : c.obtainMethod.toString()
    ));
    validSet.add("Random");
    return validSet;
  };

  const renderRow = (index) => {
    const rowState = specState[categoryKey][index];
    if (!rowState) return null;

    const id = rowState.id;
    const isSpecificCar = carByFolder[rowState.sourcePool] !== undefined;
    const specificCar = isSpecificCar ? carByFolder[rowState.sourcePool] : null;

    let validRatings = null;
    let validObtains = null;
    if (!isSpecificCar) {
      validRatings = getValidOptionsForPool(rowState.sourcePool, "rating");
      validObtains = getValidOptionsForPool(rowState.sourcePool, "obtain");
    }

    return (
      <div key={id} className="spec-grid-row">
        <div className="car-id">{id}</div>
        <div className="specs-horizontal">
          <div className="field-group">
            <select value={rowState.sourcePool} onChange={e => updateRow(index, { sourcePool: e.target.value })}>
              {[...new Set(sourcePoolOptions.map(o => o.group))].map(group => (
                <optgroup label={group} key={group}>
                  {sourcePoolOptions.filter(o => o.group === group).map(opt => (
                    <option key={opt.value} value={opt.value}>{opt.label}</option>
                  ))}
                </optgroup>
              ))}
            </select>
          </div>
          <div className="field-group">
            <select value={isSpecificCar ? specificCar.rating.toString() : rowState.sourceRating} onChange={e => updateRow(index, { sourceRating: e.target.value })} disabled={isSpecificCar}>
              {isSpecificCar ? (
                <option value={specificCar.rating.toString()}>{CAR_RATINGS[specificCar.rating] || "Unknown"}</option>
              ) : (
                RATINGS_LIST.map(opt => (
                  <option key={opt.val} value={opt.val} disabled={validRatings && !validRatings.has(opt.val)}>
                    {opt.label}
                  </option>
                ))
              )}
            </select>
          </div>
          <div className="field-group">
            <select value={isSpecificCar ? specificCar.obtainMethod.toString() : rowState.sourceObtain} onChange={e => updateRow(index, { sourceObtain: e.target.value })} disabled={isSpecificCar}>
              {isSpecificCar ? (
                <option value={specificCar.obtainMethod.toString()}>{OBTAIN_METHODS[specificCar.obtainMethod] || "Unknown"}</option>
              ) : (
                OBTAINS_LIST.map(opt => (
                  <option key={opt.val} value={opt.val} disabled={validObtains && !validObtains.has(opt.val)}>
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
              disabled={carOptions?.unlockMode === "baseGame"}>
              {ATTR_RATINGS_LIST.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))}
            </select>
          </div>
          <div className="field-group">
            <select value={rowState.attrObtain} onChange={e => updateRow(index, { attrObtain: e.target.value })}
              disabled={carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "baseGame"} >
              {ATTR_OBTAINS_LIST.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))}
            </select>
          </div>
        </div>
      </div>
    );
  };

  return (
    <div>

      {carOptions?.unlockMode === "unchanged" && (
        <div style={{
          padding: "0.5rem 1rem", marginBottom: "1rem", borderRadius: "6px",
          border: "1px solid var(--accent)", background: "var(--tab-active-bg)",
          fontSize: "0.82rem", color: "var(--text-secondary)",
        }}>
          🔒 <strong>Obtain column is locked</strong> — Car Options is set to <em>Unchanged</em>.
          Switch to a different mode in the Car Options tab to re-enable it.
        </div>
      )}

      {carOptions?.unlockMode === "baseGame" && (
        <div style={{
          padding: "0.5rem 1rem", marginBottom: "1rem", borderRadius: "6px",
          border: "1px solid var(--accent)", background: "var(--tab-active-bg)",
          fontSize: "0.82rem", color: "var(--text-secondary)",
        }}>
          🔒 <strong>Attributes are locked</strong> — Car Options is set to <em>Base Game Distribution</em>.
          The Rating and Obtain columns follow fixed rules for each slot.
        </div>
      )}

      <div style={{ marginBottom: "1rem" }}>
        <label style={{ display: "flex", alignItems: "center", gap: "0.5rem", fontWeight: "bold", fontSize: "1.1rem" }}>
          <input
            type="checkbox"
            checked={isEnabled}
            onChange={e => setSpecState(prev => ({ ...prev, [includeKey]: e.target.checked }))}
            style={{ width: "1.2rem", height: "1.2rem" }}
          />
          Include {title} in Randomization
        </label>
      </div>

      <div className="cars-full-spec" style={{ opacity: isEnabled ? 1 : 0.5, pointerEvents: isEnabled ? "auto" : "none" }}>
        <div className="presets-row">
          <label>Presets:</label>
          <select value={presetSelection} onChange={e => setPresetSelection(e.target.value)}>
            <option value="Full Random">Full Random</option>
            <option value="Original Content">Original Content</option>
          </select>
          <button className="primary" onClick={() => applyPreset(presetSelection)}>Apply</button>
        </div>

        <div className="cars-spec-section">
          <h2>{title}</h2>
          <div className="spec-grid">
            <div className="spec-grid-header">
              <div style={{ display: "flex", alignItems: "center" }}>Target Slot</div>
              <div className="column-group">
                <div className="column-group-title">Car Choice</div>
                <div className="specs-horizontal" style={{ color: "var(--text-secondary)", fontSize: "0.85rem", fontWeight: "normal", textTransform: "none" }}>
                  <div style={{ flex: 1 }}>Pool</div>
                  <div style={{ flex: 1 }}>Rating</div>
                  <div style={{ flex: 1 }}>Obtain</div>
                </div>
              </div>
              <div className="column-group">
                <div className="column-group-title">Attributes</div>
                <div className="specs-horizontal" style={{ color: "var(--text-secondary)", fontSize: "0.85rem", fontWeight: "normal", textTransform: "none" }}>
                  <div style={{ flex: 1 }}>Rating</div>
                  <div style={{ flex: 1 }}>Obtain</div>
                </div>
              </div>
            </div>
            {specState[categoryKey].map((_, index) => renderRow(index))}
          </div>
        </div>
      </div>
    </div>
  );
}