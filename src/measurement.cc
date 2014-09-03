/**
 * \file
 * \brief Implementation of measurement methods
 */

#include<string>
#include "measurement.h"
#include "acquisition.h"
#include "TFile.h"

//  includes from ROOT libraries
#include "TSystem.h"

namespace TCT {

  bool measurement::AcqsLoader(std::vector<TCT::acquisition_single> *allAcqs, uint32_t MaxAcqs){

    //if(debug) std::cout << "start PulseCheck" << std::endl;

    if(MaxAcqs == 0) {
      std::cout << "MaxAcqs is defaulted to zero, stopping " << std::endl;
      return false;
    }

    const char* filedir = _DataInFolder.c_str(); // !! change to encapsulation 
    /*
     */
    //  gROOT->ProcessLine("#include <vector>");
    printf("read files from %s\n",filedir);
    //if (!AllTests) Init();
    //GetCuts(filedir);

    // get list of files in filedir
    void *dir = gSystem->OpenDirectory(filedir);
    const char *infile;
    uint32_t nfiles = 0;
    //std::cout << "do read in" << std::endl;
    //char key = getchar();
    FILE *file;
    while(infile = gSystem->GetDirEntry(dir)) {
      if (strstr(infile,".txt") && !strstr(infile,".swp") ) {
	//printf("file: %s\n",infile);
	Char_t pathandfile[250];
	strcpy(pathandfile,filedir);
	strcat(pathandfile,infile);
	if(nfiles < 10) std::cout << "read file from: " << pathandfile << std::endl;
	if(nfiles == 10) std::cout << "suppressing further 'read from' info" << std::endl;
	file = fopen(pathandfile,"r");
	if (!file) {
	  std::cout <<"Can't open file"<<std::endl;
	  exit(1);
	}

	TCT::acquisition_single acq(nfiles); 
	//std::cout << acq->BiasVolt() << std::endl;
	//std::cout << "start Read() " << nfiles << std::endl;
	bool read = acq.Read(file, nfiles);
	fclose(file);
	//delete[] file; // doesnt work
	//if (read) std::cout << "File read!" << std::endl;
	nfiles++;
	//std::cout << "nfiles: " << nfiles;
	allAcqs->push_back(acq);
	if (nfiles > MaxAcqs && MaxAcqs > 0) break;
      }
    }
    //std::cout << "end read in" << std::endl;
    //char key = getchar();

    

    if(nfiles == 0) { 
      std::cout << " no files read, exiting..." << std::endl; return false;
    } else {
      std::cout << "Found " << allAcqs->size() << "acquision, proceed with analysis\n" <<std::endl;
    }

    return true;
  } // end of LoadAcqs()



  void measurement::AcqsAnalyser(TCT::acquisition_single *acq, uint32_t iAcq, TCT::acquisition_avg *acqAvg){

    int debug = 10;
    if(debug) std::cout << "start AcqsAnalyser" << std::endl;

    std::cout << "GOF" << std::endl;
    acq->GetOffsetNoise(iAcq, acqAvg);

    std::cout << "FHacqs" << std::endl;
    acq->FillHacqs();

    std::cout << "SF" << std::endl;
    acq->SignalFinder(acqAvg);

    std::cout << "FillNt" << std::endl;
    acq->FillNtuple(acqAvg); // !! this is w/o selection cuts

    std::cout << "Fill2DHs" << std::endl;
    acq->Fill2DHistos(acqAvg); // !! this is w/o selection cuts


    // amount of delayfilt depends on steepness, hence needs shifting now

    /*float meanfilt = 0.;
    for(Int_t i = 0; i<300; i++){
      //cout << ",  " << hProffilt->GetBinContent(i);
      meanfilt += hProffilt->GetBinContent(i+1);
    }
    meanfilt = meanfilt / 300.;
    cout << "Meanfilt = " << meanfilt << endl;

    float rmsfilt = 0.;
    for(Int_t i = 0; i<300; i++) rmsfilt += (hProffilt->GetBinContent(i+1) - meanfilt)*(hProffilt->GetBinContent(i+1) - meanfilt);
    rmsfilt /= 300.;
    cout << "rmsfilt = " << rmsfilt << endl;
    float noisefilt = TMath::Power(rmsfilt,0.5);
    cout << "noisefilt = " << noisefilt << endl;

    int count = 300;
    float limit = 3.*2*sqrt(30)*noisefilt + meanfilt;
    cout << "limit = " << limit << endl;
    while(hProffilt->GetBinContent(count) + hProffilt->GetBinContent(count+1) + hProffilt->GetBinContent(count+2) < limit){
      count++;
      if(count >  1000) {
	cout <<"problem for hProffilt" <<endl;
	break;
      }

    }
    cout << "count = " << count << endl;
    float delayfilt2 = ((float)count-500.)/10.;
    cout << "delayfilt2 = " << delayfilt2 << endl;



  */
    return;	
  }

