import { useAppContext } from "../AppProvider";
import { ARCHIPELAGO_CONTENT_CATEGORIES } from "./archipelagoContent";

const CONTENT_LABELS = {
  stockCars: "Stock Cars",
  dcCars: "Dreamcast Cars",
  stockTracks: "Stock Tracks",
  dcTracks: "Dreamcast Track",
};

function ContentAvailability({ id, category }) {
  const expectedCount = ARCHIPELAGO_CONTENT_CATEGORIES[id].folders.length;
  const complete = category.status === "complete";
  const statusLabel = complete ? "Available" : category.status === "partial" ? "Partial" : "Missing";
  const sources = category.sources.filter(source => source.packName);

  return (
    <div className="ap-content-entry">
      <div className="ap-content-heading">
        <span>{CONTENT_LABELS[id]}</span>
        <span className={`pack-chip ${complete ? "chip-active" : "chip-inactive"}`}>
          <span aria-hidden="true">{complete ? "✓" : "✕"}</span> {statusLabel} · {category.items.length}/{expectedCount}
        </span>
      </div>
      {sources.length > 0 && (
        <div className="ap-content-sources">
          {sources.map(source => <span key={source.packName}>{source.packName}</span>)}
        </div>
      )}
      {category.missingFolders.length > 0 && (
        <details className="ap-missing-content">
          <summary>{category.missingFolders.length} missing</summary>
          <p>{category.missingFolders.join(", ")}</p>
        </details>
      )}
    </div>
  );
}

export default function ArchipelagoSetupContent() {
  const { state: { app, archipelago } } = useAppContext();
  const { scanResult, scanError } = archipelago.setup;
  const ready = scanResult && !app.isScanning;

  return (
    <div className="setup-section">
      {!app.isScanning && scanError && (
        <p className="install-error" role="alert">Could not scan content: {scanError} Use Refresh to try again.</p>
      )}
      {ready && (
        <div className="setup-columns pack-columns">
          {[
            { title: "Cars", ids: ["stockCars", "dcCars"] },
            { title: "Tracks", ids: ["stockTracks", "dcTracks"] },
          ].map(column => (
            <section key={column.title} className="setup-section pack-list-box">
              <h3 className="setup-section-title">{column.title}</h3>
              {column.ids.map(id => (
                <ContentAvailability key={id} id={id} category={scanResult.categories[id]} />
              ))}
            </section>
          ))}
        </div>
      )}
    </div>
  );
}
