#include "hid.h"
#include "views.h"
#include <notification/notification_messages.h>
#include <dolphin/dolphin.h>

#define TAG "CookieClickerApp"

enum HidDebugSubmenuIndex {
    HidSubmenuIndexInstructions,
    HidSubmenuIndexClicker,
    HidSubmenuIndexCredits,
};

static void hid_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    Hid* app = context;
    if(index == HidSubmenuIndexInstructions) {
        app->view_id = BtHidViewInstructions;
        view_dispatcher_switch_to_view(app->view_dispatcher, app->view_id);
    } else if(index == HidSubmenuIndexClicker) {
        app->view_id = BtHidViewClicker;
        view_dispatcher_switch_to_view(app->view_dispatcher, app->view_id);
    } else if(index == HidSubmenuIndexCredits) {
        app->view_id = BtHidViewCredits;
        view_dispatcher_switch_to_view(app->view_dispatcher, app->view_id);
    }
}

static void bt_hid_connection_status_changed_callback(BtStatus status, void* context) {
    furi_assert(context);
    Hid* hid = context;
    bool connected = (status == BtStatusConnected);
    if(connected) {
        notification_internal_message(hid->notifications, &sequence_set_blue_255);
    } else {
        notification_internal_message(hid->notifications, &sequence_reset_blue);
    }
    hid_cc_set_connected_status(hid->hid_cc, connected);
}

static uint32_t hid_submenu_view(void* context) {
    UNUSED(context);
    return HidViewSubmenu;
}

static uint32_t hid_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

Hid* hid_alloc() {
    Hid* app = malloc(sizeof(Hid));

    // Gui
    app->gui = furi_record_open(RECORD_GUI);

    // Bt
    app->bt = furi_record_open(RECORD_BT);

    // Notifications
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Submenu view
    app->submenu = submenu_alloc();
    submenu_set_header(app->submenu, "Cookie clicker");
    submenu_add_item(
        app->submenu, "Instructions", HidSubmenuIndexInstructions, hid_submenu_callback, app);
    submenu_add_item(
        app->submenu, "BT Phone Clicker", HidSubmenuIndexClicker, hid_submenu_callback, app);
    submenu_add_item(app->submenu, "Credits", HidSubmenuIndexCredits, hid_submenu_callback, app);
    view_set_previous_callback(submenu_get_view(app->submenu), hid_exit);
    view_dispatcher_add_view(app->view_dispatcher, HidViewSubmenu, submenu_get_view(app->submenu));
    app->view_id = HidViewSubmenu;
    view_dispatcher_switch_to_view(app->view_dispatcher, app->view_id);
    return app;
}

Hid* hid_app_alloc_view(void* context) {
    furi_assert(context);
    Hid* app = context;

    // Instructions view
    app->widget_instructions = widget_alloc();
    widget_add_text_scroll_element(
        app->widget_instructions,
        0,
        0,
        128,
        64,
        "While this app is running, go to\n"
        "your phone's bluetooth\n"
        "settings and discover devices.\n"
        "You should see 'Control\n"
        "<flipper>' in the list. Click pair\n"
        "on your phone. On the Flipper\n"
        "Zero you should see a pin #.\n"
        "If the pin matches, click OK on\n"
        "your phone and the Flipper.\n"
        "You should now be connected!\n"
        "Launch the Cookie clicker app\n"
        "on your phone. Tap your\n"
        "phone's screen to click on the\n"
        "first cookie, then on the\n"
        "Flipper Zero, choose\n"
        "'Phone clicker'. Click the\n"
        "UP/DOWN buttons to set the\n"
        "speed. Click the OK button on\n"
        "the Flipper to enable/\n"
        "disable the clicker.\n"
        "Enjoy!\n");
    view_set_previous_callback(widget_get_view(app->widget_instructions), hid_submenu_view);
    view_dispatcher_add_view(
        app->view_dispatcher, BtHidViewInstructions, widget_get_view(app->widget_instructions));

    // Clicker view
    app->hid_cc = hid_cc_alloc(app);
    view_set_previous_callback(hid_cc_get_view(app->hid_cc), hid_submenu_view);
    view_dispatcher_add_view(app->view_dispatcher, BtHidViewClicker, hid_cc_get_view(app->hid_cc));

    // Credits view
    app->widget_credits = widget_alloc();
    widget_add_text_scroll_element(
        app->widget_credits,
        0,
        0,
        128,
        64,
        "Original app was from\n"
        "external/hid_app. \n\n"
        "Modified by CodeAllNight.\n"
        "https://youtube.com/\n"
        "@MrDerekJamison\n\n"
        "Thanks to all the\n"
        "contributors of the original\n"
        "app!\n\n"
        "Flip the World!\n");
    view_set_previous_callback(widget_get_view(app->widget_credits), hid_submenu_view);
    view_dispatcher_add_view(
        app->view_dispatcher, BtHidViewCredits, widget_get_view(app->widget_credits));

    return app;
}

