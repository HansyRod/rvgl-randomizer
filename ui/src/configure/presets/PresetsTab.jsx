import "./PresetsTab.css";
import { useAppContext } from "../../AppProvider";
import {
  DEFAULT_CAR_OPTIONS,
  DEFAULT_TRACK_OPTIONS,
  STOCK_CARS,
  DC_CARS,
  STOCK_TRACKS,
  makeDefaultCarsSpec,
  makeDefaultTrackSpec,
} from "../../utils/constants";
import { makeDefaultCupSpecState } from "../cupSpec/CupSpecTab";

// ─── Preset definitions ───────────────────────────────────────────────────────
// Each preset contains the exact configure sub-state that will be applied when
// the user selects it. Add new presets here to extend the list.

export const PRESETS = [
  {
    id: "basic",
    label: "Basic",
    tag: "Beginner-friendly",
    description:
      "A straightforward randomization using the game's default settings. Great if you're new to the randomizer or just want a quick, no-fuss session.",
    bullets: [
      "Car unlock mode: Full Random — ratings and unlock methods are both randomised",
      "Both Stock and DC cars included in the pool",
      "No Cheat-Only or Stunt Arena unlocks",
      "All 14 stock tracks included, fully random assignment",
      "Cup stages: 3–6 per cup, 2–8 laps per stage",
      "8 cars per race, 3 tries per cup",
    ],
    configure: {
      carOptions: DEFAULT_CAR_OPTIONS,
      trackOptions: DEFAULT_TRACK_OPTIONS,
      carsSpecState: {
        includeStockCars: true,
        includeDcCars: true,
        stockCars: makeDefaultCarsSpec(STOCK_CARS),
        dcCars: makeDefaultCarsSpec(DC_CARS),
      },
      trackSpecState: {
        includeTracks: true,
        tracks: makeDefaultTrackSpec(STOCK_TRACKS),
      },
      cupSpecState: makeDefaultCupSpecState(),
    },
  },
];

// ─── Component ───────────────────────────────────────────────────────────────

export default function PresetsTab() {
  const { state, updateCategoryCtx } = useAppContext();
  const { configure } = state;
  const selectedPreset = configure?.preset ?? "basic";

  // Apply a named preset: overwrite all configure option slices and record the
  // selected preset id so the sidebar knows to lock the other tabs.
  function handleSelectPreset(preset) {
    updateCategoryCtx("configure", {
      preset: preset.id,
      ...preset.configure,
    });
  }

  // Switch to Custom: only update the preset field so the user's current
  // configure state is preserved as the starting point for manual editing.
  function handleSelectCustom() {
    updateCategoryCtx("configure", { preset: "custom" });
  }

  return (
    <div className="presets-tab">

      {/* ── Header ── */}
      <section className="co-section">
        <h2 className="co-section-title">Randomization Presets</h2>
        <p className="co-desc">
          Choose a preset to quickly apply a curated configuration, or select{" "}
          <strong>Custom</strong> to take full control over every option.
        </p>
      </section>

      {/* ── Preset cards ── */}
      <div className="preset-card-grid">

        {PRESETS.map(preset => {
          const isSelected = selectedPreset === preset.id;
          return (
            <button
              key={preset.id}
              className={`preset-card${isSelected ? " selected" : ""}`}
              onClick={() => handleSelectPreset(preset)}
            >
              <div className="preset-card-header">
                <span className="preset-card-label">{preset.label}</span>
                {preset.tag && (
                  <span className="preset-card-tag">{preset.tag}</span>
                )}
              </div>

              <p className="preset-card-desc">{preset.description}</p>

              {isSelected && (
                <>
                  <hr className="preset-card-divider" />
                  <ul className="preset-card-bullets">
                    {preset.bullets.map((bullet, i) => (
                      <li key={i}>{bullet}</li>
                    ))}
                  </ul>
                </>
              )}
            </button>
          );
        })}

        {/* ── Custom card ── */}
        {(() => {
          const isSelected = selectedPreset === "custom";
          return (
            <button
              className={`preset-card preset-card-custom${isSelected ? " selected" : ""}`}
              onClick={handleSelectCustom}
            >
              <div className="preset-card-header">
                <span className="preset-card-label">Custom</span>
              </div>

              <p className="preset-card-desc">
                Configure every option manually across all the tabs in the sidebar.
                All settings from the last applied preset are preserved as a
                starting point.
              </p>

              {isSelected && (
                <>
                  <hr className="preset-card-divider" />
                  <ul className="preset-card-bullets">
                    <li>Full access to Car, Track and Cup settings</li>
                    <li>Car unlock mode, ratings, and obtain methods</li>
                    <li>Per-car and per-track specification overrides</li>
                    <li>Cup stage mode, lap counts, and per-cup overrides</li>
                  </ul>
                </>
              )}
            </button>
          );
        })()}

      </div>
    </div>
  );
}
