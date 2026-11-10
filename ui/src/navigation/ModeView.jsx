import { useState } from "react";
import { openUrl } from "@tauri-apps/plugin-opener";
import { useAppContext } from "../AppProvider";
import { MODES } from "./steps";
import "./ModeView.css";

export default function ModeView() {
  const { state, updateCategoryCtx } = useAppContext();
  const [linkError, setLinkError] = useState("");

  async function handleOpenProject(event) {
    event.preventDefault();
    setLinkError("");
    try {
      await openUrl(event.currentTarget.href);
    } catch (error) {
      console.error("Failed to open mode project:", error);
      setLinkError("Could not open the project link. Copy the link to open it in your browser.");
    }
  }

  return (
    <div className="mode-view">
      <h2>Choose how to play</h2>
      <div className="mode-cards" role="group" aria-label="Game mode">
        {MODES.map(mode => {
          return (
            <div key={mode.id} className={`mode-card ${state.app.mode === mode.id ? "selected" : ""}`}>
              <button
                className="mode-card-action"
                aria-pressed={state.app.mode === mode.id}
                onClick={() => updateCategoryCtx("app", { mode: mode.id, activeStep: "setup" })}
              >
                <span className="mode-card-title">{mode.label}</span>
                <span className="mode-description mode-summary">
                  {mode.desc}
                </span>
                <span className="mode-description mode-details">
                  {mode.details}
                </span>
              </button>
              <div className="mode-card-footer">
                {mode.project && (
                  <a
                    className="mode-project-link"
                    href={mode.project.url}
                    target="_blank"
                    rel="noopener noreferrer"
                    onClick={handleOpenProject}
                  >
                    {mode.project.label}
                  </a>
                )}
              </div>
            </div>
          );
        })}
      </div>
      {linkError && <p role="alert">{linkError}</p>}
    </div>
  );
}
