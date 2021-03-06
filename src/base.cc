/**
 * \file
 * \brief Implementation of Ui::base methods.
 */

#include "base.h"
#include <fstream>

//Qt includes
#include "ui_base.h"
#include "QFileDialog"
#include "ui_form_parameters.h"
#include "ui_form_sample.h"
#include "ui_form_folders.h"
#include "gui_folders.h"
#include "gui_sample.h"
#include "QMessageBox"
#include "QDateTime"
#include "qdebugstream.h"
#include "QProgressDialog"
#include "qdebug.h"
#include "QDirIterator"
#include "QProcess"

//TCT includes
#include "acquisition.h"
#include "measurement.h"
#include <scanning.h>
#include <sample.h>
#include <util.h>

#include <TCTModule.h>
#include <TCTReader.h>

//ROOT includes
#include <TSystem.h>

base::base(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::base)
{
    ui->setupUi(this);

    _DefConfigName = "def";
    config_tct = NULL;
    config_analysis = NULL;
    config_sample = NULL;
    config_mode = NULL;
    progress = NULL;
    progress_osc = NULL;
    browserProcess = NULL;

    //on_comboBox_activated(0);
    ui->buttonGroup_mode->setId(ui->mode_top,0);
    ui->buttonGroup_mode->setId(ui->mode_edge,1);
    ui->buttonGroup_mode->setId(ui->mode_bottom,2);
    on_buttonGroup_mode_buttonClicked(3);

    ui->buttonGroup_scanning->setId(ui->sc_x,1);
    ui->buttonGroup_scanning->setId(ui->sc_y,2);
    ui->buttonGroup_scanning->setId(ui->sc_z,3);

    ui->buttonGroup_optical->setId(ui->opt_x,1);
    ui->buttonGroup_optical->setId(ui->opt_y,2);
    ui->buttonGroup_optical->setId(ui->opt_z,3);

    ui->cc_ch_1->toggle();
    ui->cc_ch_2->toggle();
    ui->cc_ch_ph->toggle();
    ui->cc_ch_trig->toggle();

    ui->buttonGroup_input_type->setId(ui->mode_txt,0);
    ui->buttonGroup_input_type->setId(ui->mode_raw,1);

#ifndef USE_LECROY_RAW
    ui->mode_raw->setEnabled(false);
    ui->mode_raw->setToolTip("The code was built without LeCroy RAW data converter library.");
#endif

    TCT::util basic_config;
    std::ifstream basic_config_file("default.conf");
    if(!basic_config_file.is_open()) {
        error("default.conf file is absent. Please create file and put the DefaultFile variable in it.");
        exit(1);
    }
    basic_config.parse(basic_config_file);
    for( auto i : basic_config.ID_val()){
      if(i.first == "DefaultFile")		_DefConfigName = i.second;
    }
    if(_DefConfigName=="def") {
        error("default.conf doesn't contain the <b>DefaultFile</b> name (the default config file address).");
        exit(1);
    }
    read_config(_DefConfigName.c_str());
    fill_config();


}

base::~base()
{
    if(browserProcess != NULL && browserProcess->isOpen()) browserProcess->kill();
    if(config_mode) delete config_mode;
    if(config_tct) delete config_tct;
    if(config_analysis) delete config_analysis;
    if(config_sample) delete config_sample;
    delete ui;
}

