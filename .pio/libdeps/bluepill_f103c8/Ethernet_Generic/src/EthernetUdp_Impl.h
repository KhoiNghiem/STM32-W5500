/****************************************************************************************************************************
  EthernetUdp_Impl.h - Driver for W5x00

  Ethernet_Generic is a library for the W5x00 Ethernet shields trying to merge the good features of
  previous Ethernet libraries

  Based on and modified from

  1) Ethernet Library         https://github.com/arduino-libraries/Ethernet
  2) EthernetLarge Library    https://github.com/OPEnSLab-OSU/EthernetLarge
  3) Ethernet2 Library        https://github.com/adafruit/Ethernet2
  4) Ethernet3 Library        https://github.com/sstaub/Ethernet3

  Built by Khoi Hoang https://github.com/khoih-prog/EthernetWebServer

  Version: 2.8.1

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  2.0.0   K Hoang      31/03/2022 Initial porting and coding to support SPI2, debug, h-only library
  ...
  2.6.0   K Hoang      11/09/2022 Add support to AVR Dx (AVR128Dx, AVR64Dx, AVR32Dx, etc.) using DxCore
  2.6.1   K Hoang      23/09/2022 Fix bug for W5200
  2.6.2   K Hoang      26/10/2022 Add support to Seeed XIAO_NRF52840 and XIAO_NRF52840_SENSE using `mbed` or `nRF52` core
  2.7.0   K Hoang      14/11/2022 Fix severe limitation to permit sending larger data than 2/4/8/16K buffer
  2.7.1   K Hoang      15/11/2022 Auto-detect W5x00 and settings to set MAX_SIZE to send
  2.8.0   K Hoang      27/12/2022 Add support to W6100 using IPv4
  2.8.1   K Hoang      06/01/2023 Fix W6100 minor bug
 *****************************************************************************************************************************/
/*
    Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
    This version only offers minimal wrapping of socket.cpp
    Drop Udp.h/.cpp into the Ethernet library directory at hardware/libraries/Ethernet/

   MIT License:
   Copyright (c) 2008 Bjoern Hartmann
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

   bjoern@cs.stanford.edu 12/30/2008
*/

#pragma once

#ifndef ETHERNET_UDP_GENERIC_IMPL_H
#define ETHERNET_UDP_GENERIC_IMPL_H

#include <Arduino.h>
#include "Ethernet_Generic.hpp"

#include "Dns.h"

#include "utility/w5100.h"

/////////////////////////////////////////////////////////

/* Start EthernetUDP socket, listening at local port PORT */
uint8_t EthernetUDP::begin(uint16_t port)
{
  if (_sockindex < MAX_SOCK_NUM)
    Ethernet.socketClose(_sockindex);

  _sockindex = Ethernet.socketBegin(SnMR::UDP, port);

  if (_sockindex >= MAX_SOCK_NUM)
    return 0;

  _port = port;
  _remaining = 0;

  return 1;
}

/////////////////////////////////////////////////////////

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int EthernetUDP::available()
{
  return _remaining;
}

/////////////////////////////////////////////////////////

/* Release any resources being used by this EthernetUDP instance */
void EthernetUDP::stop()
{
  if (_sockindex < MAX_SOCK_NUM)
  {
    Ethernet.socketClose(_sockindex);
    _sockindex = MAX_SOCK_NUM;
  }
}

/////////////////////////////////////////////////////////

int EthernetUDP::beginPacket(const char *host, uint16_t port)
{
  // Look up the host first
  int ret = 0;
  DNSClient dns;
  IPAddress remote_addr;

  dns.begin(Ethernet.dnsServerIP());
  ret = dns.getHostByName(host, remote_addr);

  if (ret != 1)
    return ret;

  return beginPacket(remote_addr, port);
}

/////////////////////////////////////////////////////////

int EthernetUDP::beginPacket(IPAddress ip, uint16_t port)
{
  _offset = 0;

  ETG_LOGDEBUG("EthernetUDP:beginPacket");

  return Ethernet.socketStartUDP(_sockindex, rawIPAddress(ip), port);
}

/////////////////////////////////////////////////////////

int EthernetUDP::endPacket()
{
  return Ethernet.socketSendUDP(_sockindex);
}

/////////////////////////////////////////////////////////

size_t EthernetUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

/////////////////////////////////////////////////////////

size_t EthernetUDP::write(const uint8_t *buffer, size_t size)
{
  ETG_LOGDEBUG3("EthernetUDP:write, buffer =", (char *) buffer, ", size =", size);

  uint16_t bytes_written = Ethernet.socketBufferData(_sockindex, _offset, buffer, size);
  _offset += bytes_written;

  return bytes_written;
}

/////////////////////////////////////////////////////////

