import { copyFileSync, existsSync, mkdirSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const configuration = process.argv[2] ?? "Debug";
const isWindows = process.platform === "win32";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const uiRoot = resolve(scriptDir, "..");
const repoRoot = resolve(uiRoot, "..");
const buildDir = resolve(repoRoot, "build");
const resourcesDir = resolve(uiRoot, "src-tauri", "resources");

function run(command, args) {
  const result = spawnSync(command, args, {
    cwd: uiRoot,
    stdio: "inherit",
    shell: false,
  });

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

const configureArgs = ["-S", repoRoot, "-B", buildDir];
if (isWindows) {
  configureArgs.push("-A", "x64");
} else {
  configureArgs.push(`-DCMAKE_BUILD_TYPE=${configuration}`);
}

run("cmake", configureArgs);
run("cmake", ["--build", buildDir, "--config", configuration, "--target", "randomizer"]);

const libraryName = isWindows ? "randomizer.dll" : "randomizer.so";
const builtLibrary = isWindows
  ? resolve(buildDir, "DLL", configuration, libraryName)
  : resolve(buildDir, "DLL", libraryName);
const resourceLibrary = resolve(resourcesDir, libraryName);

if (!existsSync(builtLibrary)) {
  console.error(`Built library not found: ${builtLibrary}`);
  process.exit(1);
}

mkdirSync(dirname(resourceLibrary), { recursive: true });
copyFileSync(builtLibrary, resourceLibrary);
console.log(`Copied ${builtLibrary} -> ${resourceLibrary}`);
