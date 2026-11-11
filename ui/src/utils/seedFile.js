import { open } from "@tauri-apps/plugin-dialog";

export function getFileName(path) {
  return path.split(/[\\/]/).pop() || path;
}

export async function selectSeedFile(defaultPath) {
  const selected = await open({
    multiple: false,
    defaultPath,
    filters: [{ name: "JSON Config", extensions: ["json"] }],
  });
  if (typeof selected === "string") {
    return selected;
  }
  if (selected && typeof selected === "object" && typeof selected.path === "string") {
    return selected.path;
  }
  return "";
}
