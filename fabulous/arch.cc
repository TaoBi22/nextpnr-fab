/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018  Clifford Wolf <clifford@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <iostream>
#include <math.h>
#include "cells.h"
#include "nextpnr.h"
#include "placer1.h"
#include "placer_heap.h"
#include "router1.h"
#include "router2.h"
#include "util.h"

#include <boost/range/iterator_range.hpp>

NEXTPNR_NAMESPACE_BEGIN

WireInfo &Arch::wire_info(IdStringList wire)
{
    auto w = wires.find(wire);
    if (w == wires.end())
        NPNR_ASSERT_FALSE_STR("no wire named " + wire.str(getCtx()));
    return w->second;
}

PipInfo &Arch::pip_info(IdStringList pip)
{
    auto p = pips.find(pip);
    if (p == pips.end())
        NPNR_ASSERT_FALSE_STR("no pip named " + pip.str(getCtx()));
    return p->second;
}

BelInfo &Arch::bel_info(IdStringList bel)
{
    auto b = bels.find(bel);
    if (b == bels.end())
        NPNR_ASSERT_FALSE_STR("no bel named " + bel.str(getCtx()));
    return b->second;
}

void Arch::addWire(IdStringList name, IdString type, int x, int y)
{
    NPNR_ASSERT(wires.count(name) == 0);
    WireInfo &wi = wires[name];
    wi.name = name;
    wi.type = type;
    wi.x = x;
    wi.y = y;

    wire_ids.push_back(name);
}

void Arch::addPip(IdStringList name, IdString type, IdStringList srcWire, IdStringList dstWire, delay_t delay, Loc loc)
{
    NPNR_ASSERT(pips.count(name) == 0);
    PipInfo &pi = pips[name];
    pi.name = name;
    pi.type = type;
    pi.srcWire = srcWire;
    pi.dstWire = dstWire;
    pi.delay = delay;
    pi.loc = loc;

    wire_info(srcWire).downhill.push_back(name);
    wire_info(dstWire).uphill.push_back(name);
    pip_ids.push_back(name);

    if (int(tilePipDimZ.size()) <= loc.x)
        tilePipDimZ.resize(loc.x + 1);

    if (int(tilePipDimZ[loc.x].size()) <= loc.y)
        tilePipDimZ[loc.x].resize(loc.y + 1);

    gridDimX = std::max(gridDimX, loc.x + 1);
    gridDimY = std::max(gridDimY, loc.x + 1);
    tilePipDimZ[loc.x][loc.y] = std::max(tilePipDimZ[loc.x][loc.y], loc.z + 1);
}

void Arch::addBel(IdStringList name, IdString type, Loc loc, bool gb, bool hidden)
{
    NPNR_ASSERT(bels.count(name) == 0);
    NPNR_ASSERT(bel_by_loc.count(loc) == 0);
    BelInfo &bi = bels[name];
    bi.name = name;
    bi.type = type;
    bi.x = loc.x;
    bi.y = loc.y;
    bi.z = loc.z;
    bi.gb = gb;
    bi.hidden = hidden;

    bel_ids.push_back(name);
    bel_by_loc[loc] = name;

    if (int(bels_by_tile.size()) <= loc.x)
        bels_by_tile.resize(loc.x + 1);

    if (int(bels_by_tile[loc.x].size()) <= loc.y)
        bels_by_tile[loc.x].resize(loc.y + 1);

    bels_by_tile[loc.x][loc.y].push_back(name);

    if (int(tileBelDimZ.size()) <= loc.x)
        tileBelDimZ.resize(loc.x + 1);

    if (int(tileBelDimZ[loc.x].size()) <= loc.y)
        tileBelDimZ[loc.x].resize(loc.y + 1);

    gridDimX = std::max(gridDimX, loc.x + 1);
    gridDimY = std::max(gridDimY, loc.x + 1);
    tileBelDimZ[loc.x][loc.y] = std::max(tileBelDimZ[loc.x][loc.y], loc.z + 1);
}

