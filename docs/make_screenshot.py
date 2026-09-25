"""Render the firmware's web UI with simulated captures, for the README screenshot.

The page is taken verbatim from src/web_ui.cpp. Only fetch('/capture') is replaced
by a stub that returns what the ESP32 would send for a 250 Hz, 30 % duty PWM signal
seen through an RC low-pass (tau = 250 us), with ADC noise. No hardware is involved.

    python docs/make_screenshot.py        # needs Google Chrome or Chromium
"""

import re
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs" / "images" / "web_ui.png"

STUB = """<script>
window.fetch = async (url) => {
  const q = new URL(url, location.href).searchParams, rate = +q.get('rate'), level = +q.get('level');
  const N = 512, PRE = 128, f = 250, duty = 0.3, tau = 250e-6, vhi = 3.0;
  const raw = [], dt = 1 / rate; let v = 0;
  for (let i = 0; i < 4 * N; i++) {                      // simulate, then trigger like the firmware
    const on = ((i * dt * f) % 1) < duty;
    v += (1 - Math.exp(-dt / tau)) * ((on ? vhi : 0.05) - v);
    raw.push(Math.max(0, Math.min(4095, Math.round(v / 3.1 * 4095 + (Math.random() - 0.5) * 24))));
  }
  let t = raw.findIndex((s, i) => i > PRE + N && raw[i - 1] < level && s >= level);
  if (t < 0) t = PRE;
  return { json: async () => ({ rate, triggered: true, samples: raw.slice(t - PRE, t - PRE + N) }) };
};
</script>"""


def main() -> None:
    src = (ROOT / "src" / "web_ui.cpp").read_text()
    html = re.search(r'R"HTML\((.*)\)HTML"', src, re.S).group(1)
    html = html.replace("</head>", STUB + "</head>", 1)
    chrome = next(filter(None, map(shutil.which, ("google-chrome", "chromium", "chromium-browser"))))
    with tempfile.TemporaryDirectory() as tmp:
        page = Path(tmp) / "index.html"
        page.write_text(html)
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--virtual-time-budget=3000",
                        "--window-size=990,640", "--lang=en-US", f"--screenshot={OUT}", page.as_uri()], check=True, capture_output=True)


if __name__ == "__main__":
    OUT.parent.mkdir(exist_ok=True)
    main()
