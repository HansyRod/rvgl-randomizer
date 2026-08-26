import { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { appLocalDataDir, join } from '@tauri-apps/api/path';
import { open, confirm } from '@tauri-apps/plugin-dialog';
import { useAppContext } from '../AppProvider';
import HistoryPanel from "../components/HistoryPanel";
import LoadSeedDialog from "./LoadSeedDialog";
import SeedSummaryPanel from "../components/SeedSummaryPanel";
import "../setup/SetupView.css";
import "./GenerationTab.css";
import { PRESETS } from "../configure/presets";
import { DEFAULT_CAR_OPTIONS } from '../utils/constants';
import { normalizeConfigureContext } from "../utils/configureContext";

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
  const { carOptions, trackOptions, featureOptions, carsSpecState, trackSpecState, cupSpecState } = configure;
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
      const selectedPreset = PRESETS.find((preset) => preset.id === configure?.preset) ?? null;

      // Intercept and strip car arrays if the user unchecked them
      const filteredSpecState = {
        ...carsSpecState,
        stockCars: carsSpecState.includeStockCars === false ? [] : carsSpecState.stockCars,
        dcCars: carsSpecState.includeDcCars === false ? [] : carsSpecState.dcCars
      };

      const showRatingOptions = carOptions?.unlockMode === "random" || carOptions?.unlockMode === "randomRatings";
      const showObtainOptions = carOptions?.unlockMode === "random" || carOptions?.unlockMode === "randomUnlock";
      const showStartingCars = carOptions?.unlockMode !== "baseGame";
      const showTrackObtainOptions = trackOptions?.unlockMode === "random" || trackOptions?.unlockMode === "randomUnlock";

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
      const sanitizedTrackOptions = trackOptions ? {
        ...trackOptions,
        includeStuntArena: showTrackObtainOptions && trackOptions.includeStuntArena,
      } : null;

      const outPath = await invoke("generate_result", {
        installPath,
        scanResult,
        carsSpecState: filteredSpecState,
        carOptions: sanitizedCarOptions,
        trackSpecState,
        trackOptions: sanitizedTrackOptions,
        featureOptions: featureOptions ?? null,
        cupSpecState: cupSpecState ?? null,
        presetId: configure?.preset ?? "basic",
        presetStockMode: {
          cars: Boolean(selectedPreset?.stockMode?.cars),
          tracks: Boolean(selectedPreset?.stockMode?.tracks),
        },
        fileName: instanceName.trim(),
        profileName: profileName.trim()
      });

      const newHistory = updateGeneratedHistory(
        generatedHistoryList,
        outPath,
        instanceName.trim(),
        profileName.trim()
      );

      let seedContext = null;
      try {
        const metadata = await invoke("read_seed_context", { filePath: outPath });
        if (metadata?.uiContext) {
          seedContext = metadata.uiContext;
        }
      } catch (err) {
        console.error("Failed to read metadata from generated config:", err);
      }

      updateCategoryCtx("generate", {
        generatedFilePath: outPath,
        generatedHistory: newHistory,
        seedContext,
      });
      setMessage(`Successfully generated: ${getFileName(outPath)}`);
    } catch (error) {
      console.error(error);
      setMessage(`Error: ${error}`);
    } finally {
      setIsGenerating(false);
    }
  };

  const [loadDialogData, setLoadDialogData] = useState(null); // { file, metadata, loadedName }

  const applyLoad = async (file, metadata, loadedName, overrides = {}) => {
    const newGenerateState = {
      generatedFilePath: file,
      instanceName: loadedName,
    };

    if (metadata?.profileName) {
      newGenerateState.profileName = metadata.profileName;
    }
    newGenerateState.seedContext = metadata?.uiContext || null;

    const resolvedProfileName = newGenerateState.profileName ?? profileName;
    const newHistory = updateGeneratedHistory(
      generatedHistoryList,
      file,
      loadedName,
      resolvedProfileName
    );

    updateCategoryCtx("generate", {
      ...newGenerateState,
      generatedHistory: newHistory,
    });
    setMessage(`Loaded existing file: ${getFileName(file)}`);

    if (metadata?.uiContext) {
      if (overrides.overrideConfigure && metadata.uiContext.configure) {
        updateCategoryCtx("configure", normalizeConfigureContext(metadata.uiContext.configure));
      }

      let currentScanResult = scanResult;

      if (overrides.overrideInstall && metadata.uiContext.setup?.installPath) {
        const targetInstallPath = metadata.uiContext.setup.installPath;
        try {
          const scanRes = await invoke("scan_install", { executablePath: targetInstallPath });
          if (scanRes) {
            currentScanResult = scanRes;
            
            const history = Array.isArray(setup.installHistory) ? setup.installHistory : [];
            const nextHistory = history.filter((entry) => entry?.path !== targetInstallPath);
            nextHistory.unshift({ path: targetInstallPath, installType: scanRes.installType });
            
            updateCategoryCtx("setup", {
              installPath: targetInstallPath,
              scanResult: currentScanResult,
              installHistory: nextHistory
            });
          }
        } catch (err) {
          console.error("Failed to scan loaded install path", err);
        }
      }

      if (overrides.overridePacks && metadata.uiContext.setup?.requiredPacks && currentScanResult?.contentPacks) {
        const requiredPacks = metadata.uiContext.setup.requiredPacks;
        
        updateCategoryCtx("app", { isFetchingPack: true });
        try {
          const updatedPacks = [];
          for (const p of currentScanResult.contentPacks) {
            const shouldBeEnabled = requiredPacks.includes(p.name);
            const newPack = { 
              ...p, 
              useCars: shouldBeEnabled && p.hasCars, 
              useTracks: shouldBeEnabled && p.hasTracks 
            };
            
            if (newPack.useCars && (!newPack.cars || newPack.cars.length === 0)) {
              newPack.cars = await invoke("scan_cars_folder", { folderPath: `${newPack.absolutePath}\\cars` });
            }
            if (newPack.useTracks && (!newPack.tracks || newPack.tracks.length === 0)) {
              newPack.tracks = await invoke("scan_levels_folder", { folderPath: `${newPack.absolutePath}\\levels` });
            }
            updatedPacks.push(newPack);
          }
          
          const updatedScanResult = { ...currentScanResult, contentPacks: updatedPacks };
          updateCategoryCtx("setup", { scanResult: updatedScanResult });
        } catch (err) {
          console.error("Failed to fetch packs while loading seed:", err);
        } finally {
          updateCategoryCtx("app", { isFetchingPack: false });
        }
      }
    }
  };

  const handleLoadFile = async () => {
    let defaultPath;
    try {
      defaultPath = await join(await appLocalDataDir(), "generated");
    } catch (err) {
      console.error("Failed to determine generated seed directory:", err);
    }

    const file = getSelectedPath(await open({
      multiple: false,
      defaultPath,
      filters: [{ name: "JSON Config", extensions: ["json"] }],
    }));
    if (!file) return;

    const loadedName = getInstanceNameFromPath(file);

    try {
      const metadata = await invoke("read_seed_context", { filePath: file });
      if (metadata?.uiContext) {
        setLoadDialogData({ file, metadata, loadedName });
      } else {
        applyLoad(file, metadata, loadedName);
      }
    } catch (err) {
      console.error("Failed to read metadata from config:", err);
      applyLoad(file, null, loadedName);
    }
  };

  async function handleLoadFromHistory(entry) {
    const loadedName = entry.instanceName || getInstanceNameFromPath(entry.path);

    try {
      const metadata = await invoke("read_seed_context", { filePath: entry.path });
      if (metadata?.uiContext) {
        setLoadDialogData({ file: entry.path, metadata, loadedName });
      } else {
        applyLoad(entry.path, metadata, loadedName);
      }
    } catch (err) {
      console.error("Failed to read metadata from history entry:", err);
      applyLoad(entry.path, null, loadedName);
    }
  }

  function handleRemoveFromHistory(pathToRemove) {
    updateCategoryCtx("generate", {
      generatedHistory: generatedHistoryList.filter((entry) => entry?.path !== pathToRemove),
    });
  }

  const isGenerateBtnDisabled = isGenerating || !carsSpecState || errors.length > 0;

  return (
    <div className="tab-container generation-tab">
      <p className="tab-description" style={{ marginTop: 0 }}>
        Create a new randomized game based on your current configuration parameters.
      </p>

      <div className="generation-tab-grid">
        <div className="control-panel left-column">
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
            className="primary core-button"
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

        <div className="right-column">
          {generatedFilePath ? (
            <SeedSummaryPanel configPath={generatedFilePath} compact={false} />
          ) : (
            <div className="empty-summary-placeholder">
              <span style={{ fontSize: "2rem" }}>📋</span>
              <p style={{ margin: 0 }}>Generate or load a seed to see its summary here.</p>
            </div>
          )}
        </div>
      </div>

      <LoadSeedDialog
        isOpen={!!loadDialogData}
        seedMetadata={loadDialogData?.metadata}
        currentInstallPath={installPath}
        onClose={() => setLoadDialogData(null)}
        onConfirm={(overrides) => {
          applyLoad(loadDialogData.file, loadDialogData.metadata, loadDialogData.loadedName, overrides);
          setLoadDialogData(null);
        }}
      />
    </div>
  );
}