int EthernetUDP::parsePacket()
{
  // discard any remaining bytes in the last packet
  while (_remaining)
  {
    // could this fail (loop endlessly) if _remaining > 0 and recv in read fails?
    // should only occur if recv fails after telling us the data is there, lets
    // hope the w5100 always behaves :)
    read((uint8_t *) NULL, _remaining);
  }

  ////////////////////////////////////////

#if USING_W6100

  if (Ethernet.socketRecvAvailable(_sockindex) > 0)
  {
    //HACK - hand-parse the UDP packet using TCP recv method
    uint8_t tmpBuf[20];
    int ret = 0;

    if (W5100.getChip() == w6100)
    {
      //read 2 header bytes and get one IPv4 or IPv6
      ret = Ethernet.socketRecv(_sockindex, tmpBuf, 2);

      if (ret > 0)
      {
        _remaining = (tmpBuf[0] & (0x7)) << 8 | tmpBuf[1];

        if ((tmpBuf[0] & W6100_UDP_HEADER_IPV) == W6100_UDP_HEADER_IPV6)
        {
          // IPv6 UDP Recived
          // 0 1
          // 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
          // 18 19

          //read 16 header bytes and get IP and port from it
          ret = Ethernet.socketRecv(_sockindex, &tmpBuf[2], 18);
          _remoteIP = &tmpBuf[2];
          _remotePort = (tmpBuf[18] << 8) | tmpBuf[19];
        }
        else
        {
          // IPv4 UDP Recived
          // 0 1
          // 2 3 4 5
          // 6 7

          //read 6 header bytes and get IP and port from it
          ret = Ethernet.socketRecv(_sockindex, &tmpBuf[2], 6);
          _remoteIP = &tmpBuf[2];
          _remotePort = (tmpBuf[6] << 8) | tmpBuf[7];
        }

        ret = _remaining;
      }
    }
    else
    {
      //read 8 header bytes and get IP and port from it
      ret = Ethernet.socketRecv(_sockindex, tmpBuf, 8);

      if (ret > 0)
      {
        _remoteIP = tmpBuf;
        _remotePort = tmpBuf[4];
        _remotePort = (_remotePort << 8) + tmpBuf[5];
        _remaining = tmpBuf[6];
        _remaining = (_remaining << 8) + tmpBuf[7];

        // When we get here, any remaining bytes are the data
        ret = _remaining;
      }
    }

    return ret;
  }

  ////////////////////////////////////////

#else   // USING_W6100

  if (Ethernet.socketRecvAvailable(_sockindex) > 0)
  {
    //HACK - hand-parse the UDP packet using TCP recv method
    uint8_t tmpBuf[8];
    int ret;

    //read 8 header bytes and get IP and port from it
    ret = Ethernet.socketRecv(_sockindex, tmpBuf, 8);

    if (ret > 0)
    {
      _remoteIP = tmpBuf;
      _remotePort = tmpBuf[4];
      _remotePort = (_remotePort << 8) + tmpBuf[5];
      _remaining = tmpBuf[6];
      _remaining = (_remaining << 8) + tmpBuf[7];

      // When we get here, any remaining bytes are the data
      ret = _remaining;
    }

    return ret;
  }

#endif    // USING_W6100

  // There aren't any packets available
  return 0;
}

/////////////////////////////////////////////////////////

int EthernetUDP::read()
{
  uint8_t byte;

  if ((_remaining > 0) && (Ethernet.socketRecv(_sockindex, &byte, 1) > 0))
  {
    // We read things without any problems
    _remaining--;

    return byte;
  }

  // If we get here, there's no data available
  return -1;
}

/////////////////////////////////////////////////////////

int EthernetUDP::read(unsigned char *buffer, size_t len)
{
  if (_remaining > 0)
  {
    int got;

    if (_remaining <= len)
    {
      // data should fit in the buffer
      got = Ethernet.socketRecv(_sockindex, buffer, _remaining);
    }
    else
    {
      // too much data for the buffer,
      // grab as much as will fit
      got = Ethernet.socketRecv(_sockindex, buffer, len);
    }

    if (got > 0)
    {
      _remaining -= got;

      ETG_LOGDEBUG3("EthernetUDP:read, buffer =", (char *) buffer, ", got =", got);

      return got;
    }
  }

  // If we get here, there's no data available or recv failed
  return -1;
}

/////////////////////////////////////////////////////////

int EthernetUDP::peek()
{
  // Unlike recv, peek doesn't check to see if there's any data available, so we must.
  // If the user hasn't called parsePacket yet then return nothing otherwise they
  // may get the UDP header
  if (_sockindex >= MAX_SOCK_NUM || _remaining == 0)
    return -1;

  return Ethernet.socketPeek(_sockindex);
}

/////////////////////////////////////////////////////////

void EthernetUDP::flush()
{
  // TODO: we should wait for TX buffer to be emptied
}

/////////////////////////////////////////////////////////

/* Start EthernetUDP socket, listening at local port PORT */
uint8_t EthernetUDP::beginMulticast(IPAddress ip, uint16_t port)
{
  if (_sockindex < MAX_SOCK_NUM)
    Ethernet.socketClose(_sockindex);

  _sockindex = Ethernet.socketBeginMulticast(SnMR::UDP | SnMR::MULTI, ip, port);

  if (_sockindex >= MAX_SOCK_NUM)
    return 0;

  _port = port;
  _remaining = 0;

  return 1;
}

/////////////////////////////////////////////////////////

#endif  // ETHERNET_UDP_GENERIC_IMPL_H
