import { useMemo, useState, useCallback, memo } from "react";
import "./CarsFullSpecTab.css";
import {
  STOCK_TRACKS,
  TRACK_DIFFICULTIES,
  TRACK_OBTAIN_METHODS,
  TRACK_DIFFICULTY_SOURCE_LIST,
  TRACK_DIFFICULTY_ATTR_LIST,
  TRACK_OBTAINS_LIST,
} from "../utils/constants";

const TrackSearchModal = ({ isOpen, onClose, onSelect, availableTracks }) => {
  const [searchQuery, setSearchQuery] = useState("");
  const filteredTracks = useMemo(() => {
    const query = searchQuery.toLowerCase();
    if (!query) return availableTracks;
    return availableTracks.filter(t =>
      t.name.toLowerCase().includes(query) || t.folderName.toLowerCase().includes(query)
    );
  }, [availableTracks, searchQuery]);

  if (!isOpen) return null;
  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div className="search-modal-content" onClick={e => e.stopPropagation()}>
        <div className="search-modal-header">
          <h3>Select Specific Track</h3>
          <input
            type="text"
            placeholder="Search track name or folder..."
            value={searchQuery}
            onChange={e => setSearchQuery(e.target.value)}
            className="search-input"
          />
        </div>
        <div className="search-results-list">
          {filteredTracks.length === 0 && <div className="no-results">No tracks found...</div>}
          {filteredTracks.map(track => (
            <div
              key={track.folderName}
              className="search-result-item"
              onClick={() => { onSelect(track.folderName); onClose(); }}
            >
              <span className="car-name">{track.name}</span>
              <span className="car-folder">({track.folderName})</span>
            </div>
          ))}
        </div>
        <div className="search-modal-footer">
          <button onClick={onClose}>Cancel</button>
        </div>
      </div>
    </div>
  );
};

const SpecRow = memo(function SpecRow({
  index,
  rowState,
  updateRow,
  trackByFolder,
  sourcePoolOptionsJSX,
  trackOptions,
  onOpenSearch,
}) {
  if (!rowState) return null;
  const isGeneralPool =
    rowState.sourcePool === "Full Random" ||
    rowState.sourcePool === "Stock" ||
    rowState.sourcePool === "Custom" ||
    rowState.sourcePool.startsWith("Pack:");
  const isSpecificTrack = !isGeneralPool && trackByFolder[rowState.sourcePool] !== undefined;
  const specificTrack = isSpecificTrack ? trackByFolder[rowState.sourcePool] : null;

  const lockDifficulty =
    trackOptions?.unlockMode === "randomUnlock" ||
    trackOptions?.unlockMode === "unchanged" ||
    trackOptions?.unlockMode === "baseGame";
  const lockObtain =
    trackOptions?.unlockMode === "randomDifficulty" ||
    trackOptions?.unlockMode === "unchanged" ||
    trackOptions?.unlockMode === "baseGame";

  return (
    <div className="spec-grid-row">
      <div className="car-id">{rowState.id}</div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select
            value={rowState.sourcePool}
            onChange={e => {
              const val = e.target.value;
              if (val === "Specific Track") onOpenSearch(index);
              else updateRow(index, { sourcePool: val });
            }}
          >
            {sourcePoolOptionsJSX}
            {!isGeneralPool && (
              <optgroup label="Current Selection">
                <option value={rowState.sourcePool}>
                  {specificTrack ? specificTrack.name : rowState.sourcePool}
                </option>
              </optgroup>
            )}
            <option value="Specific Track">Specific Track...</option>
          </select>
        </div>
        <div className="field-group">
          <select
            value={isSpecificTrack ? String(specificTrack.difficulty) : rowState.sourceDifficulty}
            onChange={e => updateRow(index, { sourceDifficulty: e.target.value })}
            disabled={isSpecificTrack}
          >
            {isSpecificTrack ? (
              <option value={String(specificTrack.difficulty)}>
                {TRACK_DIFFICULTIES[specificTrack.difficulty] || "Unknown"}
              </option>
            ) : (
              TRACK_DIFFICULTY_SOURCE_LIST.map(opt => (
                <option key={opt.val} value={opt.val}>{opt.label}</option>
              ))
            )}
          </select>
        </div>
      </div>
      <div className="specs-horizontal">
        <div className="field-group">
          <select
            value={rowState.attrDifficulty}
            onChange={e => updateRow(index, { attrDifficulty: e.target.value })}
            disabled={lockDifficulty}
          >
            {TRACK_DIFFICULTY_ATTR_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>{opt.label}</option>
            ))}
          </select>
        </div>
        <div className="field-group">
          <select
            value={rowState.attrObtain}
            onChange={e => updateRow(index, { attrObtain: e.target.value })}
            disabled={lockObtain}
          >
            {TRACK_OBTAINS_LIST.map(opt => (
              <option key={opt.val} value={opt.val}>
                {opt.label}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  );
});

export default function TrackSpecTab({
  scanResult,
  specState,
  setSpecState,
  trackOptions,
}) {
  const [presetSelection, setPresetSelection] = useState("Full Random");
  const [searchModalRow, setSearchModalRow] = useState(null);
  const isEnabled = specState?.includeTracks !== false;

  const availableTracks = useMemo(() => {
    if (!scanResult) return [];
    let tracks = [];
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
    setSpecState(prev => {
      const tracks = [...(prev?.tracks || [])];
      tracks[index] = { ...tracks[index], ...updates };
      return { ...prev, tracks };
    });
  }, [setSpecState]);

  const applyPreset = () => {
    setSpecState(prev => {
      const rows = (prev?.tracks || []).map((row, i) => {
        if (presetSelection === "Original Content") {
          return {
            ...row,
            sourcePool: STOCK_TRACKS[i] || "Full Random",
            sourceDifficulty: "Random",
          };
        }
        return {
          ...row,
          sourcePool: "Full Random",
          sourceDifficulty: "Random",
        };
      });
      return { ...prev, tracks: rows };
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

      <div style={{ marginBottom: "1rem" }}>
        <label style={{ display: "flex", alignItems: "center", gap: "0.5rem", fontWeight: "bold", fontSize: "1.1rem" }}>
          <input
            type="checkbox"
            checked={isEnabled}
            onChange={e => setSpecState(prev => ({ ...prev, includeTracks: e.target.checked }))}
            style={{ width: "1.2rem", height: "1.2rem" }}
          />
          Include Tracks in Randomization
        </label>
      </div>

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
            {(specState?.tracks || []).map((row, index) => (
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