  bool measurement::AcqsSelecter(TCT::acquisition_single *acq){

    int debug = 10;
    if (debug > 9) std::cout << " selects acquisition based on certain criteria" << std::endl;

    bool select = true;

    if(acq->SelectionRan()) return acq->Select();
    else {
    	if(acq->Noise() > Noise_Cut()) {
	  select = false;
	  std::cout << " noise too high: " << acq->Noise() << " Noise cut = " << Noise_Cut() << std::endl;
	}
    }

    acq->SetSelectionRan(true);
    acq->SetSelect(select);

    return select;
  }

  void measurement::AcqsProfileFiller(TCT::acquisition_single *acq, TCT::acquisition_avg *acqAvg) {

    if (acq->Select()){
      Float_t tmp_t = -1.0;
      Float_t tmp_tfilt = -1.0;
      Float_t tmp_v = -1.0;

      for(Int_t j = 0; j < acq->Nsamples(); j++) {
	tmp_t = acq->time[j] - acq->Delay();
	tmp_tfilt = acq->time[j] - acq->Delayfilt();
	tmp_v = acq->volt[j] - acq->Offset();

	acqAvg->H2_acqs2D()->Fill(tmp_t,tmp_v,1.);
	acqAvg->Profile()->Fill(tmp_t,tmp_v,1.);
	acqAvg->ProfileFILTERED()->Fill(tmp_tfilt,tmp_v,1.); // !! ProfileFILTERED is not filtered, but shifted by certain amount with calculation based on filtered signal, for low SNR only
      }

    } 
    return;

  }

  void measurement::AcqsWriter(TCT::sample *sample, std::vector<TCT::acquisition_single> *allAcqs, TCT::acquisition_avg *acqAvg){

  

  // write results
  //string outhelp;

  //string outfolder = folder; 

  //std::cout << "size " <<   filedirstring.size() << std::endl;
  //string str1 = filedirstring.substr(0,filedirstring.size()-1);
  //string outvolt = str1.substr(str1.rfind("/")+1);
  //std::cout << outvolt << std::endl;
  //string outvolt = std::to_string(BiasVolt());

  //string str2 = str1.substr(0,str1.rfind("/"));
  //string outtemp = str2.substr(str2.rfind("/")+1);
  //std::cout << outtemp << std::endl;
  //string outtemp = std::to_string(Temp());

  //string sample = "S57_23";

  //outhelp = outfolder + "/" + sample + "/" + outtemp + "/" + sample + "_" + outtemp + "_" + outvolt + ".root";
  //std::cout << "outfile name: " << outhelp << std::endl;


  //WriteRoot(sdt::string folder, std::vector<TCT::acquisition_single> *allAcqs, TCT::acquisition_avg *acqAvg); // ?? Does this make sense? Or call WriteRoot from main directly?

  //if(nvolt > 1 || ntemp > 1){
  //std::cout << "		clearing root" << std::endl;
  //ClearRoot();

  std::string outfolder = OutFolder(); 
  std::string outsample = sample->SampleID();
  std::string outtemp = std::to_string((int)acqAvg->Temp());
  std::string outvolt = std::to_string((int)acqAvg->BiasVolt());

  std::string filename = outfolder + "/" + outsample + "/" + outtemp + "K/" + outsample + "_" + outtemp + "K_" + outvolt + "V.root";
  std::cout << "\n *** outfile written to: " << filename << " *** " << std::endl;
  TFile* hfile = new TFile(filename.c_str(),"RECREATE","TCTanalyser");

  hfile->cd();
  acqAvg->N_tuple()->Write();
  acqAvg->H2_acqs2D()->Write();
  acqAvg->Profile()->Write();
  //acqAvg->ProfileFILTERED()->Write();
  acqAvg->H2_delay_width()->Write();
  acqAvg->H2_ampl_width()->Write();
  acqAvg->H2_delay_ampl()->Write();
  acqAvg->H2_rise1090_ampl()->Write();
  acqAvg->H_noise()->Write();
  acqAvg->G_noise_evo()->Write();
  acqAvg->G_s2n_evo()->Write();
  

  hfile->Close();
  std::cout << "end AcqsWriter" << std::endl;
  return;


  }
}
