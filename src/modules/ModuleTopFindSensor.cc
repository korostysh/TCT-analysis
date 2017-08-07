/**
 * \file
 * \brief Implementation of TCT::ModuleTopFindSensor methods.
 */

#include "modules/ModuleTopFindSensor.h"
#include "TStyle.h"
#include "TMath.h"
#include "TPaveStats.h"
#include "TGraph2D.h"
#include "TCanvas.h"


namespace TCT {

/// Analyse data (should be reimplemented by developer)
bool ModuleTopFindSensor::Analysis() {

    TDirectory *dir_possearch = f_rootfile->mkdir("Sensor Position Search");
    dir_possearch->cd();

    Int_t numS1,numS2;
    Float_t Ss1,Sc0_1;                       // Optical axis step
    Float_t Ss2,Sc0_2;

    Int_t ChNumber=(config->CH1_Det())-1;              // select the oscilloscope channel
    Int_t scanning2_axis= 3 - (config->ScAxis()-1) - (config->OptAxis()-1);            // selecting scaning axis2 which is not scaning axis1 and not optic (0=x,1=y,2=z)
    Int_t scanning1_axis=config->ScAxis()-1;         // select scanning axis1 (0=x,1=y,2=z)


    SwitchAxis(scanning1_axis,numS1,Ss1,Sc0_1);
    SwitchAxis(scanning2_axis,numS2,Ss2,Sc0_2);


    TGraph **cc = new TGraph*[numS1];                  // charge collection graph
    TGraph2D  *collecting_map = new TGraph2D();     // charhe collection map


     //calculate the arb charge from the current
    CalculateCharges(ChNumber,scanning1_axis,numS1,scanning2_axis,numS2,cc,config->FTlowCH1(),config->FThighCH1());
    int k = 0;
    Double_t *y_values = 0;
    for (int i = 0 ; i < numS1; i++ ){
        y_values = cc[i]->GetY();
        for (int j = 0; j <  numS2; j++){
           collecting_map->SetPoint(k,(Sc0_1+i*Ss1),(Sc0_2+j*Ss2),y_values[j]); //NOTE change scaning axis
           k++;
        }
    }

    collecting_map->SetTitle("Sensor Positioning");
    collecting_map->Draw("AP");
    collecting_map->GetYaxis()->SetTitle("Y axis");
    collecting_map->GetXaxis()->SetTitle("X axis");
    collecting_map->Write("SensorPositionSearch");

    delete cc;
    delete collecting_map;
    return true;
}
bool ModuleTopFindSensor::CheckModuleData() {

    std::cout<<"\t- DO "<<GetTitle()<<"(type - "<<GetType()<<")\n";

    Int_t numS1,numS2;

    Float_t Ss1,Sc0_1;                       //First scanig axis step
    Float_t Ss2,Sc0_2;                       //Second scaning axis step

    Int_t scanning2_axis= 3 - (config->ScAxis()-1) - (config->OptAxis()-1);            // selecting scaning axis2 which is not scaning axis1 and not optic (0=x,1=y,2=z)
    Int_t scanning1_axis=config->ScAxis()-1;


    Int_t NOpt;
    Int_t NSc;
    std::cout<<"\t\t-- Optical Axis: "<< config->OptAxis() <<std::endl;

    SwitchAxis(scanning1_axis,numS1,Ss1,Sc0_1);
    SwitchAxis(scanning2_axis,numS2,Ss2,Sc0_2);


    std::cout<<"\t\t-- Second scaning axis: "<<  scanning2_axis <<std::endl;
    switch(config->OptAxis()) {
    case 1: NOpt = stct->Nx; break;
    case 2: NOpt = stct->Ny; break;
    case 3: NOpt = stct->Nz; break;
    }
    if(numS1>=1) std::cout<<"\t\t\tFirst scaning axis scan contains "<<numS1<<" points. OK"<<std::endl;
    else {
        std::cout<<"\t\t\tFirst scaning axis contains only "<<numS1<<" points. Not enough for sensor search."<<std::endl;
        return false;
    }
    std::cout<<"\t\t-- Scanning Axis: "<< config->ScAxis() <<std::endl;
    switch(config->ScAxis()) {
    case 1: NSc = stct->Nx; break;
    case 2: NSc = stct->Ny; break;
    case 3: NSc = stct->Nz; break;
    }
    if(numS2>=1) std::cout<<"\t\t\tSecond scaning axis contains "<<numS2<<" points. OK"<<std::endl;
    else {
        std::cout<<"\t\t\tSecond scaning axis contains only "<<numS2<<" point. Not enough for sensor search."<<std::endl;
        return false;
    }

    std::cout<<GetTitle()<<" Data Test Passed. Processing..."<<std::endl;
    return true;

}
}
