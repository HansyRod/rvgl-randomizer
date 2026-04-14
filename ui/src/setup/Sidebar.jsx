import { useAppContext } from "../AppProvider";
import { handleTogglePack } from "./packHelpers";

export default function Sidebar() {

  const context = useAppContext();

  // Destructure categories
  const { state: { setup }, updateCategoryCtx } = context;
  
  // Destructure individual variables
  const { scanResult, setupTab : activeTab } = setup;
  const { contentPacks: packs } = scanResult || {};

  const togglePack = (packIndex) => {
    handleTogglePack(packIndex, activeTab, scanResult, updateCategoryCtx);
  };

  // Filter packs that actually have content for the current tab
  const validPacks = packs.map((pack, i) => ({pack, originalIndex: i})).filter(
    p => activeTab === "cars" ? p.pack.hasCars : p.pack.hasTracks
  );

  const selectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? pack.useCars : pack.useTracks);
  const unselectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? !pack.useCars : !pack.useTracks);

  const handleRefreshPack = async (e, pack, originalIndex) => {
    e.preventDefault();
    e.stopPropagation();
    
    // You will need to implement `scan_pack_folder` in Rust to return a single updated ContentPack
    try {
      console.log("Refreshing pack:", pack.name);
      // const updatedPack = await invoke("scan_pack_folder", { folderPath: pack.absolutePath });
      // const newResult = JSON.parse(JSON.stringify(scanResult));
      // newResult.contentPacks[originalIndex] = updatedPack;
      // setScanResult(newResult);
      alert("Refresh single pack coming soon! Needs Rust backend command.");
    } catch (err) {
      console.error(err);
    }
  };

  const renderPackItem = ({pack, originalIndex}) => (
    <div key={pack.name} className="pack-item" style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
      <label style={{ display: "flex", alignItems: "center", flex: 1, overflow: "hidden" }}>
        <input 
          type="checkbox" 
          checked={activeTab === "cars" ? pack.useCars : pack.useTracks}
          onChange={() => togglePack(originalIndex)}
        />
        <span style={{ marginLeft: "6px", textOverflow: 'ellipsis', overflow: 'hidden', whiteSpace: "nowrap" }}>
          {pack.name}
        </span>
      </label>
      
      {((activeTab === "cars" && pack.useCars) || (activeTab === "tracks" && pack.useTracks)) && (
        <button 
          onClick={(e) => handleRefreshPack(e, pack, originalIndex)}
          title="Refresh pack contents"
          style={{ background: "transparent", border: "none", cursor: "pointer", padding: "0 4px", color: "var(--text-secondary)" }}
        >
          ↻
        </button>
      )}
    </div>
  );

  return (
    <div className="sidebar">
      <h3 className="sidebar-title">Content Packs</h3>
      <div style={{ fontSize: "0.8rem", color: "var(--text-secondary)", marginBottom: "0.8rem", fontStyle: "italic" }}>
        Packs included in randomization:
      </div>
      
      {validPacks.length === 0 && <p style={{opacity: 0.7, fontSize: '0.85rem'}}>No packs found.</p>}
      
      {selectedPacks.length > 0 && (
        <div style={{ display: "flex", flexDirection: "column", gap: "0.3rem" }}>
          {selectedPacks.map(renderPackItem)}
        </div>
      )}

      {unselectedPacks.length > 0 && (
        <div style={{ display: "flex", flexDirection: "column", gap: "0.3rem", marginTop: "1rem" }}>
          <div style={{ fontSize: "0.8rem", color: "var(--text-secondary)", marginBottom: "0.4rem", fontStyle: "italic" }}>
            Unselected packs:
          </div>
          {unselectedPacks.map(renderPackItem)}
        </div>
      )}
    </div>
  );
}