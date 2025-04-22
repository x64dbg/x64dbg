import asyncio
import websockets
import json
import time
from typing import TypedDict

rpc_methods = {}

def jsonrpc(func):
    rpc_methods[func.__name__] = func
    return func

class TableParams(TypedDict):
    lines: int
    offset: int

class TableResult(TypedDict):
    rows: list[list[str]]

@jsonrpc
async def table(params: TableParams) -> TableResult:
    lines = params.get("lines", 0)
    offset = params.get("offset", 0)
    rows = []

    for line in range(lines):
        rows.append([
            f"address: {line + offset}",
            f"data: {line + offset}"
        ])

    start = time.time()
    while (time.time() - start) * 1000 < 500:
        await asyncio.sleep(0.01)

    return {"rows": rows}

async def handler(websocket):
    async for message in websocket:
        try:
            request = json.loads(message)
            if request.get("jsonrpc") != "2.0":
                print(f"[server] Invalid magic: {request.get('jsonrpc')}")
                continue

            method = request.get("method")
            req_id = request.get("id")
            params = request.get("params", {})

            response = {
                "jsonrpc": "2.0",
                "id": req_id
            }

            if method in rpc_methods:
                result = await rpc_methods[method](params)
                response["result"] = result
            else:
                print(f"[server] unknown method: {method}")
                continue

            await websocket.send(json.dumps(response))

        except Exception as e:
            print(f"[server] JSON error: {e}")

async def main():
    print("[server] Listening on ws://127.0.0.1:42069")
    async with websockets.serve(handler, "127.0.0.1", 42069):
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())
