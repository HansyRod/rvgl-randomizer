import { formatValidationList, getAllCarsFromScan, getAllTracksFromScan, getTrackSpecAvailableFolders } from "./validationUtils";
import { isEffectiveStockCarsMode, isEffectiveStockTracksMode } from "./stockMode";
import { getCustomUnlockTrackCountMax, hasEnabledCustomUnlockMethod, validateCustomUnlockRanges, validateCustomUnlockRows } from "./customUnlockValidators";
import { STOCK_CARS, DC_CARS, ATTR_RATINGS_LIST } from "../utils/constants";
import { countFixedRatings, getIncludedCarSlotCounts, getIncludedSlots } from "../configure/carOptions/CarOptionsUtils";

const RATING_IDS = ["0", "1", "2", "3", "4", "5"];
const GENERIC_CAR_POOLS = new Set(["Full Random", "Stock", "DC", "Custom"]);

function isSpecificCarPool(sourcePool) {
  return Boolean(
    sourcePool &&
    !GENERIC_CAR_POOLS.has(sourcePool) &&
    !sourcePool.startsWith("Pack:")
  );
}

function getPoolCandidates(sourcePool, allCars, scanResult) {
  if (sourcePool === "Stock") {
    return allCars.filter(car => car.pool === "stock");
  }
  if (sourcePool === "DC") {
    return allCars.filter(car => car.pool === "dc");
  }
  if (sourcePool === "Custom") {
    return allCars.filter(car => car.pool === "custom");
  }
  if (sourcePool?.startsWith("Pack:")) {
    const packName = sourcePool.slice("Pack:".length);
    const pack = (scanResult.contentPacks || []).find(p => p.name === packName);
    const packFolders = new Set(
      (pack?.cars || [])
        .map(car => car.folderName?.toLowerCase())
        .filter(Boolean)
    );
    return allCars.filter(car => packFolders.has(car.folderName?.toLowerCase()));
  }
  if (isSpecificCarPool(sourcePool)) {
    const folderName = sourcePool.toLowerCase();
    return allCars.filter(car => car.folderName?.toLowerCase() === folderName);
  }
  return allCars;
}

function getStartingRowCandidates(row, allCars, scanResult, modeLocksObtain) {
  const sourcePool = row.sourcePool || "Full Random";
  let candidates = getPoolCandidates(sourcePool, allCars, scanResult);
  const specificCar = isSpecificCarPool(sourcePool);

  if (modeLocksObtain) {
    // The UI locks source obtain to Starting Car in these modes. Treat a
    // stale row value as invalid rather than allowing generation to select a
    // non-starting obtain method.
    if (row.sourceObtain !== "0" || row.attrObtain !== "Unchanged") {
      return [];
    }

    candidates = candidates.filter(car => car.obtainMethod === 0);
  } else if (row.attrObtain !== "0") {
    // Random and Random Unlock modes make the target obtain value the
    // Starting Car value for starting slots.
    return [];
  }

  // Specific cars intentionally ignore source rating/obtain filters in the
  // generator, but they still need to be an actual Starting Car when the
  // target obtain is unchanged.
  if (specificCar) {
    return candidates;
  }

  if (row.sourceRating && row.sourceRating !== "Random") {
    const sourceRating = Number.parseInt(row.sourceRating, 10);
    if (Number.isFinite(sourceRating)) {
      candidates = candidates.filter(car => car.rating === sourceRating);
    }
  }

  if (!modeLocksObtain && row.sourceObtain && row.sourceObtain !== "Random") {
    const sourceObtain = Number.parseInt(row.sourceObtain, 10);
    if (Number.isFinite(sourceObtain)) {
      candidates = candidates.filter(car => car.obtainMethod === sourceObtain);
    }
  }

  return candidates;
}

