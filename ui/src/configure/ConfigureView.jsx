import { useAppContext } from "../AppProvider";

// Configure sub-views
import CarOptionsTab from "./carOptions/CarOptionsTab";
import StockCarsFullSpecTab from "./carSpec/StockCarsFullSpecTab";
import DcCarsFullSpecTab from "./carSpec/DcCarsFullSpecTab";
import TrackOptionsTab from "./trackOptions/TrackOptionsTab";
import TrackSpecTab from "./trackOptions/TrackSpecTab";
import CupSpecTab from "./cupSpec/CupSpecTab";

const CONFIGURE_TABS = [
  { id: "car-options",      label: "Car options",   group: "Cars" },
  { id: "stock-cars-spec",  label: "Stock spec",    group: "Cars" },
  { id: "dc-cars-spec",     label: "DC spec",       group: "Cars" },
  { id: "track-options",    label: "Track options", group: "Tracks" },
  { id: "track-spec",       label: "Track spec",    group: "Tracks" },
  { id: "cup-spec",         label: "Cup spec",      group: "Cups" },
];

export default function ConfigureView() {
  const { state, updateCategoryCtx } = useAppContext();
  const { configure } = state;
  const activeTab = configure?.configureTab ?? "car-options";

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
            {CONFIGURE_TABS.filter(t => t.group === group).map(tab => (
              <button
                key={tab.id}
                className={`configure-nav-item ${activeTab === tab.id ? "active" : ""}`}
                onClick={() => setTab(tab.id)}
              >
                {tab.label}
              </button>
            ))}
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
