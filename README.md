# WebView2 Pre-Launch
This project pre-launches a WebView2 to accelerate startup of WebView2-based applications.

It can run as a process to keep a browser process tree warm for the next launch of a WebView2-based app or be linked as a library to be called as early as possible in the lifecycle of the WebView2-based app.

It relies on a feature of WebView2 which shares a browser process tree for each WebView2 created with the same user data directory.  The shared aspects of the browser process tree include:

* Browser
* Crashpad
* GPU
* Network Utility
* Storage Utility
* Audio Utility

For each new host that shares the user data directory, only a new Renderer process will be created to execute that app's unique JS.  In the browser process, a new HWND will be created where the visuals from that Renderer process can be presented.  That new HWND will be parented to an HWND created by the app host.  This is roughly equivalent to having a browser with multiple "torn off" tabs.

## Browser-startup Args
To share the browser process, we not only need to share the same user data directory but also the arguments supplied to the browser process via the additional browser args option.  If the environment arguments are not identitical, the host won't be able to create their own instance of WebView2 to attach to the pre-existing WebView2 process tree.

## API Thoughts
```
class WebViewPreLaunchController {
    // Starts webview launch on a background thread.
    static shared_ptr<WebViewPreLaunchController> Launch(args_path);

    virtual WebViewCreationArguments ReadCachedWebViewPreLaunchArguments(std::string args_path);
    virtual void CacheWebViewPreLaunchArguments(std::string args_path, WebViewCreationArguments args);

    // Blocks while pre-launch webview is closed and background thread exits.
    virtual void Close();

    // Blocks until pre-launch is completed (or until it errors out)
    virtual void WaitForPreLaunch();
};
```


Host app uses the API as follows:
```
auto webview_prelaunch_controller = WebViewPreLaunchController::Launch(args_path);

auto cachedArgs = webview_prelaunch_controller->ReadCachedWebViewPreLaunchArguments(args_path);

// Build up new args

if (args != cachedArgs) {
    webview_prelaunch_controller->CacheWebViewPreLaunchArguments(args_path, args);
    webview_prelaunch_controller->Close();
}

webview_prelaunch_controller->WaitForPreLaunch();

// Create your own WV2
```

