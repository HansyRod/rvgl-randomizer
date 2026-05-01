import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import "../configure/carOptions/CarOptionsTab.css";

export default function LoadSeedDialog({ isOpen, onClose, onConfirm, seedMetadata, currentInstallPath }) {
  const [overrideConfigure, setOverrideConfigure] = useState(false);
  const [overrideInstall, setOverrideInstall] = useState(true);
  const [overridePacks, setOverridePacks] = useState(true);
  
  const [installPathExists, setInstallPathExists] = useState(null);
  
  const seedInstallPath = seedMetadata?.uiContext?.setup?.installPath;
  const isInstallDifferent = seedInstallPath && seedInstallPath !== currentInstallPath;
  
  useEffect(() => {
    if (isOpen && seedInstallPath) {
      if (seedInstallPath === currentInstallPath) {
        setInstallPathExists(true);
      } else {
        invoke("check_file_exists", { filePath: seedInstallPath })
          .then(exists => setInstallPathExists(exists))
          .catch(() => setInstallPathExists(false));
      }
    }
  }, [isOpen, seedInstallPath, currentInstallPath]);
  
  // Disable override if the path doesn't exist or is the same
  useEffect(() => {
    if (installPathExists === false || !isInstallDifferent) {
      setOverrideInstall(false);
    }
  }, [installPathExists, isInstallDifferent]);

  if (!isOpen) return null;
  
  return (
    <div className="search-modal-overlay" onClick={onClose}>
      <div className="search-modal-content" onClick={e => e.stopPropagation()} style={{ width: '570px', maxWidth: '90vw' }}>
        <div className="search-modal-header" style={{ paddingBottom: '1rem' }}>
          <h3 style={{ margin: 0 }}>Load Seed Settings</h3>
        </div>
        <div className="car-options-tab" style={{ padding: '1rem 1.5rem', gap: '1.25rem' }}>
          <p className="co-desc">
            This seed contains configuration metadata. Select the settings you would like to restore to your current session.
          </p>
          
          <section className="co-section">
            <h2 className="co-section-title">Install Options</h2>
            <p className="co-desc">Enable these options to load the content required to play the game with this seed.</p>
            <div className="co-checkbox-group">
              <label className="co-checkbox-row" style={{ opacity: (installPathExists === false || !isInstallDifferent) ? 0.5 : 1 }}>
                <input 
                  type="checkbox" 
                  checked={overrideInstall} 
                  onChange={e => setOverrideInstall(e.target.checked)} 
                  disabled={installPathExists === false || !isInstallDifferent} 
                />
                <span style={{ display: "flex", flexDirection: "column" }}>
                  <span>Change <strong>Active Install</strong></span>
                  <span style={{ fontSize: "0.85rem", color: "var(--text-secondary)", marginTop: "0.25rem", fontWeight: "normal" }}>
                    {isInstallDifferent ? (
                      <>
                        Change RVGL installation to:<br/>
                        <code style={{ wordBreak: 'break-all', display: 'inline-block', marginTop: '0.2rem' }}>{seedInstallPath}</code>
                        {installPathExists === false && <span style={{ color: "var(--accent)", display: "block", marginTop: "0.25rem" }}>⚠️ This path no longer exists on your system.</span>}
                      </>
                    ) : (
                      "Your current install is already the one used for this seed."
                    )}
                  </span>
                </span>
              </label>

              {seedMetadata?.uiContext?.setup?.requiredPacks && (
                <label className="co-checkbox-row">
                  <input type="checkbox" checked={overridePacks} onChange={e => setOverridePacks(e.target.checked)} />
                  <span style={{ display: "flex", flexDirection: "column" }}>
                    <span>Restore <strong>Loaded Packs</strong></span>
                    <span style={{ fontSize: "0.85rem", color: "var(--text-secondary)", marginTop: "0.25rem", fontWeight: "normal" }}>
                      Update active content packs in your setup to match the seed's requirements.
                    </span>
                  </span>
                </label>
              )}
            </div>
          </section>

          <section className="co-section">
            <h2 className="co-section-title">Configure Options</h2>
            <p className="co-desc">Enable this option to use this seed's rules as a template for generating a new seed.</p>
            <div className="co-checkbox-group">
              <label className="co-checkbox-row">
                <input type="checkbox" checked={overrideConfigure} onChange={e => setOverrideConfigure(e.target.checked)} />
                <span style={{ display: "flex", flexDirection: "column" }}>
                  <span>Restore <strong>Configure Options</strong></span>
                  <span style={{ fontSize: "0.85rem", color: "var(--text-secondary)", marginTop: "0.25rem", fontWeight: "normal" }}>
                    Overwrite all Car, Track, and Cup configurations with the seed's settings.
                  </span>
                </span>
              </label>
            </div>
          </section>
        </div>
        <div className="search-modal-footer">
          <button onClick={onClose} style={{ marginRight: '0.5rem' }}>Cancel</button>
          <button className="primary" onClick={() => {
            onConfirm({ overrideConfigure, overrideInstall, overridePacks });
          }}>Load Selected Settings</button>
        </div>
      </div>
    </div>
  );
}
