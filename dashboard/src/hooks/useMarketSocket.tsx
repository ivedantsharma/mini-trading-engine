// src/hooks/useMarketSocket.tsx
import { useEffect, useRef, useState } from "react";

export type TopMsg = {
  type: "top";
  symbol: string;
  bestBid: number | null;
  bestAsk: number | null;
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

export function useMarketSocket(url = "ws://localhost:9002") {
  const wsRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);
  const [messages, setMessages] = useState<MDMsg[]>([]);

  useEffect(() => {
    const ws = new WebSocket(url);
    wsRef.current = ws;
    ws.onopen = () => setConnected(true);
    ws.onclose = () => setConnected(false);
    ws.onerror = () => setConnected(false);
    ws.onmessage = (e) => {
      try {
        const parsed = JSON.parse(e.data);
        // normalize numbers
        if (parsed.type === "top") {
          const top: TopMsg = {
            type: "top",
            symbol: parsed.symbol,
            bestBid: parsed.bestBid === null ? null : Number(parsed.bestBid),
            bestAsk: parsed.bestAsk === null ? null : Number(parsed.bestAsk),
            ts: parsed.ts ? Number(parsed.ts) : undefined,
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
        } else {
          // ignore unknown
        }
      } catch (err) {
        // ignore parse errors
        // console.warn("md parse err", err, e.data);
      }
    };
    return () => {
      if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) wsRef.current.close();
      wsRef.current = null;
    };
  }, [url]);

  // convenience: grouped latest state per symbol
  function latestForSymbol(symbol: string) {
    // iterate backward to find latest top and trades
    let top: TopMsg | undefined;
    const trades: TradeMsg[] = [];
    for (let i = messages.length - 1; i >= 0; --i) {
      const msg = messages[i];
      if (msg.symbol !== symbol) continue;
      if (msg.type === "top" && !top) top = msg;
      if (msg.type === "trade") trades.push(msg);
      if (top && trades.length >= 50) break;
    }
    return { top, trades: trades.reverse() };
  }

  return { connected, messages, latestForSymbol };
}
