import { CAR_RATINGS } from "../../utils/constants";

export default function RatingDistTable({ title, desc, data, onChange, includeSuperPro, maxSlots }) {
  const ratings = ["0", "1", "2", "3", "4", "5"];
  
  return (
    <div className="co-dist-table-box">
      <h3>{title}</h3>
      <p className="co-tiny-desc">{desc}</p>
      <table className="co-dist-table">
        <thead>
          <tr>
            <th></th>
            <th>Rating</th>
            <th>Min</th>
            <th>Max</th>
          </tr>
        </thead>
        <tbody>
          {ratings.map(rid => {
            const isSuperPro = rid === "5";
            const locked = isSuperPro && !includeSuperPro;
            const dist = data[rid];

            return (
              <tr key={rid} className={locked ? "locked-row" : ""}>
                <td>
                  <input 
                    type="checkbox" 
                    checked={dist.enabled} 
                    disabled={locked}
                    onChange={e => onChange(rid, "enabled", e.target.checked)}
                  />
                </td>
                <td className="rating-label">{CAR_RATINGS[rid]}</td>
                <td>
                  <input 
                    type="number" 
                    min={0} max={maxSlots}
                    value={dist.min} 
                    disabled={locked || !dist.enabled}
                    onChange={e => onChange(rid, "min", parseInt(e.target.value) || 0)}
                  />
                </td>
                <td>
                  <input 
                    type="number" 
                    min={0} max={maxSlots}
                    value={dist.max} 
                    disabled={locked || !dist.enabled}
                    onChange={e => onChange(rid, "max", parseInt(e.target.value) || 0)}
                  />
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
