[![vk-video-streamer](https://snapcraft.io/vk-video-streamer/badge.svg)](https://snapcraft.io/vk-video-streamer)

# VK Video Streamer
Streams IP Cam to VK Video

## Getting started
1. Install snap: `sudo snap install vk-video-streamer --edge`
2. Get `KEY` in `https://vk.com/video` -> Live -> App
3. Open config file for edit: `sudoedit /var/snap/vk-video-streamer/common/vk-streamer.conf` 
4. Replace `source` value with desired RTSP URL
5. Uncomment `key: "0000000000000_0000000000000_xxxxxxxxxx"` line by removing `#` at the beginning and replace `0000000000000_0000000000000_xxxxxxxxxx` with `KEY`
6. Restart snap: `sudo snap restart vk-video-streamer`

## Troubleshooting
* Look to the logs with `sudo snap logs vk-video-streamer` or `sudo snap logs vk-video-streamer -f`
