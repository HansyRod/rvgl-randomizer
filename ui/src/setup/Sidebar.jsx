import { useEffect, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";
import { handleTogglePack } from "./packHelpers";

export default function Sidebar() {

  const context = useAppContext();
  const [refreshNotice, setRefreshNotice] = useState("");

  // Destructure categories
  const { state: { app, setup }, updateCategoryCtx } = context;
  
  // Destructure individual variables
  const { isFetchingPack } = app;
  const { scanResult, setupTab : activeTab } = setup;
  const packs = scanResult?.contentPacks || [];

  const togglePack = (packIndex) => {
    handleTogglePack(packIndex, activeTab, scanResult, updateCategoryCtx);
  };

  // Filter packs that actually have content for the current tab
  const validPacks = packs.map((pack, i) => ({pack, originalIndex: i})).filter(
    p => activeTab === "cars" ? p.pack.hasCars : p.pack.hasTracks
  );

  const selectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? pack.useCars : pack.useTracks);
  const unselectedPacks = validPacks.filter(({pack}) => activeTab === "cars" ? !pack.useCars : !pack.useTracks);

  useEffect(() => {
    if (!refreshNotice) return undefined;

    const timeoutId = window.setTimeout(() => {
      setRefreshNotice("");
    }, 5000);

    return () => window.clearTimeout(timeoutId);
  }, [refreshNotice]);

  const handleRefreshPack = async (e, pack, originalIndex) => {
    e.preventDefault();
    e.stopPropagation();

    if (!scanResult) return;

    updateCategoryCtx("app", { isFetchingPack: true });

    try {
      const updatedPack = await invoke("scan_pack_folder", {
        folderPath: pack.absolutePath,
        useCars: pack.useCars,
        useTracks: pack.useTracks,
      });

      const nextContentPacks = [...scanResult.contentPacks];
      nextContentPacks[originalIndex] = updatedPack;
      const newResult = {
        ...scanResult,
        contentPacks: nextContentPacks,
      };

      setRefreshNotice("");
      updateCategoryCtx("setup", { scanResult: newResult });
    } catch (err) {
      console.error(err);
      const errorMessage = typeof err === "string"
        ? err
        : err?.message || "Failed to refresh pack contents.";
      setRefreshNotice(`Could not refresh "${pack.name}": ${errorMessage}`);
    } finally {
      updateCategoryCtx("app", { isFetchingPack: false });
    }
  };

  const renderPackItem = ({pack, originalIndex}) => (
    <div key={pack.name} className="pack-item" style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
      <label style={{ display: "flex", alignItems: "center", flex: 1, overflow: "hidden" }}>
        <input 
          type="checkbox" 
          checked={activeTab === "cars" ? pack.useCars : pack.useTracks}
          onChange={() => togglePack(originalIndex)}
          disabled={isFetchingPack}
        />
        <span style={{ marginLeft: "6px", textOverflow: 'ellipsis', overflow: 'hidden', whiteSpace: "nowrap" }}>
          {pack.name}
        </span>
      </label>
      
      {((activeTab === "cars" && pack.useCars) || (activeTab === "tracks" && pack.useTracks)) && (
        <button 
          onClick={(e) => handleRefreshPack(e, pack, originalIndex)}
          title="Refresh pack contents"
          disabled={isFetchingPack}
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

      {refreshNotice && (
        <div
          style={{
            marginBottom: "0.8rem",
            padding: "0.55rem 0.7rem",
            borderRadius: "8px",
            background: "color-mix(in srgb, #b43c3c 14%, transparent)",
            border: "1px solid color-mix(in srgb, #b43c3c 42%, transparent)",
            color: "var(--text-primary)",
            fontSize: "0.8rem",
          }}
        >
          {refreshNotice}
        </div>
      )}
      
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
