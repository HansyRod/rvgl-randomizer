import React, { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { open } from '@tauri-apps/plugin-dialog';

export default function GenerationTab({ 
  scanResult, 
  specState, 
  generatedFilePath, 
  setGeneratedFilePath,
  instanceName,
  setInstanceName 
}) {
  const [isGenerating, setIsGenerating] = useState(false);
  const [message, setMessage] = useState("");

  const handleGenerate = async () => {
    if (!instanceName.trim()) {
      setMessage("Please enter a valid instance name.");
      return;
    }

    if (!scanResult || !specState) {
      setMessage("Missing scan result or car configuration.");
      return;
    }

    setIsGenerating(true);
    setMessage("Generating...");

    try {
      const outPath = await invoke("generate_result", {
        scanResult,
        specState, 
        fileName: instanceName.trim()
      });
      
      setGeneratedFilePath(outPath);
      setMessage(`Successfully generated: ${outPath.split(/[\\/]/).pop()}`);
    } catch (error) {
      console.error(error);
      setMessage(`Error: ${error}`);
    } finally {
      setIsGenerating(false);
    }
  };

  const handleLoadFile = async () => {
    const file = await open({
      multiple: false,
      filters: [{ name: 'JSON Config', extensions: ['json'] }]
    });
    
    if (file) {
      setGeneratedFilePath(file);
      // Optional: Auto-fill the instance name input with the loaded file's name
      const loadedName = file.split(/[\\/]/).pop().replace('.json', '');
      setInstanceName(loadedName);
      setMessage(`Loaded existing file: ${file.split(/[\\/]/).pop()}`);
    }
  };

  return (
    <div className="tab-container">
      <p className="tab-description" style={{ marginTop: 0 }}>
        Create a new randomized game based on your current configuration parameters.
      </p>

      <div className="control-panel">
        <div className="control-group">
          <label style={{ fontWeight: "bold" }}>Instance Name:</label>
          <div style={{ display: "flex", gap: "0.5rem", alignItems: "center" }}>
            <input 
              type="text" 
              value={instanceName} 
              onChange={(e) => setInstanceName(e.target.value)}
              placeholder="e.g. my-random-seed"
              style={{ flex: 1, padding: "0.5rem" }}
            />
            <span style={{ color: "var(--text-secondary)" }}>.json</span>
          </div>
          <small style={{ color: "var(--text-secondary)" }}>This name will also be used for your game profile and packlist.</small>
        </div>

        <div className="action-row">
          <button 
            className="primary" 
            onClick={handleGenerate} 
            disabled={isGenerating || !specState}
            style={{ padding: "0.5rem 1.5rem", fontSize: "1rem" }}
          >
            {isGenerating ? "Generating..." : "⚡ Generate Instance"}
          </button>
          
          <span style={{ color: "var(--text-secondary)", margin: "0 1rem" }}>OR</span>
          
          <button onClick={handleLoadFile} style={{ padding: "0.5rem 1.5rem", fontSize: "1rem" }}>
            Load Existing JSON
          </button>
        </div>

        {message && (
          <div className="status-message" style={{ marginTop: "1rem", padding: "0.75rem", backgroundColor: "var(--bg-secondary)", borderRadius: "6px" }}>
            {message}
          </div>
        )}

        {generatedFilePath && (
          <div className="active-file-box" style={{ marginTop: "1rem", padding: "1rem", border: "1px solid var(--accent)", borderRadius: "6px", backgroundColor: "var(--bg-secondary)" }}>
            <span style={{ color: "var(--accent)", marginRight: "0.5rem" }}>✓</span>
            <strong>Ready to Launch:</strong> {generatedFilePath}
          </div>
        )}
      </div>
    </div>
  );
}