/*
bellhopcxx / bellhopcuda - C++/CUDA port of BELLHOP underwater acoustics simulator
Copyright (C) 2021-2023 The Regents of the University of California
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
#include "../common_setup.hpp"
#include "ray.hpp"
#include "field.hpp"

namespace bhc { namespace mode {

template<bool O3D, bool R3D> void PostProcessEigenrays(
    const bhcParams<O3D, R3D> &params, bhcOutputs<O3D, R3D> &outputs);
extern template void PostProcessEigenrays<false, false>(
    const bhcParams<false, false> &params, bhcOutputs<false, false> &outputs);
extern template void PostProcessEigenrays<true, false>(
    const bhcParams<true, false> &params, bhcOutputs<true, false> &outputs);
extern template void PostProcessEigenrays<true, true>(
    const bhcParams<true, true> &params, bhcOutputs<true, true> &outputs);

template<bool O3D, bool R3D> class Eigen : public Field<O3D, R3D> {
public:
    Eigen() {}
    virtual ~Eigen() {}

    virtual void Init(bhcOutputs<O3D, R3D> &outputs) const
    {
        outputs.eigen->hits    = nullptr;
        outputs.eigen->neigen  = 0;
        outputs.eigen->memsize = 0;
    }

    virtual void Preprocess(
        bhcParams<O3D, R3D> &params, bhcOutputs<O3D, R3D> &outputs) const
    {
        Field<O3D, R3D>::Preprocess(params, outputs);

        EigenInfo *eigen = outputs.eigen;
        trackdeallocate(params, eigen->hits); // Free memory if previously run
        // Use 1 / hitsMemFraction of the available memory for eigenray hits
        // (the rest for rays).
        constexpr size_t hitsMemFraction = 500;
        size_t mem     = GetInternal(params)->maxMemory - GetInternal(params)->usedMemory;
        eigen->memsize = mem / (hitsMemFraction * sizeof(EigenHit));
        if(eigen->memsize == 0) {
            EXTERR("Insufficient memory to allocate any eigen hits at all");
        } else if(eigen->memsize < 10000) {
            EXTWARN(
                "There is only enough memory to allocate %d eigen hits, using 1/%dth "
                "of the total available memory for eigen hits",
                eigen->memsize, hitsMemFraction);
        }
        trackallocate(params, "eigenray hits", eigen->hits, eigen->memsize);
        eigen->neigen = 0;
    }

    virtual void Postprocess(
        bhcParams<O3D, R3D> &params, bhcOutputs<O3D, R3D> &outputs) const
    {
        PostProcessEigenrays<O3D, R3D>(params, outputs);
    }

    virtual void Writeout(
        bhcParams<O3D, R3D> &params, bhcOutputs<O3D, R3D> &outputs) const
    {
        Ray<O3D, R3D> raymode;
        raymode.Writeout(params, outputs);
    }

    virtual void Finalize(
        bhcParams<O3D, R3D> &params, bhcOutputs<O3D, R3D> &outputs) const
    {
        trackdeallocate(params, outputs.eigen->hits);
    }
};

}} // namespace bhc::mode
