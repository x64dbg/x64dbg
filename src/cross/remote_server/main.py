import asyncio
import websockets
import json
import os
import secrets
import time
from typing import TypedDict

# Use a token from the environment, or generate a cryptographically secure one at startup.
# The token is printed to stdout so the legitimate local user can authenticate.
AUTH_TOKEN = os.environ.get("X64DBG_REMOTE_TOKEN") or secrets.token_hex(32)

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
    # Each connection must authenticate before executing any RPC methods.
    authenticated = False

    async for message in websocket:
        try:
            request = json.loads(message)
            if request.get("jsonrpc") != "2.0":
                print(f"[server] Invalid magic: {request.get('jsonrpc')}")
                continue

            method = request.get("method")
            req_id = request.get("id")
            params = request.get("params", {})

            # No authentication check before accepting commands from remote client
            # Authentication gate: only the "auth" method is allowed before authentication.
            if not authenticated:
                if method == "auth" and params.get("token") == AUTH_TOKEN:
                    authenticated = True
                    await websocket.send(json.dumps({
                        "jsonrpc": "2.0",
                        "id": req_id,
                        "result": {"authenticated": True}
                    }))
                else:
                    print("[server] Rejected unauthenticated request")
                    await websocket.send(json.dumps({
                        "jsonrpc": "2.0",
                        "id": req_id,
                        "error": {"code": -32001, "message": "Authentication required"}
                    }))
                continue

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
    print(f"[server] Auth token: {AUTH_TOKEN}")
    print("[server] Listening on ws://127.0.0.1:42069")
    async with websockets.serve(handler, "127.0.0.1", 42069):
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())