void base::on_buttonGroup_mode_buttonClicked(int index)
{
    ui->group_top->setVisible(!(bool)abs(index-0));
    ui->group_top->setGeometry(300,230,360,230);
    ui->group_edge->setVisible(!(bool)abs(index-1));
    ui->group_edge->setGeometry(300,230,360,230);
    ui->group_bottom->setVisible(!(bool)abs(index-2));
    ui->group_bottom->setGeometry(300,230,360,230);
}
void base::read_config(const char *config_file) {

    if(config_mode) delete config_mode;
    if(config_tct) delete config_tct;
    if(config_analysis) delete config_analysis;
    if(config_sample) delete config_sample;

    qDeleteAll(ui->group_top->children());
    qDeleteAll(ui->group_edge->children());
    qDeleteAll(ui->group_bottom->children());
    top_widgets.clear();
    edge_widgets.clear();
    bottom_widgets.clear();

    TCT::util analysis_card;
    std::ifstream ana_file(config_file);

    if(!ana_file.is_open()) {
        error("Analysis Config file open failed. Please check the filename in default.conf.");
        exit(1);
    }

    analysis_card.parse(ana_file);
    config_mode = new TCT::mode_selector(analysis_card.ID_val());
    config_tct = new TCT::tct_config(analysis_card.ID_val());
    config_analysis = new TCT::analysis(analysis_card.ID_val());

    TCT::util sample_card;
    std::ifstream sample_file(config_mode->SampleCard().c_str());

    if(!sample_file.is_open()) {
        error("Sample Card file open failed. Please specify the existing filename.");
    }

    sample_card.parse(sample_file);
    config_sample = new TCT::sample(sample_card.ID_val());

    config_tct->SetSampleThickness(config_sample->Thickness());
    config_tct->SetOutSample_ID(config_sample->SampleID());

    FillBoxes();

    ui->statusBar->showMessage(tr("Config File Opened"), 2000);

}
void base::fill_config() {

    ui->modes->setCurrentIndex(config_mode->Mode());

    // tct config

    ui->cc_ch_1->setChecked(config_tct->CH1_Det());
    if(config_tct->CH1_Det()) {
        ui->cc_ch_num_1->setCurrentIndex(config_tct->CH1_Det()-1);
        ui->tlow_1->setText(QString::number(config_tct->FTlowCH1()));
        ui->thigh_1->setText(QString::number(config_tct->FThighCH1()));

    }
    ui->cc_ch_2->setChecked(config_tct->CH2_Det());
    if(config_tct->CH2_Det()) {
        ui->cc_ch_num_2->setCurrentIndex(config_tct->CH2_Det()-1);
        ui->tlow_2->setText(QString::number(config_tct->FTlowCH2()));
        ui->thigh_2->setText(QString::number(config_tct->FThighCH2()));

    }
    ui->cc_ch_ph->setChecked(config_tct->CH_PhDiode());
    if(config_tct->CH_PhDiode()) {
        ui->cc_ch_num_ph->setCurrentIndex(config_tct->CH_PhDiode()-1);
        ui->tlow_diode->setText(QString::number(config_tct->FDLow()));
        ui->thigh_diode->setText(QString::number(config_tct->FDHigh()));

    }
    ui->cc_ch_trig->setChecked(config_tct->CH_Trig());
    if(config_tct->CH_Trig()) {
        ui->cc_ch_num_trig->setCurrentIndex(config_tct->CH_Trig()-1);
    }

    ui->buttonGroup_scanning->button(config_tct->ScAxis())->setChecked(true);
    ui->buttonGroup_optical->button(config_tct->OptAxis())->setChecked(true);

    ui->cc_volt_source->setCurrentIndex(config_tct->VoltSource()-1);

    ui->save_sep_chrges->setChecked(config_tct->FSeparateCharges());
    ui->save_sep_waveforms->setChecked(config_tct->FSeparateWaveforms());
    ui->timebetween->setValue(config_tct->Movements_dt());
    ui->bias_line->setValue(config_tct->CorrectBias());

    ui->buttonGroup_mode->button(config_tct->TCT_Mode())->setChecked(true);
    on_buttonGroup_mode_buttonClicked(config_tct->TCT_Mode());

    for(auto i: top_widgets) {
        ((QCheckBox*)i.first)->setChecked(config_tct->GetModule(i.second)->isEnabled());
        config_tct->GetModule(i.second)->FillParameters();
    }
    for(auto i: edge_widgets) {
        ((QCheckBox*)i.first)->setChecked(config_tct->GetModule(i.second)->isEnabled());
        config_tct->GetModule(i.second)->FillParameters();
    }
    for(auto i: bottom_widgets) {
        ((QCheckBox*)i.first)->setChecked(config_tct->GetModule(i.second)->isEnabled());
        config_tct->GetModule(i.second)->FillParameters();
    }

    // oscilloscope config

    ui->MaxAcqs->setValue(config_analysis->MaxAcqs());
    ui->Noise_Cut->setValue(config_analysis->Noise_Cut());
    ui->NoiseEnd_Cut->setValue(config_analysis->NoiseEnd_Cut());

    ui->S2n_Cut->setValue(config_analysis->S2n_Cut());
    ui->S2n_Ref->setValue(config_analysis->S2n_Ref());
    ui->AmplNegLate_Cut->setValue(config_analysis->AmplNegLate_Cut());
    ui->AmplPosLate_Cut->setValue(config_analysis->AmplPosLate_Cut());
    ui->AmplNegEarly_Cut->setValue(config_analysis->AmplNegEarly_Cut());
    ui->AmplPosEarly_Cut->setValue(config_analysis->AmplPosEarly_Cut());
    ui->dosmearing->setChecked(config_analysis->DoSmearing());
    ui->AddNoise->setValue(config_analysis->AddNoise());
    ui->AddJitter->setValue(config_analysis->AddJitter());
    ui->savetofile->setChecked(config_analysis->SaveToFile());
    ui->savesingles->setChecked(config_analysis->SaveSingles());
    ui->PrintEvent->setValue(config_analysis->PrintEvent());

    ui->buttonGroup_input_type->button(config_analysis->LeCroyRAW())->click();
#ifndef USE_LECROY_RAW
    ui->mode_txt->click();
#endif

    ui->Nsamples_start->setEnabled(false);
    ui->Nsamples_end->setEnabled(false);
    ui->PrePulseInterval->setEnabled(false);

         /*
            Nsamples_start	=	50	# in Samples
            Nsamples_end	=	50	# in Samples
            PrePulseInterval	= 	50	# in ns.
            */

}

