export const MODES = [
  {
    id: "randomizer",
    label: "Randomizer",
    desc: "Create or load a standalone randomized game.",
    details: "Configure cars, tracks, and cups in this app, then generate your game.",
  },
  {
    id: "archipelago",
    label: "Archipelago",
    desc: "Play RVGL as part of an Archipelago multiworld.",
    details: "Import the .rvgl.json generated for your player slot. Connect to the server through RVGL's in-game menu.",
    project: {
      url: "https://github.com/HansyRod/Archipelago-RVGL",
      label: "Archipelago-RVGL project on GitHub ↗",
    },
  },
];

export const STEP_DEFINITIONS = {
  mode: { id: "mode", label: "Mode", desc: "Choose how to play" },
  setup: { id: "setup", label: "Setup", desc: "Installation & content" },
  configure: { id: "configure", label: "Configure", desc: "Cars, tracks & cups" },
  generate: { id: "generate", label: "Generate", desc: "Create your seed" },
  import: { id: "import", label: "Import", desc: "Load your Archipelago file" },
  play: { id: "play", label: "Play", desc: "Launch the game" },
};

export function isValidMode(mode) {
  return MODES.some(option => option.id === mode);
}

export function getSteps(mode) {
  const ids = ["mode", "setup"];
  if (mode === "randomizer") {
    ids.push("configure", "generate", "play");
  } else if (mode === "archipelago") {
    ids.push("import", "play");
  }
  return ids.map(id => STEP_DEFINITIONS[id]);
}

export function canNavigateToStep(state, stepId) {
  const mode = state.app?.mode;
  if (!getSteps(mode).some(step => step.id === stepId)) {
    return false;
  }
  if (stepId === "mode") {
    return true;
  }
  if (!isValidMode(mode)) {
    return false;
  }
  if (stepId === "setup") {
    return true;
  }
  if (!state.setup?.installPath || !state.setup?.scanResult) {
    return false;
  }
  if (stepId === "play") {
    // AP Play stays unavailable until its import and launch stages are implemented.
    return mode === "randomizer" && Boolean(state.generate?.generatedFilePath);
  }
  return true;
}

export function getAccessibleStep(state, stepId) {
  if (canNavigateToStep(state, stepId)) {
    return stepId;
  }
  if (!isValidMode(state.app?.mode)) {
    return "mode";
  }
  if (!state.setup?.installPath || !state.setup?.scanResult) {
    return "setup";
  }
  if (stepId === "play") {
    return state.app.mode === "archipelago" ? "import" : "generate";
  }
  return "setup";
}
