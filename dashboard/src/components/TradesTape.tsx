// dashboard/src/components/TradesTape.tsx
import React from "react";
import type { TradeMsg } from "../hooks/useMarketSocket";

type Props = {
  trades: TradeMsg[];
};

export default function TradesTape({ trades }: Props) {
  // We keep a fallback timestamp just in case
  const [fallbackTs] = React.useState(() => Date.now() * 1e6);

  return (
    <div className="flex flex-col h-full w-full bg-[#181a20]">
      {/* Sticky Header */}
      <div className="grid grid-cols-3 px-2 py-1 text-[10px] text-gray-500 font-semibold bg-[#0b0e11] border-b border-[#2b3139]">
        <span>Price</span>
        <span className="text-right">Qty</span>
        <span className="text-right">Time</span>
      </div>

      {/* Scrollable Rows */}
      <div className="flex-1 overflow-y-auto font-mono text-xs">
        {trades.length === 0 && (
          <div className="text-center py-4 text-gray-600 italic">
            Waiting for trades...
          </div>
        )}
        {[...trades].reverse().map((t, i, arr) => {
          // Determine color based on price direction relative to NEXT trade in reversed array (which is the previous trade in time)
          // For the very first item (latest trade), we compare with the one after it.
          const prevPrice = arr[i + 1]?.price ?? t.price;
          const priceColor =
            t.price > prevPrice
              ? "text-[#0ecb81]" // Green
              : t.price < prevPrice
              ? "text-[#f6465d]" // Red
              : "text-gray-300"; // Grey/Unchanged

          const timeStr = new Date(
            (t.ts ?? fallbackTs) / 1e6
          ).toLocaleTimeString("en-US", {
            hour12: false,
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
          });

          return (
            <div
              key={`${t.tradeId}-${i}`}
              className="grid grid-cols-3 px-2 py-[2px] hover:bg-[#2b3139] transition-colors"
            >
              <span className={priceColor}>{t.price.toFixed(2)}</span>
              <span className="text-right text-gray-300">{t.quantity}</span>
              <span className="text-right text-gray-500 text-[10px] pt-[1px]">
                {timeStr}
              </span>
            </div>
          );
        })}
      </div>
    </div>
  );
}