void Arch::addBelInput(IdStringList bel, IdString name, IdStringList wire)
{
    NPNR_ASSERT(bel_info(bel).pins.count(name) == 0);
    PinInfo &pi = bel_info(bel).pins[name];
    pi.name = name;
    pi.wire = wire;
    pi.type = PORT_IN;

    wire_info(wire).downhill_bel_pins.push_back(BelPin{bel, name});
    wire_info(wire).bel_pins.push_back(BelPin{bel, name});
}

void Arch::addBelOutput(IdStringList bel, IdString name, IdStringList wire)
{
    NPNR_ASSERT(bel_info(bel).pins.count(name) == 0);
    PinInfo &pi = bel_info(bel).pins[name];
    pi.name = name;
    pi.wire = wire;
    pi.type = PORT_OUT;

    wire_info(wire).uphill_bel_pin = BelPin{bel, name};
    wire_info(wire).bel_pins.push_back(BelPin{bel, name});
}

void Arch::addBelInout(IdStringList bel, IdString name, IdStringList wire)
{
    NPNR_ASSERT(bel_info(bel).pins.count(name) == 0);
    PinInfo &pi = bel_info(bel).pins[name];
    pi.name = name;
    pi.wire = wire;
    pi.type = PORT_INOUT;

    wire_info(wire).downhill_bel_pins.push_back(BelPin{bel, name});
    wire_info(wire).bel_pins.push_back(BelPin{bel, name});
}

void Arch::addGroupBel(IdStringList group, IdStringList bel) { groups[group].bels.push_back(bel); }

void Arch::addGroupWire(IdStringList group, IdStringList wire) { groups[group].wires.push_back(wire); }

void Arch::addGroupPip(IdStringList group, IdStringList pip) { groups[group].pips.push_back(pip); }

void Arch::addGroupGroup(IdStringList group, IdStringList grp) { groups[group].groups.push_back(grp); }

void Arch::addDecalGraphic(DecalId decal, const GraphicElement &graphic)
{
    decal_graphics[decal].push_back(graphic);
    refreshUi();
}

void Arch::setWireDecal(WireId wire, DecalXY decalxy)
{
    wire_info(wire).decalxy = decalxy;
    refreshUiWire(wire);
}

void Arch::setPipDecal(PipId pip, DecalXY decalxy)
{
    pip_info(pip).decalxy = decalxy;
    refreshUiPip(pip);
}

void Arch::setBelDecal(BelId bel, DecalXY decalxy)
{
    bel_info(bel).decalxy = decalxy;
    refreshUiBel(bel);
}

void Arch::setGroupDecal(GroupId group, DecalXY decalxy)
{
    groups[group].decalxy = decalxy;
    refreshUiGroup(group);
}

void Arch::setWireAttr(IdStringList wire, IdString key, const std::string &value)
{
    wire_info(wire).attrs[key] = value;
}

void Arch::setPipAttr(IdStringList pip, IdString key, const std::string &value) { pip_info(pip).attrs[key] = value; }

void Arch::setBelAttr(IdStringList bel, IdString key, const std::string &value) { bel_info(bel).attrs[key] = value; }

void Arch::setLutK(int K) { args.K = K; }

void Arch::setDelayScaling(double scale, double offset)
{
    args.delayScale = scale;
    args.delayOffset = offset;
}

void Arch::addCellTimingClock(IdString cell, IdString port) { cellTiming[cell].portClasses[port] = TMG_CLOCK_INPUT; }

void Arch::addCellTimingDelay(IdString cell, IdString fromPort, IdString toPort, delay_t delay)
{
    if (get_or_default(cellTiming[cell].portClasses, fromPort, TMG_IGNORE) == TMG_IGNORE)
        cellTiming[cell].portClasses[fromPort] = TMG_COMB_INPUT;
    if (get_or_default(cellTiming[cell].portClasses, toPort, TMG_IGNORE) == TMG_IGNORE)
        cellTiming[cell].portClasses[toPort] = TMG_COMB_OUTPUT;
    cellTiming[cell].combDelays[CellDelayKey{fromPort, toPort}] = DelayQuad(delay);
}

