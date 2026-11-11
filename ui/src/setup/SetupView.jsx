import { useAppContext } from "../AppProvider";
import CarsTab from "./CarsTab";
import TracksTab from "./TracksTab";
import InstallPanel from "./InstallPanel";
import { useInstallScan } from "./useInstallScan";
import "./SetupView.css";

const SETUP_TABS = [
  {key: "install", name: "Install"},
  {key: "cars", name: "Cars"},
  {key: "tracks", name: "Tracks"}
];

// ── SetupView root ────────────────────────────────────────────────────────────

export default function SetupView() {
  const { state, updateCategoryCtx } = useAppContext();
  const { setup } = state;
  const { installPath, scanResult } = setup;
  const hasValidInstall = Boolean(installPath && scanResult);
  let activeTab = setup.setupTab;
  if (!hasValidInstall || !activeTab) {
    activeTab = "install";
  }

  const scanInstall = useInstallScan();

  function setTab(tab) {
    updateCategoryCtx("setup", { setupTab: tab });
  }

  return (
    <div className="setup-view">
      <div className="setup-sub-tabs">
        {SETUP_TABS.map(tab => {
          const { key, name } = tab;
          const disabled = key !== "install" && !hasValidInstall;
          return (
            <button
              key={key}
              className={`setup-sub-tab ${activeTab === key ? "active" : ""} ${disabled ? "disabled" : ""}`}
              onClick={() => !disabled && setTab(key)}
              disabled={disabled}
            >
              {name}
            </button>
          );
        })}
      </div>

      {activeTab === "install" && <InstallPanel scanInstall={scanInstall} />}
      {activeTab === "cars"    && <div className="pool-panel"><CarsTab /></div>}
      {activeTab === "tracks"  && <div className="pool-panel"><TracksTab /></div>}
    </div>
  );
}
