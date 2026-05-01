import { useMemo } from "react";
import { useAppContext } from "../AppProvider";
import { validateScan } from "./scanValidators";
import { validateCarOptions } from "./carValidators";
import { validateTrackSpec } from "./trackValidators";
import { validateCupSpec } from "./cupValidators";
import { validateGenerate } from "./generateValidators";

export function useValidation() {
  const { state } = useAppContext();
  const { setup, configure, generate, play } = state;

  return useMemo(() => {
    const errors = [];
    const warnings = [];

    const collect = (results) => {
      errors.push(...results.errors);
      warnings.push(...results.warnings);
    };

    if (setup.scanResult) {
      collect(validateScan(setup.scanResult));
    }

    if (setup.scanResult && configure.carOptions) {
      collect(validateCarOptions(
        configure.carOptions,
        configure.carsSpecState,
        setup.scanResult
      ));
    }

    if (setup.scanResult && configure.trackSpecState) {
      collect(validateTrackSpec(
        configure.trackSpecState,
        configure.trackOptions,
        setup.scanResult
      ));
    }

    if (configure.cupSpecState && configure.trackSpecState) {
      collect(validateCupSpec(
        configure.cupSpecState,
        configure.trackSpecState
      ));
    }

    collect(validateGenerate(generate));

    return { errors, warnings };
  }, [setup, configure, generate, play]);
}