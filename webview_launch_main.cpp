#include <iostream>
#include <thread>
#include "webview_launch.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    //webViewLaunch(nullptr);
    std::thread webview_launch_thread(webViewLaunch, nullptr);
    webview_launch_thread.join();
    return 0;
}
