import Sidebar from "./Sidebar";
import MainContent from "./MainContent";
import { useAppContext } from "../AppProvider";

export default function CarsTab() {

  const { state : { app, setup : { scanResult } } } = useAppContext();
  
  return (
    <>
      {app.mode === "randomizer" && scanResult.installType === "launcher" && (
          <Sidebar />
      )}      
      <MainContent />
    </>
  );
}