import { RATING_LABELS } from "./CupUtils";
import { getCupOpponentReferenceKey } from "./CupOpponentUtils";

function getClassQuota(carsPerClass, rating) {
  const value = Number(carsPerClass?.[rating]);
  return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
}

function getGroupedCandidates(candidates, selected, currentIndex) {
  const usedKeys = new Set(
    selected
      .filter((_, index) => index !== currentIndex)
      .map(reference => getCupOpponentReferenceKey(reference))
  );
  const available = candidates.filter(candidate => {
    const key = getCupOpponentReferenceKey(candidate.reference);
    return !usedKeys.has(key);
  });

  return [
    { label: "Specific Slot", type: "slot" },
    { label: "Specific Car", type: "car" },
  ].map(group => ({
    ...group,
    candidates: available.filter(candidate => candidate.reference.type === group.type),
  })).filter(group => group.candidates.length > 0);
}

export default function CupOpponentEditor({
  candidatesByRating,
  carsPerClass,
  opponents,
  onChange,
}) {
  const candidateByReference = new Map(
    candidatesByRating.flatMap(group =>
      group.candidates.map(candidate => [
        getCupOpponentReferenceKey(candidate.reference),
        candidate,
      ])
    )
  );

  const selectedByRating = candidatesByRating.map(() => []);
  (opponents || []).forEach(reference => {
    const candidate = candidateByReference.get(getCupOpponentReferenceKey(reference));
    if (candidate) selectedByRating[candidate.rating].push(reference);
  });

  const updateRatingSelections = (rating, index, value) => {
    const current = [...selectedByRating[rating]];
    const reference = candidatesByRating[rating].candidates.find(candidate =>
      getCupOpponentReferenceKey(candidate.reference) === value
    )?.reference;

    if (reference) {
      current[index] = reference;
    } else {
      current.splice(index, 1);
    }

    const currentRatingKeys = new Set(
      candidatesByRating[rating].candidates.map(candidate =>
        getCupOpponentReferenceKey(candidate.reference)
      )
    );
    const otherRatings = (opponents || []).filter(existing =>
      !currentRatingKeys.has(getCupOpponentReferenceKey(existing))
    );

    onChange([...otherRatings, ...current]);
  };

  return (
    <div className="cup-opponents-editor">
      <p className="co-desc">
        Choose fixed opponents by rating. The Cars Per Class value determines how many
        opponents are used; one additional entry is available as a player-car fallback.
      </p>
      <div className="cup-opponents-grid">
        {RATING_LABELS.map((label, rating) => {
          const quota = getClassQuota(carsPerClass, rating);
          const maxSelections = quota > 0 ? quota + 1 : 0;
          const selected = selectedByRating[rating];
          const candidates = candidatesByRating[rating]?.candidates || [];
          const visibleCount = maxSelections === 0
            ? 0
            : Math.min(maxSelections, Math.max(1, selected.length + 1));

          return (
            <div key={label} className="cup-opponent-group">
              <div className="cup-opponent-group-title">{label}</div>
              {maxSelections === 0 ? (
                <p className="cup-opponent-empty">No opponents of this class.</p>
              ) : candidates.length === 0 ? (
                <p className="cup-opponent-empty">No fixed-rating choices available.</p>
              ) : (
                Array.from({ length: visibleCount }, (_, index) => {
                  const selectedReference = selected[index];
                  const selectedKey = getCupOpponentReferenceKey(selectedReference);
                  const isFallback = visibleCount === maxSelections && index === maxSelections - 1;
                  const candidateGroups = getGroupedCandidates(candidates, selected, index);

                  return (
                    <div
                      key={`${label}-${index}`}
                      className={`cup-opponent-choice${isFallback ? " fallback" : ""}`}
                    >
                      {isFallback && (
                        <span className="cup-opponent-fallback-note">
                          Fallback: used only when the player car has this rating and is already listed earlier.
                        </span>
                      )}
                      <div className="cup-opponent-select-row">
                      <select
                        value={selectedKey}
                        onChange={e => updateRatingSelections(rating, index, e.target.value)}
                        className="cup-opponent-select"
                      >
                        <option value="" disabled hidden>Choose an opponent...</option>
                        {candidateGroups.map(group => (
                          <optgroup key={group.type} label={group.label}>
                            {group.candidates.map(candidate => {
                              const key = getCupOpponentReferenceKey(candidate.reference);
                              return (
                                <option key={key} value={key}>
                                  {candidate.label}
                                </option>
                              );
                            })}
                          </optgroup>
                        ))}
                      </select>
                      <button
                        type="button"
                        className={`cup-stage-remove${selectedKey ? "" : " cup-opponent-remove-hidden"}`}
                        onClick={() => updateRatingSelections(rating, index, "")}
                        aria-label={`Remove ${label} opponent ${index + 1}`}
                        aria-hidden={!selectedKey}
                        tabIndex={selectedKey ? 0 : -1}
                        title="Remove opponent"
                      >
                        ✕
                      </button>
                      </div>
                    </div>
                  );
                })
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}
