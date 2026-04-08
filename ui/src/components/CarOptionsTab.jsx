import { useEffect, useMemo } from "react";
import "./CarOptionsTab.css";
import { STOCK_CARS, DC_CARS, RATINGS_LIST, CAR_RATINGS } from "../utils/constants";

function makeDefaultSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceRating: "Random",
    sourceObtain: "Random",
    attrRating: "Random",
    attrObtain: "Random",
  }));
}

export default function CarOptionsTab({ 
  scanResult, 
  specState, 
  setSpecState, 
  carOptions, 
  setCarOptions 
}) {
  const {
    unlockMode,
    enableStartingCars,
    numStartingCars,
    startingCarsPool,
    startingCarsRating,
    includeCheatOnly,
    includeStuntArena,
  } = carOptions;

  const set = (key, value) => setCarOptions(prev => ({ ...prev, [key]: value }));

  // Immediate sync logic
  useEffect(() => {
    // Auto-initialise FullSpec state if the user hasn't been there yet
    const base = specState ?? {
      stockCars: makeDefaultSpec(STOCK_CARS),
      dcCars:    makeDefaultSpec(DC_CARS),
    };

    const newStockCars = base.stockCars.map((car, i) => {
      const out = { ...car };

      if (unlockMode === "unchanged") {
        out.attrObtain = "Unchanged";
        out.attrRating = "Unchanged";
      } else if (unlockMode === "baseGame") {
        if (i < 8) {
          out.attrRating = "0"; // Rookie
          out.attrObtain = "0"; // Starting Car
        } else {
          const groupIdx = Math.floor((i - 8) / 5);
          const localIdx = (i - 8) % 5;
          out.attrRating = String(groupIdx + 1);
          if (localIdx < 2) out.attrObtain = "1"; // Championship
          else if (localIdx < 4) out.attrObtain = "3"; // Practice
          else out.attrObtain = "4"; // Single Race
        }
      } else {
        // random, randomRatings, randomUnlock
        if (unlockMode === "random") {
          out.attrObtain = "Random";
          out.attrRating = "Random";
        } else if (unlockMode === "randomRatings") {
          out.attrObtain = "Unchanged";
          out.attrRating = "Random";
        } else {
          // randomUnlock
          out.attrObtain = "Random";
          out.attrRating = "Unchanged";
        }

        // Starting-car overrides (only in modes that allow randomizing unlock)
        const canRandomUnlock = (unlockMode === "random" || unlockMode === "randomUnlock");
        if (canRandomUnlock && enableStartingCars && numStartingCars > 0 && i < numStartingCars) {
          out.sourcePool    = startingCarsPool;
          out.sourceRating  = startingCarsRating;
          out.attrObtain    = "0"; // Force "Starting Car"
          
          // If startingCarsRating is NOT "Random", we should align the pool filter in the backend.
          // But should we also force the final rating? 
          // User said: "If a rating is chosen there, it should follow that rating, 
          // and what that means in case the ratings are not random, 
          // is limiting the car pool to only accept cars of the correct rating."
          if (startingCarsRating !== "Random") {
             // In randomRatings/random mode, we might want to force the final rating too? 
             // Logic: If user specifically picked a rating for starting car pool, 
             // they probably want them to be that rating.
             if (unlockMode === "random" || unlockMode === "randomRatings") {
                out.attrRating = startingCarsRating;
             }
          }
        }
      }

      return out;
    });

    const newDcCars = base.dcCars.map((car, i) => {
      const out = { ...car };
      if (unlockMode === "unchanged") {
        out.attrObtain = "Unchanged";
        out.attrRating = "Unchanged";
      } else if (unlockMode === "baseGame") {
        if (i === 0) {
          out.attrRating = "0"; // Rookie
          out.attrObtain = "0"; // Starting Car
        } else if (i === 1) {
          out.attrRating = "2"; // Advanced
          out.attrObtain = "1"; // Championship
        } else {
          // 2-13
          out.attrObtain = "2"; // Time Trial
          const groupIdx = Math.floor((i - 2) / 3);
          out.attrRating = String(groupIdx + 1); // 2-4:1, 5-7:2, 8-10:3, 11-13:4
        }
      } else {
        if (unlockMode === "random") {
          out.attrObtain = "Random";
          out.attrRating = "Random";
        } else if (unlockMode === "randomRatings") {
          out.attrObtain = "Unchanged";
          out.attrRating = "Random";
        } else {
          out.attrObtain = "Random";
          out.attrRating = "Unchanged";
        }
      }
      return out;
    });

    setSpecState({ stockCars: newStockCars, dcCars: newDcCars });
  }, [carOptions]); // Sync whenever options change

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

  const UNLOCK_MODES = [
    {
      id: "random",
      label: "Full Random",
      desc: "Both car ratings and unlock methods are chosen randomly.",
    },
    {
      id: "randomRatings",
      label: "Random Ratings",
      desc: "Car ratings are random, but unlock criteria stay unchanged.",
    },
    {
      id: "randomUnlock",
      label: "Random Unlock Criteria",
      desc: "Unlock criteria are random, but ratings stay unchanged.",
    },
    {
      id: "unchanged",
      label: "Unchanged",
      desc: "All car attributes stay exactly as scanned.",
    },
    {
      id: "baseGame",
      label: "Base Game Distribution",
      desc: "Follows RVGL's original fixed rules for Rating and Obtain.",
    },
  ];

  const handleSuperProToggle = (val) => {
    setCarOptions(prev => {
      const next = { ...prev, includeSuperPro: val };
      if (!val) {
        // Force Super Pro to 0 and enabled for both
        next.poolRatingDistributions = {
          ...prev.poolRatingDistributions,
          "5": { enabled: true, min: 0, max: 0 }
        };
        next.attrRatingDistributions = {
          ...prev.attrRatingDistributions,
          "5": { enabled: true, min: 0, max: 0 }
        };
      }
      return next;
    });
  };

  const updateDist = (type, ratingId, field, value) => {
    const key = type === "pool" ? "poolRatingDistributions" : "attrRatingDistributions";
    setCarOptions(prev => ({
      ...prev,
      [key]: {
        ...prev[key],
        [ratingId]: { ...prev[key][ratingId], [field]: value }
      }
    }));
  };

  const showAllowedMethods = (unlockMode === "random" || unlockMode === "randomUnlock");
  const showStartingCars = (unlockMode === "random" || unlockMode === "randomUnlock");
  const showRatingOptions = (unlockMode === "random" || unlockMode === "randomRatings");

  return (
    <div className="car-options-tab">

      {/* ── Section 1 · Car Randomization Mode ── */}
      <section className="co-section">
        <h2 className="co-section-title">Car Randomization</h2>
        <p className="co-desc">
          Controls how car ratings and unlock conditions are assigned.
        </p>
        <div className="co-mode-grid">
          {UNLOCK_MODES.map(mode => (
            <button
              key={mode.id}
              className={`co-mode-card${unlockMode === mode.id ? " selected" : ""}`}
              onClick={() => set("unlockMode", mode.id)}
            >
              <span className="co-mode-label">{mode.label}</span>
              <span className="co-mode-desc">{mode.desc}</span>
            </button>
          ))}
        </div>
      </section>

      {/* ── Section 2 · Allowed Methods ── */}
      {showAllowedMethods && (
        <section className="co-section">
          <h2 className="co-section-title">Allowed Unlock Methods</h2>
          <p className="co-desc">
            Standard methods (Starting Car, Championship, Time Trial, Practice Stars, Single Race) are always in the pool.
            Enable the options below to include additional unlock types.
          </p>
          <div className="co-checkbox-group">
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeCheatOnly}
                onChange={e => set("includeCheatOnly", e.target.checked)}
              />
              <span>
                Include <strong>Cheat Only</strong>{" "}
                <span className="co-tag">obtain −1</span>
                {" "}— cars are permanently locked without cheat codes.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeStuntArena}
                onChange={e => set("includeStuntArena", e.target.checked)}
              />
              <span>
                Include <strong>Stunt Arena</strong>{" "}
                <span className="co-tag">obtain 5</span>
                {" "}— cars unlocked via Stunt Arena completion.
              </span>
            </label>
          </div>
        </section>
      )}

      {/* ── Section 3 · Rating Options ── */}
      {showRatingOptions && (
        <section className="co-section">
          <h2 className="co-section-title">Rating Options</h2>
          <div className="co-checkbox-group" style={{ marginBottom: "1.5rem" }}>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={carOptions.includeSuperPro}
                onChange={e => handleSuperProToggle(e.target.checked)}
              />
              <span>Include <strong>Super Pro</strong> rating in randomization</span>
            </label>
          </div>

          <div className="co-dist-grid">
             <RatingDistTable 
                title="Car Pool Rating Distribution" 
                desc="Enforce how many cars are picked based on their original rating."
                data={carOptions.poolRatingDistributions}
                onChange={(rid, f, v) => updateDist("pool", rid, f, v)}
                includeSuperPro={carOptions.includeSuperPro}
             />
             <RatingDistTable 
                title="Target Rating Distribution" 
                desc="Enforce how many slots are assigned each final rating."
                data={carOptions.attrRatingDistributions}
                onChange={(rid, f, v) => updateDist("attr", rid, f, v)}
                includeSuperPro={carOptions.includeSuperPro}
             />
          </div>
        </section>
      )}

      {/* ── Section 4 · Starting Cars ── */}
      {showStartingCars && (
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
                onChange={e => set("enableStartingCars", e.target.checked)}
              />
              <span>Enable custom starting car configuration</span>
            </label>
          </div>

          {enableStartingCars && (
            <div className="co-sub-panel">
              <div className="co-field-row">
                <label className="co-field-label">Number of starting cars</label>
                <input
                  type="number"
                  min={0}
                  max={42}
                  value={numStartingCars}
                  onChange={e =>
                    set("numStartingCars", Math.max(0, Math.min(42, parseInt(e.target.value) || 0)))
                  }
                  className="co-number-input"
                />
              </div>

              <div className="co-field-row" style={{ marginTop: "1rem" }}>
                <label className="co-field-label">Starting cars eligibility pool</label>
                <select
                  value={startingCarsPool}
                  onChange={e => set("startingCarsPool", e.target.value)}
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

              <div className="co-field-row">
                <label className="co-field-label">Minimum car rating</label>
                <select
                  value={startingCarsRating}
                  onChange={e => set("startingCarsRating", e.target.value)}
                >
                  {RATINGS_LIST.map(o => (
                    <option key={o.val} value={o.val}>{o.label}</option>
                  ))}
                </select>
              </div>
            </div>
          )}
        </section>
      )}
    </div>
  );
}