void base::tovariables_config() {

    config_mode->SetMode(ui->modes->currentIndex());

    // tct part

    if(ui->cc_ch_1->isChecked()) {
        config_tct->SetCH1_Det(ui->cc_ch_num_1->currentText().toInt());
        config_tct->SetFTlowCH1(ui->tlow_1->text().toFloat());
        config_tct->SetFThighCH1(ui->thigh_1->text().toFloat());
    }
    else config_tct->SetCH1_Det(0);

    if(ui->cc_ch_2->isChecked()) {
        config_tct->SetCH2_Det(ui->cc_ch_num_2->currentText().toInt());
        config_tct->SetFTlowCH2(ui->tlow_2->text().toFloat());
        config_tct->SetFThighCH2(ui->thigh_2->text().toFloat());
    }
    else config_tct->SetCH2_Det(0);

    if(ui->cc_ch_ph->isChecked()) {
        config_tct->SetCH_PhDiode(ui->cc_ch_num_ph->currentText().toInt());
        config_tct->SetFDLow(ui->tlow_diode->text().toFloat());
        config_tct->SetFDHigh(ui->thigh_diode->text().toFloat());
    }
    else config_tct->SetCH_PhDiode(0);

    if(ui->cc_ch_trig->isChecked()) {
        config_tct->SetCH_Trig(ui->cc_ch_num_trig->currentText().toInt());
    }
    else config_tct->SetCH_Trig(0);

    config_tct->SetScAxis(ui->buttonGroup_scanning->checkedId());
    config_tct->SetOptAxis(ui->buttonGroup_optical->checkedId());
    config_tct->SetVoltSource(ui->cc_volt_source->currentText().toInt());

    config_tct->SetFSeparateCharges(ui->save_sep_chrges->isChecked());
    config_tct->SetFSeparateWaveforms(ui->save_sep_waveforms->isChecked());

    config_tct->SetMovements_dt(ui->timebetween->value());
    config_tct->SetCorrectBias(ui->bias_line->value());

    config_tct->SetTCT_Mode(ui->buttonGroup_mode->checkedId());

    for(auto i: top_widgets) {
        config_tct->GetModule(i.second)->setEnabled(((QCheckBox*)i.first)->isChecked());
        config_tct->GetModule(i.second)->ToVariables();
    }
    for(auto i: edge_widgets) {
        config_tct->GetModule(i.second)->setEnabled(((QCheckBox*)i.first)->isChecked());
        config_tct->GetModule(i.second)->ToVariables();
    }
    for(auto i: bottom_widgets) {
        config_tct->GetModule(i.second)->setEnabled(((QCheckBox*)i.first)->isChecked());
        config_tct->GetModule(i.second)->ToVariables();
    }

    // oscilloscope part

    config_analysis->SetMaxAcqs(ui->MaxAcqs->value());
    config_analysis->SetNoise_Cut(ui->Noise_Cut->value());
    config_analysis->SetNoiseEnd_Cut(ui->NoiseEnd_Cut->value());
    config_analysis->SetS2n_Cut(ui->S2n_Cut->value());
    config_analysis->SetS2n_Ref(ui->S2n_Ref->value());
    config_analysis->SetAmplNegLate_Cut(ui->AmplNegLate_Cut->value());
    config_analysis->SetAmplPosLate_Cut(ui->AmplPosLate_Cut->value());
    config_analysis->SetAmplNegEarly_Cut(ui->AmplNegEarly_Cut->value());
    config_analysis->SetAmplPosEarly_Cut(ui->AmplPosEarly_Cut->value());
    config_analysis->SetDoSmearing(ui->dosmearing->isChecked());
    config_analysis->SetAddNoise(ui->AddNoise->value());
    config_analysis->SetAddJitter(ui->AddJitter->value());
    config_analysis->SetSaveToFile(ui->savetofile->isChecked());
    config_analysis->SetSaveSingles(ui->savesingles->isChecked());
    config_analysis->SetPrintEvent(ui->PrintEvent->value());
    config_analysis->SetLeCroyRAW((bool)(ui->buttonGroup_input_type->checkedId()));
}

void base::on_window_folders_clicked()
{
    Folders *folders = new Folders(this);

    folders->ui->cc_data->setText(config_tct->DataFolder().c_str());
    folders->ui->cc_out->setText(config_tct->OutFolder().c_str());

    if(folders->exec()) {
        config_tct->SetDataFolder(folders->ui->cc_data->text().toStdString());
        config_tct->SetOutFolder(folders->ui->cc_out->text().toStdString());
        if(config_mode->Mode()==0) {
            config_analysis->SetDataFolder(config_tct->DataFolder());
            config_analysis->SetOutFolder(config_tct->OutFolder());
        }
    }

    delete folders;
}

