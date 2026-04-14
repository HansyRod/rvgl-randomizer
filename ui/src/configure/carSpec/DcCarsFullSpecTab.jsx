import CarsSpecSection from "./CarsSpecSection";
import { DC_CARS } from "../../utils/constants";

export default function DcCarsFullSpecTab() {

  return (
    <CarsSpecSection
      title="DC Cars"
      categoryKey="dcCars"
      includeKey="includeDcCars"
      defaultCarsList={DC_CARS}
    />
  );
}