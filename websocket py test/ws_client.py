# Requirement: pip install websockets
import asyncio
import websockets

uri = "ws://127.0.0.1:81"

async def main():
    async with websockets.connect(uri) as websocket:
        try:
            while True:
                message = input("Insert message: ")
                await websocket.send(message)
                response = await websocket.recv()
                print(response)

        except KeyboardInterrupt:
            await websocket.close()
            print("\n\nClient stopped.")
        
try:
    asyncio.run(main())

except KeyboardInterrupt:
    print("\nClient stopped.")
except ConnectionRefusedError:
    print("\nFailed to connect, server not available.")