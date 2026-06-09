# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import argparse
from importlib.metadata import version

from board import BOARDS
from sdfgen import DeviceTree, Sddf, SystemDescription

assert version("sdfgen").split(".")[1] == "28", "Unexpected sdfgen version"

ProtectionDomain = SystemDescription.ProtectionDomain


def generate(sdf_path: str, output_dir: str, dtb: DeviceTree):
    serial_node = dtb.node(board.serial)
    assert serial_node is not None

    serial_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=100)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=99)
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf", priority=99)
    serial_system = Sddf.Serial(sdf, serial_node, serial_driver, serial_virt_tx, virt_rx=serial_virt_rx)

    shell = ProtectionDomain("shell", "shell.elf", priority=1, budget=20000, stack_size=0x4000)
    serial_system.add_client(shell)

    for pd in [serial_driver, serial_virt_tx, serial_virt_rx, shell]:
        sdf.add_pd(pd)

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)

    with open(f"{output_dir}/{sdf_path}", "w+") as f:
        f.write(sdf.render())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)
    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))
    sdf = SystemDescription(board.arch, board.paddr_top)
    sddf = Sddf(args.sddf)

    with open(args.dtb, "rb") as f:
        dtb = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb)
