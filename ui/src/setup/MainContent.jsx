import { useState } from "react";
import PoolGrid from "./PoolGrid";
import { CAR_RATINGS } from "../utils/constants";
import { useAppContext } from "../AppProvider";

export default function MainContent() {

  const { state } = useAppContext();

  // Destructure categories
  const { install : { installPath, scanResult, setupTab : activeTab } } = state;
    
  const [ratingFilter, setRatingFilter] = useState("All");

  const filterUI = activeTab === "cars" && (
    <div className="pool-header">
      <label>Filter by Rating:</label>
      <select value={ratingFilter} onChange={e => setRatingFilter(e.target.value)}>
        <option value="All">All Ratings</option>
        {Object.entries(CAR_RATINGS).map(([val, label]) => (
          <option key={val} value={val}>{label}</option>
        ))}
      </select>
    </div>
  );

  if (scanResult.installType === "classic") {
    const list = activeTab === "cars" ? scanResult.cars : scanResult.tracks;
    // We assume executable path is the root
    const rootPath = installPath.substring(0, installPath.lastIndexOf('\\'));
    
    return (
      <div className="main-column">
        {filterUI}
        <div className="main-content">
          <PoolGrid items={list} rootPath={rootPath} activeTab={activeTab} ratingFilter={ratingFilter} />
        </div>
      </div>
    );
  }

  // Launcher Installation
  const activePacks = scanResult.contentPacks.filter(p => 
    activeTab === "cars" ? p.useCars : p.useTracks
  );

  return (
      <div className="main-column">
        {filterUI}
        <div className="main-content">
          {activePacks.length === 0 && (
            <p style={{opacity: 0.7}}>No content packs enabled. Enable packs from the sidebar.</p>
          )}
          {activePacks.map(pack => (
            <div key={pack.name} className="pool-section">
              <h2 className="pool-section-title">{pack.name}</h2>
              <PoolGrid 
                items={activeTab === "cars" ? pack.cars : pack.tracks} 
                rootPath={pack.absolutePath} 
                activeTab={activeTab}
                ratingFilter={ratingFilter} 
              />
            </div>
          ))}
        </div>
      </div>
  );
}
