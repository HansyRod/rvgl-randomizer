import React from 'react';

function PacksTab({ scanResult, extraPacks, setExtraPacks }) {
  const allPacks = scanResult?.contentPacks || [];

  const autoPacks = ["rvgl_win64", "rvgl_assets"];
  
  const selectedContentPacks = allPacks.filter(
    pack => pack.useCars || pack.useTracks
  );

  const extraPackCandidates = allPacks.filter(
    pack => 
      !autoPacks.includes(pack.name.toLowerCase()) && 
      !pack.useCars && !pack.useTracks && 
      !pack.hasCars && !pack.hasTracks
  );

  const toggleExtraPack = (packName) => {
    if (extraPacks.includes(packName)) {
      setExtraPacks(extraPacks.filter(p => p !== packName));
    } else {
      setExtraPacks([...extraPacks, packName]);
    }
  };

  return (
    <div className="tab-container packs-tab">
      <p className="tab-description" style={{ marginTop: 0 }}>
        Configure which content packs are loaded when the randomized game launches.
      </p>

      <div className="pack-group">
        <h3>Automatically selected packs:</h3>
        <ul className="pack-list">
          {autoPacks.map(packName => (
            <li key={packName}>
              <label>
                <input type="checkbox" checked disabled />
                {packName}
              </label>
            </li>
          ))}
        </ul>
      </div>

      <div className="pack-group">
        <h3>Selected car / track packs:</h3>
        {selectedContentPacks.length === 0 ? (
          <p className="empty-text">No content packs selected in Cars/Tracks tabs.</p>
        ) : (
          <ul className="pack-list">
            {selectedContentPacks.map(pack => (
              <li key={pack.name}>
                <label>
                  <input type="checkbox" checked disabled />
                  {pack.name}
                </label>
              </li>
            ))}
          </ul>
        )}
      </div>

      <div className="pack-group">
        <h3>Extra packs:</h3>
        {extraPackCandidates.length === 0 ? (
          <p className="empty-text">No extra packs available.</p>
        ) : (
          <ul className="pack-list">
            {extraPackCandidates.map(pack => (
              <li key={pack.name}>
                <label>
                  <input 
                    type="checkbox" 
                    checked={extraPacks.includes(pack.name)}
                    onChange={() => toggleExtraPack(pack.name)}
                  />
                  {pack.name}
                </label>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  );
}

export default PacksTab;