#!/usr/bin/env bash
# ============================================================
# serve.sh – Development server for mobile GBC testing
#
# Starts a local HTTP server on your LAN so you can open the
# test page on your phone (same WiFi network).
#
# Usage:
#   ./serve.sh            (default port 8080)
#   ./serve.sh 9000       (custom port)
# ============================================================

set -euo pipefail

PORT="${1:-8080}"
DIR="$(cd "$(dirname "$0")" && pwd)"

# ---- Detect LAN IP ----------------------------------------
get_ip() {
  # Linux
  if command -v hostname >/dev/null 2>&1; then
    local ip
    ip=$(hostname -I 2>/dev/null | awk '{print $1}')
    if [ -n "$ip" ]; then echo "$ip"; return; fi
  fi
  # macOS
  if command -v ipconfig >/dev/null 2>&1; then
    ipconfig getifaddr en0 2>/dev/null && return
    ipconfig getifaddr en1 2>/dev/null && return
  fi
  echo "localhost"
}

IP="$(get_ip)"

cat <<BANNER

  ┌──────────────────────────────────────────────┐
  │  Creature Collector – Dev Server             │
  ├──────────────────────────────────────────────┤
  │                                              │
  │  Local:  http://localhost:${PORT}/test/
  │  Phone:  http://${IP}:${PORT}/test/
  │                                              │
  │  1. Open the Phone URL on your mobile        │
  │     (same WiFi network as this machine)      │
  │                                              │
  │  2. After recompiling (make), pull-to-       │
  │     refresh on your phone to reload          │
  │                                              │
  └──────────────────────────────────────────────┘

  Press Ctrl+C to stop

BANNER

# ---- Start Python HTTP server with CORS + no-cache --------
cd "$DIR"
exec python3 << PYEOF
import http.server, socketserver, sys

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, HEAD, OPTIONS')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

    def log_message(self, fmt, *args):
        sys.stderr.write('  %s  %s\n' % (self.address_string(), fmt % args))

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

port = ${PORT}
try:
    with socketserver.TCPServer(('', port), Handler) as httpd:
        httpd.serve_forever()
except KeyboardInterrupt:
    print('\n  Server stopped.')
except OSError as e:
    if 'Address already in use' in str(e):
        print('  Port %d is busy. Try: ./serve.sh %d' % (port, port + 1))
    else:
        raise
PYEOF
