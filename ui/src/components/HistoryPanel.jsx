import { useEffect, useState } from "react";

const DEFAULT_PAGE_SIZE = 5;

export default function HistoryPanel({
  title,
  items,
  activeKey,
  getKey,
  getPrimaryText,
  getBadgeLabel,
  getBadgeClassName,
  getPrimaryTitle = getPrimaryText,
  actionLabel,
  onAction,
  onRemove,
  disabled = false,
  summaryLabel,
  pageSize = DEFAULT_PAGE_SIZE,
  className = "setup-section",
  style,
}) {
  const [page, setPage] = useState(0);

  const previousItems = (Array.isArray(items) ? items : []).filter((item) => getKey(item) !== activeKey);
  const lastPage = Math.max(Math.ceil(previousItems.length / pageSize) - 1, 0);
  const pageStart = page * pageSize;
  const visibleItems = previousItems.slice(pageStart, pageStart + pageSize);
  const visibleStart = previousItems.length === 0 ? 0 : pageStart + 1;
  const visibleEnd = Math.min(pageStart + pageSize, previousItems.length);

  useEffect(() => {
    if (page > lastPage) {
      setPage(lastPage);
    }
  }, [page, lastPage]);

  if (previousItems.length === 0) {
    return null;
  }

  return (
    <div className={className} style={style}>
      <h3 className="setup-section-title">{title}</h3>
      <div className="install-history-list">
        {visibleItems.map((item) => {
          const key = getKey(item);
          const badgeLabel = getBadgeLabel?.(item);
          const badgeClassName = getBadgeClassName?.(item) || "install-badge";

          return (
            <div key={key} className="install-history-item">
              <div className="install-history-details">
                <span className="install-path-text" title={getPrimaryTitle(item)}>
                  {getPrimaryText(item)}
                </span>
                {badgeLabel ? <span className={badgeClassName}>{badgeLabel}</span> : null}
              </div>
              <div style={{ display: "flex", gap: "0.5rem" }}>
                <button
                  className="btn-secondary btn-action-wide"
                  onClick={() => onAction(item)}
                  disabled={disabled}
                >
                  {actionLabel}
                </button>
                <button
                  className="btn-secondary btn-icon"
                  onClick={() => onRemove(key)}
                  disabled={disabled}
                  title="Remove from history"
                  style={{ color: "var(--text-secondary)" }}
                >
                  <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                  </svg>
                </button>
              </div>
            </div>
          );
        })}
      </div>
      <div className="history-toolbar">
        <span className="history-summary">
          Displaying {summaryLabel} {visibleStart}-{visibleEnd} of {previousItems.length}
        </span>
        <div className="history-pagination">
          <button
            className="btn-secondary btn-icon"
            onClick={() => setPage(0)}
            disabled={disabled || page === 0}
            title="First Page"
          >
            <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M11 19L4 12l7-7M20 19l-7-7 7-7M4 5v14" />
            </svg>
          </button>
          <button
            className="btn-secondary btn-icon"
            onClick={() => setPage((currentPage) => Math.max(currentPage - 1, 0))}
            disabled={disabled || page === 0}
            title="Previous"
          >
            <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M15 19l-7-7 7-7" />
            </svg>
          </button>
          <button
            className="btn-secondary btn-icon"
            onClick={() => setPage((currentPage) => Math.min(currentPage + 1, lastPage))}
            disabled={disabled || page >= lastPage}
            title="Next"
          >
            <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 5l7 7-7 7" />
            </svg>
          </button>
          <button
            className="btn-secondary btn-icon"
            onClick={() => setPage(lastPage)}
            disabled={disabled || page >= lastPage}
            title="Last Page"
          >
            <svg width="16" height="16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M13 5l7 7-7 7M4 5l7 7-7 7M20 5v14" />
            </svg>
          </button>
        </div>
      </div>
    </div>
  );
}
