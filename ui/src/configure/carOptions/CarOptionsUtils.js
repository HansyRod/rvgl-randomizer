import { STOCK_CARS, DC_CARS } from "../../utils/constants";

export function makeDefaultSpec(ids) {
  return ids.map(id => ({
    id,
    sourcePool: "Full Random",
    sourceRating: "Random",
    sourceObtain: "Random",
    attrRating: "Random",
    attrObtain: "Random",
    customUnlock: null,
  }));
}

const RATING_IDS = ["0", "1", "2", "3", "4", "5"];

export function isNumericRating(v) {
  return RATING_IDS.includes(String(v));
}

export function getIncludedCarSlotCounts(specState) {
  if (!specState) {
    return { stock: 0, dc: 0, extra: 0, total: 0 };
  }

  const stock = specState.includeStockCars === false
    ? 0
    : (specState.stockCars?.length ?? STOCK_CARS.length);
  const dc = specState.includeDcCars === false
    ? 0
    : (specState.dcCars?.length ?? DC_CARS.length);
  const extra = specState.extraCars?.length ?? 0;

  return {
    stock,
    dc,
    extra,
    total: stock + dc + extra,
  };
}

export function getIncludedSlots(specState) {
  return getIncludedCarSlotCounts(specState).total;
}

export const normalizeExtraCarRowIds = (rows = []) =>
  rows.map((row, index) => ({
    ...row,
    id: `extra-${index + 1}`,
  }));

function isSpecificCarPool(sourcePool) {
  return typeof sourcePool === "string" &&
    sourcePool !== "Full Random" &&
    sourcePool !== "Stock" &&
    sourcePool !== "DC" &&
    sourcePool !== "Custom" &&
    !sourcePool.startsWith("Pack:");
}

function getFixedRating(row, key, availableCars) {
  const specificCar = isSpecificCarPool(row?.sourcePool)
    ? availableCars.find(car => car?.folderName?.toLowerCase() === row.sourcePool.toLowerCase())
    : null;

  if (key === "sourceRating" && specificCar) return specificCar.rating;
  if (isNumericRating(row?.[key])) return row[key];

  if (key === "attrRating" && row?.attrRating === "Unchanged") {
    return specificCar?.rating ?? row?.sourceRating;
  }

  return null;
}

export function countFixedRatings(specState, key, availableCars = []) {
  const out = { "0": 0, "1": 0, "2": 0, "3": 0, "4": 0, "5": 0 };
  if (!specState) return out;

  const applyRows = (rows = []) => {
    for (const row of rows) {
      const fixedRating = String(getFixedRating(row, key, availableCars));
      if (isNumericRating(fixedRating)) out[fixedRating] += 1;
    }
  };

  if (specState.includeStockCars !== false) applyRows(specState.stockCars);
  if (specState.includeDcCars !== false) applyRows(specState.dcCars);
  applyRows(specState.extraCars);
  return out;
}

export function normalizeDistributionMap(distMap, totalSlots) {
  const normalized = {};

  for (const rid of RATING_IDS) {
    const src = distMap?.[rid] ?? { enabled: false, min: 0, max: totalSlots };
    const min = Math.max(0, Number(src.min) || 0);
    let max = Math.max(0, Number(src.max) || 0);
    if (max < min) max = min;
    normalized[rid] = { enabled: !!src.enabled, min, max };
  }

  return normalized;
}

export const getRatingByMode = (modeId) => {
  // Switch statement for handling ratings
  switch (modeId) {
    case "random":
    case "randomRatings":
      return "Random";
    case "unchanged":
    case "randomUnlock":
      return "Unchanged";
    default:
      return "";
  }
}

export const getObtainByMode = (modeId) => {
  // Switch statement for handling obtain
  switch (modeId) {
    case "random":
    case "randomUnlock":
      return "Random";
    case "unchanged":
    case "randomRatings":
      return "Unchanged";
    default:
      return "";
  }
}

export const isRatingLockedByMode = (modeId) =>
  modeId === "unchanged" || modeId === "randomUnlock";

export const isObtainLockedByMode = (modeId) =>
  modeId === "unchanged" || modeId === "randomRatings";

export const applyStartingCarOverrides = (row, opts) => {
  const out = { ...row };

  if (getObtainByMode(opts.unlockMode) === "Random") {
    out.attrObtain = "0";
  } else {
    out.sourceObtain = "0";
  }

  if (opts.enableStartingCarsPool) {
    out.sourcePool = opts.startingCarsPool;
  }
  if (opts.enableStartingCarsRating) {
    out.sourceRating = opts.startingCarsRating;
  }

  return out;
};

export const resetStartingCarOverrides = (row, opts) => {
  const out = { ...row };

  out.attrObtain = getObtainByMode(opts.unlockMode) || "Random";
  out.attrRating = getRatingByMode(opts.unlockMode) || "Random";
  out.sourceObtain = "Random";
  if (opts.enableStartingCarsPool) {
    out.sourcePool = "Full Random";
  }
  if (opts.enableStartingCarsRating) {
    out.sourceRating = "Random";
  }

  return out;
};

export const applyModeRules = (car, index, modeId, carOpts) => {
  const out = { ...car };

  if (modeId === "baseGame") {
    return out; // Base Game Distribution is handled separately
  }
  
  const rating = getRatingByMode(modeId);
  if (rating) {
    out.attrRating = rating;
  }

  const obtain = getObtainByMode(modeId);
  if (obtain) {
    out.attrObtain = obtain;
  }

  // Starting-car overrides
  if (carOpts.enableStartingCars && index < carOpts.numStartingCars) {
    if (carOpts.enableStartingCarsPool) {
      out.sourcePool = carOpts.startingCarsPool;
    }
    if (carOpts.enableStartingCarsRating) {
      out.sourceRating = carOpts.startingCarsRating;
    }
    if (obtain === "Random") {
      if (isObtainLockedByMode(carOpts.unlockMode)) {
        out.sourceObtain = "Random";
      }
      out.attrObtain = "0"; // Force "Starting Car" instead of random attribute
    }
    else if (obtain === "Unchanged") {
      out.sourceObtain = "0"; // Force "Starting Car" in source pool
      out.attrObtain = "Unchanged";
    }
  }

  return out;
};

