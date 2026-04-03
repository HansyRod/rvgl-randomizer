import { convertFileSrc } from "@tauri-apps/api/core";
import { OVERRIDE_CARBOXES } from "./constants";

// Helper to reliably retrieve image via Tauri URL conversion protocol
export function getImageSrc(item, rootPath, activeTab) {
  const osDelim = rootPath.includes('\\') ? '\\' : '/';
  if (activeTab === "cars") {
      if (OVERRIDE_CARBOXES.includes(item.folderName.toLowerCase())) {
          return new URL(`../assets/carboxes/${item.folderName.toLowerCase()}.bmp`, import.meta.url).href;
      }
      
      if (item.carboxFilename) {
          if (item.carboxFilename.includes('\\') || item.carboxFilename.includes('/')) {
              // TCARBOX uses path relative to pack
              const safeRelativePath = item.carboxFilename.replace(/\\|\//g, osDelim);
              return convertFileSrc(`${rootPath}${osDelim}${safeRelativePath}`);
          } else {
              // Default carbox/box files are local to the car folder
              return convertFileSrc(`${rootPath}${osDelim}cars${osDelim}${item.folderName}${osDelim}${item.carboxFilename}`);
          }
      } else {
          return new URL(`../assets/carboxes/${item.folderName}.bmp`, import.meta.url).href; // Legacy fallback just in case
      }
  } else {
      return convertFileSrc(`${rootPath}${osDelim}gfx${osDelim}${item.folderName}.bmp`);
  }
}
