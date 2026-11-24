"""Compile the two Cortex-M3 firmware targets and run the host exercises."""

import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess


ROOT = Path(__file__).resolve().parent
OUT = ROOT / "build"
EXE = ".exe" if os.name == "nt" else ""
SIM = OUT / f"env_sim{EXE}"
SOURCES = ["firmware/sensors.c", "firmware/mqtt.c", "firmware/monitor.c"]
TARGETS = ("stm32f103", "gd32f103")
SCENARIOS = ("normal", "checksum", "dht-timeout", "light-missing", "reconnect")
CFLAGS = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Ifirmware"]
FLASH_START = 0x08000000
FLASH_SIZE = 64 * 1024


def zig_path():
    local = ROOT / ".tools" / "zig" / "ziglang" / f"zig{EXE}"
    candidate = os.environ.get("ZIG") or (str(local) if local.is_file() else shutil.which("zig"))
    if not candidate:
        raise SystemExit("Install Zig 0.15.2 or see README.md for a project-local installation.")
    return candidate


def zig(*args):
    subprocess.run([zig_path(), *args], check=True, cwd=ROOT)


def flash_image(elf):
    """Use ELF load addresses so RAM initialization data goes into flash."""
    data = elf.read_bytes()
    if data[:7] != b"\x7fELF\x01\x01\x01":
        raise SystemExit(f"{elf.name}: expected a 32-bit little-endian ELF.")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    _, _, machine, _, entry, phoff, _, _, _, phsize, phcount, *_ = header
    if machine != 40 or phsize != 32:
        raise SystemExit(f"{elf.name}: expected ARM ELF program headers.")
    image = bytearray()
    for index in range(phcount):
        kind, offset, _, physical, size, *_ = struct.unpack_from("<8I", data, phoff + index * phsize)
        if kind != 1 or size == 0:
            continue
        start = physical - FLASH_START
        end = start + size
        if start < 0 or end > FLASH_SIZE or offset + size > len(data):
            raise SystemExit(f"{elf.name}: invalid flash load segment at 0x{physical:08x}.")
        if end > len(image):
            image.extend(b"\xff" * (end - len(image)))
        image[start:end] = data[offset:offset + size]
    return image, entry


def build_firmware(target):
    elf = Path("build") / f"{target}.elf"
    binary = elf.with_suffix(".bin")
    zig(
        "cc", "-target", "thumb-freestanding-eabi", "-mcpu=cortex_m3",
        "-Os", "-g", "-ffreestanding", "-fno-unwind-tables", "-fno-stack-protector",
        "-ffunction-sections", "-fdata-sections", "-nostdlib", *CFLAGS,
        f"-DTARGET_{target.upper()}", *SOURCES, "firmware/main.c",
        "firmware/board/startup.c", "firmware/board/board.c",
        "-Wl,-T,firmware/board/f103c8.ld", "-Wl,--gc-sections", "-Wl,-e,reset_handler",
        "-o", elf.as_posix(),
    )
    image, entry = flash_image(ROOT / elf)
    if len(image) < 16 * 4:
        raise SystemExit(f"{target}: missing vector table.")
    stack, reset = struct.unpack_from("<II", image)
    systick, = struct.unpack_from("<I", image, 15 * 4)
    if (stack != 0x20005000 or reset != entry or reset % 2 != 1
            or not FLASH_START < reset < FLASH_START + len(image)):
        raise SystemExit(f"{target}: invalid reset vector 0x{stack:08x} 0x{reset:08x}")
    if systick % 2 != 1 or not FLASH_START < systick < FLASH_START + len(image):
        raise SystemExit(f"{target}: invalid SysTick vector 0x{systick:08x}")
    (ROOT / binary).write_bytes(image)
    print(f"{target}: {len(image)} bytes, SP=0x{stack:08x}, reset=0x{reset:08x}", flush=True)


def build_sim():
    zig("cc", *CFLAGS, "-O1", "-UNDEBUG", *SOURCES, "sim/sim.c", "-o", f"build/{SIM.name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    firmware = commands.add_parser("firmware")
    firmware.add_argument("target", choices=(*TARGETS, "all"), nargs="?", default="all")
    commands.add_parser("test")
    simulate = commands.add_parser("simulate")
    simulate.add_argument("scenario", choices=SCENARIOS, nargs="?", default="normal")
    options = parser.parse_args()
    OUT.mkdir(exist_ok=True)

    if options.command == "firmware":
        for target in TARGETS if options.target == "all" else (options.target,):
            build_firmware(target)
        return

    build_sim()
    scenarios = SCENARIOS if options.command == "test" else (options.scenario,)
    for scenario in scenarios:
        subprocess.run([str(SIM), scenario], check=True)
    if options.command == "test":
        print(f"PASS: {len(SCENARIOS)} host scenarios")


if __name__ == "__main__":
    main()
