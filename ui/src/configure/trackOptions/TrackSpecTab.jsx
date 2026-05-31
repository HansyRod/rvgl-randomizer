import { useMemo, useState, useCallback, memo } from "react";
import "../carSpec/CarsFullSpecTab.css";
import { STOCK_TRACKS } from "../../utils/constants";
import { normalizeCustomUnlockRow } from "../../utils/customUnlockState";
import { useAppContext } from "../../AppProvider";
import TrackSearchModal from "./TrackSearchModal";
import TrackSpecRow from "./TrackSpecRow";

const SpecRow = memo(TrackSpecRow);

export default function TrackSpecTab() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure } = state;
  
  // Destructure individual variables
  const { scanResult } = setup;
  const { trackOptions, trackSpecState : specState } = configure;

  const [presetSelection, setPresetSelection] = useState("Full Random");
  const [searchModalRow, setSearchModalRow] = useState(null);
  const isEnabled = specState?.includeTracks !== false;

  const availableTracks = useMemo(() => {
    if (!scanResult) return [];
    let tracks;
    if (scanResult.installType === "classic") {
      tracks = scanResult.tracks || [];
    } else {
      tracks = (scanResult.contentPacks || []).filter(p => p.useTracks).flatMap(p => p.tracks);
    }
    return tracks.filter(t => t.hasValidFile && t.trackType === 0);
  }, [scanResult]);

  const trackByFolder = useMemo(() => {
    const map = {};
    for (const t of availableTracks) map[t.folderName] = t;
    return map;
  }, [availableTracks]);

  const activePacks = useMemo(() => {
    if (!scanResult || scanResult.installType === "classic") return [];
    return (scanResult.contentPacks || []).filter(p => p.useTracks).map(p => p.name);
  }, [scanResult]);

  const availablePools = useMemo(() => {
    let hasStock = false;
    let hasCustom = false;
    for (const t of availableTracks) {
      if (STOCK_TRACKS.some(s => s.toLowerCase() === t.folderName.toLowerCase())) hasStock = true;
      else hasCustom = true;
    }
    return { hasStock, hasCustom };
  }, [availableTracks]);

  const sourcePoolOptionsJSX = useMemo(() => {
    const options = [<option key="Full Random" value="Full Random">Full Random</option>];
    if (availablePools.hasStock) options.push(<option key="Stock" value="Stock">Stock Pool</option>);
    if (availablePools.hasCustom) options.push(<option key="Custom" value="Custom">Custom Pool</option>);
    const packOptions = activePacks.map(pack => (
      <option key={`Pack:${pack}`} value={`Pack:${pack}`}>Pack: {pack}</option>
    ));
    return (
      <>
        <optgroup label="General">{options}</optgroup>
        {packOptions.length > 0 && <optgroup label="Content Packs">{packOptions}</optgroup>}
      </>
    );
  }, [activePacks, availablePools]);

  const updateRow = useCallback((index, updates) => {

    const tracks = [...(specState?.tracks || [])];
    const nextRow = { ...tracks[index], ...updates };
    tracks[index] = updates.attrObtain !== undefined
      ? normalizeCustomUnlockRow(nextRow)
      : nextRow;

    updateCategoryCtx("configure", {
      trackSpecState: {
        ...specState, 
        tracks: tracks
      }
    });
  }, [specState, updateCategoryCtx]);

  const applyPreset = () => {

    const rows = (specState?.tracks || []).map((row, i) => {
      if (presetSelection === "Original Content") {
        return {
          ...row,
          sourcePool: row.id || STOCK_TRACKS[i] || "Full Random",
          sourceDifficulty: "Random",
        };
      }
      return {
        ...row,
        sourcePool: "Full Random",
        sourceDifficulty: "Random",
      };
    });

    updateCategoryCtx("configure", {
      trackSpecState: {
        ...specState, 
        tracks: rows
      }
    });

  };

  const mode = trackOptions?.unlockMode;
  const lockDifficulty = mode === "randomUnlock" || mode === "unchanged" || mode === "baseGame";
  const lockObtain = mode === "randomDifficulty" || mode === "unchanged" || mode === "baseGame";

  return (
    <div>
      <TrackSearchModal
        isOpen={searchModalRow !== null}
        onClose={() => setSearchModalRow(null)}
        onSelect={(folderName) => updateRow(searchModalRow, { sourcePool: folderName })}
        availableTracks={availableTracks}
      />

      <div className="section-lock-info">
        <strong>Generated tracks are sorted by difficulty.</strong> User configurations applied to a slot may be applied to a different slot.
      </div>

      {lockDifficulty && (
        <div className="section-lock-info">
          🔒 <strong>Difficulty column is locked</strong> by Track Options mode.
        </div>
      )}
      {lockObtain && (
        <div className="section-lock-info">
          🔒 <strong>Obtain column is locked</strong> by Track Options mode.
        </div>
      )}

      <div className="cars-full-spec" style={{ opacity: isEnabled ? 1 : 0.5, pointerEvents: isEnabled ? "auto" : "none" }}>
        <div className="presets-row">
          <label>Presets:</label>
          <select value={presetSelection} onChange={e => setPresetSelection(e.target.value)}>
            <option value="Full Random">Full Random</option>
            <option value="Original Content">Original Content</option>
          </select>
          <button className="primary" onClick={applyPreset}>Apply</button>
        </div>

        <div className="cars-spec-section">
          <h2>Track Spec</h2>
          <div className="spec-grid">
            <div className="spec-grid-header">
              <div style={{ display: "flex", alignItems: "center" }}>Target Slot</div>
              <div className="column-group">
                <div className="column-group-title">Track Choice</div>
                <div className="specs-horizontal-header">
                  <div style={{ flex: 1 }}>Pool</div>
                  <div style={{ flex: 1 }}>Difficulty</div>
                </div>
              </div>
              <div className="column-group">
                <div className="column-group-title">Attributes</div>
                <div className="specs-horizontal-header">
                  <div style={{ flex: 1 }}>Difficulty</div>
                  <div style={{ flex: 1 }}>Obtain</div>
                </div>
              </div>
            </div>
            {(specState?.tracks || []).map((row, index) => ({ row, index })).map(({ row, index }) => (
              <SpecRow
                key={`${row.id}-${index}`}
                index={index}
                rowState={row}
                updateRow={updateRow}
                trackByFolder={trackByFolder}
                sourcePoolOptionsJSX={sourcePoolOptionsJSX}
                trackOptions={trackOptions}
                onOpenSearch={setSearchModalRow}
              />
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
