/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018-19  David Shah <david@symbioticeda.com>
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

#include <algorithm>
#include <iterator>
#include "cells.h"
#include "design_utils.h"
#include "log.h"
#include "util.h"

NEXTPNR_NAMESPACE_BEGIN

// Pack LUTs and LUT-FF pairs
static void pack_lut_lutffs(Context *ctx)
{
    log_info("Packing LUT-FFs..\n");

    pool<IdString> packed_cells;
    std::vector<std::unique_ptr<CellInfo>> new_cells;
    for (auto &cell : ctx->cells) {
        CellInfo *ci = cell.second.get();
        if (ctx->verbose)
            log_info("cell '%s' is of type '%s'\n", ci->name.c_str(ctx), ci->type.c_str(ctx));
        if (is_lut(ctx, ci)) {
            std::unique_ptr<CellInfo> packed =
                    create_fabulous_cell(ctx, ctx->id("FABULOUS_LC"), ci->name.str(ctx) + "_LC");
            for (auto &attr : ci->attrs)
                packed->attrs[attr.first] = attr.second;
            packed_cells.insert(ci->name);
            if (ctx->verbose)
                log_info("packed cell %s into %s\n", ci->name.c_str(ctx), packed->name.c_str(ctx));
            // See if we can pack into a DFF
            // TODO: LUT cascade
            NetInfo *o = ci->ports.at(ctx->id("O")).net;
            CellInfo *dff = net_only_drives(ctx, o, is_ff, ctx->id("D"), true);
            auto lut_bel = ci->attrs.find(ctx->id("BEL"));
            bool packed_dff = false;
            if (dff) {
                if (ctx->verbose)
                    log_info("found attached dff %s\n", dff->name.c_str(ctx));
                auto dff_bel = dff->attrs.find(ctx->id("BEL"));
                if (lut_bel != ci->attrs.end() && dff_bel != dff->attrs.end() && lut_bel->second != dff_bel->second) {
                    // Locations don't match, can't pack
                } else {
                    lut_to_lc(ctx, ci, packed.get(), false);
                    dff_to_lc(ctx, dff, packed.get(), false);
                    ctx->nets.erase(o->name);
                    if (dff_bel != dff->attrs.end())
                        packed->attrs[ctx->id("BEL")] = dff_bel->second;
                    packed_cells.insert(dff->name);
                    if (ctx->verbose)
                        log_info("packed cell %s into %s\n", dff->name.c_str(ctx), packed->name.c_str(ctx));
                    packed_dff = true;
                }
            }
            if (!packed_dff) {
                lut_to_lc(ctx, ci, packed.get(), true);
            }
            new_cells.push_back(std::move(packed));
        }
    }
    for (auto pcell : packed_cells) {
        ctx->cells.erase(pcell);
    }
    for (auto &ncell : new_cells) {
        ctx->cells[ncell->name] = std::move(ncell);
    }
}

// Pack FFs not packed as LUTFFs
static void pack_nonlut_ffs(Context *ctx)
{
    log_info("Packing non-LUT FFs..\n");

    pool<IdString> packed_cells;
    std::vector<std::unique_ptr<CellInfo>> new_cells;

    for (auto &cell : ctx->cells) {
        CellInfo *ci = cell.second.get();
        if (is_ff(ctx, ci)) {
            std::unique_ptr<CellInfo> packed =
                    create_fabulous_cell(ctx, ctx->id("FABULOUS_LC"), ci->name.str(ctx) + "_DFFLC");
            for (auto &attr : ci->attrs)
                packed->attrs[attr.first] = attr.second;
            if (ctx->verbose)
                log_info("packed cell %s into %s\n", ci->name.c_str(ctx), packed->name.c_str(ctx));
            packed_cells.insert(ci->name);
            dff_to_lc(ctx, ci, packed.get(), true);
            new_cells.push_back(std::move(packed));
        }
    }
    for (auto pcell : packed_cells) {
        ctx->cells.erase(pcell);
    }
    for (auto &ncell : new_cells) {
        ctx->cells[ncell->name] = std::move(ncell);
    }
}