void Arch::addCellTimingSetupHold(IdString cell, IdString port, IdString clock, delay_t setup, delay_t hold)
{
    TimingClockingInfo ci;
    ci.clock_port = clock;
    ci.edge = RISING_EDGE;
    ci.setup = DelayPair(setup);
    ci.hold = DelayPair(hold);
    cellTiming[cell].clockingInfo[port].push_back(ci);
    cellTiming[cell].portClasses[port] = TMG_REGISTER_INPUT;
}

void Arch::addCellTimingClockToOut(IdString cell, IdString port, IdString clock, delay_t clktoq)
{
    TimingClockingInfo ci;
    ci.clock_port = clock;
    ci.edge = RISING_EDGE;
    ci.clockToQ = DelayQuad(clktoq);
    cellTiming[cell].clockingInfo[port].push_back(ci);
    cellTiming[cell].portClasses[port] = TMG_REGISTER_OUTPUT;
}

void Arch::clearCellBelPinMap(IdString cell, IdString cell_pin) { cells.at(cell)->bel_pins[cell_pin].clear(); }
void Arch::addCellBelPinMapping(IdString cell, IdString cell_pin, IdString bel_pin)
{
    cells.at(cell)->bel_pins[cell_pin].push_back(bel_pin);
}

// ---------------------------------------------------------------

Arch::Arch(ArchArgs args) : chipName("fabulous"), args(args)
{
    // Dummy for empty decals
    decal_graphics[DecalId()];
}

void IdString::initialize_arch(const BaseCtx *ctx) {}

// ---------------------------------------------------------------

BelId Arch::getBelByName(IdStringList name) const
{
    if (bels.count(name))
        return name;
    return BelId();
}

IdStringList Arch::getBelName(BelId bel) const { return bel; }

Loc Arch::getBelLocation(BelId bel) const
{
    auto &info = bels.at(bel);
    return Loc(info.x, info.y, info.z);
}

BelId Arch::getBelByLocation(Loc loc) const
{
    auto it = bel_by_loc.find(loc);
    if (it != bel_by_loc.end())
        return it->second;
    return BelId();
}

const std::vector<BelId> &Arch::getBelsByTile(int x, int y) const { return bels_by_tile.at(x).at(y); }

bool Arch::getBelGlobalBuf(BelId bel) const { return bels.at(bel).gb; }

uint32_t Arch::getBelChecksum(BelId bel) const
{
    // FIXME
    return 0;
}

void Arch::bindBel(BelId bel, CellInfo *cell, PlaceStrength strength)
{
    bels.at(bel).bound_cell = cell;
    cell->bel = bel;
    cell->belStrength = strength;
    refreshUiBel(bel);
}

void Arch::unbindBel(BelId bel)
{
    bels.at(bel).bound_cell->bel = BelId();
    bels.at(bel).bound_cell->belStrength = STRENGTH_NONE;
    bels.at(bel).bound_cell = nullptr;
    refreshUiBel(bel);
}

bool Arch::checkBelAvail(BelId bel) const { return bels.at(bel).bound_cell == nullptr; }

CellInfo *Arch::getBoundBelCell(BelId bel) const { return bels.at(bel).bound_cell; }

CellInfo *Arch::getConflictingBelCell(BelId bel) const { return bels.at(bel).bound_cell; }

const std::vector<BelId> &Arch::getBels() const { return bel_ids; }

IdString Arch::getBelType(BelId bel) const { return bels.at(bel).type; }

bool Arch::getBelHidden(BelId bel) const { return bels.at(bel).hidden; }

const std::map<IdString, std::string> &Arch::getBelAttrs(BelId bel) const { return bels.at(bel).attrs; }

WireId Arch::getBelPinWire(BelId bel, IdString pin) const
{
    const auto &bdata = bels.at(bel);
    if (!bdata.pins.count(pin))
        log_error("bel '%s' has no pin '%s'\n", getCtx()->nameOfBel(bel), pin.c_str(this));
    return bdata.pins.at(pin).wire;
}

PortType Arch::getBelPinType(BelId bel, IdString pin) const { return bels.at(bel).pins.at(pin).type; }

std::vector<IdString> Arch::getBelPins(BelId bel) const
{
    std::vector<IdString> ret;
    for (auto &it : bels.at(bel).pins)
        ret.push_back(it.first);
    return ret;
}

