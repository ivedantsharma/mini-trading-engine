# Running the Mini Trading Engine

## Build Status

✅ All components successfully compiled:

- C++ Backend: `engine_runner`, `api_server`, `replay_tool`
- React Dashboard: Built with Vite

## Running the System

### 1. Start the Trading Engine

```bash
cd /home/vedant-sharma/Desktop/mini-trading-engine
./build/engine/engine_runner
```

The engine listens on stdin for commands:

- `NEW symbol side type price quantity` - Add an order
- `CANCEL orderId` - Cancel an order
- `REPLAY` - Prepare for replay mode

### 2. Start the API Server (WebSocket + REST)

```bash
cd /home/vedant-sharma/Desktop/mini-trading-engine
./build/api/api_server
```

- **WebSocket**: `ws://localhost:9001` - Real-time market data
- **REST API**: `http://localhost:9003`
  - `GET /trades?symbol=AAPL&limit=100` - Historical trades
  - `GET /candles?symbol=AAPL&tf=60&limit=200` - OHLC candles (tf in seconds)

### 3. Start the Dashboard

```bash
cd /home/vedant-sharma/Desktop/mini-trading-engine/dashboard
npm run dev
```

Open `http://localhost:5173` in your browser.

**Features**:

- **Live Mode**: Real-time market data via WebSocket
- **Replay Mode**: Select symbol, click "Load Replay", and play at adjustable speed (0.5x - 10x)
- **Visualization**: Top-of-book, order book depth, candles, price spark, trade tape

### 4. Optional: Replay Tool

```bash
cd /home/vedant-sharma/Desktop/mini-trading-engine
./build/replay/replay_tool [symbol] [start_timestamp_ns] [end_timestamp_ns]
```

## Database

All trades and orders are logged to `trading.db` (SQLite3):

```bash
sqlite3 trading.db
.schema
SELECT * FROM Trades LIMIT 10;
```

## Recent Fixes

- **CandleChart Component**: Fixed TypeScript error by using `addSeries(CandlestickSeries, {...})` instead of `addCandlestickSeries()`
- **Unused Imports**: Removed unused React imports from components
- **Dashboard Build**: Successfully compiled with Vite

## Architecture Overview

```
WebSocket Flow (Port 9001):
Engine → API Server → Connected Clients (Dashboard)
  (trades, order book updates)

REST Flow (Port 9003):
Dashboard → API Server → SQLite Database
  (historical candles, trades)

Database Schema:
- Orders: orderId, symbol, side, type, price, quantity, timestamp (ns)
- Trades: tradeId, symbol, price, quantity, buyOrderId, sellOrderId, timestamp (ns)
- Candles: symbol, tf (seconds), start_ts, open, high, low, close, volume
```
