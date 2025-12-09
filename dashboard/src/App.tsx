import { useState } from "react";
import TopOfBook from "./components/TopOfBook";
import OrderBookDepth from "./components/OrderBookDepth";
import PriceSpark from "./components/PriceSpark";
import TradesTape from "./components/TradesTape";
import OrderEntry from "./components/OrderEntry";

import { useMarketSocket } from "./hooks/useMarketSocket";

export default function App() {
  const [symbol, setSymbol] = useState("AAPL");

  // Hook now provides sendRaw()
  const { connected, latestForSymbol, sendRaw } = useMarketSocket();

  const { top, trades } = latestForSymbol(symbol);

  // Depth for UI (we no longer use Math.random inside render)
  const bids =
    top?.bestBid != null
      ? [
          { price: top.bestBid, qty: 12 },
          { price: top.bestBid - 0.1, qty: 9 },
          { price: top.bestBid - 0.2, qty: 4 },
        ]
      : [];

  const asks =
    top?.bestAsk != null
      ? [
          { price: top.bestAsk, qty: 11 },
          { price: top.bestAsk + 0.1, qty: 8 },
          { price: top.bestAsk + 0.2, qty: 5 },
        ]
      : [];

  return (
    <div className="min-h-screen bg-[#0b0f19] text-white p-6">
      <div className="flex justify-between items-center mb-6">
        <h1 className="text-xl font-semibold">
          Mini Trading Engine — Dashboard{" "}
          <span className="text-sm text-green-400 ml-2">Live</span>
        </h1>

        <div className="flex items-center gap-4">
          <div className="text-gray-300 text-sm">
            <span className="mr-2">Symbol</span>
            <input
              value={symbol}
              onChange={(e) => setSymbol(e.target.value.toUpperCase())}
              className="bg-gray-800 px-2 py-1 rounded text-white w-20"
            />
          </div>

          <span
            className={
              "px-3 py-1 rounded text-sm " +
              (connected ? "bg-green-600" : "bg-red-600")
            }
          >
            {connected ? "Connected" : "Disconnected"}
          </span>
        </div>
      </div>

      {/* 2-column layout */}
      <div className="grid grid-cols-[400px_1fr] gap-6">
        {/* LEFT SIDE */}
        <div className="space-y-6">
          {/* Top Of Book */}
          <TopOfBook
            symbol={symbol}
            bestBid={top?.bestBid ?? null}
            bestAsk={top?.bestAsk ?? null}
          />

          {/* Depth + Sparkline + Order Entry */}
          <div className="space-y-6">
            <OrderBookDepth bids={bids} asks={asks} />
            <PriceSpark trades={trades} />

            {/* Order Entry Form */}
            <OrderEntry currentSymbol={symbol} sendRaw={sendRaw} />
          </div>
        </div>

        {/* RIGHT SIDE — Trades */}
        <div>
          <TradesTape trades={trades} />
        </div>
      </div>
    </div>
  );
}
