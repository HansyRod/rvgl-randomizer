import CarsSpecSection from "./CarsSpecSection";
import { STOCK_CARS } from "../../utils/constants";

export default function StockCarsFullSpecTab() {

  return (
    <CarsSpecSection
      title="Stock Cars"
      categoryKey="stockCars"
      includeKey="includeStockCars"
      defaultCarsList={STOCK_CARS}
    />
  );
}