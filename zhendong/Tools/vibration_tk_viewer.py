import json
import queue
import sys
import threading
import time
import tkinter as tk
from collections import deque
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports


BAUDRATE = 115200
MAX_POINTS = 180


class SerialReader(threading.Thread):
    def __init__(self, port, out_queue, stop_event):
        super().__init__(daemon=True)
        self.port = port
        self.out_queue = out_queue
        self.stop_event = stop_event
        self.ser = None

    def run(self):
        try:
            self.ser = serial.Serial(self.port, BAUDRATE, timeout=0.5)
            self.out_queue.put(("status", f"Connected: {self.port} @ {BAUDRATE}"))
        except Exception as exc:
            self.out_queue.put(("error", f"Open port failed: {exc}"))
            return

        while not self.stop_event.is_set():
            try:
                raw = self.ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue
                try:
                    msg = json.loads(line)
                    self.out_queue.put(("data", msg))
                except json.JSONDecodeError:
                    self.out_queue.put(("raw", line))
            except Exception as exc:
                self.out_queue.put(("error", f"Serial read error: {exc}"))
                break

        try:
            if self.ser:
                self.ser.close()
        finally:
            self.out_queue.put(("status", "Disconnected"))


class VibrationViewer(tk.Tk):
    def __init__(self, default_port=None):
        super().__init__()
        self.title("STM32 Vibration Monitor")
        self.geometry("980x660")
        self.minsize(860, 560)

        self.msg_queue = queue.Queue()
        self.stop_event = threading.Event()
        self.reader = None

        self.rms = deque(maxlen=MAX_POINTS)
        self.p2p = deque(maxlen=MAX_POINTS)
        self.score = deque(maxlen=MAX_POINTS)
        self.raw_lines = deque(maxlen=80)

        self.port_var = tk.StringVar(value=default_port or "")
        self.state_var = tk.StringVar(value="State: --")
        self.seq_var = tk.StringVar(value="Seq: --")
        self.score_var = tk.StringVar(value="Score: --")
        self.drop_var = tk.StringVar(value="Drop: --")
        self.status_var = tk.StringVar(value="Ready")

        self._build_ui()
        self.refresh_ports()
        if default_port:
            self.port_combo.set(default_port)
        self.after(50, self.poll_queue)

    def _build_ui(self):
        top = ttk.Frame(self, padding=10)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Port").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=18, state="readonly")
        self.port_combo.pack(side=tk.LEFT, padx=(6, 10))

        ttk.Button(top, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT, padx=4)
        self.connect_btn = ttk.Button(top, text="Connect", command=self.connect)
        self.connect_btn.pack(side=tk.LEFT, padx=4)
        self.disconnect_btn = ttk.Button(top, text="Disconnect", command=self.disconnect, state=tk.DISABLED)
        self.disconnect_btn.pack(side=tk.LEFT, padx=4)
        ttk.Button(top, text="Clear", command=self.clear_data).pack(side=tk.LEFT, padx=4)

        status = ttk.Frame(self, padding=(10, 0, 10, 8))
        status.pack(fill=tk.X)
        for var in (self.state_var, self.seq_var, self.score_var, self.drop_var):
            ttk.Label(status, textvariable=var, font=("Microsoft YaHei UI", 11, "bold")).pack(side=tk.LEFT, padx=(0, 22))

        body = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        body.pack(fill=tk.BOTH, expand=True, padx=10, pady=4)

        left = ttk.Frame(body)
        right = ttk.Frame(body)
        body.add(left, weight=4)
        body.add(right, weight=2)

        self.canvas = tk.Canvas(left, bg="white", highlightthickness=1, highlightbackground="#cccccc")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Configure>", lambda event: self.draw_plot())

        ttk.Label(right, text="Serial log").pack(anchor=tk.W)
        self.log = tk.Text(right, height=22, wrap=tk.NONE, font=("Consolas", 9))
        self.log.pack(fill=tk.BOTH, expand=True)

        bottom = ttk.Label(self, textvariable=self.status_var, anchor=tk.W, padding=(10, 6))
        bottom.pack(fill=tk.X)

    def refresh_ports(self):
        ports = list(list_ports.comports())
        values = [f"{p.device}  {p.description}" for p in ports]
        self.port_combo["values"] = values
        if values and not self.port_var.get():
            self.port_combo.current(0)
        if not values:
            self.status_var.set("No COM port found. Plug in USB-TTL or install its driver.")

    def selected_port(self):
        text = self.port_var.get().strip()
        if not text:
            return ""
        return text.split()[0]

    def connect(self):
        port = self.selected_port()
        if not port:
            messagebox.showwarning("No port", "No COM port selected.")
            return
        self.stop_event.clear()
        self.reader = SerialReader(port, self.msg_queue, self.stop_event)
        self.reader.start()
        self.connect_btn.configure(state=tk.DISABLED)
        self.disconnect_btn.configure(state=tk.NORMAL)
        self.status_var.set(f"Opening {port} ...")

    def disconnect(self):
        self.stop_event.set()
        self.connect_btn.configure(state=tk.NORMAL)
        self.disconnect_btn.configure(state=tk.DISABLED)

    def clear_data(self):
        self.rms.clear()
        self.p2p.clear()
        self.score.clear()
        self.raw_lines.clear()
        self.log.delete("1.0", tk.END)
        self.draw_plot()

    def poll_queue(self):
        try:
            while True:
                kind, payload = self.msg_queue.get_nowait()
                if kind == "data":
                    self.handle_data(payload)
                elif kind == "raw":
                    self.append_log("RAW  " + payload)
                elif kind == "status":
                    self.status_var.set(payload)
                elif kind == "error":
                    self.status_var.set(payload)
                    self.append_log("ERR  " + payload)
                    self.connect_btn.configure(state=tk.NORMAL)
                    self.disconnect_btn.configure(state=tk.DISABLED)
        except queue.Empty:
            pass
        self.after(50, self.poll_queue)

    def handle_data(self, msg):
        rms = float(msg.get("rms", 0.0))
        p2p = float(msg.get("p2p", 0.0))
        score = float(msg.get("score", 0.0))
        state = str(msg.get("state", "--"))
        seq = msg.get("seq", "--")
        drop = msg.get("drop", "--")

        self.rms.append(rms)
        self.p2p.append(p2p)
        self.score.append(score)

        self.state_var.set(f"State: {state}")
        self.seq_var.set(f"Seq: {seq}")
        self.score_var.set(f"Score: {score:.0f}")
        self.drop_var.set(f"Drop: {drop}")

        self.append_log(json.dumps(msg, ensure_ascii=False))
        self.draw_plot()

    def append_log(self, line):
        stamp = time.strftime("%H:%M:%S")
        self.log.insert(tk.END, f"{stamp}  {line}\n")
        self.log.see(tk.END)

    def draw_plot(self):
        c = self.canvas
        c.delete("all")
        w = max(c.winfo_width(), 10)
        h = max(c.winfo_height(), 10)
        pad_l, pad_r, pad_t, pad_b = 58, 18, 26, 42
        plot_w = w - pad_l - pad_r
        plot_h = h - pad_t - pad_b
        if plot_w <= 0 or plot_h <= 0:
            return

        c.create_rectangle(pad_l, pad_t, w - pad_r, h - pad_b, outline="#cccccc")
        for i in range(6):
            y = pad_t + plot_h * i / 5
            c.create_line(pad_l, y, w - pad_r, y, fill="#eeeeee")
        c.create_text(pad_l, 12, anchor=tk.W, text="RMS/P2P(g) and Score", fill="#333333")

        self._draw_series(list(self.rms), "#1f77b4", 0.0, self._auto_g_max(), pad_l, pad_t, plot_w, plot_h, "RMS")
        self._draw_series(list(self.p2p), "#2ca02c", 0.0, self._auto_g_max(), pad_l, pad_t, plot_w, plot_h, "P2P")
        self._draw_series(list(self.score), "#d62728", 0.0, 100.0, pad_l, pad_t, plot_w, plot_h, "Score")

        for threshold, color, label in ((60, "#ff9900", "WARNING 60"), (75, "#cc0000", "ALARM 75")):
            y = pad_t + plot_h * (1.0 - threshold / 100.0)
            c.create_line(pad_l, y, w - pad_r, y, fill=color, dash=(5, 3))
            c.create_text(w - pad_r - 6, y - 8, anchor=tk.E, text=label, fill=color, font=("Arial", 8))

        legend_x = pad_l + 8
        for i, (name, color) in enumerate((("RMS", "#1f77b4"), ("P2P", "#2ca02c"), ("Score", "#d62728"))):
            x = legend_x + i * 82
            y = h - 20
            c.create_line(x, y, x + 24, y, fill=color, width=2)
            c.create_text(x + 30, y, anchor=tk.W, text=name, fill="#333333")

    def _auto_g_max(self):
        values = list(self.rms) + list(self.p2p)
        if not values:
            return 0.05
        m = max(max(values), 0.01)
        return max(0.05, m * 1.25)

    def _draw_series(self, values, color, min_v, max_v, pad_l, pad_t, plot_w, plot_h, label):
        if len(values) < 2:
            return
        points = []
        span = max(max_v - min_v, 1e-6)
        count = len(values)
        for i, v in enumerate(values):
            x = pad_l + plot_w * i / max(count - 1, 1)
            y = pad_t + plot_h * (1.0 - (v - min_v) / span)
            y = max(pad_t, min(pad_t + plot_h, y))
            points.extend((x, y))
        self.canvas.create_line(*points, fill=color, width=2, smooth=False)


def main():
    default_port = sys.argv[1] if len(sys.argv) > 1 else None
    app = VibrationViewer(default_port)
    app.mainloop()


if __name__ == "__main__":
    main()
