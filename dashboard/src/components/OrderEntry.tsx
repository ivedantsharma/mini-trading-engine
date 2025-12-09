import React, { useState } from "react";

type Props = {
  currentSymbol: string;
  sendRaw: (s: string) => void;
};

export default function OrderEntry({ currentSymbol, sendRaw }: Props) {
  const [symbol, setSymbol] = useState(currentSymbol || "AAPL");
  const [side, setSide] = useState<"BUY" | "SELL">("BUY");
  const [type, setType] = useState<"LIMIT" | "MARKET">("LIMIT");
  const [price, setPrice] = useState<string>("100.00");
  const [qty, setQty] = useState<string>("1");

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

    console.log("Sending JSON:", msg);
    sendRaw(JSON.stringify(msg));
  }

  function cancelOrder() {
    const msg = {
      cmd: "CANCEL",
      orderId: Number(Math.floor(Math.random() * 1000000)),
    };

    console.log("Sending CANCEL:", msg);
    sendRaw(JSON.stringify(msg));
  }

  return (
    <div className="bg-gray-900 p-3 rounded border border-gray-800 space-y-2">
      <div className="text-sm text-gray-300">Order Entry</div>

      <div className="grid grid-cols-3 gap-2">
        <input
          className="bg-gray-800 p-1 rounded text-white"
          value={symbol}
          onChange={(e) => setSymbol(e.target.value)}
        />
        <select
          className="bg-gray-800 p-1 rounded text-white"
          value={side}
          onChange={(e) => setSide(e.target.value as any)}
        >
          <option>BUY</option>
          <option>SELL</option>
        </select>
        <select
          className="bg-gray-800 p-1 rounded text-white"
          value={type}
          onChange={(e) => setType(e.target.value as any)}
        >
          <option>LIMIT</option>
          <option>MARKET</option>
        </select>
      </div>

      <div className="grid grid-cols-3 gap-2">
        <input
          className="bg-gray-800 p-1 rounded text-white"
          value={price}
          onChange={(e) => setPrice(e.target.value)}
          disabled={type === "MARKET"}
        />
        <input
          className="bg-gray-800 p-1 rounded text-white"
          value={qty}
          onChange={(e) => setQty(e.target.value)}
        />
      </div>

      <div className="flex gap-2">
        <button
          className="bg-green-600 px-3 py-1 rounded text-white"
          onClick={submitOrder}
        >
          Send
        </button>
        <button
          className="bg-yellow-600 px-3 py-1 rounded text-white"
          onClick={cancelOrder}
        >
          Cancel
        </button>
      </div>
    </div>
  );
}
