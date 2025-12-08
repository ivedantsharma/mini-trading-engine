import React from "react";

type Props = {
  symbol: string;
  bestBid: number | null;
  bestAsk: number | null;
};

export default function TopOfBook({ symbol, bestBid, bestAsk }: Props) {
  return (
    <div className="grid grid-cols-2 gap-4 p-3 bg-gray-900 rounded-lg shadow-md border border-gray-800">
      <div>
        <div className="text-xs text-gray-400">Symbol</div>
        <div className="text-2xl font-mono text-white">{symbol}</div>
      </div>
      <div className="flex flex-col items-end">
        <div className="flex gap-3">
          <div className="text-xs text-gray-400">Best Bid</div>
          <div className="text-xl font-mono text-green-400">
            {bestBid !== null ? bestBid.toFixed(2) : "--"}
          </div>
        </div>
        <div className="mt-2 flex gap-3">
          <div className="text-xs text-gray-400">Best Ask</div>
          <div className="text-xl font-mono text-red-400">
            {bestAsk !== null ? bestAsk.toFixed(2) : "--"}
          </div>
        </div>
      </div>
    </div>
  );
}
