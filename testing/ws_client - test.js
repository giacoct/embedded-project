import WebSocket from "ws";

const ip = "127.0.0.1";
const port = 81;

const ws = new WebSocket(`ws://${ip}:${port}`);

ws.on("open", () => {
    console.log("Connesso al server.");

    const msg = "ciao server!";
    console.log("Invio:", msg);
    ws.send(msg);
});

ws.on("message", (data) => {
    console.log("Risposta:", data.toString());
});

// Rimane aperto
ws.on("close", () => {
    console.log("Connessione chiusa dal server.");
});

ws.on("error", (err) => {
    console.error("Errore:", err);
});
