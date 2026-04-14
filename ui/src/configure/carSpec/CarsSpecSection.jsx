import { useState, useMemo, memo, useCallback } from "react";
import "./CarsFullSpecTab.css";
import { RATINGS_LIST, OBTAINS_LIST } from "../../utils/constants";
import CarSpecRow from "./CarSpecRow";
import CarSearchModal from "./CarSearchModal";
import { useAppContext } from "../../AppProvider";

const SpecRow = memo(CarSpecRow);

export default function CarsSpecSection({title, categoryKey, includeKey, defaultCarsList}) {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure } = state;
  
  // Destructure individual variables
  const { scanResult } = setup;
  const { carOptions, carsSpecState } = configure;
  
  const [presetSelection, setPresetSelection] = useState("Full Random");
  const [searchModalRow, setSearchModalRow] = useState(null);

  const availableCars = useMemo(() => {
    if (!scanResult) return [];
    let cars;
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

  const isEnabled = carsSpecState[includeKey] !== false;
  const startingCarsActive =
    categoryKey === "stockCars" &&
    carOptions?.enableStartingCars &&
    (carOptions?.unlockMode === "random" || carOptions?.unlockMode === "randomUnlock") &&
    (carOptions?.numStartingCars || 0) > 0;
  const startingCount = startingCarsActive ? (carOptions?.numStartingCars || 0) : 0;

  const applyPreset = (preset) => {

    const currentList = carsSpecState[categoryKey] || [];
    const newList = defaultCarsList.map((id, index) => {
      const currentCar = currentList[index] || {};
      const isBaseGame = carOptions?.unlockMode === "baseGame";
      const isUnchanged = carOptions?.unlockMode === "unchanged";
      const isStartingSlot =
        categoryKey === "stockCars" &&
        carOptions?.enableStartingCars &&
        (carOptions?.numStartingCars || 0) > 0 &&
        index < (carOptions?.numStartingCars || 0);
      const preserveStartingPool = isStartingSlot && !!carOptions?.enableStartingCarsPool;
      const preserveStartingRating = isStartingSlot && !!carOptions?.enableStartingCarsRating;

      return {
        id,
        sourcePool: preserveStartingPool
          ? (currentCar.sourcePool ?? carOptions?.startingCarsPool ?? "Full Random")
          : (preset === "Original Content" ? (carByFolder[id] ? id : "Full Random") : "Full Random"),
        sourceRating: preserveStartingRating
          ? (currentCar.sourceRating ?? carOptions?.startingCarsRating ?? "Random")
          : "Random",
        sourceObtain: "Random",
        attrRating: isBaseGame 
          ? (currentCar.attrRating ?? "Random")
          : (preset === "Original Content" ? "Unchanged" : "Random"),
        attrObtain: (isBaseGame || isUnchanged)
          ? (currentCar.attrObtain ?? "Random")
          : (preset === "Original Content" ? "Unchanged" : "Random")
      };
    });

    updateCategoryCtx("configure", { carsSpecState: { ...carsSpecState, [categoryKey]: newList } });
  };

  const updateRow = useCallback((index, updates) => {
    const newCategory = [...carsSpecState[categoryKey]];
    newCategory[index] = { ...newCategory[index], ...updates };

    const nextState = {
      ...carsSpecState,
      [categoryKey]: newCategory,
    };
    updateCategoryCtx("configure", { carsSpecState: nextState });
  }, [carsSpecState, categoryKey, updateCategoryCtx]);

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
      {categoryKey === "stockCars" && carOptions?.enableStartingCars && (carOptions?.numStartingCars || 0) > 0 && (
        <div className="section-lock-info">
          🔒 <strong>Starting Car Configuration is active</strong> — first <em>{carOptions.numStartingCars}</em> stock slots have locked obtain (Starting Car){carOptions?.enableStartingCarsPool ? ", pool locked by Car Options" : ""}{carOptions?.enableStartingCarsRating ? ", rating locked by Car Options" : ""}.
        </div>
      )}

      <div style={{ marginBottom: "1rem" }}>
        <label style={{ display: "flex", alignItems: "center", gap: "0.5rem", fontWeight: "bold", fontSize: "1.1rem" }}>
          <input
            type="checkbox"
            checked={isEnabled}
            onChange={e => updateCategoryCtx("configure", { carsSpecState: { ...carsSpecState, [includeKey]: e.target.checked } })}
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
            {carsSpecState[categoryKey].map((row, index) => (
              // Starting car locks only apply to stock slots in the configured range.
              (() => {
                const isStartingSlot = startingCount > 0 && index < startingCount;
                return (
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
                lockStartingPool={isStartingSlot && !!carOptions?.enableStartingCarsPool}
                lockStartingRating={isStartingSlot && !!carOptions?.enableStartingCarsRating}
                lockStartingObtain={isStartingSlot}
              />
                );
              })()
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}