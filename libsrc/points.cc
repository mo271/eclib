// points.cc:  implementations for Point class for points on elliptic curves
//////////////////////////////////////////////////////////////////////////
//
// Copyright 1990-2012 John Cremona
// 
// This file is part of the eclib package.
// 
// eclib is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
// 
// eclib is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// 
// You should have received a copy of the GNU General Public License
// along with eclib; if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
// 
//////////////////////////////////////////////////////////////////////////
 
// originally adapted from Elliptic.cc by Oisin McGuiness

#include <eclib/points.h>  // which includes curve.h
#include <eclib/cperiods.h>
#include <NTL/RR.h>   // for the realify_point() function

//#define DEBUG_TORSION

//
// Point member functions
//

// Shifts P via T = (u, r, s, t) on to newc.   
// N.B. Assumes that (1) T(P.E) = newc
//                   (2) T(P) is on newc
// "back" means use reverse transform

Point transform(const Point& P, Curvedata* newc, 
		const bigint& u, 
		const bigint& r, const bigint& s, const bigint& t, 
		int back)
{
  if(P.is_zero()) return Point(newc);
  if(!P.isvalid())
    cout << "Attempting to trabsform the point " << P 
	 << "which is not a valid point on its curve " << P.getcurve() << "!\n";
  Point Q(newc,transform(P,u,r,s,t,back));
  if(!Q.isvalid())
    {
      cout << "Result of transforming the point " << P << " on curve "<<(Curve)*(P.E) << " via [u,r,s,t]=["<<u<<","<<r<<","<<s<<","<<t<<"]";
      if(back) cout<<" (inverse) ";
      cout << " is " << Q << " which is not a valid point on its curve " 
	   << (Curve)(*newc) << "!\n";
    }
  return Q;
}

// Genuine elliptic curve point functions:

// addition and subtraction
void Point::operator+=(const Point& Q)  // P += Q; 
{
  Point sum = (*this) + Q ;
  E = sum.E; X = sum.X; Y = sum.Y; Z = sum.Z; reduce();
  ord = 0; height = -1;
}

void Point::operator-=(const Point& Q)  // P -= Q ; 
{
  Point sum = -Q;  sum+=(*this);
  E = sum.E; X = sum.X; Y = sum.Y; Z = sum.Z; reduce();
  ord = 0; height = -1;
}
        
Point Point::operator+(const Point& Q) const // P + Q
{
  Point ans(E);
  // make sure that both points are on the same curve
  if(E != Q.E){
    cerr << "## Can't add points on different curves!" << endl;
    return ans;
  }
  // NOTE: we don't do any reductions here, but rely on assignment to do it
  // check special cases first
  if(Z==0) return Q ;
  if(Q.is_zero()) return *this ;
  if(eq(*this , Q))      {return Q.twice();}
  Point minusQ=-Q;
  if(eq(*this , minusQ)) {return ans;}       // zero
        
  // we now have genuine work to do
  // let's set up some local variables to avoid repeated references
  // coefficients
  bigint A1,A2,A3,A4,A6;  E->getai(A1,A2,A3,A4,A6);
  // coordinates of P
  const bigint& X1 = X ;
  const bigint& Y1 = Y ;
  const bigint& Z1 = Z ;
  // coordinates of Q
  const bigint& X2 = Q.X ;
  const bigint& Y2 = Q.Y ;
  const bigint& Z2 = Q.Z ;
  const bigint& Z12 = Z1 * Z2 ;
        
  const bigint& L = - Y2 * Z1 + Y1 * Z2 ;    /* lambda */
  const bigint& M = - X2 * Z1 + X1 * Z2 ;    /* mu    */
  const bigint& N = - Y1 * X2 + Y2 * X1 ;    /* nu   */
        
  const bigint& Mz =  M * M * Z12  ; 
  
  const bigint& t = L * L * Z12 + M * ( A1 * L * Z12 - M * (A2 * Z12
                                        + X1 * Z2 + X2 * Z1 ) )  ;
  const bigint& newX =  M * t ;
  const bigint& newY = - (  t * (L + A1 * M) +   Mz * (N + A3 * M )) ;    
  const bigint& newZ = M * Mz ;
  ans.init(E, newX, newY, newZ);
  return ans;
}

