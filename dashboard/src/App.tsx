import React, { useMemo, useState } from "react";
import { useMarketSocket } from "./hooks/useMarketSocket";
import TickerSelect from "./components/TickerSelect";
import TopOfBook from "./components/TopOfBook";
import OrderBookDepth from "./components/OrderBookDepth";
import TradesTape from "./components/TradesTape";
import PriceSpark from "./components/PriceSpark";

function App() {
  const { connected, messages, latestForSymbol } = useMarketSocket();
  // derive symbols from messages
  const symbols = useMemo(() => {
    const s = new Set<string>();
    for (const m of messages) s.add(m.symbol);
    return Array.from(s).sort();
  }, [messages]);

  const initialSymbol = symbols[0] ?? "AAPL";
  const [symbol, setSymbol] = useState(initialSymbol);

  // keep symbol in sync when new ones appear
  React.useEffect(() => {
    if (!symbols.includes(symbol) && symbols.length) setSymbol(symbols[0]);
  }, [symbols]);

  const { top, trades } = latestForSymbol(symbol);

  // simplistic depth placeholders: create synthetic depth if we only have top
  // add near top of App (inside the component)
  const [bids, setBids] = React.useState<Array<{ price: number; qty: number }>>(
    []
  );
  const [asks, setAsks] = React.useState<Array<{ price: number; qty: number }>>(
    []
  );

  React.useEffect(() => {
    // Whenever top changes, regenerate synthetic depth OFF the render path
    if (!top) {
      setBids([]);
      setAsks([]);
      return;
    }

    // helper to make a small random qty but keep it stable within this effect run
    const makeQty = () => Math.floor(Math.random() * 50) + 1;

    const newBids = top.bestBid
      ? Array.from({ length: 5 }, (_, i) => ({
          price: parseFloat((top.bestBid! - i * 0.1).toFixed(2)),
          qty: makeQty(),
        }))
      : [];

    const newAsks = top.bestAsk
      ? Array.from({ length: 5 }, (_, i) => ({
          price: parseFloat((top.bestAsk! + i * 0.1).toFixed(2)),
          qty: makeQty(),
        }))
      : [];

    setBids(newBids);
    setAsks(newAsks);

    // dependency ensures this effect only runs when top changes
  }, [top?.bestBid, top?.bestAsk]);

  return (
    <div className="min-h-screen bg-[#0b0f14] text-white p-4">
      <div className="max-w-7xl mx-auto">
        <header className="flex items-center justify-between mb-4">
          <div className="flex items-center gap-4">
            <div className="text-xl font-semibold">
              Mini Trading Engine — Dashboard
            </div>
            <div className="text-sm text-gray-400">Live</div>
            <div
              className={`px-2 py-1 rounded text-xs ${
                connected ? "bg-green-700" : "bg-red-700"
              }`}
            >
              {connected ? "WS Connected" : "Disconnected"}
            </div>
          </div>
          <div className="flex items-center gap-4">
            <TickerSelect
              symbols={symbols.length ? symbols : ["AAPL"]}
              value={symbol}
              onChange={setSymbol}
            />
          </div>
        </header>

        <main className="grid grid-cols-12 gap-4">
          <div className="col-span-6">
            <TopOfBook
              symbol={symbol}
              bestBid={top?.bestBid ?? null}
              bestAsk={top?.bestAsk ?? null}
            />
            <div className="mt-4 grid grid-cols-2 gap-4">
              <OrderBookDepth bids={bids} asks={asks} />
              <PriceSpark trades={trades} />
            </div>
          </div>

          <div className="col-span-6">
            <TradesTape trades={trades} />
          </div>
        </main>
      </div>
    </div>
  );
}

export default App;
