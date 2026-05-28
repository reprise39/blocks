from __future__ import annotations

import asyncio
import os
import subprocess
from pathlib import Path

import websockets


def find_project_root() -> Path:
    binary_override = os.environ.get('MYGAME_BINARY_PATH')
    if binary_override:
        return Path(binary_override).resolve().parent

    candidates = [Path.cwd(), *Path(__file__).resolve().parents]
    for candidate in candidates:
        makefile = candidate / 'Makefile'
        blocksstate_cpp = candidate / 'blocksstate.cpp'
        if makefile.exists() and blocksstate_cpp.exists():
            return candidate
    raise RuntimeError('project root not found: Makefile and blocksstate.cpp were not found')


ROOT = find_project_root()


def selected_binary(profile: str | None = None) -> Path:
    binary_override = os.environ.get('MYGAME_BINARY_PATH')
    if binary_override:
        return Path(binary_override)
    env_profile = os.environ.get('MYGAME_BINARY', '')
    if env_profile:
        profile = env_profile
    if profile == 'alt':
        binary_name = 'blocksstate_alt'
    elif profile == 'chokudai':
        binary_name = 'blocksstate_chokudai'
    else:
        binary_name = 'blocksstate'
    return ROOT / binary_name


def build_binary() -> None:
    if os.environ.get('MYGAME_BINARY_PATH'):
        return
    subprocess.run(['make', 'blocksstate', 'blocksstate_alt', 'blocksstate_chokudai'], cwd=ROOT, check=True)


def extract_board_lines(board_text: str) -> list[str]:
    lines = [line for line in board_text.splitlines() if line]
    if lines and lines[0].startswith(' '):
        lines = lines[1:]
    if len(lines) < 14:
        raise RuntimeError(f'invalid board data: expected 14 rows, got {len(lines)}')

    board_lines: list[str] = []
    for line in lines[:14]:
        if len(line) < 15:
            raise RuntimeError(f'invalid board row: {line!r}')
        board_lines.append(line[1:15])
    return board_lines


async def run(url: str, profile: str | None = None) -> None:
    build_binary()
    binary = selected_binary(profile)

    proc = await asyncio.create_subprocess_exec(
        str(binary),
        cwd=binary.parent,
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
    )

    if proc.stdin is None or proc.stdout is None:
        raise RuntimeError('failed to start C++ process')

    try:
        async with websockets.connect(url) as socket:
            player_number = await socket.recv()
            proc.stdin.write(f'{player_number}\n'.encode())
            await proc.stdin.drain()

            while True:
                board_text = await socket.recv()
                for line in extract_board_lines(board_text):
                    proc.stdin.write(f'{line}\n'.encode())
                await proc.stdin.drain()

                action = (await proc.stdout.readline()).decode().strip()
                if not action:
                    raise RuntimeError('C++ AI process closed stdout unexpectedly')

                await socket.send(action)
                if action == 'X000':
                    break
    finally:
        if proc.stdin is not None:
            proc.stdin.close()
        await proc.wait()


def main() -> None:
    import sys

    if len(sys.argv) < 2:
        raise SystemExit('usage: mygame_ai <ws_url>')
    asyncio.run(run(sys.argv[1]))


def main_alt() -> None:
    import sys

    if len(sys.argv) < 2:
        raise SystemExit('usage: mygame_ai_2 <ws_url>')
    asyncio.run(run(sys.argv[1], 'alt'))


def main_chokudai() -> None:
    import sys

    if len(sys.argv) < 2:
        raise SystemExit('usage: mygame_chokudai <ws_url>')
    asyncio.run(run(sys.argv[1], 'chokudai'))


if __name__ == '__main__':
    main()