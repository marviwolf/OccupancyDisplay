#!/bin/bash
cd nrfconnect
source ~/ncs/v2.5.0/zephyr/zephyr-env.sh
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr/bin/gn
#python3 ~/ncs/v2.5.0/modules/lib/matter/scripts/setup/nrfconnect/update_ncs.py --update
#~/connectedhomeip/scripts/setup/nrfconnect/update_ncs.py --update
rm -r build
west build -b nrf5340dk_nrf5340_cpuapp -p auto . -- \
		-DEXTRA_DTC_OVERLAY_FILE="board_overrides.overlay" \
		-DSHIELD=waveshare_epaper_gdew075t7 \
		-DOVERLAY_CONFIG="display.conf"