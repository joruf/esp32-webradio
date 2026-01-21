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
# Some preset examples
preset_00 = charthits-high.rautemusik.fm                                                    #  0 - Rautemusik
preset_01 = liveradio.swr.de/sw282p3/swr3/play.mp3                                          #  1 - SWR3
preset_02 = dg-swr-https-fra-dtag-cdn.sslcast.addradio.de/swr/swr/raka04/mp3/128/stream.mp3 #  2 - SWR3 Party
preset_03 = stream.antenne1.de/a1stg/livestream2.mp3                                        #  3 - Antenne 1
preset_04 = topradio-de-hz-fal-stream03-cluster01.radiohost.de/kissfm_128                   #  4 - Kiss FM
preset_05 = streamhq.top100station.com/top100station-high.mp3                               #  5 - Top 100 Station
preset_06 = blackblack.stream.laut.fm/blackblack                                            #  6 - Black
preset_07 = de-hz-fal-stream06.rautemusik.fm/blackbeats                                     #  7 - Black Stream
preset_08 = stream.antenne1.de/80er/livestream2.mp3                                         #  8 - Antenne 1 80ger
preset_09 = dg-wdr-https-dus-dtag-cdn.sslcast.addradio.de/wdr/1live/live/mp3/128/stream.mp3 #  9 - 1.Live
preset_10 = http://185.52.127.159/de/33001/mp3_128.mp3                                      # 10 - Energy Berlin
preset_11 = http://185.52.127.170/de/33005/mp3_128.mp3                                      # 11 - Energy Stuttgart
#
# Clock offset and daylight saving time
clk_server = pool.ntp.org                            # Time server to be used
clk_offset = 1                                       # Offset with respect to UTC in hours
clk_dst = 1                                          # Offset during daylight saving time (hours)
# Some IR codes
ir_40BF = upvolume = 2
ir_C03F = downvolume = 2
)=====" ;
