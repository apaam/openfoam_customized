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

#include "demandDrivenData.H"
#include "meshSurfaceEdgeExtractor2D.H"
#include "meshOctree.H"
#include "triSurf.H"
#include "meshSurfaceEngine.H"
#include "meshSurfaceMapper2D.H"
#include "polyMeshGen2DEngine.H"
#include "helperFunctions.H"

# ifdef USE_OMP
#include <omp.h>
# endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::Module::meshSurfaceEdgeExtractor2D::distributeBoundaryFaces()
{
    polyMeshGen2DEngine mesh2DEngine(mesh_);
    const boolList& activeFace = mesh2DEngine.activeFace();
    const boolList& zMinPoint = mesh2DEngine.zMinPoints();
    const boolList& zMaxPoint = mesh2DEngine.zMaxPoints();

    const pointFieldPMG& points = mesh_.points();
    const faceListPMG& faces = mesh_.faces();
    const labelList& owner = mesh_.owner();
    const PtrList<boundaryPatch>& boundaries = mesh_.boundaries();

    const triSurf& surf = meshOctree_.surface();
    const geometricSurfacePatchList& surfPatches = surf.patches();

    // copy boundary faces and their owner face
    VRWGraph bndFaces;
    labelLongList origFaceLabel;

    forAll(boundaries, patchI)
    {
        const label start = boundaries[patchI].patchStart();
        const label size = boundaries[patchI].patchSize();

        for (label fI = 0; fI < size; ++fI)
        {
            const label faceI = start + fI;
            const face& bf = faces[faceI];

            bndFaces.appendList(bf);
            origFaceLabel.append(faceI);
        }
    }

    // project face centres onto their nearest location on the surface mesh
    wordList patchNames(surfPatches.size()+2);
    wordList patchTypes(surfPatches.size()+2);
    forAll(surfPatches, ptchI)
    {
        patchNames[ptchI] = surfPatches[ptchI].name();
        patchTypes[ptchI] = surfPatches[ptchI].geometricType();
    }

    const label bottomEmptyId = patchNames.size() - 2;
    const label topEmptyId = patchNames.size() - 1;

    patchNames[bottomEmptyId] = "bottomEmptyFaces";
    patchTypes[bottomEmptyId] = "empty";
    patchNames[topEmptyId] = "topEmptyFaces";
    patchTypes[topEmptyId] = "empty";

    labelLongList bndFaceOwner(bndFaces.size());
    labelLongList bndFacePatch(bndFaces.size());

    # ifdef USE_OMP
    # pragma omp parallel for schedule(dynamic, 50)
    # endif
    forAll(bndFaces, bfI)
    {
        const label faceI = origFaceLabel[bfI];
        const face& f = faces[faceI];

        bndFaceOwner[bfI] = owner[faceI];

        if (!activeFace[faceI])
        {
            if (zMinPoint[f[0]])
            {
                bndFacePatch[bfI] = bottomEmptyId;
            }
            else if (zMaxPoint[f[0]])
            {
                bndFacePatch[bfI] = topEmptyId;
            }
        }
        else
        {
            // this face is active
            const point c = f.centre(points);

            // find the patch index of the nearest location on the surface mesh
            point mapPoint;
            scalar distSq;
            label patchI, nt;
            meshOctree_.findNearestSurfacePoint(mapPoint, distSq, nt, patchI, c);

            bndFacePatch[bfI] = patchI;
        }
    }

    // replace the boundary
    polyMeshGenModifier meshModifier(mesh_);

    meshModifier.replaceBoundary
    (
        patchNames,
        bndFaces,
        bndFaceOwner,
        bndFacePatch
    );

    // set correct patch types
    PtrList<boundaryPatch>& modBnd = meshModifier.boundariesAccess();
    forAll(patchTypes, patchI)
        modBnd[patchI].patchType() = patchTypes[patchI];
}


void Foam::Module::meshSurfaceEdgeExtractor2D::remapBoundaryPoints()
{
    meshSurfaceEngine mse(mesh_);
    meshSurfaceMapper2D mapper(mse, meshOctree_);

    mapper.adjustZCoordinates();

    mapper.mapVerticesOntoSurfacePatches();
}


// ************************************************************************* //
