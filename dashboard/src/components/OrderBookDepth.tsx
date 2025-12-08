import React from "react";

type Props = {
  bids?: Array<{ price: number; qty: number }>;
  asks?: Array<{ price: number; qty: number }>;
};

// For now we'll show placeholder depth using bestBid/bestAsk +/- small ticks if depth is not available.
export default function OrderBookDepth({ bids = [], asks = [] }: Props) {
  return (
    <div className="bg-gray-900 rounded p-3 border border-gray-800">
      <div className="text-sm text-gray-300 mb-2">
        Order Book Depth (top levels)
      </div>
      <div className="grid grid-cols-2 gap-2">
        <div>
          <div className="text-xs text-gray-400">Asks (price ↑)</div>
          <div className="space-y-1 mt-2">
            {asks.length ? (
              asks.map((a, i) => (
                <div
                  key={i}
                  className="flex justify-between text-sm font-mono text-red-300"
                >
                  <div>{a.price.toFixed(2)}</div>
                  <div>{a.qty}</div>
                </div>
              ))
            ) : (
              <div className="text-xs text-gray-500">No depth</div>
            )}
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Bids (price ↓)</div>
          <div className="space-y-1 mt-2">
            {bids.length ? (
              bids.map((b, i) => (
                <div
                  key={i}
                  className="flex justify-between text-sm font-mono text-green-300"
                >
                  <div>{b.price.toFixed(2)}</div>
                  <div>{b.qty}</div>
                </div>
              ))
            ) : (
              <div className="text-xs text-gray-500">No depth</div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
