export function formatInstallError(error, hasActiveInstall) {
  const message = String(error);

  if (/Unsupported RVGL executable/i.test(message)) {
    return hasActiveInstall
      ? "Selected install is not supported. The previously selected install remains active."
      : "Selected install is not supported.";
  }

  return `Could not validate the selected install: ${message}`;
}
