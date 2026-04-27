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

export default function App() {
  const { state, resetContext, updateContext, updateCategoryCtx } = useAppContext();
  const { app, setup } = state;
  const { isLoading, theme, isFetchingPack } = app;
  const activeStep = app?.activeStep ?? "setup";

  // Combine sync and async validation results
  const { errors: syncErrors, warnings: syncWarnings } = useValidation();
  const { errors: asyncErrors, warnings: asyncWarnings } = useLaunchValidation();
  const allErrors   = [...syncErrors,   ...asyncErrors];
  const allWarnings = [...syncWarnings, ...asyncWarnings];

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

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
            <GenerationTab />
          </div>
        )}
        {activeStep === "play"      && <PlayView />}
      </div>

      <footer>
        <ValidationStatus
          errors={allErrors}
          warnings={allWarnings}
          activeStep={activeStep}
          onNavigate={goToStep}
        />
        <div style={{ display: "flex", gap: "0.75rem" }}>
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
