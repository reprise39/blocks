from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import tempfile
import socket
import hashlib
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = REPO_ROOT / 'mygame'
EVALUATE_FILE = SOURCE_DIR / 'blocks_evaluate.hpp'
VENV_START = REPO_ROOT / '.venv' / 'bin' / 'start_blocksduo'
MATCH_COUNT = 20
STEP = 0.3
LOWER_BOUND = 0.0
UPPER_BOUND = 5.0


@dataclass
class WeightSpec:
    name: str
    line_name: str


SPECS = [
    WeightSpec('w_cells', 'w_cells'),
    WeightSpec('w_corner_pot', 'w_corner_pot'),
    WeightSpec('w_corner_mob', 'w_corner_mob'),
    WeightSpec('w_hard_pieces', 'w_hard_pieces'),
    WeightSpec('w_edge_pen', 'w_edge_pen'),
]


LINE_PATTERN = re.compile(
    r'^(\s*const double\s+)(w_[a-z_]+)(\s*=\s*interpolateWeight\()([0-9.]+)(,\s*)([0-9.]+)(,\s*time_factor\);.*)$'
)


def parse_weight_table(text: str) -> dict[str, tuple[float, float]]:
    weights: dict[str, tuple[float, float]] = {}
    for line in text.splitlines():
        match = LINE_PATTERN.match(line)
        if not match:
            continue
        name = match.group(2)
        start_val = float(match.group(4))
        end_val = float(match.group(6))
        weights[name] = (start_val, end_val)
    missing = [spec.line_name for spec in SPECS if spec.line_name not in weights]
    if missing:
        raise RuntimeError(f'missing weight lines: {missing}')
    return weights


def write_weight_table(text: str, weights: dict[str, tuple[float, float]]) -> str:
    lines: list[str] = []
    for line in text.splitlines():
        match = LINE_PATTERN.match(line)
        if not match:
            lines.append(line)
            continue
        prefix = match.group(1)
        name = match.group(2)
        middle = match.group(3)
        start_val, end_val = weights[name]
        suffix = match.group(7)
        if name == 'w_hard_pieces':
            start_fmt = f'{start_val:.1f}'
            end_fmt = f'{end_val:.1f}'
        else:
            start_fmt = f'{start_val:.1f}'
            end_fmt = f'{end_val:.1f}'
        lines.append(f'{prefix}{name}{middle}{start_fmt}{match.group(5)}{end_fmt}{suffix}')
    return '\n'.join(lines) + '\n'


def clamp(value: float) -> float:
    return max(LOWER_BOUND, min(UPPER_BOUND, value))


def build_copy(template_dir: Path, weights: dict[str, tuple[float, float]], build_dir: Path) -> Path:
    if build_dir.exists():
        shutil.rmtree(build_dir)
    shutil.copytree(template_dir, build_dir)
    evaluate_path = build_dir / 'blocks_evaluate.hpp'
    evaluate_text = evaluate_path.read_text()
    evaluate_path.write_text(write_weight_table(evaluate_text, weights))
    # remove any existing binary and object files copied from template to force rebuild
    try:
        bin_path = build_dir / 'blocksstate'
        if bin_path.exists():
            bin_path.unlink()
        for obj in build_dir.rglob('*.o'):
            try:
                obj.unlink()
            except Exception:
                pass
        build_subdir = build_dir / 'build'
        if build_subdir.exists() and build_subdir.is_dir():
            try:
                shutil.rmtree(build_subdir)
            except Exception:
                pass
    except Exception:
        pass
    subprocess.run(['make', 'blocksstate'], cwd=build_dir, check=True)
    binary = build_dir / 'blocksstate'
    if not binary.exists():
        raise RuntimeError(f'build failed, missing binary: {binary}')
    return binary


def clone_build_dir(source_dir: Path, target_dir: Path) -> Path:
    if target_dir.exists():
        shutil.rmtree(target_dir)
    shutil.copytree(source_dir, target_dir)
    binary = target_dir / 'blocksstate'
    if not binary.exists():
        raise RuntimeError(f'cloned build dir missing binary: {binary}')
    return binary


def score_from_output(output: str, player_a: str, player_b: str) -> tuple[int, int]:
    score_a = None
    score_b = None
    for line in output.splitlines():
        stripped = line.strip()
        if stripped.startswith(f'{player_a}:'):
            score_a = int(stripped.split(':', 1)[1].strip())
        elif stripped.startswith(f'{player_b}:'):
            score_b = int(stripped.split(':', 1)[1].strip())
    if score_a is None or score_b is None:
        raise RuntimeError(f'failed to parse final score from output:\n{output}')
    return score_a, score_b


