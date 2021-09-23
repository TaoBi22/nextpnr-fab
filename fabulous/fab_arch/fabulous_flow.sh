#!/usr/bin/env bash
set -ex
yosys -p "tcl ../synth/synth_generic.tcl 4 ${1}.json" ${1}.v
${NEXTPNR:-../../nextpnr-generic} --pre-pack fab_arch.py --pre-place fab_timing.py --json ${1}.json --post-route bitstream.py --write pnr${1}.json --verbose --debug --log ${1}_test.log
#yosys -p "read_verilog -lib ../synth/prims.v; read_json pnrblinky.json; dump -o blinky.il; show -format png -prefix blinky"
