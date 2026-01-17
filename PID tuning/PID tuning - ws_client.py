import tkinter as tk
from tkinter import ttk
import json
import asyncio
import threading
import websockets

WS_URI = "ws://127.0.0.1:81"
# WS_URI = "ws://192.168.4.1/ws"
CONFIG_FILE = "pid_constants.json"
RECONNECT_DELAY = 2  # seconds


class PIDGui:
    def __init__(self, root):
        self.root = root
        self.root.title("PID Controller")

        # Base PID variables
        self.b_kp = tk.DoubleVar()
        self.b_ki = tk.DoubleVar()
        self.b_kd = tk.DoubleVar()

        # Servo PID variables
        self.s_kp = tk.DoubleVar()
        self.s_ki = tk.DoubleVar()
        self.s_kd = tk.DoubleVar()

        self.status_var = tk.StringVar(value="Disconnected")

        self.websocket = None
        self.loop = None

        self.load_constants()
        self.build_gui()

        threading.Thread(
            target=self.start_ws_loop,
            daemon=True
        ).start()

    # ---------------- GUI ----------------

    def build_gui(self):
        main_frame = ttk.Frame(self.root, padding=10)
        main_frame.grid(row=0, column=0, sticky="nsew")

        pid_frame = ttk.Frame(main_frame)
        pid_frame.grid(row=0, column=0, sticky="nw", padx=(0, 20))

        self.build_pid_panel(
            pid_frame,
            title="Base PID",
            row=0,
            kp=self.b_kp,
            ki=self.b_ki,
            kd=self.b_kd,
            send_cmd=self.send_base_pid
        )

        self.build_pid_panel(
            pid_frame,
            title="Servo PID",
            row=0,
            column=1,
            kp=self.s_kp,
            ki=self.s_ki,
            kd=self.s_kd,
            send_cmd=self.send_servo_pid
        )

        # Right terminal
        terminal_frame = ttk.Frame(main_frame)
        terminal_frame.grid(row=0, column=1, sticky="nsew")

        ttk.Label(terminal_frame, text="Incoming WebSocket Messages").grid(
            row=0, column=0, sticky="w"
        )

        self.terminal = tk.Text(
            terminal_frame,
            width=45,
            height=14,
            state="disabled",
            background="#111",
            foreground="#0f0",
            wrap="word"
        )
        self.terminal.grid(row=1, column=0, sticky="nsew")

        scrollbar = ttk.Scrollbar(
            terminal_frame,
            orient="vertical",
            command=self.terminal.yview
        )
        scrollbar.grid(row=1, column=1, sticky="ns")
        self.terminal.configure(yscrollcommand=scrollbar.set)

        # Status bar
        status_frame = ttk.Frame(self.root, relief="sunken", padding=(5, 2))
        status_frame.grid(row=1, column=0, sticky="ew")

        ttk.Label(
            status_frame,
            textvariable=self.status_var,
            anchor="w"
        ).grid(row=0, column=0, sticky="w")

    def build_pid_panel(self, parent, title, row, kp, ki, kd, send_cmd, column=0):
        frame = ttk.Frame(parent, padding=(0, 0, 20, 0))
        frame.grid(row=row, column=column, sticky="nw")

        ttk.Label(frame, text=title).grid(
            row=0, column=0, columnspan=2, sticky="w", pady=(0, 5)
        )

        ttk.Label(frame, text="KP").grid(row=1, column=0, pady=5)
        ttk.Spinbox(frame, textvariable=kp, from_=-1000, to=1000,
                    increment=0.1, width=10).grid(row=1, column=1)

        ttk.Label(frame, text="KI").grid(row=2, column=0, pady=5)
        ttk.Spinbox(frame, textvariable=ki, from_=-1000, to=1000,
                    increment=0.1, width=10).grid(row=2, column=1)

        ttk.Label(frame, text="KD").grid(row=3, column=0, pady=5)
        ttk.Spinbox(frame, textvariable=kd, from_=-1000, to=1000,
                    increment=0.1, width=10).grid(row=3, column=1)

        ttk.Button(frame, text="SEND", command=send_cmd).grid(
            row=4, column=0, columnspan=2, pady=10
        )

    # ---------------- Persistence ----------------

    def load_constants(self):
        try:
            with open(CONFIG_FILE, "r") as f:
                data = json.load(f)

                self.b_kp.set(data.get("base", {}).get("kp", 0.0))
                self.b_ki.set(data.get("base", {}).get("ki", 0.0))
                self.b_kd.set(data.get("base", {}).get("kd", 0.0))

                self.s_kp.set(data.get("servo", {}).get("kp", 0.0))
                self.s_ki.set(data.get("servo", {}).get("ki", 0.0))
                self.s_kd.set(data.get("servo", {}).get("kd", 0.0))
        except FileNotFoundError:
            pass

    def save_constants(self):
        data = {
            "base": {
                "kp": self.b_kp.get(),
                "ki": self.b_ki.get(),
                "kd": self.b_kd.get()
            },
            "servo": {
                "kp": self.s_kp.get(),
                "ki": self.s_ki.get(),
                "kd": self.s_kd.get()
            }
        }
        with open(CONFIG_FILE, "w") as f:
            json.dump(data, f, indent=2)

    # ---------------- WebSocket ----------------

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
                    await self.receive_loop()
            except Exception:
                self.websocket = None
                self.update_status("Disconnected")
                await asyncio.sleep(RECONNECT_DELAY)

    async def receive_loop(self):
        async for message in self.websocket:
            self.root.after(0, self.display_message, message)

    async def send_message(self, message):
        if self.websocket:
            await self.websocket.send(message)

    # ---------------- Actions ----------------

    def send_base_pid(self):
        self.save_constants()
        msg = f"B#kp={self.b_kp.get()}#ki={self.b_ki.get()}#kd={self.b_kd.get()}#"
        self.send_ws(msg)

    def send_servo_pid(self):
        self.save_constants()
        msg = f"S#kp={self.s_kp.get()}#ki={self.s_ki.get()}#kd={self.s_kd.get()}#"
        self.send_ws(msg)

    def send_ws(self, msg):
        if self.loop and self.websocket:
            asyncio.run_coroutine_threadsafe(
                self.send_message(msg),
                self.loop
            )

    def display_message(self, message):
        self.terminal.configure(state="normal")
        self.terminal.insert("end", message + "\n")
        self.terminal.see("end")
        self.terminal.configure(state="disabled")

    def update_status(self, text):
        self.root.after(0, self.status_var.set, f"WebSocket: {text}")


if __name__ == "__main__":
    root = tk.Tk()
    app = PIDGui(root)
    root.mainloop()
