#!/usr/bin/env python3
"""Simple Tk UI to build and USB-flash ESP32-Webradio firmware."""

from __future__ import annotations

import glob
import os
import queue
import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, scrolledtext, ttk

ROOT = Path(__file__).resolve().parents[1]
PIO = Path.home() / ".platformio" / "penv" / "bin" / "pio"
PYTHON_PIO = Path.home() / ".platformio" / "penv" / "bin" / "python"
ENV_NAME = "esp32dev"
BUILD_DIR = ROOT / ".pio" / "build" / ENV_NAME
DEFAULT_BAUD = "460800"


def find_boot_app0() -> Path | None:
    base = Path.home() / ".platformio" / "packages"
    matches = sorted(base.glob("framework-arduinoespressif32*/tools/partitions/boot_app0.bin"))
    return matches[0] if matches else None


def list_serial_ports() -> list[str]:
    ports = sorted(glob.glob("/dev/ttyUSB*")) + sorted(glob.glob("/dev/ttyACM*"))
    return ports


def read_firmware_label() -> str:
    cfg = ROOT / "src" / "config" / "firmware_config.h"
    try:
        text = cfg.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return "?"
    for line in text.splitlines():
        if "FIRMWARE" in line and '"' in line and not line.strip().startswith("#ifndef"):
            start = line.find('"')
            end = line.rfind('"')
            if start >= 0 and end > start:
                return line[start + 1 : end]
    return "?"


