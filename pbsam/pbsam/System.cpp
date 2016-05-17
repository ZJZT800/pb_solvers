//
//  System.cpp
//  pb_solvers_code
//
//  Created by David Brookes on 9/28/15.
//  Copyright © 2015 David Brookes. All rights reserved.
//

#include "System.h"


Molecule::Molecule(int type, int type_idx, string movetype, vector<double> qs,
          vector<Pt> pos, vector<double> vdwr, vector<Pt> cens,
          vector<double> as, double drot, double dtrans)
:type_(type), typeIdx_(type_idx), moveType_(movetype), qs_(qs), pos_(pos),
vdwr_(vdwr), centers_(cens), as_(as), drot_(drot), dtrans_(dtrans),
Nc_((int) qs.size()), Ns_((int) cens.size()), cgCharges_(cens.size())
{
  int closest;
  for (int i = 0; i < Nc_; i++)
  {
    closest = find_closest_center(pos_[i]);
    cgCharges_[closest].push_back(i);
    pos_[i] = pos_[i] - centers_[closest];  // reposition charge
    chToCG_[i] = closest;
  }
}

Molecule::Molecule(int type, int type_idx, string movetype, vector<double> qs,
                       vector<Pt> pos, vector<double> vdwr, vector<Pt> msms_sp,
                       vector<Pt> msms_np, double tol_sp, int n_trials,
                       int max_trials, double beta, double drot,
                       double dtrans)
:type_(type), typeIdx_(type_idx), moveType_(movetype), qs_(qs), pos_(pos),
vdwr_(vdwr), drot_(drot), dtrans_(dtrans), Nc_((int) qs.size())
{
  find_centers(msms_sp, msms_sp, tol_sp, n_trials, max_trials);
}

Molecule::Molecule(const Molecule& mol)
:moveType_(mol.moveType_), type_(mol.type_), typeIdx_(mol.typeIdx_),
drot_(mol.drot_), dtrans_(mol.dtrans_), Nc_(mol.Nc_), qs_(mol.qs_),
pos_(mol.pos_), vdwr_(mol.vdwr_), Ns_(mol.Ns_), centers_(mol.centers_),
as_(mol.as_), cgCharges_(mol.cgCharges_), chToCG_(mol.chToCG_)
{
}


Pt Molecule::random_pt()
{
  double phi = drand48(  )*2*M_PI;
  double u = drand48(  )*2 - 1;
  
  return Pt( sqrt(1 - u*u ) * cos(phi), sqrt(1 - u*u) * sin(phi), u);
}

double Molecule::random_norm()
{
  
  double v1, v2, rsq;
  do
  {
    v1 = 2.0 * drand48(  ) - 1.0;
    v2 = 2.0 * drand48(  ) - 1.0;
    rsq = v1*v1 + v2*v2;
  } while ( rsq >= 1.0 || rsq == 0.0 );
  
  double fac = sqrt( -2.0*log(rsq )/rsq);
  return fac*v2;
}


int Molecule::find_closest_center(Pt pos)
{
  int idx = 0;
  double dmin = __DBL_MAX__;
  double d;
  for (int k = 0; k < Ns_; k++)
  {
    d = pos.dist(centers_[k]);
    if (d < dmin)
    {
      idx = k;
      dmin = d;
    }
  }
  return idx;
}

void Molecule::translate(Pt dr, double boxlen)
{
  for (int k = 0; k < Ns_; k++)
  {
    Pt dv  = centers_[k] + dr;
    
    centers_[k] = Pt(dv.x() - round(dv.x()/boxlen)*boxlen,
                 dv.y() - round(dv.y()/boxlen)*boxlen,
                 dv.z() - round(dv.z()/boxlen)*boxlen);
  }
}

void Molecule::rotate(Quat qrot)
{
  for (int k = 0; k < Ns_; k++)
  {
    centers_[k] = qrot.rotate_point(centers_[k]);
    for (int i = 0; i < cgCharges_[k].size(); i++)
    {
      pos_[cgCharges_[k][i]] = qrot.rotate_point(pos_[cgCharges_[k][i]]);
    }
  }
}

void Molecule::rotate(MyMatrix<double> rotmat)
{
  for (int k = 0; k < Ns_; k++)
  {
    centers_[k] = centers_[k].rotate(rotmat);
    for (int i = 0; i < cgCharges_[k].size(); i++)
    {
      pos_[cgCharges_[k][i]] = pos_[cgCharges_[k][i]].rotate(rotmat);
    }
  }
}

