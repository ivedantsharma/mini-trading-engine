const { spawn } = require("child_process");
const WebSocket = require("ws"); // npm i ws

const ENGINE_BIN = "./build/engine/engine_runner";
const ENGINE_ARGS = []; // if you need args or config file, adjust

// Start WebSocket server
const wss = new WebSocket.Server({ port: 8080 });
wss.on("connection", (ws) => {
  console.log("WS client connected");
});

// Spawn engine process
const engine = spawn(ENGINE_BIN, ENGINE_ARGS, {
  stdio: ["pipe", "pipe", "pipe"],
});

engine.stdout.on("data", (data) => {
  // STDOUT has trade JSON, CLI prints — show for logs
  process.stdout.write(`[engine-stdout] ${data}`);
});

engine.stderr.setEncoding("utf8");
engine.stderr.on("data", (chunk) => {
  // engine.stderr may batch multiple lines — split on newline
  const lines = chunk.toString().split("\n").filter(Boolean);
  for (const line of lines) {
    // broadcast to all connected clients
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) client.send(line);
    });
    // also log locally
    console.log("[MD]", line);
  }
});

engine.on("exit", (code, sig) => {
  console.log("Engine exited", code, sig);
  process.exit(0);
});

// Optional: simple stdin forward from terminal to engine stdin
process.stdin.on("data", (data) => {
  engine.stdin.write(data);
});
