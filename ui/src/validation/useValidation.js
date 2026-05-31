import { useMemo } from "react";
import { useAppContext } from "../AppProvider";
import { PRESETS } from "../configure/presets";
import { validateScan } from "./scanValidators";
import { validateSelectedPreset } from "./presetValidators";
import { validateCarOptions } from "./carValidators";
import { validateTrackSpec } from "./trackValidators";
import { validateCupSpec } from "./cupValidators";
import { validateGenerate } from "./generateValidators";

export function useValidation() {
  const { state } = useAppContext();
  const { setup, configure, generate, play } = state;
  const { preset } = configure || {};

  return useMemo(() => {
    const errors = [];
    const warnings = [];
    const infos = [];

    const collect = (results) => {
      if (results.errors) {
        errors.push(...results.errors);
      }
      if (results.warnings) {
        warnings.push(...results.warnings);
      }
      if (results.infos) {
        infos.push(...results.infos);
      }
    };

    if (setup.scanResult) {
      collect(validateScan(setup.scanResult, preset));
      collect(validateSelectedPreset(configure, setup.scanResult, PRESETS));
    }

    if (setup.scanResult && configure.carOptions) {
      collect(validateCarOptions(
        configure.carOptions,
        configure.carsSpecState,
        setup.scanResult,
        preset,
        configure.trackSpecState
      ));
    }

    if (setup.scanResult && configure.trackSpecState) {
      collect(validateTrackSpec(
        configure.trackSpecState,
        configure.trackOptions,
        setup.scanResult,
        preset
      ));
    }

    if (configure.cupSpecState && configure.trackSpecState) {
      collect(validateCupSpec(
        configure.cupSpecState,
        configure.trackSpecState,
        setup.scanResult
      ));
    }

    collect(validateGenerate(generate));

    return { errors, warnings, infos };
  }, [setup, configure, generate, play]);
}