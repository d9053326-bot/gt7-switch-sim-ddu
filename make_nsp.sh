#!/usr/bin/env bash
# Build an installable standalone NSP (forwarder-free) from the compiled NRO/ELF.
#
# REQUIREMENT: an NSP's NCA files are encrypted with Switch master keys, so you
# must supply YOUR OWN prod.keys (dump from your own console with Lockpick_RCM).
# Keys are never bundled with this project.
#
# Usage:  ./make_nsp.sh /path/to/prod.keys
set -euo pipefail
cd "$(dirname "$0")"

KEYS="${1:-$HOME/.switch/prod.keys}"
if [ ! -f "$KEYS" ]; then
  echo "ERROR: prod.keys not found at: $KEYS"
  echo "Dump it on your Switch with Lockpick_RCM, then run:  ./make_nsp.sh /path/to/prod.keys"
  exit 1
fi
[ -f gt7_switch_hud.elf ] || { echo "ERROR: build the NRO first (make switch-docker)"; exit 1; }

TITLEID="0100A7C0DE7A5000"   # homebrew-range title id (0100xxxxxxxxx000)
rm -rf nsp_work && mkdir -p nsp_work

docker run --rm --platform linux/amd64 -v "$PWD:/app" -v "$KEYS:/keys/prod.keys:ro" -w /app devkitpro/devkita64:latest bash -c "
set -e
apt-get update -qq && apt-get install -y -qq git build-essential >/dev/null
cd /tmp && git clone -q https://github.com/The-4n/hacBrewPack.git && cd hacBrewPack
cp config.mk.template config.mk 2>/dev/null || true
make -s && cp hacbrewpack /usr/local/bin/
cd /app/nsp_work
mkdir -p exefs romfs logo control
elf2nso ../gt7_switch_hud.elf exefs/main
cat > main.json <<'JSON'
{
 \"name\": \"GT7 Sim-DDU Pro\", \"title_id\": \"0x${TITLEID}\", \"title_id_range_min\": \"0x${TITLEID}\", \"title_id_range_max\": \"0x${TITLEID}\",
 \"main_thread_stack_size\": \"0x100000\", \"main_thread_priority\": 44, \"default_cpu_id\": 3, \"process_category\": 0,
 \"is_retail\": true, \"pool_partition\": 2, \"is_64_bit\": true, \"address_space_type\": 3,
 \"filesystem_access\": {\"permissions\": \"0xFFFFFFFFFFFFFFFF\"},
 \"service_access\": [\"*\"], \"service_host\": [\"*\"],
 \"kernel_capabilities\": [
  {\"type\":\"kernel_flags\",\"value\":{\"highest_thread_priority\":63,\"lowest_thread_priority\":24,\"lowest_cpu_id\":3,\"highest_cpu_id\":0}},
  {\"type\":\"syscalls\",\"value\":{\"svcUnknown00\":\"0x00\"}},
  {\"type\":\"min_kernel_version\",\"value\":\"0x0030\"},
  {\"type\":\"handle_table_size\",\"value\":512},
  {\"type\":\"debug_flags\",\"value\":{\"allow_debug\":false,\"force_debug\":true}}
 ]
}
JSON
npdmtool main.json exefs/main.npdm
nacptool --create 'GT7 Sim-DDU Pro' 'GT7 SimRacing' '_aw_1.10.28.2026' control/control.nacp
cp ../icon.jpg control/icon_AmericanEnglish.dat
cp ../icon.jpg logo/NintendoLogo.png 2>/dev/null || true
echo -n > logo/StartupMovie.gif
hacbrewpack --keyset /keys/prod.keys --titleid ${TITLEID} --titlename 'GT7 Sim-DDU Pro' --publisher 'GT7 SimRacing' --nopatchnacplogo -o /app/nsp_out/ 
"
ls -la nsp_out/*.nsp && cp nsp_out/*.nsp ./gt7_switch_hud.nsp && echo "OK -> gt7_switch_hud.nsp"
