/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019 OpenCFD Ltd.
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

Class
    Foam::petscLinearSolverContext

Description
    A struct containing a KSP and Mat for each equation to be solved

SourceFiles
    petscLinearSolverContext.C

\*---------------------------------------------------------------------------*/

#ifndef petscFoamLinearSolverContext_H
#define petscFoamLinearSolverContext_H

#include "solverPerformance.H"
#include "petscCacheManager.H"
#include "petscUtils.H"

// PETSc
#include "petscvec.h"
#include "petscmat.h"
#include "petscksp.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class petscLinearSolverContext Declaration
\*---------------------------------------------------------------------------*/

class petscLinearSolverContext
{
    // Private Data

        bool init_;
        bool init_aux_vectors_;

public:

    // Public Members

        Mat Amat;
        KSP ksp;

        bool useFoamTest;
        Vec ArowsSum;
        Vec *res_l1_w;

        solverPerformance performance;
        petscCacheManager caching;

        List<PetscInt> lowNonZero;
        label maxLowNonZeroPerRow;

        PetscScalar normFactor;

        PetscLogStage matstage;
        PetscLogStage pcstage;
        PetscLogStage kspstage;


    // Constructors

        //- Default construct
        petscLinearSolverContext()
        :
            init_(false),
            init_aux_vectors_(false),
            Amat(nullptr),
            ksp(nullptr),
            useFoamTest(false),
            ArowsSum(nullptr),
            res_l1_w(nullptr)
        {}


    //- Destructor
    virtual ~petscLinearSolverContext()
    {
        MatDestroy(&Amat);
        KSPDestroy(&ksp);
        VecDestroy(&ArowsSum);
        if (res_l1_w) VecDestroyVecs(2,&res_l1_w);
    }


    // Member Functions

        //- Return value of initialized
        bool initialized() const
        {
            return init_;
        }

        //- Return value of initialized
        bool& initialized()
        {
            return init_;
        }

        //- Create auxiliary rows for calculation purposes
        void createAuxVecs()
        {
            if (!Amat) return;
            if (!init_aux_vectors_)
            {
                MatCreateVecs(Amat, NULL, &ArowsSum);
                VecDuplicateVecs(ArowsSum, 2, &res_l1_w);
                init_aux_vectors_ = true;
            }
        }

        //- Sum the rows of A, placing the result in ArowsSum
        void initArowsSumVec()
        {
            if (init_aux_vectors_)
            {
                MatGetRowSum(Amat, ArowsSum);
            }
        }

        //- Compute the normFactor used in convergence testing,
        //- assumes ArowsSum has been already computed
        void computeNormFactor(Vec psi, Vec source)
        {
            if (!init_aux_vectors_) return;

            MatMult(Amat, psi, res_l1_w[1]);

            normFactor =
               Foam::PetscUtils::normFactor
               (
                   res_l1_w[1],
                   psi,
                   source,
                   ArowsSum
               );
        }

        //- Compute residual (L1) norm
        //- assumes normFactor has been already computed
        PetscReal getResidualNormL1(Vec psi, Vec source)
        {
            if (!init_aux_vectors_) return -1.;

            PetscReal rnorm;
            MatMult(Amat, psi, res_l1_w[1]);
            VecAXPY(res_l1_w[1], -1., source);
            VecNorm(res_l1_w[1], NORM_1, &rnorm);
            return rnorm/normFactor;
        }

        //- Compute residual (L1) norm
        //- assumes normFactor has been already computed
        //- as well as Adotpsi is stored in res_l1_w[1]
        PetscReal getResidualNormL1(Vec source)
        {
            if (!init_aux_vectors_) return -1.;

            PetscReal rnorm;
            VecAXPY(res_l1_w[1], -1., source);
            VecNorm(res_l1_w[1], NORM_1, &rnorm);
            return rnorm/normFactor;
        }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
