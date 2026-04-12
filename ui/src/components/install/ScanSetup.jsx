import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import { useAppContext } from "../../AppProvider"; // adjust path as needed

export default function ScanSetup() {
  const { state, updateCategoryCtx } = useAppContext();
  const { app, install } = state;
  const { isScanning } = app;
  const { installPath, scanResult } = install;

  async function handleScanSetup() {
    const selected = await open({
      multiple: false,
      directory: false,
      filters: [{ name: 'Executable', extensions: ['exe', ''] }]
    });

    if (selected) {
      const path = selected.path || selected;
      updateCategoryCtx("install", {
        installPath: path,
        scanResult: null
      });
      updateCategoryCtx("app", {
        isScanning: true
      });
      try {
        const result = await invoke("scan_install", {
          executablePath: path
        });
        updateCategoryCtx("install", {
          scanResult: result
        });
      }
      catch (err) {
        console.error("Error scanning:", err);
      }
      finally {
        updateCategoryCtx("app", {
          isScanning: false
        });
      }
    }
  }

  function handleRefresh() {
    if (installPath) {
      updateCategoryCtx("app", {
        isScanning: true
      });
      invoke("scan_install", { executablePath: installPath })
        .then((result) => updateCategoryCtx("install", { scanResult: result }))
        .catch((err) => console.error("Error refreshing scan:", err))
        .finally(() => updateCategoryCtx("app", { isScanning: false }));
    }
  }

  return (
    <div style={{ display: "flex", alignItems: "center", gap: "0.5rem", marginTop: "0.25rem" }}>
      {scanResult && (
        <span className="badge" style={{ padding: "0.3rem 0.5rem", fontSize: "0.75rem", backgroundColor: "var(--accent)" }}>
          [{scanResult.installType === "launcher" ? "Launcher Install" : "Classic Install"}]
        </span>
      )}
      <p style={{ margin: 0 }}>{installPath || "No installation selected. Browse to rvgl.exe to begin."}</p>
      
      <button className="primary" style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem" }} onClick={handleScanSetup}>
        {isScanning ? "Scanning..." : "Browse RVGL"}
      </button>

      {installPath && (
        <>
          <button onClick={handleRefresh} style={{ padding: "0.25rem 0.75rem", fontSize: "0.75rem" }}>
            ↺ Refresh
          </button>
        </>
      )}
    </div>
  );
}