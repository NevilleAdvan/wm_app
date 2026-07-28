#!/usr/bin/env bash
# 启动两个 wm-testapp 实例（红 1200 / 蓝 1201），验证 wm-daemon 把应用窗口
# 摆到 layer APP、状态栏下方。需先有 weston(wm0) + wm-daemon 在跑。
# 用法：./scripts/run_testapps.sh
set -euo pipefail
# cd to install output (output/windowsmanager/)
cd "$(dirname "$0")/../../../output/windowsmanager"

RUNTIME="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
# 无条件覆盖 .bashrc 里指向 WSLg 的 WAYLAND_DISPLAY，连到我们的 wm0 compositor。
export WAYLAND_DISPLAY="wm0"
export XDG_RUNTIME_DIR="$RUNTIME"

PIDS=()
cleanup(){ for p in "${PIDS[@]:-}"; do kill "$p" 2>/dev/null || true; done; }
trap cleanup EXIT INT TERM

echo "[testapps] launching red (id 1200) and blue (id 1201)"
./bin/wm-testapp --id 1200 --color red   --label "APP-A" &
PIDS+=($!)
sleep 0.3
./bin/wm-testapp --id 1201 --color blue   --label "APP-B" &
PIDS+=($!)

echo "[testapps] up. Ctrl+C to stop. Watch wm-daemon log for:"
echo "  [wm] surface 1200 -> layer 2000 (app) rect=0,56 1024x584"
echo "  [wm] surface 1201 -> layer 2000 (app) rect=0,56 1024x584"
wait