void base::on_window_sample_clicked()
{

    Sample *sampleUi = new Sample(this);

    sampleUi->ui->cc_sensor->setText(config_mode->SampleCard().c_str());
    sampleUi->ui->folder->setText(config_sample->Folder().c_str());
    sampleUi->ui->id->setText(config_sample->SampleID().c_str());
    sampleUi->ui->thickness->setText(QString::number(config_sample->Thickness()));

    if(sampleUi->exec()) {
        config_sample->SetFolder(sampleUi->ui->folder->text().toStdString());
        config_sample->SetSampleID(sampleUi->ui->id->text().toStdString());
        config_sample->SetThickness(sampleUi->ui->thickness->text().toFloat());
        config_tct->SetSampleThickness(config_sample->Thickness());
        config_tct->SetOutSample_ID(config_sample->SampleID());
        config_mode->SetSampleCard(sampleUi->ui->cc_sensor->text().toStdString());
    }

    delete sampleUi;

}

void base::on_window_parameters_clicked()
{
    QDialog *parameters = new QDialog(0,0);

    Ui_Parameters *parametersUi = new Ui_Parameters;
    parametersUi->setupUi(parameters);

    parametersUi->mobility_els->setText(QString::number(config_tct->mu0_els()));
    parametersUi->mobility_holes->setText(QString::number(config_tct->mu0_holes()));
    parametersUi->sat_vel->setText(QString::number(config_tct->v_sat()));
    parametersUi->res_input->setText(QString::number(config_tct->R_sensor()));
    parametersUi->res_input_diode->setText(QString::number(config_tct->R_diode()));
    parametersUi->response_diode->setText(QString::number(config_tct->RespPhoto()));
    parametersUi->splitter->setText(QString::number(config_tct->light_split()));
    parametersUi->ampl->setText(QString::number(config_tct->ampl()));
    parametersUi->epair->setText(QString::number(config_tct->E_pair()));

    if(parameters->exec()) {
        config_tct->Setmu0_els(parametersUi->mobility_els->text().toFloat());
        config_tct->Setmu0_holes(parametersUi->mobility_holes->text().toFloat());
        config_tct->Setv_sat(parametersUi->sat_vel->text().toFloat());
        config_tct->SetR_sensor(parametersUi->res_input->text().toFloat());
        config_tct->SetR_diode(parametersUi->res_input_diode->text().toFloat());
        config_tct->SetRespPhoto(parametersUi->response_diode->text().toFloat());
        config_tct->Setlight_split(parametersUi->splitter->text().toFloat());
        config_tct->Setampl(parametersUi->ampl->text().toFloat());
        config_tct->SetE_pair(parametersUi->epair->text().toFloat());
    }

    delete parametersUi;
    delete parameters;

}


void base::on_start_clicked()
{
    this->kill_tbrowser();
    if(ui->modes->currentIndex() == 0) start_osc();
    if(ui->modes->currentIndex() == 1) start_tct();
}

void base::start_tct() {
    tovariables_config();
    QStringList names;

    if(ui->tct_single->isChecked()) {

        QFileDialog dialog(this);
        dialog.setNameFilter(tr("TCT Files (*.tct)"));
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setDirectory(QString(config_tct->DataFolder().c_str()));
        dialog.setModal(true);
        if(dialog.exec()) {
            names = dialog.selectedFiles();
        }
        else {
             ui->statusBar->showMessage(tr("No files selected"));
             return;
        }
    }
    else {
        if(config_tct->DataFolder() == "def") {
            error("No data folder was specified in analysis card. Check your analysis card, that \"DataFolder = ...\" is specified correctly");
            return;
        }

        QDir dirp(config_tct->DataFolder().c_str());
        if(!dirp.exists()) {
            error("Data Folder not found. Check \"DataFolder\" in analysis card.");
            return;
        }
        ui->statusBar->showMessage(tr("Searching for Data in Folder"));
        QStringList filters;
        filters << "*.tct";
        QStringList filenames = dirp.entryList(filters);
        for(int i=0;i<filenames.length();i++) names<<dirp.absoluteFilePath(filenames.at(i));
    }
    if(names.length()==0) {
        error("No files in specified folder!");
        return;
    }

    ui->statusBar->showMessage(QString("Number of files selected: %1").arg(names.length()),500);
    int nOps = 0;
    if(config_tct->FSeparateWaveforms()) nOps++;
    for(int i=0;i<config_tct->GetNumberOfModules();i++) {
        if(config_tct->GetModule(i)->isEnabled() && config_tct->TCT_Mode()==(int)config_tct->GetModule(i)->GetType()) nOps++;
    }

    progress = new Ui::ConsoleOutput(names.length()*nOps,this);
    progress->setValue(0);
    connect(progress,SIGNAL(OpenTBrowser()),this,SLOT(on_tbrowser_clicked()));

    // begin redirecting output
    std::ofstream log_file("execution.log",std::fstream::app);
    QDebugStream *debug = new QDebugStream(std::cout, log_file, progress->Console());
    print_run(true);

    for(int i=0;i<names.length();i++) {
        if(progress->wasCanceled()) break;
        ui->statusBar->showMessage(QString("Reading file %1").arg(names.at(i).split("/").last()),1000);
        char pathandfile[250];
        strcpy(pathandfile,names.at(i).toStdString().c_str());
        TCT::Scanning daq_data;
        bool read = daq_data.ReadTCT(pathandfile,config_tct,progress);
        if(!read) {
            ui->statusBar->showMessage(QString("Processing of file %1 failed. Skipping").arg(names.at(i).split("/").last()));
            continue;
        }
    }
    progress->setValue(names.length()*nOps);
    progress->finished1(names.length()*nOps);
    print_run(false);
    delete debug;
    connect(progress,SIGNAL(canceled()),this,SLOT(deleteprogress()));
    log_file.close();
    // end redirecting output

    ui->statusBar->showMessage(tr("Finished"));
}

