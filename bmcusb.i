/*
 * bmcusb.i -
 *
 * Control Boston Micromachines Corporation (BMC) Multi-DM deformable mirror
 * system.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (c) 2014 Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>.
 * All rights reserved.
 */

require, "usb.i";

/* From CIUsbShared */
eCIUsbCmndGetFirmwareVer     = 0xF0;
eCIUsbCmndSetGetRam          = 0xF1;
eCIUsbCmndSetGetCodeEeprom   = 0xF2;
eCIUsbCmndResetAll           = 0xF3;
eCIUsbCmndGetStatusBits      = 0xF4; /* bit[0]=FrameErr, bit[1]=RdHvaE,
                                      * bit[2]=CableOk,
                                      * bit[9]=ExEepromPresent */
eCIUsbCmndSetControlBits     = 0xF5; /* bit[7]=Set/Reset, bit[1]=Freset,
                                      * bit[2]=FrameSync, bit[3]=HvEnab */
eCIUsbCmndSetGetCodeExEeprom = 0xF6;
eCIUsbCmndSetDac             = 0xF7;

BMCUSB_ENDPOINT = 2;
BMCUSB_VENDOR = 0x1781;
BMCUSB_MULTIDRIVER = 0x0ED8;

func bmcusb_probe
/* DOCUMENT bmcusb_probe;

     This subroutine probe the available USB devices for controlling Boston
     Micromachines Corporation (BMC) Multi-DM deformable mirror system.

   SEE ALSO: bmcusb_open.
 */
{
  extern BMCUSB_DEVICES;

  // Get available USB devices:
  descr = usb_probe_devices();

  // Select the ones matching vendor and product identifiers:
  sel = where((descr.vendor == BMCUSB_VENDOR)&
              (descr.product == BMCUSB_MULTIDRIVER));
  if (! is_array(sel)) error, "no USB devices for BMC Multi-DM";
  n = numberof(sel);
  write, format="Found %d BMC Multi-DM device%s\n", n, (n > 1 ? "s" : "");
  BMCUSB_DEVICES = descr(sel);
}

func bmcusb_open(j)
/* DOCUMENT dev = bmcusb_open(j);
         or dev = bmcusb_open();

     This function opens the j-th BMC device and returns a handle to it.  By
     default, the first available device is opened.  The list of available
     devices is based on the last call to bmcusb_probe.  If the list is empty,
     bmcusb_probe is automatically called.  You can also call bmcusb_probe
     before bmcusb_open to refresh the list.

   SEE ALSO: bmcusb_probe.
 */
{
  extern BMCUSB_DEVICES;
  if (is_void(BMCUSB_DEVICES)) {
    bmcusb_probe;
  }
  if (is_void(j)) {
    j = 1;
  }
  return usb_open_device(BMCUSB_DEVICES(j).bus,
                         BMCUSB_DEVICES(j).port);
}

func bmcusb_control(dev, request, send, value, index, data, lenght, timeout)
/* DOCUMENT ret = bmcusb_control(dev, type, send, value, index,
                                 data, length, timeout);

     Arguments DATA, LENGTH and TIMEOUT are optional their defaults are:
     DATA=[] (empty), LENGTH=sizeof(DATA) and TIMEOUT=1000 (one second).

   SEE ALSO: bmcusb_open, usb_control_transfer.
 */
{
  type = (send ? 0x40 : 0xC0); // request type
  if (is_void(length)) length = sizeof(data);
  if (is_void(timeout)) timeout = 1000; // one second default timeout
  ret = usb_control_transfer(dev, type, request, value, index,
                             data, length, timeout);
  if (ret < 0 && am_subroutine()) {
    usb_error, "usb_control_transfer failed", ret;
  }
  return ret;
}

func _bmcusb_set(dev, value)
{
  bmcusb_control, dev, eCIUsbCmndSetControlBits, 1n, 0, value;
}

func bmcusb_reset(dev, assert)
{
  _bmcusb_set, dev, (assert ? 0x0082 : 0x0002);
}

func bmcusb_deassertReset(dev)
{
   _bmcusb_set, dev, 0x0002;
}

func bmcusb_reset(dev){
  DEBUG_BLURB
  if(BMCUSB_DEBUG) printf("\nDEBUG: RESET.\n");

  int ret;
  char goo[8]; // is this needed???

  _bmcusb_set, dev,0x0002, goo, 0);
  if(ret) return ret;

  _bmcusb_set, dev,0x0082, goo, 0);

  return ret;
}

int bmcusb_setHV(int nDevId, int ON){
  int ret;
  DEBUG_BLURB

  if(ON) // Turn ON the High Voltage Power to the DM...
  {
    if(BMCUSB_DEBUG) printf("\nDEBUG: Turning HV ON.\n");
      // usleep(250000); // I think this is just for the MINI, but I am playing it safe.
    _bmcusb_set, dev,0x0088, NULL, 0);
  }
  else  // Turn OFF the High Voltage Power to the DM...
  {
    if(BMCUSB_DEBUG) printf("\nDEBUG: Turning HV OFF.\n");
    _bmcusb_set, dev,0x0008, NULL, 0);
  }
  return ret;
}

func bmcusb_setFrameSync(dev, assert)
{
  _bmcusb_set, dev, (assert ? 0x0084 : 0x0004);
}

// I have **NO IDEA** what this means!
func bmcusb_setLVShdn(dev, assert)
{
  _bmcusb_set, dev, (assert ? 0x0090 : 0x0010);
}

func bmcusb_setExtI2C(dev, assert)
{
  _bmcusb_set, dev, (assert ? 0x00A0 : 0x0020);
}

int bmcusb_zeroDM(int nDevId){
  DEBUG_BLURB
  return bmcusb_constantDM(nDevId, 0);
}
