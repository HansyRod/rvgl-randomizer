import { formatValidationList, getAllCarsFromScan } from "./validationUtils";
import { STOCK_CARS, DC_CARS, ATTR_RATINGS_LIST } from "../utils/constants";

export function validateCarOptions(carOptions, carsSpecState, scanResult) {
  const errors = [];
  const warnings = [];

  const includeStock = carsSpecState?.includeStockCars !== false;
  const includeDC = carsSpecState?.includeDcCars !== false;

  const allCars = getAllCarsFromScan(scanResult);
  const allFolders = new Set(allCars.map(c => c.folderName.toLowerCase()));
  
  // Check stale specific-car references
  const staleRefs = [];

  // Check stale original-car references
  const staleStockRefs = [];
  const staleDCRefs = [];

  const checkRows = (rows, label) => {
    (rows || []).forEach((row, i) => {
      const isSpecific =
        row.sourcePool &&
        row.sourcePool !== "Full Random" &&
        row.sourcePool !== "Stock" &&
        row.sourcePool !== "DC" &&
        row.sourcePool !== "Custom" &&
        !row.sourcePool.startsWith("Pack:");

      if (isSpecific && !allFolders.has(row.sourcePool.toLowerCase())) {
        staleRefs.push(`${label} slot ${i + 1} (${row.sourcePool})`);
      }
    });
  };

  if (includeStock) {
    checkRows(carsSpecState?.stockCars, "Stock");
  }
  else {
    // When stock cars are not randomized, validate they exist in the source pool
    STOCK_CARS.forEach((car, i) => {
      if (!allFolders.has(car)) {
        staleStockRefs.push(`slot ${i + 1} (${car})`);
      }
    })

    if (staleStockRefs.length > 0) {
      warnings.push({
        id: "cars_stale_stock_refs",
        scope: "carOptions",
        message: `Some stock car slots are missing from the selected content: ${formatValidationList(staleStockRefs)}. Those cars will be unavailable.`
      });
    }
  }
  if (includeDC) {
    checkRows(carsSpecState?.dcCars, "DC");
  }
  else {
    // When DC cars are not randomized, validate they exist in the source pool
    DC_CARS.forEach((car, i) => {
      if (!allFolders.has(car)) {
        staleDCRefs.push(`slot ${i + 1} (${car})`);
      }
    })
    if (staleDCRefs.length > 0) {
      warnings.push({
        id: "cars_stale_dc_refs",
        scope: "carOptions",
        message: `Some DC car slots are missing from the selected content: ${formatValidationList(staleDCRefs)}. Those cars will be unavailable.`
      });
    }
  }

  if (staleRefs.length > 0) {
    warnings.push({
      id: "cars_stale_specific_refs",
      scope: "carOptions",
      message: `Some car slots point to cars that are no longer available: ${formatValidationList(staleRefs)}. Those slots will use random cars instead.`
    });
  }

  // No car randomization, don't run the remaining checks
  if (!includeStock && !includeDC) {
    return { errors, warnings };
  }

  // Check distribution constraints are satisfiable
  const totalSlots =
    (includeStock ? (carsSpecState?.stockCars?.length ?? STOCK_CARS.length) : 0) +
    (includeDC ? (carsSpecState?.dcCars?.length ?? DC_CARS.length) : 0);

  const checkDistribution = (distMap, label) => {
    const ratings = Object.entries(distMap);

    // Only validate maxSum for rating distributions if all ratings are enabled
    // Otherwise any remaining cars can use the unrestricted ratings
    if (ratings.some((r) => !r.enabled)) {
      return;
    }

    const maxSum = ratings.reduce((s, [, d]) => s + d.max, 0);
    if (maxSum < totalSlots) {
      warnings.push({
        id: `cars_dist_max_too_low_${label}`,
        scope: "carOptions",
        message: `${label} cannot cover all selected car slots with the current maximum values. Results may not fully match this distribution.`
      });
    }
  };

  const checkSource = (sourcePool) => {
    // Check there are enough source cars of each rating that's enabled
    Object.keys(sourcePool).forEach((rating) => {
      const ratingDist = sourcePool[rating];
      if (!ratingDist.enabled) {
        return;
      }

      const ratingInt = parseInt(rating);
      const ratingCars = allCars.filter((car) => car.rating === ratingInt);

      if (ratingCars.length < ratingDist.min) {
        const label = ATTR_RATINGS_LIST.find((item) => item.val === rating).label;
        warnings.push({
          id: `cars_dist_min_too_low_${label}`,
          scope: "carOptions",
          message: `There are not enough ${label} cars available to meet this minimum. Results may use fewer than requested.`
        });
      }

    });
  }

  if (carOptions.poolRatingDistributions) {
    checkDistribution(carOptions.poolRatingDistributions, "Car Pool Rating Distribution");
    checkSource(carOptions.poolRatingDistributions);
  }
  if (carOptions.attrRatingDistributions) {
    checkDistribution(carOptions.attrRatingDistributions, "Target Rating Distribution");
  }

  if (carOptions.enableStartingCars && carOptions.numStartingCars > 0) {
    
    let candidates = allCars;

    // If mode doesn't randomize obtain, the source pool must already have
    // obtain=0 cars (Starting Car) since the selection step won't pick others.
    const modeLocksObtain =
      carOptions.unlockMode === "unchanged" ||
      carOptions.unlockMode === "randomRatings";

    if (modeLocksObtain) {
      candidates = candidates.filter(c => c.obtainMethod === 0);
    }

    // Pool constraint
    if (carOptions.enableStartingCarsPool) {
      const pool = carOptions.startingCarsPool;
      if (pool === "Stock") {
        candidates = candidates.filter(c => c.pool === "stock");
      } else if (pool === "DC") {
        candidates = candidates.filter(c => c.pool === "dc");
      } else if (pool === "Custom") {
        candidates = candidates.filter(c => c.pool === "custom");
      } else if (pool.startsWith("Pack:")) {
        const packName = pool.slice("Pack:".length);
        const pack = (scanResult.contentPacks || []).find(p => p.name === packName);
        const packFolders = new Set((pack?.cars || []).map(c => c.folderName.toLowerCase()));
        candidates = candidates.filter(c => packFolders.has(c.folderName.toLowerCase()));
      }
      // "Full Random" — no pool filter
    }

    // Rating constraint
    if (carOptions.enableStartingCarsRating && carOptions.startingCarsRating !== "Random") {
      const targetRating = parseInt(carOptions.startingCarsRating, 10);
      if (!isNaN(targetRating)) {
        candidates = candidates.filter(c => c.rating === targetRating);
      }
    }

    if (candidates.length < carOptions.numStartingCars) {
      errors.push({
        id: "cars_starting_insufficient",
        scope: "carOptions",
        message:
          `The current starting car rules need ${carOptions.numStartingCars} cars, ` +
          `but only ${candidates.length} match. Reduce the count or broaden the filters.`
      });
    }
  }

  return { errors, warnings };
}