import { invoke } from "@tauri-apps/api/core";
import { save } from "@tauri-apps/plugin-dialog";
import { getArchipelagoMetadata } from "./archipelagoSeed";

export async function loadArchipelagoFile(file) {
  let configPath = file;
  if (file.toLowerCase().endsWith(".aprvgl")) {
    try {
      configPath = await invoke("extract_archipelago_file", { filePath: file });
    } catch (error) {
      if (!error?.fileName) {
        throw error;
      }
      const outputPath = await save({
        title: "Save Archipelago JSON in a writable folder",
        defaultPath: error.fileName,
        filters: [{
          name: "RVGL JSON Config",
          extensions: ["rvgl.json"]
        }],
      });
      if (!outputPath) {
        return null;
      }
      configPath = await invoke("extract_archipelago_file", { filePath: file, outputPath });
    }
  }
  const config = await invoke("read_config_file", { filePath: configPath });
  return {
    configPath,
    metadata: getArchipelagoMetadata(config)
  };
}
