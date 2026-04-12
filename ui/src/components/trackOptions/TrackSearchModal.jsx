import { useMemo, useState } from "react";

export default function TrackSearchModal({ isOpen, onClose, onSelect, availableTracks }) {
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