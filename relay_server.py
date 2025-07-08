import asyncio
import websockets
import json
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WebSocketRelay:
    def __init__(self):
        self.html_client = None
        self.esp32_client = None
        
    async def handle_client(self, websocket, path=None):
        try:
            logger.info(f"New client connected from {websocket.remote_address}")
            
            async for message in websocket:
                try:
                    data = json.loads(message)
                    
                    if data.get("type") == "esp32_connected":
                        logger.info("ESP32 client connected")
                        self.esp32_client = websocket
                        
                        if self.html_client:
                            await self.send_to_html({
                                "type": "esp32_status",
                                "connected": True,
                                "message": "ESP32 connected"
                            })
                        
                        await self.handle_esp32_messages(websocket)
                        
                    elif "row" in data and "col" in data:
                        logger.info(f"HTML client sent coordinates: {data}")
                        self.html_client = websocket
                        
                        if self.esp32_client:
                            await self.send_to_esp32(data)
                            
                            await self.handle_html_messages(websocket)
                        else:
                            await self.send_to_html({
                                "type": "error",
                                "message": "ESP32 not connected"
                            })
                            
                    else:
                        logger.info("HTML client connected")
                        self.html_client = websocket
                        
                        await self.send_to_html({
                            "type": "esp32_status",
                            "connected": self.esp32_client is not None,
                            "message": "ESP32 connected" if self.esp32_client else "ESP32 not connected"
                        })
                        
                        await self.handle_html_messages(websocket)
                        
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
        await relay.handle_client(websocket, path)
    
    server = await websockets.serve(handler, "0.0.0.0", 8080)
    logger.info("WebSocket relay server started on port 8080")
    
    await server.wait_closed()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    except Exception as e:
        logger.error(f"Server error: {e}") 