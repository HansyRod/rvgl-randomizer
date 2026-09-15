import { useAppContext } from "../../AppProvider";
import { DEFAULT_FEATURE_OPTIONS } from "../../utils/constants";
import "../carOptions/CarOptionsTab.css";

const OPTION_GROUPS = [
  {
    title: "Load Extra Content",
    description:
      "Controls if the game loads content that isn't used by the randomization process. If an option is enabled, all content of that type present in the configured Install will be available in-game.",
    options: [
      { key: "loadExtraCars", label: "Load Extra Cars" },
      { key: "loadExtraTracks", label: "Load Extra Tracks" },
      { key: "loadExtraCups", label: "Load Extra Cups" },
    ],
  },
  {
    title: "Features",
    description: "Enable additional race features supported by the randomizer.",
    options: [
      {
        key: "enable30CarMode",
        label: "30-Car Mode",
        description: "Allow Single Race and Championship modes to use more than the native 16-car limit, up to a maximum of 30 cars.",
      },
      {
        key: "enableKnockoutMode",
        label: "Knockout Mode",
        description: "Enable the custom Knockout race mode. In this mode, the last-placed cars face the risk of elimination at the end of each lap.",
      },
    ],
  },
];

export default function GlobalOptionsTab() {
  const { state, updateCategoryCtx } = useAppContext();
  const featureOptions = {
    ...DEFAULT_FEATURE_OPTIONS,
    ...(state.configure?.featureOptions || {}),
  };

  const updateFeatureOption = (key, value) => {
    updateCategoryCtx("configure", {
      featureOptions: {
        ...featureOptions,
        [key]: value,
      },
    });
  };

  return (
    <div className="car-options-tab global-options-tab">
      {OPTION_GROUPS.map((group) => (
        <section className="co-section" key={group.title}>
          <h2 className="co-section-title">{group.title}</h2>
          <p className="co-desc">{group.description}</p>
          <div className="co-checkbox-group">
            {group.options.map((option) => (
              <label className="co-checkbox-row" key={option.key}>
                <input
                  type="checkbox"
                  checked={featureOptions[option.key]}
                  onChange={(event) =>
                    updateFeatureOption(option.key, event.target.checked)
                  }
                />
                <span>
                  <strong>{option.label}</strong>
                  {option.description ? ` — ${option.description}` : ""}
                </span>
              </label>
            ))}
          </div>
        </section>
      ))}
    </div>
  );
}
