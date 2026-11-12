import { ARCHIPELAGO_GOALS } from "../import/archipelagoSeed";

const LOGIC_DIFFICULTIES = {
  casual: "Casual",
  standard: "Standard",
  expert: "Expert",
};

const LOCATION_POOL_OPTIONS = [
  ["include_single_race_wins", "Include Single Race Wins"],
  ["include_practice_stars", "Include Practice Stars"],
  ["include_normal_time_trials", "Include Normal Time Trials"],
  ["include_reverse_time_trials", "Include Reverse Time Trials"],
  ["include_mirror_time_trials", "Include Mirror Time Trials"],
  ["include_stunt_arena_stars", "Include Stunt Arena Stars"],
  ["carsanity", "Carsanity"],
];

export default function ArchipelagoSummary({ metadata, compact }) {
  const yamlOptions = metadata.yamlOptions || {};
  const logicDifficulty = Object.hasOwn(LOGIC_DIFFICULTIES, yamlOptions.logic_difficulty)
    ? LOGIC_DIFFICULTIES[yamlOptions.logic_difficulty] : null;
  const locationPool = LOCATION_POOL_OPTIONS.flatMap(([key, label]) => {
    const value = yamlOptions[key];
    if (key === "carsanity") {
      return value === "enabled" || value === "disabled" ? [{ key, label, enabled: value === "enabled" }] : [];
    }
    return typeof value === "boolean" ? [{ key, label, enabled: value }] : [];
  });

  return (
    <>
      <div className="summary-section">
        <div className="section-label">PLAYER SLOT</div>
        <div className="section-content"><strong>{metadata.slotName}</strong></div>
      </div>
      <div className="summary-section">
        <div className="section-label">GOAL</div>
        <div className="section-content">{Object.hasOwn(ARCHIPELAGO_GOALS, metadata.goal) ? ARCHIPELAGO_GOALS[metadata.goal] : "Unknown goal"}</div>
      </div>
      {!compact && logicDifficulty && (
        <div className="summary-section">
          <div className="section-label">LOGIC DIFFICULTY</div>
          <div className="section-content">{logicDifficulty}</div>
        </div>
      )}
      {!compact && locationPool.length > 0 && (
        <div className="summary-section">
          <div className="section-label">LOCATION POOL</div>
          <ul className="location-pool-options">
            {locationPool.map(({ key, label, enabled }) => (
              <li key={key}>
                <span aria-label={enabled ? "Enabled" : "Disabled"}>{enabled ? "✓" : "✗"}</span>
                {label}
              </li>
            ))}
          </ul>
        </div>
      )}
    </>
  );
}
