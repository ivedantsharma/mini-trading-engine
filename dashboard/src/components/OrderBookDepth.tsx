import React from "react";

type Level = { price: number; qty: number };

type Props = {
  bids?: Level[];
  asks?: Level[];
};

export default function OrderBookDepth({ bids = [], asks = [] }: Props) {
  return (
    <div className="bg-gray-900 rounded p-3 border border-gray-800">
      <div className="text-sm text-gray-300 mb-2">
        Order Book Depth (Top 10)
      </div>

      <div className="grid grid-cols-2 gap-4">
        {/* ASKS */}
        <div>
          <div className="text-xs text-red-400 mb-1">ASKS</div>
          {asks.length ? (
            <div className="space-y-1">
              {asks.map((a, i) => (
                <div
                  key={i}
                  className="flex justify-between text-sm font-mono text-red-300"
                >
                  <span>{a.price.toFixed(2)}</span>
                  <span>{a.qty}</span>
                </div>
              ))}
            </div>
          ) : (
            <div className="text-xs text-gray-500">No asks</div>
          )}
        </div>

        {/* BIDS */}
        <div>
          <div className="text-xs text-green-400 mb-1">BIDS</div>
          {bids.length ? (
            <div className="space-y-1">
              {bids.map((b, i) => (
                <div
                  key={i}
                  className="flex justify-between text-sm font-mono text-green-300"
                >
                  <span>{b.price.toFixed(2)}</span>
                  <span>{b.qty}</span>
                </div>
              ))}
            </div>
          ) : (
            <div className="text-xs text-gray-500">No bids</div>
          )}
        </div>
      </div>
    </div>
  );
}