void Molecule::find_centers(vector<Pt> sp, vector<Pt> np,
                              double tol_sp, int n_trials,
                              int max_trials)
{
  vector<int> unbound (Nc_);
  
  for (int i = 0; i < Nc_; i++) unbound[i] = i;
  int j = -1;
  int ct = 0;
  
  while(unbound.size() != 0 && ct < Nc_)
  {
    j++;
    centers_.resize(j+1);
    as_.resize(j+1);
    cgCharges_.resize(j+1);
    int n_max = 0;
    
    int m=-1;
    while (m < n_trials || (m >= n_trials && n_max==0 && m < max_trials))
    {
      m++;
      CGSphere best = find_best_center(sp, np, unbound, tol_sp);
      if (best.get_n() > n_max)
      {
        centers_[j] = best.get_center();
        as_[j] = best.get_a();
        cgCharges_[j] = best.get_ch();
        n_max = best.get_n();
      }
    }
    
    if (n_max == 0)
    {
      int rand_ind = unbound[(int) floor(drand48() * unbound.size())];
      cgCharges_[j] = { rand_ind };
      n_max = 1;
    }
    
    if (n_max == 1)
    {
      centers_[j] = pos_[cgCharges_[j][0]];
      as_[j] = vdwr_[cgCharges_[j][0]];
    }
    
    // remove bound charges from list of unbound charges:
    for (int l = 0; l < cgCharges_[j].size(); l++)
    {
      for (int f = 0; f < unbound.size(); f++)
      {
        if (unbound[f] == cgCharges_[j][l])
        {
          unbound.erase(unbound.begin() + f);
        }
      }
    }
    ct++;
  }
}

CGSphere Molecule::find_best_center(vector<Pt> sp,vector<Pt> np,
                                      vector<int> unbound,
                                      double tol_sp, int iter, double beta)
{
  int sz = (int) unbound.size();
  Pt best_cen;
  double best_a = 0;
  int best_N = 0;
  vector<int> ch;  // encompassed charges of best sphere
  
  best_cen = pos_[unbound[(int) floor(drand48()*sz)]];
  
  for (int m = 0; m < iter; m++)
  {
    Pt tri_cen;
    double tri_a;
    int tri_N = 0;
    
    double cmin = __DBL_MAX__;
    
    for (int i = 0; i < sp.size(); i++)
    {
      double distsq = (sp[i] - best_cen).norm2();
      if (distsq < cmin) cmin = distsq;
    }
    double scale = sqrt(cmin);
    tri_cen = best_cen + (random_pt() * scale * random_norm());
    
    int min_id = 0;
    tri_a = __DBL_MAX__;
    for (int i = 0; i < sp.size(); i++)
    {
      double distsq = (sp[i] - tri_cen).norm2();
      if (distsq < tri_a)
      {
        tri_a = distsq;
        min_id = i;
      }
    }
    if ((sp[min_id] - tri_cen).dot(np[min_id]) < 0.0) continue;
    tri_a = sqrt(tri_a) + tol_sp;
    tri_a *= tri_a;
    
    // count number of unbound charges within this sphere
    for (int i = 0; i < sz; i++)
    {
      double dist = (pos_[unbound[i]] - tri_cen).norm() + vdwr_[unbound[i]];
      if (dist < sqrt(tri_a)) tri_N++;
    }
    
    //apply MC criteria
    if (exp(beta*(tri_N - best_N) > drand48()))
    {
      best_cen = tri_cen;
      best_N = tri_N;
      best_a = tri_a;  // should this be sqrt() ??
    }
    
    if ((m+1) % 100 == 0) beta *= 1.1;
  }
  
  ch.reserve(best_N);
  for (int i = 0; i < sz; i++)
  {
    double dist = (pos_[unbound[i]] - best_cen).norm() + vdwr_[unbound[i]];
    if (dist < sqrt(best_a))
    {
      ch.push_back(unbound[i]);
    }
  }
  
  return CGSphere(best_cen, best_a, best_N, ch);
}


System::System(const vector<Molecule>& mols, double cutoff,
               double boxlength)
