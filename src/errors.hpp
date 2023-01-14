/*
bellhopcxx / bellhopcuda - C++/CUDA port of BELLHOP underwater acoustics simulator
Copyright (C) 2021-2022 The Regents of the University of California
c/o Jules Jaffe team at SIO / UCSD, jjaffe@ucsd.edu
Based on BELLHOP, which is Copyright (C) 1983-2020 Michael B. Porter

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#ifndef _BHC_INCLUDING_COMPONENTS_
#error "Must be included from common.hpp!"
#endif

#include <cstdarg>

namespace bhc {

struct bhcInternal;

struct ErrState {
    STD::atomic<uint32_t> error, warning, errCount, warnCount;
};

extern [[noreturn]] void ExternalError(bhcInternal *internal, const char *format, ...);
extern void ExternalWarning(bhcInternal *internal, const char *format, ...);
#define EXTERR(...) ExternalError(GetInternal(params), __VA_ARGS__)
#define EXTWARN(...) ExternalWarning(GetInternal(params), __VA_ARGS__)

inline HOST_DEVICE void RunError(ErrState *errState, uint32_t code)
{
    errState->errCount.fetch_add(1u, STD::memory_order_relaxed);
    errState->error.fetch_or(1u << code, STD::memory_order_release);
}
inline HOST_DEVICE void RunWarning(ErrState *errState, uint32_t code)
{
    errState->warnCount.fetch_add(1u, STD::memory_order_relaxed);
    errState->warning.fetch_or(1u << code, STD::memory_order_relaxed);
}
inline HOST_DEVICE bool HasErrored(ErrState *errState)
{
    return errState->error.load(STD::memory_order_acquire) != 0u;
}
inline HOST_DEVICE void ResetErrState(ErrState *errState)
{
    errState->error     = 0u;
    errState->warning   = 0u;
    errState->errCount  = 0u;
    errState->warnCount = 0u;
}
extern void CheckReportErrors(const ErrState *errState);

#define BHC_ERR_OUTSIDE_ALTIMETRY 1
#define BHC_ERR_OUTSIDE_BATHYMETRY 2
#define BHC_ERR_BOUNDARY_SEG_CONTAINS_NAN 3
#define BHC_ERR_INVALID_IMAGE_INDEX 4
#define BHC_ERR_TEMPLATE 5
#define BHC_ERR_BOUNDARY_CONDITION_TYPE 6
#define BHC_ERR_INVALID_SSP_TYPE 7
#define BHC_ERR_OUTSIDE_SSP 8
#define BHC_ERR_QUAD_ISEG 9
#define BHC_ERR_RAYINIT 10
#define BHC_ERR_JOBNUM 11

#define BHC_WARN_OUTSIDE_ALTIMETRY 1
#define BHC_WARN_OUTSIDE_BATHYMETRY 2
#define BHC_WARN_WKB_UNIMPLEMENTED_3D 3
#define BHC_WARN_CERVENY_WIDTH_BUGGY 4
#define BHC_WARN_INVALID_WIDTH_BUGGY 5
#define BHC_WARN_BEAMTYPE_CARETSPACE 6
#define BHC_WARN_INVALID_TYPE_BUGGY 7
#define BHC_WARN_UNBOUNDED_BEAM 8
#define BHC_WARN_OUTSIDE_REFLCOEF 9
#define BHC_WARN_RAYS_OUTOFMEMORY 10
#define BHC_WARN_CPCHIP_INVALIDXT 11
#define BHC_WARN_CPCHIP_INVALIDCCOEF 12
#define BHC_WARN_STEP_NEGATIVE_H 13
#define BHC_WARN_TOO_FEW_BEAMS 14
#define BHC_WARN_SOURCE_OUTSIDE_BOUNDARIES 15
#define BHC_WARN_ONERAY_OUTOFMEMORY 16
#define BHC_WARN_TRIDIAG_H_VERY_NEGATIVE 17
#define BHC_WARN_TRIDIAG_H_GROWING 18

} // namespace bhc
