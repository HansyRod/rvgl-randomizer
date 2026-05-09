import { useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { confirm } from "@tauri-apps/plugin-dialog";

import { useAppContext } from "./AppProvider";
import "./App.css";

import StepNavigator from "./navigation/StepNavigator";
import SetupView from "./setup/SetupView";
import ConfigureView from "./configure/ConfigureView";
import PlayView from "./play/PlayView";

// Generate + Play
import GenerationTab from "./generate/GenerationTab";

// Validation
import { useValidation } from "./validation/useValidation";
import { useLaunchValidation } from "./validation/useLaunchValidation";
import ValidationStatus from "./validation/ValidationStatus";
import { getIsStockCars } from "./validation/validationUtils";

export default function App() {
  const { state, resetContext, updateContext, updateCategoryCtx } = useAppContext();
  const { app, setup, configure } = state;
  const { isLoading, theme, isFetchingPack } = app;
  const activeStep = app?.activeStep ?? "setup";

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
    if (setup.scanResult) {
      const isStockMode = getIsStockCars(setup.scanResult);
      if (isStockMode && configure.carsSpecState?.includeDcCars) {
        updateCategoryCtx("configure", {
          carsSpecState: {
            ...configure.carsSpecState,
            includeDcCars: false
          }
        });
      }
    }
  }, [setup.scanResult, configure.carsSpecState?.includeDcCars, updateCategoryCtx]);

  // Load cache on mount
  useEffect(() => {
    const initCache = async () => {
      try {
        const cache = await invoke("load_cache");
        updateContext(cache);
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
        {activeStep === "setup"     && <SetupView onContinue={() => goToStep("configure")} />}
        {activeStep === "configure" && <ConfigureView />}
        {activeStep === "generate"  && (
          <div style={{ flex: 1, overflowY: "auto", padding: "1rem" }}>
            <GenerationTab errors={syncErrors} warnings={syncWarnings} />
          </div>
        )}
        {activeStep === "play"      && <PlayView errors={allErrors} />}
      </div>

      <footer>
        <ValidationStatus
          errors={allErrors}
          warnings={allWarnings}
          infos={allInfos}
          activeStep={activeStep}
          onNavigate={goToStep}
        />
        <div style={{ display: "flex", gap: "0.75rem", marginLeft: "auto" }}>
          {activeStep !== "setup" && (
            <button
              className="btn-secondary"
              onClick={() => {
                const steps = ["setup", "configure", "generate", "play"];
                const idx = steps.indexOf(activeStep);
                if (idx > 0) goToStep(steps[idx - 1]);
              }}
            >
              ← Back
            </button>
          )}
          {activeStep !== "play" && setup.scanResult && (
            <button
              className="btn-primary"
              onClick={() => {
                const steps = ["setup", "configure", "generate", "play"];
                const idx = steps.indexOf(activeStep);
                if (idx < steps.length - 1) goToStep(steps[idx + 1]);
              }}
            >
              {activeStep === "generate" ? "Continue to Play →" : "Continue →"}
            </button>
          )}
        </div>
      </footer>
    </div>
  );
}
