import { useState, useMemo, useEffect, useRef } from "react";

export default function CarSearchModal({ isOpen, onClose, onSelect, availableCars }) {
  const [searchQuery, setSearchQuery] = useState("");
  const inputRef = useRef(null);
  
  useEffect(() => {
    if (isOpen) {
      setSearchQuery("");
      setTimeout(() => inputRef.current?.focus(), 10);
    }
  }, [isOpen]);

  const filteredCars = useMemo(() => {
    const query = searchQuery.toLowerCase();
    if (!query) return availableCars;
    return availableCars.filter(c => 
      c.name.toLowerCase().includes(query) || 
      c.folderName.toLowerCase().includes(query)
    );
  }, [searchQuery, availableCars]);

  if (!isOpen) return null;

  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div className="search-modal-content" onClick={e => e.stopPropagation()}>
        <div className="search-modal-header">
          <h3>Select Specific Car</h3>
          <input 
            ref={inputRef}
            type="text" 
            placeholder="Search car name or folder..." 
            value={searchQuery}
            onChange={e => setSearchQuery(e.target.value)}
            className="search-input"
          />
        </div>
        <div className="search-results-list">
          {filteredCars.length === 0 && <div className="no-results">No cars found...</div>}
          {filteredCars.map(car => (
            <div 
              key={car.folderName} 
              className="search-result-item"
              onClick={() => { onSelect(car.folderName); onClose(); }}
            >
              <span className="car-name">{car.name}</span>
              <span className="car-folder">({car.folderName})</span>
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
