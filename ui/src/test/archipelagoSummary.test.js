import assert from "node:assert/strict";
import test from "node:test";
import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { createServer } from "vite";

test("AP Import shows only the requested YAML summary; compact Play keeps its existing layout", async () => {
  const server = await createServer({ server: { middlewareMode: true, hmr: false } });
  try {
    const { default: SeedSummaryPanel } = await server.ssrLoadModule("/src/components/SeedSummaryPanel.jsx");
    const { AppProvider } = await server.ssrLoadModule("/src/AppProvider.jsx");
    const metadata = {
      slotName: "Player", profileName: "ap_test", goal: "platinum_cup",
      yamlOptions: {
        logic_difficulty: "standard", car_distribution: "shuffle", starting_cars: 3,
        include_single_race_wins: true, include_practice_stars: true,
        include_normal_time_trials: false, include_reverse_time_trials: false,
        include_mirror_time_trials: false, include_stunt_arena_stars: true, carsanity: "enabled",
      },
    };
    const render = (seedMetadata, compact = false) => renderToStaticMarkup(createElement(AppProvider, null,
      createElement(SeedSummaryPanel, { seedMetadata, compact, mode: "archipelago" })));
    const expanded = render(metadata);
    assert.match(expanded, /LOGIC DIFFICULTY/);
    assert.match(expanded, /Standard/);
    assert.match(expanded, /LOCATION POOL/);
    for (const label of ["Include Single Race Wins", "Include Practice Stars", "Include Stunt Arena Stars", "Carsanity"]) {
      assert.ok(expanded.includes(`aria-label="Enabled">✓</span>${label}`));
    }
    for (const label of ["Include Normal Time Trials", "Include Reverse Time Trials", "Include Mirror Time Trials"]) {
      assert.ok(expanded.includes(`aria-label="Disabled">✗</span>${label}`));
    }
    assert.doesNotMatch(expanded, /shuffle|Starting Cars|Car Distribution/);

    const compact = render(metadata, true);
    assert.doesNotMatch(compact, /LOGIC DIFFICULTY|LOCATION POOL/);
    assert.equal(compact, render({ ...metadata, yamlOptions: undefined }));
    assert.doesNotMatch(compact, /seed-summary-panel compact/);
    assert.match(compact, /PLAYER SLOT/);
    assert.match(compact, /GOAL/);
    assert.match(compact, /REQUIRES/);

    const missing = render({ ...metadata, yamlOptions: undefined });
    assert.doesNotMatch(missing, /LOGIC DIFFICULTY|LOCATION POOL/);
    const partial = render({ ...metadata, yamlOptions: { include_practice_stars: false, carsanity: "disabled" } });
    assert.doesNotMatch(partial, /LOGIC DIFFICULTY|Include Single Race Wins|Include Normal Time Trials/);
    assert.ok(partial.includes('aria-label="Disabled">✗</span>Include Practice Stars'));
    assert.ok(partial.includes('aria-label="Disabled">✗</span>Carsanity'));
    const logicOnly = render({ ...metadata, yamlOptions: { logic_difficulty: "expert" } });
    assert.match(logicOnly, /Expert/);
    assert.doesNotMatch(logicOnly, /LOCATION POOL/);
  } finally {
    await server.close();
  }
});
