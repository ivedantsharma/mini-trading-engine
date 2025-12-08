import React from "react";
import type { TradeMsg } from "../hooks/useMarketSocket";

type Props = {
  trades: TradeMsg[];
};

export default function TradesTape({ trades }: Props) {
  return (
    <div className="bg-gray-900 rounded p-3 border border-gray-800 h-64 overflow-auto">
      <div className="text-sm text-gray-300 mb-2">Trades</div>
      <div className="space-y-1 text-sm">
        {trades.length === 0 && (
          <div className="text-xs text-gray-500">No trades yet</div>
        )}
        {trades
          .slice()
          .reverse()
          .map((t) => (
            <div key={t.tradeId} className="flex justify-between items-center">
              <div className="font-mono">
                <span className={t.price >= 0 ? "text-green-300" : ""}>
                  {t.price.toFixed(2)}
                </span>
                {"  "}
                <span className="text-gray-400">x</span>{" "}
                <span className="text-gray-300">{t.quantity}</span>
              </div>
              <div className="text-xs text-gray-400">
                {new Date((t.ts || Date.now()) / 1e6).toLocaleTimeString()}
              </div>
            </div>
          ))}
      </div>
    </div>
  );
}
