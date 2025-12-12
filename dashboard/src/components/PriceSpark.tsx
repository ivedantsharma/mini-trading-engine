import { LineChart, Line, ResponsiveContainer, YAxis } from "recharts";
import type { TradeMsg } from "../hooks/useMarketSocket";

type Props = {
  trades: TradeMsg[];
};

export default function PriceSpark({ trades }: Props) {
  // Take last 50 trades for a meaningful micro-trend
  const data = trades.slice(-50).map((t, i) => ({ i, price: t.price }));

  // Calculate domain to auto-scale the sparkline
  const minPrice = Math.min(...data.map((d) => d.price));
  const maxPrice = Math.max(...data.map((d) => d.price));

  return (
    <div className="w-full h-full select-none" style={{ minHeight: 0 }}>
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data}>
          {/* Hide Axis but use domain to scale the line to fit the box */}
          <YAxis domain={[minPrice, maxPrice]} hide />
          <Line
            type="monotone"
            dataKey="price"
            stroke="#fcd535" // HFT Yellow
            strokeWidth={1.5}
            dot={false}
            isAnimationActive={false} // Disable animation for performance in high freq
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
