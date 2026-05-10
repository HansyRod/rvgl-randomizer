import { evaluateSelectedPreset } from "../configure/presets/presetValidation.js";

export function validateSelectedPreset(configure, scanResult, presets) {
  const selectedPreset = evaluateSelectedPreset(configure, scanResult, presets);
  if (!selectedPreset || selectedPreset.isSelectable) {
    return { errors: [], warnings: [], infos: [] };
  }

  return {
    errors: [
      {
        id: `preset_unmet_requirements_${selectedPreset.presetId}`,
        scope: "preset",
        message: `The selected preset has unmet requirements: ${selectedPreset.errors.join(" ")}`,
      },
    ],
    warnings: [],
    infos: [],
  };
}
