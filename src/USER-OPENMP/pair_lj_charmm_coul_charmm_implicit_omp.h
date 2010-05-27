/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef PAIR_CLASS

PairStyle(lj/charmm/coul/charmm/implicit/omp,PairLJCharmmCoulCharmmImplicitOMP)

#else

#ifndef LMP_PAIR_LJ_CHARMM_COUL_CHARMM_IMPLICIT_OMP_H
#define LMP_PAIR_LJ_CHARMM_COUL_CHARMM_IMPLICIT_OMP_H

#include "pair_lj_charmm_coul_charmm_omp.h"

namespace LAMMPS_NS {

class PairLJCharmmCoulCharmmImplicitOMP : public PairLJCharmmCoulCharmmOMP {
 public:
  PairLJCharmmCoulCharmmImplicitOMP(class LAMMPS *);
  virtual void compute(int, int);
  virtual double single(int, int, int, int, double, double, double, double &);
  virtual void *extract(char *);

protected:
  template <int EVFLAG, int EFLAG, int NEWTON_PAIR> void eval();
};

}

#endif
#endif
