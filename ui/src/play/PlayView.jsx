import { useAppContext } from "../AppProvider";

import LaunchTab from "./LaunchTab";
import PacksTab from "./PacksTab";

export default function PlayView({ errors }) {
  const { state } = useAppContext();
  const { setup: { scanResult } } = state;
  const isLauncher = scanResult?.installType === "launcher";

  return (
    <div style={{ display: "flex", flex: 1, overflowY: "auto", padding: "1rem" }}>
      {isLauncher && <PacksTab />}
      <LaunchTab errors={errors} />
    </div>
  );
}