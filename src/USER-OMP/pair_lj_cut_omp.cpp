/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   This software is distributed under the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Contributing author: Axel Kohlmeyer (Temple U)
------------------------------------------------------------------------- */

#include "math.h"
#include "pair_lj_cut_omp.h"
#include "atom.h"
#include "comm.h"
#include "force.h"
#include "neighbor.h"
#include "neigh_list.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

PairLJCutOMP::PairLJCutOMP(LAMMPS *lmp) :
  PairLJCut(lmp), ThrOMP(lmp, ThrData::THR_PAIR)
{
  respa_enable = 0;
  no_virial_fdotr_compute = 1;
}

/* ---------------------------------------------------------------------- */

void PairLJCutOMP::compute(int eflag, int vflag)
{
  if (eflag || vflag) {
    ev_setup(eflag,vflag);
  } else evflag = vflag_fdotr = 0;

  const int nall = atom->nlocal + atom->nghost;
  const int nthreads = comm->nthreads;
  const int inum = list->inum;

#if defined(_OPENMP)
#pragma omp parallel default(none) shared(eflag,vflag)
#endif
  {
    int ifrom, ito, tid;
    double **f;

    f = loop_setup_thr(atom->f, ifrom, ito, tid, inum, nall, nthreads);
    ThrData *thr = fix->get_thr(tid);
    ev_setup_thr(eflag,vflag,nall,eatom,vatom,thr);

    if (evflag) {
      if (eflag) {
	if (force->newton_pair) eval<1,1,1>(f, ifrom, ito, thr);
	else eval<1,1,0>(f, ifrom, ito, thr);
      } else {
	if (force->newton_pair) eval<1,0,1>(f, ifrom, ito, thr);
	else eval<1,0,0>(f, ifrom, ito, thr);
      }
    } else {
      if (force->newton_pair) eval<0,0,1>(f, ifrom, ito, thr);
      else eval<0,0,0>(f, ifrom, ito, thr);
    }
    if (vflag_fdotr)
      if (neighbor->includegroup)
	virial_fdotr_compute_thr(thr->virial_pair, atom->x, f,
				 atom->nlocal, atom->nghost, atom->nfirst);
      else
	virial_fdotr_compute_thr(thr->virial_pair, atom->x, f,
				 atom->nlocal, atom->nghost, -1);
#if defined(_OPENMP)
#pragma omp critical
#endif
    {
      eng_vdwl += thr->eng_vdwl;
      eng_coul += thr->eng_coul;
      for (int i=0; i < 6; ++i)
	virial[i] += thr->virial_pair[i];
    }
    data_reduce_thr(&(atom->f[0][0]), nall, nthreads, 3, tid);

  } // end of omp parallel region
}

template <int EVFLAG, int EFLAG, int NEWTON_PAIR>
void PairLJCutOMP::eval(double **f, int iifrom, int iito, ThrData * const thr)
{
  int i,j,ii,jj,jnum,itype,jtype;
  double xtmp,ytmp,ztmp,delx,dely,delz,evdwl,fpair;
  double rsq,r2inv,r6inv,forcelj,factor_lj;
  int *ilist,*jlist,*numneigh,**firstneigh;

  evdwl = 0.0;

  double **x = atom->x;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  double *special_lj = force->special_lj;
  double fxtmp,fytmp,fztmp;

  ilist = list->ilist;
  numneigh = list->numneigh;
  firstneigh = list->firstneigh;

  // loop over neighbors of my atoms

  for (ii = iifrom; ii < iito; ++ii) {

    i = ilist[ii];
    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];
    itype = type[i];
    jlist = firstneigh[i];
    jnum = numneigh[i];
    fxtmp=fytmp=fztmp=0.0;

    for (jj = 0; jj < jnum; jj++) {
      j = jlist[jj];
      factor_lj = special_lj[sbmask(j)];
      j &= NEIGHMASK;

      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      jtype = type[j];

      if (rsq < cutsq[itype][jtype]) {
	r2inv = 1.0/rsq;
	r6inv = r2inv*r2inv*r2inv;
	forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
	fpair = factor_lj*forcelj*r2inv;

	fxtmp += delx*fpair;
	fytmp += dely*fpair;
	fztmp += delz*fpair;
	if (NEWTON_PAIR || j < nlocal) {
	  f[j][0] -= delx*fpair;
	  f[j][1] -= dely*fpair;
	  f[j][2] -= delz*fpair;
	}

	if (EFLAG) {
	  evdwl = r6inv*(lj3[itype][jtype]*r6inv-lj4[itype][jtype])
	    - offset[itype][jtype];
	  evdwl *= factor_lj;
	}

	if (EVFLAG) ev_tally_thr(this,i,j,nlocal,NEWTON_PAIR,
				 evdwl,0.0,fpair,delx,dely,delz,thr);
      }
    }
    f[i][0] += fxtmp;
    f[i][1] += fytmp;
    f[i][2] += fztmp;
  }
}

/* ---------------------------------------------------------------------- */

double PairLJCutOMP::memory_usage()
{
  double bytes = memory_usage_thr();
  bytes += PairLJCut::memory_usage();

  return bytes;
}
