import { useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { confirm } from "@tauri-apps/plugin-dialog";

import { useAppContext } from "./AppProvider";
import "./App.css";

import StepNavigator from "./navigation/StepNavigator";
import ModeView from "./navigation/ModeView";
import ImportView from "./import/ImportView";
import { canNavigateToStep, getAccessibleStep, getSteps } from "./navigation/steps";
import SetupView from "./setup/SetupView";
import ConfigureView from "./configure/ConfigureView";
import PlayView from "./play/PlayView";

// Generate + Play
import GenerationTab from "./generate/GenerationTab";

// Validation
import { useValidation } from "./validation/useValidation";
import { useLaunchValidation } from "./validation/useLaunchValidation";
import ValidationStatus from "./validation/ValidationStatus";
import { makeDefaultTrackSpec } from "./utils/constants";
import { isEffectiveStockCarsMode, isEffectiveStockTracksMode } from "./validation/stockMode";
import { normalizeAppContext } from "./utils/configureContext";
import { formatInstallError } from "./setup/installValidation";
import { refreshTracks, scanResultNeedsRefresh } from "./utils/trackScan";

export default function App() {
  const { state, resetContext, updateContext, updateCategoryCtx } = useAppContext();
  const { app, setup, configure } = state;
  const { preset, carsSpecState, trackSpecState } = configure || {};
  const { isLoading, theme, isFetchingPack } = app;
  const activeStep = getAccessibleStep(state, app?.activeStep ?? "mode");
  const steps = getSteps(app.mode);
  const activeIdx = steps.findIndex(step => step.id === activeStep);
  const previousStep = steps[activeIdx - 1]?.id;
  const nextStep = steps[activeIdx + 1]?.id;

  // Combine sync and async validation results
  const { errors: syncErrors, warnings: syncWarnings, infos: syncInfos = [] } = useValidation();
  const { errors: asyncErrors, warnings: asyncWarnings, infos: asyncInfos = [] } = useLaunchValidation();
  const allErrors   = [...syncErrors,   ...asyncErrors];
  const allWarnings = [...syncWarnings, ...asyncWarnings];
  const allInfos    = [...syncInfos,    ...asyncInfos];

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  // Clean up includeDcCars if stock mode is active
  useEffect(() => {
    if (app.mode === "randomizer" && setup.scanResult) {
      const isStockMode = isEffectiveStockCarsMode(setup.scanResult, preset);
      if (isStockMode && carsSpecState?.includeDcCars) {
        updateCategoryCtx("configure", {
          carsSpecState: {
            ...carsSpecState,
            includeDcCars: false
          }
        });
      }
    }
  }, [app.mode, setup.scanResult, preset, carsSpecState, updateCategoryCtx]);

  useEffect(() => {
    if (app.mode !== "randomizer" || !setup.scanResult || !trackSpecState?.tracks) {
      return;
    }

    const isStockTracksMode = isEffectiveStockTracksMode(setup.scanResult, preset);
    const roofIndex = trackSpecState.tracks.findIndex(row => row?.id?.toLowerCase() === "roof");

    if (isStockTracksMode) {
      if (roofIndex !== -1) {
        const cachedRoofTrackRow = trackSpecState.tracks[roofIndex];
        const tracks = trackSpecState.tracks.filter((_, index) => index !== roofIndex);

        updateCategoryCtx("configure", {
          trackSpecState: {
            ...trackSpecState,
            tracks,
            cachedRoofTrackRow
          }
        });
      }
      return;
    }

    if (roofIndex === -1) {
      const roofRow = trackSpecState.cachedRoofTrackRow ?? makeDefaultTrackSpec(["roof"])[0];
      const tracks = [...trackSpecState.tracks];
      tracks.splice(Math.min(4, tracks.length), 0, roofRow);

      updateCategoryCtx("configure", {
        trackSpecState: {
          ...trackSpecState,
          tracks
        }
      });
    }
  }, [app.mode, setup.scanResult, preset, trackSpecState, updateCategoryCtx]);

  // Load cache on mount
  useEffect(() => {
    const initCache = async () => {
      try {
        const cache = await invoke("load_cache");
        const normalizedCache = normalizeAppContext(cache);
        updateContext(normalizedCache);

        const cachedInstallPath = normalizedCache?.setup?.installPath;
        if (cachedInstallPath) {
          try {
            await invoke("verify_rvgl_executable", {
              executablePath: cachedInstallPath,
            });

            const cachedScanResult = normalizedCache?.setup?.scanResult;
            if (scanResultNeedsRefresh(cachedScanResult)) {
              try {
                const refreshedScanResult = await refreshTracks(
                  cachedScanResult,
                  cachedInstallPath,
                );
                updateCategoryCtx("setup", { scanResult: refreshedScanResult });
              } catch (error) {
                // Keep the cached scan if a track refresh fails. The user can
                // still use the manual Refresh button in the setup panel.
                console.error("Failed to refresh tracks:", error);
              }
            }

            updateCategoryCtx("setup", { installError: "" });
          } catch (error) {
            const cachedHistory = normalizedCache?.setup?.installHistory || [];
            updateCategoryCtx("setup", {
              scanResult: null,
              installError: formatInstallError(error, false),
              installHistory: cachedHistory.filter((entry) => entry.path !== cachedInstallPath),
            });
            updateCategoryCtx("app", { activeStep: "setup" });
          }
        }
      } catch (error) {
        console.error("Failed to load cache:", error);
      } finally {
        updateCategoryCtx("app", { isLoading: false });
      }
    };
    initCache();
  }, []);

  // Save cache on change
  useEffect(() => {
    if (isLoading) return;
    invoke("save_cache", { data: state }).catch(console.error);
  }, [state, isLoading]);

  // Poll every 3 s to check if the game process is still running.
  // When it exits, reset runningPid back to 0.
  const runningPid = state.play?.runningPid ?? 0;
  useEffect(() => {
    if (runningPid === 0) return;

    const intervalId = setInterval(async () => {
      try {
        const isRunning = await invoke("is_process_running", { pid: runningPid });
        if (!isRunning) {
          updateCategoryCtx("play", { runningPid: 0 });
        }
      } catch (error) {
        console.error("is_process_running check failed:", error);
        // If the command itself errors, assume the process is gone.
        updateCategoryCtx("play", { runningPid: 0 });
      }
    }, 3000);

    return () => clearInterval(intervalId);
  }, [runningPid]);

  function goToStep(stepId) {
    if (!canNavigateToStep(state, stepId)) {
      return;
    }
    updateCategoryCtx("app", { activeStep: stepId });
  }

  async function handleClearData() {
    const confirmed = await confirm("Are you sure you want to clear the app cache?", {
      title: "Clear Data",
      kind: "warning",
    });
    if (confirmed) {
      await invoke("clear_cache");
      resetContext();
      updateCategoryCtx("app", { isLoading: false });
    }
  }

  if (isLoading) {
    return <div className="container"><div className="loading-overlay">Initializing…</div></div>;
  }

  return (
    <div className="container">
      {isFetchingPack && <div className="loading-overlay">Loading assets…</div>}

      <header>
        <div className="header-top">
          <div className="header-title-area">
            <h1>RVGL Randomizer</h1>
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: "0.75rem" }}>
            <div className="theme-selector">
              <label style={{ fontSize: "0.85rem", fontWeight: "bold" }}>Theme</label>
              <select value={theme} onChange={e => updateCategoryCtx("app", { theme: e.target.value })}>
                <option value="dark">Dark</option>
                <option value="light">Light</option>
                <option value="earthy">Earthy</option>
              </select>
            </div>
            <button
              onClick={handleClearData}
              style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem", backgroundColor: "#7a2626", color: "white" }}
            >
              Clear Cache
            </button>
          </div>
        </div>

        <StepNavigator
          activeStep={activeStep}
          onStepClick={goToStep}
          errors={allErrors}
          warnings={allWarnings}
        />
      </header>

      <div className="dashboard" style={{ overflow: "hidden" }}>
        {activeStep === "mode" && <ModeView />}
        {activeStep === "setup"     && <SetupView />}
        {activeStep === "configure" && <ConfigureView />}
        {activeStep === "generate"  && (
          <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
            <GenerationTab errors={syncErrors} warnings={syncWarnings} />
          </div>
        )}
        {activeStep === "import"    && <ImportView />}
        {activeStep === "play"      && <PlayView errors={allErrors} />}
      </div>

      <footer>
        <ValidationStatus
          errors={allErrors}
          warnings={allWarnings}
          infos={allInfos}
          activeStep={activeStep}
          steps={steps}
          onNavigate={goToStep}
        />
        <div style={{ display: "flex", gap: "0.75rem", marginLeft: "auto" }}>
          {previousStep && (
            <button
              className="btn-secondary"
              onClick={() => goToStep(previousStep)}
            >
              ← Back
            </button>
          )}
          {nextStep && (
            <button
              className="btn-primary"
              disabled={!canNavigateToStep(state, nextStep)}
              onClick={() => goToStep(nextStep)}
            >
              {nextStep === "play" ? "Continue to Play →" : "Continue →"}
            </button>
          )}
        </div>
      </footer>
    </div>
  );
}
