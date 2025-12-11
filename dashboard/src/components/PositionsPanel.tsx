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
      // Defer setState to avoid React 19 warning
      queueMicrotask(() => {
        setPositions(last.positions ?? []);
      });
    }
  }, [messages]);

  // Initial fetch from REST
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
    <div className="p-4 bg-gray-900 rounded-xl text-gray-100 shadow-md">
      <h2 className="text-lg font-semibold mb-3">Portfolio Positions</h2>

      {positions.length === 0 ? (
        <p className="text-gray-400">No positions yet.</p>
      ) : (
        <table className="w-full text-sm">
          <thead className="text-gray-400 border-b border-gray-700">
            <tr>
              <th className="text-left py-1">Symbol</th>
              <th className="text-left py-1">Qty</th>
              <th className="text-left py-1">Avg Price</th>
              <th className="text-left py-1">Realized PnL</th>
            </tr>
          </thead>
          <tbody>
            {positions.map((p) => (
              <tr key={p.symbol} className="border-b border-gray-800">
                <td className="py-1">{p.symbol}</td>
                <td className="py-1">{p.qty}</td>
                <td className="py-1">{p.avgPrice.toFixed(2)}</td>
                <td
                  className={`py-1 ${
                    p.realizedPnl >= 0 ? "text-green-400" : "text-red-400"
                  }`}
                >
                  {p.realizedPnl.toFixed(2)}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </div>
  );
}
