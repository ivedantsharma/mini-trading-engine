// dashboard/src/components/CandleChart.tsx
import { useEffect, useRef, useState } from "react";
import { createChart } from "lightweight-charts";
import type { ISeriesApi, UTCTimestamp } from "lightweight-charts";
import type { TradeMsg } from "../hooks/useMarketSocket";

type Props = {
  symbol: string;
  trades: TradeMsg[];
};

type Candle = {
  time: UTCTimestamp;
  open: number;
  high: number;
  low: number;
  close: number;
};

export default function CandleChart({ symbol, trades }: Props) {
  const chartContainerRef = useRef<HTMLDivElement | null>(null);
  const seriesRef = useRef<ISeriesApi<"Candlestick"> | null>(null);
  const chartApiRef = useRef<ReturnType<typeof createChart> | null>(null);
  const [candles, setCandles] = useState<Candle[]>([]);

  // 1. Data Loading (Unchanged)
  useEffect(() => {
    let cancelled = false;
    async function load() {
      try {
        const res = await fetch(
          `http://localhost:9003/candles?symbol=${encodeURIComponent(
            symbol
          )}&tf=60&limit=200`
        );
        if (!res.ok) return;
        const data = await res.json();

        type ApiCandle = {
          start_ts: number | string;
          open: number | string;
          high: number | string;
          low: number | string;
          close: number | string;
        };

        const formatted: Candle[] = (data || [])
          .map((c: ApiCandle) => {
            const timeSec = Math.floor(Number(c.start_ts) / 1e9);
            return {
              time: timeSec as UTCTimestamp,
              open: Number(c.open),
              high: Number(c.high),
              low: Number(c.low),
              close: Number(c.close),
            };
          })
          .reverse();

        if (cancelled) return;
        setCandles(formatted);
        if (seriesRef.current && formatted.length) {
          seriesRef.current.setData(formatted);
        }
      } catch (err) {
        console.error("Failed to load candles", err);
      }
    }
    load();
    return () => {
      cancelled = true;
    };
  }, [symbol]);

  // 2. Initialize Chart with Dynamic Sizing
  useEffect(() => {
    if (!chartContainerRef.current) return;

    // Get the exact dimensions of the parent container
    const { clientWidth, clientHeight } = chartContainerRef.current;

    const chart = createChart(chartContainerRef.current, {
      width: clientWidth,
      height: clientHeight, // Use dynamic height
      layout: {
        background: { color: "#0b0f19" },
        textColor: "#cbd5e1",
      },
      grid: {
        vertLines: { color: "#1f2937" },
        horzLines: { color: "#1f2937" },
      },
      timeScale: {
        borderColor: "#374151",
      },
    });

    const candleSeries = chart.addCandlestickSeries({
      upColor: "#16a34a",
      downColor: "#ef4444",
      borderVisible: false,
      wickUpColor: "#16a34a",
      wickDownColor: "#ef4444",
    });

    seriesRef.current = candleSeries;
    chartApiRef.current = chart;

    if (candles.length) candleSeries.setData(candles);

    // 3. Resize Observer for responsiveness
    // This handles window resizing AND container resizing automatically
    const handleResize = () => {
      if (chartContainerRef.current && chartApiRef.current) {
        chartApiRef.current.applyOptions({
          width: chartContainerRef.current.clientWidth,
          height: chartContainerRef.current.clientHeight,
        });
      }
    };

    const resizeObserver = new ResizeObserver(() => handleResize());
    resizeObserver.observe(chartContainerRef.current);

    return () => {
      resizeObserver.disconnect();
      try {
        chartApiRef.current?.remove();
      } catch {
        // ignore
      }
      chartApiRef.current = null;
      seriesRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // 4. Live Updates (Unchanged)
  useEffect(() => {
    if (!trades.length || !seriesRef.current) return;
    const lastTrade = trades[trades.length - 1];
    if (!lastTrade.ts) return;

    const tradeSec = Math.floor(Number(lastTrade.ts) / 1e9);
    const bucketStart = Math.floor(tradeSec / 60) * 60;

    setCandles((prev) => {
      const prevLast = prev.length ? prev[prev.length - 1] : undefined;

      if (!prevLast || prevLast.time < (bucketStart as UTCTimestamp)) {
        const newCandle: Candle = {
          time: bucketStart as UTCTimestamp,
          open: lastTrade.price,
          high: lastTrade.price,
          low: lastTrade.price,
          close: lastTrade.price,
        };
        seriesRef.current!.update(newCandle);
        return [...prev, newCandle];
      } else {
        const updated: Candle = {
          time: prevLast.time,
          open: prevLast.open,
          high: Math.max(prevLast.high, lastTrade.price),
          low: Math.min(prevLast.low, lastTrade.price),
          close: lastTrade.price,
        };
        seriesRef.current!.update(updated);
        const clone = [...prev];
        clone[clone.length - 1] = updated;
        return clone;
      }
    });
  }, [trades]);

  // CSS: remove hardcoded style, use w-full h-full
  return <div ref={chartContainerRef} className="w-full h-full" />;
}
