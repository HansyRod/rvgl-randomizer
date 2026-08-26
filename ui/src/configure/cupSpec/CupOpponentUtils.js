import { DC_CARS, STOCK_CARS } from "../../utils/constants";
import { getPlayableCarsFromScan } from "../../utils/scanContent";
import { isEffectiveStockCarsMode } from "../../validation/stockMode";
import { RATING_LABELS } from "./CupUtils";

export const CUP_OPPONENT_CATEGORIES = [
  {
    key: "stock",
    label: "Stock",
    specKey: "stockCars",
    includeKey: "includeStockCars",
    defaultFolders: STOCK_CARS,
  },
  {
    key: "dc",
    label: "DC",
    specKey: "dcCars",
    includeKey: "includeDcCars",
    defaultFolders: DC_CARS,
  },
];

const RATING_COUNT = RATING_LABELS.length;
const GENERIC_POOLS = new Set(["Full Random", "Stock", "DC", "Custom"]);

export function getCupOpponentReferenceKey(reference) {
  if (reference?.type === "car" && reference.folder) {
    return `car:${reference.folder.toLowerCase()}`;
  }
  if (reference?.type === "slot" && reference.category && Number.isInteger(reference.index)) {
    return `slot:${reference.category}:${reference.index}`;
  }
  return "";
}

function isSpecificCarPool(sourcePool) {
  return Boolean(sourcePool) &&
    !GENERIC_POOLS.has(sourcePool) &&
    !sourcePool.startsWith("Pack:");
}

function getCarByFolder(cars, folder) {
  const normalizedFolder = String(folder || "").toLowerCase();
  return cars.find(car => car.folderName?.toLowerCase() === normalizedFolder) ?? null;
}

function getFixedRating(row, car) {
  const attrRating = String(row?.attrRating ?? "");
  if (/^[0-5]$/.test(attrRating)) return Number(attrRating);

  // "Unchanged" means the final attribute rating matches the source rating.
  if (attrRating === "Unchanged") {
    const sourceRating = String(row?.sourceRating ?? "");
    if (/^[0-5]$/.test(sourceRating)) return Number(sourceRating);
    if (Number.isInteger(car?.rating)) return car.rating;
  }

  return null;
}

function makeCandidate({ category, index, car, rating }) {
  const isCarReference = Boolean(car);
  return {
    rating,
    label: isCarReference
      ? (car.name || car.folderName)
      : `${category.label} Slot ${index + 1}`,
    folder: car?.folderName ?? null,
    reference: isCarReference
      ? {
          type: "car",
          folder: car.folderName,
          name: car.name || car.folderName,
        }
      : { type: "slot", category: category.key, index },
  };
}

/**
 * Build the fixed opponent choices that can be offered by the cup editor.
 *
 * Each returned group contains only candidates whose final rating is known.
 * Generic slots with a random/unknown attrRating are intentionally omitted:
 * their eventual class cannot be validated before generation.
 */
export function getCupOpponentCandidates({
  scanResult,
  carsSpecState,
  preset,
} = {}) {
  const availableCars = getPlayableCarsFromScan(scanResult);
  const candidatesByRating = Array.from({ length: RATING_COUNT }, () => []);
  const seenCarReferences = new Set();

  const addCandidate = (candidate) => {
    if (!candidate || !Number.isInteger(candidate.rating) ||
        candidate.rating < 0 || candidate.rating >= RATING_COUNT) {
      return;
    }

    // A concrete car is the canonical choice when it appears in more than
    // one source slot. This also prevents duplicate car options in a class.
    if (candidate.reference.type === "car") {
      const key = candidate.reference.folder.toLowerCase();
      if (seenCarReferences.has(key)) return;
      seenCarReferences.add(key);
    }

    candidatesByRating[candidate.rating].push(candidate);
  };

  for (const category of CUP_OPPONENT_CATEGORIES) {
    const isStockMode = category.key === "dc" && isEffectiveStockCarsMode(scanResult, preset);
    const isCategoryRandomized =
      carsSpecState?.[category.includeKey] !== false && !isStockMode;

    if (!isCategoryRandomized) {
      // When a category is not randomized, its stock/DC roster remains in
      // place and each car keeps the rating reported by the scanner.
      category.defaultFolders.forEach((folder, index) => {
        const car = getCarByFolder(availableCars, folder);
        if (!car) return;
        addCandidate(makeCandidate({
          category,
          index,
          car,
          rating: car.rating,
        }));
      });
      continue;
    }

    const rows = carsSpecState?.[category.specKey] || [];
    rows.forEach((row, index) => {
      const specificCar = isSpecificCarPool(row?.sourcePool)
        ? getCarByFolder(availableCars, row.sourcePool)
        : null;
      const rating = getFixedRating(row, specificCar);

      if (rating === null) return;

      addCandidate(makeCandidate({
        category,
        index,
        car: specificCar,
        rating,
      }));
    });
  }

  return candidatesByRating.map((candidates, rating) => ({
    rating,
    label: RATING_LABELS[rating],
    candidates,
  }));
}
