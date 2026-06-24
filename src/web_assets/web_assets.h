#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

/**
 * @file web_assets.h
 * @brief Umbrella include for all PROGMEM web UI assets served by the HTTP handler.
 *
 * Layout:
 *   pages/   – HTML views (symbols: *_html)
 *   styles/  – CSS (app_css)
 *   scripts/ – JavaScript (app_js)
 *   vendor/  – Optional third-party bundles (currently unused at runtime)
 *   icons/   – Favicon and similar static images
 */

#include "web_assets/pages/index_page.h"
#include "web_assets/pages/mp3play_page.h"
#include "web_assets/pages/sound_page.h"
#include "web_assets/pages/config_page.h"
#include "web_assets/pages/scheduler_page.h"
#include "web_assets/pages/display_page.h"
#include "web_assets/pages/ota_page.h"
#include "web_assets/pages/maintenance_page.h"
#include "web_assets/pages/firmware_page.h"
#include "web_assets/pages/about_page.h"
#include "web_assets/pages/diagnostics_page.h"
#include "web_assets/styles/app.css.h"
#include "web_assets/scripts/app.js.h"
/* #include "web_assets/icons/favicon.ico.h" */

#endif
