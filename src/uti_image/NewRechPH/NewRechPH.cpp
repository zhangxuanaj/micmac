/*Header-MicMac-eLiSe-25/06/2007

    MicMac : Multi Image Correspondances par Methodes Automatiques de Correlation
    eLiSe  : ELements of an Image Software Environnement

    www.micmac.ign.fr

   
    Copyright : Institut Geographique National
    Author : Marc Pierrot Deseilligny
    Contributors : Gregoire Maillet, Didier Boldo.

[1] M. Pierrot-Deseilligny, N. Paparoditis.
    "A multiresolution and optimization-based image matching approach:
    An application to surface reconstruction from SPOT5-HRS stereo imagery."
    In IAPRS vol XXXVI-1/W41 in ISPRS Workshop On Topographic Mapping From Space
    (With Special Emphasis on Small Satellites), Ankara, Turquie, 02-2006.

[2] M. Pierrot-Deseilligny, "MicMac, un lociel de mise en correspondance
    d'images, adapte au contexte geograhique" to appears in 
    Bulletin d'information de l'Institut Geographique National, 2007.

Francais :

   MicMac est un logiciel de mise en correspondance d'image adapte 
   au contexte de recherche en information geographique. Il s'appuie sur
   la bibliotheque de manipulation d'image eLiSe. Il est distibue sous la
   licences Cecill-B.  Voir en bas de fichier et  http://www.cecill.info.


English :

    MicMac is an open source software specialized in image matching
    for research in geographic information. MicMac is built on the
    eLiSe image library. MicMac is governed by the  "Cecill-B licence".
    See below and http://www.cecill.info.

Header-MicMac-eLiSe-25/06/2007*/


#include "NewRechPH.h"

std::string NameFileNewPCarac(const std::string & aNameGlob,bool Bin)
{
    std::string aDirGlob = DirOfFile(aNameGlob);
    std::string aDirLoc= "NewPH/";
    ELISE_fp::MkDirSvp(aDirGlob+aDirLoc);
    return aDirGlob+aDirLoc + NameWithoutDir(aNameGlob) + (Bin ? ".dmp" : ".xml");
}

bool  cAppli_NewRechPH::BrinStable(const cBrinPtRemark & aBr) const
{
  int aN0 = aBr.Niv0();
  int aN1 = aN0 + aBr.Long();

  return ScaleOfNiv(aN1) >=  2 * ScaleOfNiv(aN0);

}


void  cAppli_NewRechPH::Clik()
{
   if (mW1) mW1->clik_in();
}

void cAppli_NewRechPH::AddScale(cOneScaleImRechPH * aI1,cOneScaleImRechPH *)
{
    mVI1.push_back(aI1);
}

void cAppli_NewRechPH::AddBrin(cBrinPtRemark * aBr)
{
   mVecB.push_back(aBr);
}



cPtSc CreatePtSc(const Pt2dr & aP,double aSc)
{
    cPtSc aRes;
    aRes.Pt()    = aP;
    aRes.Scale() = aSc;
    return aRes;
}

