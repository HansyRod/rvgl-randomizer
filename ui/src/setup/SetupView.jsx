import { useAppContext } from "../AppProvider";
import CarsTab from "./CarsTab";
import TracksTab from "./TracksTab";
import InstallPanel from "./InstallPanel";
import "./SetupView.css";

const SETUP_TABS = [
  {key: "install", name: "Install"},
  {key: "cars", name: "Cars"},
  {key: "tracks", name: "Tracks"}
];

// ── SetupView root ────────────────────────────────────────────────────────────

export default function SetupView() {
  const { state, updateCategoryCtx } = useAppContext();
  const { install } = state;
  const activeTab = install?.setupTab ?? "install";

  function setTab(tab) {
    updateCategoryCtx("install", { setupTab: tab });
  }

  const { install: { scanResult } } = state;
  const hasInstall = !!scanResult;

  return (
    <div className="setup-view">
      <div className="setup-sub-tabs">
        {SETUP_TABS.map(tab => {
          const { key, name } = tab;
          const disabled = key !== "install" && !hasInstall;
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

      {activeTab === "install" && <InstallPanel onContinue={() => { setTab("cars"); }} />}
      {activeTab === "cars"    && <div className="pool-panel"><CarsTab /></div>}
      {activeTab === "tracks"  && <div className="pool-panel"><TracksTab /></div>}
    </div>
  );
}