// Merge a net into a constant net
static void set_net_constant(const Context *ctx, NetInfo *orig, NetInfo *constnet, bool constval)
{
    orig->driver.cell = nullptr;
    for (auto user : orig->users) {
        if (user.cell != nullptr) {
            CellInfo *uc = user.cell;
            if (ctx->verbose)
                log_info("%s user %s\n", orig->name.c_str(ctx), uc->name.c_str(ctx));
            if ((is_lut(ctx, uc) || is_lc(ctx, uc)) && (user.port.str(ctx).at(0) == 'I') && !constval) {
                uc->ports[user.port].net = nullptr;
            } else {
                uc->ports[user.port].net = constnet;
                constnet->users.push_back(user);
            }
        }
    }
    orig->users.clear();
}

// Pack constants (simple implementation)
static void pack_constants(Context *ctx)
{
    log_info("Packing constants..\n");

    std::unique_ptr<CellInfo> gnd_cell = create_fabulous_cell(ctx, ctx->id("FABULOUS_LC"), "$PACKER_GND");
    gnd_cell->params[ctx->id("INIT")] = Property(0, 1 << ctx->args.K);
    std::unique_ptr<NetInfo> gnd_net = std::unique_ptr<NetInfo>(new NetInfo);
    gnd_net->name = ctx->id("$PACKER_GND_NET");
    gnd_net->driver.cell = gnd_cell.get();
    gnd_net->driver.port = ctx->id("O");
    gnd_cell->ports.at(ctx->id("O")).net = gnd_net.get();

    std::unique_ptr<CellInfo> vcc_cell = create_fabulous_cell(ctx, ctx->id("FABULOUS_LC"), "$PACKER_VCC");
    // Fill with 1s
    vcc_cell->params[ctx->id("INIT")] = Property(Property::S1).extract(0, (1 << ctx->args.K), Property::S1);
    std::unique_ptr<NetInfo> vcc_net = std::unique_ptr<NetInfo>(new NetInfo);
    vcc_net->name = ctx->id("$PACKER_VCC_NET");
    vcc_net->driver.cell = vcc_cell.get();
    vcc_net->driver.port = ctx->id("O");
    vcc_cell->ports.at(ctx->id("O")).net = vcc_net.get();

    std::vector<IdString> dead_nets;

    bool gnd_used = false, vcc_used = false;

    for (auto &net : ctx->nets) {
        NetInfo *ni = net.second.get();
        if (ni->driver.cell != nullptr && ni->driver.cell->type == ctx->id("GND")) {
            IdString drv_cell = ni->driver.cell->name;
            set_net_constant(ctx, ni, gnd_net.get(), false);
            gnd_used = true;
            dead_nets.push_back(net.first);
            ctx->cells.erase(drv_cell);
        } else if (ni->driver.cell != nullptr && ni->driver.cell->type == ctx->id("VCC")) {
            IdString drv_cell = ni->driver.cell->name;
            set_net_constant(ctx, ni, vcc_net.get(), true);
            vcc_used = true;
            dead_nets.push_back(net.first);
            ctx->cells.erase(drv_cell);
        }
    }

    if (gnd_used) {
        ctx->cells[gnd_cell->name] = std::move(gnd_cell);
        ctx->nets[gnd_net->name] = std::move(gnd_net);
    }

    if (vcc_used) {
        ctx->cells[vcc_cell->name] = std::move(vcc_cell);
        ctx->nets[vcc_net->name] = std::move(vcc_net);
    }

    for (auto dn : dead_nets) {
        ctx->nets.erase(dn);
    }
}

static bool is_nextpnr_iob(Context *ctx, CellInfo *cell)
{
    return cell->type == ctx->id("$nextpnr_ibuf") || cell->type == ctx->id("$nextpnr_obuf") ||
           cell->type == ctx->id("$nextpnr_iobuf");
}

static bool is_fabulous_iob(const Context *ctx, const CellInfo *cell) { return cell->type == ctx->id("IO_1_bidirectional_frame_config_pass"); }

