//filename: defaultprefs.h

// Default preferences in raw data format for PROGMEM
//
const char defprefs_txt[] PROGMEM = R"=====(
alrm_start = none     # Format: hh:min 12:30
alrm_stop = none      # Format: hh:min 13:00

# Example configuration
# Programmable input pins:
gpio_00 = uppreset = 1
gpio_12 = upvolume = 2
gpio_13 = downvolume = 2
gpio_14 = stop
gpio_17 = resume
gpio_34 = station = icecast.omroep.nl:80/radio1-bb-mp3
#
# Enter your WiFi network specs here: {NetworkName}/{Password}
# 00=Prio1, 01=Prio2 (optional)
wifi_00 = SSID1/PASSWD1
wifi_01 = SSID2/PASSWD2
#
volume = 72
toneha = 0
tonehf = 0
tonela = 0
tonelf = 0
#
preset = 6
preset_00 = charthits-high.rautemusik.fm                    # 0 - Rautemusik
preset_01 = stream.sunshine-live.de/live/mp3-192            # 1 - Sunshine Live
preset_02 = listen.technobase.fm/tunein-mp3                 # 2 - Technobase.FM (HandsUp & Dance)
preset_03 = stream.slam.nl/slam_mp3                         # 3 - SLAM Live
preset_04 = ic.radiomonster.fm/dance.ultra                  # 4 - RadioMonster Dance
preset_05 = ice1.somafm.com/groovesalad-128-mp3             # 5 - SomaFM
preset_06 = dancefm.net/listen.m3u                          # 6 - Dance FM (Global/US-Style)
preset_07 = stream.antenne1.de/a1stg/livestream2.mp3        # 7 - Antenne 1
preset_08 = topradio-de-hz-fal-stream03-cluster01.radiohost.de/kissfm_128     # 8 - Kiss FM
preset_09 = streamhq.top100station.com/top100station-high.mp3                 # 9 - TOP 100 Station
preset_10 = stream.antenne1.de/80er/livestream2.mp3                           # 10 - Antenne 1 80ger
preset_11 = play.antenne.de/dance-xxl.m3u                                     # 11 - Antenne 1 Dance
preset_12 = liveradio.swr.de/sw331ch/swr3/play.aac                            # 12 - SWR3
#
# Clock offset and daylight saving time
clk_server = pool.ntp.org                            # Time server to be used
clk_offset = 1                                       # Offset with respect to UTC in hours
clk_dst = 1                                          # Offset during daylight saving time (hours)
# Some IR codes
ir_40BF = upvolume = 2
ir_C03F = downvolume = 2
)=====" ;
