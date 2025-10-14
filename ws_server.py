# pip install websockets
import asyncio
import websockets
from socket import gethostbyname_ex, gethostname

connections = set()
HostInfo = gethostbyname_ex(gethostname())

HOST = HostInfo[2][-1]
PORT = 8080


async def echo(websocket):
    connections.add(websocket)
    try:
        async for message in websocket:
            websockets.broadcast(connections, message)
            print(message)
    finally:
        connections.remove(websocket)


async def main():
    print(f"Ip address '{HOST}', port {PORT}")
    try:
        async with websockets.serve(echo, HOST, PORT):
            await asyncio.Future()  # run forever
    except:
        exit("\r\n\033[91mERRORE: Indirizzo ip o porta non validi...\033[0m")   # a capo, rosso


asyncio.run(main())
