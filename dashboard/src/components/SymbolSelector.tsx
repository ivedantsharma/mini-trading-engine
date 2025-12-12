import React, { useState } from "react";

type Props = {
  value: string;
  onChange: (s: string) => void;
  presets?: string[];
};

export default function SymbolSelector({
  value,
  onChange,
  presets = ["AAPL", "TSLA", "MSFT", "BTC-USD"],
}: Props) {
  const [custom, setCustom] = useState("");

  function loadCustom() {
    const sym = custom.trim().toUpperCase();
    if (!sym) return;
    onChange(sym);
    setCustom("");
  }

  return (
    <div className="flex items-center gap-2">
      <select
        className="bg-gray-800 px-2 py-1 rounded text-white"
        value={value}
        onChange={(e) => onChange(e.target.value)}
      >
        {presets.map((p) => (
          <option key={p} value={p}>
            {p}
          </option>
        ))}
      </select>

      <div className="flex items-center gap-2">
        <input
          placeholder="Custom (e.g. DOGE-USD)"
          value={custom}
          onChange={(e) => setCustom(e.target.value)}
          className="bg-gray-800 px-2 py-1 rounded text-white w-36"
        />
        <button
          className="px-3 py-1 rounded bg-blue-600 text-white text-sm"
          onClick={loadCustom}
        >
          Load
        </button>
      </div>
    </div>
  );
}
