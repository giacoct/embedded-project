# Requirement: pip install websockets
import asyncio
import websockets

ip = "127.0.0.1"
port = 81

async def send_no_web(websocket):
    await asyncio.sleep(10)
    try:
        await websocket.send("#no-web#")
        print("Inviato: NO_WEB")
    except:
        print("Client disconnesso prima dell'invio di NO_WEB.")

async def echo(websocket):
    # Avvio del timer per inviare "NO_WEB" dopo 10 secondi
    asyncio.create_task(send_no_web(websocket))

    async for message in websocket:
        print(f"Received: {message}")
        await websocket.send(f"Echo: {message}")

async def main():
    async with websockets.serve(echo, ip, port):
        print(f"Server running on ws://{ip}:{port}")
        try:
            await asyncio.Future()  # Run forever
        except asyncio.CancelledError:
            print("\nServer shutting down... ", end="")

try:
    asyncio.run(main())
except KeyboardInterrupt:
    print("Server stopped.")
