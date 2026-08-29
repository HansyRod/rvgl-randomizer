import CarsSpecSection from "./CarsSpecSection";

export default function ExtraCarsFullSpecTab() {
  return (
    <CarsSpecSection
      title="Extra Cars"
      categoryKey="extraCars"
      defaultCarsList={[]}
      isDynamic
    />
  );
}
