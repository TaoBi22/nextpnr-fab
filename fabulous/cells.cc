/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2019  David Shah <david@symbioticeda.com>
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

#include "cells.h"
#include "design_utils.h"
#include "log.h"
#include "util.h"

NEXTPNR_NAMESPACE_BEGIN

void add_port(const Context *ctx, CellInfo *cell, std::string name, PortType dir)
{
    IdString id = ctx->id(name);
    NPNR_ASSERT(cell->ports.count(id) == 0);
    cell->ports[id] = PortInfo{id, nullptr, dir};
}

std::unique_ptr<CellInfo> create_fabulous_cell(Context *ctx, IdString type, std::string name)
{
    static int auto_idx = 0;
    std::unique_ptr<CellInfo> new_cell = std::unique_ptr<CellInfo>(new CellInfo());
    if (name.empty()) {
        new_cell->name = ctx->id("$nextpnr_" + type.str(ctx) + "_" + std::to_string(auto_idx++));
    } else {
        new_cell->name = ctx->id(name);
    }
    new_cell->type = type;
    if (type == ctx->id("FABULOUS_LC")) {
        //new_cell->type = ctx->id("LUT4");
        new_cell->params[ctx->id("K")] = ctx->args.K;
        new_cell->params[ctx->id("INIT")] = 0;
        new_cell->params[ctx->id("DFF_ENABLE")] = 0;
        new_cell->params[ctx->id("SET_NORESET")] = 0;
        new_cell->params[ctx->id("EN_USED")] = 0;
        new_cell->params[ctx->id("SR_USED")] = 0;

        add_port(ctx, new_cell.get(), "I0", PORT_IN);
        add_port(ctx, new_cell.get(), "I1", PORT_IN);
        add_port(ctx, new_cell.get(), "I2", PORT_IN);
        add_port(ctx, new_cell.get(), "I3", PORT_IN);
        add_port(ctx, new_cell.get(), "CIN", PORT_IN);
        add_port(ctx, new_cell.get(), "CLK", PORT_IN);
        add_port(ctx, new_cell.get(), "EN", PORT_IN);
        add_port(ctx, new_cell.get(), "SR", PORT_IN);
        add_port(ctx, new_cell.get(), "O", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q", PORT_OUT);
        //add_port(ctx, new_cell.get(), "OMUX", PORT_OUT);
        add_port(ctx, new_cell.get(), "COUT", PORT_OUT);
    } else if (type == ctx->id("LUT4AB_mux")){
        add_port(ctx, new_cell.get(), "O", PORT_OUT);
        add_port(ctx, new_cell.get(), "I0", PORT_IN);
        add_port(ctx, new_cell.get(), "I1", PORT_IN);
        add_port(ctx, new_cell.get(), "S", PORT_IN);
    } else if (type == ctx->id("IO_1_bidirectional_frame_config_pass")) {
        new_cell->params[ctx->id("INPUT_USED")] = 0;
        new_cell->params[ctx->id("OUTPUT_USED")] = 0;
        new_cell->params[ctx->id("ENABLE_USED")] = 0;
        //new_cell->type = ctx->id("IO_1_bidirectional_frame_config_pass");
        add_port(ctx, new_cell.get(), "I", PORT_IN);
        add_port(ctx, new_cell.get(), "Q", PORT_OUT);
        add_port(ctx, new_cell.get(), "O", PORT_OUT);
        add_port(ctx, new_cell.get(), "T", PORT_IN);
    } else if (type == ctx->id("InPass4_frame_config")) {
        //new_cell->type = ctx->id("InPass4_frame_config");
        add_port(ctx, new_cell.get(), "O0", PORT_OUT);
        add_port(ctx, new_cell.get(), "O1", PORT_OUT);
        add_port(ctx, new_cell.get(), "O2", PORT_OUT);
        add_port(ctx, new_cell.get(), "O3", PORT_OUT);
    } else if (type == ctx->id("OutPass4_frame_config")) {
        //new_cell->type = ctx->id("OutPass4_frame_config");
        add_port(ctx, new_cell.get(), "I0", PORT_IN);
        add_port(ctx, new_cell.get(), "I1", PORT_IN);
        add_port(ctx, new_cell.get(), "I2", PORT_IN);
        add_port(ctx, new_cell.get(), "I3", PORT_IN);
    } else if (type == ctx->id("BUFGCTRL")) {
        add_port(ctx, new_cell.get(), "I0", PORT_IN);
        add_port(ctx, new_cell.get(), "O", PORT_OUT);
     } else if (type == ctx->id("RegFile_32x4")){
        //new_cell->type = ctx->id("RegFile_32x4");
        add_port(ctx, new_cell.get(), "D0", PORT_IN);
        add_port(ctx, new_cell.get(), "D1", PORT_IN);
        add_port(ctx, new_cell.get(), "D2", PORT_IN);
        add_port(ctx, new_cell.get(), "D3", PORT_IN);
        add_port(ctx, new_cell.get(), "W_ADR0", PORT_IN);
        add_port(ctx, new_cell.get(), "W_ADR1", PORT_IN);
        add_port(ctx, new_cell.get(), "W_ADR2", PORT_IN);
        add_port(ctx, new_cell.get(), "W_ADR3", PORT_IN);
        add_port(ctx, new_cell.get(), "W_ADR4", PORT_IN);
        add_port(ctx, new_cell.get(), "W_en", PORT_IN);
        add_port(ctx, new_cell.get(), "AD0", PORT_OUT);
        add_port(ctx, new_cell.get(), "AD1", PORT_OUT);
        add_port(ctx, new_cell.get(), "AD2", PORT_OUT);
        add_port(ctx, new_cell.get(), "AD3", PORT_OUT);
        add_port(ctx, new_cell.get(), "A_ADR0", PORT_IN);
        add_port(ctx, new_cell.get(), "A_ADR1", PORT_IN);
        add_port(ctx, new_cell.get(), "A_ADR2", PORT_IN);
        add_port(ctx, new_cell.get(), "A_ADR3", PORT_IN);
        add_port(ctx, new_cell.get(), "A_ADR4", PORT_IN);
        add_port(ctx, new_cell.get(), "BD0", PORT_OUT);
        add_port(ctx, new_cell.get(), "BD1", PORT_OUT);
        add_port(ctx, new_cell.get(), "BD2", PORT_OUT);
        add_port(ctx, new_cell.get(), "BD3", PORT_OUT);
        add_port(ctx, new_cell.get(), "B_ADR0", PORT_IN);
        add_port(ctx, new_cell.get(), "B_ADR1", PORT_IN);
        add_port(ctx, new_cell.get(), "B_ADR2", PORT_IN);
        add_port(ctx, new_cell.get(), "B_ADR3", PORT_IN);
        add_port(ctx, new_cell.get(), "B_ADR4", PORT_IN);
    } else if (type == ctx->id("MULADD")){
        //new_cell->type = ctx->id("MULADD");
        add_port(ctx, new_cell.get(), "A0", PORT_IN);
        add_port(ctx, new_cell.get(), "A1", PORT_IN);
        add_port(ctx, new_cell.get(), "A2", PORT_IN);
        add_port(ctx, new_cell.get(), "A3", PORT_IN);
        add_port(ctx, new_cell.get(), "A4", PORT_IN);
        add_port(ctx, new_cell.get(), "A5", PORT_IN);
        add_port(ctx, new_cell.get(), "A6", PORT_IN);
        add_port(ctx, new_cell.get(), "A7", PORT_IN);
        add_port(ctx, new_cell.get(), "B0", PORT_IN);
        add_port(ctx, new_cell.get(), "B1", PORT_IN);
        add_port(ctx, new_cell.get(), "B2", PORT_IN);
        add_port(ctx, new_cell.get(), "B3", PORT_IN);
        add_port(ctx, new_cell.get(), "B4", PORT_IN);
        add_port(ctx, new_cell.get(), "B5", PORT_IN);
        add_port(ctx, new_cell.get(), "B6", PORT_IN);
        add_port(ctx, new_cell.get(), "B7", PORT_IN);
        add_port(ctx, new_cell.get(), "Q0", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q1", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q2", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q3", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q4", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q5", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q6", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q7", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q8", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q9", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q10", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q11", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q12", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q13", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q14", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q15", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q16", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q17", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q18", PORT_OUT);
        add_port(ctx, new_cell.get(), "Q19", PORT_OUT);
        add_port(ctx, new_cell.get(), "clr", PORT_IN);
        add_port(ctx, new_cell.get(), "C0", PORT_IN);
        add_port(ctx, new_cell.get(), "C1", PORT_IN);
        add_port(ctx, new_cell.get(), "C2", PORT_IN);
        add_port(ctx, new_cell.get(), "C3", PORT_IN);
        add_port(ctx, new_cell.get(), "C4", PORT_IN);
        add_port(ctx, new_cell.get(), "C5", PORT_IN);
        add_port(ctx, new_cell.get(), "C6", PORT_IN);
        add_port(ctx, new_cell.get(), "C7", PORT_IN);
        add_port(ctx, new_cell.get(), "C8", PORT_IN);
        add_port(ctx, new_cell.get(), "C9", PORT_IN);
        add_port(ctx, new_cell.get(), "C10", PORT_IN);
        add_port(ctx, new_cell.get(), "C11", PORT_IN);
        add_port(ctx, new_cell.get(), "C12", PORT_IN);
        add_port(ctx, new_cell.get(), "C13", PORT_IN);
        add_port(ctx, new_cell.get(), "C14", PORT_IN);
        add_port(ctx, new_cell.get(), "C15", PORT_IN);
        add_port(ctx, new_cell.get(), "C16", PORT_IN);
        add_port(ctx, new_cell.get(), "C17", PORT_IN);
        add_port(ctx, new_cell.get(), "C18", PORT_IN);
        add_port(ctx, new_cell.get(), "C19", PORT_IN); 
    } else if (type == ctx->id("FABULOUS_GB")) {
        add_port(ctx, new_cell.get(), "USER_SIGNAL_TO_GLOBAL_BUFFER", PORT_IN);
        add_port(ctx, new_cell.get(), "GLOBAL_BUFFER_OUTPUT", PORT_OUT);
    } else {
        log_error("unable to create fabulous cell of type %s", type.c_str(ctx));
    }
    return new_cell;
}

void lut_to_lc(const Context *ctx, CellInfo *lut, CellInfo *lc, bool no_dff)
{
    lc->params[ctx->id("INIT")] = lut->params[ctx->id("INIT")];

    int lut_k = int_or_default(lut->params, ctx->id("K"), 4);
    NPNR_ASSERT(lut_k <= ctx->args.K);

    //for (int i = 0; i < lut_k; i++) {
        //IdString port = ctx->id("I[" + std::to_string(i) + "]");
        //replace_port(lut, port, lc, port);
    //}
    replace_port(lut, ctx->id("I3"), lc, ctx->id("I3"));
    replace_port(lut, ctx->id("I2"), lc, ctx->id("I2"));
    replace_port(lut, ctx->id("I1"), lc, ctx->id("I1"));
    replace_port(lut, ctx->id("I0"), lc, ctx->id("I0"));

    if (no_dff) {
        //lc->params[ctx->id("FF_USED")] = 0;
        //replace_port(lut, ctx->id("Q"), lc, ctx->id("F"));
        replace_port(lut, ctx->id("O"), lc, ctx->id("O"));
    }
}

void dff_to_lc(const Context *ctx, CellInfo *dff, CellInfo *lc, bool pass_thru_lut)
{
    lc->params[ctx->id("DFF_ENABLE")] = 1;
    std::string config = dff->type.str(ctx).substr(5);
    auto citer = config.begin();
    replace_port(dff, ctx->id("CLK"), lc, ctx->id("CLK"));
    if (citer != config.end()) {
        ++citer;
    }
    if (citer != config.end() && *citer == 'E') {
        replace_port(dff, ctx->id("E"), lc, ctx->id("EN"));
        lc->params[ctx->id("EN_USED")] = 1;
        ++citer;
    }
    if (citer != config.end()) {
        if ((config.end() - citer) >= 2) {
            char c = *(citer++);
            NPNR_ASSERT(c == 'S');
            //lc->params[ctx->id("ASYNC_SR")] = Property::State::S0;
        } //else {
            //lc->params[ctx->id("ASYNC_SR")] = Property::State::S1;
        //}

        if (*citer == 'S') {
        citer++;
        replace_port(dff, ctx->id("S"), lc, ctx->id("SR"));
        lc->params[ctx->id("SET_NORESET")] = 1;
        lc->params[ctx->id("SR_USED")] = 1;
        } else {
        NPNR_ASSERT(*citer == 'R');
        citer++;
        replace_port(dff, ctx->id("R"), lc, ctx->id("SR"));
        lc->params[ctx->id("SET_NORESET")] = 0;
        lc->params[ctx->id("SR_USED")] = 1;
        }
    }
    
    NPNR_ASSERT(citer == config.end());

    if (pass_thru_lut) {
        // Fill LUT with alternating 10
        const int init_size = 1 << lc->params[ctx->id("K")].as_int64();
        std::string init;
        init.reserve(init_size);
        for (int i = 0; i < init_size; i += 2)
            init.append("10");
        lc->params[ctx->id("INIT")] = Property::from_string(init);

        //replace_port(dff, ctx->id("D"), lc, ctx->id("I[0]"));
        replace_port(dff, ctx->id("D"),lc, ctx->id("I0"));
    }

    replace_port(dff, ctx->id("O"), lc, ctx->id("Q"));
}

void nxio_to_iob(Context *ctx, CellInfo *nxio, CellInfo *iob, pool<IdString> &todelete_cells)
{
    if (nxio->type == ctx->id("$nextpnr_ibuf")) {
        iob->params[ctx->id("INPUT_USED")] = 1;
        replace_port(nxio, ctx->id("O"), iob, ctx->id("O"));
    } else if (nxio->type == ctx->id("$nextpnr_obuf")) {
        iob->params[ctx->id("OUTPUT_USED")] = 1;
        replace_port(nxio, ctx->id("I"), iob, ctx->id("I"));
    } else if (nxio->type == ctx->id("$nextpnr_iobuf")) {
        // N.B. tristate will be dealt with below
        iob->params[ctx->id("INPUT_USED")] = 1;
        iob->params[ctx->id("OUTPUT_USED")] = 1;
        replace_port(nxio, ctx->id("I"), iob, ctx->id("I"));
        replace_port(nxio, ctx->id("O"), iob, ctx->id("O"));
    } else {
        NPNR_ASSERT(false);
    }
    NetInfo *donet = iob->ports.at(ctx->id("I")).net;
    CellInfo *tbuf = net_driven_by(
            ctx, donet, [](const Context *ctx, const CellInfo *cell) { return cell->type == ctx->id("$_TBUF_"); },
            ctx->id("Y"));
    if (tbuf) {
        iob->params[ctx->id("ENABLE_USED")] = 1;
        replace_port(tbuf, ctx->id("A"), iob, ctx->id("I"));
        replace_port(tbuf, ctx->id("E"), iob, ctx->id("EN"));

        if (donet->users.size() > 1) {
            for (auto user : donet->users)
                log_info("     remaining tristate user: %s.%s\n", user.cell->name.c_str(ctx), user.port.c_str(ctx));
            log_error("unsupported tristate IO pattern for IO buffer '%s', "
                      "instantiate GENERIC_IOB manually to ensure correct behaviour\n",
                      nxio->name.c_str(ctx));
        }
        ctx->nets.erase(donet->name);
        todelete_cells.insert(tbuf->name);
    }
}

bool is_reset_port(const BaseCtx *ctx, const PortRef &port)
{
    if (port.cell == nullptr)
        return false;
    if (is_ff(ctx, port.cell))
        return port.port == ctx->id("R") || port.port == ctx->id("S");
    if (port.cell->type == ctx->id("FABULOUS_LC"))
        return port.port == ctx->id("SR");
    return false;
}

bool is_enable_port(const BaseCtx *ctx, const PortRef &port)
{
    if (port.cell == nullptr)
        return false;
    if (is_ff(ctx, port.cell))
        return port.port == ctx->id("E");
    if (port.cell->type == ctx->id("FABULOUS_LC"))
        return port.port == ctx->id("EN");
    return false;
}

NEXTPNR_NAMESPACE_END
