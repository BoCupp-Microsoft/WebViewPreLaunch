# WebView2 Pre-Launch
Pre-launches a WebView2 to accelerate startup of WebView2-based applications.

Link as a library and call the `WebViewPreLauchController::Launch` method as early as possible in the lifecycle of the WebView2-based app.

It relies on a feature of WebView2 which shares a browser process tree for each WebView2 created with the same user data directory.  The shared aspects of the browser process tree include:

* Browser
* Crashpad
* GPU
* Network Utility
* Storage Utility
* Audio Utility

For each new host that shares the user data directory, only a new Renderer process will be created to execute that app's unique JS.  In the browser process, a new HWND will be created where the visuals from that Renderer process can be presented.  That new HWND will be parented to an HWND created by the app host.  This is roughly equivalent to having a browser with multiple "torn off" tabs.

## Browser-startup Args
To share the browser process, we not only need to share the same user data directory but also the parameters used to create the WebView2 environment must be the same.  If the environment arguments are not identitical, the host won't be able to create their own instance of WebView2 to attach to the pre-launched WebView2 process tree.

## Usage
```
auto webview_prelaunch_controller = WebviewPrelaunchController::Launch(args_path);

// Run whatever startup code you want while WV2 starts on a background thread

// Wait for pre-launch to finish so that we can either continue or close it down cleanly.
webview_prelaunch_controller->WaitForPreLaunch();

// Build up new args through whatever mechanism and compare them to what we started our pre-launch with.
auto cached_args = webview_prelaunch_controller->ReadCachedWebviewPrelaunchArguments(args_path);
if (!cached_args.has_value() || args != cachedArgs.value()) {
    // Can't use the pre-launched tree this time.
    // Cache the new args to help the next launch and close the pre-launched processes. 
    webview_prelaunch_controller->CacheWebviewPrelaunchArguments(args_path, args);
    webview_prelaunch_controller->Close(/*wait_for_browser_process_exit*/true);
    webview_prelaunch_controller->WaitForClose();
}

// Create your own WV2

// Call Close at any time after your WV2 controller has been created to start shutting down the background pre-launch thread
webview_prelaunch_controller->Close(/*wait_for_browser_process_exit*/false);

// Call WaitForClose when we are ready to exit the main thread
webview_prelaunch_controller->WaitForClose();
```

## Additional Notes
The profile name which can be specified during controller creation does not need to align with the pre-launched process tree.  The browser process of WebView2 can handle multiple profiles simultaneously.