Point Point::operator-(const Point& Q) const // P - Q
{
  Point ans = (-Q) ;
  ans += (*this);
  return ans;
}

Point Point::twice(void) const // doubles P
{
  // do trivial cases
  Point ans(E);
  if( Z==0) return ans;
  bigint A1,A2,A3,A4,A6;  E->getai(A1,A2,A3,A4,A6);
  Point minusthis = -(*this);
  if(eq(*this,minusthis)) return ans;   // order 2

  const bigint& Zsq = Z * Z ;
  
  const bigint& L = 3 * X * X  +  2 * A2 * X * Z  +  
                                     A4 * Zsq  -  A1 * Y * Z ;
  const bigint& M = 2 * Y  +  A1 * X  +  A3 * Z ;
        
  const bigint& Mz = M * Z ;
  const bigint& N =  - X * X * X -A3 * Y * Zsq  + A4 * X *Zsq  +  
    2 * A6 * Z *Zsq ;
        
  const bigint& t  = L * L  +  Mz * ( A1 * L  -  M * ( A2 * Z  + 2 * X ) );  

  const bigint& newX = t * Mz ;
  const bigint& newY = - ( L * t +  Mz * ( A1 * t+ M * ( N  + A3 * Mz * Z) ) );
  const bigint& newZ = Mz * Mz * Mz;

  ans.init(E, newX, newY, newZ) ;
  return ans;
}

Point Point::operator-(void) const // -Q
{
  Point negation(*this);
  negation.Y = -Y - (E->a3)*Z - (E->a1)*X;
  return negation;
}

// calculates nP
Point operator*(int n, const Point& P)
{
  Point ans(*(P.E));
  if(P.is_zero() || n == 0) return ans;
  int negative = (n < 0) ;
  if(negative) n = - n ;
  if(n == 1) {
    ans = P;
    if(negative) ans =  -P;
    return ans ;
  }
  // now n >= 2
  if(n == 2){
    ans = P.twice() ;
    if(negative) ans =  -ans;
    return ans ;
  }
  // now n >= 3
  if(n&1) ans = P ;    // else ans is ZERO
  Point  Q = P ; 
  while(n > 1){
    Q = Q.twice() ; // 2^k P
    n /= 2 ;
    if(n&1) ans = ans + Q ;
  }
  if(negative) ans = -ans ;
  return ans ;
}

int order(Point& p)
{
  // ASSUME that point is valid; check before calling if unknown
  if (p.ord) {return p.ord;}
  bigint eight, z=p.getZ(); eight=8;
  if (is_zero(z))  {p.ord = 1; return 1; }
  if (z>eight)     {p.ord =-1; return -1;}
  Point q = p;  long ord=1;
  while ( (sign(z)!=0) && (z<=eight) ) // (worst denom of a torsion point)
    {ord++;   q+=p;  z = q.getZ(); }
  if (sign(z)!=0) ord = -1;
  p.ord = ord;
  return ord;
}

int order(Point& p, vector<Point>& multiples)
{
#ifdef DEBUG_TORSION
  cout<<"In order() with p = "<<p<<endl;
#endif
  // ASSUME that point is valid; check before calling if unknown
  multiples.clear(); multiples.reserve(13);
  multiples.push_back(Point(*(p.E)));
  if (p.is_zero()) {p.ord=1; return 1; }
  multiples.push_back(p);
  Point q = p;
  bigint eight; eight=8;
  while ( (!q.is_zero()) && (q.getZ()<=eight) 
	                 && (multiples.size()<13) ) // 12 is max poss order
  { 
    q+=p;
    if (!q.is_zero()) multiples.push_back(q);
  }
  if (q.is_zero()) 
    p.ord = multiples.size();
  else
    p.ord = -1;
#ifdef DEBUG_TORSION
  cout<<"Returning ord="<<p.ord<<", multiples = "<<multiples<<endl;
#endif
  return p.ord;
}

