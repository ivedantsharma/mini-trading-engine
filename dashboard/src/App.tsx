import { useState, useEffect } from "react";
import TopOfBook from "./components/TopOfBook";
import OrderBookDepth from "./components/OrderBookDepth";
import PriceSpark from "./components/PriceSpark";
import TradesTape from "./components/TradesTape";
import OrderEntry from "./components/OrderEntry";
import PositionsPanel from "./components/PositionsPanel";
import { useMarketSocket } from "./hooks/useMarketSocket";
import CandleChart from "./components/CandleChart";
import SymbolSelector from "./components/SymbolSelector";

interface CustomWindow extends Window {
  ws?: WebSocket;
}

export default function App() {
  const [symbol, setSymbol] = useState("AAPL");

  // REVERTED: No arguments passed here
  const {
    connected,
    latestForSymbol,
    sendRaw,
    replayMode,
    setReplayMode,
    requestReplay,
    replaySpeed,
    setReplaySpeed,
    isReplayPlaying,
    setIsReplayPlaying,
  } = useMarketSocket();

  const { top, trades } = latestForSymbol(symbol);

  useEffect(() => {
    const w = (window as unknown as CustomWindow).ws;
    if (!w) return;
    w.onopen = () => console.log("WS CONNECTED");
    w.onerror = (e: Event) => console.error("WS ERROR:", e);
  }, []);

  const bids =
    top?.bids && top.bids.length > 0
      ? top.bids
      : top?.bestBid != null
      ? [
          { price: top.bestBid, qty: 100 },
          { price: top.bestBid - 0.05, qty: 150 },
          { price: top.bestBid - 0.1, qty: 200 },
        ]
      : [];

  const asks =
    top?.asks && top.asks.length > 0
      ? top.asks
      : top?.bestAsk != null
      ? [
          { price: top.bestAsk, qty: 100 },
          { price: top.bestAsk + 0.05, qty: 150 },
          { price: top.bestAsk + 0.1, qty: 200 },
        ]
      : [];

  return (
    <div className="h-screen w-screen bg-[#0b0e11] text-[#EAECEF] font-sans flex flex-col overflow-hidden">
      {/* --- TOP HEADER BAR --- */}
      <header className="h-12 bg-[#181a20] border-b border-[#2b3139] flex items-center px-4 justify-between shrink-0">
        <div className="flex items-center gap-6">
          <div className="flex items-center gap-2">
            <div
              className={`w-2 h-2 rounded-full ${
                connected
                  ? "bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.6)]"
                  : "bg-red-500"
              }`}
            />
            <h1 className="text-lg font-bold tracking-tight text-gray-100">
              HFT<span className="text-blue-500">Engine</span>
            </h1>
          </div>

          <div className="h-6 w-px bg-[#2b3139]" />

          <div className="flex items-center gap-2">
            <SymbolSelector value={symbol} onChange={setSymbol} />
            <span className="text-xs text-gray-500 font-mono">PERP</span>
          </div>
        </div>

        {/* Replay Controls Area */}
        <div className="flex items-center gap-3 bg-[#0b0e11] px-3 py-1 rounded border border-[#2b3139]">
          <select
            value={replayMode ? "REPLAY" : "LIVE"}
            onChange={(e) => setReplayMode(e.target.value === "REPLAY")}
            className="bg-transparent text-xs font-bold text-gray-300 outline-none cursor-pointer hover:text-white"
          >
            <option value="LIVE">🟢 LIVE FEED</option>
            <option value="REPLAY">🟠 REPLAY MODE</option>
          </select>

          {replayMode && (
            <>
              <div className="h-4 w-px bg-[#2b3139]" />
              <button
                className="text-xs hover:text-blue-400 transition-colors"
                onClick={() => {
                  requestReplay(symbol, 0, Date.now() * 1e6);
                  setIsReplayPlaying(true);
                }}
              >
                Load
              </button>
              <button
                className="text-xs hover:text-green-400 transition-colors"
                onClick={() => setIsReplayPlaying(!isReplayPlaying)}
              >
                {isReplayPlaying ? "⏸ Pause" : "▶ Play"}
              </button>
              <select
                value={replaySpeed}
                onChange={(e) => setReplaySpeed(Number(e.target.value))}
                className="bg-transparent text-xs text-blue-400 outline-none"
              >
                <option value="0.5">0.5x</option>
                <option value="1">1x</option>
                <option value="2">2x</option>
                <option value="5">5x</option>
                <option value="10">10x</option>
              </select>
            </>
          )}
        </div>
      </header>

      {/* --- MAIN GRID LAYOUT --- */}
      <main className="flex-1 p-1 overflow-hidden grid grid-cols-12 gap-1">
        {/* COLUMN 1: MARKET DATA (OrderBook & Tape) */}
        <div className="col-span-3 flex flex-col gap-1 min-h-0">
          {/* Order Book Panel */}
          <div className="flex-1 bg-[#181a20] rounded border border-[#2b3139] flex flex-col min-h-0">
            <div className="px-3 py-2 border-b border-[#2b3139] text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Order Book
            </div>
            <div className="flex-1 overflow-hidden relative">
              <OrderBookDepth bids={bids} asks={asks} />
            </div>
          </div>

          {/* Trades Tape Panel */}
          <div className="h-1/3 bg-[#181a20] rounded border border-[#2b3139] flex flex-col min-h-0">
            <div className="px-3 py-2 border-b border-[#2b3139] text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Recent Trades
            </div>
            <div className="flex-1 overflow-hidden">
              <TradesTape trades={trades} />
            </div>
          </div>
        </div>

        {/* COLUMN 2: CENTER STAGE (Chart & Positions) */}
        <div className="col-span-6 flex flex-col gap-1 min-h-0">
          {/* Main Chart */}
          <div className="flex-1 bg-[#181a20] rounded border border-[#2b3139] flex flex-col min-h-0 relative group">
            <div className="absolute top-3 left-4 z-10 flex gap-4 pointer-events-none">
              {/* Quick stats overlay on chart */}
              <div className="flex flex-col">
                <span className="text-2xl font-mono text-white font-medium drop-shadow-md">
                  {top?.bestAsk?.toFixed(2) || "---"}
                </span>
                <span className="text-xs text-gray-400">Last Price</span>
              </div>
            </div>
            {/* Chart Container */}
            <div className="w-full h-full">
              <CandleChart symbol={symbol} trades={trades} />
            </div>
          </div>

          {/* Positions Panel */}
          <div className="h-[200px] bg-[#181a20] rounded border border-[#2b3139] flex flex-col min-h-0">
            <div className="px-3 py-2 border-b border-[#2b3139] flex justify-between items-center">
              <span className="text-xs font-semibold text-gray-400 uppercase tracking-wider">
                Positions & Open Orders
              </span>
            </div>
            <div className="flex-1 overflow-hidden">
              <PositionsPanel />
            </div>
          </div>
        </div>

        {/* COLUMN 3: EXECUTION (Order Entry & Top Book) */}
        <div className="col-span-3 flex flex-col gap-1 min-h-0">
          {/* Top Of Book / Summary */}
          <div className="bg-[#181a20] rounded border border-[#2b3139] p-3">
            <TopOfBook
              symbol={symbol}
              bestBid={top?.bestBid ?? null}
              bestAsk={top?.bestAsk ?? null}
            />
          </div>

          {/* Sparkline (Mini Trend) */}
          <div className="bg-[#181a20] rounded border border-[#2b3139] p-3 h-32 flex flex-col">
            <div className="text-xs font-semibold text-gray-400 uppercase mb-1">
              Volatility
            </div>
            <div className="flex-1 min-h-0">
              <PriceSpark trades={trades} />
            </div>
          </div>

          {/* Order Entry (Takes remaining height) */}
          <div className="flex-1 bg-[#181a20] rounded border border-[#2b3139] flex flex-col min-h-0">
            <div className="px-3 py-2 border-b border-[#2b3139] text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Place Order
            </div>
            <div className="flex-1 p-2 overflow-auto">
              <OrderEntry currentSymbol={symbol} sendRaw={sendRaw} />
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}
