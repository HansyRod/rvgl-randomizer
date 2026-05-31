import { useEffect, useMemo, useState } from "react";
import {
  CUSTOM_UNLOCK_COUNT_METHODS,
  CUSTOM_UNLOCK_METHODS,
  CUSTOM_UNLOCK_SPECIFIC_METHODS,
} from "../../utils/constants";
import { makeDefaultCustomUnlock } from "../../utils/customUnlockState";

const SPECIFIC_METHOD_IDS = new Set(CUSTOM_UNLOCK_SPECIFIC_METHODS.map(method => method.val));
const COUNT_METHOD_IDS = new Set(CUSTOM_UNLOCK_COUNT_METHODS.map(method => method.val));

function toPositiveInt(value, fallback = 1) {
  const parsed = parseInt(value, 10);
  if (Number.isNaN(parsed) || parsed < 1) return fallback;
  return parsed;
}

function getTrackLabel(track) {
  if (!track) return "";
  return track.name ? `${track.name} (${track.folderName})` : track.folderName;
}

export default function CustomUnlockModal({
  isOpen,
  method,
  value,
  availableTracks = [],
  excludedTrackFolders = [],
  onClose,
  onSave,
}) {
  const methodKey = String(method ?? "");
  const isSpecificMethod = SPECIFIC_METHOD_IDS.has(methodKey);
  const isCountMethod = COUNT_METHOD_IDS.has(methodKey);
  const [draft, setDraft] = useState(() => makeDefaultCustomUnlock(methodKey));
  const [searchQuery, setSearchQuery] = useState("");

  useEffect(() => {
    if (!isOpen) return;
    setDraft(value?.method === methodKey ? value : makeDefaultCustomUnlock(methodKey));
    setSearchQuery("");
  }, [isOpen, methodKey, value]);

  const excludedFolders = useMemo(
    () => new Set(excludedTrackFolders.map(folder => String(folder).toLowerCase())),
    [excludedTrackFolders]
  );

  const trackOptions = useMemo(() => {
    const query = searchQuery.trim().toLowerCase();
    return availableTracks
      .filter(track => track?.folderName && !excludedFolders.has(track.folderName.toLowerCase()))
      .filter(track => {
        if (!query) return true;
        return (
          track.name?.toLowerCase().includes(query) ||
          track.folderName.toLowerCase().includes(query)
        );
      });
  }, [availableTracks, excludedFolders, searchQuery]);

  if (!isOpen || !draft || (!isSpecificMethod && !isCountMethod)) return null;

  const methodLabel = CUSTOM_UNLOCK_METHODS[methodKey] ?? "Custom Unlock";
  const selectedFolders = new Set(draft.trackFolders || []);

  const setMode = (mode) => {
    setDraft(current => ({
      ...current,
      mode,
    }));
  };

  const setRandomTrackCount = (value) => {
    setDraft(current => ({
      ...current,
      randomTrackCount: toPositiveInt(value),
    }));
  };

  const setRequiredCount = (value) => {
    setDraft(current => ({
      ...current,
      requiredCount: toPositiveInt(value),
    }));
  };

  const toggleTrack = (folderName) => {
    setDraft(current => {
      const currentFolders = new Set(current.trackFolders || []);
      if (currentFolders.has(folderName)) currentFolders.delete(folderName);
      else currentFolders.add(folderName);

      return {
        ...current,
        trackFolders: Array.from(currentFolders),
      };
    });
  };

  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div className="search-modal-content custom-unlock-modal" onClick={e => e.stopPropagation()}>
        <div className="search-modal-header">
          <h3>{methodLabel}</h3>
        </div>

        <div className="custom-unlock-modal-body">
          {isSpecificMethod && (
            <>
              <div className="custom-unlock-mode-row">
                <label className="custom-unlock-radio">
                  <input
                    type="radio"
                    name="custom-unlock-mode"
                    value="randomTracks"
                    checked={draft.mode === "randomTracks"}
                    onChange={() => setMode("randomTracks")}
                  />
                  <span>Random tracks</span>
                </label>
                <label className="custom-unlock-radio">
                  <input
                    type="radio"
                    name="custom-unlock-mode"
                    value="specificTracks"
                    checked={draft.mode === "specificTracks"}
                    onChange={() => setMode("specificTracks")}
                  />
                  <span>Specific tracks</span>
                </label>
              </div>

              {draft.mode === "randomTracks" && (
                <div className="custom-unlock-field-row">
                  <label htmlFor="custom-unlock-random-count">Track count</label>
                  <input
                    id="custom-unlock-random-count"
                    className="co-number-input"
                    type="number"
                    min={1}
                    value={draft.randomTrackCount ?? 1}
                    onChange={e => setRandomTrackCount(e.target.value)}
                  />
                </div>
              )}

              {draft.mode === "specificTracks" && (
                <div className="custom-unlock-track-picker">
                  <input
                    type="text"
                    placeholder="Search track name or folder..."
                    value={searchQuery}
                    onChange={e => setSearchQuery(e.target.value)}
                    className="search-input"
                  />
                  <div className="custom-unlock-track-list">
                    {trackOptions.length === 0 && <div className="no-results">No tracks found...</div>}
                    {trackOptions.map(track => (
                      <label key={track.folderName} className="custom-unlock-track-option">
                        <input
                          type="checkbox"
                          checked={selectedFolders.has(track.folderName)}
                          onChange={() => toggleTrack(track.folderName)}
                        />
                        <span>{getTrackLabel(track)}</span>
                      </label>
                    ))}
                  </div>
                </div>
              )}
            </>
          )}

          {isCountMethod && (
            <div className="custom-unlock-field-row">
              <label htmlFor="custom-unlock-required-count">Required count</label>
              <input
                id="custom-unlock-required-count"
                className="co-number-input"
                type="number"
                min={1}
                value={draft.requiredCount ?? 1}
                onChange={e => setRequiredCount(e.target.value)}
              />
            </div>
          )}
        </div>

        <div className="search-modal-footer custom-unlock-modal-footer">
          <button onClick={onClose}>Cancel</button>
          <button className="primary" onClick={() => onSave(draft)}>Save</button>
        </div>
      </div>
    </div>
  );
}
