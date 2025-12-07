# Mini Trading Engine

A C++ based low-latency trading engine with:

- Matching Engine (C++)
- WebSocket API (C++)
- Tick Replay (C++)
- Real-time Dashboard (React, dark theme)
- Market Data Ticker

## Tech Stack

### Backend

- C++20
- Custom OrderBook
- Multi-threaded matching
- WebSocket server
- Tick replay tool

### Frontend

- React + Vite
- TailwindCSS + DaisyUI (Dark)
- WebSockets
- Live charts with Recharts

## Modules

mini-trading-engine/
├── engine/ # Matching engine
├── api/ # WebSocket server
├── replay/ # tick replay
├── dashboard/ # frontend

# From project root
mkdir build
cd build
cmake ..
make
