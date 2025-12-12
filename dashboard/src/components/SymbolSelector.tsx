import React, { useState } from "react";

type Props = {
  value: string;
  onChange: (s: string) => void;
  presets?: string[];
};

export default function SymbolSelector({
  value,
  onChange,
  presets = ["AAPL", "TSLA", "MSFT", "BTC-USD", "ETH-USD"],
}: Props) {
  const [isEditing, setIsEditing] = useState(false);
  const [tempValue, setTempValue] = useState("");

  const handleCustomSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (tempValue.trim()) {
      onChange(tempValue.toUpperCase());
      setIsEditing(false);
    }
  };

  if (isEditing) {
    return (
      <form onSubmit={handleCustomSubmit} className="flex items-center">
        <input
          autoFocus
          className="bg-[#2b3139] text-white text-sm px-2 py-1 rounded-l outline-none w-24 uppercase font-bold border border-blue-500"
          value={tempValue}
          onChange={(e) => setTempValue(e.target.value)}
          placeholder="SYMBOL"
          onBlur={() => setIsEditing(false)}
        />
        <button
          type="submit"
          className="bg-blue-600 text-white text-xs font-bold px-2 py-[5px] rounded-r border-y border-r border-blue-600"
        >
          GO
        </button>
      </form>
    );
  }

  return (
    <div className="group relative">
      {/* Styled Custom Select */}
      <select
        className="appearance-none bg-[#0b0e11] text-white font-bold text-sm px-3 py-1 pr-8 rounded border border-[#2b3139] hover:border-gray-500 cursor-pointer outline-none focus:border-blue-500 transition-colors"
        value={presets.includes(value) ? value : "CUSTOM"}
        onChange={(e) => {
          if (e.target.value === "CUSTOM") {
            setTempValue("");
            setIsEditing(true);
          } else {
            onChange(e.target.value);
          }
        }}
      >
        {presets.map((p) => (
          <option key={p} value={p}>
            {p}
          </option>
        ))}
        <option value="CUSTOM" className="text-blue-400 font-bold">
          + Search...
        </option>
      </select>

      {/* Custom Chevron Arrow */}
      <div className="pointer-events-none absolute inset-y-0 right-0 flex items-center px-2 text-gray-400 group-hover:text-white">
        <svg
          className="fill-current h-4 w-4"
          xmlns="http://www.w3.org/2000/svg"
          viewBox="0 0 20 20"
        >
          <path d="M9.293 12.95l.707.707L15.657 8l-1.414-1.414L10 10.828 5.757 6.586 4.343 8z" />
        </svg>
      </div>
    </div>
  );
}
