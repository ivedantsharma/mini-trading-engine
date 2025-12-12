import React from "react";

type Level = { price: number; qty: number };

type Props = {
  bids?: Level[];
  asks?: Level[];
};

export default function OrderBookDepth({ bids = [], asks = [] }: Props) {
  // Sort asks: High -> Low (so the lowest ask is at the bottom, closest to the spread)
  const sortedAsks = [...asks].sort((a, b) => b.price - a.price);
  // Sort bids: High -> Low (so the highest bid is at the top, closest to the spread)
  const sortedBids = [...bids].sort((a, b) => b.price - a.price);

  // Calculate max volume for the depth bars
  const maxQty = Math.max(
    ...bids.map((b) => b.qty),
    ...asks.map((a) => a.qty),
    1
  );

  return (
    <div className="flex flex-col h-full w-full bg-[#181a20] text-xs font-mono overflow-hidden select-none">
      {/* HEADER */}
      <div className="flex justify-between px-2 py-1 text-gray-500 font-semibold border-b border-[#2b3139] bg-[#0b0e11]">
        <span>Price(USD)</span>
        <span>Qty</span>
      </div>

      {/* SCROLLABLE AREA */}
      <div className="flex-1 overflow-y-auto no-scrollbar relative">
        {/* ASKS (Red) - Rendered from Top (High Price) to Bottom (Best Ask) */}
        <div className="flex flex-col justify-end min-h-0">
          {sortedAsks.map((a, i) => {
            const widthPct = Math.min((a.qty / maxQty) * 100, 100);
            return (
              <div
                key={`ask-${i}`}
                className="flex justify-between items-center px-2 py-0.5 relative hover:bg-[#201d24]"
              >
                {/* Depth Bar Background */}
                <div
                  className="absolute right-0 top-0 bottom-0 bg-red-900/20 z-0 transition-all duration-200"
                  style={{ width: `${widthPct}%` }}
                />
                <span className="text-[#f6465d] z-10">
                  {a.price.toFixed(2)}
                </span>
                <span className="text-gray-300 z-10">{a.qty.toFixed(0)}</span>
              </div>
            );
          })}
        </div>

        {/* SPREAD INDICATOR */}
        {asks.length > 0 && bids.length > 0 && (
          <div className="py-1 my-1 bg-[#0b0e11] border-y border-[#2b3139] text-center text-gray-400 font-bold tracking-widest text-[10px]">
            {(asks[asks.length - 1].price - bids[0].price).toFixed(2)}{" "}
            <span className="font-normal text-gray-600">SPREAD</span>
          </div>
        )}

        {/* BIDS (Green) - Rendered from Top (Best Bid) to Bottom (Low Price) */}
        <div>
          {sortedBids.map((b, i) => {
            const widthPct = Math.min((b.qty / maxQty) * 100, 100);
            return (
              <div
                key={`bid-${i}`}
                className="flex justify-between items-center px-2 py-0.5 relative hover:bg-[#16241a]"
              >
                {/* Depth Bar Background */}
                <div
                  className="absolute right-0 top-0 bottom-0 bg-green-900/20 z-0 transition-all duration-200"
                  style={{ width: `${widthPct}%` }}
                />
                <span className="text-[#0ecb81] z-10">
                  {b.price.toFixed(2)}
                </span>
                <span className="text-gray-300 z-10">{b.qty.toFixed(0)}</span>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}
