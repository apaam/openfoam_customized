/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
-------------------------------------------------------------------------------
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

#include "cellSetOption.H"
#include "volFields.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    namespace fv
    {
        defineTypeNameAndDebug(cellSetOption, 0);
    }
}


const Foam::Enum
<
    Foam::fv::cellSetOption::selectionModeType
>
Foam::fv::cellSetOption::selectionModeTypeNames_
({
    { selectionModeType::smPoints, "points" },
    { selectionModeType::smCellSet, "cellSet" },
    { selectionModeType::smCellZone, "cellZone" },
    { selectionModeType::smAll, "all" },
});


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::fv::cellSetOption::setSelection(const dictionary& dict)
{
    switch (selectionMode_)
    {
        case smPoints:
        {
            dict.readEntry("points", points_);
            break;
        }
        case smCellSet:
        {
            dict.readEntry("cellSet", cellSetName_);
            break;
        }
        case smCellZone:
        {
            dict.readEntry("cellZone", cellSetName_);
            break;
        }
        case smAll:
        {
            break;
        }
        default:
        {
            FatalErrorInFunction
                << "Unknown selectionMode "
                << selectionModeTypeNames_[selectionMode_]
                << ". Valid selectionMode types : "
                << selectionModeTypeNames_
                << exit(FatalError);
        }
    }
}


void Foam::fv::cellSetOption::setVol()
{
    // Set volume information

    scalar sumVol = 0.0;
    for (const label celli : cells_)
    {
        sumVol += mesh_.V()[celli];
    }
    reduce(sumVol, sumOp<scalar>());

    const scalar VOld = V_;
    V_ = sumVol;

    // Convert both volumes to representation using current writeprecision
    word VOldName(Time::timeName(VOld, IOstream::defaultPrecision()));
    word VName(Time::timeName(V_, IOstream::defaultPrecision()));

    if (VName != VOldName)
    {
        Info<< indent
            << "- selected " << returnReduce(cells_.size(), sumOp<label>())
            << " cell(s) with volume " << V_ << endl;
    }
}


void Foam::fv::cellSetOption::setCellSelection()
{
    switch (selectionMode_)
    {
        case smPoints:
        {
            Info<< indent << "- selecting cells using points" << endl;

            labelHashSet selectedCells;

            forAll(points_, i)
            {
                label celli = mesh_.findCell(points_[i]);
                if (celli >= 0)
                {
                    selectedCells.insert(celli);
                }

                label globalCelli = returnReduce(celli, maxOp<label>());
                if (globalCelli < 0)
                {
                    WarningInFunction
                        << "Unable to find owner cell for point " << points_[i]
                        << endl;
                }

            }

            cells_ = selectedCells.sortedToc();
            break;
        }
        case smCellSet:
        {
            Info<< indent
                << "- selecting cells using cellSet " << cellSetName_ << endl;

            cells_ = cellSet(mesh_, cellSetName_).sortedToc();
            break;
        }
        case smCellZone:
        {
            Info<< indent
                << "- selecting cells using cellZone " << cellSetName_ << endl;

            label zoneID = mesh_.cellZones().findZoneID(cellSetName_);
            if (zoneID == -1)
            {
                FatalErrorInFunction
                    << "Cannot find cellZone " << cellSetName_ << endl
                    << "Valid cellZones are " << mesh_.cellZones().names()
                    << exit(FatalError);
            }

            cells_ = mesh_.cellZones()[zoneID];
            break;
        }
        case smAll:
        {
            Info<< indent << "- selecting all cells" << endl;

            cells_ = identity(mesh_.nCells());
            break;
        }
        default:
        {
            FatalErrorInFunction
                << "Unknown selectionMode "
                << selectionModeTypeNames_[selectionMode_]
                << ". Valid selectionMode types are "
                << selectionModeTypeNames_
                << exit(FatalError);
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::cellSetOption::cellSetOption
(
    const word& name,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    fv::option(name, modelType, dict, mesh),
    timeStart_(-1),
    duration_(0),
    selectionMode_(selectionModeTypeNames_.get("selectionMode", coeffs_)),
    cellSetName_("none"),
    V_(0)
{
    Info<< incrIndent;
    read(dict);
    setSelection(coeffs_);
    setCellSelection();
    setVol();
    Info<< decrIndent;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::fv::cellSetOption::isActive()
{
    if (fv::option::isActive() && inTimeLimits(mesh_.time().value()))
    {
        // Update the cell set if the mesh is changing
        if (mesh_.changing())
        {
            if (mesh_.topoChanging())
            {
                setCellSelection();
                // Force printing of new set volume
                V_ = -GREAT;
            }
            else if (selectionMode_ == smPoints)
            {
                // This is the only geometric selection mode
                setCellSelection();
            }

            // Report new volume (if changed)
            setVol();
        }

        return true;
    }

    return false;
}


bool Foam::fv::cellSetOption::read(const dictionary& dict)
{
    if (fv::option::read(dict))
    {
        if (coeffs_.readIfPresent("timeStart", timeStart_))
        {
            coeffs_.readEntry("duration", duration_);
        }

        return true;
    }

    return false;
}


// ************************************************************************* //
