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

#include "nextpnr.h"

#ifndef FABULOUS_CELLS_H
#define FABULOUS_CELLS_H

NEXTPNR_NAMESPACE_BEGIN

// Create a generic arch cell and return it
// Name will be automatically assigned if not specified
std::unique_ptr<CellInfo> create_fabulous_cell(Context *ctx, IdString type, std::string name = "");

// Return true if a cell is a LUT
inline bool is_lut(const BaseCtx *ctx, const CellInfo *cell) { return cell->type == ctx->id("LUT1") || cell->type == ctx->id("LUT2") || cell->type == ctx->id("LUT3") || cell->type == ctx->id("LUT4"); }

// Return true if a cell is a flipflop
inline bool is_ff(const BaseCtx *ctx, const CellInfo *cell) 
{
    return  cell->type == ctx->id("LUTFF") || cell->type == ctx->id("LUTFF_E") ||
            cell->type == ctx->id("LUTFF_SR") || cell->type == ctx->id("LUTFF_SS") ||
            cell->type == ctx->id("LUTFF_ESR") || cell->type == ctx->id("LUTFF_ESS"); 
}

inline bool is_lc(const BaseCtx *ctx, const CellInfo *cell) { return cell->type == ctx->id("FABULOUS_LC"); }

// Return true if a cell is a global buffer
inline bool is_gbuf(const BaseCtx *ctx, const CellInfo *cell) { return cell->type == ctx->id("FABULOUS_GB"); }

// Convert a LUT primitive to (part of) an GENERIC_SLICE, swapping ports
// as needed. Set no_dff if a DFF is not being used, so that the output
// can be reconnected
void lut_to_lc(const Context *ctx, CellInfo *lut, CellInfo *lc, bool no_dff = true);

// Convert a DFF primitive to (part of) an GENERIC_SLICE, setting parameters
// and reconnecting signals as necessary. If pass_thru_lut is True, the LUT will
// be configured as pass through and D connected to I0, otherwise D will be
// ignored
void dff_to_lc(const Context *ctx, CellInfo *dff, CellInfo *lc, bool pass_thru_lut = false);

// Convert a nextpnr IO buffer to a GENERIC_IOB
void nxio_to_iob(Context *ctx, CellInfo *nxio, CellInfo *sbio, pool<IdString> &todelete_cells);

// Return true if a port is a reset port
bool is_reset_port(const BaseCtx *ctx, const PortRef &port);

// Return true if a port is a clock enable port
bool is_enable_port(const BaseCtx *ctx, const PortRef &port);

NEXTPNR_NAMESPACE_END

#endif
