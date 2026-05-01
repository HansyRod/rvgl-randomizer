import { useMemo } from "react";
import { useAppContext } from "../../AppProvider";
import { RATINGS_LIST, STOCK_CARS, DC_CARS } from "../../utils/constants";
import { getObtainByMode, getRatingByMode } from "./CarOptionsUtils";

export default function StartingCarConfig() {

  const { state, updateCategoryCtx } = useAppContext();
  const { configure: { carOptions, carsSpecState }, setup: { scanResult } } = state;
  
  const {
    enableStartingCars, numStartingCars,
    enableStartingCarsPool, startingCarsPool,
    enableStartingCarsRating, startingCarsRating,
  } = carOptions;

  const includeStockCars = carsSpecState?.includeStockCars !== false;
  const includeDcCars = carsSpecState?.includeDcCars !== false;

  const maxStartingCars = (includeStockCars ? STOCK_CARS.length : 0) +
                          (includeDcCars ? DC_CARS.length : 0);

  // Reapply starting-car overrides to the spec rows that are in the starting range
  function applyStartingOverrides(specState, opts, count) {
    const applyToRows = (rows, catKey, offset) =>
      rows.map((row, i) => {
        const globalIdx = offset + i;
        const isStarting = globalIdx < count;
        let out = { ...row };
        if (isStarting) {
          if (getObtainByMode(opts.unlockMode) === "Random") {
            out.attrObtain = "0";
          } else {
            out.sourceObtain = "0";
          }
          if (opts.enableStartingCarsPool) out.sourcePool = opts.startingCarsPool;
          if (opts.enableStartingCarsRating) out.sourceRating = opts.startingCarsRating;
        }
        return out;
      });

    const stockOffset = 0;
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;

    return {
      ...specState,
      stockCars: applyToRows(specState.stockCars || [], "stockCars", stockOffset),
      dcCars: applyToRows(specState.dcCars || [], "dcCars", dcOffset),
    };
  }

  function resetStartingOverrides(specState, opts, fromIdx, toIdx) {
    const resetRow = (row, globalIdx) => {
      if (globalIdx < fromIdx || globalIdx >= toIdx) return row;
      const out = { ...row };
      // reset forced obtain and rating
      out.attrObtain = getObtainByMode(opts.unlockMode) || "Random";
      out.attrRating = getRatingByMode(opts.unlockMode) || "Random";
      out.sourceObtain = "Random";
      if (opts.enableStartingCarsPool) out.sourcePool = "Full Random";
      if (opts.enableStartingCarsRating) out.sourceRating = "Random";
      return out;
    };

    const stockOffset = 0;
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;

    return {
      ...specState,
      stockCars: (specState.stockCars || []).map((r, i) => resetRow(r, stockOffset + i)),
      dcCars: (specState.dcCars || []).map((r, i) => resetRow(r, dcOffset + i)),
    };
  }

  const commit = (newOpts, newSpec) => {
    updateCategoryCtx("configure", { carOptions: newOpts, carsSpecState: newSpec });
  };

  const handleEnableStartingCars = (checked) => {
    const newOpts = { ...carOptions, enableStartingCars: checked };
    if (checked) {
      const n = Math.max(1, numStartingCars);
      newOpts.numStartingCars = n;
      const newSpec = applyStartingOverrides(carsSpecState, { ...newOpts, numStartingCars: n }, n);
      commit(newOpts, newSpec);
    } else {
      const newSpec = resetStartingOverrides(carsSpecState, newOpts, 0, numStartingCars);
      commit(newOpts, newSpec);
    }
  };

  const handleNumStartingCars = (raw) => {
    const n = Math.max(1, Math.min(maxStartingCars, parseInt(raw) || 1));
    const oldN = numStartingCars;
    const newOpts = { ...carOptions, numStartingCars: n };

    let newSpec = carsSpecState;
    if (n > oldN) {
      // rows [oldN, n) newly enter the starting range
      newSpec = applyStartingOverrides(newSpec, newOpts, n);
    } else {
      // rows [n, oldN) leave the starting range
      newSpec = resetStartingOverrides(newSpec, newOpts, n, oldN);
    }
    commit(newOpts, newSpec);
  };

  const handleEnableStartingCarsPool = (checked) => {
    const newOpts = { ...carOptions, enableStartingCarsPool: checked };
    let newSpec = carsSpecState;
    const applyToRows = (rows, offset) =>
      rows.map((r, i) => {
        if (offset + i >= numStartingCars) return r;
        return { ...r, sourcePool: checked ? startingCarsPool : "Full Random" };
      });
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;
    newSpec = {
      ...newSpec,
      stockCars: applyToRows(newSpec.stockCars || [], 0),
      dcCars: applyToRows(newSpec.dcCars || [], dcOffset),
    };
    commit(newOpts, newSpec);
  };

  const handleStartingCarsPool = (poolValue) => {
    const newOpts = { ...carOptions, startingCarsPool: poolValue };
    if (!enableStartingCarsPool) { commit(newOpts, carsSpecState); return; }
    const applyToRows = (rows, offset) =>
      rows.map((r, i) => {
        if (offset + i >= numStartingCars) return r;
        return { ...r, sourcePool: poolValue };
      });
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;
    const newSpec = {
      ...carsSpecState,
      stockCars: applyToRows(carsSpecState.stockCars || [], 0),
      dcCars: applyToRows(carsSpecState.dcCars || [], dcOffset),
    };
    commit(newOpts, newSpec);
  };

  const handleEnableStartingCarsRating = (checked) => {
    const newOpts = { ...carOptions, enableStartingCarsRating: checked };
    const applyToRows = (rows, offset) =>
      rows.map((r, i) => {
        if (offset + i >= numStartingCars) return r;
        return { ...r, sourceRating: checked ? startingCarsRating : "Random" };
      });
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;
    const newSpec = {
      ...carsSpecState,
      stockCars: applyToRows(carsSpecState.stockCars || [], 0),
      dcCars: applyToRows(carsSpecState.dcCars || [], dcOffset),
    };
    commit(newOpts, newSpec);
  };

  const handleStartingCarsRating = (ratingValue) => {
    const newOpts = { ...carOptions, startingCarsRating: ratingValue };
    if (!enableStartingCarsRating) { commit(newOpts, carsSpecState); return; }
    const applyToRows = (rows, offset) =>
      rows.map((r, i) => {
        if (offset + i >= numStartingCars) return r;
        return { ...r, sourceRating: ratingValue };
      });
    const dcOffset = includeStockCars ? STOCK_CARS.length : 0;
    const newSpec = {
      ...carsSpecState,
      stockCars: applyToRows(carsSpecState.stockCars || [], 0),
      dcCars: applyToRows(carsSpecState.dcCars || [], dcOffset),
    };
    commit(newOpts, newSpec);
  };

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
    return options;
  }, [availablePools, activePacks]);

  return (
    <section className="co-section">
      <h2 className="co-section-title">Starting Car Configuration</h2>
      <p className="co-desc">
        Optionally force the first <em>N</em> car slots to be <strong>Starting Cars</strong>.
        {includeStockCars && includeDcCars && numStartingCars > STOCK_CARS.length && (
          <span> Slots {STOCK_CARS.length + 1}–{numStartingCars} will be DC starting cars.</span>
        )}
      </p>

      <div className="co-checkbox-group" style={{ marginBottom: "1rem" }}>
        <label className="co-checkbox-row">
          <input
            type="checkbox"
            checked={enableStartingCars}
            onChange={e => handleEnableStartingCars(e.target.checked)}
          />
          <span>Enable custom starting car configuration</span>
        </label>
      </div>

      {enableStartingCars && (
        <div className="co-sub-panel">
          <div className="co-starting-grid-row">
            <label className="co-field-label co-starting-grid-label">Number of starting cars</label>
            <input
              type="number"
              min={1}
              max={maxStartingCars}
              value={numStartingCars}
              className="co-number-input co-starting-stepper"
              onChange={e => handleNumStartingCars(e.target.value)}
            />
          </div>

          <div className="co-starting-grid-row">
            <label className="co-checkbox-row co-starting-grid-label">
              <input type="checkbox" checked={enableStartingCarsPool}
                onChange={e => handleEnableStartingCarsPool(e.target.checked)} />
              <span>Starting Cars: Set Source Pool</span>
            </label>
            <select value={startingCarsPool} onChange={e => handleStartingCarsPool(e.target.value)}
              disabled={!enableStartingCarsPool} className="co-starting-select">
              {[...new Set(sourcePoolOptions.map(o => o.group))].map(group => (
                <optgroup label={group} key={group}>
                  {sourcePoolOptions.filter(o => o.group === group).map(opt => (
                    <option key={opt.value} value={opt.value}>{opt.label}</option>
                  ))}
                </optgroup>
              ))}
            </select>
          </div>

          <div className="co-starting-grid-row">
            <label className="co-checkbox-row co-starting-grid-label">
              <input type="checkbox" checked={enableStartingCarsRating}
                onChange={e => handleEnableStartingCarsRating(e.target.checked)} />
              <span>Starting Cars: Set Rating</span>
            </label>
            <select value={startingCarsRating} onChange={e => handleStartingCarsRating(e.target.value)}
              disabled={!enableStartingCarsRating} className="co-starting-select">
              {RATINGS_LIST.map(o => (
                <option key={o.val} value={o.val}>{o.label}</option>
              ))}
            </select>
          </div>
        </div>
      )}
    </section>
  );
}