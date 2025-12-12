import React from "react";

type Props = {
  symbol: string;
  bestBid: number | null;
  bestAsk: number | null;
};

export default function TopOfBook({ symbol, bestBid, bestAsk }: Props) {
  const spread = bestBid && bestAsk ? (bestAsk - bestBid).toFixed(2) : "-.--";
  return (
    <div className="flex flex-col gap-2 w-full">
      {/* Header Row */}
      <div className="flex justify-between items-center text-[10px] text-gray-500 uppercase tracking-wider px-1">
        <span className="font-semibold">Spread</span>
        <span className="text-gray-300 font-mono">{spread}</span>
      </div>

      <div className="flex gap-2">
        {/* BID BOX - Compact */}
        <div className="flex-1 bg-[#16241a] border border-[#0ecb81]/30 rounded px-3 py-2 flex justify-between items-baseline shadow-sm">
          <span className="text-[10px] text-[#0ecb81] font-bold uppercase mr-2">
            Bid
          </span>
          <span className="text-lg font-mono text-[#0ecb81] font-medium tracking-tight">
            {bestBid?.toFixed(2) ?? "---"}
          </span>
        </div>

        {/* ASK BOX - Compact */}
        <div className="flex-1 bg-[#2a141a] border border-[#f6465d]/30 rounded px-3 py-2 flex justify-between items-baseline shadow-sm">
          <span className="text-[10px] text-[#f6465d] font-bold uppercase mr-2">
            Ask
          </span>
          <span className="text-lg font-mono text-[#f6465d] font-medium tracking-tight">
            {bestAsk?.toFixed(2) ?? "---"}
          </span>
        </div>
      </div>
    </div>
  );
}
