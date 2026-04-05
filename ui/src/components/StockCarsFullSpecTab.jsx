import { useEffect } from "react";
import CarsSpecSection from "./CarsSpecSection";
import { STOCK_CARS, DC_CARS } from "../utils/constants";

export default function StockCarsFullSpecTab({ scanResult, specState, setSpecState }) {
  useEffect(() => {
    if (!specState) {
      setSpecState({
        includeStockCars: true,
        includeDcCars: true,
        stockCars: STOCK_CARS.map(id => ({
          id, sourcePool: "Full Random", sourceRating: "Random", sourceObtain: "Random", attrRating: "Random", attrObtain: "Random"
        })),
        dcCars: DC_CARS.map(id => ({
          id, sourcePool: "Full Random", sourceRating: "Random", sourceObtain: "Random", attrRating: "Random", attrObtain: "Random"
        }))
      });
    }
  }, [specState, setSpecState]);

  if (!specState || !specState.stockCars) return null;

  return (
    <CarsSpecSection
      title="Stock Cars"
      categoryKey="stockCars"
      includeKey="includeStockCars"
      defaultCarsList={STOCK_CARS}
      scanResult={scanResult}
      specState={specState}
      setSpecState={setSpecState}
    />
  );
}