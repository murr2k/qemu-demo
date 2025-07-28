# RP2040 SoC and board support
rp2040_ss = ss.source_set()
rp2040_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040.c',
  'raspberrypi-pico.c',
))

hw_arch += {'arm': rp2040_ss}