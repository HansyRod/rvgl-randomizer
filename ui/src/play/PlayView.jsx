import { useAppContext } from "../AppProvider";
import "./PlayView.css";

import LaunchTab from "./LaunchTab";
import PacksTab from "./PacksTab";

export default function PlayView({ errors }) {
  const { state } = useAppContext();
  const { setup: { scanResult } } = state;
  const isLauncher = scanResult?.installType === "launcher";

  return (
    <div className="play-view">
      {isLauncher && (
        <p className="tab-description play-view-description">
          Configure which content packs are loaded when the randomized game launches.
        </p>
      )}

      <div className={`play-view-grid ${isLauncher ? "play-view-two-column" : "play-view-single-column"}`}>
        {isLauncher && <PacksTab />}
        <LaunchTab errors={errors} />
      </div>
    </div>
  );
}
