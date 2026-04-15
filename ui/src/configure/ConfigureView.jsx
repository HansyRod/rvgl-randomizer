import { useAppContext } from "../AppProvider";

// Configure sub-views
import CarOptionsTab from "./carOptions/CarOptionsTab";
import StockCarsFullSpecTab from "./carSpec/StockCarsFullSpecTab";
import DcCarsFullSpecTab from "./carSpec/DcCarsFullSpecTab";
import TrackOptionsTab from "./trackOptions/TrackOptionsTab";
import TrackSpecTab from "./trackOptions/TrackSpecTab";
import CupSpecTab from "./cupSpec/CupSpecTab";

const CONFIGURE_TABS = [
  { id: "car-options",      label: "Car options",            group: "Cars",   disabledKey: null },
  { id: "stock-cars-spec",  label: "Stock specification",    group: "Cars",   disabledKey: "includeStockCars" },
  { id: "dc-cars-spec",     label: "DC specification",       group: "Cars",   disabledKey: "includeDcCars" },
  { id: "track-options",    label: "Track options",          group: "Tracks", disabledKey: null },
  { id: "track-spec",       label: "Track specification",    group: "Tracks", disabledKey: null },
  { id: "cup-spec",         label: "Cup specification",      group: "Cups",   disabledKey: null },
];

export default function ConfigureView() {
  const { state, updateCategoryCtx } = useAppContext();
  const { configure } = state;
  const activeTab = configure?.configureTab ?? "car-options";
  const carsSpecState = configure?.carsSpecState;

  const includeStockCars = carsSpecState?.includeStockCars !== false;
  const includeDcCars    = carsSpecState?.includeDcCars    !== false;

  function setTab(id) {
    updateCategoryCtx("configure", { configureTab: id });
  }

  const groups = [...new Set(CONFIGURE_TABS.map(t => t.group))];

  return (
    <div style={{ display: "flex", flex: 1, overflow: "hidden" }}>
      {/* Configure sidebar nav */}
      <nav className="configure-sidenav">
        {groups.map(group => (
          <div key={group} className="configure-nav-group">
            <span className="configure-nav-group-label">{group}</span>
            {CONFIGURE_TABS.filter(t => t.group === group).map(tab => {
              const isDisabled = tab.disabledKey === "includeStockCars" ? !includeStockCars
                               : tab.disabledKey === "includeDcCars"    ? !includeDcCars
                               : false;
              return (
                <button
                  key={tab.id}
                  className={`configure-nav-item ${activeTab === tab.id ? "active" : ""} ${isDisabled ? "disabled" : ""}`}
                  onClick={() => !isDisabled && setTab(tab.id)}
                  disabled={isDisabled}
                  title={isDisabled ? "Disabled — toggle in Car Options" : undefined}
                >
                  {tab.label}
                </button>
              );
            })}
          </div>
        ))}
      </nav>

      {/* Configure pane */}
      <div style={{ flex: 1, overflowY: "auto" }}>
        {activeTab === "car-options"     && <CarOptionsTab />}
        {activeTab === "stock-cars-spec" && <div style={{ padding: "1rem" }}><StockCarsFullSpecTab /></div>}
        {activeTab === "dc-cars-spec"    && <div style={{ padding: "1rem" }}><DcCarsFullSpecTab /></div>}
        {activeTab === "track-options"   && <TrackOptionsTab />}
        {activeTab === "track-spec"      && <div style={{ padding: "1rem" }}><TrackSpecTab /></div>}
        {activeTab === "cup-spec"        && <div style={{ padding: "1rem" }}><CupSpecTab /></div>}
      </div>
    </div>
  );
}