function RatingDistTable({ title, desc, data, onChange, includeSuperPro }) {
  const ratings = ["0", "1", "2", "3", "4", "5"];
  
  return (
    <div className="co-dist-table-box">
      <h3>{title}</h3>
      <p className="co-tiny-desc">{desc}</p>
      <table className="co-dist-table">
        <thead>
          <tr>
            <th></th>
            <th>Rating</th>
            <th>Min</th>
            <th>Max</th>
          </tr>
        </thead>
        <tbody>
          {ratings.map(rid => {
            const isSuperPro = rid === "5";
            const locked = isSuperPro && !includeSuperPro;
            const dist = data[rid];

            return (
              <tr key={rid} className={locked ? "locked-row" : ""}>
                <td>
                  <input 
                    type="checkbox" 
                    checked={dist.enabled} 
                    disabled={locked}
                    onChange={e => onChange(rid, "enabled", e.target.checked)}
                  />
                </td>
                <td className="rating-label">{CAR_RATINGS[rid]}</td>
                <td>
                  <input 
                    type="number" 
                    min={0} max={42} 
                    value={dist.min} 
                    disabled={locked || !dist.enabled}
                    onChange={e => onChange(rid, "min", parseInt(e.target.value) || 0)}
                  />
                </td>
                <td>
                  <input 
                    type="number" 
                    min={0} max={42} 
                    value={dist.max} 
                    disabled={locked || !dist.enabled}
                    onChange={e => onChange(rid, "max", parseInt(e.target.value) || 0)}
                  />
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
