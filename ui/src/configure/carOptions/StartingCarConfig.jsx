import { useEffect, useMemo } from "react";
import { useAppContext } from "../../AppProvider";
import { RATINGS_LIST } from "../../utils/constants";

export default function StartingCarConfig() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { configure: { carOptions }, setup: { scanResult } } = state;
  
  const {
    enableStartingCars, numStartingCars,
    enableStartingCarsPool, startingCarsPool,
    enableStartingCarsRating, startingCarsRating,
  } = carOptions;

  const set = (key, value) => updateCategoryCtx("configure", { carOptions: { ...carOptions, [key]: value } });

  useEffect(() => {
    if (enableStartingCars && numStartingCars < 1) {
      set("numStartingCars", 1);
    }
  }, [enableStartingCars, numStartingCars]);

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
      </p>

      <div className="co-checkbox-group" style={{ marginBottom: "1rem" }}>
        <label className="co-checkbox-row">
          <input
            type="checkbox"
            checked={enableStartingCars}
            onChange={e => {
              const checked = e.target.checked;
              set("enableStartingCars", checked);
              if (checked && numStartingCars < 1) {
                set("numStartingCars", 1);
              }
            }}
          />
          <span>Enable custom starting car configuration</span>
        </label>
      </div>
      {enableStartingCars && (
        <div className="co-sub-panel">
          <div className="co-starting-grid-row">
            <label className="co-field-label co-starting-grid-label">Number of starting cars</label>
            <input type="number" min={1} max={42} value={numStartingCars} className="co-number-input co-starting-stepper"
              onChange={e => set("numStartingCars", Math.max(1, Math.min(42, parseInt(e.target.value) || 1))) } />
          </div>

          <div className="co-starting-grid-row">
            <label className="co-checkbox-row co-starting-grid-label">
              <input type="checkbox" checked={enableStartingCarsPool} onChange={e => set("enableStartingCarsPool", e.target.checked)} />
              <span>Starting Cars: Set Source Pool</span>
            </label>
            <select value={startingCarsPool} onChange={e => set("startingCarsPool", e.target.value)} disabled={!enableStartingCarsPool} className="co-starting-select">
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
              <input type="checkbox" checked={enableStartingCarsRating} onChange={e => set("enableStartingCarsRating", e.target.checked)} />
              <span>Starting Cars: Set Rating</span>
            </label>
            <select value={startingCarsRating} onChange={e => set("startingCarsRating", e.target.value)} disabled={!enableStartingCarsRating} className="co-starting-select">
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