void hid_free(Hid* app) {
    furi_assert(app);

    // Reset notification
    notification_internal_message(app->notifications, &sequence_reset_blue);

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, BtHidViewClicker);
    hid_cc_free(app->hid_cc);
    view_dispatcher_remove_view(app->view_dispatcher, BtHidViewCredits);
    widget_free(app->widget_credits);
    view_dispatcher_remove_view(app->view_dispatcher, BtHidViewInstructions);
    widget_free(app->widget_instructions);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);

    // Close records
    furi_record_close(RECORD_GUI);
    app->gui = NULL;
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;
    furi_record_close(RECORD_BT);
    app->bt = NULL;

    // Free rest
    free(app);
}

void hid_hal_mouse_move(Hid* instance, int8_t dx, int8_t dy) {
    furi_assert(instance);
    furi_hal_bt_hid_mouse_move(dx, dy);
}

void hid_hal_mouse_press(Hid* instance, uint16_t event) {
    furi_assert(instance);
    furi_hal_bt_hid_mouse_press(event);
}

void hid_hal_mouse_release(Hid* instance, uint16_t event) {
    furi_assert(instance);
    furi_hal_bt_hid_mouse_release(event);
}

void hid_hal_mouse_release_all(Hid* instance) {
    furi_assert(instance);
    furi_hal_bt_hid_mouse_release_all();
}

int32_t hid_cookie_ble_app(void* p) {
    UNUSED(p);
    Hid* app = hid_alloc();
    app = hid_app_alloc_view(app);

    bt_disconnect(app->bt);

    // Wait 2nd core to update nvm storage
    furi_delay_ms(200);

    // Migrate data from old sd-card folder
    Storage* storage = furi_record_open(RECORD_STORAGE);

    storage_common_migrate(
        storage,
        EXT_PATH("apps/Tools/" HID_BT_KEYS_STORAGE_NAME),
        APP_DATA_PATH(HID_BT_KEYS_STORAGE_NAME));

    bt_keys_storage_set_storage_path(app->bt, APP_DATA_PATH(HID_BT_KEYS_STORAGE_NAME));

    furi_record_close(RECORD_STORAGE);

    if(!bt_set_profile(app->bt, BtProfileHidKeyboard)) {
        FURI_LOG_E(TAG, "Failed to switch to HID profile");
    }

    furi_hal_bt_start_advertising();
    bt_set_status_changed_callback(app->bt, bt_hid_connection_status_changed_callback, app);

    DOLPHIN_DEED(DolphinDeedPluginStart);

    view_dispatcher_run(app->view_dispatcher);

    bt_set_status_changed_callback(app->bt, NULL, NULL);

    bt_disconnect(app->bt);

    // Wait 2nd core to update nvm storage
    furi_delay_ms(200);

    bt_keys_storage_set_default_path(app->bt);

    if(!bt_set_profile(app->bt, BtProfileSerial)) {
        FURI_LOG_E(TAG, "Failed to switch to Serial profile");
    }

    hid_free(app);

    return 0;
}
