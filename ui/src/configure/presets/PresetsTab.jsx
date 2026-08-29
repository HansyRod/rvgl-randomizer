import { useMemo, useState } from "react";
import "./PresetsTab.css";
import { useAppContext } from "../../AppProvider";
import { PRESETS } from ".";
import { evaluatePresetSelection } from "./presetValidation";
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
import { normalizeConfigureContext } from "../../utils/configureContext";

// ─── Preset definitions ───────────────────────────────────────────────────────
// Each preset contains the exact configure sub-state that will be applied when
// the user selects it. Add new presets here to extend the list.

function cloneValue(value) {
  if (typeof structuredClone === "function") {
    return structuredClone(value);
  }
  return JSON.parse(JSON.stringify(value));
}

function makeDefaultCustomConfigure() {
  return {
    carOptions: cloneValue(DEFAULT_CAR_OPTIONS),
    trackOptions: cloneValue(DEFAULT_TRACK_OPTIONS),
    carsSpecState: {
      includeStockCars: true,
      includeDcCars: true,
      stockCars: makeDefaultCarsSpec(STOCK_CARS),
      dcCars: makeDefaultCarsSpec(DC_CARS),
      extraCars: [],
    },
    trackSpecState: {
      includeTracks: true,
      tracks: makeDefaultTrackSpec(STOCK_TRACKS),
      cachedRoofTrackRow: null,
    },
    cupSpecState: makeDefaultCupSpecState(),
  };
}

function CustomPresetDialog({
  isOpen,
  sourceMode,
  presetId,
  onClose,
  onConfirm,
  onSelectMode,
  onSelectPreset,
}) {
  if (!isOpen) return null;

  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div
        className="search-modal-content preset-choice-modal"
        onClick={e => e.stopPropagation()}
        style={{ width: "640px", maxWidth: "90vw" }}
      >
        <div className="search-modal-header" style={{ paddingBottom: "1rem" }}>
          <h3 style={{ margin: 0 }}>Start Custom Configuration</h3>
        </div>

        <div className="preset-choice-list">
          <div className={`preset-choice-card${sourceMode === "default" ? " selected" : ""}`}>
            <label className="preset-choice-option">
              <input
                type="radio"
                name="custom-source-mode"
                checked={sourceMode === "default"}
                onChange={() => onSelectMode("default")}
              />
              <span className="preset-choice-copy">
                <span className="preset-card-label">Default Manual Configuration</span>
                <span className="preset-card-desc">
                  Reset Car, Track, and Cup settings back to the baseline manual-editing state.
                </span>
              </span>
            </label>
          </div>

          <div className={`preset-choice-card${sourceMode === "copy" ? " selected" : ""}`}>
            <label className="preset-choice-option">
              <input
                type="radio"
                name="custom-source-mode"
                checked={sourceMode === "copy"}
                onChange={() => onSelectMode("copy")}
              />
              <span className="preset-choice-copy">
                <span className="preset-card-label">Copy Another Preset</span>
                <span className="preset-card-desc">
                  Use an existing preset as the starting point, then continue editing it as Custom.
                </span>
              </span>
            </label>

            <div className="preset-choice-select-wrap">
              <label className="preset-choice-select-label" htmlFor="custom-preset-source">
                Preset to copy
              </label>
              <select
                id="custom-preset-source"
                className="preset-choice-select"
                value={presetId}
                disabled={sourceMode !== "copy"}
                onChange={e => onSelectPreset(e.target.value)}
              >
                {PRESETS.map(preset => (
                  <option key={preset.id} value={preset.id}>
                    {preset.label}
                  </option>
                ))}
              </select>
            </div>
          </div>
        </div>

        <div className="search-modal-footer">
          <button onClick={onClose} style={{ marginRight: "0.5rem" }}>Cancel</button>
          <button className="primary" onClick={onConfirm}>Start Custom</button>
        </div>
      </div>
    </div>
  );
}

export default function PresetsTab() {
  const { state, updateCategoryCtx } = useAppContext();
  const { configure, setup } = state;
  const selectedPreset = configure?.preset ?? "basic";
  const [isCustomDialogOpen, setIsCustomDialogOpen] = useState(false);
  const [customSourceMode, setCustomSourceMode] = useState("default");
  const [customSourcePresetId, setCustomSourcePresetId] = useState(PRESETS[0]?.id ?? "");
  const presetValidity = useMemo(() => {
    return Object.fromEntries(
      PRESETS.map((preset) => [preset.id, evaluatePresetSelection(preset, setup.scanResult)])
    );
  }, [setup.scanResult]);

  // Apply a named preset: overwrite all configure option slices and record the
  // selected preset id so the sidebar knows to lock the other tabs.
  function handleSelectPreset(preset) {
    const nextConfigure = normalizeConfigureContext(preset.configure);
    updateCategoryCtx("configure", {
      preset: preset.id,
      ...nextConfigure,
    });
  }

  // Switch to Custom: open a chooser so the user can either start from the
  // default manual-editing state or copy another preset first.
  function handleSelectCustom() {
    const matchingPreset = PRESETS.find(preset => preset.id === selectedPreset);
    setCustomSourceMode("default");
    setCustomSourcePresetId(matchingPreset?.id ?? PRESETS[0]?.id ?? "");
    setIsCustomDialogOpen(true);
  }

  function handleConfirmCustom() {
    const sourcePreset = PRESETS.find(preset => preset.id === customSourcePresetId);
    const nextConfigure = customSourceMode === "copy" && sourcePreset
      ? cloneValue(sourcePreset.configure)
      : makeDefaultCustomConfigure();

    updateCategoryCtx("configure", {
      preset: "custom",
      ...normalizeConfigureContext(nextConfigure),
    });
    setIsCustomDialogOpen(false);
  }

  return (
    <div className="presets-tab">
      <CustomPresetDialog
        isOpen={isCustomDialogOpen}
        sourceMode={customSourceMode}
        presetId={customSourcePresetId}
        onClose={() => setIsCustomDialogOpen(false)}
        onConfirm={handleConfirmCustom}
        onSelectMode={setCustomSourceMode}
        onSelectPreset={setCustomSourcePresetId}
      />

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
          const validationState = presetValidity[preset.id] ?? { isSelectable: true, errors: [] };
          const isInvalid = !validationState.isSelectable;
          return (
            <button
              key={preset.id}
              className={`preset-card${isSelected ? " selected" : ""}${isInvalid ? " invalid" : ""}`}
              onClick={() => handleSelectPreset(preset)}
              disabled={isInvalid && !isSelected}
            >
              <div className="preset-card-header">
                <span className="preset-card-label">{preset.label}</span>
                {preset.tag && (
                  <span className={`preset-card-tag ${preset.tag.toLowerCase()}`}>{preset.tag}</span>
                )}
              </div>

              <p className="preset-card-desc">{preset.description}</p>

              {isInvalid && (
                <p className="preset-card-error">{validationState.errors[0]}</p>
              )}

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
                Choose whether to start from the default manual configuration
                or copy another preset as your starting point.
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
