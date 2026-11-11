import { useState, useEffect } from "react";
import { useAppContext } from "../AppProvider";
import { validateLaunch } from "./launchValidators";

export function useLaunchValidation() {
  const { state } = useAppContext();
  const { app, setup, generate, archipelago } = state;
  const mode = app.mode;
  const [asyncErrors, setAsyncErrors] = useState([]);
  const [asyncWarnings, setAsyncWarnings] = useState([]);
  const [validatedMode, setValidatedMode] = useState(null);

  useEffect(() => {
    let cancelled = false;
    if (!mode) {
      setAsyncErrors([]);
      setAsyncWarnings([]);
      setValidatedMode(null);
      return;
    }
    async function run() {
      const results = await validateLaunch(state);
      if (!cancelled) {
        setAsyncErrors(results.errors);
        setAsyncWarnings(results.warnings);
        setValidatedMode(mode);
      }
    }
    run();
    return () => { cancelled = true; };
  }, [mode, setup, generate.generatedFilePath, generate.seedContext, archipelago]);

  // Hide results from the previous mode until the current mode's checks finish.
  return validatedMode === mode
    ? { errors: asyncErrors, warnings: asyncWarnings }
    : { errors: [], warnings: [] };
}
