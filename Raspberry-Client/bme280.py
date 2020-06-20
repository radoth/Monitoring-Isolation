#!/usr/bin/python
import smbus
import time
from ctypes import c_short
from ctypes import c_byte
from ctypes import c_ubyte
import datetime
import urllib2

DEVICE = 0x77 # Default device I2C address


bus = smbus.SMBus(1) # Rev 2 Pi, Pi 2 & Pi 3 uses bus 1
                     # Rev 1 Pi uses bus 0

def getShort(data, index):
  # return two bytes from data as a signed 16-bit value
  return c_short((data[index+1] << 8) + data[index]).value

def getUShort(data, index):
  # return two bytes from data as an unsigned 16-bit value
  return (data[index+1] << 8) + data[index]

def getChar(data,index):
  # return one byte from data as a signed char
  result = data[index]
  if result > 127:
    result -= 256
  return result

def getUChar(data,index):
  # return one byte from data as an unsigned char
  result =  data[index] & 0xFF
  return result

def readBME280ID(addr=DEVICE):
  # Chip ID Register Address
  REG_ID     = 0xD0
  (chip_id, chip_version) = bus.read_i2c_block_data(addr, REG_ID, 2)
  return (chip_id, chip_version)

def readBME280All(addr=DEVICE):
  # Register Addresses
  REG_DATA = 0xF7
  REG_CONTROL = 0xF4
  REG_CONFIG  = 0xF5

  REG_CONTROL_HUM = 0xF2
  REG_HUM_MSB = 0xFD
  REG_HUM_LSB = 0xFE

  # Oversample setting - page 27
  OVERSAMPLE_TEMP = 2
  OVERSAMPLE_PRES = 2
  MODE = 1

  # Oversample setting for humidity register - page 26
  OVERSAMPLE_HUM = 2
  bus.write_byte_data(addr, REG_CONTROL_HUM, OVERSAMPLE_HUM)

  control = OVERSAMPLE_TEMP<<5 | OVERSAMPLE_PRES<<2 | MODE
  bus.write_byte_data(addr, REG_CONTROL, control)

  # Read blocks of calibration data from EEPROM
  # See Page 22 data sheet
  cal1 = bus.read_i2c_block_data(addr, 0x88, 24)
  cal2 = bus.read_i2c_block_data(addr, 0xA1, 1)
  cal3 = bus.read_i2c_block_data(addr, 0xE1, 7)

  # Convert byte data to word values
  dig_T1 = getUShort(cal1, 0)
  dig_T2 = getShort(cal1, 2)
  dig_T3 = getShort(cal1, 4)

  dig_P1 = getUShort(cal1, 6)
  dig_P2 = getShort(cal1, 8)
  dig_P3 = getShort(cal1, 10)
  dig_P4 = getShort(cal1, 12)
  dig_P5 = getShort(cal1, 14)
  dig_P6 = getShort(cal1, 16)
  dig_P7 = getShort(cal1, 18)
  dig_P8 = getShort(cal1, 20)
  dig_P9 = getShort(cal1, 22)

  dig_H1 = getUChar(cal2, 0)
  dig_H2 = getShort(cal3, 0)
  dig_H3 = getUChar(cal3, 2)

  dig_H4 = getChar(cal3, 3)
  dig_H4 = (dig_H4 << 24) >> 20
  dig_H4 = dig_H4 | (getChar(cal3, 4) & 0x0F)

  dig_H5 = getChar(cal3, 5)
  dig_H5 = (dig_H5 << 24) >> 20
  dig_H5 = dig_H5 | (getUChar(cal3, 4) >> 4 & 0x0F)

  dig_H6 = getChar(cal3, 6)

  # Wait in ms (Datasheet Appendix B: Measurement time and current calculation)
  wait_time = 1.25 + (2.3 * OVERSAMPLE_TEMP) + ((2.3 * OVERSAMPLE_PRES) + 0.575) + ((2.3 * OVERSAMPLE_HUM)+0.575)
  time.sleep(wait_time/1000)  # Wait the required time  

  # Read temperature/pressure/humidity
  data = bus.read_i2c_block_data(addr, REG_DATA, 8)
  pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4)
  temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4)
  hum_raw = (data[6] << 8) | data[7]

  #Refine temperature
  var1 = ((((temp_raw>>3)-(dig_T1<<1)))*(dig_T2)) >> 11
  var2 = (((((temp_raw>>4) - (dig_T1)) * ((temp_raw>>4) - (dig_T1))) >> 12) * (dig_T3)) >> 14
  t_fine = var1+var2
  temperature = float(((t_fine * 5) + 128) >> 8);

  # Refine pressure and adjust for temperature
  var1 = t_fine / 2.0 - 64000.0
  var2 = var1 * var1 * dig_P6 / 32768.0
  var2 = var2 + var1 * dig_P5 * 2.0
  var2 = var2 / 4.0 + dig_P4 * 65536.0
  var1 = (dig_P3 * var1 * var1 / 524288.0 + dig_P2 * var1) / 524288.0
  var1 = (1.0 + var1 / 32768.0) * dig_P1
  if var1 == 0:
    pressure=0
  else:
    pressure = 1048576.0 - pres_raw
    pressure = ((pressure - var2 / 4096.0) * 6250.0) / var1
    var1 = dig_P9 * pressure * pressure / 2147483648.0
    var2 = pressure * dig_P8 / 32768.0
    pressure = pressure + (var1 + var2 + dig_P7) / 16.0

  # Refine humidity
  humidity = t_fine - 76800.0
  humidity = (hum_raw - (dig_H4 * 64.0 + dig_H5 / 16384.0 * humidity)) * (dig_H2 / 65536.0 * (1.0 + dig_H6 / 67108864.0 * humidity * (1.0 + dig_H3 / 67108864.0 * humidity)))
  humidity = humidity * (1.0 - dig_H1 * humidity / 524288.0)
  if humidity > 100:
    humidity = 100
  elif humidity < 0:
    humidity = 0

  return temperature/100.0,pressure/100.0,humidity