const std::vector<IdString> &Arch::getBelPinsForCellPin(const CellInfo *cell_info, IdString pin) const
{
    return cell_info->bel_pins.at(pin);
}

// ---------------------------------------------------------------

WireId Arch::getWireByName(IdStringList name) const
{
    if (wires.count(name))
        return name;
    return WireId();
}

IdStringList Arch::getWireName(WireId wire) const { return wire; }

IdString Arch::getWireType(WireId wire) const { return wires.at(wire).type; }

const std::map<IdString, std::string> &Arch::getWireAttrs(WireId wire) const { return wires.at(wire).attrs; }

uint32_t Arch::getWireChecksum(WireId wire) const
{
    // FIXME
    return 0;
}

void Arch::bindWire(WireId wire, NetInfo *net, PlaceStrength strength)
{
    wires.at(wire).bound_net = net;
    net->wires[wire].pip = PipId();
    net->wires[wire].strength = strength;
    refreshUiWire(wire);
}

void Arch::unbindWire(WireId wire)
{
    auto &net_wires = wires.at(wire).bound_net->wires;

    auto pip = net_wires.at(wire).pip;
    if (pip != PipId()) {
        pips.at(pip).bound_net = nullptr;
        refreshUiPip(pip);
    }

    net_wires.erase(wire);
    wires.at(wire).bound_net = nullptr;
    refreshUiWire(wire);
}

bool Arch::checkWireAvail(WireId wire) const { return wires.at(wire).bound_net == nullptr; }

NetInfo *Arch::getBoundWireNet(WireId wire) const { return wires.at(wire).bound_net; }

NetInfo *Arch::getConflictingWireNet(WireId wire) const { return wires.at(wire).bound_net; }

const std::vector<BelPin> &Arch::getWireBelPins(WireId wire) const { return wires.at(wire).bel_pins; }

const std::vector<WireId> &Arch::getWires() const { return wire_ids; }

// ---------------------------------------------------------------

PipId Arch::getPipByName(IdStringList name) const
{
    if (pips.count(name))
        return name;
    return PipId();
}

IdStringList Arch::getPipName(PipId pip) const { return pip; }

IdString Arch::getPipType(PipId pip) const { return pips.at(pip).type; }

const std::map<IdString, std::string> &Arch::getPipAttrs(PipId pip) const { return pips.at(pip).attrs; }

uint32_t Arch::getPipChecksum(PipId wire) const
{
    // FIXME
    return 0;
}

void Arch::bindPip(PipId pip, NetInfo *net, PlaceStrength strength)
{
    WireId wire = pips.at(pip).dstWire;
    pips.at(pip).bound_net = net;
    wires.at(wire).bound_net = net;
    net->wires[wire].pip = pip;
    net->wires[wire].strength = strength;
    refreshUiPip(pip);
    refreshUiWire(wire);
}

void Arch::unbindPip(PipId pip)
{
    WireId wire = pips.at(pip).dstWire;
    wires.at(wire).bound_net->wires.erase(wire);
    pips.at(pip).bound_net = nullptr;
    wires.at(wire).bound_net = nullptr;
    refreshUiPip(pip);
    refreshUiWire(wire);
}

bool Arch::checkPipAvail(PipId pip) const { return pips.at(pip).bound_net == nullptr; }

bool Arch::checkPipAvailForNet(PipId pip, NetInfo *net) const
{
    NetInfo *bound_net = pips.at(pip).bound_net;
    return bound_net == nullptr || bound_net == net;
}

NetInfo *Arch::getBoundPipNet(PipId pip) const { return pips.at(pip).bound_net; }

NetInfo *Arch::getConflictingPipNet(PipId pip) const { return pips.at(pip).bound_net; }

WireId Arch::getConflictingPipWire(PipId pip) const { return pips.at(pip).bound_net ? pips.at(pip).dstWire : WireId(); }

const std::vector<PipId> &Arch::getPips() const { return pip_ids; }

Loc Arch::getPipLocation(PipId pip) const { return pips.at(pip).loc; }

WireId Arch::getPipSrcWire(PipId pip) const { return pips.at(pip).srcWire; }

WireId Arch::getPipDstWire(PipId pip) const { return pips.at(pip).dstWire; }

