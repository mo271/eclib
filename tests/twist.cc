// twist.cc: program to compute & display quadratic twists
//////////////////////////////////////////////////////////////////////////
//
// Copyright 1990-2012 John Cremona
// 
// This file is part of the mwrank package.
// 
// mwrank is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
// 
// mwrank is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// 
// You should have received a copy of the GNU General Public License
// along with mwrank; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
// 
//////////////////////////////////////////////////////////////////////////
 
#include "curve.h"
#include "getcurve.h"

int main(){

  initprimes(string("PRIMES").c_str(),0);

  Curvedata D, E;
  CurveRed CR;
  bigint c4,c6,twist2;
  bigint v;
  int verbose=0;
  vector<bigrational> ai(5);


while (getcurve(ai,verbose))
{
  Curvedata D(Curvedata(ai,v),1);
  E = D;
  CR = CurveRed(E);
  cout << "\nCurve is:  " << endl;  CR.display(cout);
  
  long twist=1;
  
  while(1)
    {
      cout << "\nEnter a twist value: ";
      cout << "\n(0 to set a new `original' curve, 1 to twist original,\n";
      cout << "or any other integer to twist immediately preceding output)\n ";
      cin>>ws;  if(cin.eof()) {cout<<endl; exit(0);}
      cin >> twist;
      if(twist==0) break;
      if(twist==1) { E=D;}
      else
	{
	  E.getci(c4, c6);
	  twist *= 4;        //corrects for twist not congruent -1 mod 4
	  twist2 = twist*twist;
	  c4 *= twist2;
	  c6 *= twist*twist2;
	  E = Curvedata(Curve(c4, c6), 1); //minimal form
	  CR=CurveRed(E);
	  cout << "\n\nE * " << twist/4 << " is:  \n"; CR.display(cout); 
	  cout << endl;
	}
    }
}


} //ends main