cAppli_NewRechPH::cAppli_NewRechPH(int argc,char ** argv,bool ModeTest) :
    mPowS     (pow(2.0,1/5.0)),
    mNbS      (30),
    mS0       (1.0),
    mW1       (0),
    mModeTest (ModeTest),
    mDistMinMax (3.0),
    mDoMin      (true),
    mDoMax      (true),
    mDoPly      (true),
    mPlyC       (0),
    mHistLong   (1000,0),
    mHistN0     (1000,0)
{
   double aSeuilPersist = 1.0;

   MMD_InitArgcArgv(argc,argv);
   ElInitArgMain
   (
         argc,argv,
         LArgMain()  << EAMC(mName, "Name Image",  eSAM_IsPatFile),
         LArgMain()   << EAM(mPowS, "PowS",true,"Scale Pow")
                      << EAM(mNbS,  "NbS",true,"Number of level")
                      << EAM(mS0,   "S0",true,"ScaleInit, Def=1")
                      << EAM(mDoPly, "DoPly",true,"Generate ply file, for didactic purpose")
                      << EAM(mBox, "Box",true,"Box for computation")
                      << EAM(mModeTest, "Test",true,"if true add W")
                      << EAM(aSeuilPersist, "SP",true,"Threshold persistance")
   );
   mNbInOct = log(2) / log(mPowS);

   if (mDoPly)
   {
      mPlyC = new cPlyCloud;
   }

   Pt2di aP0(0,0);
   Pt2di aP1(-1,-1);
   if (EAMIsInit(&mBox))
   {
       aP0 = mBox._p0;
       aP1 = mBox._p1;
   }
   // Create top scale
   AddScale(cOneScaleImRechPH::FromFile(*this,mS0,mName,aP0,aP1),0);

   // Create matr of link, will have do it much less memory consuming (tiling of list ?)
   mSzIm = mVI1.back()->Im().sz();
   mBufLnk  = std::vector<std::vector<cPtRemark *> >(mSzIm.y,std::vector<cPtRemark *>(mSzIm.x,(cPtRemark *)0));

   double aScaleMax = mS0*pow(mPowS,mNbS);
   // Used for  nearest point researh
   mVoisLnk = SortedVoisinDisk(-1,aScaleMax+4,true);


   if (! EAMIsInit(&mDZPlyLay))
   {
      mDZPlyLay = ElMin(mSzIm.x,mSzIm.y)/ double(mNbS);
   }
   if (mModeTest)
   {
      mW1 = Video_Win::PtrWStd(mSzIm);
   }
   // mVI1.back()->Show(mW1);

   for (int aK=0 ; aK<mNbS ; aK++)
   {
        // Init from low resol
        if (aK!=0)
        {
           AddScale
           (
              cOneScaleImRechPH::FromScale(*this,*mVI1.back(),mS0*pow(mPowS,aK)),
              0
           );

        }
        // Compute point of scale
        mVI1.back()->CalcPtsCarac();
        mVI1.back()->Show(mW1);
          
        // Links the point at different scale
        if (aK!=0)
        {
           mVI1[aK]->CreateLink(*(mVI1[aK-1]));
        }
   }


   Clik();

   for (int aK=0 ; aK<mNbS ; aK++)
   {
       mVI1[aK]->Export((aK!=0) ? mVI1[aK-1] : 0,mPlyC);
   }

   if (mPlyC)
   {
       mPlyC->PutFile("NewH.ply");
   }

   cSetPCarac aSPC;
   int aNb0 = 0;
   int aNb1 = 0;
   int aNb2 = 0;
   int aNbTot=0;
   for (const auto & aPBr  :  mVecB)
   {
//std::cout << "AAAAA\n";
       int aN0 = aPBr->Niv0();
       int aN1 = aN0 + aPBr->Long();
       mHistLong.at(aPBr->Long())++;
       mHistN0.at(aN0)++;

       if (aPBr->Long()>= (aSeuilPersist*mNbInOct))
       {
          aNbTot++;
          if (aN0==0)
          {
              aSPC.OnePCarac().push_back(cOnePCarac());
              cOnePCarac & aPC = aSPC.OnePCarac().back();

              aPC.Kind() = aPBr->P0()->Type();
              int aNiv_h,aNiv_l;

              aPC.HR() = CreatePtSc(aPBr->P0()->Pt(),ScaleOfNiv(aN0));

              cPtRemark *aPhR = aPBr->Nearest(aNiv_h,aN0 + mNbInOct);
              aPC.hR() =  CreatePtSc(aPhR->Pt(),ScaleOfNiv(aNiv_h));

              cPtRemark *aPlR = aPBr->Nearest(aNiv_l,aN1 - mNbInOct);
              aPC.lR() =  CreatePtSc(aPlR->Pt(),ScaleOfNiv(aNiv_l));

              aPC.LR() = CreatePtSc(aPBr->PLast()->Pt(),ScaleOfNiv(aN1));

              aNb0++;
          }
          else if (aN0==1) aNb1++;
          else if (aN0==2) aNb2++;

       }
//std::cout << "FFFFFF\n";

   }

    MakeFileXML(aSPC,NameFileNewPCarac(mName,true));
    MakeFileXML(aSPC,NameFileNewPCarac(mName,false));
    std::cout << "Prop0 =" << aNb0 / double(aNbTot) << "\n";
    std::cout << "Prop1 =" << aNb1 / double(aNbTot) << "\n";
    std::cout << "Prop2 =" << aNb2 / double(aNbTot) << "\n";
}

