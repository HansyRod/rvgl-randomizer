const WINDOWS_RESERVED = /^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$/i;
const INVALID_FILENAME_CHARS = /[<>:"/\\|?*\x00-\x1f]/;
const VALID_PROFILE_CHARS = /^[a-zA-Z0-9_]+$/;

export function validateGenerate(generate, configure) {
  const errors = [];
  const warnings = [];

  const { instanceName, profileName } = generate;

  // Instance name validation
  if (!instanceName || !instanceName.trim()) {
    errors.push({
      id: "gen_no_instance_name",
      scope: "generate",
      field: "instanceName",
      message: "Instance name is required."
    });
  } else {
    const name = instanceName.trim().replace(/\.json$/i, "");
    if (INVALID_FILENAME_CHARS.test(name)) {
      errors.push({
        id: "gen_invalid_instance_name",
        scope: "generate",
        field: "instanceName",
        message: "Instance name contains characters that are not allowed in file names."
      });
    }
    if (WINDOWS_RESERVED.test(name)) {
      errors.push({
        id: "gen_reserved_instance_name",
        scope: "generate",
        field: "instanceName",
        message: `"${name}" is a reserved filename on Windows and cannot be used.`
      });
    }
  }

  // Profile name validation
  if (!profileName || !profileName.trim()) {
    errors.push({
      id: "gen_no_profile_name",
      scope: "generate",
      field: "profileName",
      message: "Profile name is required."
    });
  } else if (profileName.length > 15) {
    errors.push({
      id: "gen_profile_name_too_long",
      scope: "generate",
      field: "profileName",
      message: "Profile name must be 15 characters or fewer."
    });
  } else if (!VALID_PROFILE_CHARS.test(profileName)) {
    warnings.push({
      id: "gen_profile_name_chars",
      scope: "generate",
      field: "profileName",
      message: "Profile name contains characters other than letters, numbers, and underscores. This may cause issues with RVGL's profile system."
    });
  }

  return { errors, warnings };
}