:molecules_(mols), N_((int) mols.size()), cutoff_(cutoff),
boxLength_(boxlength), t_(0)
{
  int i, j, k, maxi = 0;
  vector<int> maxj, keys(2);
  for ( k = 0; k < N_; k++)
  {
    i = molecules_[k].get_type();
    j = molecules_[k].get_type_idx();
    keys = {i,j};
    typeIdxToIdx_[keys] = k;
    maxi = ( maxi > i ) ? maxi : i;
    
    if ( i >= maxj.size() ) maxj.push_back(0);
    maxj[i] = ( maxj[i] > j ) ? maxj[i] : j;
  }
  
  maxi++;
  for ( j = 0; j < maxj.size(); j++) maxj[j]++;
  
  ntype_ = maxi;
  typect_ = maxj;
  
  check_for_overlap();
  lambda_ = calc_average_radius();
  if (boxLength_/2. < cutoff_)  compute_cutoff();
}

System::System(Setup setup, double cutoff)
:t_(0), ntype_(setup.getNType()), typect_(setup.get_type_nct())
{
  vector<Molecule> mols;
  int i, j, k=0;
  string pqrpath;
  Molecule mol;
  vector<int> keys(2);
  for (i = 0; i < setup.getNType(); i++)
  {
    
    // First a build a representative molecule of this type (which will
    // be repositioned below
    PQRFile pqrI (setup.getTypeNPQR(i));
    Molecule type_mol;
    if (pqrI.get_Ns() == 0)
    {
      MSMSFile surf_file (setup.getTypeNSurf(i));
      type_mol = Molecule(i, 0, setup.getTypeNDef(i), pqrI.get_charges(),
                          pqrI.get_atom_pts(), pqrI.get_radii(),
                          surf_file.get_sp(), surf_file.get_np(),
                          setup.get_tol_sp(), setup.get_n_trials(),
                          setup.get_max_trials(), setup.get_sph_beta(),
                          setup.getDrot(i), setup.getDtr(i));
    }
    else
    {
      type_mol = Molecule(i, -1, setup.getTypeNDef(i), pqrI.get_charges(),
                          pqrI.get_atom_pts(), pqrI.get_radii(),
                          pqrI.get_cg_centers(), pqrI.get_cg_radii(),
                          setup.getDrot(i), setup.getDtr(i));
    }
    
    TransRotFile transrot;
    XYZFile xyzI;
    if (setup.getTypeIsTransRot(i))
    {
      transrot = TransRotFile(setup.getTypeNXYZ(i), setup.getTypeNCount(i));
    }
    else
    {
      xyzI = XYZFile (setup.getTypeNXYZ(i), setup.getTypeNCount(i));
    }
    
    for (j = 0; j < setup.getTypeNCount(i); j++)
    {
      // now copy the representative molecule and reposition it
      Pt trans;
      MyMatrix<double> rot;
      Molecule mol (type_mol);
      mol.set_type_idx(j);
      
      Pt com = pqrI.get_center_geo();
      
      if (setup.getTypeIsTransRot(i))
      {
        trans = transrot.get_trans(j);
        rot = transrot.get_rotmat(j);
      }
      else
      {
        trans = xyzI.get_pts()[j] + com * -1.0;
        rot = MyMatrix<double> (3, 3, 0.0);
        rot.set_val(0, 0, 1.0);
        rot.set_val(1, 1, 1.0);
        rot.set_val(2, 2, 1.0);
      }
      
      mol.rotate(rot);
      mol.translate(trans, setup.getBLen());
      
      molecules_.push_back(mol);
      typeIdxToIdx_[keys] = k;
      k++;
    } // end j
  } // end i
  N_ = (int) molecules_.size();
  boxLength_ = setup.getBLen();
  cutoff_ = cutoff;
  
  if (boxLength_/2. < cutoff_)  compute_cutoff();
  check_for_overlap();
  lambda_ = calc_average_radius();
}

const double System::calc_average_radius() const
{
  double ave = 0;
  int total_sphere = 0;
  for (int i = 0; i < N_; i++)
  {
    total_sphere += get_Ns_i(i);
    for (int k = 0; k < get_Ns_i(i); k++)
      ave += get_aik(i, k);
  }
  ave  =  ave / total_sphere;
  return ave;
}
//

void System::compute_cutoff()
{
  cutoff_ = boxLength_/2.0;
  cout << " The desired cutoff is larger than half the box length";
  cout << ". Resetting cutoff to 1/2 the boxlength: " << cutoff_ << endl;
}


