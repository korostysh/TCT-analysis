#ifndef BASE_H
#define BASE_H

#include <QMainWindow>
#include "stdint.h"
#include "util.h"
#include "tct_config.h"
#include "sample.h"
#include "analysis.h"
#include "gui_consoleoutput.h"
#include "QProcess"

namespace Ui {
class base;
}

class base : public QMainWindow
{
    Q_OBJECT

public:
    explicit base(QWidget *parent = 0);
    ~base();

private slots:

    void on_buttonGroup_mode_buttonClicked(int index);

    void on_window_folders_clicked();

    void on_window_sample_clicked();

    void on_window_parameters_clicked();

    void on_start_clicked();

    void on_actionChange_config_triggered();

    void on_actionSave_config_triggered();

    void deleteprogress();

    void deleteprogress_osc();

    void on_tbrowser_clicked();

    void kill_tbrowser();

    void on_actionAbout_triggered();

private:
    Ui::base *ui;
    std::string _DefConfigName = "def";
    TCT::tct_config *config_tct = NULL;
    TCT::analysis *config_analysis = NULL;
    TCT::sample *config_sample = NULL;
    TCT::mode_selector *config_mode = NULL;
    Ui::ConsoleOutput *progress = NULL;
    Ui::ConsoleOsc *progress_osc = NULL;
    QProcess *browserProcess = NULL;
    void read_config(const char*);
    void fill_config();
    void tovariables_config();
    void error(const char*);
    void print_run(bool start);
    void start_tct();
    void start_osc();

};

#endif // BASE_H
