import { useAppContext } from "../AppProvider";
import "./PlayView.css";

import LaunchTab from "./LaunchTab";
import PacksTab from "./PacksTab";

export default function PlayView({ errors }) {
  const { state } = useAppContext();
  const { app, setup: { scanResult } } = state;
  const isLauncher = scanResult?.installType === "launcher";
  const isArchipelago = app.mode === "archipelago";

  return (
    <div className="play-view">
      {(isLauncher || isArchipelago) && (
        <p className="tab-description play-view-description">
          {isArchipelago
            ? "Launch RVGL, then open Archipelago in the main menu to connect to your server. To resume, use the same run and RVGL installation."
            : "Configure which content packs are loaded when the randomized game launches."}
        </p>
      )}

      <div className={`play-view-grid ${isLauncher ? "play-view-two-column" : "play-view-single-column"}`}>
        {isLauncher && <PacksTab />}
        <LaunchTab errors={errors} />
      </div>
    </div>
  );
}
