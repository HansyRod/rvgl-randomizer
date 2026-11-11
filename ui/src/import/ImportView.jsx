import { useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { useAppContext } from "../AppProvider";
import HistoryPanel from "../components/HistoryPanel";
import SeedSummaryPanel from "../components/SeedSummaryPanel";
import { getFileName, selectSeedFile } from "../utils/seedFile";
import { getArchipelagoMetadata } from "./archipelagoSeed";
import "../setup/SetupView.css";
import "../generate/GenerationTab.css";

export default function ImportView() {
  const { state, updateArchipelagoCategoryCtx } = useAppContext();
  const { importedFilePath, seedMetadata, importedHistory } = state.archipelago.import;
  const history = Array.isArray(importedHistory) ? importedHistory : [];
  const [isLoading, setIsLoading] = useState(false);
  const [message, setMessage] = useState("");
  const [error, setError] = useState("");

  async function loadFile(file) {
    setIsLoading(true);
    setMessage("");
    setError("");
    try {
      const config = await invoke("read_config_file", { filePath: file });
      const metadata = getArchipelagoMetadata(config);
      const nextHistory = history.filter(entry => entry?.path && entry.path !== file);
      nextHistory.unshift({ path: file, slotName: metadata.slotName, profileName: metadata.profileName });
      updateArchipelagoCategoryCtx("import", {
        importedFilePath: file,
        seedMetadata: metadata,
        importedHistory: nextHistory,
      });
      setMessage(`Loaded Archipelago player file: ${getFileName(file)}`);
    } catch (err) {
      setError(`Could not load ${getFileName(file)}: ${err instanceof Error ? err.message : String(err)}`);
    } finally {
      setIsLoading(false);
    }
  }

  async function handleLoadFile() {
    try {
      const file = await selectSeedFile();
      if (file) {
        await loadFile(file);
      }
    } catch (err) {
      setError(`Could not open the file chooser: ${err instanceof Error ? err.message : String(err)}`);
    }
  }

  return (
    <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
      <div className="tab-container generation-tab">
        <p className="tab-description" style={{ marginTop: 0 }}>
          Load the .rvgl.json generated for your Archipelago player slot.
        </p>
        <div className="generation-tab-grid">
          <div className="control-panel left-column">
            <div className="action-row">
              <button className="primary core-button" onClick={handleLoadFile} disabled={isLoading} style={{ padding: "0.5rem 1.5rem", fontSize: "1rem" }}>
                {isLoading ? "Loading..." : "Load Archipelago JSON"}
              </button>
            </div>
            {(message || error) && (
              <div className="status-message" role={error ? "alert" : "status"} style={{ marginTop: "1rem", padding: "0.75rem", backgroundColor: "var(--bg-secondary)", borderRadius: "6px" }}>
                {error || message}
              </div>
            )}
            <HistoryPanel
              title="Previous Archipelago runs"
              items={history.filter(entry => entry?.path)}
              activeKey={importedFilePath}
              getKey={entry => entry.path}
              getPrimaryText={entry => getFileName(entry.path)}
              getBadgeLabel={entry => `${entry.slotName} · ${entry.profileName}`}
              getBadgeClassName={() => "install-badge badge-seed"}
              actionLabel="Load"
              onAction={entry => loadFile(entry.path)}
              onRemove={path => updateArchipelagoCategoryCtx("import", {
                importedHistory: history.filter(entry => entry.path !== path),
              })}
              disabled={isLoading}
              summaryLabel="Archipelago runs"
              style={{ marginTop: "0.5rem" }}
            />
          </div>
          <div className="right-column">
            {importedFilePath && seedMetadata ? (
              <SeedSummaryPanel configPath={importedFilePath} seedMetadata={seedMetadata} mode="archipelago" compact={false} />
            ) : (
              <div className="empty-summary-placeholder">
                <span style={{ fontSize: "2rem" }}>📋</span>
                <p style={{ margin: 0 }}>Load your Archipelago file to see its summary here.</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