int Point::isvalid() const // P on its curve ?
{
  if(E == 0){
    cerr << "## Bad point: null curve pointer!"<<endl;
    return 0 ;
  }
// Null point, useful for terminating input:
  if((sign(X)==0)&&(sign(Y)==0)&&(sign(Z)==0)) return 0;   
// Point at infinity is on a curve
  if((sign(X)==0)&&(sign(Z)==0)) return 1 ;
  else{
    // should calculate 
    //                       y^2 +a1 x y + a3 y 
    //  and
    //                       x^3 + a2 x^2 + a4 x + a6
    // where 
    //          x = X/Z, y = Y/Z
    // and verify equality.
    //
    // In homogeneous coordinates:
    //
    // Lhs = Y^2 Z + a1 XYZ + a3 YZ^2 = (YZ) *(Y + a1 X + a3 Z)
    //
    //
    // Rhs = X^3 +a2 X^2 Z + a4 X Z^2 + a6 Z^3
    //
    bigint A1,A2,A3,A4,A6;    E->getai(A1,A2,A3,A4,A6);
    const bigint& Lhs = Y*Z*(Y + A1*X + A3*Z) ;
    const bigint& Rhs = A6*pow(Z,3) + X*(A4*Z*Z + X*(A2*Z + X)) ;
    return Lhs == Rhs ;
  }
}

// Find all points with a given rational x-coordinate:
vector<Point> points_from_x(Curvedata &E, const bigrational& x)
{
  //  cout<<"Trying to construct points with x="<<x<<endl;
  bigint a1,a2,a3,a4,a6,b2,b4,b6,b8;
  E.getai(a1,a2,a3,a4,a6);
  E.getbi(b2,b4,b6,b8);
  vector<Point> ans;
  bigint xn = num(x), xd2=den(x), xd, xd4, s, t, yn;
  //  cout<<"xd2 = "<<xd2<<endl;
  if(isqrt(xd2,xd)) // xd2=xd^2
    {
      //      cout<<"xd = "<<xd<<endl;
      xd4 = xd2*xd2;
      s = ((4*xn+b2*xd2)*xn+(2*b4*xd4))*xn+b6*xd4*xd2;
      //      cout<<"s = "<<s<<endl;
      if(isqrt(s,t)) // s=t^2
        {
          //          cout<<"t = "<<t<<endl;
          yn = t - (a1*xn+a3*xd2)*xd;
          divide_exact(yn,BIGINT(2),yn);
          //          cout<<"yn = "<<yn<<endl;
          Point P(E,xn*xd,yn,xd2*xd);
          //          cout<<"point="<<P<<endl;
          ans.push_back(P);
          if (!is_zero(t)) ans.push_back(-P);
        }
    }
  return ans;
}

// comparison function for sorting torsion points (on the same curve)
// We sort by (1) order of point, (2) denominator, (3) X-coordinate,
// (4) y-coordinate

struct Point_comparer {
  bool operator()(const Point& P0, const Point& Q0)
  {
    Point P(P0), Q(Q0); // take copies as args must be const but order() requires non-const
    int s = order(P)-order(Q);
    if(s) return (s<0); // true if P has smaller order
    bigint t = P.getZ()-Q.getZ();
    if(!is_zero(t)) return (t<0); // true if P has smaller Z (denominator)
    t = P.getX()-Q.getX();
    if(!is_zero(t)) return (t<0); // true if P has smaller X
    t = P.getY()-Q.getY();
    return (t<0); // true if P has smaller Y
  }
}
  Point_cmp;

// Now to sort a list Plist of points in place, do
// ::sort(Plist.begin(), Plist.end(), Point_cmp);

