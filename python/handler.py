import json
import http.server

class BaseHandler(http.server.BaseHTTPRequestHandler):
  def send_default_headers(self):
    self.send_header("X-Content-Type-Options", "nosniff")
    self.send_header("X-Frame-Options", "DENY")

  def write_json(self, data):
    self.wfile.write((json.dumps(data) + '\n').encode("utf-8"))

  def json_response(self, data, code=200):
    self.send_response(code)
    self.send_default_headers()
    self.send_header("Content-type", "application/json")
    self.end_headers()
    self.write_json(data)

  def do_OPTIONS(self):
    self.send_response(204)
    self.send_header("Allow", "OPTIONS, GET, POST")
    self.send_header("Access-Control-Allow-Headers", "Content-type")
    self.end_headers()

  def do_PUT(self):
    self.json_response({"error": "PUT not allowed"}, 405)

  def do_DELETE(self):
    self.json_response({"error": "DELETE not allowed"}, 405)
