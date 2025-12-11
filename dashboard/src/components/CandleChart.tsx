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
  const chartRef = useRef<HTMLDivElement | null>(null);
  const seriesRef = useRef<ISeriesApi<"Candlestick"> | null>(null);
  const chartApiRef = useRef<ReturnType<typeof createChart> | null>(null);
  const [candles, setCandles] = useState<Candle[]>([]);

  // Load initial candles from REST
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

        // API returns candles ordered newest->oldest; we convert & reverse
        type ApiCandle = {
          start_ts: number | string;
          open: number | string;
          high: number | string;
          low: number | string;
          close: number | string;
        };

        const formatted: Candle[] = (data || [])
          .map((c: ApiCandle) => {
            // start_ts is stored in nanoseconds in your DB; convert to seconds
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

  // Initialize chart once
  useEffect(() => {
    if (!chartRef.current) return;

    // Create chart
    const chart = createChart(chartRef.current, {
      width: chartRef.current.clientWidth,
      height: 320,
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

    // Save reference
    seriesRef.current = candleSeries;
    chartApiRef.current = chart;

    // Load existing data if present
    if (candles.length) candleSeries.setData(candles);

    const handleResize = () => {
      if (!chartRef.current || !chartApiRef.current) return;
      chartApiRef.current.applyOptions({ width: chartRef.current.clientWidth });
    };

    window.addEventListener("resize", handleResize);
    return () => {
      window.removeEventListener("resize", handleResize);
      try {
        chartApiRef.current?.remove();
      } catch {
        // ignore
      }
      chartApiRef.current = null;
      seriesRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []); // run only once

  // Live updating from trade stream (bucket = 60-sec)
  useEffect(() => {
    if (!trades.length || !seriesRef.current) return;

    const lastTrade = trades[trades.length - 1];

    // guard: require timestamp to update candles
    if (!lastTrade.ts) return;

    // lastTrade.ts in your system is nanoseconds; convert to seconds
    const tradeSec = Math.floor(Number(lastTrade.ts) / 1e9);
    const bucketStart = Math.floor(tradeSec / 60) * 60; // 60-second TF

    setCandles((prev) => {
      const prevLast = prev.length ? prev[prev.length - 1] : undefined;

      if (!prevLast || prevLast.time < (bucketStart as UTCTimestamp)) {
        // new candle
        const newCandle: Candle = {
          time: bucketStart as UTCTimestamp,
          open: lastTrade.price,
          high: lastTrade.price,
          low: lastTrade.price,
          close: lastTrade.price,
        };

        // update chart
        seriesRef.current!.update(newCandle);
        return [...prev, newCandle];
      } else {
        // update existing candle
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
    // only when trades or seriesRef change
  }, [trades]);

  return <div ref={chartRef} className="w-full" style={{ minHeight: 320 }} />;
}
