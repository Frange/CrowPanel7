#include "storage_service.h"
#include <Preferences.h>

static Preferences preferences;

void storage_init() {
    preferences.begin("crow_app", false);
}