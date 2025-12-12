// dashboard/src/components/OrderEntry.tsx
import { useState, useEffect, useRef } from "react";

type Props = {
  currentSymbol: string;
  sendRaw: (s: string) => void;
};

export default function OrderEntry({ currentSymbol, sendRaw }: Props) {
  const [symbol, setSymbol] = useState(currentSymbol);
  const [side, setSide] = useState<"BUY" | "SELL">("BUY");
  const [type, setType] = useState<"LIMIT" | "MARKET">("LIMIT");
  const [price, setPrice] = useState<string>("100.00");
  const [qty, setQty] = useState<string>("1");

  // Refs to manually focus inputs when container is clicked
  const priceInputRef = useRef<HTMLInputElement>(null);
  const qtyInputRef = useRef<HTMLInputElement>(null);

  // Keep internal symbol in sync if prop changes (optional)
  useEffect(() => {
    setSymbol(currentSymbol);
  }, [currentSymbol]);

  function submitOrder() {
    const msg = {
      cmd: "NEW",
      order: {
        symbol,
        side,
        type,
        price: type === "LIMIT" ? Number(price) : 0,
        quantity: Number(qty),
      },
    };
    sendRaw(JSON.stringify(msg));
  }

  return (
    <div className="h-full flex flex-col gap-3">
      {/* Side Toggle */}
      <div className="flex bg-[#0b0e11] rounded p-1 border border-[#2b3139]">
        <button
          onClick={() => setSide("BUY")}
          className={`flex-1 py-1.5 text-xs font-bold rounded transition-colors ${
            side === "BUY"
              ? "bg-[#0ecb81] text-white"
              : "text-gray-500 hover:text-gray-300"
          }`}
        >
          BUY
        </button>
        <button
          onClick={() => setSide("SELL")}
          className={`flex-1 py-1.5 text-xs font-bold rounded transition-colors ${
            side === "SELL"
              ? "bg-[#f6465d] text-white"
              : "text-gray-500 hover:text-gray-300"
          }`}
        >
          SELL
        </button>
      </div>

      {/* Order Type Tabs */}
      <div className="flex gap-4 text-xs font-semibold text-gray-500 px-1">
        <button
          onClick={() => setType("LIMIT")}
          className={`pb-1 border-b-2 transition-colors ${
            type === "LIMIT"
              ? "text-[#fcd535] border-[#fcd535]"
              : "border-transparent hover:text-gray-300"
          }`}
        >
          Limit
        </button>
        <button
          onClick={() => setType("MARKET")}
          className={`pb-1 border-b-2 transition-colors ${
            type === "MARKET"
              ? "text-[#fcd535] border-[#fcd535]"
              : "border-transparent hover:text-gray-300"
          }`}
        >
          Market
        </button>
      </div>

      {/* Inputs */}
      <div className="flex flex-col gap-2">
        {/* Price Input */}
        <div
          className="bg-[#2b3139] rounded px-3 py-2 flex items-center justify-between group focus-within:ring-1 focus-within:ring-[#fcd535] cursor-text"
          onClick={() => priceInputRef.current?.focus()}
        >
          <label className="text-xs text-gray-400 font-mono pointer-events-none">
            Price
          </label>
          {type === "MARKET" ? (
            <span className="text-sm font-mono text-gray-500 select-none">
              MKT
            </span>
          ) : (
            <div className="flex items-center gap-2">
              <input
                ref={priceInputRef}
                className="bg-transparent text-right text-white font-mono text-sm outline-none w-20"
                value={price}
                onChange={(e) => setPrice(e.target.value)}
              />
              <span className="text-xs text-gray-500 pointer-events-none">
                USD
              </span>
            </div>
          )}
        </div>

        {/* Quantity Input */}
        <div
          className="bg-[#2b3139] rounded px-3 py-2 flex items-center justify-between group focus-within:ring-1 focus-within:ring-[#fcd535] cursor-text"
          onClick={() => qtyInputRef.current?.focus()}
        >
          <label className="text-xs text-gray-400 font-mono pointer-events-none">
            Size
          </label>
          <div className="flex items-center gap-2">
            <input
              ref={qtyInputRef}
              className="bg-transparent text-right text-white font-mono text-sm outline-none w-20"
              value={qty}
              onChange={(e) => setQty(e.target.value)}
            />
            <span className="text-xs text-gray-500 pointer-events-none">
              UNITS
            </span>
          </div>
        </div>

        {/* Total Estimate (Visual only) */}
        {type === "LIMIT" && (
          <div className="flex justify-between px-1">
            <span className="text-[10px] text-gray-500">Est. Total</span>
            <span className="text-[10px] text-gray-300 font-mono">
              {(Number(price) * Number(qty)).toFixed(2)} USD
            </span>
          </div>
        )}
      </div>

      {/* Submit Button */}
      <button
        onClick={submitOrder}
        className={`w-full py-3 mt-auto rounded font-bold text-sm text-white shadow-lg transition-transform active:scale-95 ${
          side === "BUY"
            ? "bg-[#0ecb81] hover:bg-[#0aa86b] shadow-[0_0_10px_rgba(14,203,129,0.2)]"
            : "bg-[#f6465d] hover:bg-[#d93448] shadow-[0_0_10px_rgba(246,70,93,0.2)]"
        }`}
      >
        {side === "BUY" ? "Buy / Long" : "Sell / Short"} {symbol}
      </button>
    </div>
  );
}