DelayQuad Arch::getPipDelay(PipId pip) const { return DelayQuad(pips.at(pip).delay); }

const std::vector<PipId> &Arch::getPipsDownhill(WireId wire) const { return wires.at(wire).downhill; }

const std::vector<PipId> &Arch::getPipsUphill(WireId wire) const { return wires.at(wire).uphill; }

// ---------------------------------------------------------------

GroupId Arch::getGroupByName(IdStringList name) const { return name; }

IdStringList Arch::getGroupName(GroupId group) const { return group; }

std::vector<GroupId> Arch::getGroups() const
{
    std::vector<GroupId> ret;
    for (auto &it : groups)
        ret.push_back(it.first);
    return ret;
}

const std::vector<BelId> &Arch::getGroupBels(GroupId group) const { return groups.at(group).bels; }

const std::vector<WireId> &Arch::getGroupWires(GroupId group) const { return groups.at(group).wires; }

const std::vector<PipId> &Arch::getGroupPips(GroupId group) const { return groups.at(group).pips; }

const std::vector<GroupId> &Arch::getGroupGroups(GroupId group) const { return groups.at(group).groups; }

// ---------------------------------------------------------------

delay_t Arch::estimateDelay(WireId src, WireId dst) const
{
    const WireInfo &s = wires.at(src);
    const WireInfo &d = wires.at(dst);
    int dx = abs(s.x - d.x);
    int dy = abs(s.y - d.y);
    return (dx + dy) * args.delayScale + args.delayOffset;
}

delay_t Arch::predictDelay(const NetInfo *net_info, const PortRef &sink) const
{
    const auto &driver = net_info->driver;
    auto driver_loc = getBelLocation(driver.cell->bel);
    auto sink_loc = getBelLocation(sink.cell->bel);

    int dx = abs(sink_loc.x - driver_loc.x);
    int dy = abs(sink_loc.y - driver_loc.y);
    return (dx + dy) * args.delayScale + args.delayOffset;
}

bool Arch::getBudgetOverride(const NetInfo *net_info, const PortRef &sink, delay_t &budget) const { return false; }

ArcBounds Arch::getRouteBoundingBox(WireId src, WireId dst) const
{
    ArcBounds bb;

    int src_x = wires.at(src).x;
    int src_y = wires.at(src).y;
    int dst_x = wires.at(dst).x;
    int dst_y = wires.at(dst).y;

    bb.x0 = src_x;
    bb.y0 = src_y;
    bb.x1 = src_x;
    bb.y1 = src_y;

    auto extend = [&](int x, int y) {
        bb.x0 = std::min(bb.x0, x);
        bb.x1 = std::max(bb.x1, x);
        bb.y0 = std::min(bb.y0, y);
        bb.y1 = std::max(bb.y1, y);
    };
    extend(dst_x, dst_y);
    return bb;
}

// ---------------------------------------------------------------

bool Arch::place()
{
    std::string placer = str_or_default(settings, id("placer"), defaultPlacer);
    if (placer == "heap") {
        bool have_iobuf_or_constr = false;
        for (auto &cell : cells) {
            CellInfo *ci = cell.second.get();
            if (ci->type == id("IO_1_bidirectional_frame_config_pass") || ci->bel != BelId() || ci->attrs.count(id("BEL"))) {
                have_iobuf_or_constr = true;
                break;
            }
        }
        bool retVal;
        if (!have_iobuf_or_constr) {
            log_warning("Unable to use HeAP due to a lack of IO buffers or constrained cells as anchors; reverting to "
                        "SA.\n");
            retVal = placer1(getCtx(), Placer1Cfg(getCtx()));
        } else {
            PlacerHeapCfg cfg(getCtx());
            cfg.ioBufTypes.insert(id("IO_1_bidirectional_frame_config_pass"));
            retVal = placer_heap(getCtx(), cfg);
        }
        getCtx()->settings[getCtx()->id("place")] = 1;
        archInfoToAttributes();
        return retVal;
    } else if (placer == "sa") {
        bool retVal = placer1(getCtx(), Placer1Cfg(getCtx()));
        getCtx()->settings[getCtx()->id("place")] = 1;
        archInfoToAttributes();
        return retVal;
    } else {
        log_error("Fabulous architecture does not support placer '%s'\n", placer.c_str());
    }
}

