export default function Sidebar({ packs, activeTab, onTogglePack }) {
  // Filter packs that actually have content for the current tab
  const validPacks = packs.map((pack, i) => ({pack, originalIndex: i})).filter(
    p => activeTab === "cars" ? p.pack.hasCars : p.pack.hasTracks
  );

  const selectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? pack.useCars : pack.useTracks);
  const unselectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? !pack.useCars : !pack.useTracks);

  const renderPackItem = ({pack, originalIndex}) => (
    <label key={pack.name} className="pack-item">
      <input 
        type="checkbox" 
        checked={activeTab === "cars" ? pack.useCars : pack.useTracks}
        onChange={() => onTogglePack(originalIndex)}
      />
      <span style={{flex: 1, textOverflow: 'ellipsis', overflow: 'hidden'}}>{pack.name}</span>
    </label>
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
