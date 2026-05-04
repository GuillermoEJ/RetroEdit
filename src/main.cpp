#include "editor.h"
#include "terminal.h"
#include "config.h"
#include <string>

int main(int argc, char* argv[]) {
    retro::Config::instance().load();
    retro::Editor editor;

    if (argc >= 2) {
        editor.open(argv[1]);
    }

    editor.run();
    return 0;
}
