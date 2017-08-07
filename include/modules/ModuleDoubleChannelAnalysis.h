/**
 * \file
 * \brief Definition of the TCT::ModuleTopFocus class.
 */

#ifndef __MODULEDOUBLEChannelAnalysis_H__
#define __MODULEDOUBLEChannelAnalysis_H__ 1

#include "TCTModule.h"

namespace TCT {

  class ModuleDoubleChannelAnalysis: public TCTModule {

   public:
        ModuleDoubleChannelAnalysis(tct_config* config1, const char* name, TCT_Type type, const char* title):
            TCTModule(config1, name, type, title) {}
        bool CheckModuleData();
        bool Analysis();

    };
}
#endif 
