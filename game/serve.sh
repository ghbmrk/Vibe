#!/usr/bin/env bash
# ============================================================
# serve.sh – Development server for mobile GBC testing
#
# Starts a local HTTP server on your LAN so you can open the
# test page on your phone.  Serves with CORS headers and
# cache-busting so recompiled ROMs load instantly on refresh.
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
    hostname -I 2>/dev/null | awk '{print $1}'
    return
  fi
  # macOS
  if command -v ipconfig >/dev/null 2>&1; then
    ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null
    return
  fi
  # Fallback
  echo "localhost"
}

IP="$(get_ip)"

echo ""
echo "  ╔════════════════════════════════════════════════╗"
echo "  ║   Creature Collector – Dev Server              ║"
echo "  ╠════════════════════════════════════════════════╣"
echo "  ║                                                ║"
echo "  ║  Local:   http://localhost:${PORT}/test/        "
echo "  ║  Phone:   http://${IP}:${PORT}/test/            "
echo "  ║                                                ║"
echo "  ║  Open the Phone URL on your mobile device      ║"
echo "  ║  (must be on the same WiFi network)            ║"
echo "  ║                                                ║"
echo "  ║  Workflow:                                     ║"
echo "  ║    1. make        (compile ROM)                ║"
echo "  ║    2. Tap ↻ on phone to reload                 ║"
echo "  ║                                                ║"
echo "  ║  Desktop controls:                             ║"
echo "  ║    WASD / Arrows = D-Pad                       ║"
echo "  ║    Z = A    X = B                              ║"
echo "  ║    Enter = Start   Shift = Select              ║"
echo "  ║                                                ║"
echo "  ╚════════════════════════════════════════════════╝"
echo ""
echo "  Press Ctrl+C to stop"
echo ""

# ---- Start Python HTTP server with CORS + no-cache --------
cd "$DIR"
python3 -c "
import http.server, socketserver, sys

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Allow cross-origin (needed for WASM/fetch)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, HEAD, OPTIONS')
        # Disable caching so ROM reloads pick up new builds
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

    def log_message(self, format, *args):
        # Tidy logging
        sys.stderr.write('  %s  %s\n' % (self.address_string(), format % args))

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

try:
    with socketserver.TCPServer(('', ${PORT}), Handler) as httpd:
        httpd.serve_forever()
except KeyboardInterrupt:
    print('\n  Server stopped.')
except OSError as e:
    if 'Address already in use' in str(e):
        print(f'  Port ${PORT} is busy. Try: ./serve.sh {int('${PORT}') + 1}')
    else:
        raise
"