bool Arch::route()
{
    std::string router = str_or_default(settings, id("router"), defaultRouter);
    bool result;
    if (router == "router1") {
        result = router1(getCtx(), Router1Cfg(getCtx()));
    } else if (router == "router2") {
        router2(getCtx(), Router2Cfg(getCtx()));
        result = true;
    } else {
        log_error("Fabulous architecture does not support router '%s'\n", router.c_str());
    }
    getCtx()->settings[getCtx()->id("route")] = 1;
    archInfoToAttributes();
    return result;
}

// ---------------------------------------------------------------

const std::vector<GraphicElement> &Arch::getDecalGraphics(DecalId decal) const
{
    if (!decal_graphics.count(decal)) {
        std::cerr << "No decal named " << decal.str(getCtx()) << std::endl;
    }
    return decal_graphics.at(decal);
}

DecalXY Arch::getBelDecal(BelId bel) const { return bels.at(bel).decalxy; }

DecalXY Arch::getWireDecal(WireId wire) const { return wires.at(wire).decalxy; }

DecalXY Arch::getPipDecal(PipId pip) const { return pips.at(pip).decalxy; }

DecalXY Arch::getGroupDecal(GroupId group) const { return groups.at(group).decalxy; }

// ---------------------------------------------------------------

bool Arch::getCellDelay(const CellInfo *cell, IdString fromPort, IdString toPort, DelayQuad &delay) const
{
    if (!cellTiming.count(cell->name))
        return false;
    const auto &tmg = cellTiming.at(cell->name);
    auto fnd = tmg.combDelays.find(CellDelayKey{fromPort, toPort});
    if (fnd != tmg.combDelays.end()) {
        delay = fnd->second;
        return true;
    } else {
        return false;
    }
}

// Get the port class, also setting clockPort if applicable
TimingPortClass Arch::getPortTimingClass(const CellInfo *cell, IdString port, int &clockInfoCount) const
{
    if (!cellTiming.count(cell->name))
        return TMG_IGNORE;
    const auto &tmg = cellTiming.at(cell->name);
    if (tmg.clockingInfo.count(port))
        clockInfoCount = int(tmg.clockingInfo.at(port).size());
    else
        clockInfoCount = 0;
    return get_or_default(tmg.portClasses, port, TMG_IGNORE);
}

TimingClockingInfo Arch::getPortClockingInfo(const CellInfo *cell, IdString port, int index) const
{
    NPNR_ASSERT(cellTiming.count(cell->name));
    const auto &tmg = cellTiming.at(cell->name);
    NPNR_ASSERT(tmg.clockingInfo.count(port));
    return tmg.clockingInfo.at(port).at(index);
}

bool Arch::isBelLocationValid(BelId bel) const
{
    std::vector<const CellInfo *> cells;
    Loc loc = getBelLocation(bel);
    for (auto tbel : getBelsByTile(loc.x, loc.y)) {
        CellInfo *bound = getBoundBelCell(tbel);
        if (bound != nullptr)
            cells.push_back(bound);
    }
    return cellsCompatible(cells.data(), int(cells.size()));
}

