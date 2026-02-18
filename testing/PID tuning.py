# To use this file, UNCOMMENT the code on the ws handler 'handleWebSocketMessage()' in the 'esp32_solar_station.ino' file.

# Requirement: install websocket library (pip install websockets)
import tkinter as tk
from tkinter import ttk
import json
import asyncio
import threading
import websockets

WS_URI = "ws://192.168.4.1/ws"
CONFIG_FILE = "./testing/pid_constants.json"
RECONNECT_DELAY = 2

class PIDGui:
    def __init__(self, root):
        self.root = root
        self.root.title("PID Light")

        self.kp_base = tk.StringVar(value="0")
        self.kp_servo = tk.StringVar(value="0")
        self.deadzone = tk.StringVar(value="0")  # <-- New deadzone variable
        self.status_var = tk.StringVar(value="WebSocket: Disconnected")

        self.websocket = None
        self.loop = None

        self.load_constants()
        self.build_gui()
        threading.Thread(target=self.start_ws_loop, daemon=True).start()

    # ================= GUI =================

    def build_gui(self):
        main_frame = ttk.Frame(self.root, padding=10)
        main_frame.grid(row=0, column=0, sticky="nsew")

        # Input Panels
        self.build_input(main_frame, "Base KP", self.kp_base, self.send_base, 0)
        self.build_input(main_frame, "Servo KP", self.kp_servo, self.send_servo, 1)
        self.build_input(main_frame, "Deadzone", self.deadzone, self.send_deadzone, 2)  # <-- Deadzone input

        # Terminal
        self.terminal = tk.Text(main_frame, width=40, height=10, state="disabled", 
                                 background="#111", foreground="#0f0", wrap="word")
        self.terminal.grid(row=0, column=3, padx=10, sticky="nsew")

        # Status Bar
        ttk.Label(self.root, textvariable=self.status_var, relief="sunken", anchor="w").grid(row=1, column=0, sticky="ew")

    def build_input(self, parent, label, var, cmd, col):
        frame = ttk.LabelFrame(parent, text=label, padding=10)
        frame.grid(row=0, column=col, padx=5, sticky="n")
        ttk.Spinbox(frame, textvariable=var, from_=-1000, to=1000, increment=0.1, width=10).pack(pady=5)
        ttk.Button(frame, text="SEND", command=cmd).pack(pady=5)

    # ================= Persistence & Logic =================

    def load_constants(self):
        try:
            with open(CONFIG_FILE, "r") as f:
                data = json.load(f)
                self.kp_base.set(data.get("base", "0"))
                self.kp_servo.set(data.get("servo", "0"))
                self.deadzone.set(data.get("deadzone", "0"))  # <-- Load deadzone
        except (FileNotFoundError, json.JSONDecodeError): pass

    def save_constants(self):
        with open(CONFIG_FILE, "w") as f:
            json.dump({
                "base": self.kp_base.get(),
                "servo": self.kp_servo.get(),
                "deadzone": self.deadzone.get()  # <-- Save deadzone
            }, f)

    def send_base(self):
        self.save_constants()
        self.send_ws(f"#KB#{self.kp_base.get()}")

    def send_servo(self):
        self.save_constants()
        self.send_ws(f"#KS#{self.kp_servo.get()}")

    def send_deadzone(self):
        self.save_constants()
        self.send_ws(f"#DZ#{self.deadzone.get()}")  # <-- Send deadzone

    # ================= WebSocket =================

    def start_ws_loop(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.ws_manager())

    async def ws_manager(self):
        while True:
            try:
                self.update_status("Connecting...")
                async with websockets.connect(WS_URI) as ws:
                    self.websocket = ws
                    self.update_status("Connected")
                    async for msg in ws:
                        self.root.after(0, self.display_message, msg)
            except Exception:
                self.websocket = None
                self.update_status("Disconnected")
                await asyncio.sleep(RECONNECT_DELAY)

    def send_ws(self, msg):
        if self.loop and self.websocket:
            asyncio.run_coroutine_threadsafe(self.websocket.send(msg), self.loop)

    def display_message(self, msg):
        self.terminal.config(state="normal")
        self.terminal.insert("end", f"{msg}\n")
        self.terminal.see("end")
        self.terminal.config(state="disabled")

    def update_status(self, text):
        self.root.after(0, self.status_var.set, f"WebSocket: {text}")

if __name__ == "__main__":
    root = tk.Tk()
    app = PIDGui(root)
    root.mainloop()
