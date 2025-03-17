# RTMP Video Streameer
Base app for streaming IP Cams to RTMP servers without transcoding.  
_Only cams with video stream encoded by h264 codec are supported._

---
### YouTube Live Streamer
Streams IP Cams to YouTube Live.
[![youtube-live-streamer](https://snapcraft.io/youtube-live-streamer/badge.svg)](https://snapcraft.io/youtube-live-streamer)

#### Getting started
1. Install snap: `sudo snap install youtube-live-streamer --edge`
2. Get `stream key` in YouTube Studio (https://studio.youtube.com/channel/UC/livestreaming)
3. Open config file for edit: `sudoedit /var/snap/youtube-live-streamer/common/live-streamer.conf`
4. Replace `source` value with desired RTSP URL
5. Uncomment `youtube-stream-key: "xxxx-xxxx-xxxx-xxxx-xxxx"` line by removing `#` at the beginning and replace `xxxx-xxxx-xxxx-xxxx-xxxx` with your `stream key`
6. Restart snap: `sudo snap restart youtube-live-streamer`

#### Troubleshooting
* Look to the logs with `sudo snap logs youtube-live-streamer` or `sudo snap logs youtube-live-streamer -f`

---
### VK Video Streamer
Streams IP Cams to VK Video.
[![vk-video-streamer](https://snapcraft.io/vk-video-streamer/badge.svg)](https://snapcraft.io/vk-video-streamer)

#### Getting started
1. Install snap: `sudo snap install vk-video-streamer --edge`
2. Get `KEY` in `https://vk.com/video` -> Live -> App
3. Open config file for edit: `sudoedit /var/snap/vk-video-streamer/common/vk-streamer.conf` 
4. Replace `source` value with desired RTSP URL
5. Uncomment `key: "0000000000000_0000000000000_xxxxxxxxxx"` line by removing `#` at the beginning and replace `0000000000000_0000000000000_xxxxxxxxxx` with `KEY`
6. Restart snap: `sudo snap restart vk-video-streamer`

#### Troubleshooting
* Look to the logs with `sudo snap logs vk-video-streamer` or `sudo snap logs vk-video-streamer -f`

---
### Hints
* It's possible to view/start/stop configured video streams on http://localhost:4080 page
