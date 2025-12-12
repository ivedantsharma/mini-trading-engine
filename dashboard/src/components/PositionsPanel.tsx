import { useEffect, useState } from "react";
import { useMarketSocket } from "../hooks/useMarketSocket";

type Position = {
  symbol: string;
  qty: number;
  avgPrice: number;
  realizedPnl: number;
};

export default function PositionsPanel() {
  const { messages } = useMarketSocket();
  const [positions, setPositions] = useState<Position[]>([]);

  // Listen to WebSocket for { type: "positions" }
  useEffect(() => {
    if (!messages.length) return;
    const last = messages[messages.length - 1];

    if (last.type === "positions") {
      queueMicrotask(() => {
        setPositions(last.positions ?? []);
      });
    }
  }, [messages]);

  // Initial fetch
  useEffect(() => {
    async function load() {
      try {
        const res = await fetch("http://localhost:9003/positions");
        if (!res.ok) return;
        const data = await res.json();
        setPositions(data);
      } catch (err) {
        console.error("Failed to fetch initial positions", err);
      }
    }
    load();
  }, []);

  return (
    <div className="flex flex-col h-full w-full bg-[#181a20] text-xs">
      {/* Sticky Header */}
      <div className="grid grid-cols-4 px-3 py-2 bg-[#0b0e11] border-b border-[#2b3139] text-gray-500 font-semibold sticky top-0 z-10">
        <span className="text-left">Symbol</span>
        <span className="text-right">Size</span>
        <span className="text-right">Entry Price</span>
        <span className="text-right">Realized PnL</span>
      </div>

      {/* Scrollable Body */}
      <div className="flex-1 overflow-y-auto min-h-0">
        {positions.length === 0 ? (
          <div className="flex flex-col items-center justify-center h-full text-gray-600 italic">
            <span>No open positions</span>
          </div>
        ) : (
          positions.map((p) => (
            <div
              key={p.symbol}
              className="grid grid-cols-4 px-3 py-2 border-b border-[#2b3139] hover:bg-[#2b3139] transition-colors group"
            >
              <span className="text-white font-bold">{p.symbol}</span>
              <span
                className={`text-right font-mono ${
                  p.qty > 0
                    ? "text-green-400"
                    : p.qty < 0
                    ? "text-red-400"
                    : "text-gray-300"
                }`}
              >
                {p.qty}
              </span>
              <span className="text-right text-gray-300 font-mono">
                {p.avgPrice.toFixed(2)}
              </span>
              <span
                className={`text-right font-mono font-medium ${
                  p.realizedPnl > 0
                    ? "text-[#0ecb81]"
                    : p.realizedPnl < 0
                    ? "text-[#f6465d]"
                    : "text-gray-400"
                }`}
              >
                {p.realizedPnl > 0 ? "+" : ""}
                {p.realizedPnl.toFixed(2)}
              </span>
            </div>
          ))
        )}
      </div>
    </div>
  );
}
