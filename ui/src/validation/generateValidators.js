const WINDOWS_RESERVED = /^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$/i;
const INVALID_FILENAME_CHARS = /[<>:"/\\|?*\x00-\x1f]/;

// Characters confirmed to work properly in-game (no warning shown).
// This includes common keyboard characters not restricted by Windows:
// Space ! @ # $ % ^ & ( ) - = + [ ] { } ; ' , . ~ `
const VALID_PROFILE_CHARS = /^[a-zA-Z0-9_ !@#$%^&()\-=+\[\]{};',.~`]+$/;

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
      message: "Enter a name for the generated seed file."
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
        message: `"${name}" cannot be used as an instance name on Windows.`
      });
    }
  }

  // Profile name validation
  if (!profileName || !profileName.trim()) {
    errors.push({
      id: "gen_no_profile_name",
      scope: "generate",
      field: "profileName",
      message: "Enter a profile name."
    });
  } else if (profileName.length > 15) {
    errors.push({
      id: "gen_profile_name_too_long",
      scope: "generate",
      field: "profileName",
      message: "Profile name must be 15 characters or fewer."
    });
  } else if (INVALID_FILENAME_CHARS.test(profileName)) {
    errors.push({
      id: "gen_profile_invalid_chars",
      scope: "generate",
      field: "profileName",
      message: "Profile name cannot contain: < > : \" / \\ | ? *"
    });
  } else {
    if (profileName.startsWith(" ") || profileName.endsWith(" ")) {
      warnings.push({
        id: "gen_profile_spaces",
        scope: "generate",
        field: "profileName",
        message: "Leading or trailing spaces will be removed by the game."
      });
    }
    if (!VALID_PROFILE_CHARS.test(profileName)) {
      warnings.push({
        id: "gen_profile_name_chars",
        scope: "generate",
        field: "profileName",
        message: "Profile name contains unconfirmed characters (such as symbols, accents, or emojis) that may not display correctly in-game."
      });
    }
  }

  return { errors, warnings };
}