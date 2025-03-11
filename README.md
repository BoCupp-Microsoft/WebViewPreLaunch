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
To share the browser process, we not only need to share the same user data directory but also the arguments supplied to the browser process via the additional browser args option.  If the arguments are not identitical, the host won't be able to create their own instance of WebView2 to attach to the pre-existing WebView2 process tree.

To synchronize the arguments, the host app must link with webview_prelaunch.lib and call `PreLaunchWebView(browser_args, user_data_dir, profile_name, prelaunch_mode)`.  If a process tree was already hosted with an incompatible set of args, it will be shutdown and a new one launched with the arguments supplied.  The `prelaunch_mode` argument determines if the WebView2 process tree will be created in the calling process, or if a forked process will be started designed to outlive the app host, further reducing the overhead of subsequent launches.

When a host app reaches the point where the webview must be created for it to proceed further, it calls `WaitForPreLaunchWebView(browser_args, user_data_dir, profile_name, prehost_webview_token)` passing the token previously returned by the `PreLaunchWebView` call.  After this method returns the app host creates its environment and webview controller in the usual way.

The `WaitForPreLaunchWebView` will block ensuring the process tree has been successfully created.  It will also check the arguments in case further host app processing has determined they should be different from what was previously cached.  If the arguments have changed, a performance penalty will be incurred since we'll not only be creating the browser process tree as we would have without this library's help, but we'll also first wait for the pre-launched tree to be shutdown.

