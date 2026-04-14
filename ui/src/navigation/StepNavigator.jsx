import { useAppContext } from "../AppProvider";
import "./StepNavigator.css";

const STEPS = [
  { id: "setup",     label: "Setup",     desc: "Installation & content" },
  { id: "configure", label: "Configure", desc: "Cars, tracks & cups" },
  { id: "generate",  label: "Generate",  desc: "Create your seed" },
  { id: "play",      label: "Play",      desc: "Launch the game" },
];

export default function StepNavigator({ activeStep, onStepClick }) {
  const { state } = useAppContext();
  const activeIdx = STEPS.findIndex(s => s.id === activeStep);

  const hasInstallPath = !!state.setup?.installPath;
  const hasGeneratedPath = !!state.generate?.generatedFilePath;

  return (
    <nav className="step-navigator" aria-label="Wizard steps">
      {STEPS.map((step, i) => {
        const isDone   = i < activeIdx;
        const isActive = i === activeIdx;

        let isDisabled = false;
        if (!hasInstallPath && step.id !== "setup") {
          isDisabled = true;
        }
        if (!hasGeneratedPath && step.id === "play") {
          isDisabled = true;
        }

        const cls = ["step-nav-item", isActive ? "active" : "", isDone ? "done" : ""].join(" ").trim();

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
              {isDone && <span className="step-nav-check" aria-hidden="true">✓</span>}
            </div>
            <div className="step-nav-desc">{step.desc}</div>
          </button>
        );
      })}
    </nav>
  );
}
