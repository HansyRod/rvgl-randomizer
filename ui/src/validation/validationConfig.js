import { STEP_DEFINITIONS } from "../navigation/steps";

export const SCOPE_TO_STEP = {
  scan: "setup",
  preset: "configure",
  carOptions: "configure",
  carSpec: "configure",
  trackSpec: "configure",
  cupSpec: "configure",
  generate: "generate",
  import: "import",
  launch: "play",
};

export const SCOPE_LABELS = {
  scan: "Installation",
  preset: "Presets",
  carOptions: "Car Options",
  carSpec: "Car Specification",
  trackSpec: "Track Specification",
  cupSpec: "Cup Specification",
  generate: "Generation Settings",
  import: "Archipelago File",
  launch: "Launch",
};

export const STEP_LABELS = Object.fromEntries(
  Object.values(STEP_DEFINITIONS).map(step => [step.id, step.label])
);
