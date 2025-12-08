import React from "react";
import { LineChart, Line, ResponsiveContainer } from "recharts";
import type { TradeMsg } from "../hooks/useMarketSocket";

type Props = {
  trades: TradeMsg[];
};

export default function PriceSpark({ trades }: Props) {
  const data = trades.slice(-40).map((t, i) => ({ i, price: t.price }));
  return (
    <div className="bg-gray-900 p-2 rounded border border-gray-800">
      <ResponsiveContainer width="100%" height={60}>
        <LineChart data={data}>
          <Line
            type="monotone"
            dataKey="price"
            stroke="#10b981"
            dot={false}
            strokeWidth={2}
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
