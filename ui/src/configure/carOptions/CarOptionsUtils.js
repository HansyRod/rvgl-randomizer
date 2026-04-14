import { STOCK_CARS, DC_CARS } from "../../utils/constants";

export function makeDefaultSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceRating: "Random",
    sourceObtain: "Random",
    attrRating: "Random",
    attrObtain: "Random",
  }));
}

const RATING_IDS = ["0", "1", "2", "3", "4", "5"];

export function isNumericRating(v) {
  return RATING_IDS.includes(String(v));
}

export function getIncludedSlots(specState) {
  if (!specState) return 0;
  const stock = specState.includeStockCars === false ? 0 : (specState.stockCars?.length ?? STOCK_CARS.length);
  const dc = specState.includeDcCars === false ? 0 : (specState.dcCars?.length ?? DC_CARS.length);
  return stock + dc;
}

export function countFixedRatings(specState, key) {
  const out = { "0": 0, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0 };
  if (!specState) return out;

  const applyRows = (rows = []) => {
    for (const row of rows) {
      const val = row?.[key];
      if (isNumericRating(val)) out[String(val)] += 1;
    }
  };

  if (specState.includeStockCars !== false) applyRows(specState.stockCars);
  if (specState.includeDcCars !== false) applyRows(specState.dcCars);
  return out;
}

export function normalizeDistributionMap(distMap, fixedCounts, totalSlots) {
  const normalized = {};

  for (const rid of RATING_IDS) {
    const src = distMap?.[rid] ?? { enabled: false, min: 0, max: 42 };
    let min = Math.max(0, Number(src.min) || 0);
    min = Math.max(min, fixedCounts[rid] || 0);
    let max = Math.max(0, Number(src.max) || 0);
    if (max < min) max = min;
    normalized[rid] = { enabled: !!src.enabled, min, max };
  }

  // Keep sum(min) within available slots by reducing only above fixed floors.
  // Disabled rows should not consume budget unless the spec enforces fixed picks.
  const minConstrainedRatings = RATING_IDS.filter(
    rid => normalized[rid].enabled || (fixedCounts[rid] || 0) > 0
  );
  let minSum = minConstrainedRatings.reduce((s, rid) => s + normalized[rid].min, 0);
  if (minSum > totalSlots) {
    let overflow = minSum - totalSlots;
    const reducible = [...minConstrainedRatings].sort(
      (a, b) =>
        (normalized[b].min - (fixedCounts[b] || 0)) -
        (normalized[a].min - (fixedCounts[a] || 0))
    );
    for (const rid of reducible) {
      if (overflow <= 0) break;
      const floor = fixedCounts[rid] || 0;
      const canDrop = Math.max(0, normalized[rid].min - floor);
      const drop = Math.min(canDrop, overflow);
      normalized[rid].min -= drop;
      if (normalized[rid].max < normalized[rid].min) normalized[rid].max = normalized[rid].min;
      overflow -= drop;
    }
  }

  // If all ratings are enabled, max-sum must be able to cover all slots.
  const allEnabled = RATING_IDS.every(rid => normalized[rid].enabled);
  if (allEnabled) {
    let maxSum = RATING_IDS.reduce((s, rid) => s + normalized[rid].max, 0);
    if (maxSum < totalSlots) {
      let missing = totalSlots - maxSum;
      for (const rid of RATING_IDS) {
        if (missing <= 0) break;
        normalized[rid].max += missing;
        missing = 0;
      }
    }
  }

  return normalized;
}

export function resetFixedRatingsToRandom(specState, key, rid, keepCount) {
  if (!specState) return specState;

  const clone = {
    ...specState,
    stockCars: [...(specState.stockCars || [])],
    dcCars: [...(specState.dcCars || [])],
  };

  // Prefer preserving earlier slots; revert from the end.
  const buckets = [];
  if (clone.includeDcCars !== false) {
    for (let i = clone.dcCars.length - 1; i >= 0; i -= 1) {
      if (String(clone.dcCars[i]?.[key]) === rid) buckets.push({ cat: "dcCars", i });
    }
  }
  if (clone.includeStockCars !== false) {
    for (let i = clone.stockCars.length - 1; i >= 0; i -= 1) {
      if (String(clone.stockCars[i]?.[key]) === rid) buckets.push({ cat: "stockCars", i });
    }
  }

  let toReset = Math.max(0, buckets.length - keepCount);
  for (const b of buckets) {
    if (toReset <= 0) break;
    clone[b.cat][b.i] = { ...clone[b.cat][b.i], [key]: "Random" };
    toReset -= 1;
  }

  return clone;
}

export const applyModeRules = (car, index, modeId, carOpts) => {
  const out = { ...car };

  if (modeId === "baseGame") {
    return out; // Base Game Distribution is handled separately
  }
  
  // Switch statement for handling ratings
  switch (modeId) {
    case "random":
    case "randomRatings":
      out.attrRating = "Random";
      break;
    case "unchanged":
    case "randomUnlock":
      out.attrRating = "Unchanged";
      break;
    default:
      break;
  }

  // Switch statement for handling obtain
  switch (modeId) {
    case "random":
    case "randomUnlock":
      out.attrObtain = "Random";
      break;
    case "unchanged":
    case "randomRatings":
      out.attrObtain = "Unchanged";
      break;
    default:
      break;
  }

  // Starting-car overrides
  if (carOpts.enableStartingCars && index < carOpts.numStartingCars) {
    if (carOpts.enableStartingCarsPool) {
      out.sourcePool = carOpts.startingCarsPool;
    }
    if (carOpts.enableStartingCarsRating) {
      out.sourceRating = carOpts.startingCarsRating;
    }
    if (modeId === "random" || modeId === "randomUnlock") {
      out.attrObtain = "0"; // Force "Starting Car" instead of random attribute
    }
    else {
      out.sourceObtain = "0"; // Force "Starting Car" in source pool; it will be unchanged for these modes
    }
  }

  return out;
};

export const alignDistributionsWithSpec = (currentCarOptions, newSpecState) => {
  if (!newSpecState) return currentCarOptions;

  // 1. Calculate total slots and fixed counts from the NEW spec state
  const totalSlots = getIncludedSlots(newSpecState);
  const sourceFixed = countFixedRatings(newSpecState, "sourceRating");
  const attrFixed = countFixedRatings(newSpecState, "attrRating");

  // 2. Normalize both the pool and attr maps using those counts
  const nextPool = normalizeDistributionMap(currentCarOptions.poolRatingDistributions, sourceFixed, totalSlots);
  const nextAttr = normalizeDistributionMap(currentCarOptions.attrRatingDistributions, attrFixed, totalSlots);

  // 3. Return a new carOptions object with the aligned maps
  return {
    ...currentCarOptions,
    poolRatingDistributions: nextPool,
    attrRatingDistributions: nextAttr,
  };
}