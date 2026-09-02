import { useAppContext } from "../../AppProvider";
import { countFixedRatings, getIncludedSlots, normalizeDistributionMap, resetFixedRatingsToRandom, isNumericRating } from "./CarOptionsUtils";
import RatingDistTable from "./RatingDistTable";

export default function CarRatingsConfig() {

  const { state, updateCategoryCtx } = useAppContext();

  // Destructure categories
  const { configure: { carOptions, carsSpecState } } = state;
  
  const { includeSuperPro, poolRatingDistributions, attrRatingDistributions } = carOptions;
  const totalSlots = getIncludedSlots(carsSpecState);

  const handleSuperProToggle = (val) => {
    const next = { ...carOptions, includeSuperPro: val };
    if (!val) {
      // Force Super Pro to 0 and enabled for both
      next.poolRatingDistributions = {
        ...poolRatingDistributions,
        "5": { enabled: true, min: 0, max: 0 }
      };
      next.attrRatingDistributions = {
        ...attrRatingDistributions,
        "5": { enabled: true, min: 0, max: 0 }
      };
    }

    updateCategoryCtx("configure", { carOptions: next });
  };
  
  const updateDist = (type, ratingId, field, value) => {
    const ratingTable = type === "pool" ? "poolRatingDistributions" : "attrRatingDistributions";
    const ratingSpecColumn = type === "pool" ? "sourceRating" : "attrRating";
    const ratingStr = String(ratingId);
    const normalizedValue = field === "enabled"
      ? !!value
      : Math.max(0, Number(value) || 0);

    const fixedCarCountByRating = countFixedRatings(carsSpecState, ratingSpecColumn);
    
    let nextSpec = carsSpecState;
    // If user lowers "min" below fixed amount from spec tabs, release extras back to Random.
    if (field === "min" && carsSpecState && isNumericRating(ratingStr)) {
      const fixedRid = fixedCarCountByRating[ratingStr] || 0;
      if (normalizedValue < fixedRid) {
        nextSpec = resetFixedRatingsToRandom(carsSpecState, ratingSpecColumn, ratingStr, normalizedValue);
      }
    }

    const current = carOptions[ratingTable]?.[ratingStr] ?? { enabled: false, min: 0, max: 42 };
    const nextEntry = { ...current, [field]: normalizedValue };

    // Symmetric behavior with min/max coupling:
    // - raising min above max lowers max (handled by normalizer)
    // - lowering max below min should lower min immediately.
    if (field === "max" && nextEntry.max < nextEntry.min) {
      nextEntry.min = nextEntry.max;
    }

    const rawMap = {
      ...carOptions[ratingTable],
      [ratingStr]: nextEntry
    };
    const normalized = normalizeDistributionMap(rawMap, fixedCarCountByRating, totalSlots);
    const nextCarOptions = { ...carOptions, [ratingTable]: normalized };

    updateCategoryCtx("configure", { carOptions: nextCarOptions, carsSpecState: nextSpec });
  };

  return (
    <section className="co-section">
      <h2 className="co-section-title">Rating Options</h2>
      <div className="co-checkbox-group" style={{ marginBottom: "1.5rem" }}>
        <label className="co-checkbox-row">
          <input type="checkbox" checked={includeSuperPro} onChange={e => handleSuperProToggle(e.target.checked)} />
          <span>Include <strong>Super Pro</strong> rating in randomization</span>
        </label>
      </div>

      <div className="co-dist-grid">
        <RatingDistTable 
          title="Car Pool Rating Distribution" 
          desc="Enforce how many cars are picked based on their original rating."
          data={poolRatingDistributions}
          onChange={(rid, f, v) => updateDist("pool", rid, f, v)}
          includeSuperPro={includeSuperPro}
          maxSlots={totalSlots}
        />
        <RatingDistTable 
          title="Target Rating Distribution" 
          desc="Enforce how many slots are assigned each final rating."
          data={attrRatingDistributions}
          onChange={(rid, f, v) => updateDist("attr", rid, f, v)}
          includeSuperPro={includeSuperPro}
          maxSlots={totalSlots}
        />
      </div>
    </section>
  );
}