long Curvedata::get_ntorsion()
{
  if(ntorsion==0)
    {
#ifdef DEBUG_TORSION
      cout<<"Calling torsion_points() on "<<(Curve)(*this)<<endl;
#endif
      vector<Point> ans = torsion_points(*this);
      ntorsion = ans.size();
#ifdef DEBUG_TORSION
      cout<<"torsion_points() returns "<<ans<<" of size "<<ntorsion<<endl;
#endif
    }
#ifdef DEBUG_TORSION
      cout<<"Curvedata::get_ntorsion() returns "<<ntorsion<<endl;
#endif
  return ntorsion;
}

// Cperiods is a class containing a basis for the period lattice L;
// it knows how to compute points from z mod L; so this function
// effectively does the same as PARI's pointell() (called ellzto point
// in the next PARI release)
//
Point make_tor_pt(Curvedata& E, Cperiods& per, 
		  const bigfloat& ra1, const bigfloat& ra2, const bigfloat& ra3, 
		  const bigcomplex& z)
{
  bigcomplex cx,cy;
#ifdef DEBUG_TORSION
  cout<<"In make_tor_pt() with z = " << z << endl;
#endif
  per.XY_coords(cx,cy,z);
  cx = cx-(ra1*ra1+4*ra2)/to_bigfloat(12);
  cy = (cy - ra1*cx - ra3)/to_bigfloat(2);
#ifdef DEBUG_TORSION
  cout<<"(x,y) = ("<<(cx)<<","<<(cy)<<")\n";
#endif
  bigint x=Iround(real(cx)), y=Iround(real(cy));
  Point P(E, x, y);
  return P;
}


vector<Point> two_torsion(Curvedata& E)
{
#ifdef DEBUG_TORSION
  cout<<"\nIn two_torsion() with curve "<<(Curve)E<<"\n";
#endif
  bigint a1, a2, a3, a4, a6, b2, b4, b6, b8;
  E.getai(a1,a2,a3,a4,a6);
  E.getbi(b2,b4,b6,b8);
  int scaled=0;
  if (odd(a1) || odd(a3)) 
    { 
      b4*=BIGINT(8);
      b6*=BIGINT(16);
      scaled=1;
    }
  else 
    {
      b2=a2; b4=a4; b6=a6;
    }
  vector<bigint> xlist = Introotscubic(b2,b4,b6);
  int n, n2tors = xlist.size();

  // If there are 3 points of order 2, we order them for consistency:
  if(n2tors==3)  sort(xlist.begin(),xlist.end());

  vector<Point> two_tors;
  two_tors.push_back(Point(E)) ;     // zero point
  for(n=0; n<n2tors; n++)
    {
      bigint ei = xlist[n];
      if(scaled) 
	two_tors.push_back(Point(E,2*ei,-a1*ei-4*a3,BIGINT(8)));
      else 
	two_tors.push_back(Point(E,ei,BIGINT(0),BIGINT(1)));
    }
  ::sort(two_tors.begin(), two_tors.end(), Point_cmp);
#ifdef DEBUG_TORSION
  cout<<"\ntwo_torsion() returning "<<two_tors<<"\n";
#endif
  return two_tors;
}

// Returns vector of x values such that x/3 is the rational x-coord of
// a point in E[3], possibly with y not rational.  If y is rational
// then these x will in fact be multiples of 3 since rational
// 3-torsion is integral
vector<bigint> three_torsion_x(Curvedata& E)
{
#ifdef DEBUG_TORSION
  cout<<"\nIn three_torsion_x() with curve "<<(Curve)E<<"\n";
#endif
  bigint b2, b4, b6, b8;
  E.getbi(b2,b4,b6,b8);
  vector<bigint> xlist = Introotsquartic(b2,9*b4,27*b6,27*b8);
  // NB The implementation of Introosquartic() in marith.cc does not
  // fix the order of the roots, which depends on the order of the
  // factors in NTL's Z[X] factorization routine.  HENCE the order of
  // the 3-torsion point x-coordinates (when there are two) is not
  // well-defined without the sorting done here.
#ifdef DEBUG_TORSION
  cout<<"\nthree_torsion_x() finds unsorted xlist =  "<<xlist<<"\n";
#endif
  if(xlist.size()==2)
    {
      sort(xlist.begin(),xlist.end());
#ifdef DEBUG_TORSION
      cout<<"\nthree_torsion_x() returns sorted xlist =  "<<xlist<<"\n";
#endif
    }
  return xlist;
}

