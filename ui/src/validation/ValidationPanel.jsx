import { useState } from "react";
import { SCOPE_TO_STEP, SCOPE_LABELS, STEP_LABELS, STEP_ORDER } from "./validationConfig";

export default function ValidationPanel({ errors = [], warnings = [], infos = [], activeStep, onNavigate, onClose, panelRef }) {
  const [filter, setFilter] = useState("current");

  const allIssues = [
    ...errors.map(e => ({ ...e, issueType: "error" })),
    ...warnings.map(w => ({ ...w, issueType: "warning" })),
    ...infos.map(i => ({ ...i, issueType: "info" })),
  ];

  const currentStepIssues = allIssues.filter(
    i => SCOPE_TO_STEP[i.scope] === activeStep
  );

  const visibleIssues = filter === "current" ? currentStepIssues : allIssues;

  // Group: step → scope → issues[]
  const grouped = {};
  for (const issue of visibleIssues) {
    const step = SCOPE_TO_STEP[issue.scope] || "unknown";
    if (!grouped[step]) grouped[step] = {};
    if (!grouped[step][issue.scope]) grouped[step][issue.scope] = [];
    grouped[step][issue.scope].push(issue);
  }

  return (
    <div className="vp-panel" ref={panelRef}>
      <div className="vp-header">
        <span className="vp-title">Validation Issues</span>
        <button className="vp-close" onClick={onClose} aria-label="Close panel">✕</button>
      </div>

      <div className="vp-tabs">
        <button
          className={`vp-tab ${filter === "current" ? "active" : ""}`}
          onClick={() => setFilter("current")}
        >
          This Step
          {currentStepIssues.length > 0 && (
            <span className="vp-tab-badge">{currentStepIssues.length}</span>
          )}
        </button>
        <button
          className={`vp-tab ${filter === "all" ? "active" : ""}`}
          onClick={() => setFilter("all")}
        >
          All Steps
          <span className="vp-tab-badge">{allIssues.length}</span>
        </button>
      </div>

      <div className="vp-body">
        {visibleIssues.length === 0 ? (
          <p className="vp-empty">
            {filter === "current"
              ? "No issues on this step."
              : "No issues found."}
          </p>
        ) : (
          STEP_ORDER.filter(step => grouped[step]).map(step => (
            <div key={step} className="vp-step-group">
              {filter === "all" && (
                <button
                  className="vp-step-heading"
                  onClick={() => onNavigate(step)}
                >
                  {STEP_LABELS[step]}
                  <span className="vp-step-arrow">Go to step →</span>
                </button>
              )}
              {Object.entries(grouped[step]).map(([scope, issues]) => (
                <div key={scope} className="vp-scope-group">
                  <span className="vp-scope-label">{SCOPE_LABELS[scope] || scope}</span>
                  {issues.map(issue => (
                    <div key={issue.id} className={`vp-issue vp-issue-${issue.issueType}`}>
                      <span className="vp-issue-dot" />
                      <span className="vp-issue-msg">{issue.message}</span>
                    </div>
                  ))}
                </div>
              ))}
            </div>
          ))
        )}
      </div>
    </div>
  );
}