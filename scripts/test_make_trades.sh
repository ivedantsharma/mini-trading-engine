#!/usr/bin/env bash
WS="ws://localhost:9001"
payload() {
  cat <<EOF
{"cmd":"NEW","order":{"symbol":"AAPL","side":"$1","type":"LIMIT","price":$2,"quantity":$3}}
EOF
}

# use websocat or wscat; here using websocat if installed
# websocat supports --text and single-shot send, else use wscat manually

if command -v websocat >/dev/null 2>&1; then
  echo Sending orders via websocat...
  payload BUY 100.50 10 | websocat -t $WS
  payload SELL 100.50 10 | websocat -t $WS
  payload BUY 100.75 5  | websocat -t $WS
  payload SELL 100.20 5 | websocat -t $WS
else
  echo "Install websocat or use wscat. Falling back to echo instructions."
  echo "Use wscat and paste the 4 JSON blocks to produce trades."
fi
