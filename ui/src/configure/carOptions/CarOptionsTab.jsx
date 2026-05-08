import "./CarOptionsTab.css";
import { STOCK_CARS, DC_CARS } from "../../utils/constants";
import { useAppContext } from "../../AppProvider";
import StartingCarConfig from "./StartingCarConfig";
import CarRatingsConfig from "./CarRatingsConfig";
import { applyModeRules, makeDefaultSpec, alignDistributionsWithSpec } from "./CarOptionsUtils";

export default function CarOptionsTab() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { configure } = state;
  
  // Destructure individual variables
  const { carOptions, carsSpecState } = configure;
  
  const {
    unlockMode,
    includeCheatOnly,
    includeStuntArena,
    includeStartingCar,
    includeChampionship,
    includeTimeTrial,
    includePracticeStars,
    includeSingleRace,
  } = carOptions;

  const set = (key, value) => updateCategoryCtx("configure", { carOptions: { ...carOptions, [key]: value } });

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

  const handleModeSelect = (modeId) => {

    // Auto-initialise FullSpec state if the user hasn't been there yet
    const base = carsSpecState ?? {
      stockCars: makeDefaultSpec(STOCK_CARS),
      dcCars:    makeDefaultSpec(DC_CARS),
    };

    const newStockCars = base.stockCars.map((car, i) => {

      // Unlock mode determines the logic for how we assign the final obtain and rating attributes for each car slot.
      if (modeId !== "baseGame") {
        return applyModeRules(car, i, modeId, carOptions);
      }

      const out = { ...car };

      // Unlock mode: "Base Game Distribution" tries to mimic RVGL's original unlock rules as closely as possible.
      // First 8 cars are rookies and available as starting cars. After that, each group of 5 cars shares the same rating.~
      // The first 2 cars in the group are unlocked via Championship, next 2 via Practice, and last one via Single
      if (i < 8) {
        out.attrRating = "0"; // Rookie
        out.attrObtain = "0"; // Starting Car        
      } else {
        const groupIdx = Math.floor((i - 8) / 5);
        out.attrRating = String(groupIdx + 1); // 8-12: Amateur, 13-17: Advanced, 18-22: Semi-Pro, 23-27: Pro

        const localIdx = (i - 8) % 5;
        if (localIdx < 2) {
          out.attrObtain = "1"; // Championship
        } else if (localIdx < 4) {
          out.attrObtain = "3"; // Practice
        } else {
          out.attrObtain = "4"; // Single Race
        }
      }

      return out;
    });

    const newDcCars = base.dcCars.map((car, i) => {

      if (modeId !== "baseGame") {
        return applyModeRules(car, i + 28, modeId, carOptions);
      }

      const out = { ...car };

      // Unlock mode: "Base Game Distribution" tries to mimic RVGL's original unlock rules as closely as possible.
      // For DC, first car is a Rookie and Starting Car (BigVolt),
      // Second car is Advanced and unlocked by winning a Championship (BossVolt).
      // After that, each group of 3 cars shares the same rating and all are unlocked via Time Trial.
      if (i === 0) {
        out.attrRating = "0"; // Rookie
        out.attrObtain = "0"; // Starting Car
      } else if (i === 1) {
        out.attrRating = "2"; // Advanced
        out.attrObtain = "1"; // Championship
      } else {
        const groupIdx = Math.floor((i - 2) / 3);
        out.attrRating = String(groupIdx + 1); // 2-4:1, 5-7:2, 8-10:3, 11-13:4
        out.attrObtain = "2"; // Time Trial
      }

      return out;
    });

    const newCarsSpecState = { stockCars: newStockCars, dcCars: newDcCars };
    const newCarOptions = { ...carOptions, unlockMode: modeId };
    updateCategoryCtx("configure", { carsSpecState: newCarsSpecState, carOptions: newCarOptions });
  };

  const showAllowedMethods = (unlockMode === "random" || unlockMode === "randomUnlock");
  const showStartingCars = (unlockMode !== "baseGame");
  const showRatingOptions = (unlockMode === "random" || unlockMode === "randomRatings");

  const includeStockCars = carsSpecState?.includeStockCars !== false;
  const includeDcCars    = carsSpecState?.includeDcCars    !== false;
  const noneSelected = !includeStockCars && !includeDcCars;

  const handleInclude = (key, value) => {
    const newCarsSpecState = { ...carsSpecState, [key]: value };
    const aligned = alignDistributionsWithSpec(carOptions, newCarsSpecState);
    updateCategoryCtx("configure", {
      carsSpecState: newCarsSpecState,
      carOptions: aligned,
    });
  };

  return (
    <div className="car-options-tab">

      {/* ── Section 0 · Scope ── */}
      <section className="co-section">
        <h2 className="co-section-title">Randomization Scope</h2>
        <p className="co-desc">
          Choose which car categories are included in the randomization.
        </p>
        <div className="co-checkbox-group">
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={includeStockCars}
              onChange={e => handleInclude("includeStockCars", e.target.checked)}
            />
            <span>Randomize <strong>Stock Cars</strong></span>
          </label>
          <label className="co-checkbox-row">
            <input
              type="checkbox"
              checked={includeDcCars}
              onChange={e => handleInclude("includeDcCars", e.target.checked)}
            />
            <span>Randomize <strong>DC Cars</strong></span>
          </label>
        </div>
      </section>

      {!noneSelected && (<>

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
              onClick={() => handleModeSelect(mode.id)}
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
            Choose which unlock methods are included in the random pool.
          </p>
          <div className="co-checkbox-group">
            {/* ── Standard methods ── */}
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeStartingCar}
                onChange={e => set("includeStartingCar", e.target.checked)}
              />
              <span>
                <strong>Starting Car</strong> — car is available from the start without needing to unlock it.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeChampionship}
                onChange={e => set("includeChampionship", e.target.checked)}
              />
              <span>
                <strong>Win Championship</strong> — car is unlocked by winning a cup.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeTimeTrial}
                onChange={e => set("includeTimeTrial", e.target.checked)}
              />
              <span>
                <strong>Beat Time Trial</strong> — car is unlocked by beating Normal time trials on all tracks of its tier.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includePracticeStars}
                onChange={e => set("includePracticeStars", e.target.checked)}
              />
              <span>
                <strong>Catch Practice Stars</strong> — car is unlocked by collecting stars in practice mode on all tracks of its tier.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeSingleRace}
                onChange={e => set("includeSingleRace", e.target.checked)}
              />
              <span>
                <strong>Win Single Races</strong> — car is unlocked by winning single races on all tracks of its tier.
              </span>
            </label>

            {/* ── Extra / optional methods ── */}
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeCheatOnly}
                onChange={e => set("includeCheatOnly", e.target.checked)}
              />
              <span>
                Include <strong>Cheat Only</strong> — car is permanently locked and can only be unlocked with cheat codes.
              </span>
            </label>
            <label className="co-checkbox-row">
              <input
                type="checkbox"
                checked={includeStuntArena}
                onChange={e => set("includeStuntArena", e.target.checked)}
              />
              <span>
                Include <strong>Stunt Arena</strong> — car is unlocked by completing the Stunt Arena.
              </span>
            </label>
          </div>
        </section>
      )}

      {/* ── Section 3 · Rating Options ── */}
      { showRatingOptions && <CarRatingsConfig /> }

      { /* ── Section 4 · Starting Cars ── */ }
      { showStartingCars && <StartingCarConfig /> }
      </>)}
    </div>
  );
}

