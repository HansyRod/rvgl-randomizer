import { useState, useMemo, useEffect } from "react";
import "./CarsFullSpecTab.css";
import { CAR_RATINGS, OBTAIN_METHODS } from "../utils/constants";

const STOCK_CARS = [
  "rc", "mite", "phat", "moss", "mud", "beatall", "volken",
  "tc6", "dino", "candy", "gencar", "tc4", "mouse", "flag",
  "tc2", "r5", "tc5", "sgt", "tc3", "adeon", "fone",
  "tc1", "rotor", "cougar", "sugo", "toyeca", "amw", "panga"
];

const DC_CARS = [
  "bigvolt", "bossvolt", "jg6rc", "tc12", "tc10", "tc8", "tc11",
  "tc9", "jg1jg7", "tc7", "jg3loco", "jg4snw35", "jg5purpxl", "jg2fulonx"
];

const RATINGS_LIST = [
  { val: "Random", label: "Random" },
  { val: "0", label: "Rookie" },
  { val: "1", label: "Amateur" },
  { val: "2", label: "Advanced" },
  { val: "3", label: "Semi-Pro" },
  { val: "4", label: "Pro" },
  { val: "5", label: "Super Pro" }
];

const ATTR_RATINGS_LIST = [
  { val: "Random", label: "Random" },
  { val: "Unchanged", label: "Unchanged" },
  { val: "0", label: "Rookie" },
  { val: "1", label: "Amateur" },
  { val: "2", label: "Advanced" },
  { val: "3", label: "Semi-Pro" },
  { val: "4", label: "Pro" },
  { val: "5", label: "Super Pro" }
];

const OBTAINS_LIST = [
  { val: "Random", label: "Random" },
  { val: "0", label: "Starting Car" },
  { val: "1", label: "Championship" },
  { val: "2", label: "Time Trial" },
  { val: "3", label: "Practice" },
  { val: "4", label: "Single Race" },
  { val: "-1", label: "Cheat Only" }
];

const ATTR_OBTAINS_LIST = [
  { val: "Random", label: "Random" },
  { val: "Unchanged", label: "Unchanged" },
  { val: "0", label: "Starting Car" },
  { val: "1", label: "Championship" },
  { val: "2", label: "Time Trial" },
  { val: "3", label: "Practice" },
  { val: "4", label: "Single Race" },
  { val: "-1", label: "Cheat Only" }
];

