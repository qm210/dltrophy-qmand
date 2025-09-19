#include <iostream>
#include "QmandApp.h"

int main(int argc, char* argv[]) {

    try {
        Config config(argc, argv);
        QmandApp app(config);
        app.run();
        return 0;

    } catch (const std::system_error& e) {
        std::error_code ec = e.code();
        std::cerr << "ERROR" << ec << ": " << e.what() << std::endl;
        return ec.value();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