class UsbFlashUi(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("ESP32-Webradio USB Flash")
        self.geometry("780x520")
        self.minsize(640, 420)

        self._proc: subprocess.Popen[str] | None = None
        self._busy = False

        self._build_ui()
        self.refresh_ports()
        self._update_version_label()

    def _build_ui(self) -> None:
        pad = {"padx": 8, "pady": 6}

        top = ttk.Frame(self)
        top.pack(fill=tk.X, **pad)

        ttk.Label(top, text="Projekt:").grid(row=0, column=0, sticky=tk.W)
        ttk.Label(top, text=str(ROOT)).grid(row=0, column=1, columnspan=3, sticky=tk.W)

        ttk.Label(top, text="Firmware:").grid(row=1, column=0, sticky=tk.W)
        self.version_var = tk.StringVar(value="?")
        ttk.Label(top, textvariable=self.version_var).grid(row=1, column=1, sticky=tk.W)

        ttk.Label(top, text="USB-Port:").grid(row=2, column=0, sticky=tk.W)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=28, state="readonly")
        self.port_combo.grid(row=2, column=1, sticky=tk.W)
        ttk.Button(top, text="Ports neu laden", command=self.refresh_ports).grid(
            row=2, column=2, sticky=tk.W, padx=6
        )

        ttk.Label(top, text="Baud:").grid(row=3, column=0, sticky=tk.W)
        self.baud_var = tk.StringVar(value=DEFAULT_BAUD)
        ttk.Combobox(
            top,
            textvariable=self.baud_var,
            values=("115200", "230400", "460800", "921600"),
            width=12,
            state="readonly",
        ).grid(row=3, column=1, sticky=tk.W)

        self.full_flash_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            top,
            text="Vollflash (Bootloader + Partitionen + App)",
            variable=self.full_flash_var,
        ).grid(row=4, column=1, columnspan=2, sticky=tk.W)

        btns = ttk.Frame(self)
        btns.pack(fill=tk.X, **pad)
        self.btn_build_upload = ttk.Button(
            btns, text="Kompilieren + USB Upload", command=lambda: self.start_job(build=True)
        )
        self.btn_build_upload.pack(side=tk.LEFT, padx=(0, 8))
        self.btn_upload = ttk.Button(
            btns, text="Nur Upload (ohne Rebuild)", command=lambda: self.start_job(build=False)
        )
        self.btn_upload.pack(side=tk.LEFT, padx=(0, 8))
        self.btn_stop = ttk.Button(btns, text="Abbruch", command=self.stop_job, state=tk.DISABLED)
        self.btn_stop.pack(side=tk.LEFT)

        self.status_var = tk.StringVar(value="Bereit")
        ttk.Label(self, textvariable=self.status_var).pack(anchor=tk.W, padx=8)

        self.log = scrolledtext.ScrolledText(self, wrap=tk.WORD, height=22, font=("monospace", 10))
        self.log.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))

    def _update_version_label(self) -> None:
        self.version_var.set(read_firmware_label())

    def refresh_ports(self) -> None:
        ports = list_serial_ports()
        self.port_combo["values"] = ports
        if ports:
            current = self.port_var.get()
            if current not in ports:
                self.port_var.set(ports[0])
        else:
            self.port_var.set("")
            self.append_log("Kein /dev/ttyUSB* oder /dev/ttyACM* gefunden.\n")

    def append_log(self, text: str) -> None:
        self.log.insert(tk.END, text)
        self.log.see(tk.END)

    def set_busy(self, busy: bool) -> None:
        self._busy = busy
        state = tk.DISABLED if busy else tk.NORMAL
        self.btn_build_upload.configure(state=state)
        self.btn_upload.configure(state=state)
        self.btn_stop.configure(state=tk.NORMAL if busy else tk.DISABLED)

    def stop_job(self) -> None:
        proc = self._proc
        if proc and proc.poll() is None:
            self.append_log("\n--- Abbruch ---\n")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()

    def start_job(self, build: bool) -> None:
        if self._busy:
            return
        port = self.port_var.get().strip()
        if not port:
            messagebox.showerror("Kein Port", "Kein USB-Port ausgewaehlt.")
            return
        if not Path(port).exists():
            messagebox.showerror("Port fehlt", f"{port} existiert nicht.")
            return
        if not PIO.exists():
            messagebox.showerror("PlatformIO", f"pio nicht gefunden:\n{PIO}")
            return

        self._update_version_label()
        self.set_busy(True)
        self.status_var.set("Laeuft...")
        self.append_log(f"\n=== {'Build + Upload' if build else 'Upload only'} @ {port} ===\n")
        threading.Thread(target=self._run_job, args=(build, port), daemon=True).start()

    def _run_cmd(self, cmd: list[str], cwd: Path | None = None) -> int:
        self.after(0, lambda: self.append_log("$ " + " ".join(cmd) + "\n"))
        env = os.environ.copy()
        # Prefer PlatformIO Python tools on PATH for consistency.
        pio_bin = str(PIO.parent)
        env["PATH"] = pio_bin + os.pathsep + env.get("PATH", "")
        self._proc = subprocess.Popen(
            cmd,
            cwd=str(cwd or ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            env=env,
        )
        assert self._proc.stdout is not None
        for line in self._proc.stdout:
            self.after(0, lambda l=line: self.append_log(l))
        return self._proc.wait()

    def _free_port(self, port: str) -> None:
        # Best-effort; ignore failures if nothing holds the port.
        subprocess.run(["fuser", "-k", port], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(1)

    def _build_firmware(self) -> int:
        return self._run_cmd([str(PIO), "run", "-e", ENV_NAME, "-j", "1"], cwd=ROOT)

    def _flash_firmware(self, port: str) -> int:
        fw = BUILD_DIR / "firmware.bin"
        if not fw.is_file():
            self.after(0, lambda: self.append_log(f"Fehlt: {fw}\n"))
            return 1

        baud = self.baud_var.get().strip() or DEFAULT_BAUD
        self._free_port(port)

        if self.full_flash_var.get():
            bootloader = BUILD_DIR / "bootloader.bin"
            partitions = BUILD_DIR / "partitions.bin"
            boot_app0 = find_boot_app0()
            if not bootloader.is_file() or not partitions.is_file() or boot_app0 is None:
                self.after(
                    0,
                    lambda: self.append_log(
                        "Vollflash unmoeglich: bootloader/partitions/boot_app0 fehlen.\n"
                    ),
                )
                return 1
            cmd = [
                str(PYTHON_PIO),
                "-m",
                "esptool",
                "--chip",
                "esp32",
                "--port",
                port,
                "--baud",
                baud,
                "write-flash",
                "--flash-mode",
                "dio",
                "--flash-freq",
                "40m",
                "--flash-size",
                "4MB",
                "0x1000",
                str(bootloader),
                "0x8000",
                str(partitions),
                "0xe000",
                str(boot_app0),
                "0x10000",
                str(fw),
            ]
        else:
            cmd = [
                str(PYTHON_PIO),
                "-m",
                "esptool",
                "--chip",
                "esp32",
                "--port",
                port,
                "--baud",
                baud,
                "write-flash",
                "--flash-mode",
                "dio",
                "--flash-freq",
                "40m",
                "--flash-size",
                "4MB",
                "0x10000",
                str(fw),
            ]
        return self._run_cmd(cmd, cwd=ROOT)

    def _run_job(self, build: bool, port: str) -> None:
        rc = 0
        try:
            if build:
                rc = self._build_firmware()
                if rc != 0:
                    self.after(0, lambda: self._finish(False, f"Build fehlgeschlagen (rc={rc})"))
                    return
            rc = self._flash_firmware(port)
            ok = rc == 0
            msg = "Fertig" if ok else f"Upload fehlgeschlagen (rc={rc})"
            self.after(0, lambda: self._finish(ok, msg))
        except Exception as exc:  # noqa: BLE001 - show any worker failure in UI
            self.after(0, lambda: self._finish(False, f"Fehler: {exc}"))

    def _finish(self, ok: bool, message: str) -> None:
        self.set_busy(False)
        self.status_var.set(message)
        self.append_log(f"\n--- {message} ---\n")
        self._update_version_label()
        if not ok:
            messagebox.showerror("USB Flash", message)


def main() -> None:
    app = UsbFlashUi()
    app.mainloop()


if __name__ == "__main__":
    main()
