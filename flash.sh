#!/bin/bash
cd nrfconnect
source ~/ncs/v2.5.0/zephyr/zephyr-env.sh
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr/bin/gn
west flash --erase --recover