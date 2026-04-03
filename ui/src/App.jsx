import { useState, useEffect } from "react";
import { invoke, convertFileSrc } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import "./App.css";

const CAR_RATINGS = {
  [-2]: "Unknown",
  [-1]: "None",
  0: "Rookie",
  1: "Amateur",
  2: "Advanced",
  3: "Semi-Pro",
  4: "Pro",
  5: "Super Pro"
};

const OBTAIN_METHODS = {
  [-2]: "Unknown",
  [-1]: "None",
  0: "Unlocked",
  1: "Cup",
  2: "Time Trial",
  3: "Practice",
  4: "Races"
};

const TRACK_DIFFICULTIES = {
  1: "Easy",
  2: "Medium",
  3: "Hard",
  4: "Extreme"
};

const OVERRIDE_CARBOXES = [
  "adeon", "amw", "beatall", "bigvolt", "bossvolt", "candy", "cougar", "dino", 
  "flag", "fone", "gencar", "jg1jg7", "jg2fulonx", "jg3loco", "jg4snw35", 
  "jg5purpxl", "jg6rc", "mite", "moss", "mouse", "mud", "panga", "path", "r5", 
  "rc", "rotor", "sgt", "sugo", "tc1", "tc10", "tc11", "tc12", "tc2", "tc3", 
  "tc4", "tc5", "tc6", "tc7", "tc8", "tc9", "toyeca", "volken"
];

export default function App() {
  const [installPath, setInstallPath] = useState("");
  const [scanResult, setScanResult] = useState(null);
  const [activeTab, setActiveTab] = useState("cars");
  const [theme, setTheme] = useState("dark");
  const [isScanning, setIsScanning] = useState(false);
  const [isFetchingPack, setIsFetchingPack] = useState(false);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  async function handleScanSetup() {
    const selected = await open({
      multiple: false,
      directory: false,
      filters: [{ name: 'Executable', extensions: ['exe', ''] }]
    });

    if (selected) {
      const path = selected.path || selected;
      setInstallPath(path);
      setIsScanning(true);
      try {
        const result = await invoke("scan_install", { executablePath: path });
        setScanResult(result);
      } catch (err) {
        console.error("Error scanning:", err);
      } finally {
        setIsScanning(false);
      }
    }
  }

  async function togglePack(packIndex) {
    if (!scanResult) return;
    
    // Copy state properly
    const newResult = JSON.parse(JSON.stringify(scanResult));
    const pack = newResult.contentPacks[packIndex];
    
    if (activeTab === "cars") {
      pack.useCars = !pack.useCars;
      if (pack.useCars && pack.cars.length === 0 && pack.hasCars) {
        setIsFetchingPack(true);
        try {
          pack.cars = await invoke("scan_cars_folder", { folderPath: `${pack.absolutePath}\\cars` });
        } catch(e) { console.error(e); }
        finally { setIsFetchingPack(false); }
      }
    } else {
      pack.useTracks = !pack.useTracks;
      if (pack.useTracks && pack.tracks.length === 0 && pack.hasTracks) {
        setIsFetchingPack(true);
        try {
          pack.tracks = await invoke("scan_levels_folder", { folderPath: `${pack.absolutePath}\\levels` });
        } catch(e) { console.error(e); }
        finally { setIsFetchingPack(false); }
      }
    }

    setScanResult(newResult);
  }

  return (
    <div className="container">
      {isFetchingPack && <div className="loading-overlay">Loading Assets...</div>}
      <header>
        <div className="header-top">
          <div className="header-title-area">
            <h1>RVGL Randomizer</h1>
            <div style={{ display: "flex", alignItems: "center", gap: "0.5rem", marginTop: "0.25rem" }}>
              {scanResult && (
                <span className="badge" style={{ padding: "0.3rem 0.5rem", fontSize: "0.75rem", backgroundColor: "var(--accent)" }}>
                  [{scanResult.installType === "launcher" ? "RVGL Launcher Install" : "Classic Install"}]
                </span>
              )}
              <p style={{ margin: 0 }}>{installPath || "No installation selected. Browse to rvgl.exe to begin."}</p>
            </div>
          </div>
          <div className="header-actions">
            <button className="primary" onClick={handleScanSetup}>
              {isScanning ? "Scanning..." : "Browse RVGL"}
            </button>
          </div>
        </div>
        {scanResult && (
          <div className="tabs">
            <button 
              className={`tab ${activeTab === 'cars' ? 'active' : ''}`}
              onClick={() => setActiveTab('cars')}
            >
              Cars
            </button>
            <button 
              className={`tab ${activeTab === 'tracks' ? 'active' : ''}`}
              onClick={() => setActiveTab('tracks')}
            >
              Tracks
            </button>
          </div>
        )}
      </header>

      {scanResult && (
        <div className="dashboard">
          {scanResult.installType === "launcher" && (
             <Sidebar 
               packs={scanResult.contentPacks} 
               activeTab={activeTab} 
               onTogglePack={togglePack} 
             />
          )}
          <MainContent 
             scanResult={scanResult} 
             activeTab={activeTab} 
             installPath={installPath}
          />
        </div>
      )}

      <footer>
        <div className="theme-selector">
          <label style={{ fontSize: "0.85rem", fontWeight: "bold" }}>Theme</label>
          <select value={theme} onChange={e => setTheme(e.target.value)}>
            <option value="dark">Dark</option>
            <option value="light">Light</option>
            <option value="earthy">Earthy</option>
          </select>
        </div>
      </footer>
    </div>
  );
}

