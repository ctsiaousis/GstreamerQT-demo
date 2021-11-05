# GstTest

This project demonstrates the possibilities and modularity of GStreamer with QT integration.

It is a windows build with Qt 5.15.2 and MSVC2019 64bit compiler. The GST library is version 1.18.5.


## how to test

#### Stream with `gst-launch videotestsrc`
- Open a powershell window and write
```
gst-launch-1.0 videotestsrc ! video/x-raw,width=3040,height=2048 ! videoconvert ! x264enc ! rtph264pay config-interval=1 ! udpsink host=127.0.0.1 port=5000
```
- Open a second powershell window and write the same command with `port = 5001`, you can also provide a different video i.e. `... videotestsrc pattern=ball ! ...`

- You can also try streaming from any network device by using the **broadcast ip** which is usually (if subnet mask is 255.255.255.0) `*.*.*.255`.

*Note:* 
This generates CPU accelerated `videotestsrc`. If you want/need GPU acceleration for the h264 encoding use either: `omxh264enc` or `d3d11h264dec`, depending on availability and your hardware.

#### Stream with `GstTestServer/GstTestServer.pro`
Build the server project and run it.

___
#### Build the `GstTest.pro`
and test your configuration. Once you are happy with it, you can stream different types of media (i.e. files, webstreams, v4l devices and more).