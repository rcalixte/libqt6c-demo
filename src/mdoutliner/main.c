#include <libqt6c.h>
#include "resources.h"
#include <stdbool.h>

#define LINE_NUMBER_ROLE (QT_ITEMDATAROLE_USERROLE + 1)
#define INITIAL_MAP_CAPACITY 32
#define MAX_LINE_LENGTH 4096

struct AppTab;
struct AppWindow;
typedef struct AppTab AppTab;
typedef struct AppWindow AppWindow;

struct AppTab {
    QWidget* tab;
    QListWidget* outline;
    QTextEdit* textArea;
};

struct AppWindow {
    QMainWindow* w;
    QTabWidget* tabs;
};

static AppWindow* main_window = NULL;

static QMenu* file_menu = NULL;
static QAction* new_action = NULL;
static QAction* open_action = NULL;
static QAction* exit_action = NULL;

static QMenu* options_menu = NULL;
static QMenu* language_menu = NULL;
static QAction* english_action = NULL;
static QAction* french_action = NULL;
static QAction* spanish_action = NULL;

static QMenu* help_menu = NULL;
static QAction* about_action = NULL;

static struct {
    void** keys;
    AppTab** values;
    size_t capacity;
    size_t size;
} app_tab_map = {0};

static void map_init(size_t initial_capacity) {
    app_tab_map.keys = (void**)calloc(initial_capacity, sizeof(void*));
    app_tab_map.values = (AppTab**)calloc(initial_capacity, sizeof(AppTab*));
    app_tab_map.capacity = initial_capacity;
    app_tab_map.size = 0;
}

static void map_put(void* key, AppTab* value) {
    if (app_tab_map.size >= app_tab_map.capacity) {
        size_t new_capacity = app_tab_map.capacity * 2;
        app_tab_map.keys = (void**)realloc(app_tab_map.keys, new_capacity * sizeof(void*));
        app_tab_map.values = (AppTab**)realloc(app_tab_map.values, new_capacity * sizeof(AppTab*));
        app_tab_map.capacity = new_capacity;
    }
    app_tab_map.keys[app_tab_map.size] = key;
    app_tab_map.values[app_tab_map.size] = value;
    app_tab_map.size++;
}

static AppTab* map_get(void* key) {
    for (size_t i = 0; i < app_tab_map.size; i++) {
        if (app_tab_map.keys[i] == key) {
            return app_tab_map.values[i];
        }
    }
    return NULL;
}

static void map_remove(void* key) {
    for (size_t i = 0; i < app_tab_map.size; i++) {
        if (app_tab_map.keys[i] == key) {
            app_tab_map.keys[i] = app_tab_map.keys[app_tab_map.size - 1];
            app_tab_map.values[i] = app_tab_map.values[app_tab_map.size - 1];
            app_tab_map.size--;
            return;
        }
    }
}

static void map_cleanup() {
    free(app_tab_map.keys);
    free(app_tab_map.values);
    app_tab_map.keys = NULL;
    app_tab_map.values = NULL;
    app_tab_map.size = 0;
    app_tab_map.capacity = 0;
}

