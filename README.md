# Mini Trading Engine (C++17 + React + WebSockets + SQLite)

A fully functional, low-latency matching engine, market-data publisher, WebSocket API, REST API, and trading dashboard built from scratch.  
Designed as a learning-oriented HFT-style system with real-time charts, order entry, candles, positions tracking, and latency instrumentation.

---

## Features

### Matching Engine (C++17)

- Price-time priority order matching
- Bid/Ask depth book using `std::map`
- Market + Limit orders
- Trade generation with global trade IDs
- Top-of-book snapshot + incremental updates

### WebSocket API (Boost.Beast)

Publishes:

- Trades
- Top of Book (bestBid / bestAsk + depth)
- Positions
- Latency metadata (`sendTs`)

Accepts:

- NEW orders
- CANCEL orders
- REPLAY (historical trades streamed back through WS)

### REST API (C++ httplib)

Endpoints:

- GET /trades?symbol=AAPL&limit=50
- GET /candles?symbol=AAPL&tf=60&limit=200
- GET /positions

### SQLite Persistence

- All trades stored
- Candles auto-updated in 1s and 60s timeframe
- Candle update latency tracked via `updated_ts - start_ts`

### Real-Time Dashboard (React + Vite + Tailwind)

- Live top of book
- Live order book depth
- Trades tape
- Price sparkline
- Candlestick chart (Lightweight Charts)
- Replay mode with speed control
- Positions panel
- Order entry UI

### Latency Instrumentation

- Order → Trade latency measurement
- Server → Client WS latency measurement
- Candle update latency

### Additional Capabilities

- Multi-symbol support
- Automatic WebSocket reconnection
- Clean, modular C++ engine + API architecture
- Safe JSON parsing and TS-typed WebSocket handler

## Architecture Diagram

        ┌─────────────────────────────┐
        │        React Dashboard       │
        │  - Order Entry               │
        │  - Charts (Candles + Spark)  │
        │  - Trades Tape               │
        │  - Positions Panel           │
        └───────────────┬─────────────┘
                        │ WebSocket (9001)
                        ▼
             ┌─────────────────────────────┐
             │       API Server (C++)       │
             │  NEW / CANCEL / REPLAY       │
             │  Broadcasts market data       │
             └───────────────┬─────────────┘
                             │
                 Trades / MD │  REST (9003)
                             ▼
             ┌──────────────────────────────┐
             │         Rest Server           │
             │ /trades /candles /positions  │
             └──────────────────────────────┘
                             │
                             ▼
     ┌──────────────────────────────────────────┐
     │             SQLite Database               │
     │  Trades, Candles, Positions (in-memory)   │
     └──────────────────────────────────────────┘
                             │
                             ▼
             ┌─────────────────────────────┐
             │      Matching Engine         │
             │  - OrderBook per symbol      │
             │  - Trade Matching             │
             │  - Top-of-book updates       │
             └─────────────────────────────┘

---

## Build & Run

### 1. Build the Entire Project (Engine + API)

From the project root:

```bash
mkdir -p build
cd build
cmake ..
make -j8
```

### 2. Start the Trading Engine

```bash
cd build
./engine/engine_runner
```

### 3. Start the API Server (WebSocket + REST)

In a new terminal:

```bash
cd build
./api/api_server
```

### 4. Start the Dashboard (React)

```bash
cd dashboard
npm install
npm run dev
```

## WebSocket Messages

### Incoming (Client → Engine)

```bash
{
    "cmd": "NEW",
    "order": {
        "symbol": "AAPL",
        "side": "BUY",
        "type": "LIMIT",
        "price": 188.50,
        "quantity": 10
    }
}
```

### Outgoing (Engine → Client)

```bash
Example market data packet:
{
    "type": "top",
    "symbol": "AAPL",
    "bestBid": 188.50,
    "bestAsk": 188.55,
    "timestamp": 122334455667,
    "sendTs": 122334455900
}
```

### Latency for every WS update:

```bash
latencyNs = clientReceiveTs - sendTs
```

## Replay Mode

### Dashboard can request historical trades:

```bash
{
"cmd": "REPLAY",
"symbol": "AAPL",
"from": 0,
"to": 999999999999999
}
```

### Engine returns:

```bash
{
"type": "replayData",
"symbol": "AAPL",
"trades": [...]
}
```

## Latency Telemetry

### 1. Order → Trade latency

Included in each trade JSON:

```bash
"latencyNs": tradeTs - orderTs
```

### 2. Market Data latency

Every packet includes:

```bash
"sendTs"
```

### Client measures:

```bash clientLatencyNs = now() - sendTs

```

### 3. Candle Update latency

REST returns:

```bash
updateLatencyNs = updated_ts - start_ts
```

## Testing Checklist

### Matching Engine

- Limit vs Market orders
- Partial fills
- Multi-level depth
- Cancels update top of book

### API Server

- NEW / CANCEL working
- Replay working
- WS broadcast stable

### Dashboard

- Candles update live
- Depth + sparkline
- Positions accumulate correctly
