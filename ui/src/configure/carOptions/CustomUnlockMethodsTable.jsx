const CUSTOM_UNLOCK_TABLE_ROWS = [
  {
    id: "raceWin",
    typeLabel: "Win Single Race",
    specificKey: "includeSpecificRaceWin",
    specificMinKey: "specificRaceWinTrackCountMin",
    specificMaxKey: "specificRaceWinTrackCountMax",
    countKey: "includeRaceWinCount",
    countMinKey: "raceWinCountMin",
    countMaxKey: "raceWinCountMax",
    countMax: 99,
    description: "Require single-race wins on selected prerequisite tracks, or require a total number of race wins."
  },
  {
    id: "practiceStar",
    typeLabel: "Catch Practice Star",
    specificKey: "includeSpecificPracticeStar",
    specificMinKey: "specificPracticeStarTrackCountMin",
    specificMaxKey: "specificPracticeStarTrackCountMax",
    countKey: "includePracticeStarCount",
    countMinKey: "practiceStarCountMin",
    countMaxKey: "practiceStarCountMax",
    countMax: 99,
    description: "Require practice stars on selected prerequisite tracks, or require a total number of practice stars."
  },
  {
    id: "timeTrial",
    typeLabel: "Beat Time Trial",
    specificKey: "includeSpecificTimeTrial",
    specificMinKey: "specificTimeTrialTrackCountMin",
    specificMaxKey: "specificTimeTrialTrackCountMax",
    countKey: "includeTimeTrialCount",
    countMinKey: "timeTrialCountMin",
    countMaxKey: "timeTrialCountMax",
    countMax: 99,
    description: "Require normal time trials on selected prerequisite tracks, or require a total number of beaten time trials."
  },
  {
    id: "stuntArenaStar",
    typeLabel: "Catch Stunt Arena Star",
    specificKey: null,
    specificMinKey: null,
    specificMaxKey: null,
    countKey: "includeStuntArenaStarCount",
    countMinKey: "stuntArenaStarCountMin",
    countMaxKey: "stuntArenaStarCountMax",
    countMax: 20,
    description: "Require a total number of collected Stunt Arena stars."
  }
];

const TITLE = "Custom Unlock Methods";
const DESCRIPTION_LINES = [
  "Enable custom unlock methods and configure the random ranges used when generation assigns their conditions.",
  "Specific - Requires completing the goal on specific tracks.",
  "Count - Requires completing the goal on a certain number of tracks."
];

function toPositiveInt(value) {
  return Math.max(1, parseInt(value, 10) || 1);
}

function numberValue(options, key, fallback = 1) {
  return options[key] ?? fallback;
}

export default function CustomUnlockMethodsTable({ options, onChange }) {
  return (
    <div className="co-dist-table-box co-custom-unlock-table-box">
      <h3>{TITLE}</h3>
      <p className="co-tiny-desc co-custom-unlock-table-desc">
        {DESCRIPTION_LINES.map((line, index) => (
          <span key={line}>
            {index > 0 && <br />}
            {line}
          </span>
        ))}
      </p>
      <table className="co-dist-table co-custom-unlock-table">
        <thead>
          <tr>
            <th>Unlock Type</th>
            <th>Specific</th>
            <th>Min</th>
            <th>Max</th>
            <th>Count</th>
            <th>Min</th>
            <th>Max</th>
            <th>Description</th>
          </tr>
        </thead>
        <tbody>
          {CUSTOM_UNLOCK_TABLE_ROWS.map(row => {
            const hasSpecific = !!row.specificKey;
            const specificEnabled = hasSpecific && !!options[row.specificKey];
            const countEnabled = !!options[row.countKey];

            return (
              <tr key={row.id}>
                <td className="co-custom-unlock-type">{row.typeLabel}</td>
                <td>
                  {hasSpecific ? (
                    <label className="co-table-check" title="Enable specific prerequisite tracks">
                      <input
                        type="checkbox"
                        checked={specificEnabled}
                        onChange={e => onChange(row.specificKey, e.target.checked)}
                      />
                    </label>
                  ) : (
                    <span className="co-table-muted">-</span>
                  )}
                </td>
                <td>
                  {hasSpecific ? (
                    <input
                      type="number"
                      min={1}
                      max={99}
                      value={numberValue(options, row.specificMinKey)}
                      disabled={!specificEnabled}
                      onChange={e => onChange(row.specificMinKey, toPositiveInt(e.target.value))}
                      title={`${row.typeLabel} specific track minimum`}
                    />
                  ) : (
                    <span className="co-table-muted">-</span>
                  )}
                </td>
                <td>
                  {hasSpecific ? (
                    <input
                      type="number"
                      min={1}
                      max={99}
                      value={numberValue(options, row.specificMaxKey)}
                      disabled={!specificEnabled}
                      onChange={e => onChange(row.specificMaxKey, toPositiveInt(e.target.value))}
                      title={`${row.typeLabel} specific track maximum`}
                    />
                  ) : (
                    <span className="co-table-muted">-</span>
                  )}
                </td>
                <td>
                  <label className="co-table-check" title="Enable overall progress count">
                    <input
                      type="checkbox"
                      checked={countEnabled}
                      onChange={e => onChange(row.countKey, e.target.checked)}
                    />
                  </label>
                </td>
                <td>
                  <input
                    type="number"
                    min={1}
                    max={row.countMax}
                    value={numberValue(options, row.countMinKey)}
                    disabled={!countEnabled}
                    onChange={e => onChange(row.countMinKey, toPositiveInt(e.target.value))}
                    title={`${row.typeLabel} count minimum`}
                  />
                </td>
                <td>
                  <input
                    type="number"
                    min={1}
                    max={row.countMax}
                    value={numberValue(options, row.countMaxKey, row.countMax)}
                    disabled={!countEnabled}
                    onChange={e => onChange(row.countMaxKey, toPositiveInt(e.target.value))}
                    title={`${row.typeLabel} count maximum`}
                  />
                </td>
                <td className="co-custom-unlock-desc">{row.description}</td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
