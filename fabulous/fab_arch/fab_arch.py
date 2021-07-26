import re
from array import *
import fileinput
import sys, getopt
import csv
from simple_config import *


wires = []
bels = []
pips = []
# Create bels
try:
    with open("./bel.txt", 'r') as file :
        bel_lines = file.readlines()
except IOError:
    print("bel.txt not accessible")

for i, line in enumerate(bel_lines):
    if '#' in line:
        continue
    context = line.split(',')
    if len(context) < 6:
        continue
    tile = context[0]
    bel_x = int(context[1][1])
    bel_y = int(context[2][1])
    bel_z = int(ord(context[3])-96)
    bel_idx = context[3]
    bel_type = context[4]
    bel_ports = context[5:]
    
    bel_name = tile+'_'+bel_type+'_'+bel_idx
    wire_name = tile+'_'+port
    
    if bel_name not in bels:
        ctx.addBel(name=bel_name, type=bel_type, loc=Loc(bel_x, bel_y, bel_z), gb=False, hidden=False)
        bels.append(bel_name)
    if bel_type == "IO_1_bidirectional_frame_config_pass":
        for port in bel_ports:
            #ctx.addWire(name=tile+'_'+bel_idx+'_CLK', type='IO_CLK', x=bel_x, y=bel_y)
            if wire_name not in wires:
                wires.append(wire_name)
                ctx.addWire(name=wire_name, type='IO_'+port[-1], x=bel_x, y=bel_y)
            if port[-1] in ['I', 'T']:
                ctx.addBelInput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
            else:
                ctx.addBelOutput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
    else if bel_type == "InPass4_frame_config" or bel_type == "OutPass4_frame_config":
        for port in bel_ports:
            if wire_name not in wires:
                wires.append(wire_name)
                ctx.addWire(name=wire_name, type=port[0:8], x=bel_x, y=bel_y)
            if port[-2] == 'I':
                ctx.addBelInput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
            else :
                ctx.addBelOutput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
    else if bel_type == "LUT4":
        for port in bel_ports:
            if port[3] == 'I':
                if wire_name not in wires:
                    wires.append(wire_name)
                    ctx.addWire(name=wire_name, type="LUT_in", x=bel_x, y=bel_y)
                ctx.addBelInput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
            else if port[3] == 'O' or  port[3] == 'Q':
                if wire_name not in wires:
                    wires.append(wire_name)
                    ctx.addWire(name=wire_name, type="LUT_out", x=bel_x, y=bel_y)
                ctx.addBelOutput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
            else:
                if port[-1] == 'i':
                    if wire_name not in wires:
                        wires.append(wire_name)
                        ctx.addWire(name=wire_name, type="carry_in", x=bel_x, y=bel_y)
                    ctx.addBelInput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)
                else:
                    if wire_name not in wires:
                        wires.append(wire_name)
                        ctx.addWire(name=wire_name, type="carry_out", x=bel_x, y=bel_y)
                    ctx.addBelOutput(bel=tile+'_'+bel_type+'_'+bel_idx, name=port, wire=tile+'_'+port)

# Pips
try:
    with open("./pips.txt", 'r') as file :
        pip_lines = file.readlines()
except IOError:
    print("pips.txt not accessible")

for i, line in enumerate(pip_lines):
    if '#' in line:
        continue
    context = line.split(',')
    src_tile = context[0]
    src_x = int(context[0][1])
    src_y = int(context[0][3])
    src_port = context[1]
    dst_tile = context[2]
    dst_x = int(context[2][1])
    dst_y = int(context[2][3])
    dst_port = context[3]
    src_wire_name = src_tile+'_'+src_port
    dst_wire_name = dst_tile+'_'+dst_port
    pip_delay = int(context[4])
    pip_name = src_tile+'.'+context[5]
    
    if src_wire_name not in wires:
        wires.append(src_wire_name)
        ctx.addWire(name=src_wire_name, type="interconnect", x=src_x, y=src_y)
    if dst_wire_name not in wires:
        wires.append(dst_wire_name)
        ctx.addWire(name=dst_wire_name, type="interconnect", x=dst_x, y=dst_y)
    ctx.addPip(name=pip_name, type="interconnect",
               srcWire=src_wire_name, dstWire=dst_wire_name,
               delay=ctx.getDelayFromNS(0.05), loc=Loc(src_x, src_y, 0))

