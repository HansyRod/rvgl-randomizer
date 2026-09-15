import fs from "node:fs";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { transformWithEsbuild } from "vite";

const EXTENSIONS = [".js", ".jsx", ".mjs", ".json"];

function resolveFilePath(specifier, parentURL) {
  if (!specifier.startsWith(".") && !specifier.startsWith("/")) {
    return null;
  }

  const parentPath = parentURL ? fileURLToPath(parentURL) : process.cwd();
  const basePath = specifier.startsWith("/")
    ? path.resolve(specifier)
    : path.resolve(path.dirname(parentPath), specifier);

  if (fs.existsSync(basePath) && fs.statSync(basePath).isFile()) {
    return basePath;
  }

  for (const extension of EXTENSIONS) {
    const candidate = `${basePath}${extension}`;
    if (fs.existsSync(candidate) && fs.statSync(candidate).isFile()) {
      return candidate;
    }
  }

  for (const extension of EXTENSIONS) {
    const candidate = path.join(basePath, `index${extension}`);
    if (fs.existsSync(candidate) && fs.statSync(candidate).isFile()) {
      return candidate;
    }
  }

  return null;
}

export async function resolve(specifier, context, defaultResolve) {
  try {
    return await defaultResolve(specifier, context, defaultResolve);
  } catch (error) {
    const filePath = resolveFilePath(specifier, context.parentURL);
    if (!filePath) {
      throw error;
    }

    return {
      url: pathToFileURL(filePath).href,
      shortCircuit: true,
    };
  }
}

export async function load(url, context, defaultLoad) {
  if (url.endsWith(".css")) {
    return {
      format: "module",
      source: "export default {};",
      shortCircuit: true,
    };
  }

  if (url.endsWith(".jsx")) {
    const filePath = fileURLToPath(url);
    const source = fs.readFileSync(filePath, "utf8");
    const transformed = await transformWithEsbuild(source, filePath, {
      loader: "jsx",
      format: "esm",
      target: "es2022",
    });

    return {
      format: "module",
      source: transformed.code,
      shortCircuit: true,
    };
  }

  return defaultLoad(url, context, defaultLoad);
}