vector<Point> three_torsion(Curvedata& E)
{
#ifdef DEBUG_TORSION
  cout<<"\nIn three_torsion() with curve "<<(Curve)E<<"\n";
#endif
  bigint a1, a2, a3, a4, a6, b2, b4, b6, b8, xi, d, rd;
  E.getai(a1,a2,a3,a4,a6);
  E.getbi(b2,b4,b6,b8);
  vector<bigint> xlist = three_torsion_x(E);
  vector<Point> three_tors;
  three_tors.push_back(Point(E)) ;     // zero point
  for(unsigned int n=0; n<xlist.size(); n++)
    {
      xi = xlist[n];
      if(xi%3==0) // 3-torsion must be integral
	{
	  xi/=3;
	  d = ((4*xi+b2)*xi+(2*b4))*xi+b6;
	  if(isqrt(d,rd)) 			    
	    {
	      Point P(E,2*xi,rd-(a1*xi+a3),BIGINT(2));
	      three_tors.push_back(P);
	      three_tors.push_back(-P);
	    }
	}
    }
  ::sort(three_tors.begin(), three_tors.end(), Point_cmp);
#ifdef DEBUG_TORSION
  cout<<"\nthree_torsion() returning "<<three_tors<<"\n";
#endif
  return three_tors;
}

