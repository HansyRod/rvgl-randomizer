import { useState, useMemo, memo, useCallback, useEffect, useRef } from "react";
import "./CarsFullSpecTab.css";
import {
  CAR_RATINGS, OBTAIN_METHODS, RATINGS_LIST,
  ATTR_RATINGS_LIST, OBTAINS_LIST, ATTR_OBTAINS_LIST
} from "../utils/constants";

const CarSearchModal = ({ isOpen, onClose, onSelect, availableCars }) => {
  const [searchQuery, setSearchQuery] = useState("");
  const inputRef = useRef(null);
  
  useEffect(() => {
    if (isOpen) {
      setSearchQuery("");
      setTimeout(() => inputRef.current?.focus(), 10);
    }
  }, [isOpen]);

  const filteredCars = useMemo(() => {
    const query = searchQuery.toLowerCase();
    if (!query) return availableCars;
    return availableCars.filter(c => 
      c.name.toLowerCase().includes(query) || 
      c.folderName.toLowerCase().includes(query)
    );
  }, [searchQuery, availableCars]);

  if (!isOpen) return null;

  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div className="search-modal-content" onClick={e => e.stopPropagation()}>
        <div className="search-modal-header">
          <h3>Select Specific Car</h3>
          <input 
            ref={inputRef}
            type="text" 
            placeholder="Search car name or folder..." 
            value={searchQuery}
            onChange={e => setSearchQuery(e.target.value)}
            className="search-input"
          />
        </div>
        <div className="search-results-list">
          {filteredCars.length === 0 && <div className="no-results">No cars found...</div>}
          {filteredCars.map(car => (
            <div 
              key={car.folderName} 
              className="search-result-item"
              onClick={() => { onSelect(car.folderName); onClose(); }}
            >
              <span className="car-name">{car.name}</span>
              <span className="car-folder">({car.folderName})</span>
            </div>
          ))}
        </div>
        <div className="search-modal-footer">
          <button onClick={onClose}>Cancel</button>
        </div>
      </div>
    </div>
  );
};

const SpecRow = memo(({ 
  index, 
  rowState, 
  updateRow, 
  carByFolder, 
  sourcePoolOptionsJSX, 
  poolValidOptions,
  carOptions,
  onOpenSearch
}) => {
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

  return (
    <div className="spec-grid-row">
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
            disabled={isSpecificCar}
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
            disabled={isSpecificCar}
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
            disabled={carOptions?.unlockMode === "baseGame" || carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomUnlock"}>
            {ATTR_RATINGS_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
          </select>
        </div>
        <div className="field-group">
          <select value={rowState.attrObtain} onChange={e => updateRow(index, { attrObtain: e.target.value })}
            disabled={carOptions?.unlockMode === "baseGame" || carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomRatings"} >
            {ATTR_OBTAINS_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
          </select>
        </div>
      </div>
    </div>
  );
});

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
  const [searchModalRow, setSearchModalRow] = useState(null);

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

  const sourcePoolOptionsJSX = useMemo(() => {
    const options = [];
    options.push(<option key="Full Random" value="Full Random">Full Random</option>);
    if (availablePools.has("stock")) options.push(<option key="Stock" value="Stock">Stock Pool</option>);
    if (availablePools.has("dc")) options.push(<option key="DC" value="DC">DC Pool</option>);
    if (availablePools.has("custom")) options.push(<option key="Custom" value="Custom">Custom Pool</option>);

    const packOptions = activePacks.map(pack => (
      <option key={`Pack:${pack}`} value={`Pack:${pack}`}>Pack: {pack}</option>
    ));

    return (
      <>
        <optgroup label="General">{options}</optgroup>
        {packOptions.length > 0 && <optgroup label="Content Packs">{packOptions}</optgroup>}
      </>
    );
  }, [availablePools, activePacks]);

  const poolValidOptions = useMemo(() => {
    const map = {};
    const allPools = ["Full Random", "Stock", "DC", "Custom", ...activePacks.map(p => `Pack:${p}`)];

    const getForPool = (poolVal) => {
      if (poolVal === "Full Random") {
        return {
          ratings: new Set(RATINGS_LIST.map(r => r.val)),
          obtains: new Set(OBTAINS_LIST.map(r => r.val))
        };
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

      const ratings = new Set(matchingCars.map(c => c.rating.toString()));
      const obtains = new Set(matchingCars.map(c => c.obtainMethod.toString()));
      ratings.add("Random");
      obtains.add("Random");
      return { ratings, obtains };
    };

    for (const pool of allPools) {
      map[pool] = getForPool(pool);
    }
    return map;
  }, [availableCars, activePacks, scanResult]);

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

  const updateRow = useCallback((index, updates) => {
    setSpecState(prev => {
      const newCategory = [...prev[categoryKey]];
      newCategory[index] = { ...newCategory[index], ...updates };
      return { ...prev, [categoryKey]: newCategory };
    });
  }, [categoryKey, setSpecState]);

  return (
    <div>
      <CarSearchModal 
        isOpen={searchModalRow !== null}
        onClose={() => setSearchModalRow(null)}
        onSelect={(folderName) => updateRow(searchModalRow, { sourcePool: folderName })}
        availableCars={availableCars}
      />
      
      {(carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomUnlock") && (
        <div className="section-lock-info">
          🔒 <strong>Rating column is locked</strong> — Car Options is set to <em>{carOptions.unlockMode === "unchanged" ? "Unchanged" : "Random Unlock Criteria"}</em>.
        </div>
      )}

      {(carOptions?.unlockMode === "unchanged" || carOptions?.unlockMode === "randomRatings") && (
        <div className="section-lock-info">
          🔒 <strong>Obtain column is locked</strong> — Car Options is set to <em>{carOptions.unlockMode === "unchanged" ? "Unchanged" : "Random Ratings"}</em>.
        </div>
      )}

      {carOptions?.unlockMode === "baseGame" && (
        <div className="section-lock-info">
          🔒 <strong>Attributes are locked</strong> — Car Options is set to <em>Base Game Distribution</em>.
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
                <div className="specs-horizontal-header">
                  <div style={{ flex: 1 }}>Pool</div>
                  <div style={{ flex: 1 }}>Rating</div>
                  <div style={{ flex: 1 }}>Obtain</div>
                </div>
              </div>
              <div className="column-group">
                <div className="column-group-title">Attributes</div>
                <div className="specs-horizontal-header">
                  <div style={{ flex: 1 }}>Rating</div>
                  <div style={{ flex: 1 }}>Obtain</div>
                </div>
              </div>
            </div>
            {specState[categoryKey].map((row, index) => (
              <SpecRow
                key={row.id}
                index={index}
                rowState={row}
                updateRow={updateRow}
                carByFolder={carByFolder}
                sourcePoolOptionsJSX={sourcePoolOptionsJSX}
                poolValidOptions={poolValidOptions}
                carOptions={carOptions}
                onOpenSearch={setSearchModalRow}
              />
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}