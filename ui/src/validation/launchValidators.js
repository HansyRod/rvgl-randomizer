export function validateLaunch(generate, setup) {
  const errors = [];
  const warnings = [];

  // No generated file
  if (!generate.generatedFilePath) {
    errors.push({
      id: "launch_no_config",
      scope: "launch",
      message: "No generated seed is loaded. Generate a new seed or load an existing one before launching."
    });
  }

  // DLL check is done at runtime in Rust, but we can surface a hint here
  // if we know the install path but no dll path is resolvable — left to
  // the Rust layer since we can't check the resource path from JS easily.

  return { errors, warnings };
}