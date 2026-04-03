export default function Sidebar({ packs, activeTab, onTogglePack }) {
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
