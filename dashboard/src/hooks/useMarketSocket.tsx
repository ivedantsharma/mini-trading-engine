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

export type MDMsg = TopMsg | TradeMsg;

const WS_URL = "ws://localhost:9001";

export function useMarketSocket(url = WS_URL) {
  const wsRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);
  const [messages, setMessages] = useState<MDMsg[]>([]);

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
        try {
          const parsed = JSON.parse(e.data);

          if (parsed.type === "top") {
            const top: TopMsg = {
              type: "top",
              symbol: parsed.symbol,
              bestBid: parsed.bestBid === null ? null : Number(parsed.bestBid),
              bestAsk: parsed.bestAsk === null ? null : Number(parsed.bestAsk),
              ts: parsed.ts ?? parsed.timestamp,
            };
            setMessages((m) => [...m.slice(-499), top]);
          } else if (parsed.type === "trade") {
            const tr: TradeMsg = {
              type: "trade",
              symbol: parsed.symbol,
              tradeId: Number(parsed.tradeId),
              price: Number(parsed.price),
              quantity: Number(parsed.quantity ?? parsed.qty ?? 0),
              buyOrderId: parsed.buyOrderId ?? parsed.buyId,
              sellOrderId: parsed.sellOrderId ?? parsed.sellId,
              ts: parsed.timestamp ?? parsed.ts,
            };
            setMessages((m) => [...m.slice(-499), tr]);
          }
        } catch (err) {
          console.warn("[WS] Failed to parse message:", e.data);
        }
      };
    }

    connect();

    return () => {
      clearTimeout(reconnectTimer);
      if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN)
        wsRef.current.close();
      wsRef.current = null;
    };
  }, [url]);

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

  function latestForSymbol(symbol: string) {
    let top: TopMsg | undefined;
    const trades: TradeMsg[] = [];

    for (let i = messages.length - 1; i >= 0; --i) {
      const m = messages[i];
      if (m.symbol !== symbol) continue;

      if (m.type === "top" && !top) top = m;
      if (m.type === "trade") trades.push(m as TradeMsg);

      if (top && trades.length >= 50) break;
    }

    return { top, trades: trades.reverse() };
  }

  return { connected, messages, latestForSymbol, sendRaw };
}
