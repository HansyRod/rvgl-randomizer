import { useState, useMemo, memo, useCallback } from "react";
import "./CarsFullSpecTab.css";
import { RATINGS_LIST, OBTAINS_LIST, makeDefaultCarSpec } from "../../utils/constants";
import { normalizeCustomUnlockRow } from "../../utils/customUnlockState";
import { getPlayableCarsFromScan, getPlayableTracksFromScan, indexByFolder } from "../../utils/scanContent";
import CarSpecRow from "./CarSpecRow";
import CarSearchModal from "./CarSearchModal";
import CustomUnlockModal from "../customUnlocks/CustomUnlockModal";
import { useAppContext } from "../../AppProvider";
import {
  applyModeRules,
  applyStartingCarOverrides,
  getIncludedCarSlotCounts,
  normalizeExtraCarRowIds,
  resetStartingCarOverrides,
} from "../carOptions/CarOptionsUtils";

const SpecRow = memo(CarSpecRow);

export default function CarsSpecSection({title, categoryKey, includeKey, isDynamic = false}) {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure } = state;
  
  // Destructure individual variables
  const { scanResult } = setup;
  const { carOptions, carsSpecState } = configure;
  
  const [searchModalRow, setSearchModalRow] = useState(null);
  const [customUnlockModalRow, setCustomUnlockModalRow] = useState(null);

  const availableCars = useMemo(() => getPlayableCarsFromScan(scanResult), [scanResult]);
  const carByFolder = useMemo(() => indexByFolder(availableCars), [availableCars]);
  const availableTracks = useMemo(() => getPlayableTracksFromScan(scanResult), [scanResult]);
  const trackByFolder = useMemo(() => indexByFolder(availableTracks), [availableTracks]);

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

  const isEnabled = includeKey ? carsSpecState[includeKey] !== false : true;
  const categoryRows = carsSpecState?.[categoryKey] || [];
  const slotCounts = getIncludedCarSlotCounts(carsSpecState);
  const categoryOffsets = {
    stockCars: 0,
    dcCars: slotCounts.stock,
    extraCars: slotCounts.stock + slotCounts.dc,
  };
  const categoryOffset = categoryOffsets[categoryKey] || 0;
  const startingCarsActive =
    carOptions?.enableStartingCars &&
    carOptions?.unlockMode !== "baseGame" &&
    (carOptions?.numStartingCars || 0) > 0;
  const startingCount = startingCarsActive ? (carOptions?.numStartingCars || 0) : 0;
  const startingRowsInCategory = Math.max(
    0,
    Math.min(startingCount - categoryOffset, categoryRows.length)
  );

  const isStartingGlobalIndex = (globalIndex) =>
    startingCarsActive && globalIndex < startingCount;

  const normalizeExtraRowForCurrentMode = (row, globalIndex) => {
    let out = row;
    if (carOptions?.unlockMode !== "baseGame") {
      out = applyModeRules(row, globalIndex, carOptions.unlockMode, carOptions);
      if (isStartingGlobalIndex(globalIndex)) {
        out = applyStartingCarOverrides(out, carOptions);
      }
    }
    return normalizeCustomUnlockRow(out);
  };

  const addExtraCar = () => {
    const nextRow = normalizeExtraRowForCurrentMode(
      makeDefaultCarSpec(`extra-${categoryRows.length + 1}`),
      categoryOffset + categoryRows.length
    );
    const nextRows = normalizeExtraCarRowIds([...categoryRows, nextRow]);
    updateCategoryCtx("configure", {
      carsSpecState: {
        ...carsSpecState,
        [categoryKey]: nextRows,
      },
    });
  };

  const removeExtraCar = useCallback((index) => {
    const nextRows = categoryRows.filter((_, rowIndex) => rowIndex !== index);
    const oldIndexByRow = new Map(categoryRows.map((row, rowIndex) => [row, rowIndex]));
    const normalizedRows = nextRows.map((row, newIndex) => {
      const oldIndex = oldIndexByRow.get(row);
      if (oldIndex === undefined) return row;

      const wasStarting = isStartingGlobalIndex(categoryOffset + oldIndex);
      const isStarting = isStartingGlobalIndex(categoryOffset + newIndex);
      if (wasStarting === isStarting) return row;

      const normalizedRow = isStarting
        ? applyStartingCarOverrides(row, carOptions)
        : resetStartingCarOverrides(row, carOptions);
      return normalizeCustomUnlockRow(normalizedRow);
    });
    const renumberedRows = normalizeExtraCarRowIds(normalizedRows);

    updateCategoryCtx("configure", {
      carsSpecState: {
        ...carsSpecState,
        [categoryKey]: renumberedRows,
      },
    });
  }, [carOptions, carsSpecState, categoryKey, categoryOffset, categoryRows, isStartingGlobalIndex, updateCategoryCtx]);

  const updateRow = useCallback((index, updates) => {
    const newCategory = [...carsSpecState[categoryKey]];
    const nextRow = { ...newCategory[index], ...updates };
    newCategory[index] = updates.attrObtain !== undefined
      ? normalizeCustomUnlockRow(nextRow)
      : nextRow;

    const nextState = {
      ...carsSpecState,
      [categoryKey]: newCategory,
    };
    updateCategoryCtx("configure", {
      carsSpecState: nextState,
    });
  }, [carsSpecState, categoryKey, updateCategoryCtx]);

  const customUnlockModalRowState = customUnlockModalRow === null
    ? null
    : carsSpecState?.[categoryKey]?.[customUnlockModalRow];

  return (
    <div>
      <CarSearchModal 
        isOpen={searchModalRow !== null}
        onClose={() => setSearchModalRow(null)}
        onSelect={(folderName) => updateRow(searchModalRow, { sourcePool: folderName })}
        availableCars={availableCars}
      />
      <CustomUnlockModal
        isOpen={customUnlockModalRowState !== null}
        method={customUnlockModalRowState?.attrObtain}
        value={customUnlockModalRowState?.customUnlock}
        availableTracks={availableTracks}
        onClose={() => setCustomUnlockModalRow(null)}
        onSave={(customUnlock) => {
          updateRow(customUnlockModalRow, { customUnlock });
          setCustomUnlockModalRow(null);
        }}
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

      {carOptions?.unlockMode === "baseGame" && categoryKey !== "extraCars" && (
        <div className="section-lock-info">
          🔒 <strong>Attributes are locked</strong> — Car Options is set to <em>Base Game Distribution</em>.
        </div>
      )}
      {startingRowsInCategory > 0 && (
        <div className="section-lock-info">
          🔒 <strong>Starting Car Configuration is active</strong> — <em>{startingRowsInCategory}</em> row{startingRowsInCategory === 1 ? "" : "s"} in this section are locked as Starting Cars{carOptions?.enableStartingCarsPool ? ", pool locked by Car Options" : ""}{carOptions?.enableStartingCarsRating ? ", rating locked by Car Options" : ""}.
        </div>
      )}


      <div className="cars-full-spec" style={{ opacity: isEnabled ? 1 : 0.5, pointerEvents: isEnabled ? "auto" : "none" }}>
        <div className="cars-spec-section">
          <h2>{title}</h2>
          <div className="spec-grid">
            <div className={`spec-grid-header${isDynamic ? " has-remove" : ""}`}>
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
              {isDynamic && <div />}
            </div>
            {categoryRows.map((row, index) => (
              (() => {
                const isStartingSlot = startingCount > 0 && categoryOffset + index < startingCount;
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
                onOpenCustomUnlock={setCustomUnlockModalRow}
                trackByFolder={trackByFolder}
                lockStartingPool={isStartingSlot && !!carOptions?.enableStartingCarsPool}
                lockStartingRating={isStartingSlot && !!carOptions?.enableStartingCarsRating}
                lockStartingObtain={isStartingSlot}
                allowBaseGameAttributeEdits={categoryKey === "extraCars"}
                onRemove={isDynamic ? removeExtraCar : undefined}
              />
                );
              })()
            ))}
          </div>
          {isDynamic && (
            <button type="button" className="primary add-car-slot-button" onClick={addExtraCar}>
              Add Car Slot
            </button>
          )}
        </div>
      </div>
    </div>
  );
}