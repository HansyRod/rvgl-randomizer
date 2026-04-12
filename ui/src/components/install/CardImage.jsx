import { useState } from "react";

export default function CardImage({ src, alt }) {
  const [error, setError] = useState(false);

  if (error) {
    return (
      <div className="card-image">
        <span className="card-image-fallback">No Image</span>
      </div>
    );
  }

  return (
    <img 
      className="card-image"
      src={src} 
      alt={alt} 
      onError={() => setError(true)}
    />
  );
}
