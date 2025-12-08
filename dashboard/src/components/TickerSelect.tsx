import React from "react";

type Props = {
  symbols: string[];
  value: string;
  onChange: (s: string) => void;
};

export default function TickerSelect({ symbols, value, onChange }: Props) {
  return (
    <div className="flex items-center gap-2">
      <label className="text-sm text-gray-300">Symbol</label>
      <select
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className="select select-bordered select-sm bg-gray-900 text-white"
      >
        {symbols.map((s) => (
          <option key={s} value={s}>
            {s}
          </option>
        ))}
      </select>
    </div>
  );
}
