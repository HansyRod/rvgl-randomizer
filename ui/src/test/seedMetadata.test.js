import assert from "node:assert/strict";
import test from "node:test";
import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { createServer } from "vite";
import { formatSeedDate, getGeneratedAt } from "../utils/seedMetadata.js";

const generatedAt = "2026-10-24T09:20:54.681395400+00:00";
const legacyDate = "2025-01-02T03:04:05+00:00";

test("generation timestamps prefer metadata and fall back to legacy uiContext", () => {
  assert.equal(getGeneratedAt({ generatedAt, uiContext: { generatedAt: legacyDate } }), generatedAt);
  assert.equal(getGeneratedAt({ uiContext: { generatedAt: legacyDate } }), legacyDate);
  assert.equal(getGeneratedAt({ generatedAt }), generatedAt);
  assert.equal(getGeneratedAt(null), undefined);
  assert.equal(formatSeedDate(undefined), "Unknown date");
  assert.equal(formatSeedDate("invalid"), "Unknown date");
  assert.match(formatSeedDate(generatedAt, false), /2026/);
  assert.doesNotMatch(formatSeedDate(generatedAt, false), /\d:\d/);
  assert.match(formatSeedDate(generatedAt), /\d:\d/);
});

test("both summaries show generation time; History has a separate date-only chip", async () => {
  const server = await createServer({ server: { middlewareMode: true, hmr: false } });
  try {
    const { default: SeedSummaryPanel } = await server.ssrLoadModule("/src/components/SeedSummaryPanel.jsx");
    const { default: HistoryPanel } = await server.ssrLoadModule("/src/components/HistoryPanel.jsx");
    const { AppProvider } = await server.ssrLoadModule("/src/AppProvider.jsx");
    const renderSummary = (metadata, mode) => renderToStaticMarkup(createElement(AppProvider, null,
      createElement(SeedSummaryPanel, { seedMetadata: metadata, mode })));
    const metadata = { generatedAt, profileName: "player", uiContext: { generatedAt: legacyDate } };
    for (const mode of ["randomizer", "archipelago"]) {
      const html = renderSummary(metadata, mode);
      assert.ok(html.includes(`Generated: ${formatSeedDate(generatedAt)}`));
      assert.ok(!html.includes(formatSeedDate(legacyDate)));
      for (const unknown of [undefined, null, "", "invalid"]) {
        const withoutDate = renderSummary({ ...metadata, generatedAt: unknown, uiContext: {} }, mode);
        assert.match(withoutDate, /Generated: Unknown date/);
        assert.match(withoutDate, /Profile: player/);
      }
    }
    const legacy = renderSummary({ ...metadata, generatedAt: undefined }, "randomizer");
    assert.ok(legacy.includes(`Generated: ${formatSeedDate(legacyDate)}`));
    const html = renderToStaticMarkup(createElement(HistoryPanel, {
      title: "History", items: [{ path: "current.json", generatedAt }, { path: "older.json" }],
      activeKey: "current.json", getKey: item => item.path, getPrimaryText: item => item.path,
      getBadgeLabel: () => "Player", actionLabel: "Load", onAction() {}, onRemove() {}, summaryLabel: "runs",
    }));
    assert.ok(html.includes(`class="install-badge badge-date">${formatSeedDate(generatedAt, false)}</span>`));
    assert.equal((html.match(/badge-date/g) || []).length, 1);
    assert.match(html, />Player<\/span>/);
    assert.match(html, />Loaded<\/span>/);
    assert.ok(html.indexOf('>Loaded</span>') < html.indexOf('>current.json</span>'));
    assert.doesNotMatch(html, /Unknown date/);
  } finally {
    await server.close();
  }
});
