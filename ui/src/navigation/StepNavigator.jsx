import { useAppContext } from "../AppProvider";
import "./StepNavigator.css";
import { SCOPE_TO_STEP } from "../validation/validationConfig";

const STEPS = [
  { id: "setup",     label: "Setup",     desc: "Installation & content" },
  { id: "configure", label: "Configure", desc: "Cars, tracks & cups" },
  { id: "generate",  label: "Generate",  desc: "Create your seed" },
  { id: "play",      label: "Play",      desc: "Launch the game" },
];

export default function StepNavigator({ activeStep, onStepClick, errors = [], warnings = [] }) {
  const { state } = useAppContext();
  const activeIdx = STEPS.findIndex(s => s.id === activeStep);

  const hasValidInstall = Boolean(
    state.setup?.installPath && state.setup?.scanResult
  );
  const hasGeneratedPath = !!state.generate?.generatedFilePath;

  // Compute per-step issue counts
  const stepIssues = {};
  for (const step of STEPS) {
    stepIssues[step.id] = {
      errorCount:   errors.filter(e => SCOPE_TO_STEP[e.scope] === step.id).length,
      warningCount: warnings.filter(w => SCOPE_TO_STEP[w.scope] === step.id).length,
    };
  }

  return (
    <nav className="step-navigator" aria-label="Wizard steps">
      {STEPS.map((step, i) => {
        const isDone   = i < activeIdx;
        const isActive = i === activeIdx;
        const { errorCount, warningCount } = stepIssues[step.id];

        let isDisabled = false;
        if (!hasValidInstall && step.id !== "setup") {
          isDisabled = true;
        }
        if (!hasGeneratedPath && step.id === "play") {
          isDisabled = true;
        }

        const cls = [
          "step-nav-item",
          isActive ? "active" : "",
          isDone ? "done" : "",
          errorCount > 0 ? "has-errors" : warningCount > 0 ? "has-warnings" : ""
        ].filter(Boolean).join(" ").trim();

        // Badge shown when:
        // - done step with issues (replaces ✓)
        // - active or future step with issues
        const showErrorBadge   = errorCount > 0;
        const showWarningBadge = !showErrorBadge && warningCount > 0;
        const showCheck        = isDone && !showErrorBadge && !showWarningBadge;

        return (
          <button
            key={step.id}
            className={cls}
            onClick={() => onStepClick(step.id)}
            aria-current={isActive ? "step" : undefined}
            disabled={isDisabled}
          >
            <div className="step-nav-header">
              <span className="step-nav-num">{i + 1}</span>
              <span className="step-nav-label">{step.label}</span>
              {showErrorBadge && (
                <span className="step-nav-badge step-nav-badge-error"
                  aria-label={`${errorCount} error${errorCount !== 1 ? "s" : ""}`}>
                  !
                </span>
              )}
              {showWarningBadge && (
                <span className="step-nav-badge step-nav-badge-warning"
                  aria-label={`${warningCount} warning${warningCount !== 1 ? "s" : ""}`}>
                  !
                </span>
              )}
              {showCheck && (
                <span className="step-nav-check" aria-hidden="true">✓</span>
              )}
            </div>
            <div className="step-nav-desc">{step.desc}</div>
          </button>
        );
      })}
    </nav>
  );
}