void on_triggered(void* self) {
    const char* language = q_action_object_name(self);
    QLocale* locale = q_locale_new2(language);
    QTranslator* translator = q_translator_new();

    if (q_translator_load42(translator, locale, "mdoutliner", "_", ":/translations")) {
        q_application_install_translator(translator);
    } else {
        fprintf(stderr, "Failed to load translation for %s\n", language);
        q_translator_delete(translator);
        q_locale_delete(locale);
        libqt_free(language);
        return;
    }

    const char* file_text = q_application_translate("Main", "&File");
    q_menu_set_title(file_menu, file_text);
    libqt_free(file_text);

    const char* new_text = q_application_translate("Main", "&New Tab");
    q_action_set_text(new_action, new_text);
    libqt_free(new_text);

    const char* new_bind = q_application_translate3("Main", "Ctrl+N", "New tab");
    QKeySequence* new_key = q_keysequence_new2(new_bind);
    q_action_set_shortcut(new_action, new_key);
    q_keysequence_delete(new_key);
    libqt_free(new_bind);

    const char* open_text = q_application_translate("Main", "&Open...");
    q_action_set_text(open_action, open_text);
    libqt_free(open_text);

    const char* open_bind = q_application_translate3("Main", "Ctrl+O", "Open a file");
    QKeySequence* open_key = q_keysequence_new2(open_bind);
    q_action_set_shortcut(open_action, open_key);
    q_keysequence_delete(open_key);
    libqt_free(open_bind);

    const char* exit_text = q_application_translate("Main", "&Exit");
    q_action_set_text(exit_action, exit_text);
    libqt_free(exit_text);

    const char* exit_bind = q_application_translate3("Main", "Ctrl+Q", "Quit");
    QKeySequence* exit_key = q_keysequence_new2(exit_bind);
    q_action_set_shortcut(exit_action, exit_key);
    q_keysequence_delete(exit_key);
    libqt_free(exit_bind);

    const char* options_text = q_application_translate("Main", "&Options");
    q_menu_set_title(options_menu, options_text);
    libqt_free(options_text);

    const char* language_text = q_application_translate("Main", "&Language");
    q_menu_set_title(language_menu, language_text);
    libqt_free(language_text);

    const char* english_text = q_application_translate("Main", "&English");
    q_action_set_text(english_action, english_text);
    libqt_free(english_text);

    const char* french_text = q_application_translate("Main", "&French");
    q_action_set_text(french_action, french_text);
    libqt_free(french_text);

    const char* spanish_text = q_application_translate("Main", "&Spanish");
    q_action_set_text(spanish_action, spanish_text);
    libqt_free(spanish_text);

    const char* help_text = q_application_translate("Main", "&Help");
    q_menu_set_title(help_menu, help_text);
    libqt_free(help_text);

    const char* about_text = q_application_translate("Main", "&About Qt");
    q_action_set_text(about_action, about_text);
    libqt_free(about_text);

    q_translator_delete(translator);
    q_locale_delete(locale);
    libqt_free(language);
}

static void handle_jump_to_bookmark(void* self, void* UNUSED current, void* UNUSED previous) {
    AppTab* tab = map_get(self);
    if (!tab)
        return;

    QListWidgetItem* item = q_listwidget_current_item(self);

    QVariant* line_number_variant = q_listwidgetitem_data(item, LINE_NUMBER_ROLE);
    int line_number = q_variant_to_int(line_number_variant);
    q_variant_delete(line_number_variant);
    QTextDocument* doc = q_textedit_document(tab->textArea);

    QTextBlock* block = q_textdocument_find_block_by_line_number(doc, line_number);
    QTextCursor* cursor = q_textcursor_new4(block);

    q_textcursor_set_position(cursor, q_textblock_position(block));
    q_textedit_set_text_cursor(tab->textArea, cursor);
    q_textedit_set_focus(tab->textArea);
    q_textcursor_delete(cursor);
}

static void update_outline_for_content(AppTab* tab, const char* content) {
    q_listwidget_clear(tab->outline);

    char line[MAX_LINE_LENGTH];
    char prev_line[MAX_LINE_LENGTH];
    char tooltip[32];
    bool in_code_block = false;
    int line_number = 0;

    while (*content) {
        size_t i = 0;
        while (*content && *content != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *content++;
        }
        line[i] = '\0';
        if (*content == '\n')
            content++;

        if (!in_code_block) {
            if (line[0] == '#') {
                QListWidgetItem* bookmark = q_listwidgetitem_new7(line, tab->outline);
                snprintf(tooltip, sizeof(tooltip), "Line %d", line_number + 1);
                q_listwidgetitem_set_tool_tip(bookmark, tooltip);

                QVariant* line_num = q_variant_new4(line_number);
                q_listwidgetitem_set_data(bookmark, LINE_NUMBER_ROLE, line_num);
                q_variant_delete(line_num);
            } else if (((strncmp(line, "---", 3) == 0) || (strncmp(line, "===", 3) == 0)) && prev_line[0] != '\0') {
                QListWidgetItem* bookmark = q_listwidgetitem_new7(prev_line, tab->outline);
                snprintf(tooltip, sizeof(tooltip), "Line %d", line_number);
                q_listwidgetitem_set_tool_tip(bookmark, tooltip);

                QVariant* line_num = q_variant_new4(line_number - 1);
                q_listwidgetitem_set_data(bookmark, LINE_NUMBER_ROLE, line_num);
                q_variant_delete(line_num);
            }
        }

        if (strncmp(line, "```", 3) == 0)
            in_code_block = !in_code_block;

        strncpy(prev_line, line, sizeof(prev_line));
        line_number++;
    }
}

static void handle_text_changed(void* self) {
    AppTab* tab = map_get(self);
    if (!tab)
        return;

    const char* content = q_textedit_to_plain_text(self);
    if (!content)
        return;

    update_outline_for_content(tab, content);
    libqt_free(content);
}

