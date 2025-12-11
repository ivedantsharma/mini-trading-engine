import { useState, useEffect } from "react";
import TopOfBook from "./components/TopOfBook";
import OrderBookDepth from "./components/OrderBookDepth";
import PriceSpark from "./components/PriceSpark";
import CandlestickChart from "./components/CandlestickChart";
import TradesTape from "./components/TradesTape";
import OrderEntry from "./components/OrderEntry";
import PositionsPanel from "./components/PositionsPanel";
import { useMarketSocket } from "./hooks/useMarketSocket";
import CandleChart from "./components/CandleChart";

export default function App() {
  const [symbol, setSymbol] = useState("AAPL");

  // Hook now provides sendRaw()
  const { connected, latestForSymbol, sendRaw, buildCandles } =
    useMarketSocket();

  // Replay controls
  const {
    replayMode,
    setReplayMode,
    requestReplay,
    replaySpeed,
    setReplaySpeed,
    isReplayPlaying,
    setIsReplayPlaying,
  } = useMarketSocket();

  // Attach optional handlers to a global `ws` if something else exposes it
  useEffect(() => {
    const w = (window as unknown as { ws?: WebSocket }).ws;
    if (!w) return;
    w.onopen = () => console.log("CONNECTED TO API SERVER");
    w.onerror = (err: Event) => console.error("WS ERROR:", err);
  }, []);

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

          <div className="flex items-center gap-3">
            <select
              value={replayMode ? "REPLAY" : "LIVE"}
              onChange={(e) => setReplayMode(e.target.value === "REPLAY")}
              className="bg-gray-800 text-white px-2 py-1 rounded"
            >
              <option value="LIVE">LIVE</option>
              <option value="REPLAY">REPLAY</option>
            </select>

            {replayMode && (
              <>
                <button
                  className="bg-blue-600 px-2 py-1 rounded"
                  onClick={() => {
                    requestReplay(symbol, 0, Date.now() * 1e6);
                    setIsReplayPlaying(true);
                  }}
                >
                  Load Replay
                </button>

                <button
                  className="bg-green-600 px-2 py-1 rounded"
                  onClick={() => setIsReplayPlaying(!isReplayPlaying)}
                >
                  {isReplayPlaying ? "Pause" : "Play"}
                </button>

                <select
                  value={replaySpeed}
                  onChange={(e) => setReplaySpeed(Number(e.target.value))}
                  className="bg-gray-800 text-white px-2 py-1 rounded"
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
            <CandleChart symbol={symbol} trades={trades} />

            {/** Candles built from recent trades */}
            {(() => {
              const candles = buildCandles(trades);
              return <CandlestickChart candles={candles} />;
            })()}

            {/* Order Entry Form */}
            <OrderEntry currentSymbol={symbol} sendRaw={sendRaw} />
          </div>
        </div>

        {/* RIGHT SIDE — Trades */}
        <div>
          <TradesTape trades={trades} />
        </div>
      </div>
      <div className="mt-4">
        <PositionsPanel />
      </div>
    </div>
  );
}