function Sidebar({ packs, activeTab, onTogglePack }) {
  // Filter packs that actually have content for the current tab
  const validPacks = packs.map((pack, i) => ({pack, originalIndex: i})).filter(
    p => activeTab === "cars" ? p.pack.hasCars : p.pack.hasTracks
  );

  return (
    <div className="sidebar">
      <h3 className="sidebar-title">Content Packs</h3>
      {validPacks.length === 0 && <p style={{opacity: 0.7, fontSize: '0.85rem'}}>No packs found.</p>}
      {validPacks.map(({pack, originalIndex}) => (
        <label key={pack.name} className="pack-item">
          <input 
            type="checkbox" 
            checked={activeTab === "cars" ? pack.useCars : pack.useTracks}
            onChange={() => onTogglePack(originalIndex)}
          />
          <span style={{flex: 1, textOverflow: 'ellipsis', overflow: 'hidden'}}>{pack.name}</span>
        </label>
      ))}
    </div>
  );
}

function MainContent({ scanResult, activeTab, installPath }) {
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

function PoolGrid({ items, rootPath, activeTab, ratingFilter }) {
  if (!items || items.length === 0) {
    return <p style={{opacity: 0.5, fontStyle: 'italic', fontSize: '0.9rem'}}>Empty or loading...</p>;
  }

  // Filter out invalid items and system cars
  const validItems = items.filter(item => {
    if (!item.hasValidFile) return false;
    if (activeTab === "cars" && item.isSystemCar) return false;
    if (activeTab === "cars" && ratingFilter !== "All" && item.rating.toString() !== ratingFilter) return false;
    return true;
  });

  if (validItems.length === 0) {
    return <p style={{opacity: 0.5, fontStyle: 'italic', fontSize: '0.9rem'}}>No matching {activeTab}...</p>;
  }

  return (
    <div className="grid">
      {validItems.map(item => (
        <div key={item.folderName} className="card">
          <CardImage src={getImageSrc(item, rootPath, activeTab)} alt={item.name} />
          <div className="card-body">
            <h3 className="card-title" title={item.name}>{item.name}</h3>
            
            {activeTab === "cars" ? (
              <>
                <div className="card-subtitle">
                  <span className="badge">Rating: {CAR_RATINGS[item.rating] || "Unknown"}</span>
                </div>
                <div className="card-subtitle">
                  <span className="badge badge-obtain">Obtain: {OBTAIN_METHODS[item.obtainMethod] || "Unknown"}</span>
                </div>
              </>
            ) : (
              <>
                <div className="card-subtitle">
                  <span className="badge">Diff: {TRACK_DIFFICULTIES[item.difficulty] || "Unknown"}</span>
                  {item.hasReversed && <span className="badge badge-obtain">Reverse</span>}
                </div>
              </>
            )}
          </div>
        </div>
      ))}
    </div>
  );
}

function CardImage({ src, alt }) {
  const [error, setError] = useState(false);

  if (error) {
    return (
      <div className="card-image">
        <span className="card-image-fallback">No Image</span>
      </div>
    );
  }

  return (
    <img 
      className="card-image"
      src={src} 
      alt={alt} 
      onError={() => setError(true)}
    />
  );
}

// Helper to reliably retrieve image via Tauri URL conversion protocol
function getImageSrc(item, rootPath, activeTab) {
  const osDelim = rootPath.includes('\\') ? '\\' : '/';
  if (activeTab === "cars") {
      if (OVERRIDE_CARBOXES.includes(item.folderName.toLowerCase())) {
          return new URL(`./assets/carboxes/${item.folderName.toLowerCase()}.bmp`, import.meta.url).href;
      }
      
      if (item.carboxFilename) {
          if (item.carboxFilename.includes('\\') || item.carboxFilename.includes('/')) {
              // TCARBOX uses path relative to pack
              const safeRelativePath = item.carboxFilename.replace(/\\|\//g, osDelim);
              return convertFileSrc(`${rootPath}${osDelim}${safeRelativePath}`);
          } else {
              // Default carbox/box files are local to the car folder
              return convertFileSrc(`${rootPath}${osDelim}cars${osDelim}${item.folderName}${osDelim}${item.carboxFilename}`);
          }
      } else {
          return new URL(`./assets/carboxes/${item.folderName}.bmp`, import.meta.url).href; // Legacy fallback just in case
      }
  } else {
      return convertFileSrc(`${rootPath}${osDelim}gfx${osDelim}${item.folderName}.bmp`);
  }
}
