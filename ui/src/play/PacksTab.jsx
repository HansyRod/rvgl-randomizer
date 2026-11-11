import { useAppContext } from "../AppProvider";
import { getAutoPacks } from "../setup/packHelpers";
import { getArchipelagoPacks } from "./archipelagoPacks";

function PacksTab() {

  const { state, updateCategoryCtx, updateArchipelagoCategoryCtx } = useAppContext();

  // Destructure categories
  const { app, setup, play, archipelago } = state;
  const isArchipelago = app.mode === "archipelago";
  
  // Destructure individual variables
  const { scanResult } = setup;
  const extraPacks = isArchipelago ? archipelago.play.extraPacks : play.extraPacks;

  const allPacks = scanResult?.contentPacks || [];
  let autoPacks = getAutoPacks(scanResult);

  let selectedContentPacks = allPacks.filter(
    pack => pack.useCars || pack.useTracks
  );

  let extraPackCandidates = allPacks.filter(
    pack => 
      !autoPacks.includes(pack.name.toLowerCase()) && 
      !pack.useCars && !pack.useTracks && 
      !pack.hasCars && !pack.hasTracks
  );

  if (isArchipelago) {
    const packs = getArchipelagoPacks(scanResult, archipelago.setup.scanResult, archipelago.import.seedMetadata, extraPacks);
    autoPacks = packs.autoPacks;
    selectedContentPacks = packs.selectedContentPacks;
    extraPackCandidates = packs.extraPackCandidates;
  }

  const toggleExtraPack = (packName) => {
    const newPackList = extraPacks.includes(packName)
      ? extraPacks.filter(p => p !== packName)
      : [...extraPacks, packName];
    if (isArchipelago) {
      updateArchipelagoCategoryCtx("play", { extraPacks: newPackList });
    } else {
      updateCategoryCtx("play", { extraPacks: newPackList });
    }
  };

  return (
    <div className="tab-container packs-tab">
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
        <h3>{isArchipelago ? "Required car / track packs:" : "Selected car / track packs:"}</h3>
        {selectedContentPacks.length === 0 ? (
          <p className="empty-text">{isArchipelago ? "No required content packs found. Check Setup." : "No content packs selected in Cars/Tracks tabs."}</p>
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
