/**
 * \file
 * \brief Implementation of TCT::ModuleDoubleChannelAnalysis methods.
 */

#include "modules/ModuleDoubleChannelAnalysis.h"
#include "TStyle.h"
#include "TMath.h"
#include "TPaveStats.h"
#include <string>
#include <sstream>
#include "TLegend.h"

namespace TCT {

/// Analyse data (should be reimplemented by developer)
bool ModuleDoubleChannelAnalysis::Analysis() {

    TDirectory *dir_fsearch = f_rootfile->mkdir("DoubleChannelAnalysis");
    dir_fsearch->cd();

    //Number of channel one and channel 2;
    Int_t NumOptAxis, NumScanAxis1, NumScanAxis2; // Amount of steps for each axis

    Float_t OptStartPoint, OptStep;                       // Optical axis start point and step
    Float_t Scan1StartPoint, Scan1Step;                    // Scaning 1 axis start poind and step
    Float_t Scan2StartPoint, Scan2Step;                   // Scaning 2 axis start poind and step

    Int_t Ch1Number = (config->CH1_Det())-1;              // select the oscilloscope channel 1
    Int_t Ch2Number = (config->CH2_Det())-1;              // select the oscilloscope channel 2
    Int_t optic_axis = (config->OptAxis())-1;            // select optic axis (0=x,1=y,2=z)
    Int_t scanning_axis1 = config->ScAxis()-1;          // select scanning axis (0=x,1=y,2=z)
    Int_t scanning_axis2 = 3 - (config ->ScAxis()-1) - (config->OptAxis()-1);          // select scanning axis (0=x,1=y,2=z)

    Int_t nVolts1 = 1;
    Int_t nVolts2 = 1;

    //Int_t volt_source=(config->VoltSource());            // select

    Float_t *voltages1;
    Float_t *voltages2;

    nVolts1=stct->NU1; voltages1=stct->U1.fArray;  //u1
    nVolts2=stct->NU2; voltages2=stct->U2.fArray;  //u2

    SwitchAxis(optic_axis,NumOptAxis,OptStep,OptStartPoint);
    SwitchAxis(scanning_axis1,NumScanAxis1,Scan1Step,Scan1StartPoint);
    SwitchAxis(scanning_axis2,NumScanAxis2,Scan2Step,Scan2StartPoint);

    int TotalNPoints = nVolts1*nVolts2*NumScanAxis2*NumOptAxis;

    TCTWaveform **wf1 = new TCTWaveform*[TotalNPoints];
    TCTWaveform **wf2 = new TCTWaveform*[TotalNPoints];

    TGraph **charges1 = new TGraph*[TotalNPoints];
    TGraph **charges2 = new TGraph*[TotalNPoints];
    std::string str[TotalNPoints];
    int k = 0;

    switch ((scanning_axis1+1)) {
    case 1:    for(int n = 0; n < NumOptAxis; n++) {
            for(int m = 0; m < NumScanAxis2; m++) {
               for(int i = 0; i < nVolts1; i++) {
                   for(int j = 0; j < nVolts2; j++) {
                       wf1[k] = stct->Projection(Ch1Number,scanning_axis1,0,m,n,i,j,NumScanAxis1);
                       //std::cout << stct->GetHA(Ch2Number,0,m,n,i,j)->GetTitle() << std::endl;
                       std::ostringstream outp;
                       outp << stct->GetHA(Ch2Number,0,m,n,i,j)->GetTitle();
                       str[k]= outp.str().c_str() ;
                       std::size_t pos = str[k].find("y=");
                       str[k] = str[k].substr (pos);
                       charges1[k]=wf1[k]->CCE(config->FTlowCH1(),config->FThighCH1());   //integrate the charge in time window
                       wf2[k] = stct->Projection(Ch2Number,scanning_axis1,0,m,n,i,j,NumScanAxis1);
                       charges2[k]=wf2[k]->CCE(config->FTlowCH2(),config->FThighCH2());   //integrate the charge in time window
                       k++;
                   }
               }
           }
       }
       break;
    case 2:    for(int n = 0; n < NumOptAxis; n++) {
            for(int m = 0; m < NumScanAxis2; m++) {
               for(int i = 0; i < nVolts1; i++) {
                   for(int j = 0; j < nVolts2; j++) {
                       wf1[k] = stct->Projection(Ch1Number,scanning_axis1,m,0,n,i,j,NumScanAxis1);
                       //std::cout << stct->GetHA(Ch2Number,0,m,n,i,j)->GetTitle() << std::endl;
                       std::ostringstream outp;
                       outp << stct->GetHA(Ch2Number,m,0,n,i,j)->GetTitle();
                       str[k]= outp.str().c_str() ;
                       std::size_t pos = str[k].find("y=");
                       str[k] = str[k].substr (pos);
                       charges1[k]=wf1[k]->CCE(config->FTlowCH1(),config->FThighCH1());   //integrate the charge in time window
                       wf2[k] = stct->Projection(Ch2Number,scanning_axis1,m,0,n,i,j,NumScanAxis1);
                       charges2[k]=wf2[k]->CCE(config->FTlowCH2(),config->FThighCH2());   //integrate the charge in time window
                       k++;
                   }
               }
           }
       }
     break;
     case 3:     for(int n = 0; n < NumOptAxis; n++) {
            for(int m = 0; m < NumScanAxis2; m++) {
               for(int i = 0; i < nVolts1; i++) {
                   for(int j = 0; j < nVolts2; j++) {
                       wf1[k] = stct->Projection(Ch1Number,scanning_axis1,n,m,0,i,j,NumScanAxis1);
                       //std::cout << stct->GetHA(Ch2Number,0,m,n,i,j)->GetTitle() << std::endl;
                       std::ostringstream outp;
                       outp << stct->GetHA(Ch2Number,n,m,0,i,j)->GetTitle();
                       str[k]= outp.str().c_str() ;
                       std::size_t pos = str[k].find("y=");
                       str[k] = str[k].substr (pos);
                       charges1[k]=wf1[k]->CCE(config->FTlowCH1(),config->FThighCH1());   //integrate the charge in time window
                       wf2[k] = stct->Projection(Ch2Number,scanning_axis1,n,m,0,i,j,NumScanAxis1);
                       charges2[k]=wf2[k]->CCE(config->FTlowCH2(),config->FThighCH2());   //integrate the charge in time window
                       k++;
                   }
               }
           }
       }

        break;

    }


    for(k = 0; k < TotalNPoints; k++) {

        TMultiGraph *mg = new TMultiGraph();

        gStyle->SetOptFit(0);
        charges1[k]->SetLineColor(2);
        charges2[k]->SetLineColor(3);
        charges1[k]->SetMarkerSize(0);
        charges2[k]->SetMarkerSize(0);
        mg->Add(charges1[k],"l");
        mg->Add(charges2[k],"l");





        mg->Draw("A");

        mg->SetName(str[k].c_str());
        mg->SetTitle("Channel 1 - Red Line    Channel 2 - Green Line ");
        mg->GetXaxis()->SetTitle("x [um]");
        mg->GetYaxis()->SetTitle("Total charge [arb.]");
        mg->Write();

        //mg->BuildLegend();
        /*auto legend = new TLegend(0.1,0.7,0.48,0.9,"double_channel","Signal Channels");
        legend->AddEntry(charges1[k],"Histogram filled with random numbers", "l");
        legend->AddEntry(charges2[k],"Function abs(#frac{sin(x)}{x})","l");
        legend->Draw();
        */
        delete mg;
    }
}

/// Check before analysis (should be reimplemented by developer)
bool ModuleDoubleChannelAnalysis::CheckModuleData() {

    std::cout<<"\t- DO "<<GetTitle()<<"(type - "<<GetType()<<")\n";


    Int_t NSc;
    std::cout<<"\t\t-- Optical Axis: "<< config->OptAxis() <<std::endl;
    std::cout<<"\t\t-- Scaning Axis: "<< config->ScAxis() <<std::endl;
    std::cout<<"\t\t-- Another Axis: "<< 3 - (config ->ScAxis()-1) - (config->OptAxis()-1) + 1 <<std::endl;
    std::cout<<"\t\t-- First Channel: "<< config->CH1_Det() <<std::endl;
    std::cout<<"\t\t-- Second Channel: "<< config->CH2_Det() <<std::endl;


    std::cout<<"\t\t-- Scanning Axis: "<< config->ScAxis() <<std::endl;
    switch(config->ScAxis()) {
    case 1: NSc = stct->Nx; break;
    case 2: NSc = stct->Ny; break;
    case 3: NSc = stct->Nz; break;
    }
    if(NSc>5) std::cout<<"\t\t\tScanning axis contains "<<NSc<<" points. OK"<<std::endl;
    else {
        std::cout<<"\t\t\tScanning axis contains only "<<NSc<<" point. Not enough for graph plots."<<std::endl;
        return false;
    }

    std::cout<<GetTitle()<<" Data Test Passed. Processing..."<<std::endl;
    return true;
}

}
