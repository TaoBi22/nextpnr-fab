#!/usr/bin/env bash
set -ex
yosys -p "tcl ../synth/synth_fabulous.tcl 4 ${1} ${1}.json" ${1}.v -l ${1}_yosys_log.txt
sed 's/sequential_16bit/'${1}'/g' bitstream_temp.py>bitstream.py
${NEXTPNR:-../../nextpnr-fabulous} --pre-pack fab_arch.py --pre-place fab_timing.py --json ${1}.json --post-route bitstream.py --write pnr${1}.json --verbose --debug --log ${1}_npnr_log.log
#yosys -p "read_verilog -lib ../synth/prims.v; read_json pnrblinky.json; dump -o blinky.il; show -format png -prefix blinky"