export function validateCarOptions(carOptions, carsSpecState, scanResult, preset, trackSpecState) {
  const errors = [];
  const warnings = [];
  const infos = [];

  const includeStock = carsSpecState?.includeStockCars !== false;
  const includeDC = carsSpecState?.includeDcCars !== false;
  const extraRows = carsSpecState?.extraCars || [];

  const allCars = getAllCarsFromScan(scanResult);
  const allFolders = new Set(allCars.map(c => c.folderName.toLowerCase()));
  
  const isStockMode = isEffectiveStockCarsMode(scanResult, preset);
  if (isStockMode) {
    infos.push({
      id: "cars_stock_mode_active",
      scope: "carOptions",
      message: "Stock Content Mode is active for cars. Only the 28 base cars will be used."
    });
  }

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
  
  if (includeDC && !isStockMode) {
    checkRows(carsSpecState?.dcCars, "DC");
  }
  else if (!includeDC && !isStockMode) {
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

  const allTracks = getAllTracksFromScan(scanResult);
  const availableTrackFolders = getTrackSpecAvailableFolders(trackSpecState, allTracks);
  if (includeStock) {
    validateCustomUnlockRows(carsSpecState?.stockCars, errors, {
      scope: "carSpec",
      rowLabelPrefix: "Stock car",
      availableTrackFolders,
    });
  }
  if (includeDC && !isStockMode) {
    validateCustomUnlockRows(carsSpecState?.dcCars, errors, {
      scope: "carSpec",
      rowLabelPrefix: "DC car",
      availableTrackFolders,
    });
  }
  validateCustomUnlockRows(extraRows, errors, {
    scope: "carSpec",
    rowLabelPrefix: "Extra car",
    availableTrackFolders,
  });

  // No car randomization, don't run the remaining checks
  if (!includeStock && !includeDC && extraRows.length === 0) {
    return { errors, warnings, infos };
  }

  // Unlock method pool must have at least one method enabled when randomizing obtain values
  const showAllowedMethods =
    carOptions.unlockMode === "random" || carOptions.unlockMode === "randomUnlock";

  if (showAllowedMethods) {
    const anyMethodAllowed =
      carOptions.includeStartingCar  ||
      carOptions.includeChampionship  ||
      carOptions.includeTimeTrial     ||
      carOptions.includePracticeStars ||
      carOptions.includeSingleRace    ||
      carOptions.includeCheatOnly     ||
      carOptions.includeStuntArena    ||
      hasEnabledCustomUnlockMethod(carOptions);

    if (!anyMethodAllowed) {
      errors.push({
        id: "cars_no_unlock_methods",
        scope: "carOptions",
        message: "At least one unlock method must be enabled. Enable at least one method in \"Allowed Unlock Methods\".",
      });
    }

    validateCustomUnlockRanges(carOptions, errors, "carOptions", {
      trackCountMax: getCustomUnlockTrackCountMax(
        trackSpecState,
        allTracks,
        isEffectiveStockTracksMode(scanResult, preset)
      ),
    });
  }

  // Get list of specific cars from the configuration
  const specificCars = [
    ...(includeStock ? (carsSpecState?.stockCars || []) : STOCK_CARS.map((c) => { return { sourcePool: c } })),
    ...(includeDC && !isStockMode ? (carsSpecState?.dcCars || []) : (!isStockMode ? DC_CARS.map((c) => { return { sourcePool: c } }) : [])),
    ...extraRows,
  ].filter((row) => {
    return (
      row.sourcePool &&
      row.sourcePool !== "Full Random" &&
      row.sourcePool !== "Stock" &&
      row.sourcePool !== "DC" &&
      row.sourcePool !== "Custom" &&
      !row.sourcePool.startsWith("Pack:")
    );
  }).map(row => row.sourcePool);

  const duplicateCars = new Set();
  const seen = new Set();

  specificCars.forEach((car) => {
    const lower = car.toLowerCase();
    if (seen.has(lower)) {
      duplicateCars.add(car);
    }
    seen.add(lower);
  });

  if (duplicateCars.size > 0) {
    errors.push({
      id: "cars_duplicate_specific_refs",
      scope: "carOptions",
      message: `Some cars are present in multiple slots: ${formatValidationList(Array.from(duplicateCars))}.`
    });
  }

  // Check distribution constraints are satisfiable
  const totalSlots = getIncludedSlots({
    ...carsSpecState,
    includeDcCars: includeDC && !isStockMode,
  });

  const checkDistribution = (distMap, label, fixedCounts) => {
    const entries = RATING_IDS.map((rating) => ({
      rating,
      dist: distMap?.[rating],
      ratingLabel: ATTR_RATINGS_LIST.find((item) => item.val === rating)?.label || rating,
    }));
    const enabledEntries = entries.filter(({ dist }) => dist?.enabled);

    enabledEntries.forEach(({ rating, dist, ratingLabel }) => {
      const min = Number(dist.min);
      const max = Number(dist.max);
      const fixedCount = fixedCounts[rating] || 0;

      if (!Number.isFinite(min) || min < 0 || min > totalSlots) {
        errors.push({
          id: `cars_dist_min_range_${label}_${rating}`,
          scope: "carOptions",
          message: `${label}: ${ratingLabel} minimum must be between 0 and ${totalSlots}.`
        });
      }

      if (!Number.isFinite(max) || max < 0 || max > totalSlots) {
        errors.push({
          id: `cars_dist_max_range_${label}_${rating}`,
          scope: "carOptions",
          message: `${label}: ${ratingLabel} maximum must be between 0 and ${totalSlots}.`
        });
      }

      if (Number.isFinite(min) && Number.isFinite(max) && min > max) {
        errors.push({
          id: `cars_dist_min_above_max_${label}_${rating}`,
          scope: "carOptions",
          message: `${label}: ${ratingLabel} minimum cannot be greater than its maximum.`
        });
      }

      if (fixedCount > min) {
        warnings.push({
          id: `cars_dist_fixed_min_${label}_${rating}`,
          scope: "carOptions",
          message: `${label}: ${ratingLabel} has ${fixedCount} fixed slot${fixedCount === 1 ? "" : "s"}, but its minimum is only ${min}.`
        });
      }

      if (fixedCount > max) {
        errors.push({
          id: `cars_dist_fixed_max_${label}_${rating}`,
          scope: "carOptions",
          message: `${label}: ${ratingLabel} has ${fixedCount} fixed slot${fixedCount === 1 ? "" : "s"}, but its maximum is only ${max}.`
        });
      }
    });

    const minSum = enabledEntries.reduce((sum, { dist }) => {
      const min = Number(dist.min);
      return sum + (Number.isFinite(min) ? min : 0);
    }, 0);
    if (minSum > totalSlots) {
      errors.push({
        id: `cars_dist_min_sum_too_high_${label}`,
        scope: "carOptions",
        message: `${label} minimum values total ${minSum}, which exceeds the ${totalSlots} available slots.`
      });
    }

    // When every rating is constrained, the maxima must still be able to
    // accommodate every slot. Disabled ratings are intentionally unrestricted.
    const allRatingsEnabled = entries.every(({ dist }) => dist?.enabled);
    if (allRatingsEnabled) {
      const maxSum = entries.reduce((sum, { dist }) => {
        const max = Number(dist.max);
        return sum + (Number.isFinite(max) ? max : 0);
      }, 0);
      if (maxSum < totalSlots) {
        errors.push({
          id: `cars_dist_max_too_low_${label}`,
          scope: "carOptions",
          message: `${label} cannot cover all selected car slots with the current maximum values.`
        });
      }
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
        errors.push({
          id: `cars_dist_min_too_low_${label}`,
          scope: "carOptions",
          message: `There are not enough ${label} cars available to meet this minimum.`
        });
      }

    });
  }

  if (carOptions.poolRatingDistributions) {
    checkDistribution(
      carOptions.poolRatingDistributions,
      "Car Pool Rating Distribution",
      countFixedRatings(carsSpecState, "sourceRating", allCars)
    );
    checkSource(carOptions.poolRatingDistributions);
  }
  if (carOptions.attrRatingDistributions) {
    checkDistribution(
      carOptions.attrRatingDistributions,
      "Target Rating Distribution",
      countFixedRatings(carsSpecState, "attrRating", allCars)
    );
  }

  if (
    carOptions.enableStartingCars &&
    carOptions.unlockMode !== "baseGame" &&
    carOptions.numStartingCars > 0
  ) {
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
      const pool = carOptions.startingCarsPool || "Full Random";
      candidates = getPoolCandidates(pool, candidates, scanResult);
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

    // Validate each starting slot against the effective source and target
    // rules. The aggregate check above catches an undersized overall pool,
    // while this check catches rows whose specific source constraints cannot
    // produce a valid Starting Car (including stale mode-locked values).
    const slotCounts = getIncludedCarSlotCounts({
      ...carsSpecState,
      includeDcCars: includeDC && !isStockMode,
    });
    const startingRows = [];
    const appendStartingRows = (rows, count, label) => {
      for (let index = 0; index < count; index += 1) {
        startingRows.push({
          row: rows?.[index],
          label: `${label} slot ${index + 1}`,
        });
      }
    };

    appendStartingRows(carsSpecState?.stockCars, slotCounts.stock, "Stock");
    appendStartingRows(carsSpecState?.dcCars, slotCounts.dc, "DC");
    appendStartingRows(extraRows, slotCounts.extra, "Extra");

    const unavailableRows = startingRows
      .slice(0, carOptions.numStartingCars)
      .filter(({ row }) => {
        if (!row) {
          return true;
        }
        return getStartingRowCandidates(row, allCars, scanResult, modeLocksObtain).length === 0;
      })
      .map(({ row, label }) => `${label}${row?.id ? ` (${row.id})` : ""}`);

    if (unavailableRows.length > 0) {
      errors.push({
        id: "cars_starting_slot_unavailable",
        scope: "carOptions",
        message:
          `These Starting Car slots cannot produce a valid Starting Car: ` +
          `${formatValidationList(unavailableRows)}. Adjust their source pool, rating, or obtain settings.`
      });
    }
  }

  return { errors, warnings, infos };
}