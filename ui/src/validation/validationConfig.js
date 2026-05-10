export const SCOPE_TO_STEP = {
  scan: "setup",
  preset: "configure",
  carOptions: "configure",
  carSpec: "configure",
  trackSpec: "configure",
  cupSpec: "configure",
  generate: "generate",
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
  launch: "Launch",
};

export const STEP_LABELS = {
  setup: "Setup",
  configure: "Configure",
  generate: "Generate",
  play: "Play",
};

export const STEP_ORDER = ["setup", "configure", "generate", "play"];