// Pack IO buffers
static void pack_io(Context *ctx)
{
    pool<IdString> packed_cells;
    pool<IdString> delete_nets;

    std::vector<std::unique_ptr<CellInfo>> new_cells;
    log_info("Packing IOs..\n");

    for (auto &cell : ctx->cells) {
        CellInfo *ci = cell.second.get();
        if (is_nextpnr_iob(ctx, ci)) {
            CellInfo *iob = nullptr;
            if (ci->type == ctx->id("$nextpnr_ibuf") || ci->type == ctx->id("$nextpnr_iobuf")) {
                iob = net_only_drives(ctx, ci->ports.at(ctx->id("O")).net, is_fabulous_iob, ctx->id("PAD"), true, ci);

            } else if (ci->type == ctx->id("$nextpnr_obuf")) {
                NetInfo *net = ci->ports.at(ctx->id("I")).net;
                iob = net_only_drives(ctx, net, is_fabulous_iob, ctx->id("PAD"), true, ci);
            }
            if (iob != nullptr) {
                // Trivial case, FABULOUS_IOB used. Just destroy the net and the
                // iobuf
                log_info("%s feeds FABULOUS_IOB %s, removing %s %s.\n", ci->name.c_str(ctx), iob->name.c_str(ctx),
                         ci->type.c_str(ctx), ci->name.c_str(ctx));
                NetInfo *net = iob->ports.at(ctx->id("PAD")).net;
                if (((ci->type == ctx->id("$nextpnr_ibuf") || ci->type == ctx->id("$nextpnr_iobuf")) &&
                     net->users.size() > 1) ||
                    (ci->type == ctx->id("$nextpnr_obuf") && (net->users.size() > 2 || net->driver.cell != nullptr)))
                    log_error("PAD of %s '%s' connected to more than a single top level IO.\n", iob->type.c_str(ctx),
                              iob->name.c_str(ctx));

                if (net != nullptr) {
                    delete_nets.insert(net->name);
                    iob->ports.at(ctx->id("PAD")).net = nullptr;
                }
                if (ci->type == ctx->id("$nextpnr_iobuf")) {
                    NetInfo *net2 = ci->ports.at(ctx->id("I")).net;
                    if (net2 != nullptr) {
                        delete_nets.insert(net2->name);
                    }
                }
            } else if (bool_or_default(ctx->settings, ctx->id("disable_iobs"))) {
                // No IO buffer insertion; just remove nextpnr_[io]buf
                for (auto &p : ci->ports)
                    disconnect_port(ctx, ci, p.first);
            } else {
                // Create a FABULOUS_IOB buffer
                std::unique_ptr<CellInfo> ice_cell =
                        create_fabulous_cell(ctx, ctx->id("IO_1_bidirectional_frame_config_pass"), ci->name.str(ctx) + "$iob");
                nxio_to_iob(ctx, ci, ice_cell.get(), packed_cells);
                new_cells.push_back(std::move(ice_cell));
                iob = new_cells.back().get();
            }
            packed_cells.insert(ci->name);
            if (iob != nullptr)
                for (auto &attr : ci->attrs)
                    iob->attrs[attr.first] = attr.second;
        }
    }
    for (auto pcell : packed_cells) {
        ctx->cells.erase(pcell);
    }
    for (auto dnet : delete_nets) {
        ctx->nets.erase(dnet);
    }
    for (auto &ncell : new_cells) {
        ctx->cells[ncell->name] = std::move(ncell);
    }
}

static void insert_global(Context *ctx, NetInfo *net, bool is_reset, bool is_enable, int fanout)
{
    log_info("promoting %s%s%s (fanout %d)\n", net->name.c_str(ctx), is_reset ? " [reset]" : "", is_enable ? " [enable]" : "", fanout);

    std::string glb_name = net->name.str(ctx) + std::string("_$glb_") + (is_reset ? "sr" : "en");
    std::unique_ptr<CellInfo> gb = create_fabulous_cell(ctx, ctx->id("FABULOUS_GB"), "$gbuf_" + glb_name);
    gb->ports[ctx->id("USER_SIGNAL_TO_GLOBAL_BUFFER")].net = net;
    PortRef pr;
    pr.cell = gb.get();
    pr.port = ctx->id("USER_SIGNAL_TO_GLOBAL_BUFFER");
    net->users.push_back(pr);

    pr.cell = gb.get();
    pr.port = ctx->id("GLOBAL_BUFFER_OUTPUT");
    std::unique_ptr<NetInfo> glbnet = std::unique_ptr<NetInfo>(new NetInfo());
    glbnet->name = ctx->id(glb_name);
    glbnet->driver = pr;
    gb->ports[ctx->id("GLOBAL_BUFFER_OUTPUT")].net = glbnet.get();
    std::vector<PortRef> keep_users;
    for (auto user : net->users) {
        if ((is_reset && is_reset_port(ctx, user)) || (is_enable && is_enable_port(ctx, user)) ) 
        {
            user.cell->ports[user.port].net = glbnet.get();
            glbnet->users.push_back(user);
        } else {
            keep_users.push_back(user);
        }
    }
    net->users = keep_users;

    ctx->nets[glbnet->name] = std::move(glbnet);
    ctx->cells[gb->name] = std::move(gb);
}