static AppTab* new_app_tab() {
    AppTab* tab = (AppTab*)malloc(sizeof(AppTab));
    if (!tab)
        return NULL;

    tab->tab = q_widget_new2();
    QHBoxLayout* layout = q_hboxlayout_new(tab->tab);

    QSplitter* panes = q_splitter_new2();
    q_hboxlayout_add_widget(layout, panes);

    tab->outline = q_listwidget_new(tab->tab);
    q_splitter_add_widget(panes, tab->outline);
    q_listwidget_on_current_item_changed(tab->outline, handle_jump_to_bookmark);

    tab->textArea = q_textedit_new(tab->tab);
    map_put(tab->textArea, tab);
    map_put(tab->outline, tab);

    q_textedit_on_text_changed(tab->textArea, handle_text_changed);
    q_splitter_add_widget(panes, tab->textArea);

    int size_list[] = {250, 550};
    libqt_list sizes = {
        .len = 2,
        .data.ints = size_list,
    };
    q_splitter_set_sizes(panes, sizes);

    return tab;
}

static void handle_tab_close(void* self, int index) {
    QWidget* widget = q_tabwidget_widget(self, index);

    AppTab* tab_to_free = NULL;

    for (size_t i = 0; i < app_tab_map.size; i++) {
        AppTab* tab = app_tab_map.values[i];
        if (tab->tab == widget) {
            tab_to_free = tab;
            if (tab->textArea)
                map_remove(tab->textArea);
            if (tab->outline)
                map_remove(tab->outline);
            break;
        }
    }

    q_tabwidget_remove_tab(self, index);

    if (tab_to_free) {
        q_widget_delete(tab_to_free->tab);
        free(tab_to_free);
    }
}

static void handle_close_current_tab() {
    if (main_window) {
        int current_index = q_tabwidget_current_index(main_window->tabs);
        if (current_index >= 0) {
            handle_tab_close(main_window->tabs, current_index);
        }
    }
}

static void create_tab_with_contents(AppWindow* window, const char* title, const char* content) {
    AppTab* tab = new_app_tab();
    // the new tab is cleaned up during handle_tab_close
    if (!tab)
        return;

    q_textedit_set_text(tab->textArea, content);

    QIcon* icon = q_icon_from_theme("text-markdown");
    int idx = q_tabwidget_add_tab2(window->tabs, tab->tab, icon, title);
    q_icon_delete(icon);

    q_tabwidget_set_current_index(window->tabs, idx);
}

static void handle_new_tab() {
    create_tab_with_contents(main_window, "New Document", "");
}

static void handle_file_open() {
    const char* fname = q_filedialog_get_open_file_name4(main_window->w, "Open markdown file...", "", "Markdown files (*.md *.txt);;All Files (*)");

    FILE* file = fopen(fname, "r");
    if (!file) {
        libqt_free(fname);
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(file);
        libqt_free(fname);
        return;
    }

    size_t read = fread(content, 1, size, file);
    content[read] = '\0';
    fclose(file);

    const char* basename = strrchr(fname, '/');
    basename = basename ? basename + 1 : fname;

    create_tab_with_contents(main_window, basename, content);

    free(content);
    libqt_free(fname);
}

