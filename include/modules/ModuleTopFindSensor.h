/**
 * \file
 * \brief Definition of the TCT::ModuleTopFindSensor class.
 */

#ifndef __MODULETOPFindSensor_H__
#define __MODULETOPFindSensor_H__ 1

#include "TCTModule.h"

namespace TCT {

  class ModuleTopFindSensor: public TCTModule {

   public:
        ModuleTopFindSensor(tct_config* config1, const char* name, TCT_Type type, const char* title):
            TCTModule(config1, name, type, title) {}
        bool CheckModuleData();
        bool Analysis();

    };
}
#endif
