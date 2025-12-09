import {
  ResponsiveContainer,
  ComposedChart,
  Area,
  XAxis,
  YAxis,
  Tooltip,
  Line,
} from "recharts";

type Candle = {
  bucket: number;
  open: number;
  high: number;
  low: number;
  close: number;
};

export default function CandlestickChart({ candles }: { candles: Candle[] }) {
  return (
    <div className="bg-gray-900 p-3 rounded border border-gray-800 h-48">
      <div className="text-sm text-gray-300 mb-1">Price Chart</div>

      <ResponsiveContainer width="100%" height="100%">
        <ComposedChart data={candles}>
          <XAxis dataKey="bucket" hide />
          <YAxis hide />
          <Tooltip />
          <Area
            type="monotone"
            dataKey="close"
            fill="#3b82f6"
            fillOpacity={0.2}
            stroke="#3b82f6"
          />
          <Line type="monotone" dataKey="close" stroke="#fff" dot={false} />
        </ComposedChart>
      </ResponsiveContainer>
    </div>
  );
}
