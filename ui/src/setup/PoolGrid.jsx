import CardImage from "./CardImage";
import { CAR_RATINGS, OBTAIN_METHODS, TRACK_DIFFICULTIES } from "../utils/constants";
import { getImageSrc } from "../utils/helpers";

export default function PoolGrid({ items, rootPath, activeTab, ratingFilter }) {
  if (!items || items.length === 0) {
    return <p style={{ opacity: 0.5, fontStyle: 'italic', fontSize: '0.9rem' }}>Empty or loading...</p>;
  }

  // Filter out invalid items and system cars
  const validItems = items.filter(item => {
    if (!item.hasValidFile) return false;
    if (activeTab === "cars" && item.isSystemCar) return false;
    if (activeTab === "cars" && ratingFilter !== "All" && item.rating.toString() !== ratingFilter) return false;
    if (activeTab === "tracks" && item.trackType !== 0) return false;
    return true;
  });

  if (validItems.length === 0) {
    return <p style={{ opacity: 0.5, fontStyle: 'italic', fontSize: '0.9rem' }}>No matching {activeTab}...</p>;
  }

  return (
    <div className="grid">
      {validItems.map(item => (
        <div key={item.folderName} className="card">
          <CardImage src={getImageSrc(item, rootPath, activeTab)} alt={item.name} />
          <div className="card-body">
            <h3 className="card-title" title={item.name}>{item.name}</h3>

            {activeTab === "cars" ? (
              <>
                <div className="card-subtitle">
                  <span className="badge">Rating: {CAR_RATINGS[item.rating] || "Unknown"}</span>
                </div>
                <div className="card-subtitle">
                  <span className="badge badge-obtain">Obtain: {OBTAIN_METHODS[item.obtainMethod] || "Unknown"}</span>
                </div>
              </>
            ) : (
              <>
                <div className="card-subtitle">
                  <span className="badge">Difficulty: {TRACK_DIFFICULTIES[item.difficulty] || "Unknown"}</span>
                </div>
                {item.hasReversed && (
                  <div className="card-subtitle">
                    <span className="badge badge-obtain">Has Reverse Version</span>
                  </div>
                )}
              </>
            )}
          </div>
        </div>
      ))}
    </div>
  );
}