/* bool Arch::isBelLocationValid(BelId bel) const
{
    if (getBelType(bel) == id("FABULOUS_LC")) {
        std::array<const CellInfo *, 9> bel_cells;
        size_t num_cells = 0;
        Loc bel_loc = getBelLocation(bel);
        for (auto bel_other : getBelsByTile(bel_loc.x, bel_loc.y)) {
            CellInfo *ci_other = getBoundBelCell(bel_other);
            if (ci_other != nullptr)
                bel_cells[num_cells++] = ci_other;
        }
        return logic_cells_compatible(bel_cells.data(), num_cells);
    } else {
        CellInfo *cell = getBoundBelCell(bel);
        if (cell == nullptr)
            return true;
        else if (cell->type == id_SB_IO) {
            // Do not allow placement of input SB_IOs on blocks where there a PLL is outputting to.

            // Find shared PLL by looking for driving bel siblings from D_IN_0
            // that are a PLL clock output.
            auto wire = getBelPinWire(bel, id_D_IN_0);
            for (auto pin : getWireBelPins(wire)) {
                if (pin.pin == id_PLLOUT_A || pin.pin == id_PLLOUT_B) {
                    // Is there a PLL there ?
                    auto pll_cell = getBoundBelCell(pin.bel);
                    if (pll_cell == nullptr)
                        break;

                    // Is that port actually used ?
                    if ((pin.pin == id_PLLOUT_B) && !is_sb_pll40_dual(this, pll_cell))
                        break;

                    // Is that SB_IO used at an input ?
                    if ((cell->ports[id_D_IN_0].net == nullptr) && (cell->ports[id_D_IN_1].net == nullptr))
                        break;

                    // Are we perhaps a PAD INPUT Bel that can be placed here?
                    if (pll_cell->attrs[id("BEL_PAD_INPUT")] == getBelName(bel).str(getCtx()))
                        return true;

                    // Conflict
                    return false;
                }
            }

            Loc ioLoc = getBelLocation(bel);
            Loc compLoc = ioLoc;
            compLoc.z = 1 - compLoc.z;

            // Check LVDS pairing
            if (cell->ioInfo.lvds) {
                // Check correct z and complement location is free
                if (ioLoc.z != 0)
                    return false;
                BelId compBel = getBelByLocation(compLoc);
                CellInfo *compCell = getBoundBelCell(compBel);
                if (compCell)
                    return false;
            } else {
                // Check LVDS IO is not placed at complement location
                BelId compBel = getBelByLocation(compLoc);
                CellInfo *compCell = getBoundBelCell(compBel);
                if (compCell && compCell->ioInfo.lvds)
                    return false;

                // Check for conflicts on shared nets
                // - CLOCK_ENABLE
                // - OUTPUT_CLK
                // - INPUT_CLK
                if (compCell) {
                    bool use[6] = {
                            _io_pintype_need_clk_in(cell->ioInfo.pintype),
                            _io_pintype_need_clk_in(compCell->ioInfo.pintype),
                            _io_pintype_need_clk_out(cell->ioInfo.pintype),
                            _io_pintype_need_clk_out(compCell->ioInfo.pintype),
                            _io_pintype_need_clk_en(cell->ioInfo.pintype),
                            _io_pintype_need_clk_en(compCell->ioInfo.pintype),
                    };
                    NetInfo *nets[] = {
                            cell->ports[id_INPUT_CLK].net,    compCell->ports[id_INPUT_CLK].net,
                            cell->ports[id_OUTPUT_CLK].net,   compCell->ports[id_OUTPUT_CLK].net,
                            cell->ports[id_CLOCK_ENABLE].net, compCell->ports[id_CLOCK_ENABLE].net,
                    };

                    for (int i = 0; i < 6; i++)
                        if (use[i] && (nets[i] != nets[i ^ 1]) && (use[i ^ 1] || (nets[i ^ 1] != nullptr)))
                            return false;
                }
            }

            return get_bel_package_pin(bel) != "";
        } else if (cell->type == id_SB_GB) {
            if (cell->gbInfo.forPadIn)
                return true;
            NPNR_ASSERT(cell->ports.at(id_GLOBAL_BUFFER_OUTPUT).net != nullptr);
            const NetInfo *net = cell->ports.at(id_GLOBAL_BUFFER_OUTPUT).net;
            int glb_id = get_driven_glb_netwk(bel);
            if (net->is_reset && net->is_enable)
                return false;
            else if (net->is_reset)
                return (glb_id % 2) == 0;
            else if (net->is_enable)
                return (glb_id % 2) == 1;
            else
                return true;
        }  else {
            // TODO: IO cell clock checks
            return true;
        }
    }
} */

#ifdef WITH_HEAP
const std::string Arch::defaultPlacer = "heap";
#else
const std::string Arch::defaultPlacer = "sa";
#endif

const std::vector<std::string> Arch::availablePlacers = {"sa",
#ifdef WITH_HEAP
                                                         "heap"
#endif
};

const std::string Arch::defaultRouter = "router1";
const std::vector<std::string> Arch::availableRouters = {"router1", "router2"};

