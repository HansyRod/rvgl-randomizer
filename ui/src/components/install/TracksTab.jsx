import Sidebar from "./Sidebar";
import MainContent from "./MainContent";
import { useAppContext } from "../../AppProvider";

export default function TracksTab() {

  const { state : { install : { scanResult } } } = useAppContext();
  
  return (
    <>
      {scanResult.installType === "launcher" && (
          <Sidebar />
      )}      
      <MainContent />
    </>
  );
}