/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | cfMesh: A library for mesh generation
   \\    /   O peration     |
    \\  /    A nd           | www.cfmesh.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2014-2017 Creative Fields, Ltd.
-------------------------------------------------------------------------------
Author
     Franjo Juretic (franjo.juretic@c-fields.com)

License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "triangulateNonPlanarBaseFaces.H"
#include "decomposeFaces.H"
#include "decomposeCells.H"
#include "helperFunctions.H"
#include "meshSurfacePartitioner.H"
#include "triangle.H"

# ifdef USE_OMP
#include <omp.h>
# endif

//#define DEBUGLayer

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

bool Foam::Module::triangulateNonPlanarBaseFaces::findNonPlanarBoundaryFaces()
{
    const pointFieldPMG& points = mesh_.points();
    const label nInternalFaces = mesh_.nInternalFaces();

    meshSurfacePartitioner mPart(mesh_);
    const meshSurfaceEngine& mse = mPart.surfaceEngine();
    const labelList& faceOwner = mse.faceOwners();
    const faceList::subList& bFaces = mse.boundaryFaces();

    bool hasInvalid(false);

    # ifdef USE_OMP
    # pragma omp parallel for schedule(dynamic, 50)
    # endif
    forAll(bFaces, bfI)
    {
        const face& bf = bFaces[bfI];

        // triangle shall not be decomposed, they are flat
        if (bf.size() == 3)
        {
            continue;
        }

        // calculate min face diagonal
        scalar minDist(VGREAT);
        const point c = bf.centre(points);
        forAll(bf, pI)
        {
            minDist = Foam::min(minDist, Foam::mag(c - points[bf[pI]]));
        }

        forAll(bf, eI)
        {
            triangle<point, point> tri
            (
                points[bf[eI]],
                points[bf.nextLabel(eI)],
                c
            );

            const point triCentre = tri.centre();
            vector n = tri.unitNormal();

            forAll(bf, pI)
            {
                const scalar d = (points[bf[pI]] - triCentre) & n;

                if (d > tol_*minDist)
                {
                    invertedCell_[faceOwner[bfI]] = true;

                    decomposeFace_[nInternalFaces + bfI] = true;
                    hasInvalid = true;
                }
            }
        }
    }

    reduce(hasInvalid, maxOp<bool>());

    return hasInvalid;
}


void Foam::Module::triangulateNonPlanarBaseFaces::decomposeBoundaryFaces()
{
    // decompose base faces into triangles
    decomposeFaces triangulator(mesh_);
    triangulator.decomposeMeshFaces(decomposeFace_);
    const VRWGraph& newFacesFromFace = triangulator.newFacesForFace();

    // update face subsets
    mesh_.updateFaceSubsets(newFacesFromFace);
}


void Foam::Module::triangulateNonPlanarBaseFaces::decomposeCellsIntoPyramids()
{
    decomposeCells sc(mesh_);
    sc.decomposeMesh(invertedCell_);
}


// ************************************************************************* //