bool Arch::is_global_net(const NetInfo *net) const
{
    if (net == nullptr)
        return false;
    return net->driver.cell != nullptr && net->driver.port == id("GLOBAL_BUFFER_OUTPUT");
}

void Arch::assignArchInfo()
{
    for (auto &net : getCtx()->nets) {
        NetInfo *ni = net.second.get();
        if (is_global_net(ni))
            ni->is_global = true;
        ni->is_enable = false;
        ni->is_reset = false;
        for (auto usr : ni->users) {
            if (is_enable_port(this, usr))
                ni->is_enable = true;
            if (is_reset_port(this, usr))
                ni->is_reset = true;
        }
    }
    
    for (auto &cell : getCtx()->cells) {
        CellInfo *ci = cell.second.get();
        if (ci->type == id("FABULOUS_LC")) {
            ci->is_slice = true;
            ci->slice_clk = get_net_or_empty(ci, id("CLK"));
            ci->en = get_net_or_empty(ci, id("EN"));
            ci->sr = get_net_or_empty(ci, id("SR"));
            /* ci->dffEnable = bool_or_default(ci->params, id("DFF_ENABLE"));
            ci->inputCount = 0;
            if (get_net_or_empty(ci, id("I0")))
                ci->inputCount++;
            if (get_net_or_empty(ci, id("I1")))
                ci->inputCount++;
            if (get_net_or_empty(ci, id("I2")))
                ci->inputCount++;
            if (get_net_or_empty(ci, id("I3")))
                ci->inputCount++; */
        } else {
            ci->is_slice = false;
        }
        ci->user_group = int_or_default(ci->attrs, id("PACK_GROUP"), -1);
        // If no manual cell->bel pin rule has been created; assign a default one
        for (auto &p : ci->ports)
            if (!ci->bel_pins.count(p.first))
                ci->bel_pins.emplace(p.first, std::vector<IdString>{p.first});
    }
}

bool Arch::cellsCompatible(const CellInfo **cells, int count) const
{
    const NetInfo *clk = nullptr;
    const NetInfo *en = nullptr, *sr = nullptr;
    int group = -1;
    for (int i = 0; i < count; i++) {
        const CellInfo *ci = cells[i];
        if (ci->is_slice && (ci->slice_clk != nullptr)) {
            if (clk == nullptr)
                clk = ci->slice_clk;
            else if (clk != ci->slice_clk)
                return false;
        }
        if (ci->is_slice && (ci->en != nullptr)) {
            if (en == nullptr)
                en = ci->en;
            else if (en != ci->en)
                return false;
        }
        if (ci->is_slice && (ci->sr != nullptr)) {
            if (sr == nullptr)
                sr = ci->sr;
            else if (sr != ci->sr)
                return false;
        }
        if (ci->user_group != -1) {
            if (group == -1)
                group = ci->user_group;
            else if (group != ci->user_group)
                return false;
        }
    }
    return true;
}

/* bool Arch::logic_cells_compatible(const CellInfo **it, const size_t size) const
{
    bool dffs_exist = false;
    const NetInfo *en = nullptr, *sr = nullptr, *clk = nullptr;
    int locals_count = 0;

    for (auto cell : boost::make_iterator_range(it, it + size)) {
        if (cell->type != id("MUX8LUT_frame_config")){
            NPNR_ASSERT(cell->type == id("FABULOUS_LC"));
            if (cell->dffEnable) {
                if (!dffs_exist) {
                    dffs_exist = true;
                    clk = cell->slice_clk;
                    en = cell->en;
                    sr = cell->sr;
    
                    if (en != nullptr && !en->is_global)
                        locals_count++;
                    if (clk != nullptr && !clk->is_global)
                        locals_count++;
                    if (sr != nullptr && !sr->is_global)
                        locals_count++;
                    
                } else {
                    if (clk != cell->slice_clk)
                        return false;
                    if (en != cell->en)
                        return false;
                    if (sr != cell->sr)
                        return false;
                }
            }
        }

        locals_count += cell->inputCount;
    }

    return locals_count <= 56;
} */

NEXTPNR_NAMESPACE_END
