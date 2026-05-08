import "./PresetsTab.css";
import { useAppContext } from "../../AppProvider";
import { STOCK_LIKE_PRESET } from "./stock-like";
import { FULL_RANDOM_PRESET } from "./full-random";
import { ALL_ROOKIES_PRESET } from "./all-rookies";

// ─── Preset definitions ───────────────────────────────────────────────────────
// Each preset contains the exact configure sub-state that will be applied when
// the user selects it. Add new presets here to extend the list.

export const PRESETS = [
  STOCK_LIKE_PRESET,
  ALL_ROOKIES_PRESET,
  FULL_RANDOM_PRESET,
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