def main():

  (chip_id, chip_version) = readBME280ID()
  temperature,pressure,humidity = readBME280All()
  fileTemp=r'/var/www/html/weatherOut.txt'
  with open(fileTemp,'a') as ft:
      ft.write(datetime.datetime.now().strftime('%F %T')+' '+str(temperature)+' '+str(pressure)+' '+str(humidity)+'\n')
  rmtmsg=urllib2.urlopen('http://remoteaddress/remote.txt')
  rmtdta=rmtmsg.read()
  rmtspeak=rmtdta[2:]
  
  file1=r'/home/pi/mobile/1.txt'
  with open(file1,'r') as file1c:
      file1s=file1c.read()
      
  file2=r'/home/pi/mobile/2.txt'
  with open(file2,'r') as file2c:
      file2s=file2c.read()
      
  file3=r'/home/pi/mobile/3.txt'
  with open(file3,'r') as file3c:
      file3s=file3c.read()
      
  file4=r'/home/pi/mobile/4.txt'
  with open(file4,'r') as file4c:
      file4s=file4c.read()
      
  file5=r'/home/pi/mobile/5.txt'
  with open(file5,'r') as file5c:
      file5s=file5c.read()
      
  file6=r'/home/pi/mobile/6.txt'
  with open(file6,'r') as file6c:
      file6s=file6c.read()
      
  file7=r'/home/pi/mobile/7.txt'
  with open(file7,'r') as file7c:
      file7s=file7c.read()
      
  file8=r'/home/pi/mobile/8.txt'
  with open(file8,'r') as file8c:
      file8s=file8c.read()
      
  file9=r'/home/pi/mobile/9.txt'
  with open(file9,'r') as file9c:
      file9s=file9c.read()
      
  file10=r'/home/pi/mobile/10.txt'
  with open(file10,'r') as file10c:
      file10s=file10c.read()
      
  file11=r'/home/pi/mobile/11.txt'
  with open(file11,'r') as file11c:
      file11s=file11c.read()
      
  webpage=r'/var/www/html/mobile/web.html'
  with open(webpage,'w') as webpagec:
      webpagec.write(file1s+str(temperature)+file2s+str(temperature)+file3s+str(temperature)+file4s+str(humidity)+file5s+str(humidity)+file6s+str(humidity)+file7s+str(pressure)+file8s+str(pressure/10)+file9s+str(pressure/10)+file10s+rmtspeak+file11s)
      
  

  #print "Temperature : ", temperature, "C"
  #print "Pressure : ", pressure, "hPa"
  #print "Humidity : ", humidity, "%"

if __name__=="__main__":
   main()