bool cAppli_NewRechPH::Inside(const Pt2di & aP) const
{
    return (aP.x>=0) && (aP.y>=0) && (aP.x<mSzIm.x) && (aP.y<mSzIm.y);
}

tPtrPtRemark &  cAppli_NewRechPH::PtOfBuf(const Pt2di & aP)
{
     
    ELISE_ASSERT(Inside(aP),"cAppli_NewRechPH::PtOfBuf"); 

    return mBufLnk[aP.y][aP.x];
}

tPtrPtRemark  cAppli_NewRechPH::NearestPoint(const Pt2di & aP,const double & aDist)
{
   double aD2 = ElSquare(aDist);
   for (int aKV=0 ; aKV<int(mVoisLnk.size()) ; aKV++)
   {
       const Pt2di & aVois = mVoisLnk[aKV];
       if (square_euclid(aVois) > aD2)
          return 0;
       Pt2di aPV = aP + aVois;
       if (Inside(aPV))
       {
           tPtrPtRemark  aRes = mBufLnk[aPV.y][aPV.x];
           if (aRes) return aRes;
       }
   }
   return 0;
}

const Pt2di & cAppli_NewRechPH::SzIm() const  {return mSzIm;}

double cAppli_NewRechPH::ScaleOfNiv(const int & aNiv) const
{
   return mVI1.at(aNiv)->Scale();
}



int Test_NewRechPH(int argc,char ** argv)
{
   cAppli_NewRechPH anAppli(argc,argv,true);


   return EXIT_SUCCESS;

}



/*Footer-MicMac-eLiSe-25/06/2007

Ce logiciel est un programme informatique servant à la mise en
correspondances d'images pour la reconstruction du relief.

Ce logiciel est régi par la licence CeCILL-B soumise au droit français et
respectant les principes de diffusion des logiciels libres. Vous pouvez
utiliser, modifier et/ou redistribuer ce programme sous les conditions
de la licence CeCILL-B telle que diffusée par le CEA, le CNRS et l'INRIA 
sur le site "http://www.cecill.info".

En contrepartie de l'accessibilité au code source et des droits de copie,
de modification et de redistribution accordés par cette licence, il n'est
offert aux utilisateurs qu'une garantie limitée.  Pour les mêmes raisons,
seule une responsabilité restreinte pèse sur l'auteur du programme,  le
titulaire des droits patrimoniaux et les concédants successifs.

A cet égard  l'attention de l'utilisateur est attirée sur les risques
associés au chargement,  à l'utilisation,  à la modification et/ou au
développement et à la reproduction du logiciel par l'utilisateur étant 
donné sa spécificité de logiciel libre, qui peut le rendre complexe à 
manipuler et qui le réserve donc à des développeurs et des professionnels
avertis possédant  des  connaissances  informatiques approfondies.  Les
utilisateurs sont donc invités à charger  et  tester  l'adéquation  du
logiciel à leurs besoins dans des conditions permettant d'assurer la
sécurité de leurs systèmes et ou de leurs données et, plus généralement, 
à l'utiliser et l'exploiter dans les mêmes conditions de sécurité. 

Le fait que vous puissiez accéder à cet en-tête signifie que vous avez 
pris connaissance de la licence CeCILL-B, et que vous en avez accepté les
termes.
aooter-MicMac-eLiSe-25/06/2007*/
