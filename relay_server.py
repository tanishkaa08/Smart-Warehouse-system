import http.server
import socketserver
import urllib.parse
import json
import requests

# === CHANGE THIS TO YOUR ESP32's IP ===
ESP32_IP = '192.168.1.100'
ESP32_URL = f'http://{ESP32_IP}/position'

class RelayHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/send'):
            self.handle_position_request()
        else:
            super().do_GET()

    def handle_position_request(self):
        query = urllib.parse.urlparse(self.path).query
        params = urllib.parse.parse_qs(query)
        row = params.get('row', [None])[0]
        col = params.get('col', [None])[0]
        if not row or not col:
            self.send_error(400, 'Missing row or column')
            return
        try:
            # Forward to ESP32 as JSON POST
            data = {'row': int(row), 'col': int(col)}
            resp = requests.post(ESP32_URL, json=data, timeout=3)
            print(f"Forwarded to ESP32: {data}, ESP32 responded: {resp.text}")
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Position sent successfully to ESP32')
        except Exception as e:
            print(f"Error forwarding to ESP32: {e}")
            self.send_error(500, f'Failed to send to ESP32: {e}')

if __name__ == '__main__':
    PORT = 8000
    print(f"Relay server running on http://localhost:{PORT}")
    print(f"Will forward to ESP32 at {ESP32_URL}")
    with socketserver.TCPServer(("", PORT), RelayHandler) as httpd:
        httpd.serve_forever()
