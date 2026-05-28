import os
import subprocess
import sys
import shutil
from pathlib import Path

import asyncio
import websockets

from blocks_duo.Player import Player
from blocks_duo.WebsocketServer import WebsocketServer


class PlayerFactory:
    @staticmethod
    async def create(server: WebsocketServer, player_number: int, target: str, name: str, loop: asyncio.AbstractEventLoop):
        future: asyncio.Future[Player] = loop.create_future()

        def on_connect(socket: websockets.WebSocketServerProtocol):
            player = Player(player_number, target, name, socket)

            print(f'player: {player_number} connected')
            future.set_result(player)

        server.set_callback(on_connect)
        loop.run_in_executor(None, PlayerFactory.start_client, target, server.server_url())

        try:
            player = await asyncio.wait_for(future, 20)
            await player.send_player_number()
            print(f'player {player_number} was created.')
            return player
        finally:
            server.clear_callback()

    @staticmethod
    def start_client(target: str, url: str):
        print(f'client_script={target}')
        env = os.environ.copy()
        if target == 'mygame_ai':
            binary_path = env.get('MYGAME_BINARY_PATH_P1') or env.get('MYGAME_BINARY_PATH')
        elif target == 'mygame_ai_2':
            binary_path = env.get('MYGAME_BINARY_PATH_P2') or env.get('MYGAME_BINARY_PATH')
        else:
            binary_path = env.get('MYGAME_BINARY_PATH')
        if binary_path:
            env['MYGAME_BINARY_PATH'] = binary_path

        # Prefer calling the installed console script directly if available
        # (this avoids importing module-level build steps when packages are
        # installed into site-packages without source files). If that fails,
        # fall back to `python -m <module>`.
        # Try locating an executable script for the target
        script_path = shutil.which(target)
        if script_path is None:
            # try ~/.local/bin
            local_candidate = Path.home() / '.local' / 'bin' / target
            if local_candidate.exists():
                script_path = str(local_candidate)

        if script_path:
            try:
                subprocess.run([script_path, url], env=env)
                return
            except PermissionError:
                pass

        # Fallback: run as module with the same interpreter
        subprocess.run([sys.executable, '-m', target, url], env=env)