void System::check_for_overlap()
{
  int i, j, k1, k2;
  double dist, aik, ajk;
  Pt cen_ik, cen_jk;
  for (i = 0; i < N_; i++)
  {
    for (j = i+1; j < N_; j++)
    {
      for (k1 = 0; k1 < molecules_[i].get_ns(); k1++)
      {
        for (k2 = 0; k2 < molecules_[j].get_ns(); k1++)
        {
          aik = molecules_[i].get_ak(k1);
          ajk = molecules_[j].get_ak(k2);
          cen_ik = molecules_[i].get_centerk(k1);
          cen_jk = molecules_[j].get_centerk(k2);
          dist = get_pbc_dist_vec_base(cen_ik, cen_jk).norm();
          if (dist < (aik + ajk))
            throw OverlappingMoleculeException(i, j);
        }
      }
    }
  }
}

Pt System::get_pbc_dist_vec_base(Pt p1, Pt p2)
{
  Pt dv  = p1 - p2;
  
  Pt v = Pt(dv.x() - round(dv.x()/boxLength_)*boxLength_,
            dv.y() - round(dv.y()/boxLength_)*boxLength_,
            dv.z() - round(dv.z()/boxLength_)*boxLength_);
  
  return v;
}

//vector<Pt> System::get_allcenter() const
//{
//  vector< Pt> mol_cen(N_);
//  for ( int i = 0; i < N_; i++)
//    mol_cen[i] = molecules_[i].get_center();
//  
//  return mol_cen;
//}

bool System::less_than_cutoff(Pt v)
{
  if (v.norm() < cutoff_) return true;
  else return false;
}

//void System::reset_positions( vector<string> xyzfiles )
//{
//  int i, j, k;
//  vector<int> keys(2);
//  for (i = 0; i < ntype_; i++)
//  {
//    XYZFile xyzI (xyzfiles[i], typect_[i]);
//    for (j = 0; j < typect_[i]; j++)
//    {
//      keys = { i, j};
//      k = typeIdxToIdx_[keys];
//      Pt dist_to_new = get_centeri(k) - xyzI.get_pts()[j];
//      molecules_[k].translate(dist_to_new*-1, boxLength_);
//    }
//  }
//  
//}

void System::write_to_pqr(string outfile)
{
  int i, j, k, ct = 0;
  ofstream pqr_out;
  char pqrlin[400];
  
  pqr_out.open( outfile );
  
  for ( i = 0; i < N_; i++ )
  {
    for ( j = 0; j < get_Nc_i(i); j++)
    {
      sprintf(pqrlin,"%6d  C   CHG A%-5d    %8.3f%8.3f%8.3f %7.4f %7.4f",ct,i,
              get_posijreal(i, j).x(),
              get_posijreal(i, j).y(),
              get_posijreal(i, j).z(),
              get_qij(i, j), get_radij(i, j));
      pqr_out << "ATOM " << pqrlin << endl;
      ct++;
    }
    for (k = 0; k < get_Ns_i(i); k++)
    {
      sprintf(pqrlin,"%6d  X   CEN A%-5d    %8.3f%8.3f%8.3f %7.4f %7.4f",ct,i,
              get_centerik(i, k).x(), get_centerik(i, k).y(),
              get_centerik(i, k).z(), 0.0, get_aik(i,k));
      pqr_out << "ATOM " << pqrlin << endl;
      ct++;
    }
  }
}

void System::write_to_xyz(ofstream & xyz_out)
{
  int i, j, k, at_tot = 0;
  char xyzlin[400];
  
  for ( i = 0; i < N_; i++ )
    for ( j = 0; j < get_Nc_i(i); j++)
      at_tot++;
  at_tot += N_; // for adding CG centers
  
  xyz_out << at_tot << endl;
  xyz_out << "Atoms. Timestep (ps): " << t_ << endl;
  for ( i = 0; i < N_; i++ )
  {
    for ( j = 0; j < get_Nc_i(i); j++)
    {
      sprintf(xyzlin,"N %8.3f %8.3f %8.3f", get_posijreal(i, j).x(),
              get_posijreal(i, j).y(), get_posijreal(i, j).z());
      xyz_out << xyzlin << endl;
    }
    for (k = 0; k < get_Ns_i(i); k++)
    {
      sprintf(xyzlin,"X %8.3f %8.3f %8.3f", get_centerik(i, k).x(),
            get_centerik(i, k).y(), get_centerik(i, k).z());
      xyz_out << xyzlin << endl;
    }
  }
}