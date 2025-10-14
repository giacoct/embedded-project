# pip install websockets
import asyncio
import websockets
import sys


ADDRESS = "ws://192.168.1.12:81"

async def hello():
    async with websockets.connect(ADDRESS) as websocket:
        try:
            message = sys.argv[1]
            await websocket.send(message)
        except:
            await websocket.send("led:10")


asyncio.run(hello())