def compare_binaries(base_binary: Path, trial_binary: Path, run_log: Path) -> tuple[int, int, str]:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]

    env = os.environ.copy()
    env['MYGAME_BINARY_PATH_P1'] = str(base_binary)
    env['MYGAME_BINARY_PATH_P2'] = str(trial_binary)
    env['BLOCKS_DUO_PORT'] = str(port)
    env['BLOCKS_DUO_MATCH_DELAY_SEC'] = '0'

    # compute and log binary sha256 to ensure we are not running identical binaries
    sha_base = hashlib.sha256(base_binary.read_bytes()).hexdigest()
    sha_trial = hashlib.sha256(trial_binary.read_bytes()).hexdigest()
    header = f'BINARY_SHA256 base={sha_base} trial={sha_trial}\n'
    run_log.write_text(header)
    if sha_base == sha_trial:
        # log and skip comparison if binaries are identical (avoid aborting whole run)
        run_log.write_text(run_log.read_text() + 'INFO: base and trial binaries are identical - skipping compare\n')
        # return a neutral score indicating no change
        return 0, 0, 'identical'

    command = [str(VENV_START), 'mygame_ai', 'mygame_ai_2', str(MATCH_COUNT)]
    result = subprocess.run(command, cwd=REPO_ROOT, env=env, capture_output=True, text=True)
    output = result.stdout + '\n' + result.stderr
    run_log.write_text(run_log.read_text() + output)
    if result.returncode != 0:
        raise RuntimeError(f'start_blocksduo failed with code {result.returncode}\n{output}')

    score_base, score_trial = score_from_output(output, 'mygame_ai', 'mygame_ai_2')
    if score_base > score_trial:
        winner = 'base'
    elif score_trial > score_base:
        winner = 'trial'
    else:
        winner = 'draw'
    return score_base, score_trial, winner


def pick_direction(current: float, direction: float) -> float:
    return clamp(round(current + direction, 10))


def tune() -> None:
    original_text = EVALUATE_FILE.read_text()
    best_weights = parse_weight_table(original_text)

    timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    log_dir = REPO_ROOT / 'log'
    log_dir.mkdir(exist_ok=True)
    summary_path = log_dir / f'weight_tuning_{timestamp}.jsonl'

    with tempfile.TemporaryDirectory(prefix='blocks_tuning_') as temp_root_str:
        temp_root = Path(temp_root_str)

        with summary_path.open('w', encoding='utf-8') as summary_file:
            for spec in SPECS:
                for side in ('start', 'end'):
                    base_dir = temp_root / f'{spec.line_name}_{side}_base'
                    trial_dir = temp_root / f'{spec.line_name}_{side}_trial'
                    base_binary = build_copy(SOURCE_DIR, best_weights, base_dir)
                    direction = -STEP
                    while True:
                        current_start, current_end = best_weights[spec.line_name]
                        candidate_weights = dict(best_weights)
                        if side == 'start':
                            candidate_start = pick_direction(current_start, direction)
                            candidate_weights[spec.line_name] = (candidate_start, current_end)
                            candidate_value = candidate_start
                        else:
                            candidate_end = pick_direction(current_end, direction)
                            candidate_weights[spec.line_name] = (current_start, candidate_end)
                            candidate_value = candidate_end

                        if candidate_value == (current_start if side == 'start' else current_end):
                            break

                        trial_binary = build_copy(SOURCE_DIR, candidate_weights, trial_dir)

                        run_log = log_dir / f'{timestamp}_{spec.line_name}_{side}_{direction:+.1f}.txt'
                        base_score, trial_score, winner = compare_binaries(base_binary, trial_binary, run_log)
                        entry = {
                            'parameter': spec.line_name,
                            'side': side,
                            'direction': direction,
                            'base_value': current_start if side == 'start' else current_end,
                            'candidate_value': candidate_value,
                            'base_wins': base_score,
                            'candidate_wins': trial_score,
                            'winner': winner,
                            'accepted': False,
                        }

                        if trial_score > base_score:
                            best_weights[spec.line_name] = (
                                candidate_value if side == 'start' else current_start,
                                current_end if side == 'start' else candidate_value,
                            )
                            entry['accepted'] = True
                            entry['accepted_value'] = candidate_value
                            summary_file.write(json.dumps(entry, ensure_ascii=False) + '\n')
                            summary_file.flush()
                            base_binary = clone_build_dir(trial_dir, base_dir)
                            if direction < 0:
                                continue
                            break

                        summary_file.write(json.dumps(entry, ensure_ascii=False) + '\n')
                        summary_file.flush()
                        if direction < 0:
                            direction = STEP
                            continue
                        break

    updated_text = write_weight_table(original_text, best_weights)
    EVALUATE_FILE.write_text(updated_text)
    print(f'best weights written to {EVALUATE_FILE}')
    print(f'logs written to {summary_path}')
    print(json.dumps(best_weights, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    tune()