static AppWindow* new_app_window() {
    AppWindow* window = (AppWindow*)malloc(sizeof(AppWindow));
    if (!window)
        return NULL;

    window->w = q_mainwindow_new2();
    q_mainwindow_set_window_title(window->w, "Markdown Outliner");
    q_mainwindow_resize(window->w, 900, 600);

    QMenuBar* menubar = q_menubar_new2();

    file_menu = q_menubar_add_menu2(menubar, "&File");

    new_action = q_menubar_add_action2(file_menu, "&New Tab");
    QKeySequence* new_shortcut = q_keysequence_new2("Ctrl+N");
    q_action_set_shortcut(new_action, new_shortcut);
    q_keysequence_delete(new_shortcut);
    QIcon* new_icon = q_icon_from_theme("document-new");
    q_action_set_icon(new_action, new_icon);
    q_icon_delete(new_icon);
    q_action_on_triggered(new_action, handle_new_tab);

    open_action = q_menubar_add_action2(file_menu, "&Open...");
    QKeySequence* open_shortcut = q_keysequence_new2("Ctrl+O");
    q_action_set_shortcut(open_action, open_shortcut);
    q_keysequence_delete(open_shortcut);
    QIcon* open_icon = q_icon_from_theme("document-open");
    q_action_set_icon(open_action, open_icon);
    q_icon_delete(open_icon);
    q_action_on_triggered(open_action, handle_file_open);

    q_menubar_add_separator(file_menu);

    exit_action = q_menubar_add_action2(file_menu, "&Exit");
    QKeySequence* exit_shortcut = q_keysequence_new2("Ctrl+Q");
    q_action_set_shortcut(exit_action, exit_shortcut);
    q_keysequence_delete(exit_shortcut);
    QIcon* exit_icon = q_icon_from_theme("application-exit");
    q_action_set_icon(exit_action, exit_icon);
    q_icon_delete(exit_icon);
    q_action_on_triggered(exit_action, q_coreapplication_quit);

    options_menu = q_menubar_add_menu2(menubar, "&Options");

    language_menu = q_menubar_add_menu2(options_menu, "&Language");
    QIcon* language_icon = q_icon_from_theme("preferences-desktop-locale");
    q_menu_set_icon(language_menu, language_icon);
    q_icon_delete(language_icon);
    english_action = q_menubar_add_action2(language_menu, "&English");
    q_action_set_object_name(english_action, "en");
    q_action_on_triggered(english_action, on_triggered);
    french_action = q_menubar_add_action2(language_menu, "&French");
    q_action_set_object_name(french_action, "fr");
    q_action_on_triggered(french_action, on_triggered);
    spanish_action = q_menubar_add_action2(language_menu, "&Spanish");
    q_action_set_object_name(spanish_action, "es");
    q_action_on_triggered(spanish_action, on_triggered);

    q_menubar_add_menu(options_menu, language_menu);

    help_menu = q_menubar_add_menu2(menubar, "&Help");
    about_action = q_menubar_add_action2(help_menu, "&About Qt");
    QIcon* about_icon = q_icon_from_theme("help-about");
    q_action_set_icon(about_action, about_icon);
    q_icon_delete(about_icon);
    QKeySequence* about_shortcut = q_keysequence_new2("F1");
    q_action_set_shortcut(about_action, about_shortcut);
    q_keysequence_delete(about_shortcut);
    q_action_on_triggered(about_action, q_application_about_qt);

    q_mainwindow_set_menu_bar(window->w, menubar);

    const char* close_key = "Ctrl+W";
    QKeySequence* close_shortcut = q_keysequence_new2(close_key);
    QAction* close_action = q_mainwindow_add_action4(window->w, close_key, close_shortcut);
    q_action_set_shortcut(close_action, close_shortcut);
    q_action_on_triggered(close_action, handle_close_current_tab);
    q_keysequence_delete(close_shortcut);

    window->tabs = q_tabwidget_new(window->w);
    q_tabwidget_set_tabs_closable(window->tabs, true);
    q_tabwidget_set_movable(window->tabs, true);
    q_tabwidget_on_tab_close_requested(window->tabs, handle_tab_close);
    q_mainwindow_set_central_widget(window->w, window->tabs);

    const char* readme = "# Markdown Outliner"
                         "\n\nThis is sample markdown content and the outline is generated from it."
                         "\n\n## Features"
                         "\n\n- You can use the outline on the left to jump to parts of the document"
                         "\n- Demonstrates use of the Qt library bindings for C"
                         "\n- There is an option to change the language of the menu options"
                         "\n\n## Exercises (for the adventurous)"
                         "\n\n- Add a new language to the options"
                         "\n- Extend the language support to include the window title, tooltips, and open file dialog"
                         "\n- Port another one of the examples to this repository structure"
                         "\n- Flip the splitter view between the text and the outline"
                         "\n- Change the language options menu to show an indicator for the current language";

    create_tab_with_contents(window, "README.md", readme);

    main_window = window;
    return window;
}

int main(int argc, char* argv[]) {
    QApplication* qapp = q_application_new(&argc, argv);

    map_init(INITIAL_MAP_CAPACITY);

    bool ok = qrc_resource_rcc_resources_data_init();
    if (!ok)
        printf("Resource initialization failed!\n");

    AppWindow* app = new_app_window();
    if (!app) {
        fprintf(stderr, "Failed to create application window\n");
        return 1;
    }

    q_mainwindow_show(app->w);
    int result = q_application_exec();

    ok = qrc_resource_rcc_resources_data_delete();
    if (!ok)
        printf("Resource deinitialization failed!\n");

    map_cleanup();
    free(app);

    q_application_delete(qapp);

    return result;
}