// Simple global promoter (clock only)
static void promote_globals(Context *ctx)
{
    log_info("Promoting globals..\n");
    const int enable_fanout_thresh = 15;
    const int reset_fanout_thresh = 15;
    std::map<IdString, int> reset_count, enable_count;
    for (auto &net : ctx->nets) {
        NetInfo *ni = net.second.get();
        if (ni->driver.cell != nullptr && !ctx->is_global_net(ni)) {
            reset_count[net.first] = 0;
            enable_count[net.first] = 0;

            for (auto user : ni->users) {
                if (is_reset_port(ctx, user))
                    reset_count[net.first]++;
                if (is_enable_port(ctx, user))
                    enable_count[net.first]++;
            }
        }
    }
    int prom_globals = 0, prom_resets = 0, prom_enables = 0;
    int gbs_available = 8, resets_available = 4, enables_available = 4;
    for (auto &cell : ctx->cells)
        if (is_gbuf(ctx, cell.second.get())) {
            /* One less buffer available */
            --gbs_available;

            /* And possibly limits what we can promote */
            if (cell.second->attrs.find(ctx->id("BEL")) != cell.second->attrs.end()) {
                /* If the SB_GB is locked, doesn't matter what it drives */
                BelId bel = ctx->getBelByNameStr(cell.second->attrs[ctx->id("BEL")].as_string());
                int glb_id = ctx->get_driven_glb_netwk(bel);
                if ((glb_id % 2) == 0)
                    resets_available--;
                else if ((glb_id % 2) == 1)
                    enables_available--;
            } else {
                /* If it's free to move around, then look at what it drives */
                NetInfo *ni = cell.second->ports[ctx->id("GLOBAL_BUFFER_OUTPUT")].net;

                for (auto user : ni->users) {
                    if (is_reset_port(ctx, user)) {
                        resets_available--;
                        break;
                    } else if (is_enable_port(ctx, user)) {
                        enables_available--;
                        break;
                    }
                }
            }
        }
    while (prom_globals < gbs_available) {
        auto global_reset = std::max_element(reset_count.begin(), reset_count.end(),
                                             [](const std::pair<IdString, int> &a, const std::pair<IdString, int> &b) {
                                                 return a.second < b.second;
                                             });
        auto global_enable = std::max_element(enable_count.begin(), enable_count.end(),
                                           [](const std::pair<IdString, int> &a, const std::pair<IdString, int> &b) {
                                               return a.second < b.second;
                                           });
        if (prom_resets < resets_available && global_reset->second > reset_fanout_thresh) {
            NetInfo *rstnet = ctx->nets[global_reset->first].get();
            insert_global(ctx, rstnet, true, false, global_reset->second);
            ++prom_globals;
            ++prom_resets;
            reset_count.erase(rstnet->name);
            enable_count.erase(rstnet->name);
        } else if (prom_enables < enables_available && global_enable->second > enable_fanout_thresh) {
            NetInfo *enablenet = ctx->nets[global_enable->first].get();
            insert_global(ctx, enablenet, false, true, global_enable->second);
            ++prom_globals;
            ++prom_enables;
            reset_count.erase(enablenet->name);
            enable_count.erase(enablenet->name);
        } else {
            break;
        }
    }
}

// Main pack function
bool Arch::pack()
{
    Context *ctx = getCtx();
    try {
        log_break();
        pack_constants(ctx);
        pack_io(ctx);
        pack_lut_lutffs(ctx);
        pack_nonlut_ffs(ctx);
        //promote_globals(ctx);
        ctx->settings[ctx->id("pack")] = 1;
        ctx->assignArchInfo();
        log_info("Checksum: 0x%08x\n", ctx->checksum());
        return true;
    } catch (log_execution_error_exception) {
        return false;
    }
}

NEXTPNR_NAMESPACE_END
