import { useState } from "react";
import PoolGrid from "./PoolGrid";
import { CAR_RATINGS } from "../utils/constants";
import { useAppContext } from "../AppProvider";

export default function MainContent() {

  const { state } = useAppContext();

  // Destructure categories
  const { app, setup : { installPath, scanResult, setupTab : activeTab }, archipelago } = state;
  const isArchipelago = app.mode === "archipelago";
  const { scanResult: archipelagoScanResult } = archipelago.setup;
  const ready = archipelagoScanResult && !app.isScanning;
    
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

  let archipelagoItems = [];
  if (isArchipelago && ready) {
    const categoryIds = activeTab === "cars" ? ["stockCars", "dcCars"] : ["stockTracks", "dcTracks"];
    archipelagoItems = categoryIds.flatMap(id => archipelagoScanResult.categories[id].items);
  }

  if (scanResult.installType === "classic") {
    let list = activeTab === "cars" ? scanResult.cars : scanResult.tracks;
    if (isArchipelago) {
      list = archipelagoItems;
    }
    // We assume executable path is the root
    const rootPath = installPath.substring(0, installPath.lastIndexOf('\\'));
    
    return (
      <div className="main-column">
        {filterUI}
        <div className="main-content">
          {(!isArchipelago || list.length > 0) && (
            <PoolGrid items={list} rootPath={rootPath} activeTab={activeTab} ratingFilter={ratingFilter} includeStuntArena={isArchipelago} />
          )}
        </div>
      </div>
    );
  }

  // Launcher Installation
  const activePacks = scanResult.contentPacks.filter(p => {
    if (isArchipelago) {
      return archipelagoItems.some(item => item.source.packName === p.name);
    }
    return activeTab === "cars" ? p.useCars : p.useTracks;
  });

  return (
      <div className="main-column">
        {filterUI}
        <div className="main-content">
          {!isArchipelago && activePacks.length === 0 && (
            <p style={{opacity: 0.7}}>No content packs enabled. Enable packs from the sidebar.</p>
          )}
          {activePacks.map(pack => (
            <div key={pack.name} className="pool-section">
              <h2 className="pool-section-title">{pack.name}</h2>
              <PoolGrid 
                items={isArchipelago
                  ? archipelagoItems.filter(item => item.source.packName === pack.name)
                  : activeTab === "cars" ? pack.cars : pack.tracks}
                rootPath={pack.absolutePath} 
                activeTab={activeTab}
                ratingFilter={ratingFilter} 
                includeStuntArena={isArchipelago}
              />
            </div>
          ))}
        </div>
      </div>
  );
}
