import { useState, useEffect, useRef } from "react";
import ValidationPanel from "./ValidationPanel";
import "./ValidationStatus.css";

export default function ValidationStatus({ errors = [], warnings = [], infos = [], activeStep, onNavigate }) {
  const [open, setOpen] = useState(false);
  const panelRef = useRef(null);
  const btnRef = useRef(null);

  // Close on outside click
  useEffect(() => {
    if (!open) return;
    const handler = (e) => {
      if (
        !panelRef.current?.contains(e.target) &&
        !btnRef.current?.contains(e.target)
      ) {
        setOpen(false);
      }
    };
    document.addEventListener("mousedown", handler);
    return () => document.removeEventListener("mousedown", handler);
  }, [open]);

  // Close on Escape
  useEffect(() => {
    if (!open) return;
    const handler = (e) => { if (e.key === "Escape") setOpen(false); };
    document.addEventListener("keydown", handler);
    return () => document.removeEventListener("keydown", handler);
  }, [open]);

  if (errors.length === 0 && warnings.length === 0 && infos.length === 0) return null;

  const hasErrors = errors.length > 0;

  return (
    <div className="vs-root">
      {open && (
        <ValidationPanel
          errors={errors}
          warnings={warnings}
          infos={infos}
          activeStep={activeStep}
          onNavigate={(stepId) => { onNavigate(stepId); setOpen(false); }}
          onClose={() => setOpen(false)}
          panelRef={panelRef}
        />
      )}
      <button
        ref={btnRef}
        className={`vs-btn ${hasErrors ? "vs-has-errors" : (warnings.length > 0 ? "vs-has-warnings" : "")}`}
        onClick={() => setOpen(o => !o)}
        aria-expanded={open}
        aria-label={`${errors.length} errors, ${warnings.length} warnings, ${infos.length} infos. Click to ${open ? "hide" : "view"}.`}
      >
        {errors.length > 0 && (
          <span className="vs-pill vs-error-pill">
            <span className="vs-dot" />
            {errors.length} error{errors.length !== 1 ? "s" : ""}
          </span>
        )}
        {errors.length > 0 && warnings.length > 0 && (
          <span className="vs-separator">·</span>
        )}
        {warnings.length > 0 && (
          <span className="vs-pill vs-warning-pill">
            <span className="vs-dot" />
            {warnings.length} warning{warnings.length !== 1 ? "s" : ""}
          </span>
        )}
        {(errors.length > 0 || warnings.length > 0) && infos.length > 0 && (
          <span className="vs-separator">·</span>
        )}
        {infos.length > 0 && (
          <span className="vs-pill vs-info-pill">
            <span className="vs-dot" />
            {infos.length} info{infos.length !== 1 ? "s" : ""}
          </span>
        )}
        <span className="vs-chevron" aria-hidden="true">
          {open ? "▾" : "▴"}
        </span>
      </button>
    </div>
  );
}