void base::start_osc()
{
    tovariables_config();

    if(config_analysis->DataFolder() == "def") {
        error("No data folder was specified in analysis card. Check your analysis card, that \"DataFolder = ...\" is specified correctly");
        return;
    }

    progress_osc = new Ui::ConsoleOsc(this);
    connect(progress_osc,SIGNAL(OpenTBrowser()),this,SLOT(on_tbrowser_clicked()));
    progress_osc->show();
    // begin redirecting output
    std::ofstream log_file("execution.log",std::fstream::app);
    QDebugStream *debug = new QDebugStream(std::cout, log_file, progress_osc->Console());
    print_run(true);

    ui->statusBar->showMessage(tr("Searching data in folders"));

    std::cout << "The sample's data folder is = " << config_analysis->DataFolder() << ", searching data in subfolder(s) " <<  std::endl;

    QString datafolder(config_analysis->DataFolder().c_str());
    QDir datadir(datafolder);
    QStringList filter;
    filter<<"*.txt";
    if (!datadir.exists()) {
        error("Data Folder not found. Check \"DataFolder\" in analysis card.");
        return;
    }
    int countersubdir = 0;
    QDirIterator subfolders(datafolder,QDir::Dirs | QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(subfolders.hasNext()){
        QString dirname = subfolders.next();
        QDir subdir(dirname);
        dirname+="/";
        if(subdir.entryList(filter).length()) {
            std::vector<TCT::acquisition_single> AllAcqs;
            TCT::measurement meas(dirname.toStdString());
            if(!meas.AcqsLoader(&AllAcqs, config_analysis->MaxAcqs(),config_analysis->LeCroyRAW())) {
                std::cout << " Folder empty! Skipping folder" << std::endl;
                continue;
            };
            // now create instance of avg acquisition using Nsamples from loaded files
            TCT::acquisition_avg AcqAvg(AllAcqs[0].Nsamples());
            AcqAvg.SetPolarity(AllAcqs[0].Polarity());

            //now analyse all acquisitions
            int Nselected = 0;

            AcqAvg.SetNanalysed(AllAcqs.size());
            for(uint32_t i_acq = 0; i_acq < AllAcqs.size(); i_acq++){

                TCT::acquisition_single* acq = &AllAcqs[i_acq];
                if(config_analysis->DoSmearing()) config_analysis->AcqsSmearer(acq, config_analysis->AddNoise(), false);
                config_analysis->AcqsAnalyser(acq, i_acq, &AcqAvg);
                if(config_analysis->DoSmearing()) config_analysis->AcqsSmearer(acq, false, config_analysis->AddJitter()); // AcqsAnalyser removes jitter by determining each acqs delay. Hence, to add jitter, delay has to be manipulated after AcqsAnalyser (and before filling of profile

                if( config_analysis->AcqsSelecter(acq) ) {
                    Nselected++;
                    acq->SetSelect(true);
                }
                AcqAvg.SetNselected(Nselected);
                config_analysis->AcqsProfileFiller(acq, &AcqAvg);

            }

            config_analysis->SetOutSample_ID(config_sample->SampleID());
                config_analysis->SetOutSubFolder(dirname.split(datafolder).last().split("/").at(1).toStdString());
                config_analysis->SetOutSubsubFolder(dirname.split(datafolder).last().split("/").at(2).toStdString());


            if(config_analysis->SaveToFile())
                config_analysis->AcqsWriter(&AllAcqs, &AcqAvg, true);
                //else config_analysis->AcqsWriter(&AllAcqs, &AcqAvg, false);

            std::cout << "   Nselected = " << Nselected << std::endl;
            std::cout << "   ratio of selected acqs = " << Nselected << " / " << AllAcqs.size() << " = " << (float)Nselected/AllAcqs.size()*100. << "%\n\n" << std::endl;

            // now take care of memory management
            // delete remaning TH1Fs in acquisition_single and then clear AllAcqs
            for(int j = 0; j < AllAcqs.size(); j++) {
                AllAcqs[j].Clear();
            }
            AllAcqs.clear();

            countersubdir++;
        }
    }

    progress_osc->SetButtonEnabled();
    print_run(false);
    delete debug;
    connect(progress_osc,SIGNAL(rejected()),this,SLOT(deleteprogress_osc()));
    log_file.close();
    // end redirecting output

    ui->statusBar->showMessage(tr("Finished"));
}

void base::deleteprogress() {
    delete progress;
    progress = NULL;
}
void base::deleteprogress_osc() {
    delete progress_osc;
    progress_osc = NULL;
}

void base::on_actionChange_config_triggered()
{

    QString cpart = "";
    cpart = QFileDialog::getOpenFileName(this,
                                              tr("Open Config File"), "../testanalysis", tr("Text Files (*.txt)"));
    if(cpart=="") return;
    else {
        read_config(cpart.toStdString().c_str());
        fill_config();
    }

}

void base::on_actionSave_config_triggered()
{
    tovariables_config();

    QString cpart = "";
    cpart = QFileDialog::getSaveFileName(this,
                                              tr("Save Config File"), "../testanalysis", tr("Text Files (*.txt)"));
    if(cpart=="") return;

    std::ofstream conf_file(cpart.toStdString().c_str(),std::fstream::trunc);

    if(!conf_file.is_open()) {
        error("Can not open the text file to write config. Aborting operation.");
        return;
    }

    conf_file<<"#comments have to start with a\n";
    conf_file<<"#put group key words in []\n";
    conf_file<<"#always specify ID, TAB, \"=\", TAB, value";

    conf_file<<"\n\n[General]";
    conf_file<<"\nProjectFolder\t=\t./"; //FIXME
    conf_file<<"\nDataFolder\t=\t"<<config_tct->DataFolder();
    conf_file<<"\nOutfolder\t=\t"<<config_tct->OutFolder();
    conf_file<<"\n\n\
#Set acq mode.\n\
# 0 - taking the sets of single measurements (*.txt or *.raw files by oscilloscope). Settings are in [Analysis]\n\
# 1 - taking the data from *.tct file produced by DAQ software. Settings are in [Scanning]";
    conf_file<<"\nMode\t=\t"<<config_mode->Mode();

    conf_file<<"\n\n[Analysis]";
    conf_file<<"\nMaxAcqs\t=\t"<<config_analysis->MaxAcqs();
    conf_file<<"\nNoise_Cut\t=\t"<<config_analysis->Noise_Cut();
    conf_file<<"\nNoiseEnd_Cut\t=\t"<<config_analysis->NoiseEnd_Cut();
    conf_file<<"\nS2n_Cut\t=\t"<<config_analysis->S2n_Cut();
    conf_file<<"\nS2n_Ref\t=\t"<<config_analysis->S2n_Ref();
    conf_file<<"\nAmplNegLate_Cut\t=\t"<<config_analysis->AmplNegLate_Cut();
    conf_file<<"\nAmplPosLate_Cut\t=\t"<<config_analysis->AmplPosLate_Cut();
    conf_file<<"\nAmplNegEarly_Cut\t=\t"<<config_analysis->AmplNegEarly_Cut();
    conf_file<<"\nAmplPosEarly_Cut\t=\t"<<config_analysis->AmplPosEarly_Cut();
    conf_file<<"\nDoSmearing\t=\t"<<config_analysis->DoSmearing();
    conf_file<<"\nAddNoise\t=\t"<<config_analysis->AddNoise();
    conf_file<<"\nAddJitter\t=\t"<<config_analysis->AddJitter();
    conf_file<<"\nSaveToFile\t=\t"<<config_analysis->SaveToFile();
    conf_file<<"\nSaveSingles\t=\t"<<config_analysis->SaveSingles();
    conf_file<<"\nPrintEvent\t=\t"<<config_analysis->PrintEvent();
    conf_file<<"\nLeCroyRAW\t=\t"<<config_analysis->LeCroyRAW();

    conf_file<<"\n\n[Scanning]";
    conf_file<<"\n#Channels of oscilloscope connected to detector, photodiode, trigger. Put numbers 1,2,3,4 - corresponding to channels, no such device connected put 0.";
    conf_file<<"\nCH1_Detector\t=\t"<<config_tct->CH1_Det();
    conf_file<<"\nCH2_Detector\t=\t"<<config_tct->CH2_Det();
    conf_file<<"\n#Turning on of the Photodiode channel also adds normalisation to all scans";
    conf_file<<"\nCH_Photodiode\t=\t"<<config_tct->CH_PhDiode();
    conf_file<<"\nCH_Trigger\t=\t"<<config_tct->CH_Trig();
    conf_file<<"\n#Set optical Axis. 1-x,2-y,3-z";
    conf_file<<"\nOptical_Axis\t=\t"<<config_tct->OptAxis();
    conf_file<<"\n#Set scanning Axis. 1-x,2-y,3-z";
    conf_file<<"\nScanning_Axis\t=\t"<<config_tct->ScAxis();
    conf_file<<"\n#Set voltage source number (1 or 2)";
    conf_file<<"\nVoltage_Source\t=\t"<<config_tct->VoltSource();
    conf_file<<"\n#Time between stage movements in seconds.";
    conf_file<<"\nMovements_dt\t=\t"<<config_tct->Movements_dt();
    conf_file<<"\n#Set the integration time in ns to correct the bias line. Program averages the signal in range (0,value) and then shifts the signal by the mean value.";
    conf_file<<"\nCorrectBias\t=\t"<<config_tct->CorrectBias();

    conf_file<<"\n#Perform next operations. Analysis will start only if all needed data is present:";
    conf_file<<"\n# 0-top,1-edge,2-bottom";
    conf_file<<"\nTCT_Mode\t=\t"<<config_tct->TCT_Mode();


    bool focus_top = false;
    bool focus_edge = false;
    bool sensor_search_top = false;
    bool sensor_search_edge = false;
    for(int i=0; i<config_tct->GetNumberOfModules(); i++) {
        TCT::TCTModule* module = config_tct->GetModule(i);
        if(strcmp(module->GetName(),"Focus_Search")==0) {
            if((int)module->GetType()==0) focus_top = module->isEnabled();
            if((int)module->GetType()==1) focus_edge = module->isEnabled();
        }
        if(strcmp(module->GetName(),"Sensor_Search")==0) {
            if((int)module->GetType()==0) sensor_search_top = module->isEnabled();
            if((int)module->GetType()==1) sensor_search_edge = module->isEnabled();
        }      
    }

    conf_file<<"\n\n#Scanning over X and Y to find position of the sensor.";
    conf_file<<"\n"<<"Sensor_Search_Top"<<"\t=\t"<< sensor_search_top ;
    conf_file<<"\n"<<"Sensor_Search_Edge"<<"\t=\t"<< sensor_search_edge ;

    conf_file<<"\n\n#Scanning over optical and perpendiculr to strip axes (or along the detector depth in case of edge-tct), fitting the best position.";
    conf_file<<"\n"<<"Focus_Search_Top"<<"\t=\t"<<focus_top;
    conf_file<<"\n"<<"Focus_Search_Edge"<<"\t=\t"<< focus_edge;

    for(int i=0; i<config_tct->GetNumberOfModules(); i++) {
        TCT::TCTModule* module = config_tct->GetModule(i);
        if(strcmp(module->GetName(),"Focus_Search")!=0 && strcmp(module->GetName(),"Sensor_Search")!=0) {
            conf_file<<"\n#"<<module->GetTitle();
            conf_file<<"\n"<<module->GetName()<<"\t=\t"<<module->isEnabled();
            module->PrintConfig(conf_file);
        }
    }

    conf_file<<"\n\n#Integrate sensor signal for channel 1 from TimeSensorLow to TimeSensorHigh - ns";
    conf_file<<"\nTimeSensorLow1\t=\t"<<config_tct->FTlowCH1();
    conf_file<<"\nTimeSensorHigh1\t=\t"<<config_tct->FThighCH1();
    conf_file<<"\n\n#Integrate sensor signal for channel 2 from TimeSensorLow to TimeSensorHigh - ns";
    conf_file<<"\nTimeSensorLow2\t=\t"<<config_tct->FTlowCH2();
    conf_file<<"\nTimeSensorHigh2\t=\t"<<config_tct->FThighCH2();
    conf_file<<"\n#Integrate photodiode signal from TimeDiodeLow to TimeDiodeHigh - ns";
    conf_file<<"\nTimeDiodeLow\t=\t"<<config_tct->FDLow();
    conf_file<<"\nTimeDiodeHigh\t=\t"<<config_tct->FDHigh();
    conf_file<<"\n\n#Save charge, normed charge and photodiode charge for each Z, voltage";
    conf_file<<"\nSaveSeparateCharges\t=\t"<<config_tct->FSeparateCharges();
    conf_file<<"\n#Save waveforms for each position and voltage";
    conf_file<<"\nSaveSeparateWaveforms\t=\t"<<config_tct->FSeparateWaveforms();
    //conf_file<<"\n#Averaging the current for electric field profile from F_TLow to F_TLow+EV_Time";
    //conf_file<<"\nEV_Time\t=\t"<<config_tct->EV_Time();


    conf_file<<"\n\n[Parameters]";
    conf_file<<"\n#low-field mobility for electrons, cm2*V^-1*s^-1";
    conf_file<<"\nMu0_Electrons\t=\t"<<config_tct->mu0_els();
    conf_file<<"\n#low-field mobility for holes, cm2*V^-1*s^-1";
    conf_file<<"\nMu0_Holes\t=\t"<<config_tct->mu0_holes();
    conf_file<<"\n#saturation velocity cm/s";
    conf_file<<"\nSaturationVelocity\t=\t"<<config_tct->v_sat();
    conf_file<<"\n# amplifier amplification";
    conf_file<<"\nAmplification\t=\t"<<config_tct->ampl();
    conf_file<<"\n# factor between charge in sensor and photodiode due to light splitting: Nsensor/Ndiode";
    conf_file<<"\nLightSplitter\t=\t"<<config_tct->light_split();
    conf_file<<"\n# resistance of the sensor and diode output, Ohm";
    conf_file<<"\nResistanceSensor\t=\t"<<config_tct->R_sensor();
    conf_file<<"\nResistancePhotoDetector\t=\t"<<config_tct->R_diode();
    conf_file<<"\n# pohotodetector responce for certain wavelength, A/W";
    conf_file<<"\nResponcePhotoDetector\t=\t"<<config_tct->RespPhoto();
    conf_file<<"\n# electron-hole pair creation energy, eV";
    conf_file<<"\nEnergyPair\t=\t"<<config_tct->E_pair();

    conf_file<<"\n\n[Sensor]";
    conf_file<<"\nSampleCard\t=\t"<<config_mode->SampleCard();

    conf_file.close();

    ui->statusBar->showMessage(tr("Config File Saved"), 2000);
}

void base::error(const char *text) {
    QMessageBox *messageBox = new QMessageBox(this);
    messageBox->critical(0,"Error",text);
    messageBox->setFixedSize(500,200);
    delete messageBox;
}
void base::print_run(bool start) {

    QDateTime datetime;
    if(start) {
        std::cout<<"<-------------- NEW RUN -------------->"<<std::endl;
        std::cout<<"Run Started At: "<<datetime.currentDateTime().toString().toStdString()<<std::endl;
    }
    else std::cout<<"Run Finished At: "<<datetime.currentDateTime().toString().toStdString()<<std::endl;

}

void base::on_tbrowser_clicked()
{
    if(browserProcess != NULL && browserProcess->isOpen()) browserProcess->kill();
    QString program = QDir::currentPath()+"/tbrowser";
    browserProcess = new QProcess(this);
    browserProcess->setWorkingDirectory(config_tct->OutFolder().c_str());
    browserProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    browserProcess->start(program);
}
void base::kill_tbrowser() {
    if(browserProcess != NULL && browserProcess->isOpen()) browserProcess->kill();
}
void base::FillBoxes() {

    for(int i=0;i<config_tct->GetNumberOfModules();i++) {
        TCT::TCT_Type type = config_tct->GetModule(i)->GetType();
        if(type == TCT::_Top) top_widgets[new QCheckBox(config_tct->GetModule(i)->GetTitle())] = i;
        if(type == TCT::_Edge) edge_widgets[new QCheckBox(config_tct->GetModule(i)->GetTitle())] = i;
        if(type == TCT::_Bottom) bottom_widgets[new QCheckBox(config_tct->GetModule(i)->GetTitle())] = i;
    }

    QVBoxLayout *top_layout = new QVBoxLayout;
    for(auto i: top_widgets) {
        ((QWidget*)i.first)->setMaximumHeight(20);
        top_layout->addWidget(i.first);
        config_tct->GetModule(i.second)->AddParameters(top_layout);
    }
    top_layout->addStretch(1);
    ui->group_top->setLayout(top_layout);

    QVBoxLayout *edge_layout = new QVBoxLayout;
    for(auto i: edge_widgets) {
        ((QWidget*)i.first)->setMaximumHeight(20);
        edge_layout->addWidget(i.first);
        config_tct->GetModule(i.second)->AddParameters(edge_layout);
    }
    edge_layout->addStretch(1);
    ui->group_edge->setLayout(edge_layout);

    QVBoxLayout *bottom_layout = new QVBoxLayout;
    for(auto i: bottom_widgets) {
        ((QWidget*)i.first)->setMaximumHeight(20);
        bottom_layout->addWidget(i.first);
        config_tct->GetModule(i.second)->AddParameters(bottom_layout);
    }
    bottom_layout->addStretch(1);
    ui->group_bottom->setLayout(bottom_layout);

}

void base::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About"),
            tr("<h2>TCT Data Analysis Framework. Graphical Version.</h2>"
               "<p><img src=\"../icons/arrow.png\" width=\"15\"/> <b>Mykyta Haranko</b>, 2015-2016"
               "<p><img src=\"../icons/arrow.png\" width=\"15\"/> Oscilloscope Data analysis by <b>Hendrik Jansen</b>"
               "<p><img src=\"../icons/arrow.png\" width=\"15\"/> TCT Data files readout system by <b>Particulars</b>"
               "<p><center><img src=\"../icons/knu.png\" width=\"87\"/> <img src=\"../icons/desy.png\" width=\"87\"/> <img src=\"../icons/lpnhe.png\" height=\"87\"/> <img src=\"../icons/particulars.png\" width=\"50\"/></center>"
               "<p><b>Compiled on: </b>%1, %2").arg(QString(__DATE__)).arg(QString(__TIME__)));
}

void base::on_actionFile_Info_triggered()
{
    QStringList names;

    QFileDialog dialog(this);
    dialog.setNameFilter(tr("TCT Files (*.tct)"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(QString(config_tct->DataFolder().c_str()));
    dialog.setModal(true);
    if(dialog.exec()) {
        names = dialog.selectedFiles();
    }
    else {
        ui->statusBar->showMessage(tr("No files selected"));
        return;
    }
    char pathandfile[250];
    strcpy(pathandfile,names.at(0).toStdString().c_str());
    TCTReader *file = new TCTReader(pathandfile,-3,2);
    QMessageBox::about(this, tr("TCT File Info"),
            tr("<h2>File Info</h2>"
               "<b>Name:</b> %1"
               "<p>%2").arg(names.at(0)).arg(file->StringInfo().c_str()));
    delete file;
}
