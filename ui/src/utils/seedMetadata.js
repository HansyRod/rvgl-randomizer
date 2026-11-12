export function getGeneratedAt(metadata) {
  return metadata?.generatedAt ?? metadata?.uiContext?.generatedAt;
}

export function formatSeedDate(isoString, includeTime = true) {
  const date = isoString ? new Date(isoString) : null;
  if (!date || Number.isNaN(date.getTime())) return "Unknown date";
  return date.toLocaleDateString(undefined, {
    day: "numeric", month: "short", year: "numeric",
    ...(includeTime ? { hour: "2-digit", minute: "2-digit" } : {}),
  });
}
