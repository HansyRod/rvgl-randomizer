import { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { open, confirm } from '@tauri-apps/plugin-dialog';
import { useAppContext } from '../AppProvider';
import HistoryPanel from "../components/HistoryPanel";
import "../setup/SetupView.css";
import { DEFAULT_CAR_OPTIONS } from '../utils/constants';

const { poolRatingDistributions, attrRatingDistributions } = DEFAULT_CAR_OPTIONS;

function getSelectedPath(selected) {
  if (typeof selected === "string") {
    return selected;
  }
  if (selected && typeof selected === "object" && typeof selected.path === "string") {
    return selected.path;
  }
  return "";
}

function getFileName(path) {
  return path.split(/[\\/]/).pop() || path;
}

function getInstanceNameFromPath(path) {
  return getFileName(path).replace(/\.json$/i, "");
}

function updateGeneratedHistory(currentHistory, path, instanceName, profileName) {
  const history = Array.isArray(currentHistory) ? currentHistory : [];
  const next = history.filter((entry) => entry?.path && entry.path !== path);
  next.unshift({ path, instanceName, profileName });
  return next;
}

export default function GenerationTab({errors}) {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { setup, configure, generate } = state;
  
  // Destructure individual variables
  const { installPath, scanResult } = setup;
  const { carOptions, trackOptions, carsSpecState, trackSpecState, cupSpecState } = configure;
  const { generatedFilePath, instanceName, profileName, generatedHistory } = generate;
  const generatedHistoryList = Array.isArray(generatedHistory) ? generatedHistory : [];

  const [isGenerating, setIsGenerating] = useState(false);
  const [message, setMessage] = useState("");

  const handleGenerate = async () => {
    if (!instanceName.trim()) {
      setMessage("Please enter a valid instance name.");
      return;
    }

    if (!profileName.trim()) {
      setMessage("Please enter a valid profile name.");
      return;
    }

    if (!scanResult || !carsSpecState || !trackSpecState) {
      setMessage("Missing scan result or randomization configuration.");
      return;
    }

    setIsGenerating(true);
    setMessage("Checking profile...");

    try {
      const exists = await invoke("check_profile_exists", { 
        executablePath: installPath, 
        profileName: profileName.trim() 
      });

      if (exists) {
        const proceed = await confirm(
          "This profile name is already being used in this RVGL installation. Creating a randomization with the same profile may lead to conflicting data.\n\nDo you want to proceed anyway?",
          { title: "Profile Exists", kind: "warning" }
        );
        if (!proceed) {
          setIsGenerating(false);
          setMessage("Generation aborted.");
          return;
        }
      }
    } catch (e) {
      console.error(e);
    }

    setMessage("Generating...");

    try {
      // Intercept and strip arrays if the user unchecked them
      const filteredSpecState = {
        ...carsSpecState,
        stockCars: carsSpecState.includeStockCars === false ? [] : carsSpecState.stockCars,
        dcCars: carsSpecState.includeDcCars === false ? [] : carsSpecState.dcCars
      };
      const filteredTrackSpecState = {
        ...trackSpecState,
        tracks: trackSpecState.includeTracks === false ? [] : trackSpecState.tracks
      };

      const showRatingOptions = carOptions?.unlockMode === "random" || carOptions?.unlockMode === "randomRatings";
      const showObtainOptions = carOptions?.unlockMode === "random" || carOptions?.unlockMode === "randomUnlock";
      const showStartingCars = carOptions?.unlockMode !== "baseGame";

      const sanitizedCarOptions = carOptions ? {
        ...carOptions,
        // Pool/attr distributions only meaningful when Rating Options is visible
        poolRatingDistributions: showRatingOptions ? carOptions.poolRatingDistributions : poolRatingDistributions,
        attrRatingDistributions: showRatingOptions ? carOptions.attrRatingDistributions : attrRatingDistributions,
        // Cheat-only / stunt-arena only apply when obtain is being randomized
        includeCheatOnly: showObtainOptions && carOptions.includeCheatOnly,
        includeStuntArena: showObtainOptions && carOptions.includeStuntArena,
        // Starting car config only applies outside baseGame mode
        enableStartingCars: showStartingCars && carOptions.enableStartingCars,
      } : null;

      const outPath = await invoke("generate_result", {
        installPath,
        scanResult,
        carsSpecState: filteredSpecState,
        carOptions: sanitizedCarOptions,
        trackSpecState: filteredTrackSpecState,
        trackOptions: trackOptions ?? null,
        cupSpecState: cupSpecState ?? null,
        fileName: instanceName.trim(),
        profileName: profileName.trim()
      });

      const newHistory = updateGeneratedHistory(
        generatedHistoryList,
        outPath,
        instanceName.trim(),
        profileName.trim()
      );

      updateCategoryCtx("generate", {
        generatedFilePath: outPath,
        generatedHistory: newHistory,
      });
      setMessage(`Successfully generated: ${getFileName(outPath)}`);
    } catch (error) {
      console.error(error);
      setMessage(`Error: ${error}`);
    } finally {
      setIsGenerating(false);
    }
  };

  const handleLoadFile = async () => {
    const file = getSelectedPath(await open({
      multiple: false,
      filters: [{ name: "JSON Config", extensions: ["json"] }],
    }));
    if (!file) return;

    const loadedName = getInstanceNameFromPath(file);
    const newGenerateState = {
      generatedFilePath: file,
      instanceName: loadedName,
    };

    try {
      const configData = await invoke("read_config_file", { filePath: file });
      if (configData?.metadata?.profileName) {
        newGenerateState.profileName = configData.metadata.profileName;
      }
    } catch (err) {
      console.error("Failed to read profile name from config:", err);
    }

    const resolvedProfileName = newGenerateState.profileName ?? profileName;
    const newHistory = updateGeneratedHistory(
      generatedHistoryList,
      file,
      newGenerateState.instanceName,
      resolvedProfileName
    );

    updateCategoryCtx("generate", {
      ...newGenerateState,
      generatedHistory: newHistory,
    });
    setMessage(`Loaded existing file: ${getFileName(file)}`);
  };

  function handleLoadFromHistory(entry) {
    const nextInstanceName = entry.instanceName || getInstanceNameFromPath(entry.path);
    const nextProfileName = entry.profileName || profileName;
    const newHistory = updateGeneratedHistory(
      generatedHistoryList,
      entry.path,
      nextInstanceName,
      nextProfileName
    );

    updateCategoryCtx("generate", {
      generatedFilePath: entry.path,
      instanceName: nextInstanceName,
      profileName: nextProfileName,
      generatedHistory: newHistory,
    });
    setMessage(`Switched to: ${getFileName(entry.path)}`);
  }

  function handleRemoveFromHistory(pathToRemove) {
    updateCategoryCtx("generate", {
      generatedHistory: generatedHistoryList.filter((entry) => entry?.path !== pathToRemove),
    });
  }

  const isGenerateBtnDisabled = isGenerating || !carsSpecState || errors.length > 0;

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
              onChange={(e) => updateCategoryCtx("generate", { instanceName: e.target.value })}
              placeholder="e.g. my-random-seed"
              style={{ flex: 1, padding: "0.5rem" }}
            />
            <span style={{ color: "var(--text-secondary)" }}>.json</span>
          </div>
          <small style={{ color: "var(--text-secondary)" }}>This name will be used for your generated configuration file.</small>
        </div>

        <div className="control-group" style={{ marginTop: "1rem" }}>
          <label style={{ fontWeight: "bold" }}>Profile Name:</label>
          <div style={{ display: "flex", gap: "0.5rem", alignItems: "center" }}>
            <input 
              type="text" 
              value={profileName} 
              onChange={(e) => {
                if (e.target.value.length <= 15) {
                  updateCategoryCtx("generate", { profileName: e.target.value });
                }
              }}
              placeholder="e.g. player1"
              maxLength={15}
              style={{ flex: 1, padding: "0.5rem" }}
            />
          </div>
          <small style={{ color: "var(--text-secondary)" }}>Maximum 15 characters. This name will be used for your game profile and packlist.</small>
        </div>

        <div className="action-row">
          <button 
            className="primary" 
            onClick={handleGenerate} 
            disabled={isGenerateBtnDisabled}
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

        <HistoryPanel
          title="Previous seeds"
          items={generatedHistoryList.filter((entry) => entry?.path)}
          activeKey={generatedFilePath}
          getKey={(entry) => entry.path}
          getPrimaryText={(entry) => getFileName(entry.path)}
          getPrimaryTitle={(entry) => entry.path}
          getBadgeLabel={(entry) => entry.profileName || ""}
          getBadgeClassName={() => "install-badge badge-seed"}
          actionLabel="Load"
          onAction={handleLoadFromHistory}
          onRemove={handleRemoveFromHistory}
          disabled={isGenerating}
          summaryLabel="generated seeds"
          style={{ marginTop: "0.5rem" }}
        />
      </div>
    </div>
  );
}