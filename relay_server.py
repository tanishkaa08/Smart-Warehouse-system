import asyncio
import websockets
import json
import logging
import socket
import threading
import time

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WebSocketRelay:
    def __init__(self):
        self.html_client = None
        self.esp32_client = None
        
    async def handle_client(self, websocket, path=None):
        try:
            logger.info(f"New client connected from {websocket.remote_address}")
            try:
                logger.info(f"Client headers: {websocket.request.headers}")
                logger.info(f"Client path: {path}")
            except AttributeError:
                logger.info(f"Client path: {path} (headers not available)")
                
            await asyncio.sleep(0.1)
            client_type = None
            
            async for message in websocket:
                try:
                    data = json.loads(message)
                    logger.info(f"Received message: {data}")
                    
                    if data.get("type") == "esp32_connected":
                        logger.info("ESP32 client connected")
                        self.esp32_client = websocket
                        client_type = "esp32"
                        
                        if self.html_client:
                            await self.send_to_html({
                                "type": "esp32_status",
                                "connected": True,
                                "message": "ESP32 connected"
                            })
                            
                    elif data.get("type") == "ping":
                        logger.info("Received ping from client")
                        await websocket.send(json.dumps({"type": "pong"}))
                            
                    elif "row" in data and "col" in data:
                        logger.info(f"HTML client sent coordinates: {data}")
                        if client_type is None:
                            self.html_client = websocket
                            client_type = "html"
                        
                        if self.esp32_client:
                            await self.send_to_esp32(data)
                        else:
                            await self.send_to_html({
                                "type": "error",
                                "message": "ESP32 not connected"
                            })
                    
                    elif "result" in data and client_type == "esp32":
                        logger.info(f"Received result from ESP32: {data}")
                        if self.html_client:
                            await self.send_to_html({
                                "type": "movement_result",
                                "result": data["result"],
                                "message": "Movement completed successfully" if data["result"] else "Movement failed"
                            })
                            
                    else:
                        if client_type is None:
                            logger.info("HTML client connected")
                            self.html_client = websocket
                            client_type = "html"
                            
                            await self.send_to_html({
                                "type": "esp32_status",
                                "connected": self.esp32_client is not None,
                                "message": "ESP32 connected" if self.esp32_client else "ESP32 not connected"
                            })
                        
                except json.JSONDecodeError:
                    logger.error(f"Invalid JSON received: {message}")
                except Exception as e:
                    logger.error(f"Error handling message: {e}")
                    
        except websockets.exceptions.ConnectionClosed:
            logger.info("Client disconnected")
        except Exception as e:
            logger.error(f"Error in handle_client: {e}")
        finally:
            if websocket == self.html_client:
                self.html_client = None
                logger.info("HTML client disconnected")
            elif websocket == self.esp32_client:
                self.esp32_client = None
                logger.info("ESP32 client disconnected")
                
                if self.html_client:
                    await self.send_to_html({
                        "type": "esp32_status",
                        "connected": False,
                        "message": "ESP32 disconnected"
                    })
    
    async def handle_html_messages(self, websocket):
        try:
            async for message in websocket:
                try:
                    data = json.loads(message)
                    logger.info(f"Received from HTML: {data}")
                    
                    if "row" in data and "col" in data:
                        if self.esp32_client:
                            await self.send_to_esp32(data)
                        else:
                            await self.send_to_html({
                                "type": "error",
                                "message": "ESP32 not connected"
                            })
                            
                except json.JSONDecodeError:
                    logger.error(f"Invalid JSON from HTML: {message}")
                except Exception as e:
                    logger.error(f"Error handling HTML message: {e}")
                    
        except websockets.exceptions.ConnectionClosed:
            pass
    
    async def handle_esp32_messages(self, websocket):
        try:
            async for message in websocket:
                try:
                    data = json.loads(message)
                    logger.info(f"Received from ESP32: {data}")
                    
                    if "result" in data:
                        if self.html_client:
                            await self.send_to_html({
                                "type": "movement_result",
                                "result": data["result"],
                                "message": "Movement completed successfully" if data["result"] else "Movement failed"
                            })
                            
                except json.JSONDecodeError:
                    logger.error(f"Invalid JSON from ESP32: {message}")
                except Exception as e:
                    logger.error(f"Error handling ESP32 message: {e}")
                    
        except websockets.exceptions.ConnectionClosed:
            pass
    
    async def send_to_html(self, data):
        if self.html_client:
            try:
                await self.html_client.send(json.dumps(data))
                logger.info(f"Sent to HTML: {data}")
            except Exception as e:
                logger.error(f"Error sending to HTML: {e}")
    
    async def send_to_esp32(self, data):
        if self.esp32_client:
            try:
                await self.esp32_client.send(json.dumps(data))
                logger.info(f"Sent to ESP32: {data}")
            except Exception as e:
                logger.error(f"Error sending to ESP32: {e}")

async def main():
    relay = WebSocketRelay()
    
    logger.info("Starting WebSocket relay server...")
    logger.info("Server will listen on:")
    logger.info("- Port 8080: For both ESP32 and HTML clients")
    
    async def handler(websocket, path=None):
        try:
            logger.info(f"WebSocket connection attempt from {websocket.remote_address} on path: {path}")
            await relay.handle_client(websocket, path)
        except websockets.exceptions.InvalidHandshake as e:
            logger.error(f"Invalid WebSocket handshake: {e}")
        except websockets.exceptions.ConnectionClosedError as e:
            logger.info(f"Connection closed during handshake: {e}")
        except Exception as e:
            logger.error(f"Error in handler: {e}")
            import traceback
            traceback.print_exc()
            
    server = await websockets.serve(
        handler, 
        "0.0.0.0", 
        8080,
        ping_interval=None,
        ping_timeout=None,
        max_size=None,
        max_queue=None,
        close_timeout=10,
        subprotocols=None,
        open_timeout=10,
        compression=None
    )
    logger.info("WebSocket relay server started on port 8080")
    
    await server.wait_closed()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    except Exception as e:
        logger.error(f"Server error: {e}") 