export default function CarsFullSpecTab({ scanResult, specState, setSpecState }) {
  const [presetSelection, setPresetSelection] = useState("Full Random");

  const availableCars = useMemo(() => {
    if (!scanResult) return [];
    let cars = [];
    if (scanResult.installType === "classic") {
      cars = scanResult.cars || [];
    } else {
      cars = (scanResult.contentPacks || [])
        .filter(p => p.useCars)
        .flatMap(p => p.cars);
    }
    return cars.filter(c => !c.isSystemCar && c.hasValidFile);
  }, [scanResult]);

  const carByFolder = useMemo(() => {
    const map = {};
    for (const c of availableCars) {
      map[c.folderName] = c;
    }
    return map;
  }, [availableCars]);

  const availablePools = useMemo(() => {
    const pools = new Set(availableCars.map(c => c.pool)); // pool: "stock", "dc", "custom"
    return pools;
  }, [availableCars]);

  const activePacks = useMemo(() => {
    if (!scanResult || scanResult.installType === "classic") return [];
    return (scanResult.contentPacks || []).filter(p => p.useCars).map(p => p.name);
  }, [scanResult]);

  // Derived options based on availability
  const sourcePoolOptions = useMemo(() => {
    const options = [];
    options.push({ value: "Full Random", label: "Full Random", group: "General" });
    
    if (availablePools.has("stock")) options.push({ value: "Stock", label: "Stock Pool", group: "General" });
    if (availablePools.has("dc")) options.push({ value: "DC", label: "DC Pool", group: "General" });
    if (availablePools.has("custom")) options.push({ value: "Custom", label: "Custom Pool", group: "General" });

    for (const pack of activePacks) {
      options.push({ value: `Pack:${pack}`, label: `Pack: ${pack}`, group: "Content Packs" });
    }

    const sortedCars = [...availableCars].sort((a,b) => a.name.localeCompare(b.name));
    for (const car of sortedCars) {
      options.push({ value: car.folderName, label: car.name, group: "Specific Cars" });
    }

    return options;
  }, [availablePools, activePacks, availableCars]);

  // Initialize or handle preset
  const applyPreset = (preset) => {
    const newState = {
      stockCars: STOCK_CARS.map(id => ({
        id,
        sourcePool: preset === "Original Content" ? (carByFolder[id] ? id : "Full Random") : "Full Random",
        sourceRating: "Random",
        sourceObtain: "Random",
        attrRating: preset === "Original Content" ? "Unchanged" : "Random",
        attrObtain: preset === "Original Content" ? "Unchanged" : "Random"
      })),
      dcCars: DC_CARS.map(id => ({
        id,
        sourcePool: preset === "Original Content" ? (carByFolder[id] ? id : "Full Random") : "Full Random",
        sourceRating: "Random",
        sourceObtain: "Random",
        attrRating: preset === "Original Content" ? "Unchanged" : "Random",
        attrObtain: preset === "Original Content" ? "Unchanged" : "Random"
      }))
    };
    setSpecState(newState);
  };

  useEffect(() => {
    if (!specState) {
      applyPreset("Full Random");
    }
  }, [specState]);

  if (!specState || !specState.stockCars || !specState.dcCars) return null;

  const updateRow = (category, index, updates) => {
    setSpecState(prev => {
      const newCategory = [...prev[category]];
      newCategory[index] = { ...newCategory[index], ...updates };
      return {
        ...prev,
        [category]: newCategory
      };
    });
  };

  const getValidRatingsForPool = (poolVal) => {
    if (poolVal === "Full Random") return new Set(RATINGS_LIST.map(r => r.val));
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
    ratings.add("Random");
    return ratings;
  };

  const getValidObtainsForPool = (poolVal) => {
    if (poolVal === "Full Random") return new Set(OBTAINS_LIST.map(r => r.val));
    let matchingCars = [];
    if (poolVal === "Stock") matchingCars = availableCars.filter(c => c.pool === "stock");
    else if (poolVal === "DC") matchingCars = availableCars.filter(c => c.pool === "dc");
    else if (poolVal === "Custom") matchingCars = availableCars.filter(c => c.pool === "custom");
    else if (poolVal.startsWith("Pack:")) {
      const packName = poolVal.split(":")[1];
      const pack = (scanResult.contentPacks || []).find(p => p.name === packName);
      if (pack) matchingCars = pack.cars;
    }
    
    const obtains = new Set(matchingCars.map(c => c.obtainMethod.toString()));
    obtains.add("Random");
    return obtains;
  };

  const renderRow = (category, index) => {
    const rowState = specState[category][index];
    if (!rowState) return null;

    const id = rowState.id;
    const isSpecificCar = carByFolder[rowState.sourcePool] !== undefined;
    const specificCar = isSpecificCar ? carByFolder[rowState.sourcePool] : null;

    let validRatings = null;
    let validObtains = null;
    if (!isSpecificCar) {
      validRatings = getValidRatingsForPool(rowState.sourcePool);
      validObtains = getValidObtainsForPool(rowState.sourcePool);
    }

    return (
      <div key={id} className="spec-grid-row">
        <div className="car-id">{id}</div>
        <div className="specs-horizontal">
          <div className="field-group">
            <select 
              value={rowState.sourcePool} 
              onChange={e => updateRow(category, index, { sourcePool: e.target.value })}
            >
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
            <select 
              value={isSpecificCar ? specificCar.rating.toString() : rowState.sourceRating} 
              onChange={e => updateRow(category, index, { sourceRating: e.target.value })}
              disabled={isSpecificCar}
            >
              {isSpecificCar ? (
                 <option value={specificCar.rating.toString()}>{CAR_RATINGS[specificCar.rating] || "Unknown"}</option>
              ) : (
                 RATINGS_LIST.map(opt => (
                   <option 
                     key={opt.val} 
                     value={opt.val} 
                     disabled={validRatings && !validRatings.has(opt.val)}
                   >
                     {opt.label}
                   </option>
                 ))
              )}
            </select>
          </div>
          <div className="field-group">
            <select 
              value={isSpecificCar ? specificCar.obtainMethod.toString() : rowState.sourceObtain} 
              onChange={e => updateRow(category, index, { sourceObtain: e.target.value })}
              disabled={isSpecificCar}
            >
              {isSpecificCar ? (
                 <option value={specificCar.obtainMethod.toString()}>{OBTAIN_METHODS[specificCar.obtainMethod] || "Unknown"}</option>
              ) : (
                 OBTAINS_LIST.map(opt => (
                   <option 
                     key={opt.val} 
                     value={opt.val} 
                     disabled={validObtains && !validObtains.has(opt.val)}
                   >
                     {opt.label}
                   </option>
                 ))
              )}
            </select>
          </div>
        </div>
        <div className="specs-horizontal">
          <div className="field-group">
            <select 
              value={rowState.attrRating} 
              onChange={e => updateRow(category, index, { attrRating: e.target.value })}
            >
              {ATTR_RATINGS_LIST.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))}
            </select>
          </div>
          <div className="field-group">
            <select 
              value={rowState.attrObtain} 
              onChange={e => updateRow(category, index, { attrObtain: e.target.value })}
            >
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
    <div className="cars-full-spec">
      <div className="presets-row">
        <label>Presets:</label>
        <select value={presetSelection} onChange={e => setPresetSelection(e.target.value)}>
          <option value="Full Random">Full Random</option>
          <option value="Original Content">Original Content</option>
        </select>
        <button className="primary" onClick={() => applyPreset(presetSelection)}>Apply</button>
      </div>

      <div className="cars-spec-section">
        <h2>Stock Cars</h2>
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
          {specState.stockCars.map((_, index) => renderRow("stockCars", index))}
        </div>
      </div>

      <div className="cars-spec-section">
        <h2>DC Cars</h2>
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
              <div className="specs-horizontal" style={{ color: "var(--text-secondary)", fontSize: "0.65rem", fontWeight: "normal", textTransform: "none" }}>
                <div style={{ flex: 1 }}>Rating</div>
                <div style={{ flex: 1 }}>Obtain</div>
              </div>
            </div>
          </div>
          {specState.dcCars.map((_, index) => renderRow("dcCars", index))}
        </div>
      </div>
    </div>
  );
}
