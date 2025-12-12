// dashboard/src/hooks/useMarketSocket.tsx
import { useEffect, useRef, useState } from "react";

export type TopMsg = {
  type: "top";
  symbol: string;
  bestBid: number | null;
  bestAsk: number | null;
  bids?: { price: number; qty: number }[];
  asks?: { price: number; qty: number }[];
  ts?: number;
};

export type TradeMsg = {
  type: "trade";
  symbol: string;
  tradeId: number;
  price: number;
  quantity: number;
  buyOrderId?: number;
  sellOrderId?: number;
  ts?: number;
};

export type PositionsMsg = {
  type: "positions";
  positions: {
    symbol: string;
    qty: number;
    avgPrice: number;
    realizedPnl: number;
  }[];
};

export type MDMsg = TopMsg | TradeMsg | PositionsMsg;

const WS_URL = "ws://localhost:9001";

/* RawTrade - shape coming from backend / DB for replay/results.
   We use this to avoid `any` in map() callbacks.
*/
type RawTrade = {
  symbol: string;
  tradeId: number;
  price: number;
  quantity?: number;
  qty?: number;
  buyOrderId?: number;
  sellOrderId?: number;
  timestamp?: number;
  ts?: number;
};

export function useMarketSocket(url = WS_URL) {
  const wsRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);
  const [messages, setMessages] = useState<MDMsg[]>([]);

  // Replay state
  const [replayMode, setReplayMode] = useState(false);
  const [replayBuffer, setReplayBuffer] = useState<TradeMsg[]>([]);
  const [isReplayPlaying, setIsReplayPlaying] = useState(false);
  const [replaySpeed, setReplaySpeed] = useState(1);

  useEffect(() => {
    let ws: WebSocket | null = null;
    let reconnectTimer: ReturnType<typeof setTimeout>;

    function connect() {
      ws = new WebSocket(url);
      wsRef.current = ws;

      ws.onopen = () => {
        setConnected(true);
        console.log("[WS] Connected to", url);
      };

      ws.onclose = () => {
        setConnected(false);
        console.log("[WS] Disconnected. Reconnecting...");
        reconnectTimer = setTimeout(connect, 1500);
      };

      ws.onerror = () => {
        setConnected(false);
        console.warn("[WS] Error.");
      };

      ws.onmessage = (e) => {
        // parse safely (we still trust backend shape but avoid `any` in maps)
        let parsed: unknown;
        try {
          parsed = JSON.parse(e.data);
        } catch {
          console.warn("[WS] Received non-JSON message");
          return;
        }

        // Basic guard: parsed must be object with a `type` property
        if (
          typeof parsed !== "object" ||
          parsed === null ||
          !("type" in (parsed as object))
        ) {
          return;
        }

        const p = parsed as Record<string, unknown>;
        const t = p.type as string;

        if (t === "top") {
          const top: TopMsg = {
            type: "top",
            symbol: String(p.symbol || ""),
            bestBid: p.bestBid == null ? null : Number(p.bestBid),
            bestAsk: p.bestAsk == null ? null : Number(p.bestAsk),
            bids: Array.isArray(p.bids)
              ? (p.bids as unknown[]).map((b) => {
                  const item = b as Record<string, unknown>;
                  return {
                    price: Number(item.price),
                    qty: Number(item.qty),
                  };
                })
              : undefined,
            asks: Array.isArray(p.asks)
              ? (p.asks as unknown[]).map((a) => {
                  const item = a as Record<string, unknown>;
                  return {
                    price: Number(item.price),
                    qty: Number(item.qty),
                  };
                })
              : undefined,
            ts: p.ts ? Number(p.ts) : undefined,
          };
          setMessages((m) => [...m.slice(-499), top]);
          return;
        } else if (t === "trade") {
          // parse trade message
          const tr: TradeMsg = {
            type: "trade",
            symbol: String(p.symbol || ""),
            tradeId: Number(p.tradeId),
            price: Number(p.price),
            quantity: Number(p.quantity ?? p.qty ?? 0),
            buyOrderId: p.buyOrderId == null ? undefined : Number(p.buyOrderId),
            sellOrderId:
              p.sellOrderId == null ? undefined : Number(p.sellOrderId),
            ts: p.timestamp
              ? Number(p.timestamp)
              : p.ts
              ? Number(p.ts)
              : undefined,
          };
          setMessages((m) => [...m.slice(-499), tr]);
          return;
        } else if (t === "positions") {
          // positions update
          const rawPositions = Array.isArray(p.positions)
            ? (p.positions as unknown[])
            : [];
          const posMsg: PositionsMsg = {
            type: "positions",
            positions: rawPositions.map((item) => {
              const pp = item as Record<string, unknown>;
              return {
                symbol: String(pp.symbol || ""),
                qty: Number(pp.qty || 0),
                avgPrice: Number(pp.avgPrice || 0),
                realizedPnl: Number(pp.realizedPnl || 0),
              };
            }),
          };
          setMessages((m) => [...m.slice(-499), posMsg]);
          return;
        } else if (t === "replayData") {
          // parsed.trades expected to be array of trade rows; map safely to TradeMsg
          const tradesArray = Array.isArray(p.trades)
            ? (p.trades as unknown[])
            : [];
          const trades = tradesArray.map((item) => {
            const rt = item as RawTrade;
            const trade: TradeMsg = {
              type: "trade",
              symbol: String(rt.symbol || ""),
              tradeId: Number(rt.tradeId),
              price: Number(rt.price),
              quantity: Number(rt.quantity ?? rt.qty ?? 0),
              buyOrderId: rt.buyOrderId,
              sellOrderId: rt.sellOrderId,
              ts: rt.timestamp ?? rt.ts,
            };
            return trade;
          });
          setReplayBuffer(trades);
          return;
        }

        // unknown type — ignore
      };
    }

    connect();

    return () => {
      clearTimeout(reconnectTimer);
      if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN)
        wsRef.current.close();
      wsRef.current = null;
    };
  }, [url, replayMode]);

  // Replay playback effect
  useEffect(() => {
    if (!replayMode || !isReplayPlaying) return;

    let idx = 0;
    const interval = setInterval(() => {
      if (idx >= replayBuffer.length) {
        clearInterval(interval);
        return;
      }
      const tr = replayBuffer[idx++];
      setMessages((m) => [...m.slice(-499), tr]);
    }, 200 / Math.max(1, replaySpeed)); // speed control (avoid division by 0)

    return () => clearInterval(interval);
  }, [replayMode, isReplayPlaying, replayBuffer, replaySpeed]);

  // Sending raw command (NEW/CANCEL)
  function sendRaw(s: string) {
    if (!wsRef.current) {
      console.warn("WS missing:", s);
      return;
    }
    if (wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(s);
    } else {
      console.warn("WS not open, cannot send:", s);
    }
  }

  function requestReplay(symbol: string, from: number, to: number) {
    if (!wsRef.current) return;

    const msg = JSON.stringify({
      cmd: "REPLAY",
      symbol,
      from,
      to,
    });

    wsRef.current.send(msg);
  }

  function latestForSymbol(symbol: string) {
    let top: TopMsg | undefined;
    const trades: TradeMsg[] = [];

    for (let i = messages.length - 1; i >= 0; --i) {
      const m = messages[i];

      // skip positions messages (they don't have symbol)
      if (m.type === "positions") continue;

      // now narrowed: m is TopMsg | TradeMsg
      if ((m as TopMsg).symbol !== symbol) continue;

      if (m.type === "top" && !top) top = m as TopMsg;
      if (m.type === "trade") trades.push(m as TradeMsg);

      if (top && trades.length >= 50) break;
    }

    return { top, trades: trades.reverse() };
  }

  function buildCandles(trades: TradeMsg[], intervalMs = 2000) {
    if (!trades.length) return [];

    const buckets: Record<
      number,
      { open: number; high: number; low: number; close: number }
    > = {};

    for (const t of trades) {
      // convert nanoseconds -> milliseconds (backend is using steady_clock nanos)
      const tsMs = Math.floor((t.ts || 0) / 1_000_000);
      const bucket = Math.floor(tsMs / intervalMs);

      if (!buckets[bucket]) {
        buckets[bucket] = {
          open: t.price,
          high: t.price,
          low: t.price,
          close: t.price,
        };
      }

      const b = buckets[bucket];
      b.high = Math.max(b.high, t.price);
      b.low = Math.min(b.low, t.price);
      b.close = t.price;
    }

    return Object.entries(buckets).map(([k, v]) => ({
      bucket: Number(k),
      ...v,
    }));
  }

  return {
    connected,
    messages,
    latestForSymbol,
    sendRaw,
    buildCandles,
    replayMode,
    setReplayMode,
    replayBuffer,
    setReplayBuffer,
    isReplayPlaying,
    setIsReplayPlaying,
    replaySpeed,
    setReplaySpeed,
    requestReplay,
  };
}