// New torsion point routine using Mazur's theorem to limit possibilities 
// and computing possible real torsion from the period lattice.  Suggestion 
// of Darrin Doud.
//
vector<Point> torsion_points(Curvedata& E)  // After Darrin Doud, adapted by JC
{
  if ( E.isnull() ) return vector<Point>(0);
//
// table[i][] contains a list of possible maximal orders for a point,
// given that the 2-torsion subgroup has order i
//
  static long table[5][5] = {{},{5,7,9,3},{12,6,8,4,10},{},{8,6,4}};
  static long nt[5] = {0,4,5,0,3};
#ifdef DEBUG_TORSION
  cout<<"\nIn torsion_points() with curve "<<(Curve)E<<"\n";
#endif
  bigint a1,a2,a3,a4,a6,sa2,sa4,sa6, x, y ;
  E.getai(a1,a2,a3,a4,a6);
  bigfloat ra1=I2bigfloat(a1), ra2=I2bigfloat(a2), ra3=I2bigfloat(a3);
  long i,j,ntp=1,nt2;
  vector<Point> points;
  vector<Point> cycle;
  Cperiods per(E);  bigcomplex w1,w2,z,z2; 
  per.getwRI(w1,w2); z2=w2/to_bigfloat(2);
  int ncc = getconncomp(E);
  int found;
#ifdef DEBUG_TORSION
  cout<<"Periods: "<<per<<"\nReal Period = "<<w1<<endl;
  cout<<ncc<<" real component(s)"<<endl;
#endif
  points.push_back(Point(E)) ;       // zero point
  Point p, q;

  // We find the two-torsion algebraically
  
  vector<Point> two_tors = two_torsion(E);
  nt2=two_tors.size();
#ifdef DEBUG_TORSION
  cout<<"Size of 2-torsion subgroup = " << nt2 << endl;
  cout << two_tors << endl;
#endif

  for(i=0, found=0; (i<nt[nt2])&&(!found); i++)
    {
      long ni=table[nt2][i]; 
#ifdef DEBUG_TORSION
      cout<<"Looking for a point of order " << ni << "\n";
#endif
      if(ni==3)
	{
	  p=Point(E);
	  vector<Point> p3 = three_torsion(E);
	  if(p3.size()>1) {p=p3[1];}
	}
      else
	{
	  z=w1/to_bigfloat(ni);
	  p = make_tor_pt(E,per,ra1,ra2,ra3,z);
	}
#ifdef DEBUG_TORSION
      cout<<"p = " << p <<"?\n";
#endif
      found=(p.isvalid())&&(order(p,cycle)==ni);
      if(!found&&(ncc==2)&&even(ni))
	{
	  p = make_tor_pt(E,per,ra1,ra2,ra3,z+z2);
#ifdef DEBUG_TORSION
	  cout<<"p = " << p <<"?\n";
#endif
	  found=(p.isvalid())&&(order(p,cycle)==ni);
	  if(!found&&((ni%4)==2))
	    {
	      p = make_tor_pt(E,per,ra1,ra2,ra3,z+z+z2);
#ifdef DEBUG_TORSION
	      cout<<"p = " << p <<"?\n";
#endif
	      found=(p.isvalid())&&(order(p,cycle)==ni);
	    }
	}
      if(found)
	{
	  ntp=ni;
	  points=cycle;
#ifdef DEBUG_TORSION
	  cout<<"Found a point " << p << " of order " << ni << endl;
	  cout<<"generating subgroup "<<cycle<<endl;
#endif
	}
#ifdef DEBUG_TORSION
      else
	{
	  cout<<"none\n";
	}
#endif
    }

#ifdef DEBUG_TORSION
  cout<<"Number of points in cyclic part = " << ntp << endl;
  cout<<points<<endl;
#endif
  
  if(ntp==1) {return two_tors;}                   // C1, C2 or C2xC2
  if(nt2==4)
  // non-cyclic case, C2xC4, C2xC6 or C2xC8
  // Find a point of order 2 not in the second factor and add it in
    {
      for(i=1; i<4; i++) // 0 is first point in two_tors !
	{
	  p=two_tors[i];
	  if(find(points.begin(),points.end(),p)==points.end())
	    {
#ifdef DEBUG_TORSION
	      cout<<"Using 2-torsion point "<<p<<" as coset rep\n";
#endif
	      for(j=0; j<ntp; j++)
		{
		  points.push_back(points[j]+p);
		}
	      ntp*=2;
	      break;
	    }
#ifdef DEBUG_TORSION
	  cout<<"2-torsion point "<<p<<" is in subgroup\n";
#endif
	}
    }
  ::sort(points.begin(), points.end(), Point_cmp);
 return points;
}

//#define DEBUG_DIVISION_POINTS 1
vector<Point> Point::division_points(int m) // list of Q s.t. n*Q=this
{
  vector<Point> ans;
  vector<Point> Qs;
  Point Q;
  if (is_torsion())
    {
      Qs = torsion_points(*E);
      for (vector<Point>::iterator Qi = Qs.begin(); Qi!=Qs.end(); Qi++)
        {
          Q = *Qi;
          if (m * Q == *this)
            ans.push_back(Q);
        }
      return ans;
    }
  ZPoly pol = division_points_X_pol(E->a1,E->a2,E->a3,E->a4,E->a6, m, X, Z);
#ifdef DEBUG_DIVISION_POINTS
  cout << "division poly = " << pol << endl;
#endif

  vector<bigrational> xQs = roots(pol);
#ifdef DEBUG_DIVISION_POINTS
  cout << " with roots " << xQs << endl;
#endif
  for(vector<bigrational>::iterator xQi = xQs.begin(); xQi!=xQs.end(); xQi++)
    {
      Qs = points_from_x(*E, *xQi);
      // will have length 0 or 2 since non-torsion, and we only want
      // exctly one when there two so, must check which works
      if (Qs.size()>0)
        {
          Q = Qs[0];
          if (m*Q == *this)
            {
              ans.push_back(Q);
            }
          else
            {
              ans.push_back(-Q);
            }
        }
    }
  return ans;
}

// end of file: points.cc
